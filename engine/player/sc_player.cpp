// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <cerrno>

#include "simulationcraft.hpp"

namespace {

// Player Ready Event =======================================================

struct player_ready_event_t : public player_event_t
{
  player_ready_event_t( player_t& p,
                        timespan_t delta_time ) :
                          player_event_t( p, delta_time )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New Player-Ready Event: %s", p.name() );
  }
  virtual const char* name() const override
  { return "Player-Ready"; }
  virtual void execute() override
  {
    // There are certain chains of events where an off-gcd ability can be queued such that the queue
    // time for the action exceeds Player-Ready event (essentially end of GCD). In this case, the
    // simple solution is to just cancel the queue execute and let the actor select an action from
    // the action list as normal.
    if ( p() -> queueing )
    {
      event_t::cancel( p() -> queueing -> queue_event );
      p() -> queueing = nullptr;
    }
    // Player that's checking for off gcd actions to use, cancels that checking when there's a ready event firing.
    event_t::cancel( p() -> off_gcd );

    if ( ! p() -> execute_action() )
    {
      if ( p() -> ready_type == READY_POLL )
      {
        timespan_t x = p() -> available();

        p() -> schedule_ready( x, true );

        // Waiting Debug
        if ( sim().debug )
          sim().out_debug.printf( "%s is waiting for %.4f resource=%.2f",
                      p() -> name(), x.total_seconds(),
                      p() -> resources.current[ p() -> primary_resource() ] );
      }
      else
      {
        p() -> started_waiting = sim().current_time();
        p() -> min_threshold_trigger();
      }
    }
  }
};

// Trigger a wakeup based on a resource treshold. Only used by trigger_ready=1 actors
struct resource_threshold_event_t : public event_t
{
  player_t* player;

  resource_threshold_event_t( player_t* p, const timespan_t& delay ) :
    event_t( *p, delay ), player( p )
  {
  }

  const char* name() const override
  { return "Resource-Threshold"; }

  void execute() override
  {
    player -> trigger_ready();
    player -> resource_threshold_trigger = 0;
  }
};

// sorted_action_priority_lists =============================================

// APLs need to always be initialized in the same order, otherwise copy= profiles may break in some
// cases. Order will be: precombat -> default -> alphabetical list of custom apls
std::vector<action_priority_list_t*> sorted_action_priority_lists( const player_t* p )
{
  std::vector<action_priority_list_t*> apls = p -> action_priority_list;
  range::sort( apls, []( const action_priority_list_t* l, const action_priority_list_t* r ) {
    if ( l -> name_str == "precombat" && r -> name_str != "precombat" )
    {
      return true;
    }
    else if ( l -> name_str != "precombat" && r -> name_str == "precombat" )
    {
      return false;
    }
    else if ( l -> name_str == "default" && r -> name_str != "default" )
    {
      return true;
    }
    else if ( l -> name_str != "default" && r -> name_str == "default" )
    {
      return false;
    }
    else
    {
      return l -> name_str < r -> name_str;
    }
  } );

  return apls;
}

// has_foreground_actions ===================================================

bool has_foreground_actions( const player_t& p )
{
  return ( p.active_action_list && !p.active_action_list -> foreground_action_list.empty() );
}

// parse_talent_url =========================================================

bool parse_talent_url( sim_t* sim,
                       const std::string& name,
                       const std::string& url )
{
  assert( name == "talents" ); ( void )name;

  player_t* p = sim -> active_player;

  p -> talents_str = url;

  std::string::size_type cut_pt = url.find( '#' );

  if ( cut_pt != url.npos )
  {
    ++cut_pt;
    if ( url.find( ".battle.net" ) != url.npos || url.find( ".battlenet.com" ) != url.npos )
    {
      if ( sim -> talent_format == TALENT_FORMAT_UNCHANGED )
        sim -> talent_format = TALENT_FORMAT_ARMORY;
      return p -> parse_talents_armory( url.substr( cut_pt ) );
    }
    else if ( url.find( ".wowhead.com" ) != url.npos )
    {
      if ( sim -> talent_format == TALENT_FORMAT_UNCHANGED )
        sim -> talent_format = TALENT_FORMAT_WOWHEAD;
      std::string::size_type end = url.find( '|', cut_pt );
      return p -> parse_talents_wowhead( url.substr( cut_pt, end - cut_pt ) );
    }
  }
  else
  {
    bool all_digits = true;
    for ( size_t i = 0; i < url.size() && all_digits; i++ )
      if ( ! std::isdigit( url[ i ] ) )
        all_digits = false;

    if ( all_digits )
    {
      if ( sim -> talent_format == TALENT_FORMAT_UNCHANGED )
        sim -> talent_format = TALENT_FORMAT_NUMBERS;
      return p -> parse_talents_numbers( url );
    }
  }

  sim -> errorf( "Unable to decode talent string '%s' for player %s\n", url.c_str(), p -> name() );

  return false;
}

// parse_talent_override ====================================================

bool parse_talent_override( sim_t* sim,
                            const std::string& name,
                            const std::string& override_str )
{
  assert( name == "talent_override" ); ( void )name;
  assert( sim -> active_player );
  player_t* p = sim -> active_player;

  if ( ! p -> talent_overrides_str.empty() ) p -> talent_overrides_str += "/";
    p -> talent_overrides_str += override_str;

  return true;
}

// parse_artifact_override ====================================================

bool parse_artifact_override( sim_t* sim,
                            const std::string& name,
                            const std::string& override_str )
{
  assert( name == "artifact_override" ); ( void )name;

  player_t* p = sim -> active_player;

  if ( ! p )
    return false;

  if ( ! p -> artifact_overrides_str.empty() ) p -> artifact_overrides_str += "/";
    p -> artifact_overrides_str += override_str;

  return true;
}

// parse_timeofday ====================================================

bool parse_timeofday( sim_t* sim,
                            const std::string& name,
                            const std::string& override_str )
{
  assert( name == "timeofday" ); ( void )name;
  assert( sim -> active_player );
  player_t* p = sim -> active_player;

  if ( util::str_compare_ci( override_str, "night" ) || util::str_compare_ci( override_str, "nighttime" ) )
  {
    p -> timeofday = player_t::NIGHT_TIME;
  }
  else if ( util::str_compare_ci( override_str, "day" ) || util::str_compare_ci( override_str, "daytime" ) )
  {
    p -> timeofday = player_t::DAY_TIME;
  }
  else
  {
    sim -> errorf( "\n%s timeofday string \"%s\" not valid.\n", sim -> active_player-> name(), override_str.c_str() );
  }

  return true;
}

// parse_role_string ========================================================

bool parse_role_string( sim_t* sim,
                        const std::string& name,
                        const std::string& value )
{
  assert( name == "role" ); ( void )name;

  sim -> active_player -> role = util::parse_role_type( value );

  return true;
}


// parse_world_lag ==========================================================

bool parse_world_lag( sim_t* sim,
                      const std::string& name,
                      const std::string& value )
{
  assert( name == "world_lag" ); ( void )name;

  sim -> active_player -> world_lag = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> world_lag < timespan_t::zero() )
  {
    sim -> active_player -> world_lag = timespan_t::zero();
  }

  sim -> active_player -> world_lag_override = true;

  return true;
}


// parse_world_lag ==========================================================

bool parse_world_lag_stddev( sim_t* sim,
                             const std::string& name,
                             const std::string& value )
{
  assert( name == "world_lag_stddev" ); ( void )name;

  sim -> active_player -> world_lag_stddev = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> world_lag_stddev < timespan_t::zero() )
  {
    sim -> active_player -> world_lag_stddev = timespan_t::zero();
  }

  sim -> active_player -> world_lag_stddev_override = true;

  return true;
}

// parse_brain_lag ==========================================================

bool parse_brain_lag( sim_t* sim,
                      const std::string& name,
                      const std::string& value )
{
  assert( name == "brain_lag" ); ( void )name;

  sim -> active_player -> brain_lag = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> brain_lag < timespan_t::zero() )
  {
    sim -> active_player -> brain_lag = timespan_t::zero();
  }

  return true;
}


// parse_brain_lag_stddev ===================================================

bool parse_brain_lag_stddev( sim_t* sim,
                             const std::string& name,
                             const std::string& value )
{
  assert( name == "brain_lag_stddev" ); ( void )name;

  sim -> active_player -> brain_lag_stddev = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> brain_lag_stddev < timespan_t::zero() )
  {
    sim -> active_player -> brain_lag_stddev = timespan_t::zero();
  }

  return true;
}

// parse_specialization =====================================================

bool parse_specialization( sim_t* sim,
                           const std::string&,
                           const std::string& value )
{
  sim -> active_player -> _spec = dbc::translate_spec_str( sim -> active_player -> type, value );

  if ( sim -> active_player -> _spec == SPEC_NONE )
    sim -> errorf( "\n%s specialization string \"%s\" not valid.\n", sim -> active_player-> name(), value.c_str() );

  return true;
}

// parse stat timelines =====================================================

bool parse_stat_timelines( sim_t* sim,
                           const std::string& name,
                           const std::string& value )
{
  assert( name == "stat_timelines" ); ( void )name;

  std::vector<std::string> stats = util::string_split( value, "," );

  for ( size_t i = 0; i < stats.size(); ++i )
  {
    stat_e st = util::parse_stat_type( stats[i] );

    sim -> active_player -> stat_timelines.push_back( st );
  }

  return true;
}

// parse_origin =============================================================

bool parse_origin( sim_t* sim, const std::string&, const std::string& origin )
{
  assert( sim -> active_player );
  player_t& p = *sim -> active_player;

  p.origin_str = origin;

  std::string region, server, name;
  if ( util::parse_origin( region, server, name, origin ) )
  {
    p.region_str = region;
    p.server_str = server;
  }

  return true;
}

bool parse_set_bonus( sim_t* sim, const std::string&, const std::string& value )
{
  static const char* error_str = "%s invalid 'set_bonus' option value '%s' given, available options: %s";
  assert( sim -> active_player );

  player_t* p = sim -> active_player;

  std::vector<std::string> set_bonus_split = util::string_split( value, "=" );

  if ( set_bonus_split.size() != 2 )
  {
    sim -> errorf( error_str, p -> name(), value.c_str(), p -> sets.generate_set_bonus_options().c_str() );
    return false;
  }

  int opt_val = util::to_int( set_bonus_split[ 1 ] );
  if ( errno != 0 || ( opt_val != 0 && opt_val != 1 ) )
  {
    sim -> errorf( error_str, p -> name(), value.c_str(), p -> sets.generate_set_bonus_options().c_str() );
    return false;
  }

  set_bonus_type_e set_bonus = SET_BONUS_NONE;
  set_bonus_e bonus = B_NONE;

  if ( ! p -> sets.parse_set_bonus_option( set_bonus_split[ 0 ], set_bonus, bonus ) )
  {
    sim -> errorf( error_str, p -> name(), value.c_str(), p -> sets.generate_set_bonus_options().c_str() );
    return false;
  }

  p -> sets.set_bonus_spec_data[ set_bonus ][ specdata::spec_idx( p -> specialization() ) ][ bonus ].overridden = opt_val;

  return true;
}

bool parse_artifact( sim_t* sim, const std::string&, const std::string& value )
{
  sim -> active_player -> artifact = player_t::artifact_data_t();

  if ( value.size() == 0 )
  {
    return false;
  }

  bool ret = false;

  if ( sim -> disable_artifacts )
  {
    ret = true;
  }
  else if ( util::str_in_str_ci( value, ".wowdb.com/artifact-calculator#" ) )
  {
    ret = sim -> active_player -> parse_artifact_wowdb( value );
  }
  else if ( util::str_in_str_ci( value, ":" ) )
  {
    ret = sim -> active_player -> parse_artifact_wowhead( value );
  }

  if ( ret )
  {
    sim -> active_player -> artifact_str = value;
  }

  return ret;
}

} // UNNAMED NAMESPACE ======================================================

// This is a template for Ignite like mechanics, like of course Ignite, Hunter Piercing Shots, Priest Echo of Light, etc.
// It should get specialized in the class module

// Detailed MoP Ignite Mechanic description at
// http://us.battle.net/wow/en/forum/topic/5889309137?page=40#787

// There is still a delay between the impact of the triggering spell and the dot application/refresh and damage calculation.
void residual_action::trigger( action_t* residual_action, player_t* t, double amount )
{
  struct delay_event_t : public event_t
  {
    double additional_residual_amount;
    player_t* target;
    action_t* action;

    static timespan_t delay_duration( player_t* p )
    {
      // Use same delay as in buff application
      return p->sim->rng().gauss( p->sim->default_aura_delay,
                                  p->sim->default_aura_delay_stddev );
    }

    delay_event_t( player_t* t, action_t* a, double amount )
      : event_t( *a->player, delay_duration( a->player ) ),
        additional_residual_amount( amount ),
        target( t ),
        action( a )
    {
      if ( sim().debug )
        sim().out_debug.printf(
            "%s %s residual_action delay_event_start amount=%f",
            a->player->name(), action->name(), amount );
    }
    virtual const char* name() const override
    { return "residual_action_delay_event"; }
    virtual void execute() override
    {
      // Don't ignite on targets that are not active
      if ( target -> is_sleeping() )
        return;

      dot_t* dot = action -> get_dot( target );
      residual_periodic_state_t* dot_state = debug_cast<residual_periodic_state_t*>( dot -> state );

      assert( action -> dot_duration > timespan_t::zero() );

      if ( sim().debug )
      {
        if ( dot -> is_ticking() )
        {
          sim().out_debug.printf( "%s %s residual_action delay_event_execute target=%s amount=%f current_ticks=%d current_tick=%f",
                      action -> player -> name(), action -> name(), target -> name(),
                      additional_residual_amount, dot -> ticks_left(), dot_state -> tick_amount );
        }
        else
        {
          sim().out_debug.printf( "%s %s residual_action delay_event_execute target=%s amount=%f",
              action -> player -> name(), action -> name(), target -> name(), additional_residual_amount );
        }
      }

      // Pass total amount of damage to the ignite action, and let it divide it by the correct number of ticks!
      action_state_t* s = action -> get_state();
      s -> target = target;
      s -> result = RESULT_HIT;
      action -> snapshot_state( s, action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );
      s -> result_amount = additional_residual_amount;
      action -> schedule_travel( s );
      if ( ! action -> dual ) action -> stats -> add_execute( timespan_t::zero(), s -> target );
    }
  };

  assert( residual_action );

  make_event<delay_event_t>( *residual_action -> sim, t, residual_action, amount );
}

// ==========================================================================
// Player
// ==========================================================================

// player_t::player_t =======================================================

player_t::player_t( sim_t*             s,
                    player_e           t,
                    const std::string& n,
                    race_e             r ) :
  actor_t( s, n ),
  type( t ),
  parent( 0 ),

  index( -1 ),
  actor_spawn_index( -1 ),

  // (static) attributes
  race( r ),
  role( ROLE_NONE ),
  true_level( default_level ),
  party( 0 ),
  ready_type( READY_POLL ),
  _spec( SPEC_NONE ),
  bugs( true ),
  disable_hotfixes( 0 ),
  scale_player( true ),
  death_pct( 0.0 ),
  height( 0 ),
  combat_reach( 1.0 ),

  // dynamic stuff
  target( 0 ),
  initialized( false ), potion_used( false ),

  region_str( s -> default_region_str ), server_str( s -> default_server_str ), origin_str(),
  timeofday( DAY_TIME ), //Set to Day by Default since in raid it always switch to Day, user can override.
  gcd_ready( timespan_t::zero() ), base_gcd( timespan_t::from_seconds( 1.5 ) ), min_gcd( timespan_t::from_millis( 750 ) ),
  gcd_haste_type( HASTE_NONE ), gcd_current_haste_value( 1.0 ),
  started_waiting( timespan_t::min() ),
  pet_list(), active_pets(),
  invert_scaling( 0 ),
  // Reaction
  reaction_offset( timespan_t::from_seconds( 0.1 ) ), reaction_mean( timespan_t::from_seconds( 0.3 ) ), reaction_stddev( timespan_t::zero() ), reaction_nu( timespan_t::from_seconds( 0.25 ) ),
  // Latency
  world_lag( timespan_t::from_seconds( 0.1 ) ), world_lag_stddev( timespan_t::min() ),
  brain_lag( timespan_t::zero() ), brain_lag_stddev( timespan_t::min() ),
  world_lag_override( false ), world_lag_stddev_override( false ),
  cooldown_tolerance_( timespan_t::min() ),
  dbc( s -> dbc ),
  talent_points(),
  glyph_list(),
  artifact( artifact_data_t() ),
  base(),
  initial(),
  current(),
  // Spell Mechanics
  base_energy_regen_per_second( 0 ), base_focus_regen_per_second( 0 ), base_chi_regen_per_second( 0 ),
  last_cast( timespan_t::zero() ),
  // Defense Mechanics
  def_dr( diminishing_returns_constants_t() ),
  // Attacks
  main_hand_attack( nullptr ), off_hand_attack( nullptr ),
  current_attack_speed( 1.0 ),
  // Resources
  resources( resources_t() ),
  // Consumables
  consumables(),
  // Events
  executing( 0 ), queueing( 0 ), channeling( 0 ), strict_sequence( 0 ), readying( 0 ), off_gcd( 0 ), in_combat( false ), action_queued( false ), first_cast( true ),
  last_foreground_action( 0 ), last_gcd_action( 0 ),
  off_gcdactions(),
  cast_delay_reaction( timespan_t::zero() ), cast_delay_occurred( timespan_t::zero() ),
  callbacks( s ),
  use_apl( "" ),
  // Actions
  use_default_action_list( 0 ),
  precombat_action_list( 0 ), active_action_list( 0 ), active_off_gcd_list( 0 ), restore_action_list( 0 ),
  no_action_list_provided(),
  // Reporting
  quiet( false ),
  report_extension( new player_report_extension_t() ),
  iteration_fight_length( timespan_t::zero() ), arise_time( timespan_t::min() ),
  iteration_waiting_time( timespan_t::zero() ), iteration_pooling_time( timespan_t::zero() ),
  iteration_executed_foreground_actions( 0 ),
  rps_gain( 0 ), rps_loss( 0 ),

  tmi_window( 6.0 ),
  collected_data( name_str, *sim ),
  // Damage
  iteration_dmg( 0 ), priority_iteration_dmg( 0 ), iteration_dmg_taken( 0 ),
  dpr( 0 ),
  dps_convergence( 0 ),
  // Heal
  iteration_heal( 0 ), iteration_heal_taken( 0 ),
  iteration_absorb(), iteration_absorb_taken(),
  hpr( 0 ),

  report_information( player_processed_report_information_t() ),
  // Gear
  sets( this ),
  meta_gem( META_GEM_NONE ), matching_gear( false ),
  karazhan_trinkets_paired( false ),
  item_cooldown( cooldown_t( "item_cd", *this ) ),
  legendary_tank_cloak_cd( nullptr ),
  warlords_unseeing_eye( 0.0 ),
  auto_attack_multiplier( 1.0 ),
  // Movement & Position
  base_movement_speed( 7.0 ), passive_modifier( 0 ),
  x_position( 0.0 ), y_position( 0.0 ),
  default_x_position( 0.0 ), default_y_position( 0.0 ),
  buffs( buffs_t() ),
  debuffs( debuffs_t() ),
  gains( gains_t() ),
  procs( procs_t() ),
  uptimes( uptimes_t() ),
  racials( racials_t() ),
  passive_values( passives_t() ),
  active_during_iteration( false ),
  _mastery( spelleffect_data_t::nil() ),
  cache( this ),
  regen_type( REGEN_STATIC ),
  last_regen( timespan_t::zero() ),
  regen_caches( CACHE_MAX ),
  dynamic_regen_pets( false ),
  visited_apls_( 0 ),
  action_list_id_( 0 )
{
  actor_index = sim -> actor_list.size();
  sim -> actor_list.push_back( this );

  // Set the gear object to a special default value, so we can support gear_x=0 properly.
  // player_t::init_items will replace the defaulted gear_stats_t object with a computed one once
  // the item stats have been computed.
  gear.initialize( std::numeric_limits<double>::lowest() );

  base.skill = sim -> default_skill;
  base.mastery = 8.0;
  base.movement_direction = MOVEMENT_NONE;

  if ( !is_enemy() && type != HEALING_ENEMY )
  {
    if ( sim -> debug ) sim -> out_debug.printf( "Creating Player %s", name() );
    sim -> player_list.push_back( this );
    if ( ! is_pet() )
    {
      sim -> player_no_pet_list.push_back( this );
    }
    index = ++( sim -> num_players );
  }
  else
  {
    if ( type != HEALING_ENEMY ) // Not actually a enemy target.
    {
      ++( sim -> enemy_targets );
    }
    index = -( ++( sim -> num_enemies ) );
  }

  // Fill healng lists with all non-enemy players.
  if( !is_enemy() )
  {
    if( ! is_pet() )
    {
      sim -> healing_no_pet_list.push_back( this );
    }
    else
    {
      sim -> healing_pet_list.push_back( this );
    }
  }

  if ( ! is_pet() && sim -> stat_cache != -1 )
  {
    cache.active = sim -> stat_cache != 0;
  }
  if ( is_pet() ) current.skill = 1.0;

  resources.infinite_resource[ RESOURCE_HEALTH ] = true;

  range::fill( iteration_resource_lost, 0 );
  range::fill( iteration_resource_gained, 0 );

  range::fill( profession, 0 );

  range::fill( scales_with, false );
  range::fill( over_cap, 0 );
  range::fill( scaling_lag, 0 );
  range::fill( scaling_lag_error, 0 );

  if ( ! is_pet() )
  {
    items.resize( SLOT_MAX );
    for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    {
      items[ i ].slot = i;
      items[ i ].sim = sim;
      items[ i ].player = this;
    }
  }

  main_hand_weapon.slot = SLOT_MAIN_HAND;
  off_hand_weapon.slot = SLOT_OFF_HAND;

  if ( reaction_stddev == timespan_t::zero() )
    reaction_stddev = reaction_mean * 0.2;

  action_list_information =
    "\n"
    "# This default action priority list is automatically created based on your character.\n"
    "# It is a attempt to provide you with a action list that is both simple and practicable,\n"
    "# while resulting in a meaningful and good simulation. It may not result in the absolutely highest possible dps.\n"
    "# Feel free to edit, adapt and improve it to your own needs.\n"
    "# SimulationCraft is always looking for updates and improvements to the default action lists.\n";
}

player_t::~player_t()
{
  for ( std::map<unsigned, instant_absorb_t*>::iterator i = instant_absorb_list.begin();
        i != instant_absorb_list.end();
        ++i )
  {
    delete i -> second;
  }
}

player_t::base_initial_current_t::base_initial_current_t() :
  stats(),
  mana_regen_per_second(),
  spell_power_per_intellect( 0 ),
  spell_crit_per_intellect( 0 ),
  attack_power_per_strength( 0 ),
  attack_power_per_agility( 0 ),
  attack_crit_per_agility( 0 ),
  dodge_per_agility( 0 ),
  parry_per_strength( 0 ),
  mana_regen_per_spirit( 0 ),
  mana_regen_from_spirit_multiplier( 0 ),
  health_per_stamina( 0 ),
  resource_reduction(),
  miss( 0 ),
  dodge( 0 ),
  parry( 0 ),
  block( 0 ),
  hit( 0 ),
  expertise( 0 ),
  spell_crit_chance(),
  attack_crit_chance(),
  block_reduction(),
  mastery(),
  skill( 1.0 ),
  skill_debuff( 0.0 ),
  distance( 0 ),
  distance_to_move( 0 ),
  moving_away( 0 ), movement_direction(),
  armor_coeff( 0 ),
  sleeping( false ),
  rating(),
  attribute_multiplier(),
  spell_power_multiplier( 1.0 ),
  attack_power_multiplier( 1.0 ),
  armor_multiplier( 1.0 ),
  position( POSITION_BACK )
{
  range::fill( attribute_multiplier, 1.0 );
}

std::string player_t::base_initial_current_t::to_string()
{
  std::ostringstream s;

  s << stats.to_string();
  s << "mana_regen_per_second=" << mana_regen_per_second;
  s << " spell_power_per_intellect=" << spell_power_per_intellect;
  s << " spell_crit_per_intellect=" << spell_crit_per_intellect;
  s << " attack_power_per_strength=" << attack_power_per_strength;
  s << " attack_power_per_agility=" << attack_power_per_agility;
  s << " attack_crit_per_agility=" << attack_crit_per_agility;
  s << " dodge_per_agility=" << dodge_per_agility;
  s << " parry_per_strength=" << parry_per_strength;
  s << " mana_regen_per_spirit=" << mana_regen_per_spirit;
  s << " mana_regen_from_spirit_multiplier=" << mana_regen_from_spirit_multiplier;
  s << " health_per_stamina=" << health_per_stamina;
  // resource_reduction
  s << " miss=" << miss;
  s << " dodge=" << dodge;
  s << " parry=" << parry;
  s << " block=" << block;
  s << " spell_crit_chance=" << spell_crit_chance;
  s << " attack_crit_chance=" << attack_crit_chance;
  s << " block_reduction=" << block_reduction;
  s << " mastery=" << mastery;
  s << " skill=" << skill;
  s << " distance=" << distance;
  s << " armor_coeff=" << armor_coeff;
  s << " sleeping=" << sleeping;
  // attribute_multiplier
  s << " spell_power_multiplier=" << spell_power_multiplier;
  s << " attack_power_multiplier=" << attack_power_multiplier;
  s << " armor_multiplier=" << armor_multiplier;
  s << " position=" << util::position_type_string( position );
  return s.str();
}

// player_t::init ===========================================================

void player_t::init()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Initializing player %s", name() );

  // Find parent player in main thread
  if( ! is_pet() && ! is_enemy() && sim -> parent && sim -> thread_index > 0 )
  {
    parent = sim -> parent -> find_player( name() );
  }

  // Ensure the precombat and default lists are the first listed
  get_action_priority_list( "precombat", "Executed before combat begins. Accepts non-harmful actions only." ) -> used = true;
  get_action_priority_list( "default", "Executed every time the actor is available." );

  for (auto & elem : alist_map)
  {
    if ( elem.first == "default" )
      sim -> errorf( "Ignoring action list named default." );
    else
      get_action_priority_list( elem.first ) -> action_list_str = elem.second;
  }

  // If the owner is regenerating using dynamic resource regen, we need to
  // ensure that pets that regen dynamically also get updated correctly. Thus,
  // we copy any CACHE_x enum values from pets to the owner. Also, if we have
  // no dynamically regenerating pets, we do not need to go through extra work
  // in do_dynamic_regen() to call the pets do_dynamic_regen(), saving some cpu
  // cycles.
  if ( regen_type == REGEN_DYNAMIC )
  {
    for (auto pet : pet_list)
    {

      if ( pet -> regen_type != REGEN_DYNAMIC )
        continue;

      for ( cache_e c = CACHE_NONE; c < CACHE_MAX; c++ )
      {
        if ( pet -> regen_caches[ c ] )
          regen_caches[ c ] = true;
      }

      dynamic_regen_pets = true;
    }
  }

  // If Single-actor batch mode is used, player_collected_data_t::fight_length has to be
  // unconditionally made to record full data on actor fight lengths. This is to get proper
  // information on timelines in reports, since the sim-wide fight lengths ar no longer usable as
  // the "timeline adjustor" during analysis phase. This operation is done here so the
  // single_actor_batch does not become a positional parameter, since relying on it's state in
  // player_collected_data_t constructor would require it to be parsed before any actors are
  // defined.
  if ( sim -> single_actor_batch )
  {
    collected_data.fight_length.change_mode( false ); // Not simple
  }
}

/* Determine Spec, Talents, Professions, Glyphs
 *
 */
void player_t::init_character_properties()
{
  init_race();
  init_talents();
  init_glyphs();
  replace_spells();
  init_position();
  init_professions();

  if ( sim -> tmi_window_global > 0 )
    tmi_window = sim -> tmi_window_global;
}

/* Initialzies base variables from database or other sources
 * After player_t::init_base is executed, you can modify the base member until init_initial is called
 */
void player_t::init_base_stats()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Initializing base for player (%s)", name() );

#ifndef NDEBUG
  for ( stat_e i = STAT_NONE; i < STAT_MAX; ++i )
  {
    double s = base.stats.get_stat( i );
    if ( s > 0 )
    {
      sim -> errorf( "%s stat %s is %.4f", name(), util::stat_type_string( i ), s );
      sim -> errorf( " Please do not modify player_t::base before init_base_stats is called\n" );
    }
  }
#endif

  if ( ! is_enemy() )
    base.rating.init( dbc, level() );

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Base Ratings initialized: %s", name(), base.rating.to_string().c_str() );

  if ( ! is_enemy() )
  {
    base.stats.attribute[ STAT_STRENGTH  ]  = dbc.race_base( race ).strength + dbc.attribute_base( type, level() ).strength;
    base.stats.attribute[ STAT_AGILITY   ]  = dbc.race_base( race ).agility + dbc.attribute_base( type, level() ).agility;
    base.stats.attribute[ STAT_STAMINA   ]  = dbc.race_base( race ).stamina + dbc.attribute_base( type, level() ).stamina;
    base.stats.attribute[ STAT_INTELLECT ]  = dbc.race_base( race ).intellect + dbc.attribute_base( type, level() ).intellect;
    base.stats.attribute[ STAT_SPIRIT    ]  = dbc.race_base( race ).spirit + dbc.attribute_base( type, level() ).spirit;

    // heroic presence is treated like base stats, floored before adding in; tested 7/20/2014
    base.stats.attribute[ STAT_STRENGTH  ] += util::floor( racials.heroic_presence -> effectN( 1 ).average( this ) );
    base.stats.attribute[ STAT_AGILITY   ] += util::floor( racials.heroic_presence -> effectN( 2 ).average( this ) );
    base.stats.attribute[ STAT_INTELLECT ] += util::floor( racials.heroic_presence -> effectN( 3 ).average( this ) );
    // so is endurance. Can't tell if this is floored, ends in 0.055 @ L100. Assuming based on symmetry w/ heroic pres.
    base.stats.attribute[ STAT_STAMINA   ] += util::floor( racials.endurance -> effectN( 1 ).average( this ) );

    base.spell_crit_chance        = dbc.spell_crit_base( type, level() );
    base.attack_crit_chance       = dbc.melee_crit_base( type, level() );
    base.spell_crit_per_intellect = dbc.spell_crit_scaling( type, level() );
    base.attack_crit_per_agility  = dbc.melee_crit_scaling( type, level() );
    base.mastery = 8.0;

    resources.base[ RESOURCE_HEALTH ] = dbc.health_base( type, level() );
    resources.base[ RESOURCE_MANA   ] = dbc.resource_base( type, level() );

    base.mana_regen_per_second = dbc.regen_base( type, level() ) / 5.0;
    base.mana_regen_per_spirit = dbc.regen_spirit( type, level() );
    base.health_per_stamina    = dbc.health_per_stamina( level() );

    // players have a base 7.5% hit/exp
    base.hit       = 0.075;
    base.expertise = 0.075;
  }

  // only certain classes get Agi->Dodge conversions, dodge_per_agility defaults to 0.00
  // Racial agility modifiers and Heroic Presence do affect base dodge, but are affected
  // by diminishing returns, and handled in composite_dodge()  (tested 7/24/2014)
  if ( type == MONK || type == DRUID || type == ROGUE || type == HUNTER ||
       type == SHAMAN || type == DEMON_HUNTER )
    base.dodge_per_agility     = dbc.avoid_per_str_agi_by_level( level() ) / 100.0; // exact values given by Blizzard, only have L90-L100 data

  // only certain classes get Str->Parry conversions, dodge_per_agility defaults to 0.00
  // Racial strength modifiers and Heroic Presence do affect base parry, but are affected
  // by diminishing returns, and handled in composite_parry()  (tested 7/24/2014)
  if ( type == PALADIN || type == WARRIOR || type == DEATH_KNIGHT )
    base.parry_per_strength    = dbc.avoid_per_str_agi_by_level( level() ) / 100.0; // exact values given by Blizzard, only have L90-L100 data

  // All classes get 3% dodge and miss; add racials and racial agi mod in here too
  base.dodge = 0.03 + racials.quickness -> effectN( 1 ).percent() + dbc.race_base( race ).agility * base.dodge_per_agility;
  base.miss  = 0.03;

  // Only Warriors and Paladins (and enemies) can block, defaults is 0
  if ( type == WARRIOR || type == PALADIN || type == ENEMY || type == TMI_BOSS || type == TANK_DUMMY )
  {
    base.block = 0.03;
    base.block_reduction = 0.30;
  }

  // Only certain classes can parry, and get 3% base parry, defaults is 0
  // racial strength mod and "phantom" strength bonus added here,
  // see http://www.sacredduty.net/2014/08/06/tc401-avoidance-diminishing-returns-in-wod/
  if ( type == WARRIOR || type == PALADIN || type == ROGUE || type == DEATH_KNIGHT ||
       type == MONK || type == DEMON_HUNTER || specialization() == SHAMAN_ENHANCEMENT ||
       type == ENEMY || type == TMI_BOSS || type == TANK_DUMMY )
    base.parry = 0.03 + ( dbc.race_base( race ).strength + 0.0739 ) * base.parry_per_strength;

  // Extract avoidance DR values from table in sc_extra_data.inc
  def_dr.horizontal_shift = dbc.horizontal_shift( type );
  def_dr.vertical_stretch = dbc.vertical_stretch( type );
  def_dr.dodge_factor = dbc.dodge_factor( type );
  def_dr.parry_factor = dbc.parry_factor( type );
  def_dr.miss_factor = dbc.miss_factor( type );
  def_dr.block_factor = dbc.block_factor( type );

  base.spell_power_multiplier    = 1.0;
  base.attack_power_multiplier   = 1.0;

  if ( ( meta_gem == META_EMBER_PRIMAL ) || ( meta_gem == META_EMBER_SHADOWSPIRIT ) || ( meta_gem == META_EMBER_SKYFIRE ) || ( meta_gem == META_EMBER_SKYFLARE ) )
  {
    resources.base_multiplier[ RESOURCE_MANA ] *= 1.02;
  }

  resources.base_multiplier[ RESOURCE_MANA ] *= 1 + racials.expansive_mind -> effectN( 1 ).percent();
  resources.base_multiplier[ RESOURCE_RAGE ] *= 1 + racials.expansive_mind -> effectN( 1 ).percent();
  resources.base_multiplier[ RESOURCE_ENERGY ] *= 1 + racials.expansive_mind -> effectN( 1 ).percent();
  resources.base_multiplier[ RESOURCE_RUNIC_POWER ] *= 1 + racials.expansive_mind -> effectN( 1 ).percent();


  if ( true_level >= 50 && matching_gear )
  {
    for ( attribute_e a = ATTR_STRENGTH; a <= ATTR_SPIRIT; a++ )
    {
      base.stats.attribute[ a ] *= 1.0 + matching_gear_multiplier( a );
      // NOTE: post-matching-multiplier base stats are NOT actually floored.
      // They are only floor()-ed for the character sheet and in certain calculations.
    }
  }

  if ( world_lag_stddev < timespan_t::zero() ) world_lag_stddev = world_lag * 0.1;
  if ( brain_lag_stddev < timespan_t::zero() ) brain_lag_stddev = brain_lag * 0.1;

  if ( primary_role() == ROLE_TANK )
  {
    // Collect DTPS data for tanks even for statistics_level == 1
    if ( sim -> statistics_level >= 1  )
      collected_data.dtps.change_mode( false );
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Generic Base Stats: %s", name(), base.to_string().c_str() );

}

/* Initialzies initial variable from base + gear
 * After player_t::init_initial is executed, you can modify the initial member until combat starts
 */
void player_t::init_initial_stats()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing initial stats for player (%s)", name() );

#ifndef NDEBUG
  for ( stat_e i = STAT_NONE; i < STAT_MAX; ++i )
  {
    double s = initial.stats.get_stat( i );
    if ( s > 0 )
    {
      sim -> errorf( "%s stat %s is %.4f", name(), util::stat_type_string( i ), s );
      sim -> errorf( " Please do not modify player_t::initial before init_initial_stats is called\n" );
    }
  }
#endif

  initial = base;
  initial.stats += passive;

  // Compute current "total from gear" into total gear. Per stat, this is either the amount of stats
  // the items for the actor gives, or the overridden value (player_t::gear + player_t::enchant +
  // sim_t::enchant).
  if ( ! is_pet() && ! is_enemy() )
  {
    gear_stats_t item_stats = std::accumulate( items.begin(), items.end(), gear_stats_t(),
      []( const gear_stats_t& t, const item_t& i ) {
        return t + i.total_stats();
    });

    for ( stat_e stat = STAT_NONE; stat < STAT_MAX; ++stat )
    {
      if ( gear.get_stat( stat ) < 0 )
        total_gear.add_stat( stat, item_stats.get_stat( stat ) );
      else
        total_gear.add_stat( stat, gear.get_stat( stat ) );
    }

    if ( sim -> debug )
      sim -> out_debug.printf( "%s: Total Gear Stats: %s", name(), total_gear.to_string().c_str() );

    initial.stats += enchant;
    initial.stats += sim -> enchant;
  }

  initial.stats += total_gear;

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Generic Initial Stats: %s", name(), initial.to_string().c_str() );
}
// player_t::init_items =====================================================

bool player_t::init_items()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing items for player (%s)", name() );

  // Create items
  std::vector<std::string> splits = util::string_split( items_str, "/" );
  for ( size_t i = 0; i < splits.size(); i++ )
  {
    if ( find_item( splits[ i ] ) )
    {
      sim -> errorf( "Player %s has multiple %s equipped.\n", name(), splits[ i ].c_str() );
    }
    items.push_back( item_t( this, splits[ i ] ) );
  }

  bool slots[ SLOT_MAX ]; // true if the given item is equal to the highest armor type the player can wear
  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    slots[ i ] = ! util::is_match_slot( i );

  // We need to simple-parse the items first, this will set up some base information, and parse out
  // simple options
  for ( size_t i = 0; i < items.size(); i++ )
  {
    item_t& item = items[ i ];

    // If the item has been specified in options we want to start from scratch, forgetting about
    // lingering stuff from profile copy
    if ( ! item.options_str.empty() )
    {
      item = item_t( this, item.options_str );
      item.slot = static_cast<slot_e>( i );
    }

    if ( ! item.parse_options() )
    {
      sim -> errorf( "Unable to parse item '%s' options on player '%s'\n", item.name(), name() );
      sim -> cancel();
      return false;
    }

    if ( ! item.initialize_data() )
    {
      sim -> errorf( "Unable to initialize item '%s' base data on player '%s'\n", item.name(), name() );
      sim -> cancel();
      return false;
    }
  }

  // Once item data is initialized, initialize the parent - child relationships of each item, and
  // figure out primary artifact slot if we find one equipped.
  range::for_each( items, [ this ]( item_t& i ) {
    i.parent_slot = parent_item_slot( i );
    if ( i.parsed.data.id_artifact > 0 && i.parent_slot == SLOT_INVALID )
    {
      artifact.slot = i.slot;
    }
  } );

  // Slot initialization order vector. Needed to ensure parents of children get initialized first
  std::vector<slot_e> init_slots;
  range::for_each( items, [ &init_slots ]( const item_t& i ) { init_slots.push_back( i.slot ); } );

  // Sort items with children before items without children
  range::sort( init_slots, [ this ]( slot_e first, slot_e second ) {
    const item_t& fi = items[ first ], si = items[ second ];
    if ( fi.parent_slot != SLOT_INVALID && si.parent_slot == SLOT_INVALID )
    {
      return false;
    }
    else if ( fi.parent_slot == SLOT_INVALID && si.parent_slot != SLOT_INVALID )
    {
      return true;
    }

    return false;
  } );

  for ( size_t i = 0; i < init_slots.size(); i++ )
  {
    slot_e slot = init_slots[ i ];
    item_t& item = items[ slot ];

    if ( ! item.init() )
    {
      sim -> errorf( "Unable to initialize item '%s' on player '%s'\n", item.name(), name() );
      sim -> cancel();
      return false;
    }

    if ( ! item.is_valid_type() )
    {
      sim -> errorf( "Item '%s' on player '%s' is of invalid type\n", item.name(), name() );
      sim -> cancel();
      return false;
    }

    slots[ item.slot ] = item.is_matching_type();
  }

  matching_gear = true;
  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
  {
    if ( !slots[i] )
    {
      matching_gear = false;
      break;
    }
  }

  init_meta_gem();

  // Needs to be initialized after old set bonus system
  sets.initialize();

  // these initialize the weapons, but don't have a return value (yet?)
  init_weapon( main_hand_weapon );
  init_weapon( off_hand_weapon );

  return true;
}

// player_t::init_meta_gem ==================================================

void player_t::init_meta_gem()
{
  if ( ! meta_gem_str.empty() ) meta_gem = util::parse_meta_gem_type( meta_gem_str );

  if ( sim -> debug ) sim -> out_debug.printf( "Initializing meta-gem for player (%s)", name() );

  if ( ( meta_gem == META_AUSTERE_EARTHSIEGE ) || ( meta_gem == META_AUSTERE_SHADOWSPIRIT ) )
  {
    initial.armor_multiplier *= 1.02;
  }

}

// player_t::init_position ==================================================

void player_t::init_position()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing position for player (%s)", name() );

  if ( ! position_str.empty() )
  {
    base.position = util::parse_position_type( position_str );
    if ( base.position == POSITION_NONE )
    {
      sim -> errorf( "Player %s has an invalid position of %s.\n", name(), position_str.c_str() );
    }
    else
    {
      initial.position = base.position;
      change_position( base.position );
    }
  }
  // if the position string is empty and we're a tank, assume they want to be in front
  else if ( primary_role() == ROLE_TANK )
  {
    // set position to front
    base.position = POSITION_FRONT;
    initial.position = POSITION_FRONT;
    change_position( POSITION_FRONT );
  }

  // default to back when we have an invalid position
  if ( base.position == POSITION_NONE )
    base.position = POSITION_BACK;

  position_str = util::position_type_string( base.position );

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Position set to %s", name(), position_str.c_str() );

}

// player_t::init_race ======================================================

void player_t::init_race()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing race for player (%s)", name() );

  if ( race_str.empty() )
  {
    race_str = util::race_type_string( race );
  }
  else
  {
    race = util::parse_race_type( race_str );
    if ( race == RACE_UNKNOWN )
    {
      sim -> errorf( "%s has unknown race string specified", name() );
      race_str = util::race_type_string( race );
    }
  }
}

// player_t::init_defense ===================================================

void player_t::init_defense()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing defense for player (%s)", name() );

  if ( primary_role() == ROLE_TANK )
  {
    initial.position = POSITION_FRONT;
    if ( sim -> debug )
      sim -> out_debug.printf( "%s: Initial Position set to front because primary_role() == ROLE_TANK", name() );
  }

  if ( ! is_pet() && primary_role() == ROLE_TANK )
  {
    collected_data.health_changes.collect = true;
    collected_data.health_changes.set_bin_size( sim -> tmi_bin_size );
    collected_data.health_changes_tmi.collect = true;
    collected_data.health_changes_tmi.set_bin_size( sim -> tmi_bin_size );
  }

  // Armor Coefficient
  initial.armor_coeff = dbc.armor_mitigation_constant( level() );
  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Initial Armor Coeff set to %.4f", name(), initial.armor_coeff );

}

// player_t::init_weapon ====================================================

void player_t::init_weapon( weapon_t& w )
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing weapon ( type %s ) for player (%s)", util::weapon_type_string( w.type ), name() );

  if ( w.type == WEAPON_NONE ) return;

  if ( w.slot == SLOT_MAIN_HAND ) assert( w.type >= WEAPON_NONE && w.type < WEAPON_RANGED );
  if ( w.slot == SLOT_OFF_HAND  ) assert( w.type >= WEAPON_NONE && w.type < WEAPON_2H );
}

// player_t::init_special_effects ===============================================

struct touch_of_the_grave_spell_t : public spell_t
{
  touch_of_the_grave_spell_t( player_t* p, const spell_data_t* spell ) :
    spell_t( "touch_of_the_grave", p, spell )
  {
    background = may_crit = true;
    base_dd_min = base_dd_max = 0;
    attack_power_mod.direct = 1.0;
    spell_power_mod.direct = 1.0;
  }

  double attack_direct_power_coefficient( const action_state_t* ) const override
  {
    if ( composite_attack_power() >= composite_spell_power() )
    {
      return attack_power_mod.direct;
    }

    return 0;
  }

  double spell_direct_power_coefficient( const action_state_t* ) const override
  {
    if ( composite_spell_power() > composite_attack_power() )
    {
      return spell_power_mod.direct;
    }

    return 0;
  }
};

bool player_t::create_special_effects()
{
  if ( is_pet() || is_enemy() )
  {
    return true;
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "Creating special effects for player (%s)", name() );

  const spell_data_t* totg = find_racial_spell( "Touch of the Grave" );
  if ( totg -> ok() )
  {
    special_effect_t* effect = new special_effect_t( this );
    effect -> type = SPECIAL_EFFECT_EQUIP;
    effect -> spell_id = totg -> id();
    effect -> execute_action = new touch_of_the_grave_spell_t( this, totg -> effectN( 1 ).trigger() );
    special_effects.push_back( effect );
  }

  // Initialize all item-based special effects. This includes any DBC-backed enchants, gems, as well
  // as inherent item effects that use a spell
  for ( auto& item: items )
  {
    if ( ! item.init_special_effects() )
    {
      return false;
    }
  }

  // Set bonus initialization. Note that we err on the side of caution here and
  // require that the set bonus is "custom" (and as such, specified in the
  // master list of custom special effect in unique gear). This is to avoid
  // false positives with class-specific set bonuses that have to always be
  // implemented inside the class module anyhow.
  std::vector<const item_set_bonus_t*> bonuses = sets.enabled_set_bonus_data();
  for ( size_t i = 0; i < bonuses.size(); i++ )
  {
    special_effect_t effect( this );
    if ( ! unique_gear::initialize_special_effect( effect, bonuses[ i ] -> spell_id ) )
    {
      return false;
    }

    if ( effect.custom_init_object.size() == 0 )
    {
      continue;
    }

    special_effects.push_back( new special_effect_t( effect ) );
  }

  // Once all special effects are first-phase initialized, do a pass to first-phase initialize any
  // potential fallback special effects for the actor.
  unique_gear::initialize_special_effect_fallbacks( this );

  return true;
}

bool player_t::init_special_effects()
{
  if ( is_pet() || is_enemy() )
  {
    return true;
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing special effects for player (%s)", name() );

  // ..and then move on to second phase initialization of all special effects.
  unique_gear::init( this );

  for (auto & elem : callbacks.all_callbacks)
    elem -> initialize();

  return true;
}

// player_t::init_resources =================================================

void player_t::init_resources( bool force )
{
  if ( sim -> debug ) sim -> out_debug.printf( "Initializing resources for player (%s)", name() );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( force || resources.initial[ i ] == 0 )
    {
      resources.initial[ i ]  = resources.base[ i ];
      resources.initial[ i ] *= resources.base_multiplier[ i ];
      resources.initial[ i ] += total_gear.resource[ i ];

      // re-ordered 19/06/2016 by Theck - initial_multiplier should do something for RESOURCE_HEALTH
      if ( i == RESOURCE_HEALTH )
        resources.initial[ i ] += floor( stamina() ) * current.health_per_stamina;

      resources.initial[ i ] *= resources.initial_multiplier[ i ];
    }
  }

  resources.current = resources.max = resources.initial;

  // Only collect pet resource timelines if they get reported separately
  if ( ! is_pet() || sim -> report_pets_separately )
  {
    if ( collected_data.resource_timelines.size() == 0 )
    {
      for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
      {
        if ( resources.max[ i ] > 0 )
        {
          collected_data.resource_timelines.push_back( player_collected_data_t::resource_timeline_t( i ) );
        }
      }
    }
  }
}

// player_t::init_professions ===============================================

void player_t::init_professions()
{
  if ( professions_str.empty() ) return;

  if ( sim -> debug ) sim -> out_debug.printf( "Initializing professions for player (%s)", name() );

  std::vector<std::string> splits = util::string_split( professions_str, ",/" );

  for ( unsigned int i = 0; i < splits.size(); ++i )
  {
    std::string prof_name;
    int prof_value = 0;

    if ( 2 != util::string_split( splits[ i ], "=", "S i", &prof_name, &prof_value ) )
    {
      prof_name  = splits[ i ];
      prof_value = true_level > 85 ? 600 : 525;
    }

    int prof_type = util::parse_profession_type( prof_name );
    if ( prof_type == PROFESSION_NONE )
    {
      sim -> errorf( "Invalid profession encoding: %s\n", professions_str.c_str() );
      return;
    }

    profession[ prof_type ] = prof_value;
  }
}

// Execute Pet Action =======================================================

struct execute_pet_action_t : public action_t
{
  action_t* pet_action;
  pet_t* pet;
  std::string action_str;

  std::string pet_name;

  execute_pet_action_t( player_t* player, const std::string& name, const std::string& as, const std::string& options_str ) :
    action_t( ACTION_OTHER, "execute_" + name + "_" + as, player ),
    pet_action( nullptr ),
    pet( nullptr ),
    action_str( as ),
    pet_name( name )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
  }

  bool init_finished() override
  {
    pet = player -> find_pet( pet_name );
    // No pet found, finish init early, the action will never be ready() and never executed.
    if ( ! pet )
    {
      return true;
    }

    for ( size_t i = 0; i < pet -> action_list.size(); ++i )
    {
      action_t* a = pet -> action_list[ i ];
      if ( a -> name_str == action_str )
      {
        a -> background = true;
        pet_action = a;
      }
    }

    if ( ! pet_action )
    {
      sim -> errorf( "Player %s refers to unknown action %s for pet %s\n",
                     player -> name(), action_str.c_str(), pet -> name() );
      return false;
    }

    return action_t::init_finished();
  }

  virtual void execute() override
  {
    pet_action -> execute();
  }

  virtual bool ready() override
  {
    if ( ! pet )
    {
      return false;
    }

    if ( ! pet_action )
      return false;

    if ( ! action_t::ready() )
      return false;

    if ( pet_action -> player -> is_sleeping() )
      return false;

    return pet_action -> ready();
  }
};

// player_t::init_target ====================================================

void player_t::init_target()
{
  if ( ! target_str.empty() )
  {
    target = sim -> find_player( target_str );
  }
  if ( ! target )
  {
    target = sim -> target;
  }
}

// player_t::init_use_item_actions ==========================================

std::string player_t::init_use_item_actions( const std::string& append )
{
  std::string buffer;

  for ( size_t i = 0; i < items.size(); ++i )
  {
    if ( items[ i ].slot == SLOT_HANDS ) continue;

    if ( items[ i ].has_use_special_effect() )
    {
      buffer += "/use_item,slot=";
      buffer += items[ i ].slot_name();
      if ( ! append.empty() )
      {
        buffer += append;
      }
    }
  }
  if ( items[ SLOT_HANDS ].has_use_special_effect() )
  {
    buffer += "/use_item,slot=";
    buffer += items[ SLOT_HANDS ].slot_name();
    if ( ! append.empty() )
    {
      buffer += append;
    }
  }

  return buffer;
}

std::vector<std::string> player_t::get_item_actions( const std::string& options )
{
  std::vector<std::string> actions;

  for ( const auto& item : items )
  {
    // This will skip Addon and Enchant-based on-use effects. Addons especially are important to
    // skip from the default APLs since they will interfere with the potion timer, which is almost
    // always preferred over an Addon.
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      std::string action_string = "use_item,slot=";
      action_string += item.slot_name();
      if ( ! options.empty() )
      {
        if ( options[ 0 ] != ',' )
        {
          action_string += ',';
        }
        action_string += options;
      }

      actions.push_back( action_string );
    }
  }

  return actions;
}

// player_t::init_use_profession_actions ====================================

std::string player_t::init_use_profession_actions( const std::string& /* append */ )
{
  std::string buffer;

  return buffer;
}

std::vector<std::string> player_t::get_profession_actions()
{
  std::vector<std::string> actions;

  return actions;
}

// player_t::init_use_racial_actions ========================================

std::string player_t::init_use_racial_actions( const std::string& append )
{
  std::string buffer;
  bool race_action_found = false;

  switch ( race )
  {
    case RACE_ORC:
      buffer += "/blood_fury";
      race_action_found = true;
      break;
    case RACE_TROLL:
      buffer += "/berserking";
      race_action_found = true;
      break;
    case RACE_BLOOD_ELF:
      buffer += "/arcane_torrent";
      race_action_found = true;
      break;
    default: break;
  }

  if ( race_action_found && ! append.empty() )
  {
    buffer += append;
  }

  return buffer;
}

std::vector<std::string> player_t::get_racial_actions()
{
  std::vector<std::string> actions;

  actions.push_back( "blood_fury" );
  actions.push_back( "berserking" );
  actions.push_back( "arcane_torrent" );

  return actions;
}

/* Helper function to add actions with spell data of the same name to the action list,
 * and check if that spell data is ok()
 * returns true if spell data is ok(), false otherwise
 */

bool player_t::add_action( std::string action, std::string options, std::string alist )
{
  return add_action( find_class_spell( action ), options, alist );
}

/* Helper function to add actions to the action list if given spell data is ok()
 * returns true if spell data is ok(), false otherwise
 */

bool player_t::add_action( const spell_data_t* s, std::string options, std::string alist )
{
  if ( s -> ok() )
  {
    std::string* str = ( alist == "default" ) ? &action_list_str : &( get_action_priority_list( alist ) -> action_list_str );
    *str += "/" + dbc::get_token( s -> id() );
    if ( ! options.empty() ) *str += "," + options;
    return true;
  }
  return false;
}

/* Adds all on use item actions for all items with their on use effect
 * not excluded in the exclude_effects string
 */

void player_t::activate_action_list( action_priority_list_t* a, bool off_gcd )
{
  if ( off_gcd )
    active_off_gcd_list = a;
  else
    active_action_list = a;
  a -> used = true;
}

struct override_talent_t : action_t
{
  override_talent_t( player_t* player ) : action_t( ACTION_OTHER, "override_talent", player )
  {
    background = true;
  }
};

void player_t::override_talent( std::string& override_str )
{
  std::string::size_type cut_pt = override_str.find( ',' );

  if ( cut_pt != override_str.npos && override_str.substr( cut_pt + 1, 3 ) == "if=" )
  {
    override_talent_t* dummy_action = new override_talent_t( this );
    expr_t* expr = expr_t::parse( dummy_action, override_str.substr( cut_pt + 4 ) );
    if ( ! expr )
      return;
    bool success = expr -> success();
    delete expr;
    if ( ! success )
      return;
    override_str = override_str.substr( 0, cut_pt );
  }

  util::tokenize( override_str );

  // Support disable_row:<row> syntax
  std::string::size_type pos = override_str.find( "disable_row" );
  if ( pos != std::string::npos )
  {
    std::string row_str = override_str.substr( 11 );
    if ( ! row_str.empty() )
    {
      unsigned row = util::to_unsigned( override_str.substr( 11 ) );
      if ( row == 0 || row > MAX_TALENT_ROWS )
      {
        sim -> errorf( "talent_override: Invalid talent row %u for player %s for talent override \"%s\"\n",
          row, name(), override_str.c_str() );
        return;
      }

      talent_points.clear( row - 1 );
      if ( sim -> num_players == 1 )
      {
        sim -> errorf( "talent_override: Talent row %u for player %s disabled\n", row, name() );
      }
      return;
    }
  }

  unsigned spell_id = dbc.talent_ability_id( type, specialization(), override_str.c_str(), true );

  if ( ! spell_id || dbc.spell( spell_id ) ->id() != spell_id )
  {
    sim -> errorf( "talent_override: Override talent %s not found for player %s.\n", override_str.c_str(), name() );
    return;
  }

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_data_t* t = talent_data_t::find( type, j, i, specialization(), dbc.ptr );
      if ( t && ( t -> spell_id() == spell_id ) )
      {
        if ( true_level < std::min( ( j + 1 ) * 15, 100 ) )
        {
          sim -> errorf( "talent_override: Override talent %s is too high level for player %s.\n", override_str.c_str(), name() );
          return;
        }

        if ( sim -> debug )
        {
          if ( talent_points.has_row_col( j, i ) )
          {
            sim -> out_debug.printf( "talent_override: talent %s for player %s is already enabled\n",
                           override_str.c_str(), name() );
          }
        }
        if ( sim -> num_players == 1 )
        { // To prevent spamming up raid reports, only do this with 1 player sims.
          sim -> errorf( "talent_override: talent %s for player %s replaced talent %s in tier %d\n",
                         override_str.c_str(), name(), util::to_string( talent_points.choice( j ) + 1 ).c_str(), j + 1 );
        }
        talent_points.select_row_col( j, i );
      }
    }
  }
}

// player_t::init_talents ===================================================

void player_t::init_talents()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing talents for player (%s)", name() );

  if ( ! talent_overrides_str.empty() )
  {
    std::vector<std::string> splits = util::string_split( talent_overrides_str, "/" );
    for ( size_t i = 0; i < splits.size(); i++ )
    {
      override_talent( splits[ i ] );
    }
  }
}

// player_t::init_artifact ==================================================

void player_t::init_artifact()
{
  if ( ! artifact_enabled() )
  {
    return;
  }

  unsigned artifact_id = dbc.artifact_by_spec( specialization() );

  if ( artifact_id == 0 )
  {
    return;
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing artifact for player (%s)", name() );

  std::vector<const artifact_power_data_t*> powers = dbc.artifact_powers( artifact_id );

  if ( ! artifact_overrides_str.empty() )
  {
    std::vector<std::string> splits = util::string_split( artifact_overrides_str, "/" );
    for ( size_t i = 0; i < splits.size(); i++ )
    {
      override_artifact( powers, splits[ i ] );
    }
  }
}

// player_t::init_glyphs ====================================================

void player_t::init_glyphs()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing glyphs for player (%s)", name() );

  std::vector<std::string> glyph_names = util::string_split( glyphs_str, ",/" );

  for ( size_t i = 0; i < glyph_names.size(); i++ )
  {
    unsigned glyph_id = util::to_unsigned( glyph_names[ i ] );
    const spell_data_t* g;
    if ( glyph_id > 0 && dbc.is_glyph_spell( glyph_id ) )
      g = find_spell( glyph_id );
    else
      g = find_glyph( glyph_names[ i ] );

    if ( g == spell_data_t::not_found() )
      sim -> errorf( "Glyph %s not found in spell database for player %s.", glyph_names[ i ].c_str(), name() );

    if ( g && g -> ok() )
    {
      glyph_list.push_back( g );
    }
  }
}

// player_t::init_spells ====================================================

void player_t::init_spells()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing spells for player (%s)", name() );

  racials.quickness               = find_racial_spell( "Quickness" );
  racials.command                 = find_racial_spell( "Command" );
  racials.arcane_acuity           = find_racial_spell( "Arcane Acuity" );
  racials.heroic_presence         = find_racial_spell( "Heroic Presence" );
  racials.might_of_the_mountain   = find_racial_spell( "Might of the Mountain" );
  racials.expansive_mind          = find_racial_spell( "Expansive Mind" );
  racials.nimble_fingers          = find_racial_spell( "Nimble Fingers" );
  racials.time_is_money           = find_racial_spell( "Time is Money" );
  racials.the_human_spirit        = find_racial_spell( "The Human Spirit" );
  racials.touch_of_elune          = find_racial_spell( "Touch of Elune" );
  racials.brawn                   = find_racial_spell( "Brawn" );
  racials.endurance               = find_racial_spell( "Endurance" );
  racials.viciousness             = find_racial_spell( "Viciousness" );

  if ( ! is_enemy() )
  {
    const spell_data_t* s = find_mastery_spell( specialization() );
    if ( s -> ok() )
      _mastery = &( s -> effectN( 1 ) );
  }

  struct leech_t : public heal_t
  {
    leech_t( player_t* player ) :
      heal_t( "leech", player, player -> find_spell( 143924 ) )
    {
      background = proc = true;
      callbacks = may_crit = may_miss = may_dodge = may_parry = may_block = false;
    }

    void init() override
    {
      heal_t::init();

      snapshot_flags = update_flags = STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_VERSATILITY | STATE_MUL_PERSISTENT;
    }
  };

  if ( record_healing() )
  {
    spell.leech = new leech_t( this );
  }
  else
  {
    spell.leech = 0;
  }

  if ( artifact.n_purchased_points > 0 )
  {
    auto artifact_id = dbc.artifact_by_spec( specialization() );
    auto artifact_powers = dbc.artifact_powers( artifact_id );

    auto damage_it = range::find_if( artifact_powers, [ this ]( const artifact_power_data_t* power ) {
      auto spell_data = find_spell( power -> power_spell_id );
      return util::str_compare_ci( spell_data -> name_cstr(), "Artificial Damage" );
    } );

    artifact.artificial_damage = damage_it != artifact_powers.end()
      ? find_spell( ( *damage_it ) -> power_spell_id )
      : spell_data_t::not_found();

    auto stamina_it = range::find_if( artifact_powers, [ this ]( const artifact_power_data_t* power ) {
      auto spell_data = find_spell( power -> power_spell_id );
      return util::str_compare_ci( spell_data -> name_cstr(), "Artificial Stamina" );
    } );

    artifact.artificial_stamina = stamina_it != artifact_powers.end()
      ? find_spell( ( *stamina_it ) -> power_spell_id )
      : spell_data_t::not_found();
  }
}

// player_t::init_gains =====================================================

void player_t::init_gains()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing gains for player (%s)", name() );

  gains.arcane_torrent         = get_gain( "arcane_torrent" );
  gains.endurance_of_niuzao    = get_gain( "endurance_of_niuzao" );
  gains.energy_regen           = get_gain( "energy_regen" );
  gains.focus_regen            = get_gain( "focus_regen" );
  gains.health                 = get_gain( "external_healing" );
  gains.mana_potion            = get_gain( "mana_potion" );
  gains.mp5_regen              = get_gain( "mp5_regen" );
  gains.restore_mana           = get_gain( "restore_mana" );
  gains.touch_of_the_grave     = get_gain( "touch_of_the_grave" );
  gains.vampiric_embrace       = get_gain( "vampiric_embrace" );
  gains.leech                  = get_gain( "leech" );
}

// player_t::init_procs =====================================================

void player_t::init_procs()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing procs for player (%s)", name() );

  procs.parry_haste  = get_proc( "parry_haste" );
}

// player_t::init_uptimes ===================================================

void player_t::init_uptimes()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing uptimes for player (%s)", name() );

  uptimes.primary_resource_cap = get_uptime( util::inverse_tokenize( util::resource_type_string( primary_resource() ) ) +  " Cap" );
}

// player_t::init_benefits ==================================================

void player_t::init_benefits()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing benefits for player (%s)", name() );
}

// player_t::init_rng =======================================================

void player_t::init_rng()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing rngs for player (%s)", name() );
}

// player_t::init_stats =====================================================

void player_t::init_stats()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing stats for player (%s)", name() );

  range::fill( iteration_resource_lost, 0.0 );
  range::fill( iteration_resource_gained, 0.0 );

  if ( sim -> maximize_reporting )
  {
    for ( stat_e s = STAT_NONE; s < STAT_MAX; ++s )
    {
      stat_timelines.push_back( s );
    }
  }
  if ( ! is_pet() || sim -> report_pets_separately )
  {
    if ( collected_data.stat_timelines.size() == 0 )
    {
      for ( size_t i = 0; i < stat_timelines.size(); ++i )
      {
        collected_data.stat_timelines.push_back( player_collected_data_t::stat_timeline_t( stat_timelines[ i ] ) );
      }
    }
  }

  collected_data.reserve_memory( *this );
}

// player_t::init_absorb_priority ===========================================

void player_t::init_absorb_priority()
{
  /* Absorbs with the high priority flag will follow the order in absorb_priority
     vector when resolving. This method should be overwritten in class modules to
     declare the order of spec-specific high priority absorbs. */
}

// player_t::init_scaling ===================================================

void player_t::init_scaling()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing scaling for player (%s)", name() );

  if ( ! is_pet() && ! is_enemy() )
  {
    invert_scaling = 0;

    role_e role = primary_role();

    bool attack = ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK || role == ROLE_DPS );
    bool spell  = ( role == ROLE_SPELL  || role == ROLE_HYBRID || role == ROLE_HEAL || role == ROLE_DPS );
    bool tank   = ( role == ROLE_TANK );
    bool heal   = ( role == ROLE_HEAL );

    scales_with[ STAT_STRENGTH  ] = attack;
    scales_with[ STAT_AGILITY   ] = attack;
    scales_with[ STAT_STAMINA   ] = tank;
    scales_with[ STAT_INTELLECT ] = spell;
    scales_with[ STAT_SPIRIT    ] = heal;

    scales_with[ STAT_HEALTH ] = false;
    scales_with[ STAT_MANA   ] = false;
    scales_with[ STAT_RAGE   ] = false;
    scales_with[ STAT_ENERGY ] = false;
    scales_with[ STAT_FOCUS  ] = false;
    scales_with[ STAT_RUNIC  ] = false;

    scales_with[ STAT_SPELL_POWER               ] = spell;
    scales_with[ STAT_ATTACK_POWER              ] = attack;
    scales_with[ STAT_CRIT_RATING               ] = true;
    scales_with[ STAT_HASTE_RATING              ] = true;
    scales_with[ STAT_MASTERY_RATING            ] = true;
    scales_with[ STAT_VERSATILITY_RATING        ] = true;

    bool has_movement = false;
    for ( const auto& raid_event : sim -> raid_events )
    {
      if ( util::str_in_str_ci( raid_event -> name(), "movement" ) )
        has_movement = true;
    }

    scales_with[ STAT_SPEED_RATING              ] = has_movement;
    // scales_with[ STAT_AVOIDANCE_RATING          ] = tank; // Waste of sim time vast majority of the time. Can be enabled manually.
    scales_with[ STAT_LEECH_RATING              ] = tank;

    scales_with[ STAT_WEAPON_DPS   ] = attack;
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = false;

    scales_with[ STAT_ARMOR          ] = tank;

    if ( sim -> scaling -> scale_stat != STAT_NONE && scale_player )
    {
      double v = sim -> scaling -> scale_value;

      switch ( sim -> scaling -> scale_stat )
      {
        case STAT_STRENGTH:  initial.stats.attribute[ ATTR_STRENGTH  ] += v; break;
        case STAT_AGILITY:   initial.stats.attribute[ ATTR_AGILITY   ] += v; break;
        case STAT_STAMINA:   initial.stats.attribute[ ATTR_STAMINA   ] += v; break;
        case STAT_INTELLECT: initial.stats.attribute[ ATTR_INTELLECT ] += v; break;
        case STAT_SPIRIT:    initial.stats.attribute[ ATTR_SPIRIT    ] += v; break;

        case STAT_SPELL_POWER:
          initial.stats.spell_power += v;
          break;

        case STAT_ATTACK_POWER:
          initial.stats.attack_power += v;
          break;

        case STAT_CRIT_RATING:
          initial.stats.crit_rating += v;
          break;

        case STAT_HASTE_RATING:
          initial.stats.haste_rating += v;
          break;

        case STAT_MASTERY_RATING:
          initial.stats.mastery_rating += v;
          break;

        case STAT_VERSATILITY_RATING:
          initial.stats.versatility_rating += v;
          break;

        case STAT_DODGE_RATING:
          initial.stats.dodge_rating += v;
          break;

        case STAT_PARRY_RATING:
          initial.stats.parry_rating += v;
          break;

        case STAT_SPEED_RATING:
          initial.stats.speed_rating += v;
          break;

        case STAT_AVOIDANCE_RATING:
          initial.stats.avoidance_rating += v;
          break;

        case STAT_LEECH_RATING:
          initial.stats.leech_rating += v;
          break;

        case STAT_WEAPON_DPS:
          if ( main_hand_weapon.damage > 0 )
          {
            main_hand_weapon.damage  += main_hand_weapon.swing_time.total_seconds() * v;
            main_hand_weapon.min_dmg += main_hand_weapon.swing_time.total_seconds() * v;
            main_hand_weapon.max_dmg += main_hand_weapon.swing_time.total_seconds() * v;
          }
          break;

        case STAT_WEAPON_OFFHAND_DPS:
          if ( off_hand_weapon.damage > 0 )
          {
            off_hand_weapon.damage   += off_hand_weapon.swing_time.total_seconds() * v;
            off_hand_weapon.min_dmg  += off_hand_weapon.swing_time.total_seconds() * v;
            off_hand_weapon.max_dmg  += off_hand_weapon.swing_time.total_seconds() * v;
          }
          break;

        case STAT_ARMOR:          initial.stats.armor       += v; break;

        case STAT_BONUS_ARMOR:    initial.stats.bonus_armor += v; break;

        case STAT_BLOCK_RATING:   initial.stats.block_rating       += v; break;

        case STAT_MAX: break;

        default: assert( false ); break;
      }
    }
  }
}

// player_t::create_actions ================================================

bool player_t::create_actions()
{
  if ( action_list_str.empty() )
    no_action_list_provided = true;

  init_action_list(); // virtual function which creates the action list string

  std::string modify_action_options;

  if ( ! modify_action.empty() )
  {
    std::string::size_type cut_pt = modify_action.find( ',' );

    if ( cut_pt != modify_action.npos )
    {
      modify_action_options = modify_action.substr( cut_pt + 1 );
      modify_action         = modify_action.substr( 0, cut_pt );
    }
  }
  util::tokenize( modify_action );

  std::vector<std::string> skip_actions;
  if ( ! action_list_skip.empty() )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "Player %s: action_list_skip=%s", name(), action_list_skip.c_str() );

    skip_actions = util::string_split( action_list_skip, "/" );
  }

  if ( ! use_apl.empty() )
    copy_action_priority_list( "default", use_apl );

  if ( ! action_list_str.empty() )
    get_action_priority_list( "default" ) -> action_list_str = action_list_str;

  int j = 0;

  auto apls = sorted_action_priority_lists( this );
  for ( auto apl : apls )
  {
    assert( ! ( ! apl -> action_list_str.empty() &&
                ! apl -> action_list.empty() ) );

    // Convert old style action list to new style, all lines are without comments
    if ( ! apl -> action_list_str.empty() )
    {
      std::vector<std::string> splits = util::string_split( apl -> action_list_str, "/" );
      for ( size_t i = 0; i < splits.size(); i++ )
        apl -> action_list.push_back( action_priority_t( splits[ i ], "" ) );
    }

    if ( sim -> debug )
      sim -> out_debug.printf( "Player %s: actions.%s=%s", name(),
                     apl -> name_str.c_str(),
                     apl -> action_list_str.c_str() );

    for ( size_t i = 0; i < apl -> action_list.size(); i++ )
    {
      std::string& action_str = apl -> action_list[ i ].action_;

      std::string::size_type cut_pt = action_str.find( ',' );
      std::string action_name = action_str.substr( 0, cut_pt );
      std::string action_options;
      if ( cut_pt != std::string::npos )
        action_options = action_str.substr( cut_pt + 1 );

      action_t* a = NULL;

      cut_pt = action_name.find( ':' );
      if ( cut_pt != std::string::npos )
      {
        std::string pet_name   = action_name.substr( 0, cut_pt );
        std::string pet_action = action_name.substr( cut_pt + 1 );

        util::tokenize( pet_action );

        a = new execute_pet_action_t( this, pet_name, pet_action, action_options );
      }
      else
      {
        util::tokenize( action_name );
        if ( action_name == modify_action )
        {
          if ( sim -> debug )
            sim -> out_debug.printf( "Player %s: modify_action=%s", name(), modify_action.c_str() );

          action_options = modify_action_options;
          action_str = modify_action + "," + modify_action_options;
        }
        a = create_action( action_name, action_options );
      }

      if ( a )
      {
        bool skip = false;
        for ( size_t k = 0; k < skip_actions.size(); k++ )
        {
          if ( skip_actions[ k ] == a -> name_str )
          {
            skip = true;
            break;
          }
        }

        if ( skip )
        {
          a -> background = true;
        }
        else
        {
          //a -> action_list = action_priority_list[ alist ] -> name_str;
          a -> action_list = apl;

          a -> signature_str = action_str;
          a -> signature = &( apl -> action_list[ i ] );

          if ( sim -> separate_stats_by_actions > 0 && !is_pet() )
          {
            a -> marker = ( char ) ( ( j < 10 ) ? ( '0' + j      ) :
                          ( j < 36 ) ? ( 'A' + j - 10 ) :
                          ( j < 66 ) ? ( 'a' + j - 36 ) :
                          ( j < 79 ) ? ( '!' + j - 66 ) :
                          ( j < 86 ) ? ( ':' + j - 79 ) : '.' );

            a -> stats = get_stats( a -> name_str + "__" + a -> marker, a );
          }
          j++;
        }
      }
      else
      {
        sim -> errorf( "Player %s unable to create action: %s\n", name(), action_str.c_str() );
        sim -> cancel();
        return false;
      }
    }
  }

  bool have_off_gcd_actions = false;
  for ( auto action: action_list )
  {
    if ( action -> trigger_gcd == timespan_t::zero() && ! action -> background &&
         action -> use_off_gcd )
    {
      action -> action_list -> off_gcd_actions.push_back( action );
      // Optimization: We don't need to do off gcd stuff when there are no other off gcd actions
      // than these two
      if ( action -> name_str != "run_action_list" && action -> name_str != "swap_action_list" )
        have_off_gcd_actions = true;
    }
  }

  if ( choose_action_list.empty() ) choose_action_list = "default";

  action_priority_list_t* chosen_action_list = find_action_priority_list( choose_action_list );

  if ( ! chosen_action_list && choose_action_list != "default" )
  {
    sim -> errorf( "Action List %s not found, using default action list.\n",
      choose_action_list.c_str() );
    chosen_action_list = find_action_priority_list( "default" );
  }

  if ( chosen_action_list )
  {
    activate_action_list( chosen_action_list );
    if ( have_off_gcd_actions ) activate_action_list( chosen_action_list, true );
  }
  else
  {
    sim -> errorf( "No Default Action List available.\n" );
  }

  int capacity = std::max( 1200, static_cast<int>( sim -> max_time.total_seconds() / 2.0 ) );
  collected_data.action_sequence.reserve( capacity );
  collected_data.action_sequence.clear();

  return true;
}


// player_t::init_actions ===================================================

bool player_t::init_actions()
{
  for ( size_t i = 0; i < action_list.size(); ++i )
  {
    action_list[ i ] -> init();
  }

  range::for_each( action_list, []( action_t* a ) { a -> consolidate_snapshot_flags(); } );

  return true;
}

// player_t::init_assessors =================================================
void player_t::init_assessors()
{

  // Target related mitigation
  assessor_out_damage.add( assessor::TARGET_MITIGATION, []( dmg_e dmg_type, action_state_t* state )
  {
    state -> target -> assess_damage( state -> action -> get_school(), dmg_type, state );
    return assessor::CONTINUE;
  } );

  // Target damage
  assessor_out_damage.add( assessor::TARGET_DAMAGE, []( dmg_e, action_state_t* state )
  {
    state -> target -> do_damage( state );
    return assessor::CONTINUE;
  } );

  // Logging and debug .. Technically, this should probably be in action_t::assess_damage, but we
  // don't need this piece of code for the vast majority of sims, so it makes sense to yank it out
  // completely from there, and only conditionally include it if logging/debugging is enabled.
  if ( sim -> log || sim -> debug || sim -> debug_seed.size() > 0 )
  {
    assessor_out_damage.add( assessor::LOG, [ this ]( dmg_e type, action_state_t* state )
    {
      if ( sim -> debug )
      {
        state -> debug();
      }

      if ( sim -> log )
      {
        if ( type == DMG_DIRECT )
        {
          sim -> out_log.printf( "%s %s hits %s for %.0f %s damage (%s)",
                         name(), state -> action -> name(),
                         state -> target -> name(), state -> result_amount,
                         util::school_type_string( state -> action -> get_school() ),
                         util::result_type_string( state -> result ) );
        }
        else // DMG_OVER_TIME
        {
          dot_t* dot = state -> action -> get_dot( state -> target );
          sim -> out_log.printf( "%s %s ticks (%d of %d) %s for %.0f %s damage (%s)",
                         name(), state -> action -> name(),
                         dot -> current_tick, dot -> num_ticks,
                         state -> target -> name(), state -> result_amount,
                         util::school_type_string( state -> action -> get_school() ),
                         util::result_type_string( state -> result ) );
        }
      }
      return assessor::CONTINUE;
    } );
  }

  // Leech, if the player has leeching enabled (disabled by default)
  if ( spell.leech )
  {
    assessor_out_damage.add( assessor::LEECH, [ this ]( dmg_e, action_state_t* state )
    {
      // Leeching .. sanity check that the result type is a damaging one, so things hopefully don't
      // break in the future if we ever decide to not separate heal and damage assessing.
      double leech_pct = 0;
      if ( ( state -> result_type == DMG_DIRECT || state -> result_type == DMG_OVER_TIME ) &&
        state -> result_amount > 0 &&
        ( leech_pct = state -> action -> composite_leech( state ) ) > 0 )
      {
        double leech_amount = leech_pct * state -> result_amount;
        spell.leech -> base_dd_min = spell.leech -> base_dd_max = leech_amount;
        spell.leech -> schedule_execute();
      }
      return assessor::CONTINUE;
    } );
  }

  // Generic actor callbacks
  assessor_out_damage.add( assessor::CALLBACKS, [ this ]( dmg_e, action_state_t* state )
  {
    if ( ! state -> action -> callbacks )
    {
      return assessor::CONTINUE;
    }

    proc_types pt = state -> proc_type();
    proc_types2 pt2 = state -> impact_proc_type2();
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      action_callback_t::trigger( callbacks.procs[ pt ][ pt2 ], state -> action, state );

    return assessor::CONTINUE;
  } );
}

// player_t::init_finished ==================================================

bool player_t::init_finished()
{
  bool ret = true;

  for ( size_t i = 0, end = action_list.size(); i < end; ++i )
  {
    if ( ! action_list[ i ] -> init_finished() )
    {
      ret = false;
    }
  }

  // Naive recording of minimum energy thresholds for the actor.
  // TODO: Energy pooling, and energy-based expressions (energy>=10) are not included yet
  for ( size_t i = 0; i < action_list.size(); ++i )
  {
    if ( ! action_list[ i ] -> background && action_list[ i ] -> base_costs[ primary_resource() ] > 0 )
    {
      if ( std::find( resource_thresholds.begin(), resource_thresholds.end(),
            action_list[ i ] -> base_costs[ primary_resource() ] ) == resource_thresholds.end() )
      {
        resource_thresholds.push_back( action_list[ i ] -> base_costs[ primary_resource() ] );
      }
    }
  }

  std::sort( resource_thresholds.begin(), resource_thresholds.end() );

  range::for_each( cooldown_list, [ this ]( cooldown_t* c ) {
    if ( c -> hasted )
    {
      dynamic_cooldown_list.push_back( c );
    }
  } );

  // Sort outbound assessors
  assessor_out_damage.sort();

  if ( sim -> debug )
  {
    range::for_each( items, [ this ]( const item_t& item ) {
      if ( item.active() )
      {
        sim -> out_debug << item.to_string();
      }
    } );
  }

  return ret;
}

// player_t::min_threshold_trigger ==========================================

// Add a wake-up call at the next resource threshold level, compared to the current resource status
// of the actor
void player_t::min_threshold_trigger()
{
  if ( ready_type == READY_POLL )
  {
    return;
  }

  if ( resource_thresholds.size() == 0 )
  {
    return;
  }

  resource_e pres = primary_resource();
  if ( pres != RESOURCE_MANA && pres != RESOURCE_ENERGY && pres != RESOURCE_FOCUS )
  {
    return;
  }

  size_t i = 0, end = resource_thresholds.size();
  double threshold = 0;
  for ( ; i < end; ++i )
  {
    if ( resources.current[ pres ] < resource_thresholds[ i ] )
    {
      threshold = resource_thresholds[ i ];
      break;
    }
  }

  timespan_t time_to_threshold = timespan_t::zero();
  if ( i < resource_thresholds.size() )
  {
    double rps = 0;
    switch ( pres )
    {
      case RESOURCE_MANA:
        rps = mana_regen_per_second();
        break;
      case RESOURCE_ENERGY:
        rps = energy_regen_per_second();
        break;
      case RESOURCE_FOCUS:
        rps = focus_regen_per_second();
        break;
      default:
        break;
    }

    if ( rps > 0 )
    {
      double diff = threshold - resources.current[ pres ];
      time_to_threshold = timespan_t::from_seconds( diff / rps );
    }
  }

  // If time to threshold is zero, there's no need to reset anything
  if ( time_to_threshold <= timespan_t::zero() )
  {
    return;
  }

  timespan_t occurs = sim -> current_time() + time_to_threshold;
  if ( resource_threshold_trigger )
  {
    // We should never ever be doing threshold-based wake up calls if there already is a
    // Player-ready event.
    assert( ! readying );

    if ( occurs > resource_threshold_trigger -> occurs() )
    {
      resource_threshold_trigger -> reschedule( time_to_threshold );
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "Player %s rescheduling Resource-Threshold event: threshold=%.1f delay=%.3f",
            name(), threshold, time_to_threshold.total_seconds() );
      }
    }
    else if ( occurs < resource_threshold_trigger -> occurs() )
    {
      event_t::cancel( resource_threshold_trigger );
      resource_threshold_trigger = make_event<resource_threshold_event_t>( *sim, this, time_to_threshold );
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "Player %s recreating Resource-Threshold event: threshold=%.1f delay=%.3f",
            name(), threshold, time_to_threshold.total_seconds() );
      }
    }
  }
  else
  {
    // We should never ever be doing threshold-based wake up calls if there already is a
    // Player-ready event.
    assert( ! readying );

    resource_threshold_trigger = make_event<resource_threshold_event_t>( *sim, this, time_to_threshold );
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "Player %s scheduling new Resource-Threshold event: threshold=%.1f delay=%.3f",
          name(), threshold, time_to_threshold.total_seconds() );
    }
  }
}

// player_t::create_buffs ===================================================

void player_t::create_buffs()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Creating Auras, Buffs, and Debuffs for player (%s)", name() );

  if ( ! is_enemy() && type != PLAYER_GUARDIAN )
  {
    if ( type != PLAYER_PET )
    {
      // Racials
      buffs.berserking                = haste_buff_creator_t( this, "berserking", find_spell( 26297 ) ).add_invalidate( CACHE_HASTE );
      buffs.stoneform                 = buff_creator_t( this, "stoneform", find_spell( 65116 ) );
      buffs.blood_fury                = stat_buff_creator_t( this, "blood_fury" )
                                        .spell( find_racial_spell( "Blood Fury" ) )
                                        .add_invalidate( CACHE_SPELL_POWER )
                                        .add_invalidate( CACHE_ATTACK_POWER );
      buffs.fortitude                 = buff_creator_t( this, "fortitude", find_spell( 137593 ) ).activated( false );
      buffs.shadowmeld                = buff_creator_t( this, "shadowmeld", find_spell( 58984 ) ).cd( timespan_t::zero() );

      buffs.archmages_greater_incandescence_agi = buff_creator_t( this, "archmages_greater_incandescence_agi", find_spell( 177172 ) )
        .add_invalidate( CACHE_AGILITY );
      buffs.archmages_greater_incandescence_str = buff_creator_t( this, "archmages_greater_incandescence_str", find_spell( 177175 ) )
        .add_invalidate( CACHE_STRENGTH );
      buffs.archmages_greater_incandescence_int = buff_creator_t( this, "archmages_greater_incandescence_int", find_spell( 177176 ) )
        .add_invalidate( CACHE_INTELLECT );

      buffs.archmages_incandescence_agi = buff_creator_t( this, "archmages_incandescence_agi", find_spell( 177161 ) )
        .add_invalidate( CACHE_AGILITY );
      buffs.archmages_incandescence_str = buff_creator_t( this, "archmages_incandescence_str", find_spell( 177160 ) )
        .add_invalidate( CACHE_STRENGTH );
      buffs.archmages_incandescence_int = buff_creator_t( this, "archmages_incandescence_int", find_spell( 177159 ) )
        .add_invalidate( CACHE_INTELLECT );

      // Legendary meta haste buff
      buffs.tempus_repit              = buff_creator_t( this, "tempus_repit", find_spell( 137590 ) ).add_invalidate( CACHE_HASTE ).activated( false );

      buffs.darkflight         = buff_creator_t( this, "darkflight", find_racial_spell( "darkflight" ) );

      buffs.nitro_boosts       = buff_creator_t( this, "nitro_boosts", find_spell( 54861 ) );

      buffs.amplification = buff_creator_t( this, "amplification", find_spell( 146051 ) )
                            .add_invalidate( CACHE_MASTERY )
                            .add_invalidate( CACHE_HASTE )
                            .add_invalidate( CACHE_SPIRIT )
                            .chance( 0 );
      buffs.amplification_2 = buff_creator_t( this, "amplification_2", find_spell( 146051 ) )
                              .add_invalidate( CACHE_MASTERY )
                              .add_invalidate( CACHE_HASTE )
                              .add_invalidate( CACHE_SPIRIT )
                              .chance( 0 );

      buffs.temptation = buff_creator_t( this, "temptation", find_spell( 234143 ) )
        .cd( timespan_t::zero() )
        .chance( 1 )
        .default_value( 0.1 ); //Not in spelldata
    }
  }

  buffs.courageous_primal_diamond_lucidity = buff_creator_t( this, "lucidity" )
                                             .spell( find_spell( 137288 ) );

  buffs.body_and_soul = buff_creator_t( this, "body_and_soul" )
                        .spell( find_spell( 64129 ) )
                        .max_stack( 1 )
                        .duration( timespan_t::from_seconds( 4.0 ) );

  buffs.angelic_feather = buff_creator_t( this, "angelic_feather" )
                          .spell( find_spell( 121557 ) )
                          .max_stack( 1 )
                          .duration( timespan_t::from_seconds( 6.0 ) );

  struct raid_movement_buff_t : public buff_t
  {
    raid_movement_buff_t( player_t* p ) :
      buff_t( buff_creator_t( p, "raid_movement" ).max_stack( 1 ) )
    { }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );
      player -> finish_moving();
    }
  };

  buffs.raid_movement = new raid_movement_buff_t( this );
  buffs.self_movement = buff_creator_t( this, "self_movement" ).max_stack( 1 );

  // Infinite-Stacking Buffs and De-Buffs
  buffs.stunned        = buff_creator_t( this, "stunned" ).max_stack( 1 );
  debuffs.bleeding     = buff_creator_t( this, "bleeding" ).max_stack( 1 );
  debuffs.casting      = buff_creator_t( this, "casting" ).max_stack( 1 ).quiet( 1 );
  debuffs.invulnerable = buff_creator_t( this, "invulnerable" ).max_stack( 1 );
  debuffs.vulnerable   = buff_creator_t( this, "vulnerable" ).max_stack( 1 );
  debuffs.flying       = buff_creator_t( this, "flying" ).max_stack( 1 );
  debuffs.dazed        = buff_creator_t( this, "dazed", find_spell( 15571 ) );
  debuffs.damage_taken = buff_creator_t( this, "damage_taken" ).duration( timespan_t::from_seconds( 20.0 ) ).max_stack( 999 );

  // stuff moved from old init_debuffs method

  debuffs.mortal_wounds           = buff_creator_t( this, "mortal_wounds", find_spell( 115804 ) )
                                    .default_value( std::fabs( find_spell( 115804 ) -> effectN( 1 ).percent() ) );
}

// player_t::find_item ======================================================

item_t* player_t::find_item( const std::string& str )
{
  for ( size_t i = 0; i < items.size(); i++ )
    if ( str == items[ i ].name() )
      return &items[ i ];

  return 0;
}

// player_t::has_t18_class_trinket ============================================

bool player_t::has_t18_class_trinket() const
{
  // Class modules should override this with their individual trinket detection
  return false;
}

// player_t::level ==========================================================

int player_t::level() const
{
  if ( sim -> timewalk > 0 && ! is_enemy() )
    return sim -> timewalk;
  else
    return true_level;
}

// player_t::energy_regen_per_second ========================================

double player_t::energy_regen_per_second() const
{
  double r = 0;
  if ( base_energy_regen_per_second )
    r = base_energy_regen_per_second * ( 1.0 / cache.attack_haste() );

  if ( buffs.surge_of_energy && buffs.surge_of_energy -> up() )
  {
    r *= 1.0 + buffs.surge_of_energy -> data().effectN( 1 ).percent();
  }
  return r;
}

// player_t::focus_regen_per_second =========================================

double player_t::focus_regen_per_second() const
{
  double r = 0;
  if ( base_focus_regen_per_second )
    r = base_focus_regen_per_second * ( 1.0 / cache.attack_haste() );
  return r;
}

// player_t::mana_regen_per_second ==========================================

double player_t::mana_regen_per_second() const
{
  return current.mana_regen_per_second + cache.spirit() * current.mana_regen_per_spirit * current.mana_regen_from_spirit_multiplier;
}

// player_t::composite_attack_haste =========================================

double player_t::composite_melee_haste() const
{
  double h;

  h = std::max( 0.0, composite_melee_haste_rating() ) / current.rating.attack_haste;

  h = 1.0 / ( 1.0 + h );

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.bloodlust -> up() )
      h *= 1.0 / ( 1.0 + buffs.bloodlust -> data().effectN( 1 ).percent() );

    if ( buffs.mongoose_mh && buffs.mongoose_mh -> up() )
      h *= 1.0 / ( 1.0 + 30 / current.rating.attack_haste );

    if ( buffs.mongoose_oh && buffs.mongoose_oh -> up() )
      h *= 1.0 / ( 1.0 + 30 / current.rating.attack_haste );

    if ( buffs.berserking -> up() )
      h *= 1.0 / ( 1.0 + buffs.berserking -> data().effectN( 1 ).percent() );

    h *= 1.0 / ( 1.0 + racials.nimble_fingers -> effectN( 1 ).percent() );
    h *= 1.0 / ( 1.0 + racials.time_is_money -> effectN( 1 ).percent() );

    if ( timeofday == NIGHT_TIME )
       h *= 1.0 / ( 1.0 + racials.touch_of_elune -> effectN( 1 ).percent() );
  }

  return h;
}

// player_t::composite_attack_speed =========================================

double player_t::composite_melee_speed() const
{
  double h = composite_melee_haste();

  if ( buffs.fel_winds && buffs.fel_winds -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.fel_winds -> value() );
  }

  return h;
}

// player_t::composite_attack_power =========================================

double player_t::composite_melee_attack_power() const
{
  double ap = current.stats.attack_power;

  ap += current.attack_power_per_strength * cache.strength();
  ap += current.attack_power_per_agility  * cache.agility();

  return ap;
}

// player_t::composite_attack_power_multiplier ==============================

double player_t::composite_attack_power_multiplier() const
{
  return current.attack_power_multiplier;
}

// player_t::composite_attack_crit_chance ==========================================

double player_t::composite_melee_crit_chance() const
{
  double ac = current.attack_crit_chance + composite_melee_crit_rating() / current.rating.attack_crit;

  if ( current.attack_crit_per_agility )
    ac += ( cache.agility() / current.attack_crit_per_agility / 100.0 );

  ac += racials.viciousness -> effectN( 1 ).percent();
  ac += racials.arcane_acuity -> effectN( 1 ).percent();

  if ( timeofday == DAY_TIME )
    ac += racials.touch_of_elune -> effectN( 1 ).percent();

  return ac;
}

// player_t::composite_melee_expertise =====================================

double player_t::composite_melee_expertise( const weapon_t* ) const
{
  double e = current.expertise;

  //e += composite_expertise_rating() / current.rating.expertise;

  return e;
}

// player_t::composite_attack_hit ===========================================

double player_t::composite_melee_hit() const
{
  double ah = current.hit;

  //ah += composite_melee_hit_rating() / current.rating.attack_hit;

  return ah;
}

// player_t::composite_armor ================================================

double player_t::composite_armor() const
{
  double a = current.stats.armor;

  a *= composite_armor_multiplier();

  // Traditionally, armor multipliers have only applied to base armor from gear
  // and not bonus armor. I'm assuming this will continue in WoD.
  //TODO: need to test in beta to be sure - Theck, 4/26/2014
  a += current.stats.bonus_armor;

  return a;
}

// player_t::composite_armor_multiplier =====================================

double player_t::composite_armor_multiplier() const
{
  double a = current.armor_multiplier;

  if ( meta_gem == META_AUSTERE_PRIMAL )
  {
    a += 0.02;
  }

  return a;
}

// player_t::composite_miss ============================================

double player_t::composite_miss() const
{
  // this is sort of academic, since we know of no sources of miss that are actually subject to DR.
  // However, we've been told that it follows the same DR formula as the other avoidance stats, and
  // we've been given the relevant constants (def_dr.miss_factor etc.). I've coded this mostly for symmetry,
  // such that it will be fully-functional if in the future we suddenly gain some source of bonus_miss.

  // Start with sources not subject to DR - base miss (stored in current.miss)
  double total_miss = current.miss;

  // bonus_miss is miss from rating or other sources subject to DR
  double bonus_miss = 0.0;

  // if we have any bonus_miss, apply diminishing returns and add it to total_miss
  if ( bonus_miss > 0 )
    total_miss += bonus_miss / ( def_dr.miss_factor * bonus_miss * 100 * def_dr.vertical_stretch + def_dr.horizontal_shift );

  assert( total_miss >= 0.0 && total_miss <= 1.0 );

  return total_miss;
}

// player_t::composite_block ===========================================
// Two methods here.  The first has no arguments and is the method we override in
// class modules (for example, to add spec/talent/etc.-based block contributions).
// The second method accepts a dobule and handles base block and diminishing returns.
// See paladin_t::composite_block() to see how this works.

double player_t::composite_block() const
{
  return player_t::composite_block_dr( 0.0 );
}

double player_t::composite_block_dr( double extra_block ) const
{
  // Start with sources not subject to DR - base block (stored in current.block)
  double total_block = current.block;

  // bonus_block is block from rating or other sources subject to DR (passed from class module via extra_block)
  double bonus_block = composite_block_rating() / current.rating.block;
  bonus_block += extra_block;

  // bonus_block gets rounded because that's how blizzard rolls...
  bonus_block = util::round( 12800 * bonus_block ) / 12800;

  // if we have any bonus_block, apply diminishing returns and add it to total_block
  if ( bonus_block > 0 )
    total_block += bonus_block / ( def_dr.block_factor * bonus_block * 100 * def_dr.vertical_stretch + def_dr.horizontal_shift );

  return total_block;
}

// player_t::composite_dodge ===========================================

double player_t::composite_dodge() const
{
  // Start with sources not subject to DR - base dodge (stored in current.dodge). Base stats no longer give dodge/parry.
  double total_dodge = current.dodge;

  // bonus_dodge is from rating and bonus Agility
  double bonus_dodge = composite_dodge_rating() / current.rating.dodge;
  bonus_dodge += cache.agility() * current.dodge_per_agility;

  // but not class base agility or racial modifiers (irrelevant for enemies)
  if ( ! is_enemy() )
    bonus_dodge -= ( dbc.attribute_base( type, level() ).agility + dbc.race_base( race ).agility ) * current.dodge_per_agility;

  // if we have any bonus_dodge, apply diminishing returns and add it to total_dodge.
  if ( bonus_dodge != 0 )
    total_dodge += bonus_dodge / ( def_dr.dodge_factor * bonus_dodge * 100 * def_dr.vertical_stretch + def_dr.horizontal_shift );

  return total_dodge;
}

// player_t::composite_parry ===========================================

double player_t::composite_parry() const
{
  // Start with sources not subject to DR - base parry (stored in current.parry). Base stats no longer give dodge/parry.
  double total_parry = current.parry;

  // bonus_parry is from rating and bonus Strength
  double bonus_parry = composite_parry_rating() / current.rating.parry;
  bonus_parry += cache.strength() * current.parry_per_strength;

  // but not class base strength or racial modifiers (irrelevant for enemies)
  if ( ! is_enemy() )
    bonus_parry -= ( dbc.attribute_base( type, level() ).strength + dbc.race_base( race ).strength ) * current.parry_per_strength;

  // if we have any bonus_parry, apply diminishing returns and add it to total_parry.
  if ( bonus_parry != 0 )
    total_parry += bonus_parry / ( def_dr.parry_factor * bonus_parry * 100 * def_dr.vertical_stretch + def_dr.horizontal_shift );


  return total_parry;
}

// player_t::composite_block_reduction =================================

double player_t::composite_block_reduction() const
{
  double b = current.block_reduction;

  if ( meta_gem == META_ETERNAL_SHADOWSPIRIT  ||
       meta_gem == META_ETERNAL_PRIMAL )
  {
    b += 0.01;
  }

  return b;
}

// player_t::composite_crit_block ======================================

double player_t::composite_crit_block() const
{
  return 0;
}

// player_t::composite_crit_avoidance========================================

double player_t::composite_crit_avoidance() const
{
  return 0;
}

// player_t::composite_spell_haste ==========================================

double player_t::composite_spell_haste() const
{
  double h;

  h = std::max( 0.0, composite_spell_haste_rating() ) / current.rating.spell_haste;

  h = 1.0 / ( 1.0 + h );

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.bloodlust -> up() )
      h *= 1.0 / ( 1.0 + buffs.bloodlust -> data().effectN( 1 ).percent() );

    if ( buffs.berserking -> up() )
      h *= 1.0 / ( 1.0 + buffs.berserking -> data().effectN( 1 ).percent() );

    if ( buffs.tempus_repit -> up() )
      h *= 1.0 / ( 1.0 + buffs.tempus_repit -> data().effectN( 1 ).percent() );

    h *= 1.0 / ( 1.0 + racials.nimble_fingers -> effectN( 1 ).percent() );
    h *= 1.0 / ( 1.0 + racials.time_is_money -> effectN( 1 ).percent() );

    if ( timeofday == NIGHT_TIME )
       h *= 1.0 / ( 1.0 + racials.touch_of_elune -> effectN( 1 ).percent() );

  }

  return h;
}

// player_t::composite_spell_speed ==========================================

double player_t::composite_spell_speed() const
{
  double h = cache.spell_haste();

  return  h;
}

// player_t::composite_spell_power ==========================================

double player_t::composite_spell_power( school_e /* school */ ) const
{
  double sp = current.stats.spell_power;

  sp += current.spell_power_per_intellect * cache.intellect();

  return sp;
}

// player_t::composite_spell_power_multiplier ===============================

double player_t::composite_spell_power_multiplier() const
{
  return current.spell_power_multiplier;
}

// player_t::composite_spell_crit_chance ===========================================

double player_t::composite_spell_crit_chance() const
{
  double sc = current.spell_crit_chance + composite_spell_crit_rating() / current.rating.spell_crit;

  if ( current.spell_crit_per_intellect > 0 )
  {
    sc += ( cache.intellect() / current.spell_crit_per_intellect / 100.0 );
  }

  sc += racials.viciousness -> effectN( 1 ).percent();
  sc += racials.arcane_acuity -> effectN( 1 ).percent();

  if ( timeofday == DAY_TIME )
    sc += racials.touch_of_elune -> effectN( 1 ).percent();

  return sc;
}

// player_t::composite_spell_hit ============================================

double player_t::composite_spell_hit() const
{
  double sh = current.hit;

  //sh += composite_spell_hit_rating() / current.rating.spell_hit;

  sh += composite_melee_expertise();

  return sh;
}

// player_t::composite_mastery ==============================================

double player_t::composite_mastery() const
{
  return util::round( current.mastery + composite_mastery_rating() / current.rating.mastery, 2 );
}

// player_t::composite_bonus_armor =========================================

double player_t::composite_bonus_armor() const
{
  return current.stats.bonus_armor;
}

// player_t::composite_damage_versatility ===================================

double player_t::composite_damage_versatility() const
{
  double cdv = composite_damage_versatility_rating() / current.rating.damage_versatility;

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.legendary_tank_buff )
      cdv += buffs.legendary_tank_buff -> value();
  }

  return cdv;
}

// player_t::composite_heal_versatility ====================================

double player_t::composite_heal_versatility() const
{
  double chv = composite_heal_versatility_rating() / current.rating.heal_versatility;

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.legendary_tank_buff )
      chv += buffs.legendary_tank_buff -> value();
  }

  return chv;
}

// player_t::composite_mitigation_versatility ===============================

double player_t::composite_mitigation_versatility() const
{
  double cmv = composite_mitigation_versatility_rating() / current.rating.mitigation_versatility;

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.legendary_tank_buff )
      cmv += buffs.legendary_tank_buff -> value() / 2;
  }

  return cmv;
}

// player_t::composite_leech ================================================

double player_t::composite_leech() const
{
  return composite_leech_rating() / current.rating.leech;
}

// player_t::composite_run_speed ================================================

double player_t::composite_run_speed() const
{
  // speed DRs using the following formula:
  double pct = composite_speed_rating() / current.rating.speed;

  double coefficient = std::exp( -.0003 * composite_speed_rating() );

  return pct * coefficient * .1;
}

// player_t::composite_avoidance ================================================

double player_t::composite_avoidance() const
{
  return composite_avoidance_rating() / current.rating.avoidance;
}

// player_t::composite_player_pet_damage_multiplier =========================

double player_t::composite_player_pet_damage_multiplier( const action_state_t* ) const
{
  double m = 1.0;

  m *= 1.0 + racials.command -> effectN( 1 ).percent();

  return m;
}

// player_t::composite_player_multiplier ====================================

double player_t::composite_player_multiplier( school_e  school  ) const
{
  double m = 1.0;

  if ( buffs.brute_strength && buffs.brute_strength -> up() )
  {
    m *= 1.0 + buffs.brute_strength -> data().effectN( 1 ).percent();
  }

  if ( buffs.legendary_aoe_ring && buffs.legendary_aoe_ring -> up() )
    m *= 1.0 + buffs.legendary_aoe_ring -> default_value;
                                                                          // Artifacts get a free +6 purchased
  m *= 1.0 + artifact.artificial_damage -> effectN( 2 ).percent() * .01 * ( artifact.n_purchased_points + 6 );

  if ( buffs.taste_of_mana && buffs.taste_of_mana -> up() && school != SCHOOL_PHYSICAL )
  {
    m *= 1.0 + buffs.taste_of_mana -> default_value;
  }
  return m;
}

// player_t::composite_player_td_multiplier =================================

double player_t::composite_player_td_multiplier( school_e /* school */,  const action_t* /* a */ ) const
{
  return 1.0;
}

// player_t::composite_player_heal_multiplier ===============================

double player_t::composite_player_heal_multiplier( const action_state_t* ) const
{
  return 1.0;
}

// player_t::composite_player_th_multiplier =================================

double player_t::composite_player_th_multiplier( school_e /* school */ ) const
{
  return 1.0;
}

// player_t::composite_player_absorb_multiplier =============================

double player_t::composite_player_absorb_multiplier( const action_state_t* ) const
{
  return 1.0;
}

double player_t::composite_player_critical_damage_multiplier( const action_state_t* /* s */ ) const
{
  double m = 1.0;

  m += racials.brawn -> effectN( 1 ).percent();
  m += racials.might_of_the_mountain -> effectN( 1 ).percent();
  if ( buffs.incensed )
  {
    m += buffs.incensed -> check_value();
  }

  return m;
}

double player_t::composite_player_critical_healing_multiplier() const
{
  double m = 1.0;

  m += racials.brawn -> effectN( 1 ).percent();
  m += racials.might_of_the_mountain -> effectN( 1 ).percent();

  return m;
}

// player_t::composite_movement_speed =======================================
// There are 2 categories of movement speed buffs in WoD
// Passive and Temporary, both which stack additively. Passive buffs include movement speed enchant, unholy presence, cat form
// and generally anything that has the ability to be kept active all fight. These permanent buffs do stack with each other.
// Temporary includes all other speed bonuses, however, only the highest temporary bonus will be added on top.

double player_t::temporary_movement_modifier() const
{
  double temporary = 0;

  if ( ! is_enemy() )
  {
    if ( buffs.windwalking_movement_aura -> up() )
      temporary = std::max( buffs.windwalking_movement_aura -> data().effectN( 1 ).percent(), temporary );

    if ( buffs.darkflight -> up() )
      temporary = std::max( buffs.darkflight -> data().effectN( 1 ).percent(), temporary );

    if ( buffs.nitro_boosts -> up() )
      temporary = std::max( buffs.nitro_boosts -> data().effectN( 1 ).percent(), temporary );

    if ( buffs.stampeding_roar -> up() )
      temporary = std::max( buffs.stampeding_roar -> data().effectN( 1 ).percent(), temporary );

    if ( buffs.body_and_soul -> up() )
      temporary = std::max( buffs.body_and_soul -> data().effectN( 1 ).percent(), temporary );

    if ( buffs.angelic_feather -> up() )
      temporary = std::max( buffs.angelic_feather -> data().effectN( 1 ).percent(), temporary );
  }

  return temporary;
}

double player_t::passive_movement_modifier() const
{
  double passive = passive_modifier;

  passive += racials.quickness -> effectN( 2 ).percent();
  if ( buffs.aggramars_stride )
  {
    passive += ( buffs.aggramars_stride -> check_value() * composite_melee_haste() );
  }
  passive += composite_run_speed();

  return passive;
}

double player_t::composite_movement_speed() const
{
  double speed = base_movement_speed;

  double passive = passive_movement_modifier();

  double temporary = temporary_movement_modifier();

  speed *= ( 1 + passive + temporary );

  // Movement speed snares are multiplicative, works similarly to temporary speed boosts in that only the highest value counts.
  if ( debuffs.dazed -> up() )
    speed *= debuffs.dazed -> data().effectN( 1 ).percent();

  return speed;
}

// player_t::composite_attribute ============================================

double player_t::composite_attribute( attribute_e attr ) const
{
  double a = current.stats.attribute[ attr ];
  double m = ( ( true_level >= 50 ) && matching_gear ) ? ( 1.0 + matching_gear_multiplier( attr ) ) : 1.0;

  a = util::floor( ( a - base.stats.attribute[ attr ] ) * m ) + base.stats.attribute[ attr ];

  return a;
}

// player_t::composite_attribute_multiplier =================================

double player_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = current.attribute_multiplier[ attr ];

  if ( is_pet() || is_enemy() ) return m;

  switch ( attr )
  {
    case ATTR_STRENGTH:
      if ( buffs.archmages_greater_incandescence_str -> check() )
        m *= 1.0 + buffs.archmages_greater_incandescence_str -> data().effectN( 1 ).percent();
      if ( buffs.archmages_incandescence_str -> check() )
        m *= 1.0 + buffs.archmages_incandescence_str -> data().effectN( 1 ).percent();
      break;
    case ATTR_AGILITY:
      if ( buffs.archmages_greater_incandescence_agi -> check() )
        m *= 1.0 + buffs.archmages_greater_incandescence_agi -> data().effectN( 1 ).percent();
      if ( buffs.archmages_incandescence_agi -> check() )
        m *= 1.0 + buffs.archmages_incandescence_agi -> data().effectN( 1 ).percent();
      break;
    case ATTR_INTELLECT:
      if ( buffs.archmages_greater_incandescence_int -> check() )
        m *= 1.0 + buffs.archmages_greater_incandescence_int -> data().effectN( 1 ).percent();
      if ( buffs.archmages_incandescence_int -> check() )
        m *= 1.0 + buffs.archmages_incandescence_int -> data().effectN( 1 ).percent();
      break;
    case ATTR_SPIRIT:
      if ( buffs.amplification )
        m *= 1.0 + passive_values.amplification_1;
      if ( buffs.amplification_2 )
        m *= 1.0 + passive_values.amplification_2;
      break;
    case ATTR_STAMINA:                                                         // Artifacts get a free +6 purchased
      m *= 1.0 + artifact.artificial_stamina -> effectN( 2 ).percent() * .01 * ( artifact.n_purchased_points + 6 );
      break;
    default:
      break;
  }

  return m;
}

// player_t::composite_rating_multiplier ====================================

double player_t::composite_rating_multiplier( rating_e rating ) const
{
  double v = 1.0;

  switch( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      if ( buffs.amplification )
        v *= 1.0 + passive_values.amplification_1;
      if ( buffs.amplification_2 )
        v *= 1.0 + passive_values.amplification_2;
      v *= 1.0 + racials.the_human_spirit -> effectN( 1 ).percent();
      break;
    case RATING_MASTERY:
      if ( buffs.amplification )
        v *= 1.0 + passive_values.amplification_1;
      if ( buffs.amplification_2 )
        v *= 1.0 + passive_values.amplification_2;
      v *= 1.0 + racials.the_human_spirit -> effectN( 1 ).percent();
      break;
    case RATING_SPELL_CRIT:
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
      v *= 1.0 + racials.the_human_spirit -> effectN( 1 ).percent();
      break;
    case RATING_DAMAGE_VERSATILITY:
    case RATING_HEAL_VERSATILITY:
    case RATING_MITIGATION_VERSATILITY:
      v *= 1.0 + racials.the_human_spirit -> effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return v;
}

// player_t::composite_rating ===============================================

double player_t::composite_rating( rating_e rating ) const
{
  double v = 0;

  // Internally, we treat all the primary rating types as a single entity;
  // in game, they are actually split into spell/ranged/melee
  switch ( rating )
  {
    case RATING_SPELL_CRIT:
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
      v = current.stats.crit_rating; break;
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      v = current.stats.haste_rating; break;
    case RATING_SPELL_HIT:
    case RATING_MELEE_HIT:
    case RATING_RANGED_HIT:
      v = current.stats.hit_rating; break;
    case RATING_MASTERY:
      v = current.stats.mastery_rating;
      break;
    case RATING_DAMAGE_VERSATILITY:
    case RATING_HEAL_VERSATILITY:
    case RATING_MITIGATION_VERSATILITY:
      v = current.stats.versatility_rating; break;
    case RATING_EXPERTISE:
      v = current.stats.expertise_rating; break;
    case RATING_DODGE:
      v = current.stats.dodge_rating; break;
    case RATING_PARRY:
      v = current.stats.parry_rating; break;
    case RATING_BLOCK:
      v = current.stats.block_rating; break;
    case RATING_LEECH:
      v = current.stats.leech_rating; break;
    case RATING_SPEED:
      v = current.stats.speed_rating; break;
    case RATING_AVOIDANCE:
      v = current.stats.avoidance_rating; break;
    default: break;
  }

  return util::round( v * composite_rating_multiplier( rating ), 0 );
}

// player_t::composite_player_vulnerability =================================

double player_t::composite_player_vulnerability( school_e /* school */ ) const
{
  double m = 1.0;

  if ( debuffs.vulnerable -> check() )
    m *= 1.0 + debuffs.vulnerable -> value();

  // 1% damage taken per stack, arbitrary because this buff is completely fabricated!
  if ( debuffs.damage_taken -> check() )
    m *= 1.0 + debuffs.damage_taken -> current_stack * 0.01;

  return m;
}

double player_t::composite_mitigation_multiplier( school_e /* school */ ) const
{
  return 1.0;
}

double player_t::composite_mastery_value() const
{
  return composite_mastery() * mastery_coefficient();
}

#if defined(SC_USE_STAT_CACHE)

// player_t::invalidate_cache ===============================================

void player_t::invalidate_cache( cache_e c )
{
  if ( ! cache.active ) return;

  if ( sim -> debug ) sim -> out_debug.printf( "%s invalidates %s", name(), util::cache_type_string( c ) );

  // Special linked invalidations
  switch ( c )
  {
    case CACHE_STRENGTH:
      if ( current.attack_power_per_strength > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      if ( current.parry_per_strength > 0 )
        invalidate_cache( CACHE_PARRY );
      break;
    case CACHE_AGILITY:
      if ( current.attack_power_per_agility > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      if ( current.dodge_per_agility > 0 )
        invalidate_cache( CACHE_DODGE );
      break;
    case CACHE_INTELLECT:
      if ( current.spell_power_per_intellect > 0 )
        invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_ATTACK_HASTE:
      invalidate_cache( CACHE_ATTACK_SPEED );
      break;
    case CACHE_SPELL_HASTE:
      invalidate_cache( CACHE_SPELL_SPEED );
      break;
    case CACHE_BONUS_ARMOR:
      invalidate_cache( CACHE_ARMOR );
    default: break;
  }

  // Normal invalidation of the corresponding Cache
  switch ( c )
  {
    case CACHE_EXP:
      invalidate_cache( CACHE_ATTACK_EXP );
      invalidate_cache( CACHE_SPELL_HIT  );
      break;
    case CACHE_HIT:
      invalidate_cache( CACHE_ATTACK_HIT );
      invalidate_cache( CACHE_SPELL_HIT  );
      break;
    case CACHE_CRIT_CHANCE:
      invalidate_cache( CACHE_ATTACK_CRIT_CHANCE );
      invalidate_cache( CACHE_SPELL_CRIT_CHANCE  );
      break;
    case CACHE_HASTE:
      invalidate_cache( CACHE_ATTACK_HASTE );
      invalidate_cache( CACHE_SPELL_HASTE  );
      break;
    case CACHE_SPEED:
      invalidate_cache( CACHE_ATTACK_SPEED );
      invalidate_cache( CACHE_SPELL_SPEED  );
      break;
    case CACHE_VERSATILITY:
      invalidate_cache( CACHE_DAMAGE_VERSATILITY );
      invalidate_cache( CACHE_HEAL_VERSATILITY );
      invalidate_cache( CACHE_MITIGATION_VERSATILITY );
      break;
    default:
      cache.invalidate( c );
      break;
  }
}

#endif

void player_t::sequence_add_wait( const timespan_t& amount, const timespan_t& ts )
{
  // Collect iteration#1 data, for log/debug/iterations==1 simulation iteration#0 data
  if ( ( sim -> iterations <= 1 && sim -> current_iteration == 0 ) ||
       ( sim -> iterations > 1 && sim -> current_iteration == 1 ) )
  {
    if ( collected_data.action_sequence.size() <= sim -> expected_max_time() * 2.0 + 3.0 )
    {
      if ( in_combat )
      {
        if ( collected_data.action_sequence.size() && collected_data.action_sequence.back() -> wait_time > timespan_t::zero() )
          collected_data.action_sequence.back() -> wait_time += amount;
        else
          collected_data.action_sequence.push_back( new player_collected_data_t::action_sequence_data_t( ts, amount, this ) );
      }
    }
    else
    {
      assert( false && "Collected too much action sequence data."
      "This means there is a serious overflow of executed actions in the first iteration, which should be fixed." );
    }
  }
}

void player_t::sequence_add( const action_t* a, const player_t* target, const timespan_t& ts )
{
  // Collect iteration#1 data, for log/debug/iterations==1 simulation iteration#0 data
  if ( ( a -> sim -> iterations <= 1 && a -> sim -> current_iteration == 0 ) ||
       ( a -> sim -> iterations > 1 && a -> sim -> current_iteration == 1 ) )
  {
    if ( collected_data.action_sequence.size() <= sim -> expected_max_time() * 2.0 + 3.0 )
    {
      if ( in_combat )
        collected_data.action_sequence.push_back( new player_collected_data_t::action_sequence_data_t( a, target, ts, this ) );
      else
        collected_data.action_sequence_precombat.push_back( new player_collected_data_t::action_sequence_data_t( a, target, ts, this ) );
    }
    else
    {
      assert( false && "Collected too much action sequence data."
      "This means there is a serious overflow of executed actions in the first iteration, which should be fixed." );
    }
  }
}

// player_t::combat_begin ===================================================

void player_t::combat_begin()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Combat begins for player %s", name() );

  if ( ! is_pet() && ! is_add() )
  {
    arise();
  }

  init_resources( true );

  if ( ! is_pet() && ! is_add() )
  {
    for ( size_t i = 0; i < precombat_action_list.size(); i++ )
    {
      if ( precombat_action_list[ i ] -> ready() )
      {
        action_t* action = precombat_action_list[ i ];
        if ( action -> harmful )
        {
          if ( first_cast )
          {
            action -> execute();
            sequence_add( action, action -> target, sim -> current_time() );
            first_cast = false;
          }
          else if ( sim -> debug )
          {
            sim -> out_debug.printf( "Player %s attempting to cast multiple harmful spells precombat.", name() );
          }
        }
        else
        {
          action -> execute();
          sequence_add( action, action -> target, sim -> current_time() );
        }
      }
    }
  }
  first_cast = false;

  if ( ! precombat_action_list.empty() )
    in_combat = true;

  // re-initialize collected_data.health_changes.previous_*_level
  // necessary because food/flask are counted as resource gains, and thus provide phantom
  // gains on the timeline if not corrected
  collected_data.health_changes.previous_gain_level = 0.0;
  collected_data.health_changes_tmi.previous_gain_level = 0.0;
  // forcing a resource timeline data collection in combat_end() seems to have rendered this next line unnecessary
  collected_data.health_changes.previous_loss_level = 0.0;
  collected_data.health_changes_tmi.previous_gain_level = 0.0;

  if ( buffs.cooldown_reduction )
    buffs.cooldown_reduction -> trigger();

  if ( buffs.amplification )
    buffs.amplification -> trigger();

  if ( buffs.amplification_2 )
    buffs.amplification_2 -> trigger();

  if ( buffs.aggramars_stride )
    buffs.aggramars_stride -> trigger();

  if ( buffs.tyrants_decree_driver )
  { // Assume actor has stacked the buff to max stack precombat.
    buffs.tyrants_decree_driver -> trigger();
    buffs.tyrants_immortality   -> trigger( buffs.tyrants_immortality -> max_stack() );
  }
}

// player_t::combat_end =====================================================

void player_t::combat_end()
{
  for ( size_t i = 0; i < pet_list.size(); ++i )
    pet_list[ i ] -> combat_end();

  if ( ! is_pet() )
  {
    demise();
  }
  else
    cast_pet() -> dismiss();

  double f_length = iteration_fight_length.total_seconds();
  double w_time = iteration_waiting_time.total_seconds();

  if ( ready_type == READY_POLL && sim -> auto_ready_trigger )
    if ( ! is_pet() && ! is_enemy() )
      if ( f_length > 0 && ( w_time / f_length ) > 0.25 )
      {
   // ready_type = READY_TRIGGER;
      }

  if ( sim -> debug )
    sim -> out_debug.printf( "Combat ends for player %s at time %.4f fight_length=%.4f", name(), sim -> current_time().total_seconds(), iteration_fight_length.total_seconds() );
}

// startpoint for statistical data collection

void player_t::datacollection_begin()
{
  // Check whether the actor was arisen at least once during the _previous_ iteration
  // Note that this check is dependant on sim_t::combat_begin() having
  // sim_t::datacollection_begin() call before the player_t::combat_begin() calls.
  if ( ! active_during_iteration )
    return;

  if ( sim -> debug )
    sim -> out_debug.printf( "Data collection begins for player %s", name() );

  iteration_fight_length = timespan_t::zero();
  iteration_waiting_time = timespan_t::zero();
  iteration_pooling_time = timespan_t::zero();
  iteration_executed_foreground_actions = 0;
  iteration_dmg = 0;
  priority_iteration_dmg = 0;
  iteration_heal = 0;
  iteration_absorb = 0.0;
  iteration_absorb_taken = 0.0;
  iteration_dmg_taken = 0;
  iteration_heal_taken = 0;
  active_during_iteration = false;

  range::fill( iteration_resource_lost, 0 );
  range::fill( iteration_resource_gained, 0 );

  if ( collected_data.health_changes.collect )
  {
    collected_data.health_changes.timeline.clear(); // Drop Data
    collected_data.health_changes.timeline_normalized.clear();
  }

  if ( collected_data.health_changes_tmi.collect )
  {
    collected_data.health_changes_tmi.timeline.clear(); // Drop Data
    collected_data.health_changes_tmi.timeline_normalized.clear();
  }

  range::for_each( buff_list, std::mem_fn(&buff_t::datacollection_begin ) );
  range::for_each( stats_list, std::mem_fn(&stats_t::datacollection_begin ) );
  range::for_each( uptime_list, std::mem_fn(&uptime_t::datacollection_begin ) );
  range::for_each( benefit_list, std::mem_fn(&benefit_t::datacollection_begin ) );
  range::for_each( proc_list, std::mem_fn(&proc_t::datacollection_begin ) );
  range::for_each( pet_list, std::mem_fn(&pet_t::datacollection_begin ) );
  range::for_each( sample_data_list, std::mem_fn(&luxurious_sample_data_t::datacollection_begin ) );
}

// endpoint for statistical data collection

void player_t::datacollection_end()
{
  // This checks if the actor was arisen at least once during this iteration.
  if ( ! requires_data_collection() )
    return;

  if ( sim -> debug )
    sim -> out_debug.printf( "Data collection ends for player %s at time %.4f fight_length=%.4f", name(), sim -> current_time().total_seconds(), iteration_fight_length.total_seconds() );

  for ( size_t i = 0; i < pet_list.size(); ++i )
    pet_list[ i ] -> datacollection_end();

  if ( arise_time >= timespan_t::zero() )
  {
    // If we collect data while the player is still alive, capture active time up to now
    assert( sim -> current_time() >= arise_time );
    iteration_fight_length += sim -> current_time() - arise_time;
    arise_time = sim -> current_time();
  }

  for ( size_t i = 0; i < stats_list.size(); ++i )
    stats_list[ i ] -> datacollection_end();

  if ( ! is_enemy() && ! is_add() )
  {
    sim -> iteration_dmg += iteration_dmg;
    sim -> priority_iteration_dmg += priority_iteration_dmg;
    sim -> iteration_heal += iteration_heal;
    sim -> iteration_absorb += iteration_absorb;
  }

  // make sure TMI-relevant timeline lengths all match for tanks
  if ( ! is_enemy() && ! is_pet() && primary_role() == ROLE_TANK )
  {
    collected_data.timeline_healing_taken.add( sim -> current_time(), 0.0 );
    collected_data.timeline_dmg_taken.add( sim -> current_time(), 0.0 );
    collected_data.health_changes.timeline.add( sim -> current_time(), 0.0 );
    collected_data.health_changes.timeline_normalized.add( sim -> current_time(), 0.0 );
    collected_data.health_changes_tmi.timeline.add( sim -> current_time(), 0.0 );
    collected_data.health_changes_tmi.timeline_normalized.add( sim -> current_time(), 0.0 );
  }
  collected_data.collect_data( *this );


  range::for_each( buff_list, std::mem_fn(&buff_t::datacollection_end ) );

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> datacollection_end( iteration_fight_length );

  range::for_each( benefit_list, std::mem_fn(&benefit_t::datacollection_end ) );
  range::for_each( proc_list, std::mem_fn(&proc_t::datacollection_end ) );
  range::for_each( sample_data_list, std::mem_fn(&luxurious_sample_data_t::datacollection_end ) );
}

// player_t::merge ==========================================================

namespace { namespace buff_merge {

// a < b iff ( a.name < b.name || ( a.name == b.name && a.source < b.source ) )
bool compare( const buff_t* a, const buff_t* b )
{
  assert( a );
  assert( b );

  if ( a -> name_str < b -> name_str )
    return true;
  if ( b -> name_str < a -> name_str )
    return false;

  // NULL and player are identically considered "bottom" for source comparison
  bool a_is_bottom = ( ! a -> source || a -> source == a -> player );
  bool b_is_bottom = ( ! b -> source || b -> source == b -> player );

  if ( a_is_bottom )
    return ! b_is_bottom;

  if ( b_is_bottom )
    return false;

  // If neither source is bottom, order by source index
  return a -> source -> index < b -> source -> index;
}

#ifndef NDEBUG
const char* source_name( const buff_t& b )
{ return b.source ? b.source -> name() : "(none)"; }
#endif

// Sort buff_list and check for uniqueness
void prepare( player_t& p )
{
  range::sort( p.buff_list, compare );
  // For all i, p.buff_list[ i ] <= p.buff_list[ i + 1 ]

#ifndef NDEBUG
  if ( p.buff_list.empty() ) return;

  for ( size_t i = 0, last = p.buff_list.size() - 1; i < last; ++i )
  {
    // We know [ i ] <= [ i + 1 ] due to sorting; if also
    // ! ( [ i ] < [ i + 1 ] ), then [ i ] == [ i + 1 ].
    if ( ! compare( p.buff_list[ i ], p.buff_list[ i + 1 ] ) )
    {
      p.sim -> errorf( "Player %s has duplicate buffs named '%s' with source '%s' - the end is near.",
                       p.name(), p.buff_list[ i ] -> name(), source_name( *p.buff_list[ i ] ) );
    }
  }
#endif
}

void report_unmatched( const buff_t& b )
{
#ifndef NDEBUG
  /* Don't complain about targetdata buffs, since it is perfectly viable that the buff
   * is not created in another thread, because of our on-demand targetdata creation
   */
  if ( ! b.source || b.source == b.player )
  {
    b.sim -> errorf( "Player '%s' can't merge buff %s with source '%s'",
                     b.player -> name(), b.name(), source_name( b ) );
  }
#else
  // "Use" the parameters to silence compiler warnings.
  ( void ) b;
#endif
}

void check_tail( player_t& p, size_t first )
{
#ifndef NDEBUG
  for ( size_t last = p.buff_list.size() ; first < last; ++first )
    report_unmatched( *p.buff_list[ first ] );
#else
  // "Use" the parameters to silence compiler warnings.
  ( void )p;
  ( void )first;
#endif
}

// Merge stats of matching buffs from right into left.
void merge( player_t& left, player_t& right )
{
  prepare( left );
  prepare( right );

  // Both buff_lists are sorted, join them mergesort-style.
  size_t i = 0, j = 0;
  while ( i < left.buff_list.size() && j < right.buff_list.size() )
  {
    if ( compare( left.buff_list[ i ], right.buff_list[ j ] ) )
    {
      // [ i ] < [ j ]
      report_unmatched( *left.buff_list[ i ] );
      ++i;
    }
    else if ( compare( right.buff_list[ j ], left.buff_list[ i ] ) )
    {
      // [ i ] > [ j ]
      report_unmatched( *right.buff_list[ j ] );
      ++j;
    }
    else
    {
      // [ i ] == [ j ]
      left.buff_list[ i ] -> merge( *right.buff_list[ j ] );
      ++i, ++j;
    }
  }

  check_tail( left, i );
  check_tail( right, j );
}

} } // namespace {anonymous}::buff_merge

void player_t::merge( player_t& other )
{
  collected_data.merge( other.collected_data );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    iteration_resource_lost  [ i ] += other.iteration_resource_lost  [ i ];
    iteration_resource_gained[ i ] += other.iteration_resource_gained[ i ];
  }

  buff_merge::merge( *this, other );

  // Procs
  for ( size_t i = 0; i < proc_list.size(); ++i )
  {
    proc_t& proc = *proc_list[ i ];
    if ( proc_t* other_proc = other.find_proc( proc.name_str ) )
      proc.merge( *other_proc );
    else
    {
#ifndef NDEBUG
      sim -> errorf( "%s player_t::merge can't merge proc %s", name(), proc.name() );
#endif
    }
  }

  // Gains
  for ( size_t i = 0; i < gain_list.size(); ++i )
  {
    gain_t& gain = *gain_list[ i ];
    if ( gain_t* other_gain = other.find_gain( gain.name_str ) )
      gain.merge( *other_gain );
    else
    {
#ifndef NDEBUG
      sim -> errorf( "%s player_t::merge can't merge gain %s", name(), gain.name() );
#endif
    }
  }

  // Stats
  for ( size_t i = 0; i < stats_list.size(); ++i )
  {
    stats_t& stats = *stats_list[ i ];
    if ( stats_t* other_stats = other.find_stats( stats.name_str ) )
      stats.merge( *other_stats );
    else
    {
#ifndef NDEBUG
      sim -> errorf( "%s player_t::merge can't merge stats %s", name(), stats.name() );
#endif
    }
  }

  // Uptimes
  for ( size_t i = 0; i < uptime_list.size(); ++i )
  {
    uptime_t& uptime = *uptime_list[ i ];
    if ( uptime_t* other_uptime = other.find_uptime( uptime.name_str ) )
      uptime.merge( *other_uptime );
    else
    {
#ifndef NDEBUG
      sim -> errorf( "%s player_t::merge can't merge uptime %s", name(), uptime.name() );
#endif
    }
  }

  // Benefits
  for ( size_t i = 0; i < benefit_list.size(); ++i )
  {
    benefit_t& benefit = *benefit_list[ i ];
    if ( benefit_t* other_benefit = other.find_benefit( benefit.name_str ) )
      benefit.merge( *other_benefit );
    else
    {
#ifndef NDEBUG
      sim -> errorf( "%s player_t::merge can't merge benefit %s", name(), benefit.name() );
#endif
    }
  }

  // Sample Data
  for ( size_t i = 0; i < sample_data_list.size(); ++i )
  {
    luxurious_sample_data_t& sd = *sample_data_list[ i ];
    if ( luxurious_sample_data_t* other_sd = other.find_sample_data( sd.name_str ) )
      sd.merge( *other_sd );
    else
    {
#ifndef NDEBUG
      sim -> errorf( "%s player_t::merge can't merge proc %s", name(), sd.name_str.c_str() );
#endif
    }
  }

  // Action Map
  size_t n_entries = std::min( action_list.size(), other.action_list.size() );
  if ( action_list.size() != other.action_list.size() )
  {
    sim -> errorf( "%s player_t::merge action lists for actor differ (other=%s, size=%u, other.size=%u)!",
      name(), other.name(), action_list.size(), other.action_list.size() );
  }

  for ( size_t i = 0; i < n_entries; ++i )
  {
    if ( action_list[ i ] -> internal_id == other.action_list[ i ] -> internal_id )
    {
      action_list[ i ] -> total_executions += other.action_list[ i ] -> total_executions;
    }
    else
    {
      sim -> errorf( "%s player_t::merge can't merge action %s::%s with %s::%s",
          name(), action_list[ i ] -> action_list ? action_list[ i ] -> action_list -> name_str.c_str() : "(none)",
          action_list[ i ] -> signature_str.c_str(),
          other.action_list[ i ] -> action_list ? other.action_list[ i ] -> action_list -> name_str.c_str() : "(none)",
          other.action_list[ i ] -> signature_str.c_str() );
    }
  }
}

// player_t::reset ==========================================================

void player_t::reset()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Resetting player %s", name() );

  last_cast = timespan_t::zero();
  gcd_ready = timespan_t::zero();

  cache.invalidate_all();

  // Reset current stats to initial stats
  current = initial;

  current.sleeping = true;

  change_position( initial.position );

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s current stats ( reset to initial ): %s", name(), current.to_string().c_str() );
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> reset();

  last_foreground_action = 0;
  last_gcd_action = 0;
  off_gcdactions.clear();

  first_cast = true;

  executing = nullptr;
  queueing = nullptr;
  channeling = nullptr;
  readying = nullptr;
  strict_sequence = 0;
  off_gcd = 0;
  in_combat = false;

  current_attack_speed = 0;
  gcd_current_haste_value = 1.0;
  gcd_haste_type = HASTE_NONE;

  cast_delay_reaction = timespan_t::zero();
  cast_delay_occurred = timespan_t::zero();

  main_hand_weapon.buff_type  = 0;
  main_hand_weapon.buff_value = 0;
  main_hand_weapon.bonus_dmg  = 0;

  off_hand_weapon.buff_type  = 0;
  off_hand_weapon.buff_value = 0;
  off_hand_weapon.bonus_dmg  = 0;

  x_position = default_x_position;
  y_position = default_y_position;

  callbacks.reset();

  init_resources( true );

  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ] -> reset();

  for ( size_t i = 0; i < cooldown_list.size(); ++i )
    cooldown_list[ i ] -> reset_init();

  for ( size_t i = 0; i < dot_list.size(); ++i )
    dot_list[ i ] -> reset();

  for ( size_t i = 0; i < stats_list.size(); ++i )
    stats_list[ i ] -> reset();

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> reset();

  for ( size_t i = 0; i < proc_list.size(); ++i )
    proc_list[ i ] -> reset();

  range::for_each( rppm_list, []( real_ppm_t* rppm ) { rppm -> reset(); } );

  potion_used = 0;

  item_cooldown.reset( false );

  incoming_damage.clear();

  resource_threshold_trigger = 0;

  for ( auto& elem : variables )
    elem -> reset();

#ifndef NDEBUG
  for (auto & elem : active_dots)
  {
    assert( elem == 0 );
  }
#endif
}

// player_t::trigger_ready ==================================================

void player_t::trigger_ready()
{
  if ( ready_type == READY_POLL ) return;

  if ( readying ) return;
  if ( executing ) return;
  if ( queueing ) return;
  if ( channeling ) return;

  if ( current.sleeping ) return;

  if ( buffs.stunned -> check() ) return;

  if ( sim -> debug ) sim -> out_debug.printf( "%s is triggering ready, interval=%f", name(), ( sim -> current_time() - started_waiting ).total_seconds() );

  assert( started_waiting >= timespan_t::zero() );
  iteration_waiting_time += sim -> current_time() - started_waiting;
  started_waiting = timespan_t::min();

  schedule_ready( available() );
}

// player_t::schedule_ready =================================================

void player_t::schedule_ready( timespan_t delta_time,
                               bool       waiting )
{
#ifndef NDEBUG
  if ( readying )
  {
    sim -> errorf( "\nplayer_t::schedule_ready assertion error: readying == true ( player %s )\n", name() );
    assert( 0 );
  }
#endif
  action_t* was_executing = ( channeling ? channeling : executing );

  if ( queueing )
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s canceling queued action %s at %.3f", name(), queueing -> name(),
          queueing -> queue_event -> occurs().total_seconds() );
    }
    event_t::cancel( queueing -> queue_event );
    queueing = nullptr;
  }

  if ( current.sleeping )
    return;

  executing = nullptr;
  queueing = nullptr;
  channeling = nullptr;
  action_queued = false;

  started_waiting = timespan_t::min();

  timespan_t gcd_adjust = gcd_ready - ( sim -> current_time() + delta_time );
  if ( gcd_adjust > timespan_t::zero() ) delta_time += gcd_adjust;

  if ( waiting )
  {
    sequence_add_wait( delta_time, sim -> current_time() );
    iteration_waiting_time += delta_time;
  }
  else
  {
    timespan_t lag = timespan_t::zero();

    if ( last_foreground_action )
    {
      if ( last_foreground_action -> ability_lag > timespan_t::zero() )
      {
        timespan_t ability_lag = rng().gauss( last_foreground_action -> ability_lag, last_foreground_action -> ability_lag_stddev );
        timespan_t gcd_lag     = rng().gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        timespan_t diff        = ( gcd_ready + gcd_lag ) - ( sim -> current_time() + ability_lag );
        if ( diff > timespan_t::zero() && sim -> strict_gcd_queue )
        {
          lag = gcd_lag;
        }
        else
        {
          lag = ability_lag;
          action_queued = true;
        }
      }
      else if ( last_foreground_action -> gcd() == timespan_t::zero() )
      {
        lag = timespan_t::zero();
      }
      else if ( last_foreground_action -> channeled && !last_foreground_action->interrupt_immediate_occurred)
      {
        lag = rng().gauss( sim -> channel_lag, sim -> channel_lag_stddev );
      }
      else
      {
        timespan_t   gcd_lag = rng().gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        timespan_t queue_lag = rng().gauss( sim -> queue_lag, sim -> queue_lag_stddev );

        timespan_t diff = ( gcd_ready + gcd_lag ) - ( sim -> current_time() + queue_lag );

        if ( diff > timespan_t::zero() && sim -> strict_gcd_queue )
        {
          lag = gcd_lag;
        }
        else
        {
          lag = queue_lag;
          action_queued = true;
        }
      }
    }

    if ( lag < timespan_t::zero() ) lag = timespan_t::zero();

    if ( type == PLAYER_GUARDIAN )
      lag = timespan_t::zero(); // Guardians do not seem to feel the effects of queue/gcd lag in WoD.

    delta_time += lag;
  }

  if ( last_foreground_action )
  {
    // This is why "total_execute_time" is not tracked per-target!
    last_foreground_action -> stats -> iteration_total_execute_time += delta_time;
  }

  readying = make_event<player_ready_event_t>( *sim, *this, delta_time );

  if ( was_executing && was_executing -> gcd() > timespan_t::zero() && ! was_executing -> background && ! was_executing -> proc && ! was_executing -> repeating )
  {
    // Record the last ability use time for cast_react
    cast_delay_occurred = readying -> occurs();
    cast_delay_reaction = rng().gauss( brain_lag, brain_lag_stddev );
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s %s schedule_ready(): cast_finishes=%f cast_delay=%f",
                     name_str.c_str(),
                     was_executing -> name_str.c_str(),
                     readying -> occurs().total_seconds(),
                     cast_delay_reaction.total_seconds() );
    }
  }
}

// player_t::arise ==========================================================

void player_t::arise()
{
  if ( sim -> log )
    sim -> out_log.printf( "%s tries to arise.", name() );

  if ( ! initial.sleeping )
    current.sleeping = false;

  if ( current.sleeping )
    return;

  actor_spawn_index = sim -> global_spawn_index++;

  if ( sim -> log )
    sim -> out_log.printf( "%s arises. Spawn Index=%d", name(), actor_spawn_index );

  init_resources( true );

  cache.invalidate_all();

  readying = 0;
  off_gcd = 0;

  arise_time = sim -> current_time();
  last_regen = sim -> current_time();

  if ( is_enemy() )
  {
    sim -> active_enemies++;
    sim -> target_non_sleeping_list.push_back( this );
  }
  else
  {
    sim -> active_allies++;
    sim -> player_non_sleeping_list.push_back( this );
  }

  if ( has_foreground_actions( *this ) )
    schedule_ready();

  active_during_iteration = true;

  for ( auto callback: callbacks.all_callbacks ) {
    dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( callback );

    if ( cb -> cooldown && cb -> effect.item &&
         cb -> effect.item -> parsed.initial_cd > timespan_t::zero() )
    {
      timespan_t initial_cd = std::min( cb -> effect.cooldown(), cb -> effect.item -> parsed.initial_cd );
      cb -> cooldown -> start( initial_cd );

      if ( sim -> log )
      {
        sim -> out_log.printf( "%s sets initial cooldown for %s to %.2f seconds.",
          name(),
          cb -> effect.name().c_str(),
          initial_cd.total_seconds() );
      }
    }
  }
}

// player_t::demise =========================================================

void player_t::demise()
{
  // No point in demising anything if we're not even active
  if ( current.sleeping )
    return;

  if ( sim -> log )
    sim -> out_log.printf( "%s demises.. Spawn Index=%u", name(), actor_spawn_index );

  /* Do not reset spawn index, because the player can still have damaging events ( dots ) which
   * need to be associated with eg. resolve Diminishing Return list.
   */

  assert( arise_time >= timespan_t::zero() );
  iteration_fight_length += sim -> current_time() - arise_time;
  // Arise time has to be set to default value before actions are canceled.
  arise_time = timespan_t::min();
  current.distance_to_move = 0;

  if ( readying )
  {
    event_t::cancel( readying );
    readying = 0;
  }

  event_t::cancel( off_gcd );

  for ( size_t i = 0; i < callbacks_on_demise.size(); ++i )
  {
    callbacks_on_demise[i]( this );
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> expire();
    // Dead actors speak no lies .. or proc aura delayed buffs
    event_t::cancel( b -> delay );
    event_t::cancel( b -> expiration_delay );
  }
  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ] -> cancel();

  // sim -> cancel_events( this );

  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_list[i] -> demise();
  }

  for ( size_t i = 0; i < dot_list.size(); ++i )
    dot_list[ i ] -> cancel();

  if ( is_enemy() )
  {
    sim -> active_enemies--;
    sim -> target_non_sleeping_list.find_and_erase_unordered( this );
  }
  else
  {
    sim -> active_allies--;
    sim -> player_non_sleeping_list.find_and_erase_unordered( this );
  }

  current.sleeping = true;
}

// player_t::interrupt ======================================================

void player_t::interrupt()
{
  // FIXME! Players will need to override this to handle background repeating actions.

  if ( sim -> log ) sim -> out_log.printf( "%s is interrupted", name() );

  if ( executing  ) executing  -> interrupt_action();
  if ( queueing   ) queueing   -> interrupt_action();
  if ( channeling ) channeling -> interrupt_action();

  if ( strict_sequence )
  {
    strict_sequence -> cancel();
    strict_sequence = 0;
  }

  if ( buffs.stunned -> check() )
  {
    if ( readying ) event_t::cancel( readying );
    if ( off_gcd ) event_t::cancel( off_gcd );
  }
  else
  {
    if ( ! readying && ! current.sleeping ) schedule_ready();
  }
}

// player_t::halt ===========================================================

void player_t::halt()
{
  if ( sim -> log ) sim -> out_log.printf( "%s is halted", name() );

  interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
  if (  off_hand_attack )  off_hand_attack -> cancel();
}

// player_t::stun() =========================================================

void player_t::stun()
{
  halt();
}

// player_t::moving =========================================================

void player_t::moving()
{
  halt();
}

// player_t::clear_debuffs===================================================

void player_t::clear_debuffs()
{
  if ( sim -> log ) sim -> out_log.printf( "%s clears debuffs", name() );

  // Clear Dots
  for ( size_t i = 0; i < dot_list.size(); ++i )
  {
    dot_t* dot = dot_list[ i ];
    dot -> cancel();
  }

  // Clear all buffs of type debuff_t
  // We have to make sure that we label all
  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    debuff_t* debuff = dynamic_cast<debuff_t*>( buff_list[ i ] );
    if ( debuff )
      debuff -> expire();
  }
}

// player_t::execute_action =================================================

action_t* player_t::execute_action()
{
  readying = 0;
  off_gcd = 0;

  action_t* action = 0;

  if ( regen_type == REGEN_DYNAMIC )
    do_dynamic_regen();

  if ( ! strict_sequence )
  {
    visited_apls_ = 0; // Reset visited apl list
    action = select_action( *active_action_list );
  }
  // Committed to a strict sequence of actions, just perform them instead of a priority list
  else
    action = strict_sequence;

  last_foreground_action = action;

  if ( restore_action_list != 0 )
  {
    activate_action_list( restore_action_list );
    restore_action_list = 0;
  }

  if ( action )
  {
    action -> line_cooldown.start();
    action -> queue_execute( false );
    if ( ! action -> quiet )
    {
      iteration_executed_foreground_actions++;
      action -> total_executions++;
      if ( action -> trigger_gcd > timespan_t::zero() )
        last_gcd_action = action;
      else
        off_gcdactions.push_back( action );

      sequence_add( action, action -> target, sim -> current_time() );
    }
  }

  return action;
}

// player_t::regen ==========================================================

void player_t::regen( timespan_t periodicity )
{
  if ( regen_type == REGEN_DYNAMIC && sim -> debug )
    sim -> out_debug.printf( "%s dynamic regen, last=%.3f interval=%.3f",
        name(), last_regen.total_seconds(), periodicity.total_seconds() );

  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; r++ )
  {
    if ( resources.is_active( r ) )
    {
      double base;
      gain_t* gain;

      switch ( r )
      {
        case RESOURCE_ENERGY:
          base = energy_regen_per_second();
          gain = gains.energy_regen;
          break;

        case RESOURCE_FOCUS:
          base = focus_regen_per_second();
          gain = gains.focus_regen;
          break;

        case RESOURCE_MANA:
          base = mana_regen_per_second();
          gain = gains.mp5_regen;
          break;

        default:
          continue;
      }

      if ( gain && base )
        resource_gain( r, base * periodicity.total_seconds(), gain );
    }
  }
}

// player_t::collect_resource_timeline_information ==========================

void player_t::collect_resource_timeline_information()
{
  for (auto & elem : collected_data.resource_timelines)
  {
    elem.timeline.add( sim -> current_time(),
        resources.current[ elem.type ] );
  }

  for (auto & elem : collected_data.stat_timelines)
  {
    switch ( elem.type )
    {
      case STAT_STRENGTH:
        elem.timeline.add( sim -> current_time(), cache.strength() );
        break;
      case STAT_AGILITY:
        elem.timeline.add( sim -> current_time(), cache.agility() );
        break;
      case STAT_INTELLECT:
        elem.timeline.add( sim -> current_time(), cache.intellect() );
        break;
      case STAT_SPELL_POWER:
        elem.timeline.add( sim -> current_time(), cache.spell_power( SCHOOL_NONE ) );
        break;
      case STAT_ATTACK_POWER:
        elem.timeline.add( sim -> current_time(), cache.attack_power() );
        break;
      default:
        elem.timeline.add( sim -> current_time(), 0 );
        break;
    }
  }
}

// player_t::resource_loss ==================================================

double player_t::resource_loss( resource_e resource_type,
                                double    amount,
                                gain_t*   source,
                                action_t* )
{
  if ( amount == 0 )
    return 0.0;

  if ( current.sleeping )
    return 0.0;

  if ( resource_type == primary_resource() )
    uptimes.primary_resource_cap -> update( false, sim -> current_time() );

  double actual_amount;

  if ( ! resources.is_infinite( resource_type ) || is_enemy() )
  {
    actual_amount = std::min( amount, resources.current[ resource_type ] );
    resources.current[ resource_type ] -= actual_amount;
    iteration_resource_lost[ resource_type ] += actual_amount;
  }
  else
  {
    actual_amount = amount;
    resources.current[ resource_type ] -= actual_amount;
    iteration_resource_lost[ resource_type ] += actual_amount;
  }

  if ( source )
  {
    source -> add( resource_type, actual_amount * -1, ( amount - actual_amount ) * -1 );
  }

  if ( resource_type == RESOURCE_MANA )
  {
    last_cast = sim -> current_time();
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "Player %s loses %.2f (%.2f) %s. pct=%.2f%% (%.0f/%.0f)",
                   name(),
                   actual_amount,
                   amount,
                   util::resource_type_string( resource_type ),
                   resources.max[ resource_type ] ? resources.pct( resource_type ) * 100 : 0,
                   resources.current[ resource_type ],
                   resources.max[ resource_type ] );

  return actual_amount;
}

// player_t::resource_gain ==================================================

double player_t::resource_gain( resource_e resource_type,
                                double    amount,
                                gain_t*   source,
                                action_t* action )
{
  if ( current.sleeping || amount == 0.0 )
    return 0.0;

  double actual_amount = std::min( amount, resources.max[ resource_type ] - resources.current[ resource_type ] );

  if ( actual_amount > 0.0 )
  {
    resources.current[ resource_type ] += actual_amount;
    iteration_resource_gained [ resource_type ] += actual_amount;
  }

  if ( resource_type == primary_resource() && resources.max[ resource_type ] <= resources.current[ resource_type ] )
    uptimes.primary_resource_cap -> update( true, sim -> current_time() );

  if ( source )
  {
    source -> add( resource_type, actual_amount, amount - actual_amount );
  }

  if ( sim -> log )
  {
    sim -> out_log.printf( "%s gains %.2f (%.2f) %s from %s (%.2f/%.2f)",
                   name(), actual_amount, amount,
                   util::resource_type_string( resource_type ),
                   source ? source -> name() : action ? action -> name() : "unknown",
                   resources.current[ resource_type ], resources.max[ resource_type ] );
  }

  return actual_amount;
}

// player_t::resource_available =============================================

bool player_t::resource_available( resource_e resource_type,
                                   double cost ) const
{
  if ( resource_type == RESOURCE_NONE || cost <= 0 || resources.is_infinite( resource_type ) )
  {
    return true;
  }

  bool available = resources.current[ resource_type ] >= cost;

#ifndef NDEBUG
  if ( ! resources.active_resource[ resource_type ] )
  {
    assert( available && "Insufficient inactive resource to cast!" );
  }
#endif

  return available;
}

// player_t::recalculate_resources.max ======================================

void player_t::recalculate_resource_max( resource_e resource_type )
{
  resources.max[ resource_type ]  = resources.base[ resource_type ];
  resources.max[ resource_type ] *= resources.base_multiplier[ resource_type ];
  resources.max[ resource_type ] += total_gear.resource[ resource_type ];

  switch ( resource_type )
  {
    case RESOURCE_HEALTH:
    {
      // Calculate & set maximum health
      resources.max[ resource_type ] += floor( stamina() ) * current.health_per_stamina;

      // Make sure the player starts combat with full health
      if ( ! in_combat )
        resources.current[ resource_type ] = resources.max[ resource_type ];
      break;
    }
    default: break;
  }
  resources.max[ resource_type ] += resources.temporary[ resource_type ];

  resources.max[ resource_type ] *= resources.initial_multiplier[ resource_type ];
  // Sanity check on current values
  resources.current[ resource_type ] = std::min( resources.current[ resource_type ], resources.max[ resource_type ] );
}

// player_t::primary_role ===================================================

role_e player_t::primary_role() const
{
  return role;
}

// player_t::primary_tree_name ==============================================

const char* player_t::primary_tree_name() const
{
  return dbc::specialization_string( specialization() ).c_str();
}

// player_t::normalize_by ===================================================

stat_e player_t::normalize_by() const
{
  if ( sim -> normalized_stat != STAT_NONE )
    return sim -> normalized_stat;

  const scale_metric_e sm = this -> sim -> scaling -> scaling_metric;

  role_e role = primary_role();
  if ( role == ROLE_SPELL || role == ROLE_HEAL )
    return STAT_INTELLECT;
  else if ( role == ROLE_TANK && ( sm == SCALE_METRIC_TMI || sm == SCALE_METRIC_DEATHS ) && scaling[ sm ].get_stat( STAT_STAMINA) != 0.0 )
    return STAT_STAMINA;
  else if ( type == DRUID || type == HUNTER || type == SHAMAN || type == ROGUE || type == MONK )
    return STAT_AGILITY;
  else if ( type == DEATH_KNIGHT || type == PALADIN || type == WARRIOR )
    return STAT_STRENGTH;

  return STAT_ATTACK_POWER;
}

// player_t::health_percentage() ============================================

double player_t::health_percentage() const
{
  return resources.pct( RESOURCE_HEALTH ) * 100;
}

// player_t::max_health() ============================================

double player_t::max_health() const
{
  return resources.max[RESOURCE_HEALTH];
}

// player_t::current_health() ============================================

double player_t::current_health() const
{
  return resources.current[RESOURCE_HEALTH];
}

// target_t::time_to_percent ====================================================

timespan_t player_t::time_to_percent( double percent ) const
{
  timespan_t time_to_percent;
  double ttp;

  if ( iteration_dmg_taken > 0.0 && resources.base[RESOURCE_HEALTH] > 0 && sim -> current_time() >= timespan_t::from_seconds( 1.0 ) && !sim -> fixed_time )
    ttp = ( resources.current[RESOURCE_HEALTH] - ( percent * 0.01 * resources.base[RESOURCE_HEALTH] ) ) / ( iteration_dmg_taken / sim -> current_time().total_seconds() );
  else
    ttp = ( sim -> expected_iteration_time - sim -> current_time() ).total_seconds() * ( 100 - percent ) * 0.01;

  time_to_percent = timespan_t::from_seconds( ttp );

  if ( time_to_percent < timespan_t::zero() )
    return timespan_t::zero();
  else
    return time_to_percent;
}

// player_t::total_reaction_time ============================================

timespan_t player_t::total_reaction_time()
{
  return reaction_offset + rng().exgauss( reaction_mean, reaction_stddev, reaction_nu );
}

// player_t::stat_gain ======================================================

void player_t::stat_gain( stat_e    stat,
                          double    amount,
                          gain_t*   gain,
                          action_t* action,
                          bool      temporary_stat )
{
  if ( amount <= 0 ) return;

  // bail out if this is a stat that doesn't work for this class
  if ( convert_hybrid_stat( stat ) == STAT_NONE ) return;

  int temp_value = temporary_stat ? 1 : 0;

  cache_e cache_type = cache_from_stat( stat );
  if ( regen_type == REGEN_DYNAMIC && regen_caches[ cache_type ] )
    do_dynamic_regen();

  if ( sim -> log ) sim -> out_log.printf( "%s gains %.2f %s%s", name(), amount, util::stat_type_string( stat ), temporary_stat ? " (temporary)" : "" );

  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_STRENGTH:
    case STAT_AGILITY:
    case STAT_INTELLECT:
    case STAT_SPIRIT:
    case STAT_SPELL_POWER:
    case STAT_ATTACK_POWER:
    case STAT_EXPERTISE_RATING:
    case STAT_HIT_RATING:
    case STAT_CRIT_RATING:
    case STAT_ARMOR:
    case STAT_BONUS_ARMOR:
    case STAT_DODGE_RATING:
    case STAT_PARRY_RATING:
    case STAT_BLOCK_RATING:
    case STAT_MASTERY_RATING:
    case STAT_VERSATILITY_RATING:
    case STAT_LEECH_RATING:
    case STAT_AVOIDANCE_RATING:
    case STAT_SPEED_RATING:
      current.stats.add_stat( stat, amount );
      invalidate_cache( cache_type );
      break;

    case STAT_HASTE_RATING:
    {
      current.stats.add_stat( stat, amount );
      invalidate_cache( cache_type );

      adjust_dynamic_cooldowns();
      adjust_global_cooldown( HASTE_ANY );
      adjust_auto_attack( HASTE_ANY );
      // Queued execute must be adjusted after dynamid cooldowns / global cooldown
      adjust_action_queue_time();
      break;
    }

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
      {
        current.stats.attribute[ i ] += amount;
        invalidate_cache( static_cast<cache_e>( i ) );
      }
      break;

    case STAT_HEALTH:
    case STAT_MANA:
    case STAT_RAGE:
    case STAT_ENERGY:
    case STAT_FOCUS:
    case STAT_RUNIC:
    {
      resource_e r = ( ( stat == STAT_HEALTH ) ? RESOURCE_HEALTH :
                       ( stat == STAT_MANA   ) ? RESOURCE_MANA   :
                       ( stat == STAT_RAGE   ) ? RESOURCE_RAGE   :
                       ( stat == STAT_ENERGY ) ? RESOURCE_ENERGY :
                       ( stat == STAT_FOCUS  ) ? RESOURCE_FOCUS  : RESOURCE_RUNIC_POWER );
      resource_gain( r, amount, gain, action );
    }
    break;

    case STAT_MAX_HEALTH:
    case STAT_MAX_MANA:
    case STAT_MAX_RAGE:
    case STAT_MAX_ENERGY:
    case STAT_MAX_FOCUS:
    case STAT_MAX_RUNIC:
    {
      resource_e r = ( ( stat == STAT_MAX_HEALTH ) ? RESOURCE_HEALTH :
                       ( stat == STAT_MAX_MANA   ) ? RESOURCE_MANA   :
                       ( stat == STAT_MAX_RAGE   ) ? RESOURCE_RAGE   :
                       ( stat == STAT_MAX_ENERGY ) ? RESOURCE_ENERGY :
                       ( stat == STAT_MAX_FOCUS  ) ? RESOURCE_FOCUS  : RESOURCE_RUNIC_POWER );

      resources.temporary[ r ] += temp_value * amount;
      recalculate_resource_max( r );
      resource_gain( r, amount, gain, action );
    }
    break;

    // Unused but known stats to prevent asserting
    case STAT_RESILIENCE_RATING:
      break;

    default: assert( false ); break;
  }

  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_ALL:
    {
      recalculate_resource_max( RESOURCE_HEALTH );
      // Adjust current health to new max on stamina gains, if the actor is not in combat
      if ( ! in_combat )
      {
        double delta = resources.max[ RESOURCE_HEALTH ] - resources.current[ RESOURCE_HEALTH ];
        resource_gain( RESOURCE_HEALTH, delta );
      }
      break;
    }
    default: break;
  }
}

// player_t::stat_loss ======================================================

void player_t::stat_loss( stat_e    stat,
                          double    amount,
                          gain_t*   gain,
                          action_t* action,
                          bool      temporary_buff )
{
  if ( amount <= 0 ) return;

  // bail out if this is a stat that doesn't work for this class
  if ( convert_hybrid_stat( stat ) == STAT_NONE ) return;

  cache_e cache_type = cache_from_stat( stat );
  if ( regen_type == REGEN_DYNAMIC && regen_caches[ cache_type ] )
    do_dynamic_regen();

  if ( sim -> log ) sim -> out_log.printf( "%s loses %.2f %s%s", name(), amount, util::stat_type_string( stat ), ( temporary_buff ) ? " (temporary)" : "" );

  int temp_value = temporary_buff ? 1 : 0;
  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_STRENGTH:
    case STAT_AGILITY:
    case STAT_INTELLECT:
    case STAT_SPIRIT:
    case STAT_SPELL_POWER:
    case STAT_ATTACK_POWER:
    case STAT_EXPERTISE_RATING:
    case STAT_HIT_RATING:
    case STAT_CRIT_RATING:
    case STAT_ARMOR:
    case STAT_BONUS_ARMOR:
    case STAT_DODGE_RATING:
    case STAT_PARRY_RATING:
    case STAT_BLOCK_RATING:
    case STAT_MASTERY_RATING:
    case STAT_VERSATILITY_RATING:
    case STAT_LEECH_RATING:
    case STAT_AVOIDANCE_RATING:
    case STAT_SPEED_RATING:
      current.stats.add_stat( stat, -amount );
      invalidate_cache( cache_type );
      break;

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
      {
        current.stats.attribute  [ i ] -= amount;
        invalidate_cache( ( cache_e ) i );
      }
      break;

    case STAT_HEALTH:
    case STAT_MANA:
    case STAT_RAGE:
    case STAT_ENERGY:
    case STAT_FOCUS:
    case STAT_RUNIC:
    {
      resource_e r = ( ( stat == STAT_HEALTH ) ? RESOURCE_HEALTH :
                       ( stat == STAT_MANA   ) ? RESOURCE_MANA   :
                       ( stat == STAT_RAGE   ) ? RESOURCE_RAGE   :
                       ( stat == STAT_ENERGY ) ? RESOURCE_ENERGY :
                       ( stat == STAT_FOCUS  ) ? RESOURCE_FOCUS  : RESOURCE_RUNIC_POWER );
      resource_loss( r, amount, gain, action );
    }
    break;

    case STAT_MAX_HEALTH:
    case STAT_MAX_MANA:
    case STAT_MAX_RAGE:
    case STAT_MAX_ENERGY:
    case STAT_MAX_FOCUS:
    case STAT_MAX_RUNIC:
    {
      resource_e r = ( ( stat == STAT_MAX_HEALTH ) ? RESOURCE_HEALTH :
                       ( stat == STAT_MAX_MANA   ) ? RESOURCE_MANA   :
                       ( stat == STAT_MAX_RAGE   ) ? RESOURCE_RAGE   :
                       ( stat == STAT_MAX_ENERGY ) ? RESOURCE_ENERGY :
                       ( stat == STAT_MAX_FOCUS  ) ? RESOURCE_FOCUS  : RESOURCE_RUNIC_POWER );

      resources.temporary[ r ] -= temp_value * amount;
      recalculate_resource_max( r );
      double delta = resources.current[ r ] - resources.max[ r ];
      if ( delta > 0 ) resource_loss( r, delta, gain, action );

    }
    break;

    case STAT_HASTE_RATING:
    {
      current.stats.haste_rating   -= amount;
      invalidate_cache( cache_type );

      adjust_dynamic_cooldowns();
      adjust_global_cooldown( HASTE_ANY );
      adjust_auto_attack( HASTE_ANY );
      // Queued execute must be adjusted after dynamid cooldowns / global cooldown
      adjust_action_queue_time();
      break;
    }

    // Unused but known stats to prevent asserting
    case STAT_RESILIENCE_RATING:
      break;

    default: assert( false ); break;
  }

  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_ALL:
    {
      recalculate_resource_max( RESOURCE_HEALTH );
      // Adjust current health to new max on stamina gains
      double delta = resources.current[ RESOURCE_HEALTH ] - resources.max[ RESOURCE_HEALTH ];
      if ( delta > 0 )
        resource_loss( RESOURCE_HEALTH, delta, gain, action );
      break;
    }
    default: break;
  }
}

/* Adjusts the current rating -> % modifer of a stat
 * Invalidates corresponding cache
 */
void player_t::modify_current_rating( rating_e r, double amount )
{
  current.rating.get( r ) += amount;
  invalidate_cache( cache_from_rating( r ) );
}

// player_t::cost_reduction_gain ============================================

void player_t::cost_reduction_gain( school_e school,
                                    double        amount,
                                    gain_t*       /* gain */,
                                    action_t*     /* action */ )
{
  if ( amount <= 0 ) return;

  if ( sim -> log )
    sim -> out_log.printf( "%s gains a cost reduction of %.0f on abilities of school %s", name(), amount,
                   util::school_type_string( school ) );

  if ( school > SCHOOL_MAX_PRIMARY )
  {
    for ( school_e i = SCHOOL_NONE; ++i < SCHOOL_MAX_PRIMARY; )
    {
      if ( util::school_type_component( school, i ) )
      {
        current.resource_reduction[ i ] += amount;
      }
    }
  }
  else
  {
    current.resource_reduction[ school ] += amount;
  }
}

// player_t::cost_reduction_loss ============================================

void player_t::cost_reduction_loss( school_e school,
                                    double        amount,
                                    action_t*     /* action */ )
{
  if ( amount <= 0 ) return;

  if ( sim -> log )
    sim -> out_log.printf( "%s loses a cost reduction %.0f on abilities of school %s", name(), amount,
                   util::school_type_string( school ) );

  if ( school > SCHOOL_MAX_PRIMARY )
  {
    for ( school_e i = SCHOOL_NONE; ++i < SCHOOL_MAX_PRIMARY; )
    {
      if ( util::school_type_component( school, i ) )
      {
        current.resource_reduction[ i ] -= amount;
      }
    }
  }
  else
  {
    current.resource_reduction[ school ] -= amount;
  }
}

// player_t::get_raw_dps ==============================================

double player_t::get_raw_dps( action_state_t* s )
{
  double raw_dps = 0;

  if ( main_hand_attack && main_hand_attack -> execute_time() > timespan_t::zero() )
  {
    raw_dps = ( main_hand_attack -> base_da_min( s ) + main_hand_attack -> base_da_max( s ) ) / 2.0;
    raw_dps /= main_hand_attack -> execute_time().total_seconds();
  }

  return raw_dps;
}

namespace assess_dmg_helper_functions {

void account_parry_haste( player_t& p, action_state_t* s )
{
  if ( ! p.is_enemy() )
  {
    // Parry Haste accounting
    if ( s -> result == RESULT_PARRY )
    {
      if ( p.main_hand_attack && p.main_hand_attack -> execute_event )
      {
        // Parry haste mechanics:  When parrying an attack, the game subtracts 40% of the player's base swing timer
        // from the time remaining on the current swing timer.  However, this effect cannot reduce the current swing
        // timer to less than 20% of the base value.  The game uses hasted values.  To illustrate that, two examples:
        // base weapon speed: 2.6, 30% haste, thus base swing timer is 2.6/1.3=2.0 seconds
        // 1) if we parry when the current swing timer has 1.8 seconds remaining, then it gets reduced by 40% of 2.0, or 0.8 seconds,
        //    and the current swing timer becomes 1.0 seconds.
        // 2) if we parry when the current swing timer has 1.0 second remaining the game tries to subtract 0.8 seconds, but hits the
        //    minimum value (20% of 2.0, or 0.4 seconds.  The current swing timer becomes 0.4 seconds.
        // Thus, the result is that the current swing timer becomes max(current_swing_timer-0.4*base_swing_timer,0.2*base_swing_timer)

        // the reschedule_execute(x) function we call to perform this tries to reschedule the effect such that it occurs at
        // (sim->current_time() + x).  Thus we need to give it the difference between sim->current_time() and the new target of execute_event->occurs().
        // That value is simply the remaining time on the current swing timer.

        // first, we need the hasted base swing timer, swing_time
        timespan_t swing_time = p.main_hand_attack -> time_to_execute;

        // and we also need the time remaining on the current swing timer
        timespan_t current_swing_timer = p.main_hand_attack -> execute_event -> occurs() - p.sim-> current_time();

        // next, check that the current swing timer is longer than 0.2*swing_time - if not we do nothing
        if ( current_swing_timer > 0.20 * swing_time )
        {
          // now we apply parry-hasting.  Subtract 40% of base swing timer from current swing timer
          current_swing_timer -= 0.40 * swing_time;

          // enforce 20% base swing timer minimum
          current_swing_timer = std::max( current_swing_timer, 0.20 * swing_time );

          // now reschedule the event and log a parry haste
          p.main_hand_attack -> reschedule_execute( current_swing_timer );
          p.procs.parry_haste -> occur();
        }
      }
    }
  }
}

void account_blessing_of_sacrifice( player_t& p, action_state_t* s )
{
  if ( p.buffs.blessing_of_sacrifice -> check() )
  {
    // figure out how much damage gets redirected
    double redirected_damage = s -> result_amount * ( p.buffs.blessing_of_sacrifice -> data().effectN( 1 ).percent() );

    // apply that damage to the source paladin
    p.buffs.blessing_of_sacrifice -> trigger( s -> action, 0, redirected_damage, timespan_t::zero() );

    // mitigate that amount from the target.
    // Slight inaccuracy: We do not get a feedback of paladin health buffer expiration here.
    s -> result_amount -= redirected_damage;

    if ( p.sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      p.sim -> out_debug.printf( "Damage to %s after Blessing of Sacrifice is %f", s -> target -> name(), s -> result_amount );
  }
}

bool absorb_sort( absorb_buff_t* a, absorb_buff_t* b )
{
  return a -> current_value < b -> current_value;
}

void account_absorb_buffs( player_t& p, action_state_t* s, school_e school )
{
  /* ABSORB BUFFS

     std::vector<absorb_buff_t*> absorb_buff_list; is a dynamic vector, which contains
     the currently active absorb buffs of a player.

     std:map<int,instant_absorb_t*> instant_absorb_list; is a map by spell id that contains
     the data for instant absorb effects.

     std::vector<int> absorb_priority; is a vector that contains the sequencing for
     high priority (tank) absorbs and instant absorbs, by spell ID. High priority and
     instant absorbs MUST be pushed into this array inside init_absorb_priority() to
     function.

  */

  if ( ! ( p.absorb_buff_list.empty() && p.instant_absorb_list.empty() ) )
  {
    /* First, handle high priority absorbs and instant absorbs. These absorbs should
       obey the sequence laid out in absorb_priority. To achieve this, we loop through
       absorb_priority in order, then find and apply the appropriate high priority
       or instant absorb. */
    for ( size_t i = 0; i < p.absorb_priority.size(); i++ )
    {
      // Check for the absorb ID in instantaneous absorbs first, since its low cost.
      if ( p.instant_absorb_list.find( p.absorb_priority[ i ] ) != p.instant_absorb_list.end() )
      {
        instant_absorb_t* ab = p.instant_absorb_list[ p.absorb_priority[ i ] ];

        // eligibility is handled in the instant absorb's handler
        double absorbed = ab -> consume( s );

        s -> result_amount -= absorbed;
        s -> self_absorb_amount += absorbed;

        if ( p.sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() && absorbed != 0 )
          p.sim -> out_debug.printf( "Damage to %s after %s is %f", s -> target -> name(), ab -> name.c_str(), s -> result_amount );
      }
      else
      {
        // Check for the absorb ID in high priority absorbs.
        for ( size_t j = 0; j < p.absorb_buff_list.size(); j++ )
        {
          if ( p.absorb_buff_list[ j ] -> data().id() == p.absorb_priority[ i ] )
          {
            absorb_buff_t* ab = p.absorb_buff_list[ j ];

            assert( ab -> high_priority && "Absorb buff with set priority is not flagged for high priority." );

            if ( ( ( ab -> eligibility && ab -> eligibility( s ) ) // Use the eligibility function if there is one
              || ( school == SCHOOL_NONE || dbc::is_school( ab -> absorb_school, school ) ) ) // Otherwise check by school
              && ab -> up() )
            {
              double absorbed = ab -> consume( s -> result_amount );

              s -> result_amount -= absorbed;

              // track result using only self-absorbs separately
              if ( ab -> source == &p || p.is_my_pet( ab -> source ) )
                s -> self_absorb_amount += absorbed;

              if ( p.sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() && absorbed != 0 )
                p.sim -> out_debug.printf( "Damage to %s after %s is %f", s -> target -> name(), ab -> name(), s -> result_amount );
            }

            if ( ab -> current_value <= 0 )
              ab -> expire();

            break;
          }
        }
      }

      if ( s -> result_amount <= 0 )
      {
        assert( s -> result_amount == 0 );
        break;
      }
    }

    // Second, we handle any low priority absorbs. These absorbs obey the rule of "smallest first".

    // Sort the absorbs by size, then we can loop through them in order.
    std::sort( p.absorb_buff_list.begin(), p.absorb_buff_list.end(), absorb_sort );
    size_t offset = 0;

    while ( offset < p.absorb_buff_list.size() && s -> result_amount > 0 && ! p.absorb_buff_list.empty() )
    {
      absorb_buff_t* ab = p.absorb_buff_list[ offset ];

      /* Check absorb eligbility by school and custom eligibility function, skipping high priority
         absorbs since those have already been processed above. */
      if ( ab -> high_priority || ( ab -> eligibility && ! ab -> eligibility( s ) ) // Use the eligibility function if there is one
        || ( school != SCHOOL_NONE && ! dbc::is_school( ab -> absorb_school, school ) ) ) // Otherwise check by school
      {
        offset++;
        continue;
      }

      // Don't be too paranoid about inactive absorb buffs in the list. Just expire them
      if ( ab -> up() )
      {
        // Consume the absorb and grab the effective amount consumed.
        double absorbed = ab -> consume( s -> result_amount );

        s -> result_amount -= absorbed;

        // track result using only self-absorbs separately
        if ( ab -> source == &p || p.is_my_pet( ab -> source ) )
          s -> self_absorb_amount += absorbed;

        if ( p.sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
          p.sim -> out_debug.printf( "Damage to %s after %s is %f", s -> target -> name(), ab -> name(), s -> result_amount );

        if ( s -> result_amount <= 0 )
        {
          // Buff is not fully consumed
          assert( s -> result_amount == 0 );
          break;
        }
      }

      // So, it turns out it's possible to have absorb buff behavior, where
      // there's a "minimum value" for the absorb buff, even after absorbing
      // damage more than its current value. In this case, the absorb buff should
      // not be expired, as the current_value still has something left.
      if ( ab -> current_value <= 0 )
      {
        ab -> expire();
        assert( p.absorb_buff_list.empty() || p.absorb_buff_list[ 0 ] != ab );
      }
      else
        offset++;
    } // end of absorb list loop
  }

  p.iteration_absorb_taken += s -> self_absorb_amount;

  s -> result_absorbed = s -> result_amount;
}

void account_legendary_tank_cloak( player_t& p, action_state_t* s )
{
  // Legendary Tank Cloak Proc - max absorb of 1e7 hardcoded (in spellid 146193, effect 1)
  if ( p.legendary_tank_cloak_cd && p.legendary_tank_cloak_cd -> up()  // and the cloak's cooldown is up
       && s -> result_amount > p.resources.current[ RESOURCE_HEALTH ] ) // attack exceeds player health
  {
    if ( s -> result_amount > 1e7 )
    {
      p.gains.endurance_of_niuzao -> add( RESOURCE_HEALTH, 1e7, 0 );
      s -> result_amount -= 1e7;
      s -> result_absorbed += 1e7;
    }
    else
    {
      p.gains.endurance_of_niuzao -> add( RESOURCE_HEALTH, s -> result_amount, 0 );
      s -> result_absorbed += s -> result_amount;
      s -> result_amount = 0;
    }
    p.legendary_tank_cloak_cd -> start();
  }
}

/* Statistical data collection
 */
void collect_dmg_taken_data( player_t& p, const action_state_t* s, double result_ignoring_external_absorbs )
{
  p.iteration_dmg_taken += s -> result_amount;

  // collect data for timelines
  p.collected_data.timeline_dmg_taken.add( p.sim -> current_time(), s -> result_amount );

  // tank-specific data storage
  if ( p.collected_data.health_changes.collect )
  {
    // health_changes covers everything, used for ETMI and other things
    p.collected_data.health_changes.timeline.add( p.sim -> current_time(), s -> result_amount );
    p.collected_data.health_changes.timeline_normalized.add( p.sim -> current_time(), s -> result_amount / p.resources.max[ RESOURCE_HEALTH ] );

    // store value in incoming damage array for conditionals
    p.incoming_damage.push_back( std::pair<timespan_t, double>( p.sim -> current_time(), s -> result_amount ) );
  }
  if ( p.collected_data.health_changes_tmi.collect )
  {
    // health_changes_tmi ignores external effects (e.g. external absorbs), used for raw TMI
    p.collected_data.health_changes_tmi.timeline.add( p.sim -> current_time(), result_ignoring_external_absorbs );
    p.collected_data.health_changes_tmi.timeline_normalized.add( p.sim -> current_time(), result_ignoring_external_absorbs / p.resources.max[ RESOURCE_HEALTH ] );
  }
}

/* If Guardian spirit saves the life of the player, return true,
 * otherwise false.
 */
bool try_guardian_spirit( player_t& p, double actual_amount )
{
  // This can only save the target, if the damage is less than 200% of the target's health as of 4.0.6
  if ( ! p.is_enemy() && p.buffs.guardian_spirit -> check() && actual_amount <= ( p.resources.max[ RESOURCE_HEALTH] * 2 ) )
  {
    // Just assume that this is used so rarely that a strcmp hack will do
    //stats_t* stat = buffs.guardian_spirit -> source ? buffs.guardian_spirit -> source -> get_stats( "guardian_spirit" ) : 0;
    //double gs_amount = resources.max[ RESOURCE_HEALTH ] * buffs.guardian_spirit -> data().effectN( 2 ).percent();
    //resource_gain( RESOURCE_HEALTH, s -> result_amount );
    //if ( stat ) stat -> add_result( gs_amount, gs_amount, HEAL_DIRECT, RESULT_HIT );
    p.buffs.guardian_spirit -> expire();
    return true;
  }

  return false;
}

} // assess_dmg_helper_functions

// player_t::assess_damage ==================================================

void player_t::assess_damage( school_e school,
                              dmg_e    type,
                              action_state_t* s )
{
  using namespace assess_dmg_helper_functions;

  account_parry_haste( *this, s );

  if ( s -> result_amount <= 0.0 )
    return;

  target_mitigation( school, type, s );

  // store post-mitigation, pre-absorb value
  s -> result_mitigated = s -> result_amount;

  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> out_debug.printf( "Damage to %s after all mitigation is %f", s -> target -> name(), s -> result_amount );

  account_blessing_of_sacrifice( *this, s );

  assess_damage_imminent_pre_absorb( school, type, s );

  account_absorb_buffs( *this, s, school );

  assess_damage_imminent( school, type, s );

  account_legendary_tank_cloak( *this, s );
}

void player_t::do_damage( action_state_t* incoming_state )
{
  using namespace assess_dmg_helper_functions;

  double actual_amount = 0.0;
  collect_dmg_taken_data( *this, incoming_state,
    incoming_state -> result_mitigated - incoming_state -> self_absorb_amount );

  if ( incoming_state -> result_amount > 0.0 )
  {
    actual_amount = resource_loss( RESOURCE_HEALTH, incoming_state -> result_amount, nullptr,
      incoming_state -> action );
  }

  // New callback system; proc abilities on incoming events.
  // TODO: How to express action causing/not causing incoming callbacks?
  if ( incoming_state -> action && incoming_state -> action -> callbacks )
  {
    proc_types pt = incoming_state -> proc_type();
    proc_types2 pt2 = incoming_state -> execute_proc_type2();
    // For incoming landed abilities, get the impact type for the proc.
    //if ( pt2 == PROC2_LANDED )
    //  pt2 = s -> impact_proc_type2();

    // On damage/heal in. Proc flags are arranged as such that the "incoming"
    // version of the primary proc flag is always follows the outgoing version.
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      action_callback_t::trigger( callbacks.procs[pt + 1][pt2], incoming_state -> action, incoming_state );
  }

  // Check if target is dying
  if ( health_percentage() <= death_pct && ! resources.is_infinite( RESOURCE_HEALTH ) )
  {
    if ( ! try_guardian_spirit( *this, actual_amount ) )
    { // Player was not saved by guardian spirit, kill him
      if ( ! current.sleeping )
      {
        collected_data.deaths.add( sim -> current_time().total_seconds() );
      }
      if ( sim -> log ) sim -> out_log.printf( "%s has died.", name() );
      make_event<player_demise_event_t>( *sim, *this );
    }
  }
}

void player_t::assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* )
{

}

void player_t::assess_damage_imminent( school_e, dmg_e, action_state_t* )
{

}

// player_t::target_mitigation ==============================================

void player_t::target_mitigation( school_e school,
                                  dmg_e dmg_type,
                                  action_state_t* s )
{
  if ( s -> result_amount == 0 )
    return;

  if ( buffs.pain_supression && buffs.pain_supression -> up() )
    s -> result_amount *= 1.0 + buffs.pain_supression -> data().effectN( 1 ).percent();

  if ( buffs.naarus_discipline && buffs.naarus_discipline -> check() )
    s -> result_amount *= 1.0 + buffs.naarus_discipline -> stack_value();

  if ( buffs.stoneform && buffs.stoneform -> up() )
    s -> result_amount *= 1.0 + buffs.stoneform -> data().effectN( 1 ).percent();

  if ( buffs.fortitude && buffs.fortitude -> up() )
    s -> result_amount *= 1.0 + buffs.fortitude -> data().effectN( 1 ).percent();

  if ( s -> action -> is_aoe() )
    s -> result_amount *= 1.0 - cache.avoidance();

  // TODO-WOD: Where should this be? Or does it matter?
  s -> result_amount *= 1.0 - cache.mitigation_versatility();

  if ( school == SCHOOL_PHYSICAL && dmg_type == DMG_DIRECT )
  {
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s before armor mitigation is %f", s -> target -> name(), s -> result_amount );

    // Armor
    if ( s -> action )
    {
      double armor = s -> target_armor;
      double resist = armor / ( armor + s -> action -> player -> current.armor_coeff );
      resist = clamp( resist, 0.0, 0.85 );
      s -> result_amount *= 1.0 - resist;
    }

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after armor mitigation is %f", s -> target -> name(), s -> result_amount );

    double pre_block_amount = s -> result_amount;

    if ( s -> block_result == BLOCK_RESULT_BLOCKED )
    {
      s -> result_amount *= std::max( 0.0, 1 - composite_block_reduction() );
      if ( s -> result_amount <= 0 ) return;
    }

    if ( s -> block_result == BLOCK_RESULT_CRIT_BLOCKED )
    {
      s -> result_amount *=  std::max( 0.0, 1 - 2 * composite_block_reduction() );
      if ( s -> result_amount <= 0 ) return;
    }

    s -> blocked_amount = pre_block_amount  - s -> result_amount;

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() && s -> blocked_amount > 0.0)
      sim -> out_debug.printf( "Damage to %s after blocking is %f", s -> target -> name(), s -> result_amount );
  }
}

// player_t::assess_heal ====================================================

void player_t::assess_heal( school_e, dmg_e, action_state_t* s )
{
  // Increases to healing taken should modify result_total in order to correctly calculate overhealing
  // and other effects based on raw healing.
  if ( buffs.guardian_spirit -> up() )
    s -> result_total *= 1.0 + buffs.guardian_spirit -> data().effectN( 1 ).percent();

  // process heal
  s -> result_amount = resource_gain( RESOURCE_HEALTH, s -> result_total, 0, s -> action );

  // if the target is a tank record this event on damage timeline
  if ( ! is_pet() && primary_role() == ROLE_TANK )
  {
    // health_changes and timeline_healing_taken record everything, accounting for overheal and so on
    collected_data.timeline_healing_taken.add( sim -> current_time(), - ( s -> result_amount ) );
    collected_data.health_changes.timeline.add( sim -> current_time(), - ( s -> result_amount ) );
    collected_data.health_changes.timeline_normalized.add( sim -> current_time(), - ( s -> result_amount ) / resources.max[ RESOURCE_HEALTH ] );

    // health_changes_tmi ignores external healing - use result_total to count player overhealing as effective healing
    if (  s -> action -> player == this || is_my_pet( s -> action -> player ) )
    {
      collected_data.health_changes_tmi.timeline.add( sim -> current_time(), - ( s -> result_total ) );
      collected_data.health_changes_tmi.timeline_normalized.add( sim -> current_time(), - ( s -> result_total ) / resources.max[ RESOURCE_HEALTH ] );
    }
  }

  // store iteration heal taken
  iteration_heal_taken += s -> result_amount;
}

// player_t::summon_pet =====================================================

void player_t::summon_pet( const std::string& pet_name,
                           const timespan_t duration )
{
  if ( pet_t* p = find_pet( pet_name ) )
    p -> summon( duration );
  else
    sim -> errorf( "Player %s is unable to summon pet '%s'\n", name(), pet_name.c_str() );
}

// player_t::dismiss_pet ====================================================

void player_t::dismiss_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );
  assert( p );
  p -> dismiss();
}

// player_t::recent_cast ====================================================

bool player_t::recent_cast()
{
  return ( last_cast > timespan_t::zero() ) && ( ( last_cast + timespan_t::from_seconds( 5.0 ) ) > sim -> current_time() );
}

// player_t::find_dot =======================================================

dot_t* player_t::find_dot( const std::string& name,
                           player_t* source ) const
{
  for ( size_t i = 0; i < dot_list.size(); ++i )
  {
    dot_t* d = dot_list[ i ];
    if ( d -> source == source &&
         d -> name_str == name )
      return d;
  }
  return 0;
}

// player_t::clear_action_priority_lists() ==================================

void player_t::clear_action_priority_lists() const
{
  for ( size_t i = 0; i < action_priority_list.size(); i++ )
  {
    action_priority_list_t* a = action_priority_list[ i ];
    a -> action_list_str.clear();
    a -> action_list.clear();
  }
}

// player_t::copy_action_priority_lists() ==================================
// Replaces "old_list" action_priority_list data with "new_list" action_priority_list data

void player_t::copy_action_priority_list( const std::string& old_list, const std::string& new_list )
{
  action_priority_list_t* ol = find_action_priority_list( old_list );
  action_priority_list_t* nl = find_action_priority_list( new_list );

  if ( ol && nl )
  {
    ol -> action_list = nl -> action_list;
    ol -> action_list_str = nl -> action_list_str;
    ol -> foreground_action_list = nl -> foreground_action_list;
    ol -> off_gcd_actions = nl -> off_gcd_actions;
    ol -> random = nl -> random;
  }
}

template <typename T>
T* find_vector_member( const std::vector<T*>& list, const std::string& name )
{
  for (auto t : list)
  {

    if ( t -> name_str == name )
      return t;
  }
  return nullptr;
}

// player_t::find_action_priority_list( const std::string& name ) ===========

action_priority_list_t* player_t::find_action_priority_list( const std::string& name ) const
{ return find_vector_member( action_priority_list, name ); }

pet_t* player_t::find_pet( const std::string& name ) const
{ return find_vector_member( pet_list, name ); }

stats_t* player_t::find_stats( const std::string& name ) const
{ return find_vector_member( stats_list, name ); }

gain_t* player_t::find_gain ( const std::string& name ) const
{ return find_vector_member( gain_list, name ); }

proc_t* player_t::find_proc ( const std::string& name ) const
{ return find_vector_member( proc_list, name ); }

luxurious_sample_data_t* player_t::find_sample_data( const std::string& name ) const
{ return find_vector_member( sample_data_list, name ); }

benefit_t* player_t::find_benefit ( const std::string& name ) const
{ return find_vector_member( benefit_list, name ); }

uptime_t* player_t::find_uptime ( const std::string& name ) const
{ return find_vector_member( uptime_list, name ); }

cooldown_t* player_t::find_cooldown( const std::string& name ) const
{ return find_vector_member( cooldown_list, name ); }

action_t* player_t::find_action( const std::string& name ) const
{ return find_vector_member( action_list, name ); }

// player_t::get_cooldown ===================================================

cooldown_t* player_t::get_cooldown( const std::string& name )
{
  cooldown_t* c = find_cooldown( name );

  if ( !c )
  {
    c = new cooldown_t( name, *this );

    cooldown_list.push_back( c );
  }

  return c;
}

// player_t::get_rppm =======================================================

real_ppm_t* player_t::get_rppm( const std::string& name, const spell_data_t* data, const item_t* item )
{
  auto it = range::find_if( rppm_list, [ &name ]( const real_ppm_t* rppm ) {
    return util::str_compare_ci( rppm -> name(), name );
  } );

  if ( it != rppm_list.end() )
  {
    return *it;
  }

  real_ppm_t* new_rppm = new real_ppm_t( name, this, data, item );
  rppm_list.push_back( new_rppm );

  return new_rppm;
}

real_ppm_t* player_t::get_rppm( const std::string& name, double freq, double mod, rppm_scale_e s )
{
  auto it = range::find_if( rppm_list, [ &name ]( const real_ppm_t* rppm ) {
    return util::str_compare_ci( rppm -> name(), name );
  } );

  if ( it != rppm_list.end() )
  {
    return *it;
  }

  real_ppm_t* new_rppm = new real_ppm_t( name, this, freq, mod, s );
  rppm_list.push_back( new_rppm );

  return new_rppm;
}

// player_t::get_dot ========================================================

dot_t* player_t::get_dot( const std::string& name,
                          player_t* source )
{
  dot_t* d = find_dot( name, source );

  if ( ! d )
  {
    d = new dot_t( name, this, source );
    dot_list.push_back( d );
  }

  return d;
}

// player_t::get_gain =======================================================

gain_t* player_t::get_gain( const std::string& name )
{
  gain_t* g = find_gain( name );

  if ( !g )
  {
    g = new gain_t( name );

    gain_list.push_back( g );
  }

  return g;
}

// player_t::get_proc =======================================================

proc_t* player_t::get_proc( const std::string& name )
{
  proc_t* p = find_proc( name );

  if ( !p )
  {
    p = new proc_t( *sim, name );

    proc_list.push_back( p );
  }

  return p;
}

// player_t::get_sample_data =======================================================

luxurious_sample_data_t* player_t::get_sample_data( const std::string& name )
{
  luxurious_sample_data_t* sd = find_sample_data( name );

  if ( !sd )
  {
    sd = new luxurious_sample_data_t( *this, name );

    sample_data_list.push_back( sd );
  }

  return sd;
}

// player_t::get_stats ======================================================

stats_t* player_t::get_stats( const std::string& n, action_t* a )
{
  stats_t* stats = find_stats( n );

  if ( ! stats )
  {
    stats = new stats_t( n, this );

    stats_list.push_back( stats );
  }

  assert( stats -> player == this );

  if ( a )
    stats -> action_list.push_back( a );

  return stats;
}

// player_t::get_benefit ====================================================

benefit_t* player_t::get_benefit( const std::string& name )
{
  benefit_t* u = find_benefit( name );

  if ( !u )
  {
    u = new benefit_t( name );

    benefit_list.push_back( u );
  }

  return u;
}

// player_t::get_uptime =====================================================

uptime_t* player_t::get_uptime( const std::string& name )
{
  uptime_t* u = find_uptime( name );


  if ( !u )
  {
    u = new uptime_t(  name );

    uptime_list.push_back( u );
  }

  return u;
}

// player_t::get_action_priority_list( const std::string& name ) ============

action_priority_list_t* player_t::get_action_priority_list( const std::string& name, const std::string& comment )
{
  action_priority_list_t* a = find_action_priority_list( name );
  if ( ! a )
  {
    if ( action_list_id_ == 64 )
    {
      sim -> errorf( "%s maximum number of action lists is 64", name_str.c_str() );
      sim -> cancel();
    }

    a = new action_priority_list_t( name, this );
    a -> action_list_comment_str = comment;
    a -> internal_id = action_list_id_++;
    a -> internal_id_mask = 1ULL << ( a -> internal_id );

    action_priority_list.push_back( a );
  }
  return a;
}

// Wait For Cooldown Action =================================================

wait_for_cooldown_t::wait_for_cooldown_t( player_t* player, const std::string& cd_name ) :
  wait_action_base_t( player, "wait_for_" + cd_name ),
  wait_cd( player -> get_cooldown( cd_name ) ), a( player -> find_action( cd_name ) )
{
  assert( a );
  interrupt_auto_attack = false;
  quiet = true;
}

timespan_t wait_for_cooldown_t::execute_time() const
{ assert( wait_cd -> duration > timespan_t::zero() ); return wait_cd -> remains(); }

namespace { // ANONYMOUS

// Chosen Movement Actions ==================================================

struct start_moving_t : public action_t
{
  start_moving_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "start_moving", player )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 0.5 );
    harmful = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    player -> buffs.self_movement -> trigger();

    if ( sim -> log )
      sim -> out_log.printf( "%s starts moving.", player -> name() );

    update_ready();
  }

  virtual bool ready() override
  {
    if ( player -> buffs.self_movement -> check() )
      return false;

    return action_t::ready();
  }
};

struct stop_moving_t : public action_t
{
  stop_moving_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "stop_moving", player )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 0.5 );
    harmful = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    player -> buffs.self_movement -> expire();

    if ( sim -> log ) sim -> out_log.printf( "%s stops moving.", player -> name() );
    update_ready();
  }

  virtual bool ready() override
  {
    if ( ! player -> buffs.self_movement -> check() )
      return false;

    return action_t::ready();
  }
};

struct variable_t : public action_t
{
  action_var_e operation;
  action_variable_t* var;
  std::string value_str, var_name_str;
  expr_t* value_expression;

  variable_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_VARIABLE, "variable", player ),
    operation( OPERATION_SET ), var( nullptr ), value_expression( nullptr )
  {
    quiet = true;
    harmful = proc = callbacks = may_miss = may_crit = may_block = may_parry = may_dodge = false;
    trigger_gcd = timespan_t::zero();

    std::string operation_;
    double default_ = 0;
    timespan_t delay_;

    add_option( opt_string( "name", name_str ) );
    add_option( opt_string( "value", value_str ) );
    add_option( opt_string( "op", operation_ ) );
    add_option( opt_float( "default", default_ ) );
    add_option( opt_timespan( "delay", delay_ ) );
    parse_options( options_str );

    if ( name_str.empty() )
    {
      sim -> errorf( "Player %s unnamed 'variable' action used", player -> name() );
      background = true;
      return;
    }

    // Figure out operation
    if ( ! operation_.empty() )
    {
      if      ( util::str_compare_ci( operation_, "set"   ) ) operation = OPERATION_SET;
      else if ( util::str_compare_ci( operation_, "print" ) ) operation = OPERATION_PRINT;
      else if ( util::str_compare_ci( operation_, "reset" ) ) operation = OPERATION_RESET;
      else if ( util::str_compare_ci( operation_, "add"   ) ) operation = OPERATION_ADD;
      else if ( util::str_compare_ci( operation_, "sub"   ) ) operation = OPERATION_SUB;
      else if ( util::str_compare_ci( operation_, "mul"   ) ) operation = OPERATION_MUL;
      else if ( util::str_compare_ci( operation_, "div"   ) ) operation = OPERATION_DIV;
      else if ( util::str_compare_ci( operation_, "pow"   ) ) operation = OPERATION_POW;
      else if ( util::str_compare_ci( operation_, "mod"   ) ) operation = OPERATION_MOD;
      else if ( util::str_compare_ci( operation_, "min"   ) ) operation = OPERATION_MIN;
      else if ( util::str_compare_ci( operation_, "max"   ) ) operation = OPERATION_MAX;
      else if ( util::str_compare_ci( operation_, "floor" ) ) operation = OPERATION_FLOOR;
      else if ( util::str_compare_ci( operation_, "ceil"  ) ) operation = OPERATION_CEIL;
      else
      {
        sim -> errorf( "Player %s unknown operation '%s' given for variable, valid values are 'set', 'print', and 'reset'.", player -> name(), operation_.c_str() );
        background = true;
        return;
      }
    }

    // Printing needs a delay, otherwise the action list will not progress
    if ( operation == OPERATION_PRINT && delay_ == timespan_t::zero() )
      delay_ = timespan_t::from_seconds( 1.0 );

    if ( operation != OPERATION_FLOOR && operation != OPERATION_CEIL && operation != OPERATION_RESET &&
         operation != OPERATION_PRINT )
    {
      if ( value_str.empty() )
      {
        sim -> errorf( "Player %s no value expression given for variable '%s'", player -> name(), name_str.c_str() );
        background = true;
        return;
      }
    }

    // Add a delay
    if ( delay_ > timespan_t::zero() )
    {
      std::string cooldown_name = "variable_actor";
      cooldown_name += util::to_string( player -> index );
      cooldown_name += "_";
      cooldown_name += name_str;

      cooldown = player -> get_cooldown( cooldown_name );
      cooldown -> duration = delay_;
    }

    // Find the variable
    for ( auto& elem : player -> variables )
    {
      if ( util::str_compare_ci( elem -> name_, name_str ) )
      {
        var = elem;
        break;
      }
    }

    if ( ! var )
    {
      player -> variables.push_back( new action_variable_t( name_str, default_ ) );
      var = player -> variables.back();
    }
  }

  void init() override
  {
    action_t::init();

    if ( ! background &&
         operation != OPERATION_FLOOR && operation != OPERATION_CEIL &&
         operation != OPERATION_RESET && operation != OPERATION_PRINT )
    {
      value_expression = expr_t::parse( this, value_str );
      if ( ! value_expression )
      {
        sim -> errorf( "Player %s unable to parse 'variable' value '%s'", player -> name(), value_str.c_str() );
        background = true;
      }
    }
  }

  ~variable_t()
  {
    delete value_expression;
  }

  // Note note note, doesn't do anything that a real action does
  void execute() override
  {
    if ( sim -> debug && operation != OPERATION_PRINT )
    {
      sim -> out_debug.printf( "%s variable name=%s op=%d value=%f default=%f sig=%s",
        player -> name(), var -> name_.c_str(), operation, var -> current_value_, var -> default_, signature_str.c_str() );
    }

    switch ( operation )
    {
      case OPERATION_SET:
        var -> current_value_ = value_expression -> eval();
        break;
      case OPERATION_ADD:
        var -> current_value_ += value_expression -> eval();
        break;
      case OPERATION_SUB:
        var -> current_value_ -= value_expression -> eval();
        break;
      case OPERATION_MUL:
        var -> current_value_ *= value_expression -> eval();
        break;
      case OPERATION_DIV:
      {
        auto v = value_expression -> eval();
        // Disallow division by zero, set value to zero
        if ( v == 0 )
        {
          var -> current_value_ = 0;
        }
        else
        {
          var -> current_value_ /= v;
        }
        break;
      }
      case OPERATION_POW:
        var -> current_value_ = std::pow( var -> current_value_, value_expression -> eval() );
        break;
      case OPERATION_MOD:
      {
        // Disallow division by zero, set value to zero
        auto v = value_expression -> eval();
        if ( v == 0 )
        {
          var -> current_value_ = 0;
        }
        else
        {
          var -> current_value_ = std::fmod( var -> current_value_, value_expression -> eval() );
        }
        break;
      }
      case OPERATION_MIN:
        var -> current_value_ = std::min( var -> current_value_, value_expression -> eval() );
        break;
      case OPERATION_MAX:
        var -> current_value_ = std::max( var -> current_value_, value_expression -> eval() );
        break;
      case OPERATION_FLOOR:
        var -> current_value_ = util::floor( var -> current_value_ );
        break;
      case OPERATION_CEIL:
        var -> current_value_ = util::ceil( var -> current_value_ );
        break;
      case OPERATION_PRINT:
        // Only spit out prints in main thread
        if ( sim -> parent == 0 )
          std::cout << "actor=" << player -> name_str << " time=" << sim -> current_time().total_seconds()
            << " iteration=" << sim -> current_iteration << " variable=" << var -> name_.c_str()
            << " value=" << var -> current_value_ << std::endl;
        break;
      case OPERATION_RESET:
        var -> reset();
        break;
      default:
        assert( 0 );
        break;
    }
  }
};

// ===== Racial Abilities ===================================================

struct racial_spell_t : public spell_t
{
  racial_spell_t( player_t* p, const std::string& token, const spell_data_t* spell, const std::string& options ) :
    spell_t( token, p, spell )
  {
    parse_options( options );
  }

  void init() override
  {
    spell_t::init();

    if ( &data() == &spell_data_not_found_t::singleton )
      background = true;
  }
};

// Shadowmeld ===============================================================

struct shadowmeld_t : public racial_spell_t
{
  shadowmeld_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "shadowmeld", p -> find_racial_spell( "Shadowmeld" ), options_str )
  { }

  void execute() override
  {
    racial_spell_t::execute();

    player -> buffs.shadowmeld -> trigger();

    // Shadowmeld stops autoattacks
    if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      event_t::cancel( player -> main_hand_attack -> execute_event );

    if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
      event_t::cancel( player -> off_hand_attack -> execute_event );
  }
};

// Arcane Torrent ===========================================================

struct arcane_torrent_t : public racial_spell_t
{
  double gain_pct;
  double gain_energy;

  arcane_torrent_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "arcane_torrent", p -> find_racial_spell( "Arcane Torrent" ), options_str ),
    gain_pct( 0 ), gain_energy( 0 )
  {
    energize_type = ENERGIZE_ON_CAST;
    // Some specs need special handling here
    switch ( p -> specialization() )
    {
      case MAGE_FROST:
      case MAGE_FIRE:
      case MAGE_ARCANE:
      case WARLOCK_AFFLICTION:
      case WARLOCK_DEMONOLOGY:
      case WARLOCK_DESTRUCTION:
        gain_pct = data().effectN( 2 ).percent();
        break;
      case MONK_MISTWEAVER:
        gain_pct = data().effectN( 3 ).percent();
        break;
      case MONK_WINDWALKER:
      {
        parse_effect_data( data().effectN( 2 ) ); // Chi
        gain_energy = data().effectN( 4 ).base_value(); // Energy
        break;
      }
      case MONK_BREWMASTER:
        gain_energy = data().effectN( 4 ).base_value();
        break;
      case PALADIN_HOLY:
        gain_pct = data().effectN( 3 ).percent();
        break;
      default:
        break;
    }
  }

  void execute() override
  {
    racial_spell_t::execute();

    if ( gain_pct > 0 )
    {
      double gain = player -> resources.max [ RESOURCE_MANA ] * gain_pct;
      player -> resource_gain( RESOURCE_MANA, gain, player -> gains.arcane_torrent );
    }

    if ( gain_energy > 0 )
      player -> resource_gain( RESOURCE_ENERGY, gain_energy, player -> gains.arcane_torrent );
  }
};

// Berserking ===============================================================

struct berserking_t : public racial_spell_t
{
  berserking_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "berserking", p -> find_racial_spell( "Berserking" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player -> buffs.berserking -> trigger();
  }
};

// Blood Fury ===============================================================

struct blood_fury_t : public racial_spell_t
{
  blood_fury_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "blood_fury", p -> find_racial_spell( "Blood Fury" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player -> buffs.blood_fury -> trigger();
  }
};

// Darkflight ==============================================================

struct darkflight_t : public racial_spell_t
{
  darkflight_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "darkflight", p -> find_racial_spell( "Darkflight" ), options_str )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player -> buffs.darkflight -> trigger();
  }
};

// Rocket Barrage ===========================================================

struct rocket_barrage_t : public racial_spell_t
{
  rocket_barrage_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "rocket_barrage", p -> find_racial_spell( "Rocket Barrage" ), options_str )
  {
    parse_options( options_str );
  }
};

// Stoneform ================================================================

struct stoneform_t : public racial_spell_t
{
  stoneform_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "stoneform", p -> find_racial_spell( "Stoneform" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player -> buffs.stoneform -> trigger();
  }
};

// Restart Sequence Action ==================================================

struct restart_sequence_t : public action_t
{
  sequence_t* seq;
  std::string seq_name_str;

  restart_sequence_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "restart_sequence", player ),
    seq( 0 ), seq_name_str( "default" ) // matches default name for sequences
  {
    add_option( opt_string( "name", seq_name_str ) );
    parse_options( options_str );
    ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void init() override
  {
    action_t::init();

    if ( ! seq )
    {
      for ( size_t i = 0; i < player -> action_list.size() && !seq; ++i )
      {
        action_t* a = player -> action_list[ i ];
        if ( a && a -> type != ACTION_SEQUENCE )
          continue;

        if ( ! seq_name_str.empty() )
          if ( a && seq_name_str != a -> name_str )
            continue;

        seq = dynamic_cast<sequence_t*>( a );
      }

      if ( !seq )
      {
        sim -> errorf( "Can't find sequence %s\n",
                       seq_name_str.empty() ? "(default)" : seq_name_str.c_str() );
        sim -> cancel();
        return;
      }
    }
  }

  virtual void execute() override
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s restarting sequence %s", player -> name(), seq_name_str.c_str() );
    seq -> restart();
  }

  virtual bool ready() override
  {
    bool ret = seq && seq -> can_restart();
    if ( ret ) return action_t::ready();

    return ret;
  }
};

// Restore Mana Action ======================================================

struct restore_mana_t : public action_t
{
  double mana;

  restore_mana_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "restore_mana", player ),
    mana( 0 )
  {
    add_option( opt_float( "mana", mana ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    double mana_missing = player -> resources.max[ RESOURCE_MANA ] - player -> resources.current[ RESOURCE_MANA ];
    double mana_gain = mana;

    if ( mana_gain == 0 || mana_gain > mana_missing ) mana_gain = mana_missing;

    if ( mana_gain > 0 )
    {
      player -> resource_gain( RESOURCE_MANA, mana_gain, player -> gains.restore_mana );
    }
  }
};

// Snapshot Stats ===========================================================

struct snapshot_stats_t : public action_t
{
  bool completed;
  spell_t*  proxy_spell;
  attack_t* proxy_attack;
  role_e role;

  snapshot_stats_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "snapshot_stats", player ),
    completed( false ), proxy_spell( 0 ), proxy_attack( 0 ),
    role( player -> primary_role() )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL )
    {
      proxy_spell = new spell_t( "snapshot_spell", player );
      proxy_spell -> background = true;
      proxy_spell -> callbacks = false;
    }

    if ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK )
    {
      proxy_attack = new melee_attack_t( "snapshot_attack", player );
      proxy_attack -> background = true;
      proxy_attack -> callbacks = false;
    }
  }

  bool init_finished() override
  {
    player_t* p = player;
    for ( size_t i = 0; i < p -> pet_list.size(); ++i )
    {
      pet_t* pet = p -> pet_list[ i ];
      action_t* pet_snapshot = pet -> find_action( "snapshot_stats" );
      if ( ! pet_snapshot )
      {
        pet_snapshot = pet -> create_action( "snapshot_stats", "" );
        pet_snapshot -> init();
      }
    }

    return action_t::init_finished();
  }

  virtual void execute() override
  {
    player_t* p = player;

    if ( completed ) return;

    completed = true;

    if ( sim -> current_iteration > 0 ) return;

    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", p -> name(), name() );

    player_collected_data_t::buffed_stats_t& buffed_stats = p -> collected_data.buffed_stats_snapshot;

    for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; ++i )
      buffed_stats.attribute[ i ] = floor( p -> get_attribute( i ) );

    buffed_stats.resource     = p -> resources.max;

    buffed_stats.spell_haste  = p -> cache.spell_haste();
    buffed_stats.spell_speed  = p -> cache.spell_speed();
    buffed_stats.attack_haste = p -> cache.attack_haste();
    buffed_stats.attack_speed = p -> cache.attack_speed();
    buffed_stats.mastery_value = p -> cache.mastery_value();
    buffed_stats.bonus_armor = p -> composite_bonus_armor();
    buffed_stats.damage_versatility = p -> cache.damage_versatility();
    buffed_stats.heal_versatility = p -> cache.heal_versatility();
    buffed_stats.mitigation_versatility = p -> cache.mitigation_versatility();
    buffed_stats.run_speed = p -> cache.run_speed();
    buffed_stats.avoidance = p -> cache.avoidance();
    buffed_stats.leech = p -> cache.leech();

    buffed_stats.spell_power  = util::round( p -> cache.spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier() );
    buffed_stats.spell_hit    = p -> cache.spell_hit();
    buffed_stats.spell_crit_chance = p -> cache.spell_crit_chance();
    buffed_stats.manareg_per_second          = p -> mana_regen_per_second();

    buffed_stats.attack_power = p -> cache.attack_power() * p -> composite_attack_power_multiplier();
    buffed_stats.attack_hit   = p -> cache.attack_hit();
    buffed_stats.mh_attack_expertise = p -> composite_melee_expertise( &( p -> main_hand_weapon ) );
    buffed_stats.oh_attack_expertise = p -> composite_melee_expertise( &( p -> off_hand_weapon ) );
    buffed_stats.attack_crit_chance  = p -> cache.attack_crit_chance();

    buffed_stats.armor        = p -> composite_armor();
    buffed_stats.miss         = p -> composite_miss();
    buffed_stats.dodge        = p -> cache.dodge();
    buffed_stats.parry        = p -> cache.parry();
    buffed_stats.block        = p -> cache.block();
    buffed_stats.crit         = p -> cache.crit_avoidance();

    double spell_hit_extra = 0, attack_hit_extra = 0, expertise_extra = 0;

    // The code below is not properly handling the case where the player has
    // so much Hit Rating or Expertise Rating that the extra amount of stat
    // they have is higher than the delta.

    // In this case, the following line in sc_scaling.cpp
    //     if ( divisor < 0.0 ) divisor += ref_p -> over_cap[ i ];
    // would give divisor a positive value (whereas it is expected to always
    // remain negative).
    // Also, if a player has an extra amount of Hit Rating or Expertise Rating
    // that is equal to the ``delta'', the following line in sc_scaling.cpp
    //     double score = ( delta_score - ref_score ) / divisor;
    // will cause a division by 0 error.
    // We would need to increase the delta before starting the scaling sim to remove this error

    if ( role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL )
    {
      double chance = proxy_spell -> miss_chance( proxy_spell -> composite_hit(), sim -> target );
      if ( chance < 0 ) spell_hit_extra = -chance * p -> current_rating().spell_hit;
    }

    if ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK )
    {
      double chance = proxy_attack -> miss_chance( proxy_attack -> composite_hit(), sim -> target );
      if ( p -> dual_wield() ) chance += 0.19;
      if ( chance < 0 ) attack_hit_extra = -chance * p -> current_rating().attack_hit;
      if ( p -> position() != POSITION_FRONT )
      {
        chance = proxy_attack -> dodge_chance( p -> cache.attack_expertise(), sim -> target );
        if ( chance < 0 ) expertise_extra = -chance * p -> current_rating().expertise;
      }
      else if ( p -> position() == POSITION_FRONT )
      {
        chance = proxy_attack -> parry_chance( p -> cache.attack_expertise(), sim -> target );
        if ( chance < 0 ) expertise_extra = -chance * p -> current_rating().expertise;
      }
    }

    p -> over_cap[ STAT_HIT_RATING ] = std::max( spell_hit_extra, attack_hit_extra );
    p -> over_cap[ STAT_EXPERTISE_RATING ] = expertise_extra;

    for ( size_t i = 0; i < p -> pet_list.size(); ++i )
    {
      pet_t* pet = p -> pet_list[ i ];
      action_t* pet_snapshot = pet -> find_action( "snapshot_stats" );
      if ( pet_snapshot )
      {
        pet_snapshot -> execute();
      }
    }
  }

  virtual void reset() override
  {
    action_t::reset();

    completed = false;
  }

  virtual bool ready() override
  {
    if ( completed || sim -> current_iteration > 0 ) return false;
    return action_t::ready();
  }
};

// Wait Fixed Action ========================================================

struct wait_fixed_t : public wait_action_base_t
{
  std::unique_ptr<expr_t> time_expr;

  wait_fixed_t( player_t* player, const std::string& options_str ) :
    wait_action_base_t( player, "wait" ),
    time_expr()
  {
    std::string sec_str = "1.0";

    add_option( opt_string( "sec", sec_str ) );
    parse_options( options_str );
    interrupt_auto_attack = false; //Probably shouldn't interrupt autoattacks while waiting.
    quiet = true;

    time_expr = std::unique_ptr<expr_t>( expr_t::parse( this, sec_str ) );
    if ( ! time_expr )
    {
      sim -> errorf( "%s: Unable to generate wait expression from '%s'", player -> name(), options_str.c_str() );
      background = true;
    }
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t wait = timespan_t::from_seconds( time_expr -> eval() );
    if ( wait <= timespan_t::zero() ) wait = player -> available();
    return wait;
  }
};

// Wait Until Ready Action ==================================================

// wait until actions *before* this wait are ready.
struct wait_until_ready_t : public wait_fixed_t
{
  wait_until_ready_t( player_t* player, const std::string& options_str ):
    wait_fixed_t( player, options_str )
  {
    interrupt_auto_attack = false;
    quiet = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t wait = wait_fixed_t::execute_time();
    timespan_t remains = timespan_t::zero();

    for ( size_t i = 0; i < player -> action_list.size(); ++i )
    {
      action_t* a = player -> action_list[ i ];
      assert( a );
      if ( a == NULL ) // For release builds.
        break;
      if ( a == this )
        break;
      if ( a -> background ) continue;

      remains = a -> cooldown -> remains();
      if ( remains > timespan_t::zero() && remains < wait ) wait = remains;

      remains = a -> get_dot() -> remains();
      if ( remains > timespan_t::zero() && remains < wait ) wait = remains;
    }

    if ( wait <= timespan_t::zero() ) wait = player -> available();

    return wait;
  }
};

// Use Item Action ==========================================================

struct use_item_t : public action_t
{
  item_t* item;
  std::string use_name;
  action_t* action;
  buff_t* buff;
  cooldown_t* cooldown_group;
  timespan_t cooldown_group_duration;

  use_item_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "use_item", player ),
    item( nullptr ), action( nullptr ), buff( nullptr ),
    cooldown_group( nullptr ), cooldown_group_duration( timespan_t::zero() )
  {
    std::string item_name, item_slot;

    add_option( opt_string( "name", item_name ) );
    add_option( opt_string( "slot", item_slot ) );
    parse_options( options_str );

    if ( ! item_name.empty() )
    {
      item = player -> find_item( item_name );
      if ( ! item )
      {
        sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which is not currently equipped.\n", player -> name(), item_name.c_str() );
        return;
      }

      name_str = name_str + "_" + item_name;
    }
    else if ( ! item_slot.empty() )
    {
      slot_e s = util::parse_slot_type( item_slot );
      if ( s == SLOT_INVALID )
      {
        sim -> errorf( "Player %s attempting 'use_item' action with invalid slot name '%s'.", player -> name(), item_slot.c_str() );
        background = true;
      }

      item = &( player -> items[ s ] );

      if ( ! item || ! item -> active() )
      {
        sim -> errorf( "Player %s attempting 'use_item' action with invalid item '%s' in slot '%s'.", player -> name(), item -> name(), item_slot.c_str() );
        item = 0;
        background = true;
      }
      else
      {
        name_str = name_str + "_" + item -> name();
      }
    }
    else
    {
      sim -> errorf( "Player %s has 'use_item' action with no 'name=' or 'slot=' option.\n", player -> name() );
      background = true;
    }
  }

  void init() override
  {
    action_t::init();

    if ( ! item )
      return;

    // Parse Special Effect
    const special_effect_t& e = item -> special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
    if ( e.type == SPECIAL_EFFECT_USE )
    {
      // Create a buff
      if ( e.buff_type() != SPECIAL_EFFECT_BUFF_NONE )
      {
        buff = buff_t::find( player, e.name() );
        if ( ! buff )
        {
          buff = e.create_buff();
        }
      }

      // Create an action
      if ( e.action_type() != SPECIAL_EFFECT_ACTION_NONE )
      {
        action = e.create_action();
      }

      stats = player ->  get_stats( name_str, this );

      // Setup the long-duration cooldown for this item effect
      cooldown = player -> get_cooldown( e.cooldown_name() );
      cooldown -> duration = e.cooldown();
      trigger_gcd = timespan_t::zero();

      // Use DBC-backed item cooldown system for any item data coming from our local database that
      // has no user-given 'use' option on items.
      if ( e.item && util::str_compare_ci( e.item -> source_str, "local" ) &&
           e.item -> option_use_str.empty() )
      {
        std::string cdgrp = e.cooldown_group_name();
        // No cooldown group will not trigger a shared cooldown
        if ( ! cdgrp.empty() )
        {
          cooldown_group = player -> get_cooldown( cdgrp );
          cooldown_group_duration = e.cooldown_group_duration();
        }
        else
        {
          cooldown_group = &( player -> item_cooldown );
        }
      }
      else
      {
        cooldown_group = &( player -> item_cooldown );
        // Presumes on-use items will always have static durations. Considering the client data
        // system hardcodes the cooldown group durations in the DBC files, this is a reasonably safe
        // bet for now.
        if ( buff )
        {
          cooldown_group_duration = buff -> buff_duration;
        }
      }
    }

    if ( ! buff && ! action )
    {
      sim -> errorf( "Player %s has 'use_item' action with no custom buff or action setup.\n", player -> name() );
      background = true;
      return;
    }
  }

  virtual void execute() override
  {
    bool triggered = buff == 0;
    if ( buff )
      triggered = buff -> trigger();

    if ( triggered && action &&
         ( ! buff || buff -> check() == buff -> max_stack() ) )
    {
      action -> schedule_execute();

      // Decide whether to expire the buff even with 1 max stack
      if ( buff && buff -> max_stack() > 1 )
      {
        buff -> expire();
      }
    }

    // Enable to report use_item ability
    //if ( ! dual ) stats -> add_execute( time_to_execute );

    update_ready();

    // Start the shared cooldown
    if ( cooldown_group_duration > timespan_t::zero() )
    {
      cooldown_group -> start( cooldown_group_duration );
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s starts shared cooldown for %s (%s). Will be ready at %.4f",
            player -> name(), name(), cooldown_group -> name(),
            cooldown_group -> ready.total_seconds() );
      }
    }
  }

  virtual bool ready() override
  {
    if ( ! item ) return false;

    if ( cooldown_group -> remains() > timespan_t::zero() )
    {
      return false;
    }

    if ( action && ! action -> ready() )
    {
      return false;
    }

    return action_t::ready();
  }

  expr_t* create_special_effect_expr( const std::vector<std::string>& data_str_split )
  {
    struct use_item_buff_type_expr_t : public expr_t
    {
      double v;

      use_item_buff_type_expr_t( bool state ) :
        expr_t( "use_item_buff_type" ),
        v( state ? 1.0 : 0 )
      { }

      bool is_constant( double* ) override
      { return true; }

      double evaluate() override
      { return v; }
    };

    if ( data_str_split.size() != 2 || ! util::str_compare_ci( data_str_split[ 0 ], "use_buff" ) )
    {
      return 0;
    }

    stat_e stat = util::parse_stat_type( data_str_split[ 1 ] );
    if ( stat == STAT_NONE )
    {
      return 0;
    }

    const special_effect_t& e = item -> special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );

    return new use_item_buff_type_expr_t( e.stat_type() == stat );
  }

  expr_t* create_expression( const std::string& name_str ) override
  {
    std::vector<std::string> split = util::string_split( name_str, "." );
    if ( expr_t* e = create_special_effect_expr( split ) )
    {
      return e;
    }

    return action_t::create_expression( name_str );
  }
};

// Cancel Buff ==============================================================

struct cancel_buff_t : public action_t
{
  buff_t* buff;

  cancel_buff_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "cancel_buff", player ), buff( 0 )
  {
    std::string buff_name;
    add_option( opt_string( "name", buff_name ) );
    parse_options( options_str );
    ignore_false_positive = true;
    if ( buff_name.empty() )
    {
      sim -> errorf( "Player %s uses cancel_buff without specifying the name of the buff\n", player -> name() );
      sim -> cancel();
    }

    buff = buff_t::find( player, buff_name );

    // if the buff isn't in the player_t -> buff_list, try again in the player_td_t -> target -> buff_list
    if ( ! buff )
    {
      buff = buff_t::find( player -> get_target_data( player ) -> target, buff_name );
    }

    if ( ! buff )
    {
      sim -> errorf( "Player %s uses cancel_buff with unknown buff %s\n", player -> name(), buff_name.c_str() );
      sim -> cancel();
    }
    else if ( !buff -> can_cancel )
    {
      sim -> errorf( "Player %s uses cancel_buff on %s, which cannot be cancelled in game\n", player -> name(), buff_name.c_str() );
      sim -> cancel();
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    if ( sim -> log ) sim -> out_log.printf( "%s cancels buff %s", player -> name(), buff -> name() );
    buff -> expire();
  }

  virtual bool ready() override
  {
    if ( ! buff || ! buff -> check() )
      return false;

    return action_t::ready();
  }
};

struct swap_action_list_t : public action_t
{
  action_priority_list_t* alist;

  swap_action_list_t( player_t* player, const std::string& options_str, const std::string& name = "swap_action_list" ) :
    action_t( ACTION_OTHER, name, player ), alist( 0 )
  {
    std::string alist_name;
    int randomtoggle = 0;
    add_option( opt_string( "name", alist_name ) );
    add_option( opt_int( "random", randomtoggle ) );
    parse_options( options_str );
    ignore_false_positive = true;
    if ( alist_name.empty() )
    {
      sim -> errorf( "Player %s uses %s without specifying the name of the action list\n", player -> name(), name.c_str() );
      sim -> cancel();
    }

    alist = player -> find_action_priority_list( alist_name );

    if ( ! alist )
    {
      sim -> errorf( "Player %s uses %s with unknown action list %s\n", player -> name(), name.c_str(), alist_name.c_str() );
      sim -> cancel();
    }
    else if ( randomtoggle == 1 )
      alist -> random = randomtoggle;

    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;
  }

  virtual void execute() override
  {
    if ( sim -> log ) sim -> out_log.printf( "%s swaps to action list %s", player -> name(), alist -> name_str.c_str() );
    player -> activate_action_list( alist );
  }

  virtual bool ready() override
  {
    if ( player -> active_action_list == alist )
      return false;

    return action_t::ready();
  }
};

struct run_action_list_t : public swap_action_list_t
{
  run_action_list_t( player_t* player, const std::string& options_str ) :
    swap_action_list_t( player, options_str, "run_action_list" )
  {
    quiet = true;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    if ( sim -> log ) sim -> out_log.printf( "%s runs action list %s", player -> name(), alist -> name_str.c_str() );

    if ( player -> restore_action_list == 0 ) player -> restore_action_list = player -> active_action_list;
    player -> activate_action_list( alist );
  }
};

/* Pool Resource
 *
 * If the next action in the list would be "ready" if it was not constrained by resource,
 * then this command will pool resource until we have enough.
 */

struct pool_resource_t : public action_t
{
  resource_e resource;
  std::string resource_str;
  timespan_t wait;
  int for_next;
  action_t* next_action;
  std::string amount_str;
  expr_t* amount_expr;

  pool_resource_t( player_t* p, const std::string& options_str, resource_e r = RESOURCE_NONE ) :
    action_t( ACTION_OTHER, "pool_resource", p ),
    resource( r != RESOURCE_NONE ? r : p -> primary_resource() ),
    wait( timespan_t::from_seconds( 0.251 ) ),
    for_next( 0 ),
    next_action( 0 ), amount_expr( nullptr )
  {
    quiet = true;
    add_option( opt_timespan( "wait", wait ) );
    add_option( opt_bool( "for_next", for_next ) );
    add_option( opt_string( "resource", resource_str ) );
    add_option( opt_string( "extra_amount", amount_str ) );
    parse_options( options_str );

    if ( !resource_str.empty() )
    {
      resource_e res = util::parse_resource_type( resource_str );
      if ( res != RESOURCE_NONE )
        resource = res;
    }
  }

  ~pool_resource_t()
  {
    delete amount_expr;
  }

  bool init_finished() override
  {
    if ( ! action_t::init_finished() )
    {
      return false;
    }

    if ( ! amount_str.empty() )
    {
      return ( amount_expr = expr_t::parse( this, amount_str, sim -> optimize_expressions ) ) != 0;
    }

    return true;
  }

  virtual void reset() override
  {
    action_t::reset();

    if ( ! next_action && for_next )
    {
      for ( size_t i = 0; i < player -> action_priority_list.size(); i++ )
      {
        for ( size_t j = 0; j < player -> action_priority_list[ i ] -> foreground_action_list.size(); j++ )
        {
          if ( player -> action_priority_list[ i ] -> foreground_action_list[ j ] != this )
            continue;

          if ( ++j != player -> action_priority_list[ i ] -> foreground_action_list.size() )
          {
            next_action = player -> action_priority_list[ i ] -> foreground_action_list[ j ];
            break;
          }
        }
      }

      if ( ! next_action )
      {
        sim -> errorf( "%s: can't find next action.\n", __FUNCTION__ );
        background = true;
      }
    }
  }

  virtual void execute() override
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    player -> iteration_pooling_time += wait;
  }

  virtual timespan_t gcd() const override
  {
    return wait;
  }

  virtual bool ready() override
  {
    bool rd = action_t::ready();
    if ( ! rd )
      return rd;

    if ( next_action )
    {
      if ( next_action -> ready() )
        return false;

      // If the next action in the list would be "ready" if it was not constrained by energy,
      // then this command will pool energy until we have enough.

      double theoretical_cost = next_action -> cost() + ( amount_expr ? amount_expr -> eval() : 0 );
      player -> resources.current[ resource ] += theoretical_cost;

      bool resource_limited = next_action -> ready();

      player -> resources.current[ resource ] -= theoretical_cost;

      if ( ! resource_limited )
        return false;
    }

    return rd;
  }
};


} // UNNAMED NAMESPACE

// player_t::create_action ==================================================

action_t* player_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "arcane_torrent"     ) return new     arcane_torrent_t( this, options_str );
  if ( name == "berserking"         ) return new         berserking_t( this, options_str );
  if ( name == "blood_fury"         ) return new         blood_fury_t( this, options_str );
  if ( name == "darkflight"         ) return new         darkflight_t( this, options_str );
  if ( name == "shadowmeld"         ) return new         shadowmeld_t( this, options_str );
  if ( name == "cancel_buff"        ) return new        cancel_buff_t( this, options_str );
  if ( name == "swap_action_list"   ) return new   swap_action_list_t( this, options_str );
  if ( name == "run_action_list"    ) return new    run_action_list_t( this, options_str );
  if ( name == "call_action_list"   ) return new   call_action_list_t( this, options_str );
  if ( name == "restart_sequence"   ) return new   restart_sequence_t( this, options_str );
  if ( name == "restore_mana"       ) return new       restore_mana_t( this, options_str );
  if ( name == "rocket_barrage"     ) return new     rocket_barrage_t( this, options_str );
  if ( name == "sequence"           ) return new           sequence_t( this, options_str );
  if ( name == "strict_sequence"    ) return new    strict_sequence_t( this, options_str );
  if ( name == "snapshot_stats"     ) return new     snapshot_stats_t( this, options_str );
  if ( name == "start_moving"       ) return new       start_moving_t( this, options_str );
  if ( name == "stoneform"          ) return new          stoneform_t( this, options_str );
  if ( name == "stop_moving"        ) return new        stop_moving_t( this, options_str );
  if ( name == "use_item"           ) return new           use_item_t( this, options_str );
  if ( name == "wait"               ) return new         wait_fixed_t( this, options_str );
  if ( name == "wait_until_ready"   ) return new   wait_until_ready_t( this, options_str );
  if ( name == "pool_resource"      ) return new      pool_resource_t( this, options_str );
  if ( name == "variable"           ) return new           variable_t( this, options_str );

  return consumable::create_action( this, name, options_str );
}

// player_t::parse_talents_numbers ==========================================

bool player_t::parse_talents_numbers( const std::string& talent_string )
{
  talent_points.clear();

  int i_max = std::min( static_cast<int>( talent_string.size() ),
                        MAX_TALENT_ROWS );

  for ( int i = 0; i < i_max; ++i )
  {
    char c = talent_string[ i ];
    if ( c < '0' || c > ( '0' + MAX_TALENT_COLS )  )
    {
      sim -> errorf( "Player %s has illegal character '%c' in talent encoding.\n", name(), c );
      return false;
    }
    if ( c > '0' )
      talent_points.select_row_col( i, c - '1' );
  }

  create_talents_numbers();

  return true;
}

// player_t::parse_talents_armory ===========================================

bool player_t::parse_talents_armory( const std::string& talent_string )
{
  talent_points.clear();

  if ( talent_string.size() < 2 )
  {
    sim -> errorf( "Player %s has malformed MoP battle.net talent string. Empty or too short string.\n", name() );
    return false;
  }

  // Verify class
  {
    player_e w_class = PLAYER_NONE;
    switch ( talent_string[ 0 ] )
    {
      case 'd' : w_class = DEATH_KNIGHT; break;
      case 'g' : w_class = DEMON_HUNTER; break;
      case 'U' : w_class = DRUID; break;
      case 'Y' : w_class = HUNTER; break;
      case 'e' : w_class = MAGE; break;
      case 'f' : w_class = MONK; break;
      case 'b' : w_class = PALADIN; break;
      case 'X' : w_class = PRIEST; break;
      case 'c' : w_class = ROGUE; break;
      case 'W' : w_class = SHAMAN; break;
      case 'V' : w_class = WARLOCK; break;
      case 'Z' : w_class = WARRIOR; break;
      default:
        sim -> errorf( "Player %s has malformed talent string '%s': invalid class character '%c'.\n", name(),
                       talent_string.c_str(), talent_string[ 0 ] );
        return false;
    }

    if ( w_class != type )
    {
      sim -> errorf( "Player %s has malformed talent string '%s': specified class %s does not match player class %s.\n", name(),
                     talent_string.c_str(), util::player_type_string( w_class ), util::player_type_string( type ) );
      return false;
    }
  }

  std::string::size_type cut_pt = talent_string.find( '!' );
  if ( cut_pt  == talent_string.npos )
  {
    sim -> errorf( "Player %s has malformed talent string '%s'.\n", name(), talent_string.c_str() );
    return false;
  }

  std::string spec_string = talent_string.substr( 1, cut_pt - 1 );
  if ( ! spec_string.empty() )
  {
    unsigned specidx = 0;
    // A spec was specified
    switch ( spec_string[ 0 ] )
    {
      case 'a' : specidx = 0; break;
      case 'Z' : specidx = 1; break;
      case 'b' : specidx = 2; break;
      case 'Y' : specidx = 3; break;
      default:
        sim -> errorf( "Player %s has malformed talent string '%s': invalid spec character '%c'.\n",
                       name(), talent_string.c_str(), spec_string[ 0 ] );
        return false;
    }

    _spec = dbc.spec_by_idx( type, specidx );
  }

  std::string t_str = talent_string.substr( cut_pt + 1 );
  if ( t_str.empty() )
  {
    // No talents picked.
    return true;
  }

//  if ( t_str.size() < MAX_TALENT_ROWS )
//  {
//    sim -> errorf( "Player %s has malformed talent string '%s': talent list too short.\n",
//                   name(), talent_string.c_str() );
//    return false;
//  }

  for ( size_t i = 0; i < std::min( t_str.size(), (size_t)MAX_TALENT_ROWS ); ++i )
  {
    switch ( t_str[ i ] )
    {
      case '.':
        break;
      case '0':
      case '1':
      case '2':
        talent_points.select_row_col( static_cast<int>( i ), t_str[ i ] - '0' );
        break;
      default:
        sim -> errorf( "Player %s has malformed talent string '%s': talent list has invalid character '%c'.\n",
                       name(), talent_string.c_str(), t_str[ i ] );
        return false;
    }
  }

  create_talents_armory();

  return true;
}

// player_t::create_talents_wowhead =========================================

void player_t::create_talents_wowhead()
{
  // The Wowhead talent scheme encodes three talent selections per character
  // in at most two characters to represent all six talent choices. Basically,
  // each "row" of choices is numbered from 0 (no choice) to 3 (rightmost
  // talent). For each set of 3 rows, total together
  //   <first row choice> + (4 * <second row>) + (16 * <third row>)
  // If that total is zero, the encoding character is omitted. If non-zero,
  // add the total to the ascii code for '/' to obtain the encoding
  // character.
  //
  // Decoding is pretty simple, subtract '/' from the encoded character to
  // obtain the original total, the row choices are then total % 4,
  // (total / 4) % 4, and (total / 16) % 4 respectively.

  talents_str.clear();
  std::string result = "http://www.wowhead.com/talent#";

  // Class Type
  {
    char c;
    switch ( type )
    {
      case DEATH_KNIGHT:  c = 'k'; break;
      case DRUID:         c = 'd'; break;
      case HUNTER:        c = 'h'; break;
      case MAGE:          c = 'm'; break;
      case MONK:          c = 'n'; break;
      case PALADIN:       c = 'l'; break;
      case PRIEST:        c = 'p'; break;
      case ROGUE:         c = 'r'; break;
      case SHAMAN:        c = 's'; break;
      case WARLOCK:       c = 'o'; break;
      case WARRIOR:       c = 'w'; break;
      default:
        return;
    }
    result += c;
  }

  // Spec if specified
  {
    uint32_t idx = 0;
    uint32_t cid = 0;
    if ( dbc.spec_idx( _spec, cid, idx ) && ( ( int ) cid == util::class_id( type ) ) )
    {
      switch ( idx )
      {
        case 0: result += '!'; break;
        case 1: result += 'x'; break;
        case 2: result += 'y'; break;
        case 3: result += 'z'; break;
        default: break;
      }
    }
  }

  int encoding[ 2 ] = { 0, 0 };

  for ( int tier = 0; tier < 2; ++tier )
  {
    for ( int row = 0, multiplier = 1; row < 3; ++row, multiplier *= 4 )
    {
      for ( int col = 0; col < MAX_TALENT_COLS; ++col )
      {
        if ( talent_points.has_row_col( ( tier * 3 ) + row, col ) )
        {
          encoding[ tier ] += ( col + 1 ) * multiplier;
          break;
        }
      }
    }
  }

  if ( encoding[ 0 ] == 0 )
  {
    if ( encoding[ 1 ] == 0 )
      return;
    // The representation for NO talent selected in all 3 rows is to omit the
    // character; astute observers will see right away that talent specs with
    // no selections in the first three rows but with talents in the second 3
    // rows will break the encoding scheme.
    //
    // Select the first talent in the first tier as a workaround.
    encoding[ 0 ] = 1;
  }

  result += encoding[ 0 ] + '/';
  if ( encoding[ 1 ] != 0 )
    result += encoding[ 1 ] + '/';

  talents_str = result;
}

/// Get average item level the player is wearing
double player_t::avg_item_level() const
{
  double avg_ilvl = 0.0;
  int num_ilvl_items = 0;
  for ( const auto& item : items )
  {
    if ( item.slot != SLOT_SHIRT && item.slot != SLOT_TABARD
        && item.slot != SLOT_RANGED && item.active() )
    {
      avg_ilvl += item.item_level();
      num_ilvl_items++;
    }
  }

  if ( num_ilvl_items > 1 )
    avg_ilvl /= num_ilvl_items;

  return avg_ilvl;
}

void player_t::create_talents_armory()
{
  if ( is_enemy() ) return;

  talents_str.clear();
  std::string result = "http://";

  std::string region = region_str;
  if ( region.empty() && ! origin_str.empty() )
  {
    std::string server, name;
    if ( util::parse_origin( region, server, name, origin_str ) )
    {
      region_str = region;
      server_str = server;
    }
  }
  if ( region.empty() )
    region = sim  -> default_region_str;
  if ( region.empty() )
    region = "us";
  result += region;

  result += ".battle.net/wow/en/tool/talent-calculator#";

  {
    char c;
    switch ( type )
    {
      case DEATH_KNIGHT : c = 'd'; break;
      case DEMON_HUNTER : c = 'g'; break;
      case DRUID        : c = 'U'; break;
      case HUNTER       : c = 'Y'; break;
      case MAGE         : c = 'e'; break;
      case MONK         : c = 'f'; break;
      case PALADIN      : c = 'b'; break;
      case PRIEST       : c = 'X'; break;
      case ROGUE        : c = 'c'; break;
      case SHAMAN       : c = 'W'; break;
      case WARLOCK      : c = 'V'; break;
      case WARRIOR      : c = 'Z'; break;
      default:
        return;
    }
    result += c;
  }

  if ( _spec != SPEC_NONE )
  {
    uint32_t cid, sid;

    if ( ! dbc.spec_idx( _spec, cid, sid ) )
    {
      assert( false );
      return;
    }

    switch ( sid )
    {
      case 0: result += 'a'; break;
      case 1: result += 'Z'; break;
      case 2: result += 'b'; break;
      case 3: result += 'Y'; break;
      default:
        assert( false );
        return;
    }
  }

  result += '!';

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    bool found = false;
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      if ( talent_points.has_row_col( j, i ) )
      {
        result += util::to_string( i );
        found = true;
        break;
      }
    }
    if ( ! found )
      result += '.';
  }

  talents_str = result;
}

void player_t::create_talents_numbers()
{
  talents_str.clear();

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    talents_str += util::to_string( talent_points.choice( j ) + 1 );
  }
}

// player_t::parse_talents_wowhead ==========================================

bool player_t::parse_talents_wowhead( const std::string& talent_string )
{
  talent_points.clear();

  if ( talent_string.empty() )
  {
    sim -> errorf( "Player %s has empty wowhead talent string.\n", name() );
    return false;
  }

  // Verify class
  {
    player_e w_class;
    switch ( talent_string[ 0 ] )
    {
      case 'k' : w_class = DEATH_KNIGHT; break;
      case 'd' : w_class = DRUID; break;
      case 'h' : w_class = HUNTER; break;
      case 'm' : w_class = MAGE; break;
      case 'n' : w_class = MONK; break;
      case 'l' : w_class = PALADIN; break;
      case 'p' : w_class = PRIEST; break;
      case 'r' : w_class = ROGUE; break;
      case 's' : w_class = SHAMAN; break;
      case 'o' : w_class = WARLOCK; break;
      case 'w' : w_class = WARRIOR; break;
      default:
        sim -> errorf( "Player %s has malformed wowhead talent string '%s': invalid class character '%c'.\n",
                       name(), talent_string.c_str(), talent_string[ 0 ] );
        return false;
    }

    if ( w_class != type )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string '%s': specified class %s does not match player class %s.\n",
                     name(), talent_string.c_str(), util::player_type_string( w_class ), util::player_type_string( type ) );
      return false;
    }
  }

  std::string::size_type idx = 1;

  // Parse spec if specified
  {
    int w_spec_idx;
    switch ( talent_string[ idx++ ] )
    {
      case '!': w_spec_idx = 0; break;
      case 'x': w_spec_idx = 1; break;
      case 'y': w_spec_idx = 2; break;
      case 'z': w_spec_idx = 3; break;
      default:  w_spec_idx = -1; --idx; break;
    }

    if ( w_spec_idx >= 0 )
      _spec = dbc.spec_by_idx( type, w_spec_idx );
  }

  if ( talent_string.size() > idx + 2 )
  {
    sim -> errorf( "Player %s has malformed wowhead talent string '%s': too long.\n",
                   name(), talent_string.c_str() );
    return false;
  }

  for ( int tier = 0; tier < 2 && idx + tier < talent_string.size(); ++tier )
  {
    // Process 3 rows of talents per encoded character.
    int total = static_cast<unsigned char>( talent_string[ idx + tier ] );

    if ( ( total < '0' ) || ( total > '/' + 3 * ( 1 + 4 + 16 ) ) )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string '%s': encoded character '%c' in position %d is invalid.\n",
                     name(), talent_string.c_str(), total, ( int )idx );
      return false;
    }

    total -= '/';

    for ( int row = 0; row < 3; ++row, total /= 4 )
    {
      int selection = total % 4;
      if ( selection )
        talent_points.select_row_col( 3 * tier + row, selection - 1 );
    }
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "Player %s wowhead talent string translation: '%s'\n",
                   name(), talent_points.to_string().c_str() );
  }

  create_talents_wowhead();

  return true;
}

// player_t::parse_artifact_wowdb ===========================================

bool player_t::parse_artifact_wowdb( const std::string& artifact_string )
{
  static const std::string decode( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/_" );

  auto data_offset = artifact_string.find( '#' );
  if ( data_offset == std::string::npos )
  {
    return false;
  }

  std::string artifact_data = artifact_string.substr( data_offset + 2, 10 );

  for ( size_t idx = 0; idx < artifact_data.size(); ++idx )
  {
    auto value = decode.find( artifact_data[ idx ] );
    if ( value == std::string::npos )
    {
      continue;
    }

    artifact.points[ idx * 2 ].first = value & 0x7;
    artifact.points[ idx * 2 + 1 ].first = (value & 0x38) >> 3;

    artifact.n_points += artifact.points[ idx * 2 ].first;
    artifact.n_points += artifact.points[ idx * 2 + 1 ].first;
  }

  return true;
}

// player_t::parse_artifact_wowhead =========================================

bool player_t::parse_artifact_wowhead( const std::string& artifact_string )
{
  auto splits = util::string_split( artifact_string, ":" );
  if ( splits.size() < 5 )
  {
    return false;
  }

  unsigned artifact_id = util::to_unsigned( splits[ 0 ] );
  size_t n_relics = 0, n_excess_points = 0, relic_idx = 0;
  for ( size_t i = 1; i < 5; ++i )
  {
    if ( ! util::str_compare_ci( splits[ i ], "0" ) )
    {
      artifact.relics[ relic_idx ] = util::to_unsigned( splits[ i ] );
      n_relics++;
    }
    relic_idx++;
  }

  auto artifact_powers = dbc.artifact_powers( artifact_id );
  for ( size_t i = 5; i < splits.size(); i += 2 )
  {
    unsigned power_id = util::to_unsigned( splits[ i ] );
    unsigned rank = util::to_unsigned( splits[ i + 1 ] );

    auto pos = range::find_if( artifact_powers,
                               [ &power_id ]( const artifact_power_data_t* power_data ) {
                                 return power_data -> id == power_id;
                               } );

    if ( pos != artifact_powers.end() )
    {
      artifact.points[ pos - artifact_powers.begin() ].first = rank;
      // Sanitize the input on any ranks > 3 so that we can get accurate purchased ranks for the
      // user, even if the string contains zero relic ids. Note the power type check, power type 5
      // is the 20 rank "final" power.
      if ( n_relics == 0 && rank > 3 && ( *pos ) -> power_type != 5 )
      {
        n_excess_points += rank - 3;
      }

      artifact.n_points += rank;
    }
  }

  if ( artifact.n_points < ( n_relics == 0 ? n_excess_points : n_relics ) )
  {
    artifact.n_purchased_points = 0;
  }
  else
  {
    artifact.n_purchased_points = artifact.n_points - static_cast<unsigned int>(( n_relics == 0 ? n_excess_points : n_relics ));
  }

  // The initial power does not count towards the purchased points
  if ( artifact.n_purchased_points > 0 )
  {
    artifact.n_purchased_points--;
  }

  return true;
}

bool parse_min_gcd( sim_t* sim,
    const std::string& name,
    const std::string& value )
{
  if ( name != "min_gcd" ) return false;

  double v = std::strtod( value.c_str(), nullptr );
  if ( v <= 0 )
  {
    sim -> errorf( " %s: Invalid value '%s' for global minimum cooldown.",
        sim -> active_player -> name(), value.c_str() );
    return false;
  }

  sim -> active_player -> min_gcd = timespan_t::from_seconds( v );
  return true;
}

// player_t::override_artifact ==============================================

void player_t::override_artifact( const std::vector<const artifact_power_data_t*>& powers, const std::string& override_str )
{
  std::string::size_type split = override_str.find( ':' );

  if ( split == std::string::npos )
  {
    sim -> errorf( "artifact_override: Invalid override_str %s for player %s.\n", override_str.c_str(), name() );
    return;
  }

  const std::string& override_rank_str = override_str.substr( split + 1, override_str.size() );

  if ( override_rank_str.empty() )
  {
    sim -> errorf( "artifact_override: Invalid override_str %s for player %s.\n", override_str.c_str(), name() );
    return;
  }

  std::string name = override_str.substr( 0, split );
  util::tokenize( name );

  const artifact_power_data_t* power_data = nullptr;
  size_t power_index = 0;

  // Find the power by name
  for ( power_index = 0; power_index < powers.size(); ++power_index )
  {
    const artifact_power_data_t* power = powers[ power_index ];
    if ( power -> name == 0 )
    {
      continue;
    }

    std::string power_name = power -> name;
    util::tokenize( power_name );

    if ( util::str_compare_ci( name, power_name ) )
    {
      power_data = power;
      break;
    }
  }

  if ( ! power_data )
  {
    sim -> errorf( "artifact_override: Override artifact power %s not found for player %s.\n", override_str.c_str(), this -> name() );
    return;
  }

  unsigned override_rank = util::to_unsigned( override_rank_str );

  // 1 rank powers use the zeroth (only) entry, multi-rank spells have 0 -> max rank entries
  std::vector<const artifact_power_rank_t*> ranks = dbc.artifact_power_ranks( power_data -> id );

  // Rank data missing for the power
  if ( override_rank > ranks.size() )
  {
    sim -> errorf( "artifact_override: %s too high rank (%u/%u) given for artifact power %s",
        this -> name(), override_rank, ranks.size(),
        power_data -> name ? power_data -> name : "Unknown");
    return;
  }

  if ( sim -> debug )
  {
    sim -> out_log.printf( "artifact_override: Player %s overrides power %s to rank %u.",
      this -> name(), name.c_str(), override_rank );
  }

  artifact.points[ power_index ].first = override_rank;
  artifact.points[ power_index ].second = 0; // If someone is trying to override the artifact, they probably don't want to take relic gems into account.
}

// player_t::replace_spells =================================================

// TODO: HOTFIX handling
void player_t::replace_spells()
{
  uint32_t class_idx, spec_index;

  if ( ! dbc.spec_idx( _spec, class_idx, spec_index ) )
    return;

  // Search spec spells for spells to replace.
  if ( _spec != SPEC_NONE )
  {
    for ( unsigned int i = 0; i < dbc.specialization_ability_size(); i++ )
    {
      unsigned id = dbc.specialization_ability( class_idx, spec_index, i );
      if ( id == 0 )
      {
        break;
      }
      const spell_data_t* s = dbc.spell( id );
      if ( s -> replace_spell_id() && ( ( int )s -> level() <= true_level ) )
      {
        // Found a spell we should replace
        dbc.replace_id( s -> replace_spell_id(), id );
      }
    }
  }

  // Search talents for spells to replace.
  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      if ( talent_points.has_row_col( j, i ) && true_level < std::min( ( j + 1 ) * 15, 100 ) )
      {
        talent_data_t* td = talent_data_t::find( type, j, i, specialization(), dbc.ptr );
        if ( td && td -> replace_id() )
        {
          dbc.replace_id( td -> replace_id(), td -> spell_id() );
          break;
        }
      }
    }
  }

  // Search glyph spells for spells to replace.
  for ( unsigned int j = 0; j < GLYPH_MAX; j++ )
  {
    for ( unsigned int i = 0; i < dbc.glyph_spell_size(); i++ )
    {
      unsigned id = dbc.glyph_spell( class_idx, j, i );
      if ( id == 0 )
      {
        break;
      }
      const spell_data_t* s = dbc.spell( id );
      if ( s -> replace_spell_id() )
      {
        // Found a spell that might need replacing. Check to see if we have that glyph activated
        for ( std::vector<const spell_data_t*>::iterator it = glyph_list.begin(); it != glyph_list.end(); ++it )
        {
          assert( *it );
          if ( ( *it ) -> id() == id )
          {
            dbc.replace_id( s -> replace_spell_id(), id );
          }
        }
      }
    }
  }

  // Search general spells for spells to replace (a spell you learn earlier might be replaced by one you learn later)
  if ( _spec != SPEC_NONE )
  {
    for ( unsigned int i = 0; i < dbc.class_ability_size(); i++ )
    {
      unsigned id = dbc.class_ability( class_idx, 0, i );
      if ( id == 0 )
      {
        break;
      }
      const spell_data_t* s = dbc.spell( id );
      if ( s -> replace_spell_id() && ( ( int )s -> level() <= true_level ) )
      {
        // Found a spell we should replace
        dbc.replace_id( s -> replace_spell_id(), id );
      }
    }
  }
}


/* Retrieves the Spell Data Associated with a given talent.
 * If the player does not have have the talent activated, or the talent is not found,
 * spell_data_t::not_found() is returned.
 *
 * The talent search by name is case sensitive, including all special characters!
 */
const spell_data_t* player_t::find_talent_spell( const std::string& n,
                                                 const std::string& token,
                                                 specialization_e s,
                                                 bool name_tokenized,
                                                 bool check_validity ) const
{
  if ( s == SPEC_NONE ) {
    s = specialization();
  }

  // Get a talent's spell id for a given talent name
  unsigned spell_id = dbc.talent_ability_id( type, s, n.c_str(), name_tokenized );

  if ( ! spell_id && token.empty() )
    spell_id = dbc::get_token_id( n );

  if ( !spell_id && sim -> debug )
    sim -> out_debug.printf( "Player %s: Can't find talent with name %s.\n",
                   name(), n.c_str() );

  if ( ! spell_id )
    return spell_data_t::not_found();

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_data_t* td = talent_data_t::find( type, j, i, s, dbc.ptr );
      // Loop through all our classes talents, and check if their spell's id match the one we maped to the given talent name
      if ( td && ( td -> spell_id() == spell_id ) )
      {
        // check if we have the talent enabled or not
        // std::min( 100, x ) dirty fix so that we can access tier 7 talents at level 100 and not level 105
        if ( check_validity && ( ! talent_points.has_row_col( j, i ) || true_level < std::min( ( j + 1 ) * 15, 100 ) ) )
          return spell_data_t::not_found();

        // We have that talent enabled.
        dbc.add_token( spell_id, token );

        return dbc::find_spell( this, td -> spell_id() );
      }
    }
  }

  /* Talent not enabled */
  return spell_data_t::not_found();
}

// player_t::find_glyph =====================================================

const spell_data_t* player_t::find_glyph( const std::string& n ) const
{
  if ( unsigned spell_id = dbc.glyph_spell_id( type, n.c_str() ) )
  {
    return dbc::find_spell( this, spell_id );
  }
  else
    return spell_data_t::not_found();
}


// player_t::find_glyph_spell ===============================================

const spell_data_t* player_t::find_glyph_spell( const std::string& /* n */, const std::string& /* token */ ) const
{
  /*
  if ( const spell_data_t* g = find_glyph( n ) )
  {
    for ( std::vector<const spell_data_t*>::const_iterator i = glyph_list.begin(); i != glyph_list.end(); ++i )
    {
      assert( *i );
      if ( ( *i ) -> id() == g -> id() )
      {
        if ( dbc::get_token( g -> id() ).empty() )
          dbc.add_token( g -> id(), token );

        return dbc::find_spell( this, g );
      }
    }
  }
  */
  return spell_data_t::not_found();
}

// player_t::find_specialization_spell ======================================

const spell_data_t* player_t::find_specialization_spell( const std::string& name, const std::string& token, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    if ( unsigned spell_id = dbc.specialization_ability_id( _spec, name.c_str() ) )
    {
      const spell_data_t* spell = dbc.spell( spell_id );
      if ( ( ( int )spell -> level() <= true_level ) )
      {
        if ( dbc::get_token( spell_id ).empty() )
          dbc.add_token( spell_id, token );

        return dbc::find_spell( this, spell );
      }
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_specialization_spell( unsigned spell_id, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    auto spell = dbc.spell( spell_id );
    if ( dbc.is_specialization_ability( _spec, spell_id ) &&
         ( as<int>( spell -> level() ) <= true_level ) )
    {
      return dbc::find_spell( this, spell );
    }
  }

  return spell_data_t::not_found();
}

// player_t::find_artifact_spell ==========================================

artifact_power_t player_t::find_artifact_spell( const std::string& name, bool tokenized ) const
{
  if ( ! artifact_enabled() )
  {
    return artifact_power_t();
  }

  // Find the artifact for the player
  unsigned artifact_id = dbc.artifact_by_spec( specialization() );
  if ( artifact_id == 0 )
  {
    return artifact_power_t();
  }

  std::vector<const artifact_power_data_t*> powers = dbc.artifact_powers( artifact_id );
  const artifact_power_data_t* power_data = nullptr;
  size_t power_index = 0;

  // Find the power by name
  for ( power_index = 0; power_index < powers.size(); ++power_index )
  {
    const artifact_power_data_t* power = powers[ power_index ];
    if ( power -> name == 0 )
    {
      continue;
    }

    std::string power_name = power -> name;
    if ( tokenized )
      util::tokenize( power_name );

    if ( util::str_compare_ci( name, power_name ) )
    {
      power_data = power;
      break;
    }
  }

  // No power found
  if ( !power_data )
  {
    return artifact_power_t();
  }

  auto total_ranks = artifact.points[ power_index ].first + artifact.points[ power_index ].second;

  // User input did not select this power
  if ( total_ranks == 0 )
  {
    return artifact_power_t();
  }

  // Single rank powers can only be set to 0 or 1
  if ( power_data -> max_rank == 1 && total_ranks > 1 )
  {
    return artifact_power_t();
  }

  // 1 rank powers use the zeroth (only) entry, multi-rank spells have 0 -> max rank entries
  std::vector<const artifact_power_rank_t*> ranks = dbc.artifact_power_ranks( power_data -> id );
  auto rank_index = total_ranks - 1;

  // Rank data missing for the power
  if ( rank_index + 1 > as<int>(ranks.size()) )
  {
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s too high rank (%u/%u) given for artifact power %s (iondex %u)",
          this -> name(), artifact.points[ power_index ], ranks.size(),
          power_data -> name ? power_data -> name : "Unknown", power_index );
    }

    return artifact_power_t();
  }

  // Finally, all checks satisfied, return a real spell
  return artifact_power_t( total_ranks,
                           find_spell( ranks[ rank_index ] -> id_spell() ),
                           power_data,
                           ranks[ rank_index ] );
}

// player_t::find_mastery_spell =============================================

const spell_data_t* player_t::find_mastery_spell( specialization_e s, const std::string& token, uint32_t idx ) const
{
  if ( s != SPEC_NONE && s == _spec )
  {
    if ( unsigned spell_id = dbc.mastery_ability_id( s, idx ) )
    {
      const spell_data_t* spell = dbc.spell( spell_id );
      if ( ( int )spell -> level() <= true_level )
      {
        if ( dbc::get_token( spell_id ).empty() )
          dbc.add_token( spell_id, token );

        return dbc::find_spell( this, spell );
      }
    }
  }

  return spell_data_t::not_found();
}

/*
 * Tries to find spell data by name, by going through various spell lists in following order:
 *
 * class spell, specialization spell, mastery spell, talent spell, glyphs spell,
 * racial spell, pet_spell
 */

const spell_data_t* player_t::find_spell( const std::string& name, const std::string& token, specialization_e s ) const
{
  const spell_data_t* sp = find_class_spell( name, token, s );
  assert( sp );
  if ( sp -> ok() ) return sp;

  sp = find_specialization_spell( name, token );
  assert( sp );
  if ( sp -> ok() ) return sp;

  if ( s != SPEC_NONE )
  {
    sp = find_mastery_spell( s, token, 0 );
    assert( sp );
    if ( sp -> ok() ) return sp;
  }

  sp = find_talent_spell( name, token );
  assert( sp );
  if ( sp -> ok() ) return sp;

  sp = find_glyph_spell( name, token );
  assert( sp );
  if ( sp -> ok() ) return sp;

  sp = find_racial_spell( name, token );
  assert( sp );
  if ( sp -> ok() ) return sp;

  sp = find_pet_spell( name, token );
  assert( sp );
  if ( sp -> ok() ) return sp;

  return spell_data_t::not_found();
}

// player_t::find_racial_spell ==============================================

const spell_data_t* player_t::find_racial_spell( const std::string& name, const std::string& token, race_e r ) const
{
  if ( unsigned spell_id = dbc.race_ability_id( type, ( r != RACE_NONE ) ? r : race, name.c_str() ) )
  {
    const spell_data_t* s = dbc.spell( spell_id );
    if ( s -> id() == spell_id )
    {
      if ( dbc::get_token( spell_id ).empty() )
        dbc.add_token( spell_id, token );

      return dbc::find_spell( this, s );
    }
  }

  return spell_data_t::not_found();
}

// player_t::find_class_spell ===============================================

const spell_data_t* player_t::find_class_spell( const std::string& name, const std::string& token, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    if ( unsigned spell_id = dbc.class_ability_id( type, _spec, name.c_str() ) )
    {
      const spell_data_t* spell = dbc.spell( spell_id );
      if ( spell -> id() == spell_id && ( int )spell -> level() <= true_level )
      {
        if ( dbc::get_token( spell_id ).empty() )
          dbc.add_token( spell_id, token );

        return dbc::find_spell( this, spell );
      }
    }
  }

  return spell_data_t::not_found();
}

// player_t::find_pet_spell =================================================

const spell_data_t* player_t::find_pet_spell( const std::string& name, const std::string& token ) const
{
  if ( unsigned spell_id = dbc.pet_ability_id( type, name.c_str() ) )
  {
    const spell_data_t* s = dbc.spell( spell_id );
    if ( s -> id() == spell_id )
    {
      if ( dbc::get_token( spell_id ).empty() )
        dbc.add_token( spell_id, token );
      return dbc::find_spell( this, s );
    }
  }

  return spell_data_t::not_found();
}

// player_t::find_spell =====================================================

const spell_data_t* player_t::find_spell( const unsigned int id, const std::string& token ) const
{
  if ( id )
  {
    const spell_data_t* s = dbc.spell( id );
    if ( s -> id() && ( int )s -> level() <= true_level )
    {
      if ( dbc::get_token( id ).empty() )
        dbc.add_token( id, token );
      return dbc::find_spell( this, s );
    }
  }

  return spell_data_t::not_found();
}

namespace {
expr_t* deprecate_expression( player_t* p, action_t* a, const std::string& old_name, const std::string& new_name )
{
  p -> sim -> errorf( "Use of \"%s\" ( action %s ) in action expressions is deprecated: use \"%s\" instead.\n",
                      old_name.c_str(), a -> name(), new_name.c_str() );

  return p -> create_expression( a, new_name );
}

struct player_expr_t : public expr_t
{
  player_t& player;

  player_expr_t( const std::string& n, player_t& p ) :
    expr_t( n ), player( p ) {}
};

struct position_expr_t : public player_expr_t
{
  int mask;
  position_expr_t( const std::string& n, player_t& p, int m ) :
    player_expr_t( n, p ), mask( m ) {}
  virtual double evaluate() override { return ( 1 << player.position() ) & mask; }
};
}

// player_t::create_expression ==============================================

expr_t* player_t::create_expression( action_t* a,
                                     const std::string& expression_str )
{
  if ( expression_str == "level" )
    return expr_t::create_constant( "level", true_level );
  if ( expression_str == "name" )
    return expr_t::create_constant( "name", actor_index );
  if ( expression_str == "self" )
    return expr_t::create_constant( "self", actor_index );
  if ( expression_str == "multiplier" )
  {
    return make_fn_expr(expression_str, [this, &a]{ return cache.player_multiplier( a -> get_school() ); });
  }
  if ( expression_str == "in_combat" )
    return make_ref_expr( "in_combat", in_combat );
  if ( expression_str == "attack_haste" )
    return make_fn_expr(expression_str, [this]{ return cache.attack_haste();} );
  if ( expression_str == "attack_speed" )
    return make_fn_expr(expression_str, [this]{ return cache.attack_speed();} );
  if ( expression_str == "spell_haste" )
    return make_fn_expr(expression_str, [this]{ return cache.spell_speed(); } );

  if ( expression_str == "desired_targets" )
    return expr_t::create_constant( expression_str, sim -> desired_targets );

  // time_to_pct expressions
  if ( util::str_in_str_ci( expression_str, "time_to_" ) )
  {
    std::vector<std::string> parts = util::string_split( expression_str, "_" );
    double percent;

    if ( util::str_in_str_ci( parts[2], "die" ) )
      percent = 0.0;
    else if ( util::str_in_str_ci( parts[2], "pct" ) )
      percent = static_cast<double>( util::str_to_num<int>( parts[2] ) );
    else
      percent = -1;
    // skip construction if the percent is nonsensical
    if ( percent >= 0.0 )
    {
      struct time_to_percent_t: public expr_t
      {
        double percent;
        player_t* player;
        time_to_percent_t( const std::string& n, player_t* p, double percent ):
          expr_t( n ), percent( percent ), player( p )
        { }

        double evaluate() override
        {
          double time;
          time = player -> time_to_percent( percent ).total_seconds();
          return time;
        }
      };

      return new time_to_percent_t( parts[2], this, percent );
    }
  }

  if ( expression_str == "health_pct" )
    return deprecate_expression( this, a, expression_str, "health.pct" );

  if ( expression_str == "mana_pct" )
    return deprecate_expression( this, a, expression_str, "mana.pct" );

  if ( expression_str == "energy_regen" )
    return deprecate_expression( this, a, expression_str, "energy.regen" );

  if ( expression_str == "focus_regen" )
    return deprecate_expression( this, a, expression_str, "focus.regen" );

  if ( expression_str == "time_to_max_energy" )
    return deprecate_expression( this, a, expression_str, "energy.time_to_max" );

  if ( expression_str == "time_to_max_focus" )
    return deprecate_expression( this, a, expression_str, "focus.time_to_max" );

  if ( expression_str == "max_mana_nonproc" )
    return deprecate_expression( this, a, expression_str, "mana.max_nonproc" );

  if ( expression_str == "ptr" )
    return expr_t::create_constant( "ptr", dbc.ptr );

  if ( expression_str == "position_front" )
    return new position_expr_t( "position_front", *this,
                                ( 1 << POSITION_FRONT ) | ( 1 << POSITION_RANGED_FRONT ) );
  if ( expression_str == "position_back" )
    return new position_expr_t( "position_back", *this,
                                ( 1 << POSITION_BACK ) | ( 1 << POSITION_RANGED_BACK ) );

  if ( expression_str == "mastery_value" )
    return  make_mem_fn_expr( expression_str, this-> cache, &player_stat_cache_t::mastery_value );

  if ( expr_t* q = create_resource_expression( expression_str ) )
    return q;

  // time_to_bloodlust conditional
  if ( expression_str == "time_to_bloodlust" )
  {
    struct time_to_bloodlust_expr_t : public expr_t
    {
      player_t* player;

      time_to_bloodlust_expr_t( player_t* p, const std::string& name ) :
        expr_t( name ), player( p )
      {
      }

      double evaluate() override
      {
        return player -> calculate_time_to_bloodlust();
      }
    };

    return new time_to_bloodlust_expr_t( this, expression_str );
  }

  // T18 Hellfire Citadel class trinket
  if ( expression_str == "t18_class_trinket" )
  {
    return expr_t::create_constant( expression_str, has_t18_class_trinket() );
  }

  // incoming_damage_X expressions
  if ( util::str_in_str_ci( expression_str, "incoming_damage_" ) )
  {
    std::vector<std::string> parts = util::string_split( expression_str, "_" );
    timespan_t window_duration;

    if ( util::str_in_str_ci( parts[ 2 ], "ms" ) )
      window_duration = timespan_t::from_millis( util::str_to_num<int>( parts[ 2 ] ) );
    else
      window_duration = timespan_t::from_seconds( util::str_to_num<int>( parts[ 2 ] ) );

    // skip construction if the duration is nonsensical
    if ( window_duration > timespan_t::zero() )
    {
      struct inc_dmg_expr_t : public expr_t
      {
        player_t* player;
        timespan_t duration;

        inc_dmg_expr_t( player_t* p, const std::string& time_str, const timespan_t& duration ) :
          expr_t( "incoming_damage_" + time_str ), player( p ), duration( duration )
        {
        }

        double evaluate() override
        {
          return player -> compute_incoming_damage( duration );
        }
      };

      return new inc_dmg_expr_t( this, parts[ 2 ], window_duration );
    }
  }

  // Get the actor's raw initial haste percent
  if ( expression_str == "raw_haste_pct" )
  {
    return make_fn_expr( expression_str, [this]() {
      double h = std::max( 0.0, initial.stats.haste_rating ) /
                 initial_rating().spell_haste;

      return h;
    } );
  }

  // everything from here on requires splits
  std::vector<std::string> splits = util::string_split( expression_str, "." );
  // player variables
  if ( splits[ 0 ] == "variable" && splits.size() == 2 )
  {
    struct variable_expr_t : public expr_t
    {
      player_t* player_;
      const action_variable_t* var_;

      variable_expr_t( player_t* p, const std::string& name ) :
        expr_t( "variable" ), player_( p ), var_( 0 )
      {
        for ( auto& elem : player_ -> variables )
        {
          if ( util::str_compare_ci( name, elem -> name_ ) )
          {
            var_ = elem;
            break;
          }
        }
      }

      double evaluate() override
      { return var_ -> current_value_; }
    };

    variable_expr_t* expr = new variable_expr_t( this, splits[ 1 ] );
    if ( ! expr -> var_ )
    {
      sim -> errorf( "Player %s no variable named '%s' found", name(), splits[ 1 ].c_str() );
      delete expr;
    }
    else
      return expr;
  }

  // trinkets
  if ( splits[ 0 ] == "trinket" )
  {
    if ( expr_t* expr = unique_gear::create_expression( a, expression_str ) )
      return expr;
  }

  if ( splits.size() == 2 && splits[ 0 ] == "equipped" )
  {
    unsigned item_id = util::to_unsigned( splits[ 1 ] );
    for ( size_t i = 0; i < items.size(); ++i )
    {
      if ( item_id > 0 && items[ i ].parsed.data.id == item_id )
      {
        return expr_t::create_constant( "item_equipped", 1 );
      }
      else if ( util::str_compare_ci( items[ i ].name_str, splits[ 1 ] ) )
      {
        return expr_t::create_constant( "item_equipped", 1 );
      }
    }

    return expr_t::create_constant( "item_equipped", 0 );
  }

  if ( splits.size() == 2 && ( splits[ 0 ] == "main_hand" || splits[ 0 ] == "off_hand" ) )
  {
    double weapon_status = -1;
    if ( splits[ 0 ] == "main_hand" && util::str_compare_ci( splits[ 1 ], "2h" ) )
    {
      weapon_status = static_cast<double>( main_hand_weapon.group() == WEAPON_2H );
    }
    else if ( splits[ 0 ] == "main_hand" && util::str_compare_ci( splits[ 1 ], "1h" ) )
    {
      weapon_status = static_cast<double>( main_hand_weapon.group() == WEAPON_1H ||
                                           main_hand_weapon.group() == WEAPON_SMALL );
    }
    else if ( splits[ 0 ] == "off_hand" && util::str_compare_ci( splits[ 1 ], "2h" ) )
    {
      weapon_status = static_cast<double>( off_hand_weapon.group() == WEAPON_2H );
    }
    else if ( splits[ 0 ] == "off_hand" && util::str_compare_ci( splits[ 1 ], "1h" ) )
    {
      weapon_status = static_cast<double>( off_hand_weapon.group() == WEAPON_1H ||
                                           off_hand_weapon.group() == WEAPON_SMALL );
    }

    if ( weapon_status > -1 )
    {
      return expr_t::create_constant( "weapon_type_expr", weapon_status );
    }
  }

  if ( splits[ 0 ] == "legendary_ring" )
  {
    if ( expr_t* expr = unique_gear::create_expression( a, expression_str ) )
      return expr;
  }

  // race
  if ( splits[ 0 ] == "race" && splits.size() == 2 )
  {
    struct race_expr_t : public const_expr_t
    {
      race_expr_t( player_t& p, const std::string& race_name ) :
        const_expr_t( "race", p.race_str == race_name )
      {
      }
    };
    return new race_expr_t( *this, splits[ 1 ] );
  }

  // role
  if ( splits[ 0 ] == "role" && splits.size() == 2 )
  {
    struct role_expr_t : public const_expr_t
    {
      role_expr_t( player_t& p, const std::string& role ) :
        const_expr_t( "role", util::str_compare_ci( util::role_type_string( p.primary_role() ), role ) )
      {}
    };
    return new role_expr_t( *this, splits[ 1 ] );
  }

  // pet
  if ( splits[ 0 ] == "pet" )
  {
    struct pet_expr_t : public expr_t
    {
      const pet_t& pet;
      pet_expr_t( const std::string& name, pet_t& p ) :
        expr_t( name ), pet( p ) {}
    };

    pet_t* pet = find_pet( splits[ 1 ] );
    if ( ! pet )
    {
      return expr_t::create_constant( "pet_not_found_expr", -1.0 );
    }

    if ( splits.size() == 2 )
    {
      return expr_t::create_constant( "pet_index_expr", static_cast<double>( pet -> actor_index ) );
    }
    // pet.foo.blah
    else
    {
      if ( splits[ 2 ] == "active" )
      {
        struct pet_active_expr_t : public pet_expr_t
        {
          pet_active_expr_t( pet_t* p ) : pet_expr_t( "pet_active", *p )
          { }

          double evaluate() override
          { return ! pet.is_sleeping(); }
        };

        return new pet_active_expr_t( pet );
      }
      else if ( splits[ 2 ] == "remains" )
      {
        struct pet_remains_expr_t : public pet_expr_t
        {
          pet_remains_expr_t( pet_t* p ) : pet_expr_t( "pet_remains", *p )
          { }

          double evaluate() override
          {
            if ( pet.expiration && pet.expiration -> remains() > timespan_t::zero() )
              return pet.expiration -> remains().total_seconds();
            else
              return 0;
          }
        };
        return new pet_remains_expr_t( pet );
      }
      else
      {
        return pet -> create_expression( a, expression_str.substr( splits[ 1 ].length() + 5 ) );
      }
    }
  }

  // owner
  else if ( splits[ 0 ] == "owner" )
  {
    if ( pet_t* pet = dynamic_cast<pet_t*>( this ) )
    {
      if ( pet -> owner )
        return pet -> owner -> create_expression( a, expression_str.substr( 6 ) );
    }
    // FIXME: report failure.
  }

  // stat
  else if ( splits[ 0 ] == "stat" && splits.size() == 2 )
  {
    if ( util::str_compare_ci( "spell_haste", splits[ 1 ] ) )
    {
      struct spell_haste_expr_t : public player_expr_t
      {
        spell_haste_expr_t( player_t& p ) : player_expr_t( "spell_haste", p ) { }
        double evaluate() override { return 1.0 / player.cache.spell_haste() - 1.0; }
      };
      return new spell_haste_expr_t( *this );
    }

    stat_e stat = util::parse_stat_type( splits[ 1 ] );
    switch ( stat )
    {
      case STAT_STRENGTH:
      case STAT_AGILITY:
      case STAT_STAMINA:
      case STAT_INTELLECT:
      case STAT_SPIRIT:
      {
        struct attr_expr_t : public player_expr_t
        {
          attribute_e attr;
          attr_expr_t( const std::string& name, player_t& p, attribute_e a ) :
            player_expr_t( name, p ), attr( a ) {}
          virtual double evaluate() override
          { return player.cache.get_attribute( attr ); }
        };
        return new attr_expr_t( expression_str, *this, static_cast<attribute_e>( stat ) );
      }

      case STAT_SPELL_POWER:
      {
        struct sp_expr_t : player_expr_t
        {
          sp_expr_t( const std::string& name, player_t& p ) :
            player_expr_t( name, p ) {}
          virtual double evaluate() override
          {
            return player.cache.spell_power( SCHOOL_MAX ) * player.composite_spell_power_multiplier();
          }
        };
        return new sp_expr_t( expression_str, *this );
      }

      case STAT_ATTACK_POWER:
      {
        struct ap_expr_t : player_expr_t
        {
          ap_expr_t( const std::string& name, player_t& p ) :
            player_expr_t( name, p ) {}
          virtual double evaluate() override
          {
            return player.cache.attack_power() * player.composite_attack_power_multiplier();
          }
        };
        return new ap_expr_t( expression_str, *this );
      }

      case STAT_EXPERTISE_RATING: return make_mem_fn_expr( expression_str, *this, &player_t::composite_expertise_rating );
      case STAT_HIT_RATING:       return make_mem_fn_expr( expression_str, *this, &player_t::composite_melee_hit_rating );
      case STAT_CRIT_RATING:      return make_mem_fn_expr( expression_str, *this, &player_t::composite_melee_crit_rating );
      case STAT_HASTE_RATING:     return make_mem_fn_expr( expression_str, *this, &player_t::composite_melee_haste_rating );
      case STAT_ARMOR:            return make_ref_expr( expression_str, current.stats.armor );
      case STAT_BONUS_ARMOR:      return make_ref_expr( expression_str, current.stats.bonus_armor );
      case STAT_DODGE_RATING:     return make_mem_fn_expr( expression_str, *this, &player_t::composite_dodge_rating );
      case STAT_PARRY_RATING:     return make_mem_fn_expr( expression_str, *this, &player_t::composite_parry_rating );
      case STAT_BLOCK_RATING:     return make_mem_fn_expr( expression_str, *this, &player_t::composite_block_rating );
      case STAT_MASTERY_RATING:   return make_mem_fn_expr( expression_str, *this, &player_t::composite_mastery_rating );
      case RATING_DAMAGE_VERSATILITY: return make_mem_fn_expr( expression_str, *this, &player_t::composite_damage_versatility_rating );
      case RATING_HEAL_VERSATILITY: return make_mem_fn_expr( expression_str, *this, &player_t::composite_heal_versatility_rating );
      case RATING_MITIGATION_VERSATILITY: return make_mem_fn_expr( expression_str, *this, &player_t::composite_mitigation_versatility_rating );
      default: break;
    }

    // FIXME: report error and return?
  }

  else if ( splits[ 0 ] == "using_apl" && splits.size() == 2 )
  {
    struct use_apl_expr_t : public expr_t
    {
      bool is_match;
      std::string apl_name;

      use_apl_expr_t( player_t* p, const std::string& apl_str, const std::string& use_apl ) :
        expr_t( "using_apl_" + apl_str ), apl_name( apl_str )
      {
        (void) p;
        is_match = util::str_compare_ci( apl_str, use_apl );
      }

      double evaluate() override
      {
        return is_match;
      }
    };

    return new use_apl_expr_t( this, splits[ 1 ], use_apl );
  }


  else if ( splits.size() == 3 )
  {
    if ( splits[ 0 ] == "buff" || splits[ 0 ] == "debuff" )
    {
      a -> player -> get_target_data( this );
      buff_t* buff = buff_t::find_expressable( buff_list, splits[ 1 ], a -> player );
      if ( ! buff ) buff = buff_t::find( this, splits[ 1 ], this ); // Raid debuffs
      if ( buff ) return buff_t::create_expression( splits[ 1 ], a, splits[ 2 ], buff );
    }
    else if ( splits[ 0 ] == "cooldown" )
    {
      cooldown_t* cooldown = get_cooldown( splits[ 1 ] );
      if ( cooldown )
      {
        return cooldown -> create_expression( a, splits[ 2 ] );
      }
    }
    else if ( splits[ 0 ] == "dot" )
    {
      // FIXME! DoT Expressions should not need to get the dot itself.
      return get_dot( splits[ 1 ], a -> player ) -> create_expression( a, splits[ 2 ], false );
    }
    else if ( splits[ 0 ] == "swing" )
    {
      std::string& s = splits[ 1 ];
      slot_e hand = SLOT_INVALID;
      if ( s == "mh" || s == "mainhand" || s == "main_hand" ) hand = SLOT_MAIN_HAND;
      if ( s == "oh" || s ==  "offhand" || s ==  "off_hand" ) hand = SLOT_OFF_HAND;
      if ( hand == SLOT_INVALID ) return 0;
      if ( splits[ 2 ] == "remains" )
      {
        struct swing_remains_expr_t : public player_expr_t
        {
          slot_e slot;
          swing_remains_expr_t( player_t& p, slot_e s ) :
            player_expr_t( "swing_remains", p ), slot( s ) {}
          virtual double evaluate() override
          {
            attack_t* attack = ( slot == SLOT_MAIN_HAND ) ? player.main_hand_attack : player.off_hand_attack;
            if ( attack && attack -> execute_event ) return attack -> execute_event -> remains().total_seconds();
            return 9999;
          }
        };
        return new swing_remains_expr_t( *this, hand );
      }
    }

    if ( splits[ 0 ] == "spell" && splits[ 2 ] == "exists" )
    {
      struct spell_exists_expr_t : public expr_t
      {
        const std::string name;
        player_t& player;
        spell_exists_expr_t( const std::string& n, player_t& p ) : expr_t( n ), name( n ), player( p ) {}
        virtual double evaluate() override { return player.find_spell( name ) -> ok(); }
      };
      return new spell_exists_expr_t( splits[ 1 ], *this );
    }
  }
  else if ( splits.size() == 2 )
  {
    if ( splits[ 0 ] == "set_bonus" )
      return sets.create_expression( this, splits[ 1 ] );

    if ( splits[ 0 ] == "active_dot" )
    {
      struct active_dot_expr_t : public expr_t
      {
        const player_t& player;
        unsigned id;

        active_dot_expr_t( const player_t& p, unsigned action_id ) :
          expr_t( "active_dot_expr" ), player( p ), id( action_id )
        { }

        double evaluate() override
        { return player.get_active_dots( id ); }
      };

      int internal_id = find_action_id( splits[ 1 ] );
      if ( internal_id > -1 )
        return new active_dot_expr_t( *this, internal_id );
    }

    if ( splits[ 0 ] == "movement" )
    {
      struct raid_movement_expr_t : public expr_t
      {
        player_t* player;

        raid_movement_expr_t( const std::string& name, player_t* p ) :
          expr_t( name ), player( p )
        { }
      };

      if ( splits[ 1 ] == "remains" )
      {
        struct rm_remains_expr_t : public raid_movement_expr_t
        {
          rm_remains_expr_t( const std::string& n, player_t* p ) :
            raid_movement_expr_t( n, p )
          { }

          double evaluate() override
          {
            if ( player -> current.distance_to_move > 0 )
              return ( player -> current.distance_to_move / player -> composite_movement_speed() );
            else
              return player -> buffs.raid_movement -> remains().total_seconds();
          }
        };

        return new rm_remains_expr_t( splits[ 1 ], this );
      }
      else if ( splits[ 1 ] == "distance" )
      {
        struct rm_distance_expr_t : public raid_movement_expr_t
        {
          rm_distance_expr_t( const std::string& n, player_t* p ) :
            raid_movement_expr_t( n, p )
          { }

          double evaluate() override
          { return player -> current.distance_to_move; }
        };

        return new rm_distance_expr_t( splits[ 1 ], this );
      }
      else if ( splits[ 1 ] == "speed" )
        return make_mem_fn_expr( splits[ 1 ], *this, &player_t::composite_movement_speed );
    }
  }

  if ( ( splits.size() == 3 ) && ( ( splits[ 0 ] == "glyph" ) || ( splits[ 0 ] == "talent" ) ) )
  {
    struct s_expr_t : public player_expr_t
    {
      spell_data_t* s;

      s_expr_t( const std::string& name, player_t& p, spell_data_t* sp ) :
        player_expr_t( name, p ), s( sp ) {}
      virtual double evaluate() override
      { return ( s && s -> ok() ); }
    };

    if ( splits[ 2 ] != "enabled" )
    {
      return 0;
    }

    spell_data_t* s;

    if ( splits[ 0 ] == "talent" )
    {
      s = const_cast< spell_data_t* >( find_talent_spell( splits[ 1 ], std::string(), specialization(), true ) );
    }
    else
    {
      s = const_cast< spell_data_t* >( find_glyph_spell( splits[ 1 ] ) );
    }

    if( sim -> optimize_expressions )
      return expr_t::create_constant( expression_str, ( s && s -> ok() ) ? 1.0 : 0.0 );
    else
      return new s_expr_t( expression_str, *this, s );
  }
  else if ( splits.size() == 3 && splits[ 0 ] == "artifact" && ( splits[ 2 ] == "enabled" || splits[ 2 ] == "rank" ) )
  {
    artifact_power_t power = find_artifact_spell( splits[ 1 ], true );

    if ( splits[ 2 ] == "enabled" )
    {
      return expr_t::create_constant( expression_str, power.rank() > 0 ? 1.0 : 0.0 );
    }
    else if ( splits[ 2 ] == "rank" )
    {
      return expr_t::create_constant( expression_str, power.rank() );
    }
  }
  else if ( ( splits.size() == 3 && splits[ 0 ] == "action" ) || splits[ 0 ] == "in_flight" || splits[ 0 ] == "in_flight_to_target" )
  {
    std::vector<action_t*> in_flight_list;
    bool in_flight_singleton = ( splits[ 0 ] == "in_flight" || splits[ 0 ] == "in_flight_to_target" );
    std::string action_name = ( in_flight_singleton ) ? a -> name_str : splits[ 1 ];
    for ( size_t i = 0; i < action_list.size(); ++i )
    {
      action_t* action = action_list[ i ];
      if ( action -> name_str == action_name )
      {
        if ( in_flight_singleton || splits[ 2 ] == "in_flight" || splits[ 2 ] == "in_flight_to_target" )
        {
          in_flight_list.push_back( action );
        }
        else
        {
          return action -> create_expression( splits[ 2 ] );
        }
      }
    }
    if ( ! in_flight_list.empty() )
    {
      if ( splits[ 0 ] == "in_flight" || ( ! in_flight_singleton && splits[ 2 ] == "in_flight" ) )
      {
        struct in_flight_multi_expr_t : public expr_t
        {
          const std::vector<action_t*> action_list;
          in_flight_multi_expr_t( const std::vector<action_t*>& al ) :
            expr_t( "in_flight" ), action_list( al ) {}
          virtual double evaluate() override
          {
            for ( size_t i = 0; i < action_list.size(); i++ )
            {
              if ( action_list[ i ] -> has_travel_events() )
                return true;
            }
            return false;
          }
        };
        return new in_flight_multi_expr_t( in_flight_list );
      }
      else if ( splits[ 0 ] == "in_flight_to_target" || ( ! in_flight_singleton && splits[ 2 ] == "in_flight_to_target" ) )
      {
        struct in_flight_to_target_multi_expr_t : public expr_t
        {
          const std::vector<action_t*> action_list;
          action_t& action;

          in_flight_to_target_multi_expr_t( const std::vector<action_t*>& al, action_t& a ) :
            expr_t( "in_flight_to_target" ), action_list( al ), action( a ) {}
          virtual double evaluate() override
          {
            for ( size_t i = 0; i < action_list.size(); i++ )
            {
              if ( action_list[ i ] -> has_travel_events_for( action.target ) )
                return true;
            }
            return false;
          }
        };
        return new in_flight_to_target_multi_expr_t( in_flight_list, *a );
      }
    }
  }

  return sim -> create_expression( a, expression_str );
}

expr_t* player_t::create_resource_expression( const std::string& name_str )
{
  struct resource_expr_t : public player_expr_t
  {
    resource_e rt;

    resource_expr_t( const std::string& n, player_t& p, resource_e r ) :
      player_expr_t( n, p ), rt( r ) {}
  };

  std::vector<std::string> splits = util::string_split( name_str, "." );
  if ( splits.empty() )
    return 0;

  resource_e r = util::parse_resource_type( splits[ 0 ] );
  if ( r == RESOURCE_NONE )
    return 0;

  if ( splits.size() == 1 )
    return make_ref_expr( name_str, resources.current[ r ] );

  if ( splits.size() == 2 )
  {
    if ( splits[ 1 ] == "deficit" )
    {
      struct resource_deficit_expr_t : public resource_expr_t
      {
        resource_deficit_expr_t( const std::string& n, player_t& p, resource_e r ) :
          resource_expr_t( n, p, r ) {}
        virtual double evaluate() override
        { return player.resources.max[ rt ] - player.resources.current[ rt ]; }
      };
      return new resource_deficit_expr_t( name_str, *this, r );
    }

    else if ( splits[ 1 ] == "pct" || splits[ 1 ] == "percent" )
    {
      if ( r == RESOURCE_HEALTH )
      {
        return make_mem_fn_expr( name_str, *this, &player_t::health_percentage );
      }
      else
      {
        struct resource_pct_expr_t : public resource_expr_t
        {
          resource_pct_expr_t( const std::string& n, player_t& p, resource_e r  ) :
            resource_expr_t( n, p, r ) {}
          virtual double evaluate() override
          { return player.resources.pct( rt ) * 100.0; }
        };
        return new resource_pct_expr_t( name_str, *this, r  );
      }
    }

    else if ( splits[ 1 ] == "max" )
      return make_ref_expr( name_str, resources.max[ r ] );

    else if ( splits[ 1 ] == "max_nonproc" )
      return make_ref_expr( name_str, collected_data.buffed_stats_snapshot.resource[ r ] );

    else if ( splits[ 1 ] == "pct_nonproc" )
    {
      struct resource_pct_nonproc_expr_t : public resource_expr_t
      {
        resource_pct_nonproc_expr_t( const std::string& n, player_t& p, resource_e r ) :
          resource_expr_t( n, p, r ) {}
        virtual double evaluate() override
        { return player.resources.current[ rt ] / player.collected_data.buffed_stats_snapshot.resource[ rt ] * 100.0; }
      };
      return new resource_pct_nonproc_expr_t( name_str, *this, r );
    }
    else if ( splits[ 1 ] == "net_regen" )
    {
      struct resource_net_regen_expr_t : public resource_expr_t
      {
        resource_net_regen_expr_t( const std::string& n, player_t& p, resource_e r ) :
          resource_expr_t( n, p, r ) {}
        virtual double evaluate() override
        {
          timespan_t now = player.sim -> current_time();
          if ( now != timespan_t::zero() )
            return ( player.iteration_resource_gained[ rt ] - player.iteration_resource_lost[ rt ] ) / now.total_seconds();
          else
            return 0;
        }
      };
      return new resource_net_regen_expr_t( name_str, *this, r );
    }
    else if ( splits[ 1 ] == "regen" )
    {
      if ( r == RESOURCE_ENERGY )
        return make_mem_fn_expr( name_str, *this, &player_t::energy_regen_per_second );
      else if ( r == RESOURCE_FOCUS )
        return make_mem_fn_expr( name_str, *this, &player_t::focus_regen_per_second );
      else if ( r == RESOURCE_MANA )
        return make_mem_fn_expr( name_str, *this, &player_t::mana_regen_per_second );
    }

    else if ( splits[1] == "time_to_max" )
    {
      if ( r == RESOURCE_ENERGY )
      {
        struct time_to_max_energy_expr_t : public resource_expr_t
        {
          time_to_max_energy_expr_t( player_t& p, resource_e r ) :
            resource_expr_t( "time_to_max_energy", p, r )
          {
          }
          virtual double evaluate() override
          {
            return ( player.resources.max[RESOURCE_ENERGY] -
              player.resources.current[RESOURCE_ENERGY] ) /
              player.energy_regen_per_second();
          }
        };
        return new time_to_max_energy_expr_t( *this, r );
      }
      else if ( r == RESOURCE_FOCUS )
      {
        struct time_to_max_focus_expr_t : public resource_expr_t
        {
          time_to_max_focus_expr_t( player_t& p, resource_e r ) :
            resource_expr_t( "time_to_max_focus", p, r )
          {
          }
          virtual double evaluate() override
          {
            return ( player.resources.max[RESOURCE_FOCUS] -
              player.resources.current[RESOURCE_FOCUS] ) /
              player.focus_regen_per_second();
          }
        };
        return new time_to_max_focus_expr_t( *this, r );
      }
      else if ( r == RESOURCE_MANA )
      {
        struct time_to_max_mana_expr_t : public resource_expr_t
        {
          time_to_max_mana_expr_t( player_t& p, resource_e r ) :
            resource_expr_t( "time_to_max_mana", p, r )
          {
          }
          virtual double evaluate() override
          {
            return ( player.resources.max[RESOURCE_MANA] -
              player.resources.current[RESOURCE_MANA] ) /
              player.mana_regen_per_second();
          }
        };
        return new time_to_max_mana_expr_t( *this, r );
      }
    }
  }

  return 0;
}

// player_t::compute_incoming_damage =======================================

double player_t::compute_incoming_damage( timespan_t interval )
{
  double amount = 0;

  if ( incoming_damage.size() > 0 )
  {
    std::vector< std::pair< timespan_t, double > >::reverse_iterator i, end;
    for ( i = incoming_damage.rbegin(), end = incoming_damage.rend(); i != end; ++i )
    {
      if ( sim -> current_time() - ( *i ).first > interval )
        break;

      amount += ( *i ).second;
    }
  }

  return amount;
}

// sim_t::calculate_time_to_bloodlust =======================================

double player_t::calculate_time_to_bloodlust()
{
  // only bother if the sim is automatically casting bloodlust. Otherwise error out
  if ( sim -> overrides.bloodlust )
  {
    timespan_t time_to_bl = timespan_t::from_seconds( -1 );
    timespan_t bl_pct_time = timespan_t::from_seconds( -1 );

    // first, check bloodlust_time.  If it's >0, we just compare to current_time()
    // if it's <0, then use time_to_die estimate
    if ( sim -> bloodlust_time > timespan_t::zero() )
      time_to_bl = sim -> bloodlust_time - sim -> current_time();
    else if ( sim -> bloodlust_time < timespan_t::zero() )
      time_to_bl = target -> time_to_percent( 0.0 ) + sim -> bloodlust_time;

    // check bloodlust_percent, if >0 then we need to estimate time based on time_to_die and health_percentage
    if ( sim -> bloodlust_percent > 0 && target -> health_percentage() > 0 )
      bl_pct_time = ( target -> health_percentage() - sim -> bloodlust_percent ) * target -> time_to_percent( 0.0 ) / target -> health_percentage();

    // now that we have both times, we want to check for the Exhaustion buff.  If either time is shorter than
    // the remaining duration on Exhaustion, we won't get that bloodlust and should ignore it
    if ( buffs.exhaustion -> check() )
    {
      if ( time_to_bl < buffs.exhaustion -> remains() )
        time_to_bl = timespan_t::from_seconds( -1 );
      if ( bl_pct_time < buffs.exhaustion -> remains() )
        bl_pct_time = timespan_t::from_seconds( -1 );
    }
    else
    {
      // the sim's bloodlust_check event fires every second, so negative times under 1 second should be treated as zero for safety.
      // without this, time_to_bloodlust can spike to the next target value up to a second before bloodlust is actually cast.
      // probably a non-issue since the worst case is likely to be casting something 1 second too early, but we may as well account for it
      if ( time_to_bl < timespan_t::zero() && - time_to_bl < timespan_t::from_seconds( 1.0 ) )
        time_to_bl = timespan_t::zero();
      if ( bl_pct_time < timespan_t::zero() && - bl_pct_time < timespan_t::from_seconds( 1.0 ) )
        bl_pct_time = timespan_t::zero();
    }

    // if both times are non-negative, take the shortest one since that will happen first
    if ( bl_pct_time >= timespan_t::zero() && time_to_bl >= timespan_t::zero() )
      time_to_bl = std::min( bl_pct_time, time_to_bl );
    // otherwise, at least one is negative, so take the larger of the two
    else
      time_to_bl = std::max( bl_pct_time, time_to_bl );

    // if both are negative, time_to_bl will still be negative and we won't be benefitting from another BL cast.
    // Otherwise we return the positive time until BL is being cast.
    if ( time_to_bl >= timespan_t::zero() )
      return time_to_bl.total_seconds();
  }
  else if ( sim -> debug )
    sim -> errorf( "Trying to call time_to_bloodlust conditional with overrides.bloodlust=0" );

  // Return a nonsensical time that's much longer than the simulation.  This happens if time_to_bl was negative
  // or if overrides.bloodlust was 0
  return 3 * sim -> expected_iteration_time.total_seconds();
}

void player_t::recreate_talent_str( talent_format_e format )
{
  switch ( format )
  {
    case TALENT_FORMAT_UNCHANGED: break;
    case TALENT_FORMAT_ARMORY: create_talents_armory(); break;
    case TALENT_FORMAT_WOWHEAD: create_talents_wowhead(); break;
    default: create_talents_numbers(); break;
  }
}

// player_t::create_profile =================================================

std::string generate_artifact_str( player_t* player )
{
  std::ostringstream s;

  s << "artifact=";
  s << player -> dbc.artifact_by_spec( player -> specialization() );
  s << ":0:0:0:0";

  const auto powers = player -> dbc.artifact_powers( player -> dbc.artifact_by_spec( player -> specialization() ) );
  for ( size_t i = 0; i < player -> artifact.points.size(); ++i )
  {
    if ( player -> artifact.points[ i ].first == 0 )
    {
      continue;
    }

    s << ":" << powers[ i ] -> id << ":" << +player -> artifact.points[ i ].first;
  }
  s << std::endl;

  return s.str();
}

std::string player_t::create_profile( save_e stype )
{
  std::string profile_str;
  std::string term;

  term = "\n";

  if ( ! report_information.comment_str.empty() )
  {
    profile_str += "# " + report_information.comment_str + term;
  }

  if ( stype == SAVE_ALL )
  {
    profile_str += util::player_type_string( type );
    profile_str += "=\"" + name_str + '"' + term;
    if ( ! origin_str.empty() )
      profile_str += "origin=\"" + origin_str + '"' + term;
    if ( ! report_information.thumbnail_url.empty() )
      profile_str += "thumbnail=\"" + report_information.thumbnail_url + '"' + term;
    profile_str += "level=" + util::to_string( true_level ) + term;
    profile_str += "race=" + race_str + term;
    if ( race == RACE_NIGHT_ELF ) {
      profile_str += "timeofday=" + util::to_string( timeofday == player_t::NIGHT_TIME ? "night" : "day" ) + term;
    }
    profile_str += "role=";
    profile_str += util::role_type_string( primary_role() ) + term;
    profile_str += "position=" + position_str + term;

    if ( professions_str.size() > 0 )
    {
      profile_str += "professions=" + professions_str + term;
    }
  }

  if ( stype == SAVE_ALL || stype == SAVE_TALENTS )
  {
    if ( ! talents_str.empty() )
    {
      recreate_talent_str( sim -> talent_format );
      profile_str += "talents=" + talents_str + term;
    }

    if ( talent_overrides_str.size() > 0 )
    {
      std::vector<std::string> splits = util::string_split( talent_overrides_str, "/" );
      for ( size_t i = 0; i < splits.size(); i++ )
      {
        profile_str += "talent_override=" + splits[ i ] + term;
      }
    }

    if ( artifact_overrides_str.size() > 0 )
    {
      std::vector<std::string> splits = util::string_split( artifact_overrides_str, "/" );
      for ( size_t i = 0; i < splits.size(); i++ )
      {
        profile_str += "artifact_override=" + splits[ i ] + term;
      }
    }

    if ( glyphs_str.size() > 0 )
    {
      profile_str += "glyphs=" + glyphs_str + term;
    }

    if ( artifact_str.size() > 0 )
    {
      profile_str += "artifact=" + artifact_str + term;
    }
    else if ( artifact.n_points > 0 )
    {
      profile_str += generate_artifact_str( this );
    }
  }

  if ( stype == SAVE_ALL )
  {
    profile_str += "spec=";
    profile_str += dbc::specialization_string( specialization() ) + term;
  }

  if ( stype == SAVE_ALL || stype == SAVE_ACTIONS )
  {
    if ( !action_list_str.empty() || use_default_action_list )
    {
      // If we created a default action list, add comments
      if ( no_action_list_provided )
        profile_str += action_list_information;

      auto apls = sorted_action_priority_lists( this );
      for ( const auto apl : apls )
      {
        if ( ! apl -> action_list_comment_str.empty() )
        {
          profile_str += term + "# " + apl -> action_list_comment_str;
        }
        profile_str += term;

        bool first = true;
        for ( const auto& action : apl -> action_list )
        {
          if ( ! action.comment_.empty() )
          {
            profile_str += "# " + action.comment_ + term;
          }

          profile_str += "actions";
          if ( ! util::str_compare_ci( apl -> name_str, "default" ) )
          {
            profile_str += "." + apl -> name_str;
          }

          profile_str += first ? "=" : "+=/";
          profile_str += action.action_ + term;

          first = false;
        }
      }
    }
  }

  if ( ( stype == SAVE_ALL || stype == SAVE_GEAR ) && ! items.empty() )
  {
    profile_str += "\n";
    const slot_e SLOT_OUT_ORDER[] =
    {
      SLOT_HEAD, SLOT_NECK, SLOT_SHOULDERS, SLOT_BACK, SLOT_CHEST, SLOT_SHIRT, SLOT_TABARD, SLOT_WRISTS,
      SLOT_HANDS, SLOT_WAIST, SLOT_LEGS, SLOT_FEET, SLOT_FINGER_1, SLOT_FINGER_2, SLOT_TRINKET_1, SLOT_TRINKET_2,
      SLOT_MAIN_HAND, SLOT_OFF_HAND, SLOT_RANGED,
    };

    for (auto & slot : SLOT_OUT_ORDER)
    {
      item_t& item = items[ slot ];
      if ( item.active() )
      {
        profile_str += item.slot_name();
        profile_str += "=" + item.encoded_item() + term;
        if ( sim -> save_gear_comments && ! item.encoded_comment().empty() )
          profile_str += "# " + item.encoded_comment() + term;
      }
    }
    if ( ! items_str.empty() )
    {
      profile_str += "items=" + items_str + term;
    }

    profile_str += "\n# Gear Summary" + term;
    double avg_ilvl = util::round( avg_item_level(), 2 );
    profile_str += "# gear_ilvl=" + util::to_string( avg_ilvl, 2 ) + term;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( i == STAT_NONE || i == STAT_ALL ) continue;

      if ( gear.get_stat( i ) < 0 && total_gear.get_stat( i ) > 0 )
      {
        profile_str += "# gear_";
        profile_str += util::stat_type_string( i );
        profile_str += "=" + util::to_string( total_gear.get_stat( i ), 0 ) + term;
      }
      else if ( gear.get_stat( i ) >= 0 )
      {
        profile_str += "# gear_";
        profile_str += util::stat_type_string( i );
        profile_str += "=" + util::to_string( gear.get_stat( i ), 0 ) + term;
      }
    }
    if ( meta_gem != META_GEM_NONE )
    {
      profile_str += "# meta_gem=";
      profile_str += util::meta_gem_type_string( meta_gem );
      profile_str += term;
    }

    // Set Bonus
    profile_str += sets.to_profile_string( term );

    if ( enchant.attribute[ ATTR_STRENGTH  ] != 0 )  profile_str += "enchant_strength="
         + util::to_string( enchant.attribute[ ATTR_STRENGTH  ] ) + term;

    if ( enchant.attribute[ ATTR_AGILITY   ] != 0 )  profile_str += "enchant_agility="
         + util::to_string( enchant.attribute[ ATTR_AGILITY   ] ) + term;

    if ( enchant.attribute[ ATTR_STAMINA   ] != 0 )  profile_str += "enchant_stamina="
         + util::to_string( enchant.attribute[ ATTR_STAMINA   ] ) + term;

    if ( enchant.attribute[ ATTR_INTELLECT ] != 0 )  profile_str += "enchant_intellect="
         + util::to_string( enchant.attribute[ ATTR_INTELLECT ] ) + term;

    if ( enchant.attribute[ ATTR_SPIRIT    ] != 0 )  profile_str += "enchant_spirit="
         + util::to_string( enchant.attribute[ ATTR_SPIRIT    ] ) + term;

    if ( enchant.spell_power                 != 0 )  profile_str += "enchant_spell_power="
         + util::to_string( enchant.spell_power ) + term;

    if ( enchant.attack_power                != 0 )  profile_str += "enchant_attack_power="
         + util::to_string( enchant.attack_power ) + term;

    if ( enchant.expertise_rating            != 0 )  profile_str += "enchant_expertise_rating="
         + util::to_string( enchant.expertise_rating ) + term;

    if ( enchant.armor                       != 0 )  profile_str += "enchant_armor="
         + util::to_string( enchant.armor ) + term;

    if ( enchant.haste_rating                != 0 )  profile_str += "enchant_haste_rating="
         + util::to_string( enchant.haste_rating ) + term;

    if ( enchant.hit_rating                  != 0 )  profile_str += "enchant_hit_rating="
         + util::to_string( enchant.hit_rating ) + term;

    if ( enchant.crit_rating                 != 0 )  profile_str += "enchant_crit_rating="
         + util::to_string( enchant.crit_rating ) + term;

    if ( enchant.mastery_rating              != 0 )  profile_str += "enchant_mastery_rating="
         + util::to_string( enchant.mastery_rating ) + term;

    if ( enchant.versatility_rating            != 0 )  profile_str += "enchant_versatility_rating="
         + util::to_string( enchant.versatility_rating ) + term;

    if ( enchant.resource[ RESOURCE_HEALTH ] != 0 )  profile_str += "enchant_health="
         + util::to_string( enchant.resource[ RESOURCE_HEALTH ] ) + term;

    if ( enchant.resource[ RESOURCE_MANA   ] != 0 )  profile_str += "enchant_mana="
         + util::to_string( enchant.resource[ RESOURCE_MANA   ] ) + term;

    if ( enchant.resource[ RESOURCE_RAGE   ] != 0 )  profile_str += "enchant_rage="
         + util::to_string( enchant.resource[ RESOURCE_RAGE   ] ) + term;

    if ( enchant.resource[ RESOURCE_ENERGY ] != 0 )  profile_str += "enchant_energy="
         + util::to_string( enchant.resource[ RESOURCE_ENERGY ] ) + term;

    if ( enchant.resource[ RESOURCE_FOCUS  ] != 0 )  profile_str += "enchant_focus="
         + util::to_string( enchant.resource[ RESOURCE_FOCUS  ] ) + term;

    if ( enchant.resource[ RESOURCE_RUNIC_POWER  ] != 0 )  profile_str += "enchant_runic="
         + util::to_string( enchant.resource[ RESOURCE_RUNIC_POWER  ] ) + term;
  }
  return profile_str;
}


// player_t::copy_from ======================================================

void player_t::copy_from( player_t* source )
{
  origin_str = source -> origin_str;
  true_level = source -> true_level;
  race_str = source -> race_str;
  timeofday = source -> timeofday;
  race = source -> race;
  role = source -> role;
  _spec = source -> _spec;
  position_str = source -> position_str;
  professions_str = source -> professions_str;
  source -> recreate_talent_str( TALENT_FORMAT_UNCHANGED );
  parse_talent_url( sim, "talents", source -> talents_str );
  if ( ! source -> artifact_str.empty() )
  {
    parse_artifact( sim, "artifact", source -> artifact_str );
  }
  else
  {
    artifact = source -> artifact;
  }
  talent_overrides_str = source -> talent_overrides_str;
  artifact_overrides_str = source -> artifact_overrides_str;
  glyphs_str = source -> glyphs_str;
  action_list_str = source -> action_list_str;
  alist_map = source -> alist_map;
  use_apl = source -> use_apl;

  meta_gem = source -> meta_gem;
  for ( size_t i = 0; i < items.size(); i++ )
  {
    items[ i ] = source -> items[ i ];
    items[ i ].player = this;
  }

  sets = source -> sets;
  sets.actor = this;
  gear = source -> gear;
  enchant = source -> enchant;
  bugs = source -> bugs;
}


// class_modules::create::options ===========================================

void player_t::create_options()
{
  options.reserve(180);
    add_option( opt_string( "name", name_str ) );
    add_option( opt_func( "origin", parse_origin ) );
    add_option( opt_string( "region", region_str ) );
    add_option( opt_string( "server", server_str ) );
    add_option( opt_string( "thumbnail", report_information.thumbnail_url ) );
    add_option( opt_string( "id", id_str ) );
    add_option( opt_func( "talents", parse_talent_url ) );
    add_option( opt_func( "talent_override", parse_talent_override ) );
    add_option( opt_func( "artifact", parse_artifact ) );
    add_option( opt_func( "artifact_override", parse_artifact_override ) );
    add_option( opt_string( "glyphs", glyphs_str ) );
    add_option( opt_string( "race", race_str ) );
    add_option( opt_func( "timeofday", parse_timeofday ) );
    add_option( opt_int( "level", true_level, 0, MAX_LEVEL ) );
    add_option( opt_bool( "ready_trigger", ready_type ) );
    add_option( opt_func( "role", parse_role_string ) );
    add_option( opt_string( "target", target_str ) );
    add_option( opt_float( "skill", base.skill, 0, 1.0 ) );
    add_option( opt_float( "distance", base.distance, 0, std::numeric_limits<double>::max() ) );
    add_option( opt_string( "position", position_str ) );
    add_option( opt_string( "professions", professions_str ) );
    add_option( opt_string( "actions", action_list_str ) );
    add_option( opt_append( "actions+", action_list_str ) );
    add_option( opt_map( "actions.", alist_map ) );
    add_option( opt_string( "action_list", choose_action_list ) );
    add_option( opt_bool( "sleeping", initial.sleeping ) );
    add_option( opt_bool( "quiet", quiet ) );
    add_option( opt_string( "save", report_information.save_str ) );
    add_option( opt_string( "save_gear", report_information.save_gear_str ) );
    add_option( opt_string( "save_talents", report_information.save_talents_str ) );
    add_option( opt_string( "save_actions", report_information.save_actions_str ) );
    add_option( opt_string( "comment", report_information.comment_str ) );
    add_option( opt_bool( "bugs", bugs ) );
    add_option( opt_func( "world_lag", parse_world_lag ) );
    add_option( opt_func( "world_lag_stddev", parse_world_lag_stddev ) );
    add_option( opt_func( "brain_lag", parse_brain_lag ) );
    add_option( opt_func( "brain_lag_stddev", parse_brain_lag_stddev ) );
    add_option( opt_timespan( "cooldown_tolerance", cooldown_tolerance_ ) );
    add_option( opt_bool( "scale_player", scale_player ) );
    add_option( opt_string( "tmi_output", tmi_debug_file_str ) );
    add_option( opt_float( "tmi_window", tmi_window, 0, std::numeric_limits<double>::max() ) );
    add_option( opt_func( "spec", parse_specialization ) );
    add_option( opt_func( "specialization", parse_specialization ) );
    add_option( opt_func( "stat_timelines", parse_stat_timelines ) );
    add_option( opt_bool( "disable_hotfixes", disable_hotfixes ) );
    add_option( opt_func( "min_gcd", parse_min_gcd ) );

    // Positioning
    add_option( opt_float( "x_pos", default_x_position ) );
    add_option( opt_float( "y_pos", default_y_position ) );

    // Items
    add_option( opt_string( "meta_gem",  meta_gem_str ) );
    add_option( opt_string( "items",     items_str ) );
    add_option( opt_append( "items+",    items_str ) );
    add_option( opt_string( "head",      items[ SLOT_HEAD      ].options_str ) );
    add_option( opt_string( "neck",      items[ SLOT_NECK      ].options_str ) );
    add_option( opt_string( "shoulders", items[ SLOT_SHOULDERS ].options_str ) );
    add_option( opt_string( "shoulder",  items[ SLOT_SHOULDERS ].options_str ) );
    add_option( opt_string( "shirt",     items[ SLOT_SHIRT     ].options_str ) );
    add_option( opt_string( "chest",     items[ SLOT_CHEST     ].options_str ) );
    add_option( opt_string( "waist",     items[ SLOT_WAIST     ].options_str ) );
    add_option( opt_string( "legs",      items[ SLOT_LEGS      ].options_str ) );
    add_option( opt_string( "leg",       items[ SLOT_LEGS      ].options_str ) );
    add_option( opt_string( "feet",      items[ SLOT_FEET      ].options_str ) );
    add_option( opt_string( "foot",      items[ SLOT_FEET      ].options_str ) );
    add_option( opt_string( "wrists",    items[ SLOT_WRISTS    ].options_str ) );
    add_option( opt_string( "wrist",     items[ SLOT_WRISTS    ].options_str ) );
    add_option( opt_string( "hands",     items[ SLOT_HANDS     ].options_str ) );
    add_option( opt_string( "hand",      items[ SLOT_HANDS     ].options_str ) );
    add_option( opt_string( "finger1",   items[ SLOT_FINGER_1  ].options_str ) );
    add_option( opt_string( "finger2",   items[ SLOT_FINGER_2  ].options_str ) );
    add_option( opt_string( "ring1",     items[ SLOT_FINGER_1  ].options_str ) );
    add_option( opt_string( "ring2",     items[ SLOT_FINGER_2  ].options_str ) );
    add_option( opt_string( "trinket1",  items[ SLOT_TRINKET_1 ].options_str ) );
    add_option( opt_string( "trinket2",  items[ SLOT_TRINKET_2 ].options_str ) );
    add_option( opt_string( "back",      items[ SLOT_BACK      ].options_str ) );
    add_option( opt_string( "main_hand", items[ SLOT_MAIN_HAND ].options_str ) );
    add_option( opt_string( "off_hand",  items[ SLOT_OFF_HAND  ].options_str ) );
    add_option( opt_string( "tabard",    items[ SLOT_TABARD    ].options_str ) );

    // Set Bonus
    add_option( opt_func( "set_bonus",         parse_set_bonus                ) );

    // Gear Stats
    add_option( opt_float( "gear_strength",         gear.attribute[ ATTR_STRENGTH  ] ) );
    add_option( opt_float( "gear_agility",          gear.attribute[ ATTR_AGILITY   ] ) );
    add_option( opt_float( "gear_stamina",          gear.attribute[ ATTR_STAMINA   ] ) );
    add_option( opt_float( "gear_intellect",        gear.attribute[ ATTR_INTELLECT ] ) );
    add_option( opt_float( "gear_spirit",           gear.attribute[ ATTR_SPIRIT    ] ) );
    add_option( opt_float( "gear_spell_power",      gear.spell_power ) );
    add_option( opt_float( "gear_attack_power",     gear.attack_power ) );
    add_option( opt_float( "gear_expertise_rating", gear.expertise_rating ) );
    add_option( opt_float( "gear_haste_rating",     gear.haste_rating ) );
    add_option( opt_float( "gear_hit_rating",       gear.hit_rating ) );
    add_option( opt_float( "gear_crit_rating",      gear.crit_rating ) );
    add_option( opt_float( "gear_parry_rating",     gear.parry_rating ) );
    add_option( opt_float( "gear_dodge_rating",     gear.dodge_rating ) );
    add_option( opt_float( "gear_health",           gear.resource[ RESOURCE_HEALTH ] ) );
    add_option( opt_float( "gear_mana",             gear.resource[ RESOURCE_MANA   ] ) );
    add_option( opt_float( "gear_rage",             gear.resource[ RESOURCE_RAGE   ] ) );
    add_option( opt_float( "gear_energy",           gear.resource[ RESOURCE_ENERGY ] ) );
    add_option( opt_float( "gear_focus",            gear.resource[ RESOURCE_FOCUS  ] ) );
    add_option( opt_float( "gear_runic",            gear.resource[ RESOURCE_RUNIC_POWER  ] ) );
    add_option( opt_float( "gear_armor",            gear.armor ) );
    add_option( opt_float( "gear_mastery_rating",   gear.mastery_rating ) );
    add_option( opt_float( "gear_versatility_rating", gear.versatility_rating ) );
    add_option( opt_float( "gear_bonus_armor",      gear.bonus_armor ) );
    add_option( opt_float( "gear_leech_rating",     gear.leech_rating ) );
    add_option( opt_float( "gear_run_speed_rating", gear.speed_rating ) );

    // Stat Enchants
    add_option( opt_float( "enchant_strength",         enchant.attribute[ ATTR_STRENGTH  ] ) );
    add_option( opt_float( "enchant_agility",          enchant.attribute[ ATTR_AGILITY   ] ) );
    add_option( opt_float( "enchant_stamina",          enchant.attribute[ ATTR_STAMINA   ] ) );
    add_option( opt_float( "enchant_intellect",        enchant.attribute[ ATTR_INTELLECT ] ) );
    add_option( opt_float( "enchant_spirit",           enchant.attribute[ ATTR_SPIRIT    ] ) );
    add_option( opt_float( "enchant_spell_power",      enchant.spell_power ) );
    add_option( opt_float( "enchant_attack_power",     enchant.attack_power ) );
    add_option( opt_float( "enchant_expertise_rating", enchant.expertise_rating ) );
    add_option( opt_float( "enchant_armor",            enchant.armor ) );
    add_option( opt_float( "enchant_haste_rating",     enchant.haste_rating ) );
    add_option( opt_float( "enchant_hit_rating",       enchant.hit_rating ) );
    add_option( opt_float( "enchant_crit_rating",      enchant.crit_rating ) );
    add_option( opt_float( "enchant_mastery_rating",   enchant.mastery_rating ) );
    add_option( opt_float( "enchant_versatility_rating", enchant.versatility_rating ) );
    add_option( opt_float( "enchant_bonus_armor",      enchant.bonus_armor ) );
    add_option( opt_float( "enchant_leech_rating",     enchant.leech_rating ) );
    add_option( opt_float( "enchant_run_speed_rating", enchant.speed_rating ) );
    add_option( opt_float( "enchant_health",           enchant.resource[ RESOURCE_HEALTH ] ) );
    add_option( opt_float( "enchant_mana",             enchant.resource[ RESOURCE_MANA   ] ) );
    add_option( opt_float( "enchant_rage",             enchant.resource[ RESOURCE_RAGE   ] ) );
    add_option( opt_float( "enchant_energy",           enchant.resource[ RESOURCE_ENERGY ] ) );
    add_option( opt_float( "enchant_focus",            enchant.resource[ RESOURCE_FOCUS  ] ) );
    add_option( opt_float( "enchant_runic",            enchant.resource[ RESOURCE_RUNIC_POWER  ] ) );

    // Regen
    add_option( opt_bool( "infinite_energy", resources.infinite_resource[ RESOURCE_ENERGY ] ) );
    add_option( opt_bool( "infinite_focus",  resources.infinite_resource[ RESOURCE_FOCUS  ] ) );
    add_option( opt_bool( "infinite_health", resources.infinite_resource[ RESOURCE_HEALTH ] ) );
    add_option( opt_bool( "infinite_mana",   resources.infinite_resource[ RESOURCE_MANA   ] ) );
    add_option( opt_bool( "infinite_rage",   resources.infinite_resource[ RESOURCE_RAGE   ] ) );
    add_option( opt_bool( "infinite_runic",  resources.infinite_resource[ RESOURCE_RUNIC_POWER  ] ) );
    add_option( opt_bool( "infinite_astral_power", resources.infinite_resource[ RESOURCE_ASTRAL_POWER ] ) );

    // Misc
    add_option( opt_string( "skip_actions", action_list_skip ) );
    add_option( opt_string( "modify_action", modify_action ) );
    add_option( opt_string( "use_apl", use_apl ) );
    add_option( opt_timespan( "reaction_time_mean", reaction_mean ) );
    add_option( opt_timespan( "reaction_time_stddev", reaction_stddev ) );
    add_option( opt_timespan( "reaction_time_nu", reaction_nu ) );
    add_option( opt_timespan( "reaction_time_offset", reaction_offset ) );
    add_option( opt_bool( "stat_cache", cache.active ) );
    add_option( opt_bool( "karazhan_trinkets_paired", karazhan_trinkets_paired ) );
}

// player_t::create =========================================================

player_t* player_t::create( sim_t*,
                            const player_description_t& )
{
  return 0;
}

namespace { // UNNAMED NAMESPACE

void player_convergence( int convergence_scale,
                         double confidence_estimator,
                         extended_sample_data_t& dps,
                         std::vector<double>& dps_convergence_error,
                         double dps_error,
                         double& dps_convergence )
{
  // Error Convergence ======================================================

  int    convergence_iterations = 0;
  double convergence_std_dev = 0;

  if ( dps.data().size() > 1 && convergence_scale > 1 && !dps.simple )
  {
    double convergence_dps = 0;
    double convergence_min = +1.0E+50;
    double convergence_max = -1.0E+50;
    for ( unsigned int i = 0; i < dps.data().size(); i += convergence_scale )
    {
      double i_dps = dps.data()[ i ];
      convergence_dps += i_dps;
      if ( convergence_min > i_dps ) convergence_min = i_dps;
      if ( convergence_max < i_dps ) convergence_max = i_dps;
    }
    convergence_iterations = ( as<int>( dps.data().size() ) + convergence_scale - 1 ) / convergence_scale;
    convergence_dps /= convergence_iterations;

    dps_convergence_error.assign( dps.data().size(), 0 );

    double sum_of_squares = 0;

    for ( unsigned int i = 0; i < dps.data().size(); i++ )
    {
      double delta = dps.data()[ i ] - convergence_dps;
      double delta_squared = delta * delta;

      sum_of_squares += delta_squared;

      if ( i > 1 )
        dps_convergence_error[ i ] = confidence_estimator * sqrt( sum_of_squares / i ) / sqrt( ( float ) i );

      if ( ( i % convergence_scale ) == 0 )
        convergence_std_dev += delta_squared;
    }
  }

  if ( convergence_iterations > 1 ) convergence_std_dev /= convergence_iterations;
  convergence_std_dev = sqrt( convergence_std_dev );
  double convergence_error = confidence_estimator * convergence_std_dev;
  if ( convergence_iterations > 1 ) convergence_error /= sqrt( ( float ) convergence_iterations );

  if ( convergence_error > 0 )
    dps_convergence = convergence_error / ( dps_error * convergence_scale );
}

} // UNNAMED NAMESPACE

/*
 * Analyze statistical data of a player
 */

void player_t::analyze( sim_t& s )
{
  assert( s.iterations > 0 );

  pre_analyze_hook();

  // Sample Data Analysis ===================================================

  // sample_data_t::analyze(calc_basics,calc_variance,sort )

  collected_data.analyze( *this );

  for ( size_t i = 0; i < buff_list.size(); i++ )
    buff_list[ i ] -> analyze();

  range::sort( stats_list, []( const stats_t* l, const stats_t* r ) { return l -> name_str < r -> name_str; } );

  if (  quiet ) return;
  if (  collected_data.fight_length.mean() == 0 ) return;

  range::for_each( sample_data_list, std::mem_fn(&luxurious_sample_data_t::analyze ) );

  // Pet Chart Adjustment ===================================================
  size_t max_buckets = static_cast<size_t>(  collected_data.fight_length.max() );

  // Make the pet graphs the same length as owner's
  if (  is_pet() )
  {
    player_t* o =  cast_pet() -> owner;
    max_buckets = static_cast<size_t>( o -> collected_data.fight_length.max() );
  }

  // Stats Analysis =========================================================
  std::vector<stats_t*> tmp_stats_list( stats_list.begin(), stats_list.end() );

  for ( size_t i = 0; i <  pet_list.size(); ++i )
  {
    pet_t* pet =  pet_list[ i ];
    // Append pet -> stats_list to stats_list
    tmp_stats_list.insert( tmp_stats_list.end(), pet -> stats_list.begin(), pet -> stats_list.end() );
  }

  size_t num_stats = tmp_stats_list.size();

  if ( !  is_pet() )
  {
    for ( size_t i = 0; i < num_stats; i++ )
    {
      stats_t* stats = tmp_stats_list[ i ];
      stats -> analyze();

      if ( stats -> type == STATS_DMG )
        stats -> portion_amount =  collected_data.compound_dmg.mean() ? stats -> actual_amount.mean() / collected_data.compound_dmg.mean() : 0.0 ;
      else if ( stats -> type == STATS_HEAL || stats -> type == STATS_ABSORB )
      {
        stats -> portion_amount =  collected_data.compound_heal.mean() ? stats -> actual_amount.mean() : collected_data.compound_absorb.mean() ? stats -> actual_amount.mean() : 0.0;
        double total_heal_and_absorb = collected_data.compound_heal.mean() + collected_data.compound_absorb.mean();
        if ( total_heal_and_absorb )
        {
          stats -> portion_amount /= total_heal_and_absorb;
        }
      }
    }
  }

  // Actor Lists ============================================================
  if (  !  quiet && !  is_enemy() && !  is_add() && ! (  is_pet() && s.report_pets_separately ) )
  {
    s.players_by_dps.push_back( this );
    s.players_by_priority_dps.push_back( this );
    s.players_by_hps.push_back( this );
    s.players_by_hps_plus_aps.push_back( this );
    s.players_by_dtps.push_back( this );
    s.players_by_tmi.push_back( this );
    s.players_by_name.push_back( this );
    s.players_by_apm.push_back( this );
    s.players_by_variance.push_back( this );
  }
  if ( !  quiet && (  is_enemy() ||  is_add() ) && ! (  is_pet() && s.report_pets_separately ) )
    s.targets_by_name.push_back( this );

  // Resources & Gains ======================================================

  double rl = collected_data.resource_lost[  primary_resource() ].mean();

  dpr = ( rl > 0 ) ? (  collected_data.dmg.mean() / rl ) : -1.0;
  hpr = ( rl > 0 ) ? (  collected_data.heal.mean() / rl ) : -1.0;

  rps_loss = rl /  collected_data.fight_length.mean();
  rps_gain = rl /  collected_data.fight_length.mean();

  for ( size_t i = 0; i < gain_list.size(); ++i )
    gain_list[ i ] -> analyze( s );

  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_t* pet =  pet_list[ i ];
    for ( size_t j = 0; j < pet -> gain_list.size(); ++j )
      pet -> gain_list[ j ] -> analyze( s );
  }

  // Damage Timelines =======================================================

  collected_data.timeline_dmg.init( max_buckets );
  for ( size_t i = 0, is_hps = ( primary_role() == ROLE_HEAL ); i < num_stats; i++ )
  {
    stats_t* stats = tmp_stats_list[ i ];
    if ( ( stats -> type != STATS_DMG ) == is_hps )
    {
      size_t j_max = std::min( max_buckets, stats -> timeline_amount.data().size() );
      for ( size_t j = 0; j < j_max; j++ )
        collected_data.timeline_dmg.add( j, stats -> timeline_amount.data()[ j ] );
    }
  }

  recreate_talent_str( s.talent_format );

  // Error Convergence ======================================================
  player_convergence( s.convergence_scale, s.confidence_estimator,
                      collected_data.dps,  dps_convergence_error,  sim_t::distribution_mean_error( s, collected_data.dps ),  dps_convergence );
}

// Return sample_data reference over which this player gets scaled ( scale factors, reforge plots, etc. )
// By default this will be his personal dps or hps

scaling_metric_data_t player_t::scaling_for_metric( scale_metric_e metric ) const
{
  const player_t* q = nullptr;
  if ( ! sim -> scaling -> scale_over_player.empty() )
    q = sim -> find_player( sim -> scaling -> scale_over_player );
  if ( !q )
    q = this;

  switch ( metric )
  {
    case SCALE_METRIC_DPS:        return scaling_metric_data_t( metric, q -> collected_data.dps );
    case SCALE_METRIC_DPSE:       return scaling_metric_data_t( metric, q -> collected_data.dpse );
    case SCALE_METRIC_HPS:        return scaling_metric_data_t( metric, q -> collected_data.hps );
    case SCALE_METRIC_HPSE:       return scaling_metric_data_t( metric, q -> collected_data.hpse );
    case SCALE_METRIC_APS:        return scaling_metric_data_t( metric, q -> collected_data.aps );
    case SCALE_METRIC_DPSP:       return scaling_metric_data_t( metric, q -> collected_data.prioritydps );
    case SCALE_METRIC_HAPS:
      {
        double mean = q -> collected_data.hps.mean() + q -> collected_data.aps.mean();
        double stddev = sqrt( q -> collected_data.hps.mean_variance + q -> collected_data.aps.mean_variance );
        return scaling_metric_data_t( metric, "Healing + Absorb per second", mean, stddev );
      }
    case SCALE_METRIC_DTPS:       return scaling_metric_data_t( metric, q -> collected_data.dtps );
    case SCALE_METRIC_DMG_TAKEN:  return scaling_metric_data_t( metric, q -> collected_data.dmg_taken );
    case SCALE_METRIC_HTPS:       return scaling_metric_data_t( metric, q -> collected_data.htps );
    case SCALE_METRIC_TMI:        return scaling_metric_data_t( metric, q -> collected_data.theck_meloree_index );
    case SCALE_METRIC_ETMI:       return scaling_metric_data_t( metric, q -> collected_data.effective_theck_meloree_index );
    case SCALE_METRIC_DEATHS:     return scaling_metric_data_t( metric, q -> collected_data.deaths );
    default:
      if ( q -> primary_role() == ROLE_TANK )
        return scaling_metric_data_t( SCALE_METRIC_DTPS, q -> collected_data.dtps );
      else if ( q -> primary_role() == ROLE_HEAL )
        return scaling_for_metric( SCALE_METRIC_HAPS );
      else
       return scaling_metric_data_t( SCALE_METRIC_DPS, q -> collected_data.dps );
  }
}

// Change the player position ( fron/back, etc. ) and update attack hit table

void player_t::change_position( position_e new_pos )
{
  if ( sim -> debug )
    sim -> out_debug.printf( "%s changes position from %s to %s.", name(), util::position_type_string( position() ), util::position_type_string( new_pos ) );

  current.position = new_pos;
}

/* Invalidate ALL stats
 */
void player_stat_cache_t::invalidate_all()
{
  if ( ! active ) return;

  range::fill( valid, false );
  range::fill( spell_power_valid, false );
  range::fill( player_mult_valid, false );
  range::fill( player_heal_mult_valid, false );
}

/* Invalidate ALL stats
 */
void player_stat_cache_t::invalidate( cache_e c )
{
  switch ( c )
  {
    case CACHE_SPELL_POWER:
      range::fill( spell_power_valid, false );
      break;
    case CACHE_PLAYER_DAMAGE_MULTIPLIER:
      range::fill( player_mult_valid, false );
      break;
    case CACHE_PLAYER_HEAL_MULTIPLIER:
      range::fill( player_heal_mult_valid, false );
      break;
    default:
      valid[ c ] = false;
      break;
  }
}

/* Helper function to access attribute cache functions by attribute-enumeration
 */
double player_stat_cache_t::get_attribute( attribute_e a ) const
{
  switch ( a )
  {
    case ATTR_STRENGTH: return strength();
    case ATTR_AGILITY: return agility();
    case ATTR_STAMINA: return stamina();
    case ATTR_INTELLECT: return intellect();
    case ATTR_SPIRIT: return spirit();
    default: assert( false ); break;
  }
  return 0.0;
}

#if defined(SC_USE_STAT_CACHE)
// player_stat_cache_t::strength ============================================

double player_stat_cache_t::strength() const
{
  if ( ! active || ! valid[ CACHE_STRENGTH ] )
  {
    valid[ CACHE_STRENGTH ] = true;
    _strength = player -> strength();
  }
  else assert( _strength == player -> strength() );
  return _strength;
}

// player_stat_cache_t::agiity ==============================================

double player_stat_cache_t::agility() const
{
  if ( ! active || ! valid[ CACHE_AGILITY ] )
  {
    valid[ CACHE_AGILITY ] = true;
    _agility = player -> agility();
  }
  else assert( _agility == player -> agility() );
  return _agility;
}

// player_stat_cache_t::stamina =============================================

double player_stat_cache_t::stamina() const
{
  if ( ! active || ! valid[ CACHE_STAMINA ] )
  {
    valid[ CACHE_STAMINA ] = true;
    _stamina = player -> stamina();
  }
  else assert( _stamina == player -> stamina() );
  return _stamina;
}

// player_stat_cache_t::intellect ===========================================

double player_stat_cache_t::intellect() const
{
  if ( ! active || ! valid[ CACHE_INTELLECT ] )
  {
    valid[ CACHE_INTELLECT ] = true;
    _intellect = player -> intellect();
  }
  else assert( _intellect == player -> intellect() );
  return _intellect;
}

// player_stat_cache_t::spirit ==============================================

double player_stat_cache_t::spirit() const
{
  if ( ! active || ! valid[ CACHE_SPIRIT ] )
  {
    valid[ CACHE_SPIRIT ] = true;
    _spirit = player -> spirit();
  }
  else assert( _spirit == player -> spirit() );
  return _spirit;
}

// player_stat_cache_t::spell_power =========================================

double player_stat_cache_t::spell_power( school_e s ) const
{
  if ( ! active || ! spell_power_valid[ s ] )
  {
    spell_power_valid[ s ] = true;
    _spell_power[ s ] = player -> composite_spell_power( s );
  }
  else assert( _spell_power[ s ] == player -> composite_spell_power( s ) );
  return _spell_power[ s ];
}

// player_stat_cache_t::attack_power ========================================

double player_stat_cache_t::attack_power() const
{
  if ( ! active || ! valid[ CACHE_ATTACK_POWER ] )
  {
    valid[ CACHE_ATTACK_POWER ] = true;
    _attack_power = player -> composite_melee_attack_power();
  }
  else assert( _attack_power == player -> composite_melee_attack_power() );
  return _attack_power;
}

// player_stat_cache_t::attack_expertise ====================================

double player_stat_cache_t::attack_expertise() const
{
  if ( ! active || ! valid[ CACHE_ATTACK_EXP ] )
  {
    valid[ CACHE_ATTACK_EXP ] = true;
    _attack_expertise = player -> composite_melee_expertise();
  }
  else assert( _attack_expertise == player -> composite_melee_expertise() );
  return _attack_expertise;
}

// player_stat_cache_t::attack_hit ==========================================

double player_stat_cache_t::attack_hit() const
{
  if ( ! active || ! valid[ CACHE_ATTACK_HIT ] )
  {
    valid[ CACHE_ATTACK_HIT ] = true;
    _attack_hit = player -> composite_melee_hit();
  }
  else
  {
    if ( _attack_hit != player -> composite_melee_hit() )
    {
      assert( false );
    }
    // assert( _attack_hit == player -> composite_attack_hit() );
  }
  return _attack_hit;
}

// player_stat_cache_t::attack_crit_chance =========================================

double player_stat_cache_t::attack_crit_chance() const
{
  if ( ! active || ! valid[ CACHE_ATTACK_CRIT_CHANCE ] )
  {
    valid[ CACHE_ATTACK_CRIT_CHANCE ] = true;
    _attack_crit_chance = player -> composite_melee_crit_chance();
  }
  else assert( _attack_crit_chance == player -> composite_melee_crit_chance() );
  return _attack_crit_chance;
}

// player_stat_cache_t::attack_haste ========================================

double player_stat_cache_t::attack_haste() const
{
  if ( ! active || ! valid[ CACHE_ATTACK_HASTE ] )
  {
    valid[ CACHE_ATTACK_HASTE ] = true;
    _attack_haste = player -> composite_melee_haste();
  }
  else assert( _attack_haste == player -> composite_melee_haste() );
  return _attack_haste;
}

// player_stat_cache_t::attack_speed ========================================

double player_stat_cache_t::attack_speed() const
{
  if ( ! active || ! valid[ CACHE_ATTACK_SPEED ] )
  {
    valid[ CACHE_ATTACK_SPEED ] = true;
    _attack_speed = player -> composite_melee_speed();
  }
  else assert( _attack_speed == player -> composite_melee_speed() );
  return _attack_speed;
}

// player_stat_cache_t::spell_hit ===========================================

double player_stat_cache_t::spell_hit() const
{
  if ( ! active || ! valid[ CACHE_SPELL_HIT ] )
  {
    valid[ CACHE_SPELL_HIT ] = true;
    _spell_hit = player -> composite_spell_hit();
  }
  else assert( _spell_hit == player -> composite_spell_hit() );
  return _spell_hit;
}

// player_stat_cache_t::spell_crit_chance ==========================================

double player_stat_cache_t::spell_crit_chance() const
{
  if ( ! active || ! valid[ CACHE_SPELL_CRIT_CHANCE ] )
  {
    valid[ CACHE_SPELL_CRIT_CHANCE ] = true;
    _spell_crit_chance = player -> composite_spell_crit_chance();
  }
  else assert( _spell_crit_chance == player -> composite_spell_crit_chance() );
  return _spell_crit_chance;
}

// player_stat_cache_t::spell_haste =========================================

double player_stat_cache_t::spell_haste() const
{
  if ( ! active || ! valid[ CACHE_SPELL_HASTE ] )
  {
    valid[ CACHE_SPELL_HASTE ] = true;
    _spell_haste = player -> composite_spell_haste();
  }
  else assert( _spell_haste == player -> composite_spell_haste() );
  return _spell_haste;
}

// player_stat_cache_t::spell_speed =========================================

double player_stat_cache_t::spell_speed() const
{
  if ( ! active || ! valid[ CACHE_SPELL_SPEED ] )
  {
    valid[ CACHE_SPELL_SPEED ] = true;
    _spell_speed = player -> composite_spell_speed();
  }
  else assert( _spell_speed == player -> composite_spell_speed() );
  return _spell_speed;
}

double player_stat_cache_t::dodge() const
{
  if ( ! active || ! valid[ CACHE_DODGE ] )
  {
    valid[ CACHE_DODGE ] = true;
    _dodge = player -> composite_dodge();
  }
  else assert( _dodge == player -> composite_dodge() );
  return _dodge;
}

double player_stat_cache_t::parry() const
{
  if ( ! active || ! valid[ CACHE_PARRY ] )
  {
    valid[ CACHE_PARRY ] = true;
    _parry = player -> composite_parry();
  }
  else assert( _parry == player -> composite_parry() );
  return _parry;
}

double player_stat_cache_t::block() const
{
  if ( ! active || ! valid[ CACHE_BLOCK ] )
  {
    valid[ CACHE_BLOCK ] = true;
    _block = player -> composite_block();
  }
  else assert( _block == player -> composite_block() );
  return _block;
}

double player_stat_cache_t::crit_block() const
{
  if ( ! active || ! valid[ CACHE_CRIT_BLOCK ] )
  {
    valid[ CACHE_CRIT_BLOCK ] = true;
    _crit_block = player -> composite_crit_block();
  }
  else assert( _crit_block == player -> composite_crit_block() );
  return _crit_block;
}

double player_stat_cache_t::crit_avoidance() const
{
  if ( ! active || ! valid[ CACHE_CRIT_AVOIDANCE ] )
  {
    valid[ CACHE_CRIT_AVOIDANCE ] = true;
    _crit_avoidance = player -> composite_crit_avoidance();
  }
  else assert( _crit_avoidance == player -> composite_crit_avoidance() );
  return _crit_avoidance;
}

double player_stat_cache_t::miss() const
{
  if ( ! active || ! valid[ CACHE_MISS ] )
  {
    valid[ CACHE_MISS ] = true;
    _miss = player -> composite_miss();
  }
  else assert( _miss == player -> composite_miss() );
  return _miss;
}

double player_stat_cache_t::armor() const
{
  if ( ! active || ! valid[ CACHE_ARMOR ] || ! valid[ CACHE_BONUS_ARMOR ] )
  {
    valid[ CACHE_ARMOR ] = true;
    _armor = player -> composite_armor();
  }
  else assert( _armor == player -> composite_armor() );
  return _armor;
}

double player_stat_cache_t::mastery() const
{
  if ( ! active || ! valid[ CACHE_MASTERY ] )
  {
    valid[ CACHE_MASTERY ] = true;
    _mastery = player -> composite_mastery();
    _mastery_value = player -> composite_mastery_value();
  }
  else assert( _mastery == player -> composite_mastery() );
  return _mastery;
}

/* This is composite_mastery * specialization_mastery_coefficient !
 *
 * If you need the pure mastery value, use player_t::composite_mastery
 */
double player_stat_cache_t::mastery_value() const
{
  if ( ! active || ! valid[ CACHE_MASTERY ] )
  {
    valid[ CACHE_MASTERY ] = true;
    _mastery = player -> composite_mastery();
    _mastery_value = player -> composite_mastery_value();
  }
  else assert( _mastery_value == player -> composite_mastery_value() );
  return _mastery_value;
}

double player_stat_cache_t::bonus_armor() const
{
  if ( ! active || ! valid[ CACHE_BONUS_ARMOR ] )
  {
    valid[ CACHE_BONUS_ARMOR ] = true;
    _bonus_armor = player -> composite_bonus_armor();
  }
  else assert( _bonus_armor == player -> composite_bonus_armor() );
  return _bonus_armor;
}

double player_stat_cache_t::damage_versatility() const
{
  if ( ! active || ! valid[ CACHE_DAMAGE_VERSATILITY ] )
  {
    valid[ CACHE_DAMAGE_VERSATILITY ] = true;
    _damage_versatility = player -> composite_damage_versatility();
  }
  else assert( _damage_versatility == player -> composite_damage_versatility() );
  return _damage_versatility;
}

double player_stat_cache_t::heal_versatility() const
{
  if ( ! active || ! valid[ CACHE_HEAL_VERSATILITY ] )
  {
    valid[ CACHE_HEAL_VERSATILITY ] = true;
    _heal_versatility = player -> composite_heal_versatility();
  }
  else assert( _heal_versatility == player -> composite_heal_versatility() );
  return _heal_versatility;
}

double player_stat_cache_t::mitigation_versatility() const
{
  if ( ! active || ! valid[ CACHE_MITIGATION_VERSATILITY ] )
  {
    valid[ CACHE_MITIGATION_VERSATILITY ] = true;
    _mitigation_versatility = player -> composite_mitigation_versatility();
  }
  else assert( _mitigation_versatility == player -> composite_mitigation_versatility() );
  return _mitigation_versatility;
}

double player_stat_cache_t::leech() const
{
  if ( ! active || ! valid[ CACHE_LEECH ] )
  {
    valid[ CACHE_LEECH ] = true;
    _leech = player -> composite_leech();
  }
  else assert( _leech == player -> composite_leech() );
  return _leech;
}

double player_stat_cache_t::run_speed() const
{
  if ( !active || !valid[ CACHE_RUN_SPEED ] )
  {
    valid[ CACHE_RUN_SPEED ] = true;
    _run_speed = player -> composite_movement_speed();
  }
  else assert( _run_speed == player -> composite_movement_speed() );
  return _run_speed;
}

double player_stat_cache_t::avoidance() const
{
  if ( !active || !valid[ CACHE_AVOIDANCE ] )
  {
    valid[ CACHE_AVOIDANCE ] = true;
    _avoidance = player -> composite_avoidance();
  }
  else assert( _avoidance == player -> composite_avoidance() );
  return _avoidance;
}

// player_stat_cache_t::player_multiplier =============================================

double player_stat_cache_t::player_multiplier( school_e s ) const
{
  if ( ! active || ! player_mult_valid[ s ] )
  {
    player_mult_valid[ s ] = true;
    _player_mult[ s ] = player -> composite_player_multiplier( s );
  }
  else assert( _player_mult[ s ] == player -> composite_player_multiplier( s ) );
  return _player_mult[ s ];
}

// player_stat_cache_t::player_heal_multiplier =============================================

double player_stat_cache_t::player_heal_multiplier( const action_state_t* s ) const
{
  school_e sch = s -> action -> get_school();

  if ( ! active || ! player_heal_mult_valid[ sch ] )
  {
    player_heal_mult_valid[ sch ] = true;
    _player_heal_mult[ sch ] = player -> composite_player_heal_multiplier( s );
  }
  else assert( _player_heal_mult[ sch ] == player -> composite_player_heal_multiplier( s ) );
  return _player_heal_mult[ sch ];
}

#endif

player_collected_data_t::action_sequence_data_t::action_sequence_data_t( const action_t* a, const player_t* t, const timespan_t& ts, const player_t* p ) :
  action( a ), target( t ), time( ts ), wait_time( timespan_t::zero() )
{
  for ( size_t i = 0; i < p -> buff_list.size(); ++i )
  {
    buff_t* b = p -> buff_list[ i ];
    if ( b -> check() && ! b -> quiet && ! b -> constant )
      buff_list.push_back( std::make_pair( b, b -> check() ) );
  }

  range::fill( resource_snapshot, -1 );
  range::fill( resource_max_snapshot, -1 );

  for ( resource_e i = RESOURCE_HEALTH; i < RESOURCE_MAX; ++i )
  {
    if ( p -> resources.max[ i ] > 0.0 )
    {
      resource_snapshot[ i ] = p -> resources.current[ i ];
      resource_max_snapshot[ i ] = p -> resources.max[ i ];
    }
  }
}

player_collected_data_t::action_sequence_data_t::action_sequence_data_t( const timespan_t& ts, const timespan_t& wait, const player_t* p ) :
  action( 0 ), target( 0 ), time( ts ), wait_time( wait )
{
  for ( size_t i = 0; i < p -> buff_list.size(); ++i )
  {
    buff_t* b = p -> buff_list[ i ];
    if ( b -> check() && ! b -> quiet && ! b -> constant )
      buff_list.push_back( std::make_pair( b, b -> check() ) );
  }

  range::fill( resource_snapshot, -1 );
  range::fill( resource_max_snapshot, -1 );

  for ( resource_e i = RESOURCE_HEALTH; i < RESOURCE_MAX; ++i )
  {
    if ( p -> resources.max[ i ] > 0.0 )
    {
      resource_snapshot[ i ] = p -> resources.current[ i ];
      resource_max_snapshot[ i ] = p -> resources.max[ i ];

    }
  }
}

player_collected_data_t::player_collected_data_t( const std::string& player_name, sim_t& s ) :
  fight_length( player_name + " Fight Length", s.statistics_level < 2),
  waiting_time(player_name + " Waiting Time", s.statistics_level < 4),
  pooling_time(player_name + " Pooling Time", s.statistics_level < 4),
  executed_foreground_actions(player_name + " Executed Foreground Actions", s.statistics_level < 4),
  dmg( player_name + " Damage", s.statistics_level < 2 ),
  compound_dmg( player_name + " Total Damage", s.statistics_level < 2 ),
  prioritydps( player_name + " Priority Target Damage Per Second", s.statistics_level < 1 ),
  dps( player_name + " Damage Per Second", s.statistics_level < 1 ),
  dpse( player_name + " Damage Per Second (Effective)", s.statistics_level < 2 ),
  dtps( player_name + " Damage Taken Per Second", s.statistics_level < 2 ),
  dmg_taken( player_name + " Damage Taken", s.statistics_level < 2 ),
  timeline_dmg(),
  heal( player_name + " Heal", s.statistics_level < 2 ),
  compound_heal( player_name + " Total Heal", s.statistics_level < 2 ),
  hps( player_name + " Healing Per Second", s.statistics_level < 1 ),
  hpse( player_name + " Healing Per Second (Effective)", s.statistics_level < 2 ),
  htps( player_name + " Healing Taken Per Second", s.statistics_level < 2 ),
  heal_taken( player_name + " Healing Taken", s.statistics_level < 2 ),
  absorb( player_name + " Absorb", s.statistics_level < 2 ),
  compound_absorb( player_name + " Total Absorb", s.statistics_level < 2 ),
  aps( player_name + " Absorb Per Second", s.statistics_level < 1 ),
  atps( player_name + " Absorb Taken Per Second", s.statistics_level < 2 ),
  absorb_taken( player_name + " Absorb Taken", s.statistics_level < 2 ),
  deaths( player_name + " Deaths", s.statistics_level < 2 ),
  theck_meloree_index( player_name + " Theck-Meloree Index", s.statistics_level < 1 ),
  effective_theck_meloree_index( player_name + "Theck-Meloree Index (Effective)", s.statistics_level < 1 ),
  max_spike_amount( player_name + " Max Spike Value", s.statistics_level < 1 ),
  target_metric( player_name + " Target Metric", false ),
  resource_timelines(),
  combat_end_resource( RESOURCE_MAX ),
  stat_timelines(),
  health_changes(),
  health_changes_tmi(),
  buffed_stats_snapshot()
{ }

void player_collected_data_t::reserve_memory( const player_t& p )
{
  int size = std::min( p.sim -> iterations, 10000 );
  fight_length.reserve( size );
  // DMG
  dmg.reserve( size );
  compound_dmg.reserve( size );
  dps.reserve( size );
  prioritydps.reserve( size );
  dpse.reserve( size );
  dtps.reserve( size );
  // HEAL
  heal.reserve( size );
  compound_heal.reserve( size );
  hps.reserve( size );
  hpse.reserve( size );
  htps.reserve( size );
  heal_taken.reserve( size );
  deaths.reserve( size );

  if ( ! p.is_pet() && p.primary_role() == ROLE_TANK )
  {
    theck_meloree_index.reserve( size );
    effective_theck_meloree_index.reserve( size );
    p.sim -> num_tanks++;
  }
}

void player_collected_data_t::merge( const player_collected_data_t& other )
{
  fight_length.merge( other.fight_length );
  waiting_time.merge( other.waiting_time );
  executed_foreground_actions.merge( other.executed_foreground_actions );
  // DMG
  dmg.merge( other.dmg );
  compound_dmg.merge( other.compound_dmg );
  dps.merge( other.dps );
  prioritydps.merge( other.prioritydps );
  dtps.merge( other.dtps );
  dpse.merge( other.dpse );
  dmg_taken.merge( other.dmg_taken );
  // HEAL
  heal.merge( other.heal );
  compound_heal.merge( other.compound_heal );
  hps.merge( other.hps );
  htps.merge( other.htps );
  hpse.merge( other.hpse );
  heal_taken.merge( other.heal_taken );
  // Tank
  deaths.merge( other.deaths );
  timeline_dmg_taken.merge( other.timeline_dmg_taken );
  timeline_healing_taken.merge( other.timeline_healing_taken );
  theck_meloree_index.merge( other.theck_meloree_index );
  effective_theck_meloree_index.merge( other.effective_theck_meloree_index );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    resource_lost  [ i ].merge( other.resource_lost[ i ] );
    resource_gained[ i ].merge( other.resource_gained[ i ] );
  }

  assert( resource_timelines.size() == other.resource_timelines.size() );
  for ( size_t i = 0; i < resource_timelines.size(); ++i )
  {
    assert( resource_timelines[ i ].type == other.resource_timelines[ i ].type );
    assert( resource_timelines[ i ].type != RESOURCE_NONE );
    resource_timelines[ i ].timeline.merge ( other.resource_timelines[ i ].timeline );
  }

  assert( stat_timelines.size() == other.stat_timelines.size() );
  for ( size_t i = 0; i < stat_timelines.size(); ++i )
  {
    assert( stat_timelines[ i ].type == other.stat_timelines[ i ].type );
    stat_timelines[ i ].timeline.merge ( other.stat_timelines[ i ].timeline );
  }

  health_changes.merged_timeline.merge( other.health_changes.merged_timeline );
  health_changes_tmi.merged_timeline.merge( other.health_changes_tmi.merged_timeline );
}

void player_collected_data_t::analyze( const player_t& p )
{
  fight_length.analyze();
  // DMG
  dmg.analyze();
  compound_dmg.analyze();
  dps.analyze();
  prioritydps.analyze();
  dpse.analyze();
  dmg_taken.analyze();
  dtps.analyze();
  // Heal
  heal.analyze();
  compound_heal.analyze();
  hps.analyze();
  hpse.analyze();
  heal_taken.analyze();
  htps.analyze();
  // Absorb
  absorb.analyze();
  compound_absorb.analyze();
  aps.analyze();
  absorb_taken.analyze();
  atps.analyze();
  // Tank
  deaths.analyze();
  theck_meloree_index.analyze();
  effective_theck_meloree_index.analyze();
  max_spike_amount.analyze();

  if ( ! p.sim -> single_actor_batch )
  {
    timeline_dmg_taken.adjust( *p.sim );
    timeline_healing_taken.adjust( *p.sim );

    range::for_each( resource_timelines, [ &p ]( resource_timeline_t& tl ) { tl.timeline.adjust( *p.sim ); } );
    range::for_each( stat_timelines, [ &p ]( stat_timeline_t& tl ) { tl.timeline.adjust( *p.sim ); } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( *p.sim );
    health_changes_tmi.merged_timeline.adjust( *p.sim );
  }
  // Single actor batch mode has to analyze the timelines in relation to their own fight lengths,
  // instead of the simulation-wide fight length.
  else
  {
    timeline_dmg_taken.adjust( fight_length );
    timeline_healing_taken.adjust( fight_length );

    range::for_each( resource_timelines, [ this ]( resource_timeline_t& tl ) { tl.timeline.adjust( fight_length ); } );
    range::for_each( stat_timelines, [ this ]( stat_timeline_t& tl ) { tl.timeline.adjust( fight_length ); } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( fight_length );
    health_changes_tmi.merged_timeline.adjust( fight_length );
  }
}

//This is pretty much only useful for dev debugging at this point, would need to modify to make it useful to users
void player_collected_data_t::print_tmi_debug_csv( const sc_timeline_t* nma, const std::vector<double>& wv, const player_t& p )
{
  if ( ! p.tmi_debug_file_str.empty() )
  {
    io::ofstream f;
    f.open( p.tmi_debug_file_str );
    // write elements to CSV
    f << p.name_str << " TMI data:\n";

    f << "damage,healing,health chg,norm health chg,norm mov avg, weighted val\n";

    for ( size_t i = 0; i < health_changes.timeline.data().size(); i++ )
    {
      f.format( "%f,%f,%f,%f,%f,%f\n", timeline_dmg_taken.data()[ i ],
          timeline_healing_taken.data()[ i ],
          health_changes.timeline.data()[ i ],
          health_changes.timeline_normalized.data()[ i ],
          nma -> data()[ i ],
          wv[ i ] );
    }
    f << "\n";
  }
}

double player_collected_data_t::calculate_max_spike_damage( const health_changes_timeline_t& tl, int window )
{
  double max_spike = 0;

  // declare sliding average timeline
  sc_timeline_t sliding_average_tl;

  // create sliding average timelines from data
  tl.timeline_normalized.build_sliding_average_timeline( sliding_average_tl, window );

  // pull the data out of the normalized sliding average timeline
  std::vector<double> weighted_value = sliding_average_tl.data();

  // extract the max spike size from the sliding average timeline
  max_spike = *std::max_element( weighted_value.begin(), weighted_value.end() ); // todo: remove weighted_value here
  max_spike *= window;

  return max_spike;
}

double player_collected_data_t::calculate_tmi( const health_changes_timeline_t& tl, int window, double f_length, const player_t& p )
{
  // The Theck-Meloree Index is a metric that attempts to quantize the smoothness of damage intake.
  // It performs an exponentially-weighted sum of the moving average of damage intake, with larger
  // damage spikes being weighted more heavily. A formal definition of the metric can be found here:
  // http://www.sacredduty.net/theck-meloree-index-standard-reference-document/

  // accumulator
  double tmi = 0;

  // declare sliding average timeline
  sc_timeline_t sliding_average_tl;

  // create sliding average timelines from data
  tl.timeline_normalized.build_sliding_average_timeline( sliding_average_tl, window );

  // pull the data out of the normalized sliding average timeline
  std::vector<double> weighted_value = sliding_average_tl.data();

  // define constants
  double D = 10; // filtering strength
  double c2 = 450; // N_0, default fight length for normalization
  double c1 = 100000 / D; // health scale factor, determines slope of plot

  for (auto & elem : weighted_value)
  {
    // weighted_value is the moving average (i.e. 1-second), so multiply by window size to get damage in "window" seconds
    elem *= window;

    // calculate exponentially-weighted contribution of this data point using filter strength D
    elem = std::exp( D * elem );

    // add to the TMI total; strictly speaking this should be moved outside the for loop and turned into a sort() followed by a sum for numerical accuracy
    tmi += elem;
  }

  // multiply by vertical offset factor c2
  tmi *= c2;
  // normalize for fight length - should be equivalent to dividing by tl.timeline_normalized.data().size()
  tmi /= f_length;
  tmi *= tl.get_bin_size();
  // take log of result
  tmi = std::log( tmi );
  // multiply by health decade scale factor
  tmi *= c1;

  // if an output file has been defined, write to it
  if ( ! p.tmi_debug_file_str.empty() )
    print_tmi_debug_csv( &sliding_average_tl, weighted_value, p );

  return tmi;
}

void player_collected_data_t::collect_data( const player_t& p )
{
  double f_length = p.iteration_fight_length.total_seconds();
  double sim_length = p.sim -> current_time().total_seconds();
  double w_time = p.iteration_waiting_time.total_seconds();
  double p_time = p.iteration_pooling_time.total_seconds();
  assert( p.iteration_fight_length <= p.sim -> current_time() );

  fight_length.add( f_length );
  waiting_time.add( w_time );
  pooling_time.add( p_time );

  executed_foreground_actions.add( p.iteration_executed_foreground_actions );

  // Player only dmg/heal
  dmg.add( p.iteration_dmg );
  heal.add( p.iteration_heal );
  absorb.add( p.iteration_absorb );

  double total_iteration_dmg = p.iteration_dmg; // player + pet dmg
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_dmg += p.pet_list[i] -> iteration_dmg;
  }
  double total_priority_iteration_dmg = p.priority_iteration_dmg;
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_priority_iteration_dmg += p.pet_list[i] -> priority_iteration_dmg;
  }
  double total_iteration_heal = p.iteration_heal; // player + pet heal
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_heal += p.pet_list[ i ] -> iteration_heal;
  }
  double total_iteration_absorb = p.iteration_absorb;
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_absorb += p.pet_list[ i ] -> iteration_absorb;
  }

  compound_dmg.add( total_iteration_dmg );
  prioritydps.add( f_length ? total_priority_iteration_dmg / f_length : 0 );
  dps.add( f_length ? total_iteration_dmg / f_length : 0 );
  dpse.add( sim_length ? total_iteration_dmg / sim_length : 0 );
  double dps_metric = f_length ? ( total_iteration_dmg / f_length ) : 0;

  compound_heal.add( total_iteration_heal );
  hps.add( f_length ? total_iteration_heal / f_length : 0 );
  hpse.add( sim_length ? total_iteration_heal / sim_length : 0 );
  compound_absorb.add( total_iteration_absorb );
  aps.add( f_length ? total_iteration_absorb / f_length : 0.0 );
  double heal_metric = f_length ? ( ( total_iteration_heal + total_iteration_absorb ) / f_length ) : 0;

  heal_taken.add( p.iteration_heal_taken );
  htps.add( f_length ? p.iteration_heal_taken / f_length : 0 );
  absorb_taken.add( p.iteration_absorb_taken );
  atps.add( f_length ? p.iteration_absorb_taken / f_length : 0.0 );

  dmg_taken.add( p.iteration_dmg_taken );
  dtps.add( f_length ? p.iteration_dmg_taken / f_length : 0 );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    resource_lost  [ i ].add( p.iteration_resource_lost[i] );
    resource_gained[ i ].add( p.iteration_resource_gained[i] );
  }

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
    combat_end_resource[ i ].add( p.resources.current[ i ] );

  // Health Change Calculations - only needed for tanks
  double tank_metric = 0;
  if ( ! p.is_pet() && p.primary_role() == ROLE_TANK )
  {

    double tmi = 0; // TMI result
    double etmi = 0; // ETMI result
    double max_spike = 0; // Maximum spike size
    health_changes.merged_timeline.merge( health_changes.timeline );
    health_changes_tmi.merged_timeline.merge( health_changes_tmi.timeline );

    // Calculate Theck-Meloree Index (TMI), ETMI, and maximum spike damage
    if ( ! p.is_enemy() ) // Boss TMI is irrelevant, causes problems in iteration #1
    {
      if ( f_length )
      {
        // define constants and variables
        int window = (int) std::floor( p.tmi_window / health_changes_tmi.get_bin_size() + 0.5 ); // window size, bin time replaces 1 eventually

        // Standard TMI uses health_changes_tmi, ignoring externals - use health_changes_tmi
        tmi = calculate_tmi( health_changes_tmi, window, f_length, p );

        // ETMI includes external healing - use health_changes
        etmi = calculate_tmi( health_changes, window, f_length, p );

        // Max spike uses health_changes_tmi as well, ignores external heals - use health_changes_tmi
        max_spike = calculate_max_spike_damage( health_changes_tmi, window );

        tank_metric = tmi;
      }
    }
    theck_meloree_index.add( tmi );
    effective_theck_meloree_index.add( etmi );
    max_spike_amount.add( max_spike * 100.0 );
  }

  if ( p.sim -> target_error > 0 && ! p.is_pet() && ! p.is_enemy() )
  {
    double metric=0;

    switch( p.primary_role() )
    {
    case ROLE_ATTACK:
    case ROLE_SPELL:
    case ROLE_HYBRID:
    case ROLE_DPS:
      metric = dps_metric;
      break;

    case ROLE_TANK:
      metric = tank_metric;
      break;

    case ROLE_HEAL:
      metric = heal_metric;
      break;

    default:;
    }

    player_collected_data_t& cd = p.parent ? p.parent -> collected_data : *this;

    AUTO_LOCK( cd.target_metric_mutex );
    cd.target_metric.add( metric );
  }
}

std::ostream& player_collected_data_t::data_str( std::ostream& s ) const
{
  fight_length.data_str( s );
  dmg.data_str( s );
  compound_dmg.data_str( s );
  dps.data_str( s );
  prioritydps.data_str( s );
  dpse.data_str( s );
  dtps.data_str( s );
  dmg_taken.data_str( s );
  timeline_dmg.data_str( s );
  timeline_dmg_taken.data_str( s );
  // Heal
  heal.data_str( s );
  compound_heal.data_str( s );
  hps.data_str( s );
  hpse.data_str( s );
  htps.data_str( s );
  heal_taken.data_str( s );
  timeline_healing_taken.data_str( s );
  // TMI
  theck_meloree_index.data_str( s );
  effective_theck_meloree_index.data_str( s );
  max_spike_amount.data_str( s );

  return s;
}

void player_talent_points_t::clear()
{  range::fill( choices, -1 ); }

std::string player_talent_points_t::to_string() const
{
  std::ostringstream ss;

  ss << "{ ";
  for ( int i = 0; i < MAX_TALENT_ROWS; ++i )
  {
    if ( i ) ss << ", ";
    ss << choice( i );
  }
  ss << " }";

  return ss.str();
}

luxurious_sample_data_t::luxurious_sample_data_t( player_t& p, std::string n ) :
    extended_sample_data_t( n, p.sim -> statistics_level < 3 ),
  player( p ),
  buffer_value( 0.0 )
{

}

action_t* player_t::select_action( const action_priority_list_t& list )
{
  // Mark this action list as visited with the APL internal id
  visited_apls_ |= list.internal_id_mask;

  // Cached copy for recursion, we'll need it if we come back from a
  // call_action_list tree, with nothing to show for it.
  uint64_t _visited = visited_apls_;
  size_t attempted_random = 0;

  for ( size_t i = 0, num_actions = list.foreground_action_list.size(); i < num_actions; ++i )
  {
    visited_apls_ = _visited;
    action_t* a = 0;

    if ( list.random == 1 )
    {
      size_t random = static_cast<size_t>( rng().range( 0, static_cast<double>( num_actions ) ) );
      a = list.foreground_action_list[random];
    }
    else
    {
      double skill = list.player -> current.skill - list.player -> current.skill_debuff;
      if ( skill != 1 && rng().roll( ( 1 - skill ) * 0.5 ) )
      {
        size_t max_random_attempts = static_cast<size_t>( num_actions * ( skill * 0.5 ) );
        size_t random = static_cast<size_t>( rng().range( 0, static_cast<double>( num_actions ) ) );
        a = list.foreground_action_list[random];
        attempted_random++;
        // Limit the amount of attempts to select a random action based on skill, then bail out and try again in 100 ms.
        if ( attempted_random > max_random_attempts )
          break;
      }
      else
      {
        a = list.foreground_action_list[i];
      }
    }

    if ( a -> background ) continue;

    if ( a -> wait_on_ready == 1 )
      break;

    if ( a -> ready() )
    {
      // Execute variable operation, and continue processing
      if ( a -> type == ACTION_VARIABLE )
      {
        a -> execute();
        continue;
      }
      // Call_action_list action, don't execute anything, but rather recurse
      // into the called action list.
      else if ( a -> type == ACTION_CALL )
      {
        call_action_list_t* call = static_cast<call_action_list_t*>( a );
        // If the called APLs bitflag (based on internal id) is up, we're in an
        // infinite loop, and need to cancel the sim
        if ( visited_apls_ & call -> alist -> internal_id_mask )
        {
          sim -> errorf( "%s action list in infinite loop", name() );
          sim -> cancel();
          return 0;
        }

        // We get an action from the call, return it
        if ( action_t* real_a = select_action( *call -> alist ) )
        {
          if ( real_a -> action_list )
            real_a -> action_list -> used = true;
          return real_a;
        }
      }
      // .. in branch prediction, we trust
      else
      {
        return a;
      }
    }
  }

  return 0;
}

player_t* player_t::actor_by_name_str( const std::string& name ) const
{
  // Check player pets first
  for ( size_t i = 0; i < pet_list.size(); i++ )
  {
    if ( util::str_compare_ci( pet_list[ i ] -> name_str, name ) )
      return pet_list[ i ];
  }

  // Check harmful targets list
  for ( size_t i = 0; i < sim -> target_list.size(); i++ )
  {
    if ( util::str_compare_ci( sim -> target_list[ i ] -> name_str, name ) )
      return sim -> target_list[ i ];
  }

  // Finally, check player (non pet list), don't support targeting other
  // people's pets for now
  for ( size_t i = 0; i < sim -> player_no_pet_list.size(); i++ )
  {
    if ( util::str_compare_ci( sim -> player_no_pet_list[ i ] -> name_str, name ) )
      return sim -> player_no_pet_list[ i ];
  }

  return 0;
}

bool player_t::artifact_enabled() const
{
  if ( artifact.artifact_ == 1 )
  {
    return true;
  }
  else if ( artifact.artifact_ == 0 )
  {
    return false;
  }
  else
  {
    return artifact.slot != SLOT_INVALID;
  }
}

slot_e player_t::parent_item_slot( const item_t& item ) const
{
  unsigned parent = dbc.parent_item( item.parsed.data.id );
  if ( parent == 0 )
  {
    return SLOT_INVALID;
  }

  auto it = range::find_if( items, [ parent ]( const item_t& item ) {
    return parent == item.parsed.data.id;
  } );

  if ( it != items.end() )
  {
    return (*it).slot;
  }

  return SLOT_INVALID;
}

slot_e player_t::child_item_slot( const item_t& item ) const
{
  unsigned child = dbc.child_item( item.parsed.data.id );
  if ( child == 0 )
  {
    return SLOT_INVALID;
  }

  auto it = range::find_if( items, [ child ]( const item_t& item ) {
    return child == item.parsed.data.id;
  } );

  if ( it != items.end() )
  {
    return (*it).slot;
  }

  return SLOT_INVALID;
}

// TODO: This currently does not take minimum GCD into account, should it?
void player_t::adjust_global_cooldown( haste_type_e haste_type )
{
  // Don't adjust if the current gcd isnt hasted
  if ( gcd_haste_type == HASTE_NONE )
  {
    return;
  }

  // Don't adjust if the changed haste type isnt current GCD haste scaling type
  if ( haste_type != HASTE_ANY && gcd_haste_type != haste_type )
  {
    return;
  }

  // Don't adjust elapsed GCDs
  if ( ( readying && readying -> occurs() <= sim -> current_time() ) ||
       ( gcd_ready <= sim -> current_time() ) )
  {
    return;
  }

  double new_haste = 0;
  switch ( gcd_haste_type )
  {
    case HASTE_SPELL:
      new_haste = cache.spell_haste();
      break;
    case HASTE_ATTACK:
      new_haste = cache.attack_haste();
      break;
    case SPEED_SPELL:
      new_haste = cache.spell_speed();
      break;
    case SPEED_ATTACK:
      new_haste = cache.attack_speed();
      break;
    // SPEED_ANY and HASTE_ANY are nonsensical, actions have to have a correct GCD haste type so
    // they can be adjusted on state changes.
    default:
      assert( 0 && "player_t::adjust_global_cooldown called without proper haste type" );
      break;
  }

  // Haste did not change, don't do anything
  if ( new_haste == gcd_current_haste_value )
  {
    return;
  }

  double delta = new_haste / gcd_current_haste_value;
  timespan_t remains = readying ? readying -> remains() : ( gcd_ready - sim -> current_time() ),
             new_remains = remains * delta;

  // Don't bother processing too small (less than a millisecond) granularity changes
  if ( remains == new_remains )
  {
    return;
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s adjusting GCD due to haste change: old_ready=%.3f new_ready=%.3f old_haste=%f new_haste=%f delta=%f",
      name(),
      ( sim -> current_time() + remains ).total_seconds(),
      ( sim -> current_time() + new_remains ).total_seconds(),
      gcd_current_haste_value, new_haste, delta );
  }

  // We need to adjust the event (GCD is already elapsing)
  if ( readying )
  {
    // GCD speeding up, recreate the event
    if ( delta < 1 )
    {
      event_t::cancel( readying );
      readying = make_event<player_ready_event_t>( *sim, *this, new_remains );
    }
    // GCD slowing down, just reschedule into future
    else
    {
      readying -> reschedule( new_remains );
    }
  }

  gcd_ready = sim -> current_time() + new_remains;
  gcd_current_haste_value = new_haste;
}

void player_t::adjust_auto_attack( haste_type_e haste_type )
{
  // Don't adjust autoattacks on spell-derived haste
  if ( haste_type == SPEED_SPELL || haste_type == HASTE_SPELL )
  {
    return;
  }

  if ( main_hand_attack ) main_hand_attack -> reschedule_auto_attack( current_attack_speed );
  if ( off_hand_attack ) off_hand_attack -> reschedule_auto_attack( current_attack_speed );

  current_attack_speed = cache.attack_speed();
}

// Adjust the queue-delayed action execution if the ability currently being executed has a hasted
// cooldown, and haste changes
void player_t::adjust_action_queue_time()
{
  if ( ! queueing )
  {
    return;
  }

  queueing -> reschedule_queue_event();
}

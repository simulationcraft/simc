// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ============================================

// hymn_of_hope_buff ========================================================

struct hymn_of_hope_buff_t : public buff_t
{
  double mana_gain;

  hymn_of_hope_buff_t( player_t* p, const std::string& n, const spell_data_t* sp ) :
    buff_t ( buff_creator_t( p, n, sp ) ), mana_gain( 0 )
  { }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    buff_t::start( stacks, value, duration );

    // Extra Mana is only added at the start, not on refresh. Tested 20/01/2011.
    // Extra Mana is set by current max_mana, doesn't change when max_mana changes.
    mana_gain = player -> resources.max[ RESOURCE_MANA ] * data().effectN( 2 ).percent();
    player -> stat_gain( STAT_MAX_MANA, mana_gain, player -> gains.hymn_of_hope );
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    player -> stat_loss( STAT_MAX_MANA, mana_gain );
  }
};

// Player Ready Event =======================================================

struct player_ready_event_t : public event_t
{
  player_ready_event_t( player_t& p,
                        timespan_t delta_time ) :
                          event_t( p, "Player-Ready" )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New Player-Ready Event: %s", p.name() );

    sim().add_event( this, delta_time );
  }

  virtual void execute()
  {
    // Player that's checking for off gcd actions to use, cancels that checking when there's a ready event firing.
    if ( p() -> off_gcd )
      core_event_t::cancel( p() -> off_gcd );

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
      else p() -> started_waiting = sim().current_time;
    }
  }
};

/* Event which will demise the player
 * - Reason for it are that we need to finish the current action ( eg. a dot tick ) without
 * killing off target dependent things ( eg. dot state ).
 */
struct demise_event_t : public event_t
{
  demise_event_t( player_t& p,
                  timespan_t delta_time = timespan_t::zero() /* Instantly kill the player */ ) :
     event_t( p, "Player-Demise" )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New Player-Demise Event: %s", p.name() );

    sim().add_event( this, delta_time );
  }

  virtual void execute()
  {
    p() -> demise();
  }
};



// has_foreground_actions ===================================================

bool has_foreground_actions( player_t* p )
{
  if ( ! p -> active_action_list ) return false;
  return ( p -> active_action_list -> foreground_action_list.size() > 0 );
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
      if ( ! isdigit( url[ i ] ) )
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
    sim->errorf( "\n%s specialization string \"%s\" not valid.\n", sim -> active_player-> name(), value.c_str() );

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

} // UNNAMED NAMESPACE ======================================================

// This is a template for Ignite like mechanics, like of course Ignite, Hunter Piercing Shots, Priest Echo of Light, etc.
// It should get specialized in the class module

// Detailed MoP Ignite Mechanic description at
// http://us.battle.net/wow/en/forum/topic/5889309137?page=40#787

// There is still a delay between the impact of the triggering spell and the dot application/refresh and damage calculation.
void ignite::trigger_pct_based( action_t* ignite_action,
                                player_t* t,
                                double dmg )
{
  struct delay_event_t : public event_t
  {
    double additional_ignite_dmg;
    player_t* target;
    action_t* action;

    delay_event_t( player_t* t, action_t* a, double dmg ) :
      event_t( *a -> player, "Ignite Sampling Event" ),
      additional_ignite_dmg( dmg ), target( t ), action( a )
    {
      // Use same delay as in buff application
      timespan_t delay_duration = sim().gauss( sim().default_aura_delay, sim().default_aura_delay_stddev );

      if ( sim().debug )
        sim().out_debug.printf( "New %s Sampling Event: %s ( delta time: %.4f )",
                    action -> name(), p() -> name(), delay_duration.total_seconds() );

      sim().add_event( this, delay_duration );
    }

    virtual void execute()
    {
      if ( target -> is_sleeping() )
        return;

      dot_t* dot = action -> get_dot( target );

      double new_total_ignite_dmg = additional_ignite_dmg;

      assert( action -> num_ticks > 0 );

      if ( dot -> ticking )
        new_total_ignite_dmg += action -> base_td * dot -> ticks();

      if ( sim().debug )
      {
        if ( dot -> ticking )
        {
          sim().out_debug.printf( "ignite_delay_event::execute(): additional_damage=%f current_ignite_tick=%f ticks_left=%d new_ignite_dmg=%f",
                      additional_ignite_dmg, action -> base_td, dot -> ticks(), new_total_ignite_dmg );
        }
        else
        {
          sim().out_debug.printf( "ignite_delay_event::execute(): additional_damage=%f new_ignite_dmg=%f",
                      additional_ignite_dmg, new_total_ignite_dmg );
        }
      }

      // Pass total amount of damage to the ignite action, and let it divide it by the correct number of ticks!

      // action execute
      {
        action_state_t* s = action -> get_state();
        s -> target = target;
        s -> result = RESULT_HIT;
        action -> snapshot_state( s, action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );
        s -> result_amount = new_total_ignite_dmg;
        action -> schedule_travel( s );
        if ( ! action -> dual ) action -> stats -> add_execute( timespan_t::zero(), s -> target );
      }
    }
  };

  assert( ignite_action );

  new ( *ignite_action -> sim ) delay_event_t( t, ignite_action, dmg );
}

// Stormlash ================================================================

struct stormlash_spell_t : public spell_t
{
  stormlash_spell_t( player_t* p ) :
    spell_t( "stormlash", p, p -> find_spell( 120687 ) )
  {
    may_crit   = true;
    special    = true;
    background = true;
    proc       = true;
    callbacks  = false;
    base_attack_power_multiplier = 0;
    base_spell_power_multiplier  = 0;
  }
};

struct stormlash_callback_t : public action_callback_t
{
  spell_t*  stormlash_spell;
  cooldown_t* cd;
  action_t* stormlash_aggregate;
  stats_t* stormlash_aggregate_stat;
  std::vector< stats_t* > stormlash_sources;

  stormlash_callback_t( player_t* p );

  virtual void activate();

  virtual void trigger( action_t* a, void* call_data );
};

stormlash_callback_t::stormlash_callback_t( player_t* p ) :
  action_callback_t( p ),
  stormlash_spell( new stormlash_spell_t( p ) ),
  cd( p -> get_cooldown( "stormlash" ) ),
  stormlash_aggregate( 0 ),
  stormlash_aggregate_stat()
{
  cd -> duration = timespan_t::from_seconds( 0.1 );
  stormlash_sources.resize( p -> sim -> num_players + 1 );
}

void stormlash_callback_t::activate()
{
  action_callback_t::activate();

  if ( ! stormlash_aggregate )
    return;

  player_t* o = stormlash_aggregate -> player -> cast_pet() -> owner;
  if ( ! stormlash_sources[ o -> index ] )
  {
    std::string stat_name = "stormlash_";
    if ( listener -> is_pet() )
      stat_name += listener -> cast_pet() -> owner -> name_str + "_";
    stat_name += listener -> name_str;

    stormlash_sources[ o -> index ] = stormlash_aggregate -> player -> get_stats( stat_name, stormlash_aggregate );
  }

  stormlash_aggregate_stat = stormlash_sources[ o -> index ];
}

// http://us.battle.net/wow/en/forum/topic/5889309137?page=101#2017
void stormlash_callback_t::trigger( action_t* a, void* call_data )
{
  if ( cd -> down() )
    return;

  action_state_t* s = reinterpret_cast< action_state_t* >( call_data );

  if ( s -> result_amount == 0 )
    return;

  if ( a -> stormlash_da_multiplier == 0 && s -> result_type == DMG_DIRECT )
    return;

  if ( a -> stormlash_ta_multiplier == 0 && s -> result_type == DMG_OVER_TIME )
    return;

  cd -> start();

  double amount = 0;
  double base_power = std::max( a -> player -> cache.attack_power() * a -> player -> composite_attack_power_multiplier() * 0.2,
                                a -> player -> cache.spell_power( a -> school ) * a -> player -> composite_spell_power_multiplier() * 0.3 );
  double base_multiplier = 1.0;
  double cast_time_multiplier = 1.0;
  bool auto_attack = false;

  // Autoattacks
  if ( a -> special == false )
  {
    auto_attack = true;

    // Presume non-special attacks without weapon are main hand attacks
    if ( ! a -> weapon || a -> weapon -> slot == SLOT_MAIN_HAND )
      base_multiplier = 0.4;
    else
      base_multiplier = 0.2;

    if ( a -> weapon )
      cast_time_multiplier = a -> weapon -> swing_time.total_seconds() / 2.6;
    // If no weapon is defined for autoattacks, we should break really .. but use base_execute_time of the spell then.
    else
      cast_time_multiplier = a -> base_execute_time.total_seconds() / 2.6;
  }
  else
  {
    if ( a -> data().cast_time( a -> player -> level ) > timespan_t::from_seconds( 1.5 ) )
      cast_time_multiplier = a -> data().cast_time( a -> player -> level ).total_seconds() / 1.5;
  }

  if ( a -> player -> type == PLAYER_PET )
    base_multiplier *= 0.2;

  amount = base_power * base_multiplier * cast_time_multiplier;
  amount *= ( s -> result_type == DMG_DIRECT ) ? a -> stormlash_da_multiplier : a -> stormlash_ta_multiplier;

  if ( a -> sim -> debug )
  {
    a -> sim -> out_debug.printf( "%s stormlash proc by %s: power=%.3f base_mul=%.3f cast_time=%.3f cast_time_mul=%.3f spell_multiplier=%.3f amount=%.3f %s",
                        a -> player -> name(), a -> name(), base_power, base_multiplier, a -> data().cast_time( a -> player -> level ).total_seconds(), cast_time_multiplier,
                        ( a -> stormlash_da_multiplier ) ? a -> stormlash_da_multiplier : a -> stormlash_ta_multiplier,
                        amount,
                        ( auto_attack ) ? "(auto attack)" : "" );
  }

  stormlash_spell -> base_dd_min = amount - ( a -> sim -> average_range == 0 ? amount * .15 : 0 );
  stormlash_spell -> base_dd_max = amount + ( a -> sim -> average_range == 0 ? amount * .15 : 0 );

  if ( unlikely( stormlash_aggregate && stormlash_aggregate -> player -> cast_pet() -> owner == a -> player ) )
  {
    assert( stormlash_aggregate_stat -> player == stormlash_aggregate -> player );
    stats_t* tmp_stats = stormlash_spell -> stats;
    stormlash_spell -> stats = stormlash_aggregate_stat;
    stormlash_spell -> execute();
    stormlash_spell -> stats = tmp_stats;
  }
  else
    stormlash_spell -> execute();

  if ( unlikely( stormlash_aggregate && stormlash_aggregate -> player -> cast_pet() -> owner != a -> player ) )
  {
    assert( stormlash_aggregate_stat -> player == stormlash_aggregate -> player );
    stormlash_aggregate_stat -> add_execute( timespan_t::zero(), stormlash_spell -> target );
    stormlash_aggregate_stat -> add_result( stormlash_spell -> execute_state -> result_amount,
                                            stormlash_spell -> execute_state -> result_amount,
                                            stormlash_spell -> execute_state -> result_type,
                                            stormlash_spell -> execute_state -> result,
                                            stormlash_spell -> execute_state -> block_result,
                                            stormlash_spell -> target );
  }
}

stormlash_buff_t::stormlash_buff_t( player_t* p, const spell_data_t* s ) :
  buff_t( buff_creator_t( p, "stormlash", s ).cd( timespan_t::from_seconds( 0.1 ) ).duration( timespan_t::from_seconds( 10.0 ) ) ),
  stormlash_aggregate( 0 ),
  stormlash_cb( new stormlash_callback_t( p ) )
{
  p -> callbacks.register_direct_damage_callback( SCHOOL_SPELL_MASK | SCHOOL_ATTACK_MASK, stormlash_cb );
  p -> callbacks.register_tick_damage_callback( SCHOOL_SPELL_MASK | SCHOOL_ATTACK_MASK, stormlash_cb );
  stormlash_cb -> deactivate();
}

void stormlash_buff_t::execute( int stacks, double value, timespan_t duration )
{
  buff_t::execute( stacks, value, duration );
  if ( stormlash_aggregate )
    stormlash_cb -> stormlash_aggregate = stormlash_aggregate;
  stormlash_cb -> activate();
}

void stormlash_buff_t::expire_override()
{
  buff_t::expire_override();

  if ( stormlash_cb -> stormlash_aggregate == stormlash_aggregate )
    stormlash_cb -> stormlash_aggregate = 0;
  stormlash_cb -> deactivate();
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

  index( -1 ),

  // (static) attributes
  race( r ),
  role( ROLE_HYBRID ),
  level( default_level ),
  party( 0 ), member( 0 ),
  ready_type( READY_POLL ),
  _spec( SPEC_NONE ),
  bugs( true ),
  scale_player( true ),
  tmi_self_only( false ),
  death_pct( 0.0 ),

  // dynamic stuff
  target( 0 ),
  initialized( 0 ), potion_used( false ),

  region_str( s -> default_region_str ), server_str( s -> default_server_str ), origin_str(),
  gcd_ready( timespan_t::zero() ), base_gcd( timespan_t::from_seconds( 1.5 ) ), started_waiting( timespan_t::min() ),
  pet_list(), active_pets(),
  vengeance_list( this ),invert_scaling( 0 ),
  avg_ilvl( 0 ),
  // Reaction
  reaction_offset( timespan_t::from_seconds( 0.1 ) ), reaction_mean( timespan_t::from_seconds( 0.3 ) ), reaction_stddev( timespan_t::zero() ), reaction_nu( timespan_t::from_seconds( 0.25 ) ),
  // Latency
  world_lag( timespan_t::from_seconds( 0.1 ) ), world_lag_stddev( timespan_t::min() ),
  brain_lag( timespan_t::zero() ), brain_lag_stddev( timespan_t::min() ),
  world_lag_override( false ), world_lag_stddev_override( false ),
  dbc( s -> dbc ),
  talent_points(),
  glyph_list(),
  base(),
  initial(),
  current(),
  // Spell Mechanics
  base_energy_regen_per_second( 0 ), base_focus_regen_per_second( 0 ), base_chi_regen_per_second( 0 ),
  last_cast( timespan_t::zero() ),
  // Defense Mechanics
  diminished_dodge_cap( 0 ), diminished_parry_cap( 0 ), diminished_block_cap( 0 ), diminished_kfactor( 0 ),
  // Attacks
  main_hand_attack( 0 ), off_hand_attack( 0 ),
  // Resources
  resources( resources_t() ),
  // Consumables
  active_elixir(),
  flask( FLASK_NONE ),
  food( FOOD_NONE ),
  // Events
  executing( 0 ), channeling( 0 ), readying( 0 ), off_gcd( 0 ), in_combat( false ), action_queued( false ),
  last_foreground_action( 0 ),
  cast_delay_reaction( timespan_t::zero() ), cast_delay_occurred( timespan_t::zero() ),
  // Actions
  action_list_default( 0 ),
  precombat_action_list( 0 ), active_action_list( 0 ), active_off_gcd_list( 0 ), restore_action_list( 0 ),
  no_action_list_provided(),
  // Reporting
  quiet( 0 ),
  iteration_fight_length( timespan_t::zero() ), arise_time( timespan_t::min() ),
  iteration_waiting_time( timespan_t::zero() ), iteration_executed_foreground_actions( 0 ),
  rps_gain( 0 ), rps_loss( 0 ),
  buffed( buffed_stats_t() ),
  collected_data( player_collected_data_t( name_str, *sim ) ),
  vengeance( collected_data.vengeance_timeline ),
  // Damage
  iteration_dmg( 0 ), iteration_dmg_taken( 0 ),
  dpr( 0 ),
  dps_convergence( 0 ),
  // Heal
  iteration_heal( 0 ), iteration_heal_taken( 0 ),
  hpr( 0 ),

  report_information( player_processed_report_information_t() ),
  // Gear
  sets( nullptr ),
  meta_gem( META_GEM_NONE ), matching_gear( false ),
  item_cooldown( cooldown_t( "item_cd", *this ) ),
  legendary_tank_cloak_cd( get_cooldown( "endurance_of_niuzao" ) ),
  // Scaling
  scaling_lag( 0 ), scaling_lag_error( 0 ),
  // Movement & Position
  base_movement_speed( 7.0 ), x_position( 0.0 ), y_position( 0.0 ),
  buffs( buffs_t() ),
  potion_buffs( potion_buffs_t() ),
  debuffs( debuffs_t() ),
  gains( gains_t() ),
  procs( procs_t() ),
  uptimes( uptimes_t() ),
  racials( racials_t() ),
  active_during_iteration( false ),
  _mastery( spelleffect_data_t::nil() ),
  cache( this )
{
  actor_index = sim -> actor_list.size();
  sim -> actor_list.push_back( this );

  base.skill = sim -> default_skill;
  base.mastery = 8.0;

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
    index = - ( ++( sim -> num_enemies ) );
  }

  if ( ! is_pet() && sim -> stat_cache != -1 )
  {
    cache.active = sim -> stat_cache != 0;
  }
  if ( is_pet() ) current.skill = 1.0;

  resources.infinite_resource[ RESOURCE_HEALTH ] = true;

  range::fill( resource_lost, 0 );
  range::fill( resource_gained, 0 );

  range::fill( profession, 0 );

  range::fill( scales_with, false );
  range::fill( over_cap, 0 );

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
  spell_crit(),
  attack_crit(),
  block_reduction(),
  mastery(),
  skill( 1.0 ),
  distance( 0 ),
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
  s << " spell_crit=" << spell_crit;
  s << " attack_crit=" << attack_crit;
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
// player_t::~player_t ======================================================

player_t::~player_t()
{
  delete sets;
}

static bool check_actors( sim_t* sim )
{
  bool too_quiet = true; // Check for at least 1 active player
  bool zero_dds = true; // Check for at least 1 player != TANK/HEAL

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[ i ];
    if ( ! p -> quiet ) too_quiet = false;
    if ( p -> primary_role() != ROLE_HEAL && p -> primary_role() != ROLE_TANK && ! p -> is_pet() ) zero_dds = false;
  }

  if ( too_quiet && ! sim -> debug )
  {
    sim -> errorf( "No active players in sim!" );
    return false;
  }

  // Set Fixed_Time when there are no DD's present
  if ( zero_dds && ! sim -> debug )
    sim -> fixed_time = true;

  return true;
}

// init_debuffs =============================================================

static bool init_debuffs( sim_t* sim )
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing Auras, Buffs, and De-Buffs." );

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    // MOP Debuffs
    p -> debuffs.slowed_casting           = buff_creator_t( p, "slowed_casting", p -> find_spell( 73975 ) )
                                            .default_value( std::fabs( p -> find_spell( 73975 ) -> effectN( 3 ).percent() ) );

    p -> debuffs.magic_vulnerability     = buff_creator_t( p, "magic_vulnerability", p -> find_spell( 104225 ) )
                                           .default_value( p -> find_spell( 104225 ) -> effectN( 1 ).percent() );

    p -> debuffs.physical_vulnerability  = buff_creator_t( p, "physical_vulnerability", p -> find_spell( 81326 ) )
                                           .default_value( p -> find_spell( 81326 ) -> effectN( 1 ).percent() );

    p -> debuffs.ranged_vulnerability    = buff_creator_t( p, "ranged_vulnerability", p -> find_spell( 1130 ) )
                                           .default_value( p -> find_spell( 1130 ) -> effectN( 2 ).percent() );

    p -> debuffs.mortal_wounds           = buff_creator_t( p, "mortal_wounds", p -> find_spell( 115804 ) )
                                           .default_value( std::fabs( p -> find_spell( 115804 ) -> effectN( 1 ).percent() ) );

    p -> debuffs.weakened_armor          = buff_creator_t( p, "weakened_armor", p -> find_spell( 113746 ) )
                                           .default_value( std::fabs( p -> find_spell( 113746 ) -> effectN( 1 ).percent() ) )
                                           .add_invalidate( CACHE_ARMOR );

    p -> debuffs.weakened_blows          = buff_creator_t( p, "weakened_blows", p -> find_spell( 115798 ) )
                                           .default_value( std::fabs( p -> find_spell( 115798 ) -> effectN( 1 ).percent() ) )
                                           .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  return true;
}

// init_parties =============================================================

static bool init_parties( sim_t* sim )
{
  // Parties
  if ( sim -> debug )
    sim -> out_debug.printf( "Building Parties." );

  int party_index = 0;
  for ( size_t i = 0; i < sim -> party_encoding.size(); i++ )
  {
    std::string& party_str = sim -> party_encoding[ i ];

    if ( party_str == "reset" )
    {
      party_index = 0;
      for ( size_t i = 0; i < sim -> player_list.size(); ++i )
        sim -> player_list[ i ] -> party = 0;
    }
    else if ( party_str == "all" )
    {
      int member_index = 0;
      for ( size_t i = 0; i < sim -> player_list.size(); ++i )
      {
        player_t* p = sim -> player_list[ i ];
        p -> party = 1;
        p -> member = member_index++;
      }
    }
    else
    {
      party_index++;

      std::vector<std::string> player_names = util::string_split( party_str, ",;/" );
      int member_index = 0;

      for ( size_t j = 0; j < player_names.size(); j++ )
      {
        player_t* p = sim -> find_player( player_names[ j ] );
        if ( ! p )
        {
          sim -> errorf( "Unable to find player %s for party creation.\n", player_names[ j ].c_str() );
          return false;
        }
        p -> party = party_index;
        p -> member = member_index++;
        for ( size_t i = 0; i < p -> pet_list.size(); ++i )
        {
          pet_t* pet = p -> pet_list[ i ];
          pet -> party = party_index;
          pet -> member = member_index++;
        }
      }
    }
  }

  return true;
}

// player_t::init ===========================================================

bool player_t::init( sim_t* sim )
{
  // FIXME! This should probably move to sc_sim.cpp
  // Having two versions of player_t::init is confusing.

  if ( sim -> debug )
    sim -> out_debug.printf( "Creating Pets." );

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> create_pets();
  }

  if ( ! init_debuffs( sim ) )
    return false;

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    const module_t* m = module_t::get( i );
    if ( m ) m -> init( sim );
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing Players." );

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[ i ];
    if ( sim -> default_actions && ! p -> is_pet() )
    {
      p -> clear_action_priority_lists();
      p -> action_list_str.clear();
    };
    p -> init();
    p -> initialized = 1;
  }


  // Determine Spec, Talents, Professions, Glyphs
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_target ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_character_properties ) );

  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_items ) );

  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_spells ) );

  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_base_stats ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_initial_stats ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_defense ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::create_buffs ) ); // keep here for now
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_enchant ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_scaling ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_unique_gear ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::_init_actions ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_gains ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_procs ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_uptimes ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_benefits ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_rng ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_stats ) );

  if ( ! check_actors( sim ) )
    return false;

  if ( ! init_parties( sim ) )
    return false;

  // Callbacks
  if ( sim -> debug )
    sim -> out_debug.printf( "Registering Callbacks." );

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[ i ];
    p -> register_callbacks();
  }

  return true;
}

// player_t::init ===========================================================

void player_t::init()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Initializing player %s", name() );

  // Ensure the precombat and default lists are the first listed
  get_action_priority_list( "precombat", "Executed before combat begins. Accepts non-harmful actions only." ) -> used = true;
  get_action_priority_list( "default", "Executed every time the actor is available." );

  for ( std::map<std::string, std::string>::iterator it = alist_map.begin(), end = alist_map.end(); it != end; ++it )
  {
    if ( it -> first == "default" )
      sim -> errorf( "Ignoring action list named default." );
    else
      get_action_priority_list( it -> first ) -> action_list_str = it -> second;
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
}

void scale_challenge_mode( player_t& p, const rating_t& rating )
{

  if ( p.sim -> scale_to_itemlevel != -1 && ! p.is_enemy() && &p != p.sim -> heal_target ) //scale p.gear to itemlevel 463
  {
    int old_rating_sum = 0;

    old_rating_sum += ( int ) p.gear.get_stat( STAT_SPIRIT );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_EXPERTISE_RATING );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_HIT_RATING );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_CRIT_RATING );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_HASTE_RATING );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_DODGE_RATING );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_PARRY_RATING );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_BLOCK_RATING );
    old_rating_sum += ( int ) p.gear.get_stat( STAT_MASTERY_RATING );

    int old_rating_sum_wo_hit_exp = ( int ) ( old_rating_sum - p.gear.get_stat( STAT_EXPERTISE_RATING ) - p.gear.get_stat( STAT_HIT_RATING ) );

    int target_rating_sum_wo_hit_exp = old_rating_sum;

    switch ( p.specialization() )
    {
      case WARRIOR_ARMS:
      case WARRIOR_FURY:
      case WARRIOR_PROTECTION:
      case PALADIN_PROTECTION:
      case PALADIN_RETRIBUTION:
      case HUNTER_BEAST_MASTERY:
      case HUNTER_MARKSMANSHIP:
      case HUNTER_SURVIVAL:
      case ROGUE_ASSASSINATION:
      case ROGUE_COMBAT:
      case ROGUE_SUBTLETY:
      case DEATH_KNIGHT_BLOOD:
      case DEATH_KNIGHT_FROST:
      case DEATH_KNIGHT_UNHOLY:
      case SHAMAN_ENHANCEMENT:
      case MONK_BREWMASTER:
      case MONK_WINDWALKER:
      case DRUID_FERAL:
      case DRUID_GUARDIAN: //melee or tank will be set to 7.5/7.5. Tanks also exp to 7.5 as they might not be able to reach exp hard cap
        p.gear.set_stat( STAT_HIT_RATING, 0.075 * rating.attack_hit );
        p.gear.set_stat( STAT_EXPERTISE_RATING, 0.075 * rating.expertise );
        target_rating_sum_wo_hit_exp = ( int ) ( old_rating_sum - 0.075 * rating.attack_hit - 0.075 * rating.expertise );
        break;
      case PRIEST_SHADOW:
      case SHAMAN_ELEMENTAL:
      case MAGE_ARCANE:
      case MAGE_FIRE:
      case MAGE_FROST:
      case WARLOCK_AFFLICTION:
      case WARLOCK_DEMONOLOGY:
      case WARLOCK_DESTRUCTION:
      case DRUID_BALANCE: //caster are set to 15% spell hit
        p.gear.set_stat( STAT_HIT_RATING, 0.15 * rating.spell_hit );
        target_rating_sum_wo_hit_exp = ( int ) ( old_rating_sum - 0.15 * rating.spell_hit );

      default:
        break;
    }

    // every secondary stat but hit/exp gets a share of the target_rating_sum according to its previous ratio (without hit/exp)

    p.gear.set_stat( STAT_SPIRIT, floor( p.gear.get_stat( STAT_SPIRIT ) / old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    p.gear.set_stat( STAT_CRIT_RATING, floor( p.gear.get_stat( STAT_CRIT_RATING ) / old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    p.gear.set_stat( STAT_HASTE_RATING, floor( p.gear.get_stat( STAT_HASTE_RATING ) / old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    p.gear.set_stat( STAT_DODGE_RATING, floor( p.gear.get_stat( STAT_DODGE_RATING ) / old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    p.gear.set_stat( STAT_PARRY_RATING, floor( p.gear.get_stat( STAT_PARRY_RATING ) / old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    p.gear.set_stat( STAT_BLOCK_RATING, floor( p.gear.get_stat( STAT_BLOCK_RATING ) / old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    p.gear.set_stat( STAT_MASTERY_RATING, floor( p.gear.get_stat( STAT_MASTERY_RATING ) / old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
  }

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
    base.rating.init( dbc, level );

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Base Ratings initialized: %s", name(), base.rating.to_string().c_str() );

  scale_challenge_mode( *this, base.rating ); // needs initialzied base.rating

  total_gear = gear + enchant;
  if (  ! is_pet() ) total_gear += sim -> enchant;
  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Total Gear Stats: %s", name(), total_gear.to_string().c_str() );

  if ( ! is_enemy() )
  {
    base.stats.attribute[ STAT_STRENGTH ]  = dbc.race_base( race ).strength + dbc.attribute_base( type, level ).strength;
    base.stats.attribute[ STAT_AGILITY ]   = dbc.race_base( race ).agility + dbc.attribute_base( type, level ).agility;
    base.stats.attribute[ STAT_STAMINA ]   = dbc.race_base( race ).stamina + dbc.attribute_base( type, level ).stamina;
    base.stats.attribute[ STAT_INTELLECT ] = dbc.race_base( race ).intellect + dbc.attribute_base( type, level ).intellect;
    base.stats.attribute[ STAT_SPIRIT ]    = dbc.race_base( race ).spirit + dbc.attribute_base( type, level ).spirit;
    if ( race == RACE_HUMAN ) base.stats.attribute[ STAT_SPIRIT ] *= 1.03;

    base.spell_crit               = dbc.spell_crit_base( type, level );
    base.attack_crit              = dbc.melee_crit_base( type, level );
    base.spell_crit_per_intellect = dbc.spell_crit_scaling( type, level );
    base.attack_crit_per_agility  = dbc.melee_crit_scaling( type, level );
    base.mastery = 8.0;

    resources.base[ RESOURCE_HEALTH ] = dbc.health_base( type, level );
    if ( race == RACE_TAUREN )
      resources.base[ RESOURCE_HEALTH ] *= 1.0 + find_spell( 20550 ) -> effectN( 1 ).percent();
    resources.base[ RESOURCE_MANA   ] = dbc.resource_base( type, level );

    base.mana_regen_per_second = dbc.regen_base( type, level ) / 5.0;
    base.mana_regen_per_spirit = dbc.regen_spirit( type, level );
    base.health_per_stamina    = dbc.health_per_stamina( level );
    base.dodge_per_agility     = 1 / 10000.0 / 100.0; // default at L90, modified for druid/monk in class module
    base.parry_per_strength    = 0.0;                 // only certain classes get STR->parry conversions, handle in class module
  }

  base.spell_power_multiplier    = 1.0;
  base.attack_power_multiplier   = 1.0;

  if ( ( meta_gem == META_EMBER_PRIMAL ) || ( meta_gem == META_EMBER_SHADOWSPIRIT ) || ( meta_gem == META_EMBER_SKYFIRE ) || ( meta_gem == META_EMBER_SKYFLARE ) )
  {
    resources.base_multiplier[ RESOURCE_MANA ] *= 1.02;
  }
  if ( race == RACE_GNOME )
  {
    resources.base_multiplier[ RESOURCE_MANA ] *= 1.05;
  }

  if ( level >= 50 && matching_gear )
  {
    for ( attribute_e a = ATTR_STRENGTH; a <= ATTR_SPIRIT; a++ )
    {
      base.stats.attribute[ a ] *= 1.0 + matching_gear_multiplier( a );
      base.stats.attribute[ a ] = util::floor( base.stats.attribute[ a ] );
    }
  }

  if ( world_lag_stddev < timespan_t::zero() ) world_lag_stddev = world_lag * 0.1;
  if ( brain_lag_stddev < timespan_t::zero() ) brain_lag_stddev = brain_lag * 0.1;

  if ( primary_role() == ROLE_TANK )
  {
    // set position to front
    base.position = POSITION_FRONT;
    initial.position = POSITION_FRONT;
    change_position( POSITION_FRONT );

    // Collect DTPS data for tanks even for statistics_level == 1
    if ( sim -> statistics_level >= 1  )
      collected_data.dtps.change_mode( false );
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Generic Base Stats: %s", name(), base.to_string().c_str() );

}

double stat_gain_mining( int level )
{
  if      ( level >= 600 ) return 480.0;
  else if ( level >= 525 ) return 120.0;
  else if ( level >= 450 ) return  60.0;
  else if ( level >= 375 ) return  30.0;
  else if ( level >= 300 ) return  10.0;
  else if ( level >= 225 ) return   7.0;
  else if ( level >= 150 ) return   5.0;
  else if ( level >=  75 ) return   3.0;
  return 0.0;
}

double stat_gain_skinning( int level )
{
  if      ( level >= 600 ) return 480.0;
  else if ( level >= 525 ) return  80.0;
  else if ( level >= 450 ) return  40.0;
  else if ( level >= 375 ) return  20.0;
  else if ( level >= 300 ) return  12.0;
  else if ( level >= 225 ) return   9.0;
  else if ( level >= 150 ) return   6.0;
  else if ( level >=  75 ) return   3.0;
  return 0.0;
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
  initial.stats += total_gear;
  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Generic Initial Stats: %s", name(), initial.to_string().c_str() );



  // Profession bonus

  // Miners gain additional stamina
  initial.stats.attribute[ ATTR_STAMINA ] += stat_gain_mining( profession[ PROF_MINING ] );

  // Skinners gain additional crit rating
  initial.stats.crit_rating += stat_gain_skinning( profession[ PROF_SKINNING ] );
}
// player_t::init_items =====================================================

void player_t::init_items()
{
  if ( is_pet() ) return;

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

  unsigned num_ilvl_items = 0;
  // Accumulator for the stats given by items. Stats from items cannot directly
  // be added to ``gear'', as it would cause stat values specified on the
  // command line (with options such as gear_intellect=x) to be added to the
  // values given by items (instead of overriding them).
  // After the loop (and the meta-gem initialization), the stats accumulated
  // from items are added into ``gear'', provided that there is no
  // command line option override.
  gear_stats_t item_stats;
  for ( size_t i = 0; i < items.size(); i++ )
  {
    item_t& item = items[ i ];

    // If the item has been specified in options we want to start from scratch, forgetting about lingering stuff from profile copy
    if ( ! item.options_str.empty() )
    {
      item = item_t( this, item.options_str );
      item.slot = static_cast<slot_e>( i );
    }

    if ( ! item.init() )
    {
      sim -> errorf( "Unable to initialize item '%s' on player '%s'\n", item.name(), name() );
      sim -> cancel();
      return;
    }

    if ( ! item.is_valid_type() )
    {
      sim -> errorf( "Item '%s' on player '%s' is of invalid type\n", item.name(), name() );
      sim -> cancel();
      return;
    }

    if ( item.slot != SLOT_SHIRT && item.slot != SLOT_TABARD && item.slot != SLOT_RANGED && item.active() )
    {
      avg_ilvl += item.item_level();
      num_ilvl_items++;
    }

    slots[ item.slot ] = item.is_matching_type();

    item_stats += item.stats;

  }

  if ( num_ilvl_items > 1 )
    avg_ilvl /= num_ilvl_items;

  switch ( type )
  {
    case MAGE:
    case PRIEST:
    case WARLOCK:
      matching_gear = true;
      break;
    default:
      matching_gear = true;
      for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
      {
        if ( ! slots[ i ] )
        {
          matching_gear = false;
          break;
        }
      }
      break;
  }

  init_meta_gem( item_stats );

  // Adding stats from items into ``gear''. If for a given stat,
  // the value in gear is different than 0, it means that this stat
  // value was overridden by a command line option.
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( gear.get_stat( i ) == 0 )
      gear.add_stat( i, item_stats.get_stat( i ) );
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s gear: %s", name(), gear.to_string().c_str() );
  }

  set_bonus.init( this );

  init_weapon ( main_hand_weapon );
  init_weapon( off_hand_weapon );
}

// player_t::init_meta_gem ==================================================

void player_t::init_meta_gem( gear_stats_t& item_stats )
{
  if ( ! meta_gem_str.empty() ) meta_gem = util::parse_meta_gem_type( meta_gem_str );

  if ( sim -> debug ) sim -> out_debug.printf( "Initializing meta-gem for player (%s)", name() );

  if      ( meta_gem == META_AGILE_SHADOWSPIRIT         ) item_stats.attribute[ ATTR_AGILITY ] += 54;
  else if ( meta_gem == META_AGILE_PRIMAL               ) item_stats.attribute[ ATTR_AGILITY ] += 216;
  else if ( meta_gem == META_AUSTERE_EARTHSIEGE         ) item_stats.attribute[ ATTR_STAMINA ] += 32;
  else if ( meta_gem == META_AUSTERE_SHADOWSPIRIT       ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_AUSTERE_PRIMAL             ) item_stats.attribute[ ATTR_STAMINA ] += 324;
  else if ( meta_gem == META_BEAMING_EARTHSIEGE         ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_BRACING_EARTHSIEGE         ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_BRACING_EARTHSTORM         ) item_stats.attribute[ ATTR_INTELLECT ] += 12;
  else if ( meta_gem == META_BRACING_SHADOWSPIRIT       ) item_stats.attribute[ ATTR_INTELLECT ] += 54;
  else if ( meta_gem == META_BURNING_SHADOWSPIRIT       ) item_stats.attribute[ ATTR_INTELLECT ] += 54;
  else if ( meta_gem == META_BURNING_PRIMAL             ) item_stats.attribute[ ATTR_INTELLECT ] += 216;
  else if ( meta_gem == META_CHAOTIC_SHADOWSPIRIT       ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_CHAOTIC_SKYFIRE            ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_CHAOTIC_SKYFLARE           ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_DESTRUCTIVE_SHADOWSPIRIT   ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_DESTRUCTIVE_PRIMAL         ) item_stats.crit_rating += 432;
  else if ( meta_gem == META_DESTRUCTIVE_SKYFIRE        ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_DESTRUCTIVE_SKYFLARE       ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_EFFULGENT_SHADOWSPIRIT     ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_EMBER_SHADOWSPIRIT         ) item_stats.attribute[ ATTR_INTELLECT ] += 54;
  else if ( meta_gem == META_EMBER_PRIMAL               ) item_stats.attribute[ ATTR_INTELLECT ] += 216;
  else if ( meta_gem == META_EMBER_SKYFIRE              ) item_stats.attribute[ ATTR_INTELLECT ] += 12;
  else if ( meta_gem == META_EMBER_SKYFLARE             ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_ENIGMATIC_SHADOWSPIRIT     ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_ENIGMATIC_PRIMAL           ) item_stats.crit_rating += 432;
  else if ( meta_gem == META_ENIGMATIC_SKYFLARE         ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_ENIGMATIC_STARFLARE        ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_ENIGMATIC_SKYFIRE          ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_ETERNAL_EARTHSIEGE         ) item_stats.dodge_rating += 21;
  else if ( meta_gem == META_ETERNAL_SHADOWSPIRIT       ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_ETERNAL_PRIMAL             ) item_stats.dodge_rating += 432;
  else if ( meta_gem == META_FLEET_SHADOWSPIRIT         ) item_stats.mastery_rating += 54;
  else if ( meta_gem == META_FLEET_PRIMAL               ) item_stats.mastery_rating += 432;
  else if ( meta_gem == META_FORLORN_SHADOWSPIRIT       ) item_stats.attribute[ ATTR_INTELLECT ] += 54;
  else if ( meta_gem == META_FORLORN_PRIMAL             ) item_stats.attribute[ ATTR_INTELLECT ] += 216;
  else if ( meta_gem == META_FORLORN_SKYFLARE           ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_FORLORN_STARFLARE          ) item_stats.attribute[ ATTR_INTELLECT ] += 17;
  else if ( meta_gem == META_IMPASSIVE_SHADOWSPIRIT     ) item_stats.crit_rating += 54;
  else if ( meta_gem == META_IMPASSIVE_PRIMAL           ) item_stats.crit_rating += 432;
  else if ( meta_gem == META_IMPASSIVE_SKYFLARE         ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_IMPASSIVE_STARFLARE        ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_INSIGHTFUL_EARTHSIEGE      ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_INSIGHTFUL_EARTHSTORM      ) item_stats.attribute[ ATTR_INTELLECT ] += 12;
  else if ( meta_gem == META_INVIGORATING_EARTHSIEGE    ) item_stats.haste_rating += 21;
  else if ( meta_gem == META_PERSISTENT_EARTHSHATTER    ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_PERSISTENT_EARTHSIEGE      ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_POWERFUL_EARTHSHATTER      ) item_stats.attribute[ ATTR_STAMINA ] += 26;
  else if ( meta_gem == META_POWERFUL_EARTHSIEGE        ) item_stats.attribute[ ATTR_STAMINA ] += 32;
  else if ( meta_gem == META_POWERFUL_EARTHSTORM        ) item_stats.attribute[ ATTR_STAMINA ] += 18;
  else if ( meta_gem == META_POWERFUL_SHADOWSPIRIT      ) item_stats.attribute[ ATTR_STAMINA ] += 81;
  else if ( meta_gem == META_POWERFUL_PRIMAL            ) item_stats.attribute[ ATTR_STAMINA ] += 324;
  else if ( meta_gem == META_RELENTLESS_EARTHSIEGE      ) item_stats.attribute[ ATTR_AGILITY ] += 21;
  else if ( meta_gem == META_RELENTLESS_EARTHSTORM      ) item_stats.attribute[ ATTR_AGILITY ] += 12;
  else if ( meta_gem == META_REVERBERATING_SHADOWSPIRIT ) item_stats.attribute[ ATTR_STRENGTH ] += 54;
  else if ( meta_gem == META_REVERBERATING_PRIMAL       ) item_stats.attribute[ ATTR_STRENGTH ] += 216;
  else if ( meta_gem == META_REVITALIZING_SHADOWSPIRIT  ) item_stats.attribute[ ATTR_SPIRIT ] += 54;
  else if ( meta_gem == META_REVITALIZING_PRIMAL        ) item_stats.attribute[ ATTR_SPIRIT ] += 432;
  else if ( meta_gem == META_REVITALIZING_SKYFLARE      ) item_stats.attribute[ ATTR_SPIRIT ] += 22;
  else if ( meta_gem == META_SWIFT_SKYFIRE              ) item_stats.crit_rating += 12;
  else if ( meta_gem == META_SWIFT_SKYFLARE             ) item_stats.crit_rating += 21;
  else if ( meta_gem == META_SWIFT_STARFLARE            ) item_stats.crit_rating += 17;
  else if ( meta_gem == META_TIRELESS_STARFLARE         ) item_stats.attribute[ ATTR_INTELLECT ] += 17;
  else if ( meta_gem == META_TIRELESS_SKYFLARE          ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_TRENCHANT_EARTHSHATTER     ) item_stats.attribute[ ATTR_INTELLECT ] += 17;
  else if ( meta_gem == META_TRENCHANT_EARTHSIEGE       ) item_stats.attribute[ ATTR_INTELLECT ] += 21;
  else if ( meta_gem == META_CAPACITIVE_PRIMAL          ) item_stats.crit_rating += 324;
  else if ( meta_gem == META_INDOMITABLE_PRIMAL         ) item_stats.attribute[ ATTR_STAMINA ] += 324;
  else if ( meta_gem == META_COURAGEOUS_PRIMAL          ) item_stats.attribute[ ATTR_INTELLECT ] += 324;
  else if ( meta_gem == META_SINISTER_PRIMAL            ) item_stats.crit_rating += 324;

  if ( ( meta_gem == META_AUSTERE_EARTHSIEGE ) || ( meta_gem == META_AUSTERE_SHADOWSPIRIT ) )
  {
    initial.armor_multiplier *= 1.02;
  }
  /*
  else if ( ( meta_gem == META_EMBER_SHADOWSPIRIT ) || ( meta_gem == META_EMBER_SKYFIRE ) || ( meta_gem == META_EMBER_SKYFLARE ) )
  {
    mana_per_intellect *= 1.02;
  }
  else if ( meta_gem == META_BEAMING_EARTHSIEGE )
  {
    mana_per_intellect *= 1.02;
  }
  */
  else if ( meta_gem == META_MYSTICAL_SKYFIRE )
  {
    special_effect_t data;
    data.name_str     = "mystical_skyfire";
    data.trigger_type = PROC_SPELL;
    data.trigger_mask = RESULT_HIT_MASK;
    data.stat         = STAT_HASTE_RATING;
    data.stat_amount  = 320;
    data.proc_chance  = 0.15;
    data.duration     = timespan_t::from_seconds( 4 );
    data.cooldown     = timespan_t::from_seconds( 45 );

    unique_gear::register_stat_proc( this, data );
  }
  else if ( meta_gem == META_INSIGHTFUL_EARTHSTORM )
  {
    special_effect_t data;
    data.name_str     = "insightful_earthstorm";
    data.trigger_type = PROC_SPELL;
    data.trigger_mask = RESULT_HIT_MASK;
    data.stat         = STAT_MANA;
    data.stat_amount  = 300;
    data.proc_chance  = 0.05;
    data.cooldown     = timespan_t::from_seconds( 15 );

    unique_gear::register_stat_proc( this, data );
  }
  else if ( meta_gem == META_INSIGHTFUL_EARTHSIEGE )
  {
    special_effect_t data;
    data.name_str     = "insightful_earthsiege";
    data.trigger_type = PROC_SPELL;
    data.trigger_mask = RESULT_HIT_MASK;
    data.stat         = STAT_MANA;
    data.stat_amount  = 600;
    data.proc_chance  = 0.05;
    data.cooldown     = timespan_t::from_seconds( 15 );

    unique_gear::register_stat_proc( this, data );
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
  }

  // default to back when we have an invalid position
  if ( base.position == POSITION_NONE )
  {
    base.position = POSITION_BACK;
  }

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
    if ( race == RACE_NONE )
    {
      sim -> errorf( "%s has unknown race string specified", name() );
      race_str = util::race_type_string( race );
    }
  }
}

// player_t::weapon_racial ==================================================

bool player_t::weapon_racial( weapon_t* weapon )
{
  if ( ! weapon )
    return false;

  switch ( race )
  {
    case RACE_ORC:
    {
      switch ( weapon -> type )
      {
        case WEAPON_AXE:
        case WEAPON_AXE_2H:
        case WEAPON_FIST:
        case WEAPON_NONE:
          return true;
        default:;
      }
      break;
    }
    case RACE_TROLL:
    {
      if ( WEAPON_2H < weapon -> type && weapon -> type <= WEAPON_RANGED )
        return true;
      break;
    }
    case RACE_HUMAN:
    {
      switch ( weapon -> type )
      {
        case WEAPON_MACE:
        case WEAPON_MACE_2H:
        case WEAPON_SWORD:
        case WEAPON_SWORD_2H:
          return true;
        default:;
      }
      break;
    }
    case RACE_DWARF:
    {
      switch ( weapon -> type )
      {
        case WEAPON_MACE:
        case WEAPON_MACE_2H:
        case WEAPON_BOW:
        case WEAPON_CROSSBOW:
        case WEAPON_GUN:
        case WEAPON_WAND:
        case WEAPON_THROWN:
        case WEAPON_RANGED:
          return true;
        default:;
      }
      break;
    }
    case RACE_GNOME:
    {
      switch ( weapon -> type )
      {
        case WEAPON_DAGGER:
        case WEAPON_SWORD:
          return true;
        default:;
      }
      break;
    }
    default:;
  }

  return false;
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

  // Armor Coefficient
  double a, b;
  if ( level > 85 )
  {
    a = 4037.5;
    b = -317117.5;
  }
  else if ( level > 80 )
  {
    a = 2167.5;
    b = -158167.5;
  }
  else if ( level >= 60 )
  {
    a = 467.5;
    b = -22167.5;
  }
  else
  {
    a = 85.0;
    b = 400.0;
  }
  initial.armor_coeff = a * level + b;
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

// player_t::init_unique_gear ===============================================

void player_t::init_unique_gear()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Initializing unique gear for player (%s)", name() );

  unique_gear::init( this );
}

// player_t::init_enchant ===================================================

void player_t::init_enchant()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Initializing enchants for player (%s)", name() );

  unique_gear::initialize_special_effects( this );
}

// player_t::init_resources =================================================

void player_t::init_resources( bool force )
{
  if ( sim -> debug ) sim -> out_debug.printf( "Initializing resources for player (%s)", name() );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( force || resources.initial[ i ] == 0 )
    {
      resources.initial[ i ] = (   resources.base[ i ] * resources.base_multiplier[ i ]
                                   + gear.resource[ i ] + enchant.resource[ i ]
                                   + ( is_pet() ? 0 : sim -> enchant.resource[ i ] )
                               ) * resources.initial_multiplier[ i ];
      if ( i == RESOURCE_HEALTH )
      {
        // The first 20pts of stamina only provide 1pt of health.
        double adjust = ( is_pet() || is_enemy() || is_add() ) ? 0 : std::min( 20, static_cast<int>( floor( stamina() ) ) );
        resources.initial[ i ] += ( floor( stamina() ) - adjust ) * current.health_per_stamina + adjust;
      }
    }
  }


  resources.current = resources.max = resources.initial;
  if ( type == WARRIOR )
    resources.current [RESOURCE_RAGE] = 0; //Warriors do not have full resource bars pre-combat.

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
      prof_value = level > 85 ? 600 : 525;
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

  execute_pet_action_t( player_t* player, pet_t* p, const std::string& as, const std::string& options_str ) :
    action_t( ACTION_OTHER, "execute_" + p -> name_str + "_" + as, player ),
    pet_action( 0 ), pet( p ), action_str( as )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero();
  }

  virtual void reset()
  {
    action_t::reset();

    if ( sim -> current_iteration == 0 )
    {
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
      }
    }
  }

  virtual void execute()
  {
    pet_action -> execute();
  }

  virtual bool ready()
  {
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
    if ( items[ i ].parsed.use.active() )
    {
      buffer += "/use_item,slot=";
      buffer += items[ i ].slot_name();
      if ( ! append.empty() )
      {
        buffer += append;
      }
    }
  }
  if ( items[ SLOT_HANDS ].parsed.use.active() )
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

std::vector<std::string> player_t::get_item_actions()
{
  std::vector<std::string> actions;

  for ( size_t i = 0; i < items.size(); i++ )
  {
    // Make sure hands slot comes last
    if ( items[ i ].slot == SLOT_HANDS ) continue;
    if ( items[ i ].parsed.use.active() )
      actions.push_back( std::string( "use_item,slot=" ) + items[ i ].slot_name() );
  }

  if ( items[ SLOT_HANDS ].parsed.use.active() )
    actions.push_back( std::string( "use_item,slot=" ) + items[ SLOT_HANDS ].slot_name() );

  return actions;
}

// player_t::init_use_profession_actions ====================================

std::string player_t::init_use_profession_actions( const std::string& append )
{
  std::string buffer;

  // Lifeblood
  if ( profession[ PROF_HERBALISM ] >= 1 )
  {
    buffer += "/lifeblood";

    if ( ! append.empty() )
    {
      buffer += append;
    }
  }

  return buffer;
}

std::vector<std::string> player_t::get_profession_actions()
{
  std::vector<std::string> actions;

  if ( profession[ PROF_HERBALISM ] >= 1 )
    actions.push_back( "lifeblood" );

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

std::string player_t::include_default_on_use_items( player_t& p, const std::string& exclude_effects )
{
  std::string s;

  // Usable Item
  for ( size_t i = 0; i < p.items.size(); i++ )
  {
    item_t& item = p.items[ i ];
    if ( item.parsed.use.active() )
    {
      if ( ! item.parsed.use.name_str.empty() && exclude_effects.find( item.parsed.use.name_str ) != std::string::npos )
        continue;
      s += "/use_item,name=";
      s += item.name();
    }
  }

  return s;
}

/* Adds a specific on use item with effects contained in effect_names ( split by ',' ),
 * when it is present on the players gear.
 */

std::string player_t::include_specific_on_use_item( player_t& p, const std::string& effect_names, const std::string& options )
{
  std::string s;

  std::vector<std::string> splits = util::string_split( effect_names, "," );

  // Usable Item
  for ( size_t i = 0; i < p.items.size(); i++ )
  {
    item_t& item = p.items[ i ];
    if ( item.parsed.use.active() )
    {
      for ( size_t j = 0; j < splits.size(); ++j )
      {
        if ( splits[ j ] == item.parsed.use.name_str )
        {
          s += "/use_item,name=";
          s += item.name();
          s += options;
          break;
        }
      }
    }
  }

  return s;
}

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

void player_t::override_talent( std::string override_str )
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

  unsigned spell_id = dbc.talent_ability_id( type, override_str.c_str(), true );

  if ( ! spell_id || dbc.spell( spell_id ) ->id() != spell_id )
  {
    sim -> errorf( "Override talent %s not found for player %s.\n", override_str.c_str(), name() );
    return;
  }

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_data_t* td = talent_data_t::find( type, j, i, dbc.ptr );
      if ( td && ( td -> spell_id() == spell_id ) )
      {
        if ( level < ( j + 1 ) * 15 )
        {
          sim -> errorf( "Override talent %s is too high level for player %s.\n", override_str.c_str(), name() );
          return;
        }
        if ( sim -> debug )
        {
          if ( talent_points.has_row_col( j, i ) )
          {
            sim -> out_debug.printf( "talent_override: talent %s for player %s is already enabled\n",
                           override_str.c_str(), name() );
          }
          else
          {
            sim -> out_debug.printf( "talent_override: talent %s for player %s replaced talent %d in tier %d\n",
                           override_str.c_str(), name(), talent_points.choice( j ) + 1, j + 1 );
          }
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


// player_t::init_glyphs ====================================================

void player_t::init_glyphs()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing glyphs for player (%s)", name() );

  std::vector<std::string> glyph_names = util::string_split( glyphs_str, ",/" );

  for ( size_t i = 0; i < glyph_names.size(); i++ )
  {
    const spell_data_t* g = find_glyph( glyph_names[ i ] );
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

  racials.quickness = find_racial_spell( "Quickness" );
  racials.command   = find_racial_spell( "Command" );

  if ( ! is_enemy() )
  {
    const spell_data_t* s = find_mastery_spell( specialization() );
    if ( s -> ok() )
      _mastery = &( s -> effectN( 1 ) );
  }
}

// player_t::init_gains =====================================================

void player_t::init_gains()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing gains for player (%s)", name() );

  gains.arcane_torrent         = get_gain( "arcane_torrent" );
  gains.blessing_of_might      = get_gain( "blessing_of_might" );
  gains.dark_rune              = get_gain( "dark_rune" );
  gains.endurance_of_niuzao    = get_gain( "endurance_of_niuzao" );
  gains.energy_regen           = get_gain( "energy_regen" );
  gains.essence_of_the_red     = get_gain( "essence_of_the_red" );
  gains.focus_regen            = get_gain( "focus_regen" );
  gains.health                 = get_gain( "external_healing" );
  gains.hymn_of_hope           = get_gain( "hymn_of_hope_max_mana" );
  gains.innervate              = get_gain( "innervate" );
  gains.mana_potion            = get_gain( "mana_potion" );
  gains.mana_spring_totem      = get_gain( "mana_spring_totem" );
  gains.mp5_regen              = get_gain( "mp5_regen" );
  gains.restore_mana           = get_gain( "restore_mana" );
  gains.spellsurge             = get_gain( "spellsurge" );
  gains.touch_of_the_grave     = get_gain( "touch_of_the_grave" );
  gains.vampiric_embrace       = get_gain( "vampiric_embrace" );
  gains.vampiric_touch         = get_gain( "vampiric_touch" );
  gains.water_elemental        = get_gain( "water_elemental" );
}

// player_t::init_procs =====================================================

void player_t::init_procs()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing procs for player (%s)", name() );

  procs.hat_donor    = get_proc( "hat_donor"   );
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

  range::fill( resource_lost, 0.0 );
  range::fill( resource_gained, 0.0 );

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

// player_t::init_scaling ===================================================

void player_t::init_scaling()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Initializing scaling for player (%s)", name() );

  if ( ! is_pet() && ! is_enemy() )
  {
    invert_scaling = 0;

    role_e role = primary_role();

    bool attack = ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK );
    bool spell  = ( role == ROLE_SPELL  || role == ROLE_HYBRID || role == ROLE_HEAL );
    bool tank   =   role == ROLE_TANK;

    scales_with[ STAT_STRENGTH  ] = attack;
    scales_with[ STAT_AGILITY   ] = attack;
    scales_with[ STAT_STAMINA   ] = tank;
    scales_with[ STAT_INTELLECT ] = spell;
    scales_with[ STAT_SPIRIT    ] = spell;

    scales_with[ STAT_HEALTH ] = false;
    scales_with[ STAT_MANA   ] = false;
    scales_with[ STAT_RAGE   ] = false;
    scales_with[ STAT_ENERGY ] = false;
    scales_with[ STAT_FOCUS  ] = false;
    scales_with[ STAT_RUNIC  ] = false;

    scales_with[ STAT_SPELL_POWER       ] = spell;

    scales_with[ STAT_ATTACK_POWER             ] = attack;
    scales_with[ STAT_EXPERTISE_RATING         ] = attack || tank;
    scales_with[ STAT_EXPERTISE_RATING2        ] = tank || ( attack && ( position() == POSITION_FRONT ) );

    scales_with[ STAT_HIT_RATING                ] = true;
    scales_with[ STAT_CRIT_RATING               ] = true;
    scales_with[ STAT_HASTE_RATING              ] = true;
    scales_with[ STAT_MASTERY_RATING            ] = true;

    scales_with[ STAT_WEAPON_DPS   ] = attack;
    scales_with[ STAT_WEAPON_SPEED ] = sim -> weapon_speed_scale_factors ? attack : false;

    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = false;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = false;

    scales_with[ STAT_ARMOR          ] = tank;
    scales_with[ STAT_DODGE_RATING   ] = tank;
    scales_with[ STAT_PARRY_RATING   ] = tank;

    scales_with[ STAT_BLOCK_RATING ] = tank;

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

        case STAT_SPELL_POWER:       initial.stats.spell_power += v; break;

        case STAT_ATTACK_POWER:      initial.stats.attack_power              += v; break;

        case STAT_EXPERTISE_RATING:
        case STAT_EXPERTISE_RATING2:
          initial.stats.expertise_rating += v;
          initial.stats.expertise_rating2 += v;
          break;

        case STAT_HIT_RATING:
        case STAT_HIT_RATING2:
          initial.stats.hit_rating += v;
          initial.stats.hit_rating2 += v;
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

        case STAT_WEAPON_DPS:
          if ( main_hand_weapon.damage > 0 )
          {
            main_hand_weapon.damage  += main_hand_weapon.swing_time.total_seconds() * v;
            main_hand_weapon.min_dmg += main_hand_weapon.swing_time.total_seconds() * v;
            main_hand_weapon.max_dmg += main_hand_weapon.swing_time.total_seconds() * v;
          }
          break;

        case STAT_WEAPON_SPEED:
          if ( main_hand_weapon.swing_time > timespan_t::zero() )
          {
            timespan_t new_speed = ( main_hand_weapon.swing_time + timespan_t::from_seconds( v ) );
            double mult = new_speed / main_hand_weapon.swing_time;

            main_hand_weapon.min_dmg *= mult;
            main_hand_weapon.max_dmg *= mult;
            main_hand_weapon.damage  *= mult;

            main_hand_weapon.swing_time = new_speed;
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

        case STAT_WEAPON_OFFHAND_SPEED:
          if ( off_hand_weapon.swing_time > timespan_t::zero() )
          {
            timespan_t new_speed = ( off_hand_weapon.swing_time + timespan_t::from_seconds( v ) );
            double mult = new_speed / off_hand_weapon.swing_time;

            off_hand_weapon.min_dmg *= mult;
            off_hand_weapon.max_dmg *= mult;
            off_hand_weapon.damage  *= mult;

            off_hand_weapon.swing_time = new_speed;
          }
          break;

        case STAT_ARMOR:          initial.stats.armor       += v; break;
        case STAT_DODGE_RATING:   initial.stats.dodge_rating       += v; break;
        case STAT_PARRY_RATING:   initial.stats.parry_rating       += v; break;

        case STAT_BLOCK_RATING:   initial.stats.block_rating       += v; break;

        case STAT_MAX: break;

        default: assert( false ); break;
      }
    }
  }
}

// player_t::_init_actions ==================================================

void player_t::_init_actions()
{
  if ( action_list_str.empty() )
    no_action_list_provided = true;

  init_actions(); // virtual function which creates the action list string

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

  if ( ! action_list_str.empty() )
    get_action_priority_list( "default" ) -> action_list_str = action_list_str;

  int j = 0;

  for ( unsigned int alist = 0; alist < action_priority_list.size(); alist++ )
  {
    assert( ! ( ! action_priority_list[ alist ] -> action_list_str.empty() &&
                ! action_priority_list[ alist ] -> action_list.size() == 0 ) );

    // Convert old style action list to new style, all lines are without comments
    if ( ! action_priority_list[ alist ] -> action_list_str.empty() )
    {
      std::vector<std::string> splits = util::string_split( action_priority_list[ alist ] -> action_list_str, "/" );
      for ( size_t i = 0; i < splits.size(); i++ )
        action_priority_list[ alist ] -> action_list.push_back( action_priority_t( splits[ i ], "" ) );
    }

    if ( sim -> debug )
      sim -> out_debug.printf( "Player %s: actions.%s=%s", name(),
                     action_priority_list[ alist ] -> name_str.c_str(),
                     action_priority_list[ alist ] -> action_list_str.c_str() );

    for ( size_t i = 0; i < action_priority_list[ alist ] -> action_list.size(); i++ )
    {
      std::string& action_str = action_priority_list[ alist ] -> action_list[ i ].action_;

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

        pet_t* pet = find_pet( pet_name );
        if ( pet )
        {
          a =  new execute_pet_action_t( this, pet, pet_action, action_options );
        }
        else
        {
          sim -> errorf( "Player %s refers to unknown pet %s in action: %s\n",
                         name(), pet_name.c_str(), action_str.c_str() );
        }
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
          a -> action_list = action_priority_list[ alist ] -> name_str;

          a -> marker = ( char ) ( ( j < 10 ) ? ( '0' + j      ) :
                                   ( j < 36 ) ? ( 'A' + j - 10 ) :
                                   ( j < 66 ) ? ( 'a' + j - 36 ) :
                                   ( j < 79 ) ? ( '!' + j - 66 ) :
                                   ( j < 86 ) ? ( ':' + j - 79 ) : '.' );

          a -> signature_str = action_str;
          a -> signature = &( action_priority_list[ alist ] -> action_list[ i ] );

          if (  sim -> separate_stats_by_actions > 0 && !is_pet() )
          {
            a -> stats = get_stats( a -> name_str + "__" + a -> marker, a );
          }
          j++;
        }
      }
      else
      {
        sim -> errorf( "Player %s unable to create action: %s\n", name(), action_str.c_str() );
        sim -> cancel();
        return;
      }
    }
  }

  bool have_off_gcd_actions = false;
  for ( size_t i = 0; i < action_list.size(); ++i )
  {
    action_t* action = action_list[ i ];
    action -> init();
    if ( action -> trigger_gcd == timespan_t::zero() && ! action -> background && action -> use_off_gcd )
    {
      find_action_priority_list( action -> action_list ) -> off_gcd_actions.push_back( action );
      // Optimization: We don't need to do off gcd stuff when there are no other off gcd actions than these two
      if ( action -> name_str != "run_action_list" && action -> name_str != "swap_action_list" )
        have_off_gcd_actions = true;
    }
  }

  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ] -> consolidate_snapshot_flags();

  if ( choose_action_list.empty() ) choose_action_list = "default";

  action_priority_list_t* chosen_action_list = find_action_priority_list( choose_action_list );

  if ( ! chosen_action_list && choose_action_list != "default" )
  {
    sim -> errorf( "Action List %s not found, using default action list.\n", choose_action_list.c_str() );
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
}

// player_t::create_buffs ===================================================

void player_t::create_buffs()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Creating buffs for player (%s)", name() );

  if ( ! is_enemy() && type != PLAYER_GUARDIAN )
  {
    if ( type != PLAYER_PET )
    {
      // Racials
      buffs.berserking                = haste_buff_creator_t( this, "berserking", find_spell( 26297 ) ).add_invalidate( CACHE_HASTE );
      buffs.heroic_presence           = buff_creator_t( this, "heroic_presence" ).max_stack( 1 ).add_invalidate( CACHE_HIT );
      buffs.stoneform                 = buff_creator_t( this, "stoneform", find_spell( 65116 ) );
      buffs.blood_fury                = stat_buff_creator_t( this, "blood_fury" )
                                        .spell( find_racial_spell( "Blood Fury" ) )
                                        .add_invalidate( CACHE_SPELL_POWER )
                                        .add_invalidate( CACHE_ATTACK_POWER );
      buffs.fortitude                 = buff_creator_t( this, "fortitude", find_spell( 137593 ) ).add_invalidate( CACHE_STAMINA ).activated( false );
      buffs.shadowmeld                = buff_creator_t( this, "shadowmeld", find_spell( 58984 ) ).cd( timespan_t::zero() );

      // Legendary meta haste buff
      buffs.tempus_repit              = buff_creator_t( this, "tempus_repit", find_spell( 137590 ) ).add_invalidate( CACHE_HASTE ).activated( false );

      // Profession buffs
      double lb_amount = 0.0;
      if      ( profession[ PROF_HERBALISM ] >= 600 )
        lb_amount = 2880;
      else if ( profession[ PROF_HERBALISM ] >= 525 )
        lb_amount = 480;
      else if ( profession[ PROF_HERBALISM ] >= 450 )
        lb_amount = 240;
      else if ( profession[ PROF_HERBALISM ] >= 375 )
        lb_amount = 120;
      else if ( profession[ PROF_HERBALISM ] >= 300 )
        lb_amount = 70;
      else if ( profession[ PROF_HERBALISM ] >= 225 )
        lb_amount = 55;
      else if ( profession[ PROF_HERBALISM ] >= 150 )
        lb_amount = 35;
      else if ( profession[ PROF_HERBALISM ] >= 75 )
        lb_amount = 15;
      else if ( profession[ PROF_HERBALISM ] >= 1 )
        lb_amount = 5;

      buffs.lifeblood = stat_buff_creator_t( this, "lifeblood" )
                        .max_stack( 1 )
                        .duration( timespan_t::from_seconds( 20.0 ) )
                        .add_stat( STAT_HASTE_RATING, lb_amount );

      // Vengeance
      buffs.vengeance = buff_creator_t( this, "vengeance" )
                        .max_stack( 1 )
                        .duration( timespan_t::from_seconds( 20.0 ) )
                        .default_value( 0 )
                        .add_invalidate( CACHE_ATTACK_POWER );

      // Potions
      struct potion_buff_creator : public stat_buff_creator_t
      {
        potion_buff_creator( player_t* p,
                             const std::string& n,
                             timespan_t cd = timespan_t::from_seconds( 60.0 ) ) :
          stat_buff_creator_t ( p,  n )
        {
          max_stack( 1 );
          this -> cd( cd );
          // Kludge of the century, version 2
          chance( p -> sim -> allow_potions );
        }
      };

      // Cataclysm
      potion_buffs.speed      = potion_buff_creator( this, "speed_potion" )
                                .duration( timespan_t::from_seconds( 15.0 ) )
                                .add_stat( STAT_HASTE_RATING, 500.0 );
      potion_buffs.volcanic   = potion_buff_creator( this, "volcanic_potion" )
                                .duration( timespan_t::from_seconds( 25.0 ) )
                                .add_stat( STAT_INTELLECT, 1200.0 );
      potion_buffs.earthen    = potion_buff_creator( this, "earthen_potion" )
                                .duration( timespan_t::from_seconds( 25.0 ) )
                                .add_stat( STAT_ARMOR, 4800.0 );
      potion_buffs.golemblood = potion_buff_creator( this, "golemblood_potion" )
                                .duration( timespan_t::from_seconds( 25.0 ) )
                                .add_stat( STAT_STRENGTH, 1200.0 );
      potion_buffs.tolvir     = potion_buff_creator( this, "tolvir_potion" )
                                .duration( timespan_t::from_seconds( 25.0 ) )
                                .add_stat( STAT_AGILITY, 1200.0 );

      // MoP
      potion_buffs.jade_serpent = potion_buff_creator( this, "jade_serpent_potion" )
                                  .spell( find_spell( 105702 ) );
      potion_buffs.mountains    = potion_buff_creator( this, "mountains_potion" )
                                  .spell( find_spell( 105698 ) );
      potion_buffs.mogu_power   = potion_buff_creator( this, "mogu_power_potion" )
                                  .spell( find_spell( 105706 ) );
      potion_buffs.virmens_bite = potion_buff_creator( this, "virmens_bite_potion" )
                                  .spell( find_spell( 105697 ) );

      buffs.amplified = buff_creator_t( this, "amplified", find_spell( 146051 ) )
                        .add_invalidate( CACHE_MASTERY )
                        .add_invalidate( CACHE_HASTE )
                        .add_invalidate( CACHE_SPIRIT )
                        .chance( 0 )
                        /* .add_invalidate( CACHE_PLAYER_CRITICAL_DAMAGE ) */
                        /* .add_invalidate( CACHE_PLAYER_CRITICAL_HEALING ) */;

    buffs.cooldown_reduction = buff_creator_t( this, "cooldown_reduction" )
                               .chance( 0 )
                               .default_value( 1 );
    }

    buffs.hymn_of_hope              = new hymn_of_hope_buff_t( this, "hymn_of_hope", find_spell( 64904 ) );

    buffs.stormlash                 = new stormlash_buff_t( this, find_spell( 120687 ) );
  }

  buffs.courageous_primal_diamond_lucidity = buff_creator_t( this, "lucidity" )
                                             .spell( find_spell( 137288 ) );

  buffs.body_and_soul             = buff_creator_t( this, "body_and_soul" )
                                    .max_stack( 1 )
                                    .duration( timespan_t::from_seconds( 4.0 ) );

  buffs.grace                     = buff_creator_t( this,  "grace" )
                                    .max_stack( 3 )
                                    .duration( timespan_t::from_seconds( 15.0 ) );

  buffs.raid_movement = buff_creator_t( this, "raid_movement" ).max_stack( 1 );
  buffs.self_movement = buff_creator_t( this, "self_movement" ).max_stack( 1 );

  // Infinite-Stacking Buffs and De-Buffs

  buffs.stunned        = buff_creator_t( this, "stunned" ).max_stack( 1 );
  debuffs.bleeding     = buff_creator_t( this, "bleeding" ).max_stack( 1 );
  debuffs.casting      = buff_creator_t( this, "casting" ).max_stack( 1 ).quiet( 1 );
  debuffs.invulnerable = buff_creator_t( this, "invulnerable" ).max_stack( 1 );
  debuffs.vulnerable   = buff_creator_t( this, "vulnerable" ).max_stack( 1 );
  debuffs.flying       = buff_creator_t( this, "flying" ).max_stack( 1 );
}

// player_t::find_item ======================================================

item_t* player_t::find_item( const std::string& str )
{
  for ( size_t i = 0; i < items.size(); i++ )
    if ( str == items[ i ].name() )
      return &items[ i ];

  return 0;
}

// player_t::energy_regen_per_second ========================================

double player_t::energy_regen_per_second()
{
  double r = 0;
  if ( base_energy_regen_per_second )
    r = base_energy_regen_per_second * ( 1.0 / cache.attack_haste() );
  return r;
}

// player_t::focus_regen_per_second =========================================

double player_t::focus_regen_per_second()
{
  double r = 0;
  if ( base_focus_regen_per_second )
    r = base_focus_regen_per_second * ( 1.0 / cache.attack_haste() );
  return r;
}

// player_t::mana_regen_per_second ==========================================

double player_t::mana_regen_per_second()
{
  return current.mana_regen_per_second + cache.spirit() * current.mana_regen_per_spirit * current.mana_regen_from_spirit_multiplier;
}

// player_t::composite_attack_haste =========================================

double player_t::composite_melee_haste()
{
  double h = 1.0 / ( 1.0 + std::max( 0.0, composite_melee_haste_rating() ) / current.rating.attack_haste );

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.bloodlust -> up() )
    {
      h *= 1.0 / ( 1.0 + buffs.bloodlust -> data().effectN( 1 ).percent() );
    }

    if ( buffs.unholy_frenzy -> check() )
    {
      h *= 1.0 / ( 1.0 + buffs.unholy_frenzy -> value() );
    }

    if ( buffs.mongoose_mh && buffs.mongoose_mh -> up() ) h *= 1.0 / ( 1.0 + 30 / current.rating.attack_haste );
    if ( buffs.mongoose_oh && buffs.mongoose_oh -> up() ) h *= 1.0 / ( 1.0 + 30 / current.rating.attack_haste );

    if ( buffs.berserking -> up() )
    {
      h *= 1.0 / ( 1.0 + buffs.berserking -> data().effectN( 1 ).percent() );
    }
  }

  return h;
}

// player_t::composite_attack_speed =========================================

double player_t::composite_melee_speed()
{
  double h = composite_melee_haste();

  if ( race == RACE_GOBLIN )
  {
    h *= 1.0 / ( 1.0 + 0.01 );
  }

  if ( ! is_enemy() && ! is_add() && sim -> auras.attack_speed -> check() )
    h *= 1.0 / ( 1.0 + sim -> auras.attack_speed -> value() );

  return h;
}

// player_t::composite_attack_power =========================================

double player_t::composite_melee_attack_power()
{
  double ap = current.stats.attack_power;

  ap += current.attack_power_per_strength * ( cache.strength() - 10 );
  ap += current.attack_power_per_agility  * ( cache.agility() - 10 );

  if ( ! is_enemy() )
    ap += buffs.vengeance -> value();

  return ap;
}

// player_t::composite_attack_power_multiplier ==============================

double player_t::composite_attack_power_multiplier()
{
  double m = current.attack_power_multiplier;

  if ( ! is_pet() && ! is_enemy() && sim -> auras.attack_power_multiplier -> check() )
    m *= 1.0 + sim -> auras.attack_power_multiplier -> value();

  return m;
}

// player_t::composite_attack_crit ==========================================

double player_t::composite_melee_crit()
{
  double ac = current.attack_crit + composite_melee_crit_rating() / current.rating.attack_crit;

  if ( current.attack_crit_per_agility )
    ac += ( cache.agility() / current.attack_crit_per_agility / 100.0 );

  if ( ! is_pet() && ! is_enemy() && ! is_add() && sim -> auras.critical_strike -> check() )
    ac += sim -> auras.critical_strike -> value();

  if ( race == RACE_WORGEN )
    ac += 0.01;

  return ac;
}

// player_t::composite_attack_expertise =====================================

double player_t::composite_melee_expertise( weapon_t* weapon )
{
  double e = composite_expertise_rating() / current.rating.expertise;

  if ( weapon_racial( weapon ) )
    e += 0.01;

  return e;
}

// player_t::composite_attack_hit ===========================================

double player_t::composite_melee_hit()
{
  double ah = composite_melee_hit_rating() / current.rating.attack_hit;

  if ( buffs.heroic_presence && buffs.heroic_presence -> up() )
    ah += 0.01;

  return ah;
}

// player_t::composite_armor ================================================

double player_t::composite_armor()
{
  double a = current.stats.armor;

  a *= composite_armor_multiplier();

  if ( debuffs.weakened_armor -> check() )
    a *= 1.0 - debuffs.weakened_armor -> check() * debuffs.weakened_armor -> value();

  if ( debuffs.shattering_throw -> check() )
    a *= 1.0 - debuffs.shattering_throw -> value();

  return a;
}

// player_t::composite_armor_multiplier =====================================

double player_t::composite_armor_multiplier()
{
  double a = current.armor_multiplier;

  if ( meta_gem == META_AUSTERE_PRIMAL )
  {
    a += 0.02;
  }

  return a;
}

// player_t::composite_miss ============================================

double player_t::composite_miss()
{
  double m = current.miss;

  assert( m >= 0.0 && m <= 1.0 );

  return m;
}

// player_t::composite_block ===========================================

double player_t::composite_block()
{
  double block_by_rating = composite_block_rating() / current.rating.block;

  double b = current.block;

  if ( block_by_rating > 0 )
  {
    //the block by rating gets rounded because that's how blizzard rolls...
    b += 1 / ( 1 / diminished_block_cap + diminished_kfactor / ( util::round( 12800 * block_by_rating ) / 12800 ) );
  }

  return b;
}

// player_t::composite_dodge ===========================================

double player_t::composite_dodge()
{
  double dodge_by_dodge_rating = composite_dodge_rating() / current.rating.dodge;
  double dodge_by_agility = ( cache.agility() - base.stats.attribute[ ATTR_AGILITY ] ) * current.dodge_per_agility;

  double d = current.dodge;

  d += base.stats.attribute[ ATTR_AGILITY ] * current.dodge_per_agility;

  if ( dodge_by_agility > 0 || dodge_by_dodge_rating > 0 )
  {
    d += 1 / ( 1 / diminished_dodge_cap + diminished_kfactor / ( dodge_by_dodge_rating + dodge_by_agility ) );
  }

  d += racials.quickness -> effectN( 1 ).percent();

  return d;
}

// player_t::composite_parry ===========================================

double player_t::composite_parry()
{

  //changed it to match the typical formulation

  double parry_by_parry_rating = composite_parry_rating() / current.rating.parry;
  double parry_by_strength = ( cache.strength() - base.stats.attribute[ ATTR_STRENGTH ] ) * current.parry_per_strength;
  // these are pre-DR values

  double p = current.parry;

  if ( current.parry_per_strength > 0 )
  {
    p += base.stats.attribute[ ATTR_STRENGTH ] * current.parry_per_strength;
  }

  if ( parry_by_strength > 0 || parry_by_parry_rating > 0 )
  {
    p += 1 / ( 1 / diminished_parry_cap + diminished_kfactor / ( parry_by_parry_rating + parry_by_strength ) );
  }
  return p; //this is the post-DR parry value
}



// player_t::composite_block_reduction =================================

double player_t::composite_block_reduction()
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

double player_t::composite_crit_block()
{
  return 0;
}

// player_t::composite_crit_avoidance========================================

double player_t::composite_crit_avoidance()
{
  return 0;
}

// player_t::composite_spell_haste ==========================================

double player_t::composite_spell_haste()
{
  double h = 1.0 / ( 1.0 + std::max( 0.0, composite_spell_haste_rating() ) / current.rating.spell_haste );

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.bloodlust -> up() )
    {
      h *= 1.0 / ( 1.0 + buffs.bloodlust -> data().effectN( 1 ).percent() );
    }

    if ( buffs.berserking -> up() )
      h *= 1.0 / ( 1.0 + buffs.berserking -> data().effectN( 1 ).percent() );

    if ( buffs.tempus_repit -> up() )
      h *= 1.0 / ( 1.0 + buffs.tempus_repit -> data().effectN( 1 ).percent() );

    if ( sim -> auras.spell_haste -> check() )
      h *= 1.0 / ( 1.0 + sim -> auras.spell_haste -> value() );

    if ( race == RACE_GOBLIN )
    {
      h *= 1.0 / ( 1.0 + 0.01 );
    }
  }

  return h;
}

// player_t::composite_spell_speed ==========================================

double player_t::composite_spell_speed()
{
  double h = cache.spell_haste();

  return  h;
}

// player_t::composite_spell_power ==========================================

double player_t::composite_spell_power( school_e /* school */ )
{
  double sp = current.stats.spell_power;

  sp += current.spell_power_per_intellect * ( cache.intellect() - 10 ); // The spellpower is always lower by 10, cata beta build 12803

  return sp;
}

// player_t::composite_spell_power_multiplier ===============================

double player_t::composite_spell_power_multiplier()
{
  double m = current.spell_power_multiplier;

  if ( ! is_pet() && ! is_enemy() && sim -> auras.spell_power_multiplier -> check() )
    m *= 1.0 + sim -> auras.spell_power_multiplier -> value();

  return m;
}

// player_t::composite_spell_crit ===========================================

double player_t::composite_spell_crit()
{
  double sc = current.spell_crit + composite_spell_crit_rating() / current.rating.spell_crit;

  if ( current.spell_crit_per_intellect > 0 )
  {
    sc += ( cache.intellect() / current.spell_crit_per_intellect / 100.0 );
  }

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( sim -> auras.critical_strike -> check() )
      sc += sim -> auras.critical_strike -> value();
  }

  if ( race == RACE_WORGEN )
    sc += 0.01;

  return sc;
}

// player_t::composite_spell_hit ============================================

double player_t::composite_spell_hit()
{
  double sh = composite_spell_hit_rating() / current.rating.spell_hit;

  if ( weapon_racial( &main_hand_weapon ) )
  {
    sh += 0.01;
  }

  if ( buffs.heroic_presence && buffs.heroic_presence -> up() )
    sh += 0.01;

  sh += composite_melee_expertise();

  return sh;
}

// player_t::composite_mastery ==============================================

double player_t::composite_mastery()
{
  return util::round( current.mastery + composite_mastery_rating() / current.rating.mastery, 2 );
}

// player_t::composite_player_multiplier ====================================

double player_t::composite_player_multiplier( school_e school )
{
  double m = 1.0;

  if ( type != PLAYER_GUARDIAN )
  {
    if ( school == SCHOOL_PHYSICAL && debuffs.weakened_blows -> check() )
      m *= 1.0 - debuffs.weakened_blows -> value();

    if ( buffs.tricks_of_the_trade -> check() )
    {
      // because of the glyph we now track the damage % increase in the buff value
      m *= 1.0 + buffs.tricks_of_the_trade -> value();
    }
  }

  if ( ( race == RACE_TROLL ) && ( sim -> target -> race == RACE_BEAST ) )
  {
    m *= 1.05;
  }

  return m;
}

// player_t::composite_player_td_multiplier =================================

double player_t::composite_player_td_multiplier( school_e /* school */, action_t* /* a */ )
{
  return 1.0;
}

// player_t::composite_player_heal_multiplier ===============================

double player_t::composite_player_heal_multiplier( school_e /* school */ )
{
  return 1.0;
}

// player_t::composite_player_th_multiplier =================================

double player_t::composite_player_th_multiplier( school_e /* school */ )
{
  return 1.0;
}

// player_t::composite_player_absorb_multiplier =============================

double player_t::composite_player_absorb_multiplier( school_e /* school */ )
{
  return 1.0;
}

double player_t::composite_player_critical_damage_multiplier()
{
  double m = 1.0;

  m *= 1.0 + buffs.skull_banner -> value();

  return m;
}

double player_t::composite_player_critical_healing_multiplier()
{
  double m = 1.0;

  return m;
}

// player_t::composite_movement_speed =======================================

double player_t::composite_movement_speed()
{
  double speed = base_movement_speed;

  speed *= 1.0 + buffs.body_and_soul -> value();

  // From http://www.wowpedia.org/Movement_speed_effects
  // Additional items looked up

  // Pursuit of Justice, Quickening: 8%/15%

  // DK: Unholy Presence: 15%

  // Druid: Feral Swiftness: 15%/30%

  // Aspect of the Cheetah/Pack: 30%, with talent Pathfinding +34%/38%

  // Shaman Ghost Wolf: 30%, with Glyph 35%

  // Druid: Travel Form 40%

  // Druid: Dash: 50/60/70

  // Mage: Blazing Speed: 5%/10% chance after being hit for 50% for 8 sec
  //       Improved Blink: 35%/70% for 3 sec after blink
  //       Glyph of Invisibility: 40% while invisible

  // Rogue: Sprint 70%

  // Swiftness Potion: 50%

  return speed;
}

// player_t::composite_attribute ============================================

double player_t::composite_attribute( attribute_e attr )
{
  double a = current.stats.attribute[ attr ];
  double m = ( ( level >= 50 ) && matching_gear ) ? ( 1.0 + matching_gear_multiplier( attr ) ) : 1.0;

  switch ( attr )
  {
    case ATTR_SPIRIT:
      if ( race == RACE_HUMAN )
        a += ( a - base.stats.get_stat( STAT_SPIRIT ) ) * 0.03;
      break;
    default:
      break;
  }

  a = util::floor( ( a - base.stats.attribute[ attr ] ) * m ) + base.stats.attribute[ attr ];

  return a;
}

// player_t::composite_attribute_multiplier =================================

double player_t::composite_attribute_multiplier( attribute_e attr )
{
  double m = current.attribute_multiplier[ attr ];

  if ( is_pet() || is_enemy() ) return m;

  switch ( attr )
  {
    case ATTR_STRENGTH:
    case ATTR_AGILITY:
    case ATTR_INTELLECT:
      if ( sim -> auras.str_agi_int -> check() )
        m *= 1.0 + sim -> auras.str_agi_int -> value();
      break;
    case ATTR_STAMINA:
      if ( sim -> auras.stamina -> check() )
        m *= 1.0 + sim -> auras.stamina -> value();
      break;
    case ATTR_SPIRIT:
      m *= 1.0 + buffs.amplified -> value();
      break;
    default:
      break;
  }

  return m;
}

// player_t::composite_rating_multiplier ====================================

double player_t::composite_rating_multiplier( rating_e rating )
{
  double v = 1.0;

  if ( is_pet() || is_enemy() )
    return v;

  // Internally, we treat all the primary rating types as a single entity; 
  // in game, they are actually split into spell/ranged/melee
  switch ( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      v *= 1.0 + buffs.amplified -> value(); break;
    case RATING_MASTERY:
      v *= 1.0 + buffs.amplified -> value(); break;
    default: break;
  }

  return v;
}

// player_t::composite_rating ===============================================

double player_t::composite_rating( rating_e rating )
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
      if ( ! is_pet() && ! is_enemy() && sim -> auras.mastery -> check() )
        v += sim -> auras.mastery -> value();
      break;
    case RATING_EXPERTISE:
      v = current.stats.expertise_rating; break;
    case RATING_DODGE:
      v = current.stats.dodge_rating; break;
    case RATING_PARRY:
      v = current.stats.parry_rating; break;
    case RATING_BLOCK:
      v = current.stats.block_rating; break;
    default: break;
  }

  return v * composite_rating_multiplier( rating );
}

// player_t::composite_player_vulnerability =================================

double player_t::composite_player_vulnerability( school_e school )
{
  double m = 1.0;

  if ( debuffs.magic_vulnerability -> check() &&
       school != SCHOOL_NONE && school != SCHOOL_PHYSICAL )
    m *= 1.0 + debuffs.magic_vulnerability -> value();
  else if ( debuffs.physical_vulnerability -> check() &&
            ( school == SCHOOL_PHYSICAL ) )
    m *= 1.0 + debuffs.physical_vulnerability -> value();

  if ( debuffs.vulnerable -> check() )
    m *= 1.0 + debuffs.vulnerable -> value();

  return m;
}

// player_t::composite_ranged_attack_player_vulnerability ===================

double player_t::composite_ranged_attack_player_vulnerability()
{
  // MoP: Increase ranged damage taken by 5%. make sure
  if ( debuffs.ranged_vulnerability -> check() )
    return 1.0 + debuffs.ranged_vulnerability -> value();

  return 1.0;
}

double player_t::composite_mitigation_multiplier( school_e /* school */ )
{
  return 1.0;
}

double player_t::composite_mastery_value()
{
  return composite_mastery() * mastery_coefficient();
}

#ifdef SC_STAT_CACHE

// player_t::invalidate_cache ===============================================

void player_t::invalidate_cache( cache_e c )
{
  if ( ! cache.active ) return;

  if ( sim -> log ) sim -> out_log.printf( "%s invalidates %s", name(), util::cache_type_string( c ) );

  // Special linked invalidations
  switch ( c )
  {
    case CACHE_STRENGTH:
      if ( current.attack_power_per_strength > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      if ( current.parry_per_strength > 0 )
        invalidate_cache( CACHE_PARRY );
    case CACHE_AGILITY:
      if ( current.attack_power_per_agility > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      if ( current.attack_crit_per_agility > 0 )
        invalidate_cache( CACHE_ATTACK_CRIT );
      if ( current.dodge_per_agility > 0 )
        invalidate_cache( CACHE_DODGE );
      break;
    case CACHE_INTELLECT:
      if ( current.spell_crit_per_intellect > 0 )
        invalidate_cache( CACHE_SPELL_CRIT );
      if ( current.spell_power_per_intellect > 0 )
        invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_ATTACK_HASTE:
      invalidate_cache( CACHE_ATTACK_SPEED );
      break;
    case CACHE_SPELL_HASTE:
      invalidate_cache( CACHE_SPELL_SPEED );
      break;
    default: break;
  }

  // Normal invalidation of the corresponding Cache
  switch ( c )
  {
    case CACHE_SPELL_POWER:
      range::fill( cache.spell_power_valid, false );
      break;
    case CACHE_EXP:
      invalidate_cache( CACHE_ATTACK_EXP );
      invalidate_cache( CACHE_SPELL_HIT  );
      break;
    case CACHE_HIT:
      invalidate_cache( CACHE_ATTACK_HIT );
      invalidate_cache( CACHE_SPELL_HIT  );
      break;
    case CACHE_CRIT:
      invalidate_cache( CACHE_ATTACK_CRIT );
      invalidate_cache( CACHE_SPELL_CRIT  );
      break;
    case CACHE_HASTE:
      invalidate_cache( CACHE_ATTACK_HASTE );
      invalidate_cache( CACHE_SPELL_HASTE  );
      break;
    case CACHE_SPEED:
      invalidate_cache( CACHE_ATTACK_SPEED );
      invalidate_cache( CACHE_SPELL_SPEED  );
      break;
    case CACHE_PLAYER_DAMAGE_MULTIPLIER:
      range::fill( cache.player_mult_valid, false );
      break;
    case CACHE_PLAYER_HEAL_MULTIPLIER:
      range::fill( cache.player_heal_mult_valid, false );
      break;
    default:
      cache.valid[ c ] = false; // Invalidates only its own cache.
      break;
  }
}

#endif

void player_t::sequence_add( const action_t* a, const player_t* target, const timespan_t& ts )
{
  if ( a -> marker )
    // Collect iteration#1 data, for log/debug/iterations==1 simulation iteration#0 data
    if ( ( a -> sim -> iterations <= 1 && a -> sim -> current_iteration == 0 ) ||
         ( a -> sim -> iterations > 1 && a -> sim -> current_iteration == 1 ) )
      collected_data.action_sequence.push_back( new player_collected_data_t::action_sequence_data_t( a, target, ts, this ) );
};

// player_t::combat_begin ===================================================

void player_t::combat_begin()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Combat begins for player %s", name() );

  if ( ! is_pet() && ! is_add() )
  {
    arise();
  }

  if ( race == RACE_DRAENEI )
  {
    buffs.heroic_presence -> trigger();
  }

  init_resources( true );

  for ( size_t i = 0; i < precombat_action_list.size(); i++ )
  {
    if ( precombat_action_list[ i ] -> ready() )
      precombat_action_list[ i ] -> execute();
  }

  if ( ! precombat_action_list.empty() )
    in_combat = true;

  // re-initialize collected_data.health_changes.previous_*_level
  // necessary because food/flask are counted as resource gains, and thus provide phantom
  // gains on the timeline if not corrected
  collected_data.health_changes.previous_gain_level = resource_gained [ RESOURCE_HEALTH ];
  // forcing a resource timeline data collection in combat_end() seems to have rendered this next line unnecessary
  collected_data.health_changes.previous_loss_level = resource_lost [ RESOURCE_HEALTH ];

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
        if ( sim -> debug )
          sim -> out_debug.printf( "Combat ends for player %s at time %.4f fight_length=%.4f", name(), sim -> current_time.total_seconds(), iteration_fight_length.total_seconds() );
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
  iteration_executed_foreground_actions = 0;
  iteration_dmg = 0;
  iteration_heal = 0;
  iteration_dmg_taken = 0;
  iteration_heal_taken = 0;
  active_during_iteration = false;

  if ( ! is_pet() && primary_role() == ROLE_TANK )
  {
    collected_data.health_changes.timeline.clear(); // Drop Data
    collected_data.health_changes.timeline_normalized.clear();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < stats_list.size(); ++i )
    stats_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < benefit_list.size(); ++i )
    benefit_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < proc_list.size(); ++i )
    proc_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < pet_list.size(); ++i )
    pet_list[ i ] -> datacollection_begin();
}

// endpoint for statistical data collection

void player_t::datacollection_end()
{
  // This checks if the actor was arisen at least once during this iteration.
  if ( ! requires_data_collection() )
    return;

  if ( sim -> debug )
    sim -> out_debug.printf( "Data collection ends for player %s at time %.4f fight_length=%.4f", name(), sim -> current_time.total_seconds(), iteration_fight_length.total_seconds() );

  for ( size_t i = 0; i < pet_list.size(); ++i )
    pet_list[ i ] -> datacollection_end();

  if ( arise_time >= timespan_t::zero() )
  {
    // If we collect data while the player is still alive, capture active time up to now
    assert( sim -> current_time >= arise_time );
    iteration_fight_length += sim -> current_time - arise_time;
    arise_time = sim -> current_time;
  }

  for ( size_t i = 0; i < stats_list.size(); ++i )
    stats_list[ i ] -> datacollection_end();

  if ( ! is_enemy() && ! is_add() )
  {
    sim -> iteration_dmg += iteration_dmg;
    sim -> iteration_heal += iteration_heal;
  }
  
  // make sure TMI-relevant timeline lengths all match for tanks
  if ( ! is_enemy() && ! is_pet() && primary_role() == ROLE_TANK )
  {
    collected_data.timeline_healing_taken.add( sim -> current_time, 0.0 );
    collected_data.timeline_dmg_taken.add( sim -> current_time, 0.0 );
    collected_data.health_changes.timeline.add( sim -> current_time, 0.0 );
    collected_data.health_changes.timeline_normalized.add( sim -> current_time, 0.0 );
  }
  collected_data.collect_data( *this );

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> datacollection_end();

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> datacollection_end( iteration_fight_length );

  for ( size_t i = 0; i < benefit_list.size(); ++i )
    benefit_list[ i ] -> datacollection_end();

  for ( size_t i = 0; i < proc_list.size(); ++i )
    proc_list[ i ] -> datacollection_end();
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
  /* Don't complain about targetdata buffs, since it is percetly viable that the buff
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
    resource_lost  [ i ] += other.resource_lost  [ i ];
    resource_gained[ i ] += other.resource_gained[ i ];
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

  // Vengeance Timeline
  vengeance.merge( other.vengeance );

  // Action Map
  for ( size_t i = 0; i < other.action_list.size(); ++i )
    action_list[ i ] -> total_executions += other.action_list[ i ] -> total_executions;
}

// player_t::reset ==========================================================

void player_t::reset()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Resetting player %s", name() );

  last_cast = timespan_t::zero();
  gcd_ready = timespan_t::zero();

  cache.invalidate();

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

  executing = 0;
  channeling = 0;
  readying = 0;
  off_gcd = 0;
  in_combat = false;

  cast_delay_reaction = timespan_t::zero();
  cast_delay_occurred = timespan_t::zero();

  main_hand_weapon.buff_type  = 0;
  main_hand_weapon.buff_value = 0;
  main_hand_weapon.bonus_dmg  = 0;

  off_hand_weapon.buff_type  = 0;
  off_hand_weapon.buff_value = 0;
  off_hand_weapon.bonus_dmg  = 0;

  active_elixir.battle = active_elixir.guardian = false;
  flask           = FLASK_NONE;
  food            = FOOD_NONE;

  callbacks.reset();

  init_resources( true );

  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ] -> reset();

  for ( size_t i = 0; i < cooldown_list.size(); ++i )
    cooldown_list[ i ] -> reset( false );

  for ( size_t i = 0; i < dot_list.size(); ++i )
    dot_list[ i ] -> reset();

  for ( size_t i = 0; i < stats_list.size(); ++i )
    stats_list[ i ] -> reset();

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> reset();

  for ( size_t i = 0; i < proc_list.size(); ++i )
    proc_list[ i ] -> reset();

  potion_used = 0;

  temporary = gear_stats_t();

  item_cooldown.reset( false );

  incoming_damage.clear();
}

// player_t::trigger_ready ==================================================

void player_t::trigger_ready()
{
  if ( ready_type == READY_POLL ) return;

  if ( readying ) return;
  if ( executing ) return;
  if ( channeling ) return;

  if ( current.sleeping ) return;

  if ( buffs.stunned -> check() ) return;

  if ( sim -> debug ) sim -> out_debug.printf( "%s is triggering ready, interval=%f", name(), ( sim -> current_time - started_waiting ).total_seconds() );

  assert( started_waiting >= timespan_t::zero() );
  iteration_waiting_time += sim -> current_time - started_waiting;
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

  if ( current.sleeping )
    return;

  executing = 0;
  channeling = 0;
  action_queued = false;

  started_waiting = timespan_t::min();

  timespan_t gcd_adjust = gcd_ready - ( sim -> current_time + delta_time );
  if ( gcd_adjust > timespan_t::zero() ) delta_time += gcd_adjust;

  if ( unlikely( waiting ) )
  {
    iteration_waiting_time += delta_time;
  }
  else
  {
    timespan_t lag = timespan_t::zero();

    if ( last_foreground_action && ! last_foreground_action -> auto_cast )
    {
      if ( last_foreground_action -> ability_lag > timespan_t::zero() )
      {
        timespan_t ability_lag = rng().gauss( last_foreground_action -> ability_lag, last_foreground_action -> ability_lag_stddev );
        timespan_t gcd_lag     = rng().gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        timespan_t diff        = ( gcd_ready + gcd_lag ) - ( sim -> current_time + ability_lag );
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
      else if ( last_foreground_action -> channeled )
      {
        lag = rng().gauss( sim -> channel_lag, sim -> channel_lag_stddev );
      }
      else
      {
        timespan_t   gcd_lag = rng().gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        timespan_t queue_lag = rng().gauss( sim -> queue_lag, sim -> queue_lag_stddev );

        timespan_t diff = ( gcd_ready + gcd_lag ) - ( sim -> current_time + queue_lag );

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

    delta_time += lag;
  }

  if ( last_foreground_action )
  {
    // This is why "total_execute_time" is not tracked per-target!
    last_foreground_action -> stats -> iteration_total_execute_time += delta_time;
  }

  readying = new ( *sim ) player_ready_event_t( *this, delta_time );

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

  if ( sim -> log )
    sim -> out_log.printf( "%s arises.", name() );

  init_resources( true );

  cache.invalidate();

  readying = 0;
  off_gcd = 0;

  arise_time = sim -> current_time;

  if ( unlikely( is_enemy() ) )
  {
    sim -> active_enemies++;
    sim -> target_non_sleeping_list.push_back( this );
  }
  else
  {
    sim -> active_allies++;
    sim -> player_non_sleeping_list.push_back( this );
  }

  if ( ! is_enemy() && ! is_pet() )
  {
    buffs.amplified -> trigger();
    buffs.cooldown_reduction -> trigger();
  }

  if ( has_foreground_actions( this ) )
    schedule_ready();

  active_during_iteration = true;
}

// player_t::demise =========================================================

void player_t::demise()
{
  // No point in demising anything if we're not even active
  if ( current.sleeping )
    return;

  if ( sim -> log )
    sim -> out_log.printf( "%s demises.", name() );

  assert( arise_time >= timespan_t::zero() );
  iteration_fight_length += sim -> current_time - arise_time;
  arise_time = timespan_t::min();

  current.sleeping = true;
  if ( readying )
  {
    core_event_t::cancel( readying );
    readying = 0;
  }

  if ( unlikely( is_enemy() ) )
  {
    sim -> active_enemies--;
    sim -> target_non_sleeping_list.find_and_erase_unordered( this );
  }
  else
  {
    sim -> active_allies--;
    sim -> player_non_sleeping_list.find_and_erase_unordered( this );
  }

  core_event_t::cancel( off_gcd );

  // stops vengeance and clear vengeance_list
  vengeance_stop();

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> expire();
    // Dead actors speak no lies .. or proc aura delayed buffs
    core_event_t::cancel( b -> delay );
    core_event_t::cancel( b -> expiration_delay );
  }
  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ] -> cancel();

  // sim -> cancel_events( this );

  for ( size_t i = 0; i < pet_list.size(); ++i )
    pet_list[ i ] -> demise();

  for ( size_t i = 0; i < dot_list.size(); ++i )
    dot_list[ i ] -> cancel();
}

// player_t::interrupt ======================================================

void player_t::interrupt()
{
  // FIXME! Players will need to override this to handle background repeating actions.

  if ( sim -> log ) sim -> out_log.printf( "%s is interrupted", name() );

  if ( executing  ) executing  -> interrupt_action();
  if ( channeling ) channeling -> interrupt_action();

  if ( buffs.stunned -> check() )
  {
    if ( readying ) core_event_t::cancel( readying );
    if ( off_gcd ) core_event_t::cancel( off_gcd );
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
  // FIXME! In the future, some movement events may not cause auto-attack to stop.

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

  for ( size_t i = 0, num_actions = active_action_list -> foreground_action_list.size(); i < num_actions; ++i )
  {
    action_t* a = active_action_list -> foreground_action_list[ i ];

    if ( a -> background ) continue;

    if ( unlikely( a -> wait_on_ready == 1 ) )
      break;

    if ( a -> ready() )
    {
      action = a;
      break;
    }
  }

  last_foreground_action = action;

  if ( restore_action_list != 0 )
  {
    activate_action_list( restore_action_list );
    restore_action_list = 0;
  }

  if ( action )
  {
    action -> line_cooldown.start();
    action -> schedule_execute();
    if ( ! action -> quiet )
    {
      iteration_executed_foreground_actions++;
      action -> total_executions++;

      sequence_add( action, action -> target, sim -> current_time );
    }
  }

  return action;
}

// player_t::regen ==========================================================

void player_t::regen( timespan_t periodicity )
{
  resource_e r = primary_resource();
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
      return;
  }

  if ( gain && base )
    resource_gain( r, base * periodicity.total_seconds(), gain );

}

// player_t::collect_resource_timeline_information ==========================

void player_t::collect_resource_timeline_information()
{
  for ( size_t j = 0, size = collected_data.resource_timelines.size(); j < size; ++j )
  {
    collected_data.resource_timelines[ j ].timeline.add( sim -> current_time,
        resources.current[ collected_data.resource_timelines[ j ].type ] );
  }

  for ( size_t j = 0, size = collected_data.stat_timelines.size(); j < size; ++j )
  {
    switch ( collected_data.stat_timelines[ j ].type )
    {
      case STAT_STRENGTH:
        collected_data.stat_timelines[ j ].timeline.add( sim -> current_time, cache.strength() );
        break;
      case STAT_AGILITY:
        collected_data.stat_timelines[ j ].timeline.add( sim -> current_time, cache.agility() );
        break;
      case STAT_INTELLECT:
        collected_data.stat_timelines[ j ].timeline.add( sim -> current_time, cache.intellect() );
        break;
      case STAT_SPELL_POWER:
        collected_data.stat_timelines[ j ].timeline.add( sim -> current_time, cache.spell_power( SCHOOL_NONE ) );
        break;
      case STAT_ATTACK_POWER:
        collected_data.stat_timelines[ j ].timeline.add( sim -> current_time, cache.attack_power() );
        break;
      default:
        collected_data.stat_timelines[ j ].timeline.add( sim -> current_time, 0 );
        break;
    }
  }
}

// player_t::resource_loss ==================================================

double player_t::resource_loss( resource_e resource_type,
                                double    amount,
                                gain_t*   source,
                                action_t* action )
{
  if ( amount == 0 )
    return 0;

  if ( current.sleeping )
    return 0;

  if ( resource_type == primary_resource() )
    uptimes.primary_resource_cap -> update( false, sim -> current_time );

  double actual_amount;

  if ( ! resources.is_infinite( resource_type ) || is_enemy() )
  {
    actual_amount = std::min( amount, resources.current[ resource_type ] );
    resources.current[ resource_type ] -= actual_amount;
    resource_lost[ resource_type ] += actual_amount;
  }
  else
  {
    actual_amount = amount;
    resources.current[ resource_type ] -= actual_amount;
    resource_lost[ resource_type ] += actual_amount;
  }

  if ( source )
  {
    source -> add( resource_type, actual_amount * -1, ( amount - actual_amount ) * -1 );
  }

  if ( resource_type == RESOURCE_MANA )
  {
    last_cast = sim -> current_time;
  }

  action_callback_t::trigger( callbacks.resource_loss[ resource_type ], action, ( void* ) &actual_amount );

  if ( sim -> debug )
    sim -> out_debug.printf( "Player %s loses %.2f (%.2f) %s. health pct: %.2f (%.0f/%.0f)",
                   name(), actual_amount, amount, util::resource_type_string( resource_type ), health_percentage(), resources.current[ resource_type ], resources.max[ resource_type ] );

  return actual_amount;
}

// player_t::resource_gain ==================================================

double player_t::resource_gain( resource_e resource_type,
                                double    amount,
                                gain_t*   source,
                                action_t* action )
{
  if ( current.sleeping )
    return 0;

  double actual_amount = std::min( amount, resources.max[ resource_type ] - resources.current[ resource_type ] );

  if ( actual_amount > 0 )
  {
    resources.current[ resource_type ] += actual_amount;
    resource_gained [ resource_type ] += actual_amount;
  }

  if ( resource_type == primary_resource() && resources.max[ resource_type ] <= resources.current[ resource_type ] )
    uptimes.primary_resource_cap -> update( true, sim -> current_time );

  if ( source )
  {
    source -> add( resource_type, actual_amount, amount - actual_amount );
  }

  action_callback_t::trigger( callbacks.resource_gain[ resource_type ], action, ( void* ) &actual_amount );

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
                                   double cost )
{
  if ( resource_type == RESOURCE_NONE || cost <= 0 || resources.is_infinite( resource_type ) )
  {
    return true;
  }

  return resources.current[ resource_type ] >= cost;
}

// player_t::recalculate_resources.max ======================================

void player_t::recalculate_resource_max( resource_e resource_type )
{
  // The first 20pts of intellect/stamina only provide 1pt of mana/health.

  resources.max[ resource_type ] = resources.base[ resource_type ] * resources.base_multiplier[ resource_type ] +
                                   gear.resource[ resource_type ] +
                                   enchant.resource[ resource_type ] +
                                   ( is_pet() ? 0 : sim -> enchant.resource[ resource_type ] );
  resources.max[ resource_type ] *= resources.initial_multiplier[ resource_type ];
  switch ( resource_type )
  {
    case RESOURCE_MANA:
    {
      break;
    }
    case RESOURCE_HEALTH:
    {
      double adjust = ( is_pet() || is_enemy() || is_add() ) ? 0 : std::min( 20, ( int ) floor( stamina() ) );
      resources.max[ resource_type ] += ( floor( stamina() ) - adjust ) * current.health_per_stamina + adjust;
      break;
    }
    default: break;
  }
}

// player_t::primary_role ===================================================

role_e player_t::primary_role() const
{
  return role;
}

// player_t::primary_tree_name ==============================================

const char* player_t::primary_tree_name()
{
  return dbc::specialization_string( specialization() ).c_str();
}

// player_t::normalize_by ===================================================

stat_e player_t::normalize_by()
{
  if ( sim -> normalized_stat != STAT_NONE )
    return sim -> normalized_stat;

  const std::string& so = this -> sim -> scaling -> scale_over;

  role_e role = primary_role();
  if ( role == ROLE_SPELL || role == ROLE_HEAL )
    return STAT_INTELLECT;
  else if ( role == ROLE_TANK && ( so == "tmi" || so == "dtps" || so == "dmg_taken" || so == "deaths" || so == "theck_meloree_index" ) )
    return STAT_STAMINA;
  else if ( type == DRUID || type == HUNTER || type == SHAMAN || type == ROGUE || type == MONK )
    return STAT_AGILITY;
  else if ( type == DEATH_KNIGHT || type == PALADIN || type == WARRIOR )
    return STAT_STRENGTH;

  return STAT_ATTACK_POWER;
}

// player_t::health_percentage() ============================================

double player_t::health_percentage()
{
  return resources.pct( RESOURCE_HEALTH ) * 100;
}

// target_t::time_to_die ====================================================

timespan_t player_t::time_to_die()
{
  // FIXME: Someone can figure out a better way to do this, for now, we NEED to
  // wait a minimum gcd before starting to estimate fight duration based on health,
  // otherwise very odd things happen with multi-actor simulations and time_to_die
  // expressions
  if ( iteration_dmg_taken > 0.0 && resources.base[ RESOURCE_HEALTH ] > 0 && sim -> current_time >= timespan_t::from_seconds( 1.0 ) )
  {
    return sim -> current_time * ( resources.current[ RESOURCE_HEALTH ] / iteration_dmg_taken );
  }
  else
  {
    return ( sim -> expected_time - sim -> current_time );
  }
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

  if ( sim -> log ) sim -> out_log.printf( "%s gains %.2f %s%s", name(), amount, util::stat_type_string( stat ), temporary_stat ? " (temporary)" : "" );

  int temp_value = temporary_stat ? 1 : 0;
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
    case STAT_DODGE_RATING:
    case STAT_PARRY_RATING:
    case STAT_BLOCK_RATING:
    case STAT_MASTERY_RATING:
      current.stats.add_stat( stat, amount );
      temporary.add_stat( stat, temp_value * amount );
      invalidate_cache( cache_from_stat( stat ) );
      break;

    case STAT_HASTE_RATING:
    {
      double old_attack_speed = 0;
      if ( main_hand_attack || off_hand_attack )
        old_attack_speed = cache.attack_speed();

      current.stats.add_stat( stat, amount );
      temporary.add_stat( stat, temp_value * amount );
      invalidate_cache( cache_from_stat( stat ) );

      if ( main_hand_attack )
        main_hand_attack -> reschedule_auto_attack( old_attack_speed );
      if ( off_hand_attack )
        off_hand_attack -> reschedule_auto_attack( old_attack_speed );
      break;
    }

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
      {
        temporary.attribute[ i ] += temp_value * amount;
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
      resources.max[ r ] += amount;
      resource_gain( r, amount, gain, action );
    }
    break;

    default: assert( false ); break;
  }

  switch ( stat )
  {
    case STAT_STAMINA: recalculate_resource_max( RESOURCE_HEALTH ); break;
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
    case STAT_DODGE_RATING:
    case STAT_PARRY_RATING:
    case STAT_BLOCK_RATING:
    case STAT_MASTERY_RATING:
      current.stats.add_stat( stat, -amount );
      temporary.add_stat( stat, temp_value * -amount );
      invalidate_cache( cache_from_stat( stat ) );
      break;

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
      {
        temporary.attribute[ i ] -= temp_value * amount;
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
      recalculate_resource_max( r );
      double delta = resources.current[ r ] - resources.max[ r ];
      if ( delta > 0 ) resource_loss( r, delta, gain, action );
    }
    break;

    case STAT_HASTE_RATING:
    {
      double old_attack_speed = 0;
      if ( main_hand_attack || off_hand_attack )
        old_attack_speed = cache.attack_speed();

      temporary.haste_rating -= temp_value * amount;
      current.stats.haste_rating   -= amount;
      invalidate_cache( CACHE_HASTE );

      if ( main_hand_attack )
        main_hand_attack -> reschedule_auto_attack( old_attack_speed );
      if ( off_hand_attack )
        off_hand_attack -> reschedule_auto_attack( old_attack_speed );
      break;
    }
    default: assert( false ); break;
  }

  switch ( stat )
  {
    case STAT_STAMINA: recalculate_resource_max( RESOURCE_HEALTH ); break;
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

// player_t::assess_damage ==================================================

void player_t::assess_damage( school_e school,
                              dmg_e    type,
                              action_state_t* s )
{

  if ( ! is_enemy() )
  {
    // Parry Haste accounting
    if ( s -> result == RESULT_PARRY )
    {
      if ( main_hand_attack && main_hand_attack -> execute_event )
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
        // (sim->current_time + x).  Thus we need to give it the difference between sim->current_time and the new target of execute_event->occurs().
        // That value is simply the remaining time on the current swing timer.

        // first, we need the hasted base swing timer, swing_time
        timespan_t swing_time = main_hand_attack -> time_to_execute;

        // and we also need the time remaining on the current swing timer
        timespan_t current_swing_timer = main_hand_attack -> execute_event -> occurs() - sim-> current_time;

        // next, check that the current swing timer is longer than 0.2*swing_time - if not we do nothing
        if ( current_swing_timer > 0.20 * swing_time )
        {
          // now we apply parry-hasting.  Subtract 40% of base swing timer from current swing timer
          current_swing_timer -= 0.40 * swing_time;

          // enforce 20% base swing timer minimum
          current_swing_timer = std::max( current_swing_timer, 0.20 * swing_time );

          // now reschedule the event and log a parry haste
          main_hand_attack -> reschedule_execute( current_swing_timer );
          procs.parry_haste -> occur();
        }
      }
    }
  }

  target_mitigation( school, type, s );

  // store post-mitigation, pre-absorb value
  s -> result_mitigated = s -> result_amount;
  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> out_debug.printf( "Damage to %s after all mitigation is %f", s -> target -> name(), s -> result_amount );


  if ( buffs.hand_of_sacrifice -> check() && s -> result_amount > 0 )
  {
    // figure out how much damage gets redirected
    double redirected_damage = s -> result_amount * ( buffs.hand_of_sacrifice -> data().effectN( 1 ).percent() );

    // apply that damage to the source paladin
    buffs.hand_of_sacrifice -> trigger( s -> action, 0, redirected_damage, timespan_t::zero() );

    // mitigate that amount from the target.
    // Slight inaccuracy: We do not get a feedback of paladin health buffer expiration here.
    s -> result_amount -= redirected_damage;

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Hand of Sacrifice is %f", s -> target -> name(), s -> result_amount );
  }

  /* ABSORB BUFFS
   *
   * std::vector<absorb_buff_t*> absorb_buff_list; is a dynamic vector, which contains
   * the currently active absorb buffs of a player.
   */
  size_t offset = 0;
  double result_ignoring_external_absorbs = s -> result_amount;

  while ( offset < absorb_buff_list.size() && s -> result_amount > 0 && ! absorb_buff_list.empty() )
  {
    absorb_buff_t* ab = absorb_buff_list[ offset ];

    if ( school != SCHOOL_NONE && ! dbc::is_school( ab -> absorb_school, school ) )
    {
      offset++;
      continue;
    }

    // Don't be too paranoid about inactive absorb buffs in the list. Just expire them
    if ( ab -> up() )
    {
      // Get absorb value of the buff
      double buff_value = ab -> current_value;
      double value = std::min( s -> result_amount, buff_value );

      ab -> consume( value );

      s -> result_amount -= value;
      // track result using only self-absorbs separately
      if ( ab -> source == this || is_my_pet( ab -> source ) )
      {
        result_ignoring_external_absorbs -= value;
        s -> self_absorb_amount += value;
      }
      

      if ( value < buff_value )
      {
        // Buff is not fully consumed
        assert( s -> result_amount == 0 );
        break;
      }

      if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() && buff_value > 0 )
        sim -> out_debug.printf( "Damage to %s after %s is %f", s -> target -> name(), ab -> name(), s -> result_amount );
    }

    ab -> expire();
    assert( absorb_buff_list.empty() || absorb_buff_list[ 0 ] != ab );
  } // end of absorb list loop

  s -> result_absorbed = s -> result_amount;

  assess_damage_imminent( school, type, s );

  // Legendary Tank Cloak Proc - max absorb of 1e7 hardcoded (in spellid 146193, effect 1)
  if ( ! is_enemy() && ! is_pet() && 
       ( items[ SLOT_BACK ].parsed.data.id == 102245 || items[ SLOT_BACK ].parsed.data.id == 102250 ) // a legendary tank cloak is present
       && legendary_tank_cloak_cd -> up()  // and the cloak's cooldown is up
       && s -> result_amount > resources.current[ RESOURCE_HEALTH ] ) // attack exceeds player health
  {
    if ( s -> result_amount > 1e7 )
    {
      gains.endurance_of_niuzao -> add( RESOURCE_HEALTH, 1e7, 0 );
      s -> result_amount -= 1e7;
      s -> result_absorbed += 1e7;
    }
    else
    {
      gains.endurance_of_niuzao -> add( RESOURCE_HEALTH, s -> result_amount, 0 );
      s -> result_absorbed += s -> result_amount;
      s -> result_amount = 0;
    }
    legendary_tank_cloak_cd -> start();      
  }

  iteration_dmg_taken += s -> result_amount;

  // collect data for timelines
  collected_data.timeline_dmg_taken.add( sim -> current_time, s -> result_amount );
  // tank-specific data storage
  if ( ! is_pet() && primary_role() == ROLE_TANK )
  {
    if ( tmi_self_only || sim -> tmi_actor_only )
    {
      collected_data.health_changes.timeline.add( sim -> current_time, result_ignoring_external_absorbs );
      collected_data.health_changes.timeline_normalized.add( sim -> current_time, result_ignoring_external_absorbs / resources.max[ RESOURCE_HEALTH ] );
    } 
    else
    {
      collected_data.health_changes.timeline.add( sim -> current_time, s -> result_amount );
      collected_data.health_changes.timeline_normalized.add( sim -> current_time, s -> result_amount / resources.max[ RESOURCE_HEALTH ] );
    }

    // store value in incoming damage array for conditionals
    incoming_damage.push_back( std::pair<timespan_t, double>( sim -> current_time, s -> result_amount ) );
  }

  double actual_amount = 0;
  if ( s -> result_amount > 0 ) actual_amount = resource_loss( RESOURCE_HEALTH, s -> result_amount, 0, s -> action );

  action_callback_t::trigger( callbacks.incoming_attack[ s -> result ], s -> action, s );

  if ( s -> result_amount <= 0 )
    return;

  else if ( health_percentage() <= death_pct && ! resources.is_infinite( RESOURCE_HEALTH ) )
  {
    // This can only save the target, if the damage is less than 200% of the target's health as of 4.0.6
    if ( ! is_enemy() && buffs.guardian_spirit -> check() && actual_amount <= ( resources.max[ RESOURCE_HEALTH] * 2 ) )
    {
      // Just assume that this is used so rarely that a strcmp hack will do
      //stats_t* stat = buffs.guardian_spirit -> source ? buffs.guardian_spirit -> source -> get_stats( "guardian_spirit" ) : 0;
      //double gs_amount = resources.max[ RESOURCE_HEALTH ] * buffs.guardian_spirit -> data().effectN( 2 ).percent();
      //resource_gain( RESOURCE_HEALTH, s -> result_amount );
      //if ( stat ) stat -> add_result( gs_amount, gs_amount, HEAL_DIRECT, RESULT_HIT );
      buffs.guardian_spirit -> expire();
    }
    else
    {
      if ( ! current.sleeping )
      {
        collected_data.deaths.add( sim -> current_time.total_seconds() );
      }
      if ( sim -> log ) sim -> out_log.printf( "%s has died.", name() );
      new ( *sim ) demise_event_t( *this );
    }
  }


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

  if ( buffs.stoneform && buffs.stoneform -> up() )
    s -> result_amount *= 1.0 + buffs.stoneform -> data().effectN( 1 ).percent();

  if ( buffs.fortitude && buffs.fortitude -> up() )
    s -> result_amount *= 1.0 + buffs.fortitude -> data().effectN( 1 ).percent();

  if ( school != SCHOOL_PHYSICAL )
  {
    if ( buffs.devotion_aura -> up() )
    {
      s -> result_amount *= 1.0 + buffs.devotion_aura -> data().effectN( 1 ).percent();
    }
  }
  
  if ( school == SCHOOL_PHYSICAL && dmg_type == DMG_DIRECT )
  {
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s before armor mitigation is %f", s -> target -> name(), s -> result_amount );

    // Armor
    if ( s -> action )
    {
      double resist = s -> action -> target_armor( this ) / ( s -> action -> target_armor( this ) + s -> action -> player -> current.armor_coeff );

      if ( resist < 0.0 )
        resist = 0.0;
      else if ( resist > 0.75 )
        resist = 0.75;
      s -> result_amount *= 1.0 - resist;
    }

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after armor mitigation is %f", s -> target -> name(), s -> result_amount );
    
    double pre_block_amount = s -> result_amount;

    if ( s -> block_result == BLOCK_RESULT_BLOCKED )
    {
      s -> result_amount *= ( 1 - composite_block_reduction() );
      if ( s -> result_amount <= 0 ) return;
    }

    if ( s -> block_result == BLOCK_RESULT_CRIT_BLOCKED )
    {
      s -> result_amount *= ( 1 - 2 * composite_block_reduction() );
      if ( s -> result_amount <= 0 ) return;
    }

    s -> blocked_amount = pre_block_amount  - s -> result_amount;

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() && s -> blocked_amount > 0.0)
      sim -> out_debug.printf( "Damage to %s after blocking is %f", s -> target -> name(), s -> result_amount );
  }
}

// player_t::assess_heal ====================================================

void player_t::assess_heal( school_e, dmg_e, heal_state_t* s )
{
  if ( buffs.guardian_spirit -> up() )
    s -> result_amount *= 1.0 + buffs.guardian_spirit -> data().effectN( 1 ).percent();

  s -> total_result_amount = s -> result_amount;
  s -> result_amount = resource_gain( RESOURCE_HEALTH, s -> result_amount, 0, s -> action );

  // if the target is a tank record this event on damage timeline
  if ( ! is_pet() && role == ROLE_TANK )
  {
    // tmi_self_only flag disables recording of external healing - use total_result_amount to ignore overhealing
    if ( ( tmi_self_only || sim -> tmi_actor_only ) && ( s -> action -> player == this || is_my_pet( s -> action -> player ) ) )
    {
      collected_data.timeline_healing_taken.add( sim -> current_time, - ( s -> total_result_amount ) );
      collected_data.health_changes.timeline.add( sim -> current_time, - ( s -> total_result_amount ) );
      collected_data.health_changes.timeline_normalized.add( sim -> current_time, - ( s -> total_result_amount ) / resources.max[ RESOURCE_HEALTH ] );
    }
    // otherwise just record everything, accounting for overheal
    else if ( ! tmi_self_only && ! sim -> tmi_actor_only )
    {
      collected_data.timeline_healing_taken.add( sim -> current_time, - ( s -> result_amount ) );
      collected_data.health_changes.timeline.add( sim -> current_time, - ( s -> result_amount ) );
      collected_data.health_changes.timeline_normalized.add( sim -> current_time, - ( s -> result_amount ) / resources.max[ RESOURCE_HEALTH ] );
    }
  }
  // store iteration heal taken
  iteration_heal_taken += s -> result_amount;
}

// player_t::summon_pet =====================================================

void player_t::summon_pet( const std::string& pet_name,
                           timespan_t  duration )
{
  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_t* p = pet_list[ i ];
    if ( p -> name_str == pet_name )
    {
      p -> summon( duration );
      return;
    }
  }
  sim -> errorf( "Player %s is unable to summon pet '%s'\n", name(), pet_name.c_str() );
}

// player_t::dismiss_pet ====================================================

void player_t::dismiss_pet( const std::string& pet_name )
{
  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_t* p = pet_list[ i ];
    if ( p -> name_str == pet_name )
    {
      p -> dismiss();
      return;
    }
  }
  assert( 0 );
}

// player_t::recent_cast ====================================================

bool player_t::recent_cast()
{
  return ( last_cast > timespan_t::zero() ) && ( ( last_cast + timespan_t::from_seconds( 5.0 ) ) > sim -> current_time );
}

// player_t::find_dot =======================================================

dot_t* player_t::find_dot( const std::string& name,
                           player_t* source )
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

// player_t::find_action_priority_list( const std::string& name ) ===========

action_priority_list_t* player_t::find_action_priority_list( const std::string& name )
{
  for ( size_t i = 0; i < action_priority_list.size(); i++ )
  {
    action_priority_list_t* a = action_priority_list[ i ];
    if ( a -> name_str == name )
      return a;
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

template <typename T>
T* find_vector_member( const std::vector<T*>& list, const std::string& name )
{
  for ( size_t i = 0; i < list.size(); ++i )
  {
    T* t = list[ i ];
    if ( t -> name_str == name )
      return t;
  }
  return nullptr;
}

// player_t::find_stats =====================================================

stats_t* player_t::find_stats( const std::string& name )
{ return find_vector_member( stats_list, name ); }

gain_t* player_t::find_gain ( const std::string& name )
{ return find_vector_member( gain_list, name ); }

proc_t* player_t::find_proc ( const std::string& name )
{ return find_vector_member( proc_list, name ); }

benefit_t* player_t::find_benefit ( const std::string& name )
{ return find_vector_member( benefit_list, name ); }

uptime_t* player_t::find_uptime ( const std::string& name )
{ return find_vector_member( uptime_list, name ); }

cooldown_t* player_t::find_cooldown( const std::string& name )
{ return find_vector_member( cooldown_list, name ); }

action_t* player_t::find_action( const std::string& name )
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

// player_t::get_position_distance ==========================================

double player_t::get_position_distance( double m, double v )
{
  // Square of Euclidean distance since sqrt() is slow
  double delta_x = this -> x_position - m;
  double delta_y = this -> y_position - v;
  return delta_x * delta_x + delta_y * delta_y;
}

// player_t::get_player_distance ============================================

double player_t::get_player_distance( player_t& p )
{
  return get_position_distance( p.x_position, p.y_position );
}

// player_t::get_action_priority_list( const std::string& name ) ============

action_priority_list_t* player_t::get_action_priority_list( const std::string& name, const std::string& comment )
{
  action_priority_list_t* a = find_action_priority_list( name );
  if ( ! a )
  {
    a = new action_priority_list_t( name, this );
    a -> action_list_comment_str = comment;
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
}

timespan_t wait_for_cooldown_t::execute_time()
{ assert( wait_cd -> duration > timespan_t::zero() ); return wait_cd -> remains(); }

namespace { // ANONYMOUS

// Chosen Movement Actions ==================================================

struct start_moving_t : public action_t
{
  start_moving_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "start_moving", player )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 0.5 );
    harmful = false;
  }

  virtual void execute()
  {
    player -> buffs.self_movement -> trigger();

    if ( sim -> log )
      sim -> out_log.printf( "%s starts moving.", player -> name() );

    update_ready();
  }

  virtual bool ready()
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
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 0.5 );
    harmful = false;
  }

  virtual void execute()
  {
    player -> buffs.self_movement -> expire();

    if ( sim -> log ) sim -> out_log.printf( "%s stops moving.", player -> name() );
    update_ready();
  }

  virtual bool ready()
  {
    if ( ! player -> buffs.self_movement -> check() )
      return false;

    return action_t::ready();
  }
};

// ===== Racial Abilities ===================================================

struct racial_spell_t : public spell_t
{
  racial_spell_t( player_t* p, const std::string token, const spell_data_t* spell, const std::string& options ) :
    spell_t( token, p, spell )
  {
    parse_options( NULL, options );
  }

  void init()
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

  void execute()
  {
    racial_spell_t::execute();

    player -> buffs.shadowmeld -> trigger();
  }
};

// Arcane Torrent ===========================================================

struct arcane_torrent_t : public racial_spell_t
{
  resource_e resource;
  double gain;

  arcane_torrent_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "arcane_torrent", p -> find_racial_spell( "Arcane Torrent" ), options_str ),
    resource( RESOURCE_NONE ), gain( 0 )
  {
    resource = util::translate_power_type( static_cast<power_e>( data().effectN( 2 ).misc_value1() ) );

    switch ( resource )
    {
      case RESOURCE_ENERGY:
      case RESOURCE_FOCUS:
      case RESOURCE_RAGE:
      case RESOURCE_RUNIC_POWER:
        gain = data().effectN( 2 ).resource( resource );
        break;
      default:
        break;
    }
  }

  virtual void execute()
  {
    if ( resource == RESOURCE_MANA )
      gain = player -> resources.max [ RESOURCE_MANA ] * data().effectN( 2 ).resource( resource );

    player -> resource_gain( resource, gain, player -> gains.arcane_torrent );

    racial_spell_t::execute();
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

  virtual void execute()
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

  virtual void execute()
  {
    racial_spell_t::execute();

    player -> buffs.blood_fury -> trigger();
  }
};

// Rocket Barrage ===========================================================

struct rocket_barrage_t : public racial_spell_t
{
  rocket_barrage_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "rocket_barrage", p -> find_racial_spell( "Rocket Barrage" ), options_str )
  {
    parse_options( NULL, options_str );

    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;
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

  virtual void execute()
  {
    racial_spell_t::execute();

    player -> buffs.stoneform -> trigger();
  }
};

// Lifeblood ================================================================

struct lifeblood_t : public spell_t
{
  lifeblood_t( player_t* player, const std::string& options_str ) :
    spell_t( "lifeblood", player )
  {
    parse_options( NULL, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 120 );
  }

  void init()
  {
    spell_t::init();

    if ( player -> profession[ PROF_HERBALISM ] < 450 )
    {
      sim -> errorf( "Player %s attempting to execute action %s without Herbalism.\n",
                     player -> name(), name() );

      background = true; // prevent action from being executed
    }
  }

  virtual void execute()
  {
    spell_t::execute();

    player -> buffs.lifeblood -> trigger();
  }

  virtual bool ready()
  {
    if ( player -> profession[ PROF_HERBALISM ] < 450 )
      return false;

    return spell_t::ready();
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
    option_t options[] =
    {
      opt_string( "name", seq_name_str ),
      opt_null()
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero();
  }

  virtual void init()
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

  virtual void execute()
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s restarting sequence %s", player -> name(), seq_name_str.c_str() );
    seq -> restart();
  }

  virtual bool ready()
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
    option_t options[] =
    {
      opt_float( "mana", mana ),
      opt_null()
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
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

  snapshot_stats_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "snapshot_stats", player ),
    completed( false )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    player_t* p = player;

    if ( completed ) return;

    completed = true;

    if ( sim -> current_iteration > 0 ) return;

    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", p -> name(), name() );

    for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; ++i )
      p -> buffed.attribute[ i ] = floor( p -> get_attribute( i ) );

    p -> buffed.resource     = p -> resources.max;

    p -> buffed.spell_haste  = p -> cache.spell_haste();
    p -> buffed.spell_speed  = p -> cache.spell_speed();
    p -> buffed.attack_haste = p -> cache.attack_haste();
    p -> buffed.attack_speed = p -> cache.attack_speed();
    p -> buffed.mastery_value = p -> cache.mastery_value();

    p -> buffed.spell_power  = util::round( p -> cache.spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier() );
    p -> buffed.spell_hit    = p -> cache.spell_hit();
    p -> buffed.spell_crit   = p -> cache.spell_crit();
    p -> buffed.manareg_per_second          = p -> mana_regen_per_second();

    p -> buffed.attack_power = p -> cache.attack_power() * p -> composite_attack_power_multiplier();
    p -> buffed.attack_hit   = p -> cache.attack_hit();
    p -> buffed.mh_attack_expertise = p -> composite_melee_expertise( &( p -> main_hand_weapon ) );
    p -> buffed.oh_attack_expertise = p -> composite_melee_expertise( &( p -> off_hand_weapon ) );
    p -> buffed.attack_crit  = p -> cache.attack_crit();

    p -> buffed.armor        = p -> composite_armor();
    p -> buffed.miss         = p -> composite_miss();
    p -> buffed.dodge        = p -> cache.dodge();
    p -> buffed.parry        = p -> cache.parry();
    p -> buffed.block        = p -> cache.block();
    p -> buffed.crit         = p -> cache.crit_avoidance();

    role_e role = p -> primary_role();
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
      spell_t* spell = new spell_t( "snapshot_spell", p  );
      spell -> background = true;
      spell -> init();
      double chance = spell -> miss_chance( spell -> composite_hit(), sim -> target );
      if ( chance < 0 ) spell_hit_extra = -chance * p -> current_rating().spell_hit;
    }

    if ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK )
    {
      attack_t* attack = new melee_attack_t( "snapshot_attack", p );
      attack -> background = true;
      attack -> init();
      double chance = attack -> miss_chance( attack -> composite_hit(), sim -> target );
      if ( p -> dual_wield() ) chance += 0.19;
      if ( chance < 0 ) attack_hit_extra = -chance * p -> current_rating().attack_hit;
      if ( p -> position() != POSITION_FRONT  )
      {
        chance = attack -> dodge_chance( p -> cache.attack_expertise(), sim -> target );
        if ( chance < 0 ) expertise_extra = -chance * p -> current_rating().expertise;
      }
      else if ( p ->position() == POSITION_FRONT )
      {
        chance = attack -> parry_chance( p -> cache.attack_expertise(), sim -> target );
        if ( chance < 0 ) expertise_extra = -chance * p -> current_rating().expertise;
      }
    }

    p -> over_cap[ STAT_HIT_RATING ] = std::max( spell_hit_extra, attack_hit_extra );
    p -> over_cap[ STAT_EXPERTISE_RATING ] = expertise_extra;

    for ( size_t i = 0; i < p -> pet_list.size(); ++i )
    {
      pet_t* pet = p -> pet_list[ i ];
      action_t* pet_snapshot = pet -> find_action( "snapshot_stats" );
      if ( ! pet_snapshot ) pet_snapshot = pet -> create_action( "snapshot_stats", "" );
      pet_snapshot -> execute();
    }
  }

  virtual void reset()
  {
    action_t::reset();

    completed = false;
  }

  virtual bool ready()
  {
    if ( completed || sim -> current_iteration > 0 ) return false;
    return action_t::ready();
  }
};

// Wait Fixed Action ========================================================

struct wait_fixed_t : public wait_action_base_t
{
  std::auto_ptr<expr_t> time_expr;

  wait_fixed_t( player_t* player, const std::string& options_str ) :
    wait_action_base_t( player, "wait" ),
    time_expr()
  {
    std::string sec_str = "1.0";

    option_t options[] =
    {
      opt_string( "sec", sec_str ),
      opt_null()
    };
    parse_options( options, options_str );

    time_expr = std::auto_ptr<expr_t>( expr_t::parse( this, sec_str ) );
  }

  virtual timespan_t execute_time()
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
  wait_until_ready_t( player_t* player, const std::string& options_str ) :
    wait_fixed_t( player, options_str )
  {}

  virtual timespan_t execute_time()
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
  spell_t* discharge;
  action_callback_t* trigger;
  stat_buff_t* buff;
  std::string use_name;

  use_item_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "use_item", player ),
    item( 0 ), discharge( 0 ), trigger( 0 ), buff( 0 )
  {
    std::string item_name, item_slot;

    option_t options[] =
    {
      opt_string( "name", item_name ),
      opt_string( "slot", item_slot ),
      opt_null()
    };
    parse_options( options, options_str );

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
        return;
      }

      item = &( player -> items[ s ] );

      if ( ! item -> active() )
      {
        sim -> errorf( "Player %s attempting 'use_item' action with invalid item '%s' in slot '%s'.", player -> name(), item -> name(), item_slot.c_str() );
        item = 0;
        return;
      }

      name_str = name_str + "_" + item -> name();
    }
    else
    {
      sim -> errorf( "Player %s has 'use_item' action with no 'name=' or 'slot=' option.\n", player -> name() );
      return;
    }

    if ( ! item -> parsed.use.active() )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which has no 'use=' encoding.\n", player -> name(), item -> name() );
      item = nullptr;
      return;
    }

    stats = player ->  get_stats( name_str, this );

    special_effect_t& e = item -> parsed.use;

    use_name = e.name_str.empty() ? item -> name() : e.name_str;

    if ( e.trigger_type )
    {
      if ( e.cost_reduction && e.school && e.discharge_amount )
        trigger = unique_gear::register_cost_reduction_proc( player, e );
      else if ( e.stat )
      {
        trigger = unique_gear::register_stat_proc( player, e );
      }
      else if ( e.school )
      {
        trigger = unique_gear::register_discharge_proc( player, e );
      }

      if ( trigger ) trigger -> deactivate();
    }
    else if ( e.school )
    {
      struct discharge_spell_t : public spell_t
      {
        discharge_spell_t( const char* n, player_t* p, double a, school_e s, unsigned int override_result_es_mask = 0, unsigned int result_es_mask = 0 ) :
          spell_t( n, p, spell_data_t::nil() )
        {
          school = s;
          trigger_gcd = timespan_t::zero();
          base_dd_min = a;
          base_dd_max = a;
          may_crit    = ( s != SCHOOL_DRAIN ) && ( ( override_result_es_mask & RESULT_CRIT_MASK ) ? ( result_es_mask & RESULT_CRIT_MASK ) : true ); // Default true
          may_miss    = ( override_result_es_mask & RESULT_MISS_MASK ) ? ( result_es_mask & RESULT_MISS_MASK ) != 0 : may_miss;
          background  = true;
          base_spell_power_multiplier = 0;
        }
      };

      discharge = new discharge_spell_t( use_name.c_str(), player, e.discharge_amount, e.school, e.override_result_es_mask, e.result_es_mask );
    }
    else if ( e.stat )
    {
      if ( e.max_stacks  == 0 ) e.max_stacks  = 1;
      if ( e.proc_chance == 0 ) e.proc_chance = 1;

      buff = dynamic_cast<stat_buff_t*>( buff_t::find( player, use_name, player ) );
      if ( ! buff )
        buff = stat_buff_creator_t( player, use_name ).max_stack( e.max_stacks )
               .duration( e.duration )
               .cd( e.cooldown )
               .chance( e.proc_chance )
               .reverse( e.reverse )
               .add_stat( e.stat, e.stat_amount );
    }
    else if ( e.execute_action )
    {
      assert( player == e.execute_action -> player ); // check if the action is from the same player. Might be overly strict
    }
    else assert( false );

    std::string cooldown_name = use_name;
    cooldown_name += "_";
    cooldown_name += item -> slot_name();

    cooldown = player -> get_cooldown( cooldown_name );
    cooldown -> duration = item -> parsed.use.cooldown;
    trigger_gcd = timespan_t::zero();

    if ( buff != 0 ) buff -> cooldown = cooldown;
  }

  void lockout( timespan_t duration )
  {
    if ( duration <= timespan_t::zero() ) return;

    player -> item_cooldown.start( duration );
  }

  virtual void execute()
  {
    if ( discharge )
    {
      discharge -> execute();
    }
    else if ( trigger )
    {
      if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), use_name.c_str() );

      trigger -> activate();

      if ( item -> parsed.use.duration != timespan_t::zero() )
      {
        struct trigger_expiration_t : public event_t
        {
          item_t* item;
          action_callback_t* trigger;

          trigger_expiration_t( player_t& player, item_t* i, action_callback_t* t ) :
            event_t( player, i -> name() ), item( i ), trigger( t )
          {
            sim().add_event( this, item -> parsed.use.duration );
          }
          virtual void execute()
          {
            trigger -> deactivate();
          }
        };

        new ( *sim ) trigger_expiration_t( *player, item, trigger );

        lockout( item -> parsed.use.duration );
      }
    }
    else if ( buff )
    {
      if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), use_name.c_str() );
      buff -> trigger();
      lockout( buff -> buff_duration );
    }
    else if ( action_t* a = item -> parsed.use.execute_action )
    {
      a -> execute();
    }
    else assert( false );

    // Enable to report use_item ability
    //if ( ! dual ) stats -> add_execute( time_to_execute );

    update_ready();
  }

  virtual void reset()
  {
    action_t::reset();
    if ( trigger ) trigger -> deactivate();
  }

  virtual bool ready()
  {
    if ( ! item ) return false;

    if ( player -> item_cooldown.remains() > timespan_t::zero() ) return false;

    return action_t::ready();
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
    option_t options[] =
    {
      opt_string( "name", buff_name ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( buff_name.empty() )
    {
      sim -> errorf( "Player %s uses cancel_buff without specifying the name of the buff\n", player -> name() );
      sim -> cancel();
    }

    buff = buff_t::find( player, buff_name );

    if ( ! buff )
    {
      sim -> errorf( "Player %s uses cancel_buff with unknown buff %s\n", player -> name(), buff_name.c_str() );
      sim -> cancel();
    }
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s cancels buff %s", player -> name(), buff -> name() );
    buff -> expire();
  }

  virtual bool ready()
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
    option_t options[] =
    {
      opt_string( "name", alist_name ),
      opt_null()
    };
    parse_options( options, options_str );

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

    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s swaps to action list %s", player -> name(), alist -> name_str.c_str() );
    player -> activate_action_list( alist );
  }

  virtual bool ready()
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
  }

  virtual void execute()
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
  double amount;

  pool_resource_t( player_t* p, const std::string& options_str, resource_e r = RESOURCE_NONE ) :
    action_t( ACTION_OTHER, "pool_resource", p ),
    resource( r != RESOURCE_NONE ? r : p -> primary_resource() ),
    wait( timespan_t::from_seconds( 0.251 ) ),
    for_next( 0 ),
    next_action( 0 ), amount( 0 )
  {
    option_t options[] =
    {
      opt_timespan( "wait", wait ),
      opt_bool( "for_next", for_next ),
      opt_string( "resource", resource_str ),
      opt_float( "extra_amount", amount ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( !resource_str.empty() )
    {
      resource_e r = util::parse_resource_type( resource_str );
      if ( r != RESOURCE_NONE )
        resource = r;
    }
  }

  virtual void reset()
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
        sim -> cancel();
      }
    }
  }

  virtual void execute()
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );
  }

  virtual timespan_t gcd()
  {
    return wait;
  }

  virtual bool ready()
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

      double theoretical_cost = next_action -> cost() + amount;
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
  if ( name == "shadowmeld"         ) return new         shadowmeld_t( this, options_str );
  if ( name == "cancel_buff"        ) return new        cancel_buff_t( this, options_str );
  if ( name == "swap_action_list"   ) return new   swap_action_list_t( this, options_str );
  if ( name == "run_action_list"    ) return new    run_action_list_t( this, options_str );
  if ( name == "lifeblood"          ) return new          lifeblood_t( this, options_str );
  if ( name == "restart_sequence"   ) return new   restart_sequence_t( this, options_str );
  if ( name == "restore_mana"       ) return new       restore_mana_t( this, options_str );
  if ( name == "rocket_barrage"     ) return new     rocket_barrage_t( this, options_str );
  if ( name == "sequence"           ) return new           sequence_t( this, options_str );
  if ( name == "snapshot_stats"     ) return new     snapshot_stats_t( this, options_str );
  if ( name == "start_moving"       ) return new       start_moving_t( this, options_str );
  if ( name == "stoneform"          ) return new          stoneform_t( this, options_str );
  if ( name == "stop_moving"        ) return new        stop_moving_t( this, options_str );
  if ( name == "use_item"           ) return new           use_item_t( this, options_str );
  if ( name == "wait"               ) return new         wait_fixed_t( this, options_str );
  if ( name == "wait_until_ready"   ) return new   wait_until_ready_t( this, options_str );
  if ( name == "pool_resource"      ) return new      pool_resource_t( this, options_str );

  return consumable::create_action( this, name, options_str );
}

// player_t::find_pet =======================================================

pet_t* player_t::find_pet( const std::string& pet_name )
{
  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_t* p = pet_list[ i ];
    if ( p -> name_str == pet_name )
      return p;
  }

  return 0;
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

  if ( t_str.size() < MAX_TALENT_ROWS )
  {
    sim -> errorf( "Player %s has malformed talent string '%s': talent list too short.\n",
                   name(), talent_string.c_str() );
    return false;
  }

  for ( size_t i = 0; i < MAX_TALENT_ROWS; ++i )
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

// player_t::replace_spells =================================================

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
      if ( s -> replace_spell_id() && ( ( int )s -> level() <= level ) )
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
      if ( talent_points.has_row_col( j, i ) && ( level >= ( ( j + 1 ) * 15 ) ) )
      {
        talent_data_t* td = talent_data_t::find( type, j, i, dbc.ptr );
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
      if ( s -> replace_spell_id() && ( ( int )s -> level() <= level ) )
      {
        // Found a spell we should replace
        dbc.replace_id( s -> replace_spell_id(), id );
      }
    }
  }
}


// player_t::find_talent_spell ==============================================

const spell_data_t* player_t::find_talent_spell( const std::string& n,
                                                 const std::string& token,
                                                 bool name_tokenized,
                                                 bool check_validity ) const
{
  // Get a talent's spell id for a given talent name
  unsigned spell_id = dbc.talent_ability_id( type, n.c_str(), name_tokenized );

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
      talent_data_t* td = talent_data_t::find( type, j, i, dbc.ptr );
      // Loop through all our classes talents, and check if their spell's id match the one we maped to the given talent name
      if ( td && ( td -> spell_id() == spell_id ) )
      {
        // check if we have the talent enabled or not
        if ( check_validity && ( ! talent_points.has_row_col( j, i ) || level < ( j + 1 ) * 15 ) )
          return spell_data_t::not_found();

        // We have that talent enabled.
        dbc.add_token( spell_id, token );

        return td -> spell();
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
    return dbc.spell( spell_id );
  else
    return spell_data_t::not_found();
}


// player_t::find_glyph_spell ===============================================

const spell_data_t* player_t::find_glyph_spell( const std::string& n, const std::string& token ) const
{
  if ( const spell_data_t* g = find_glyph( n ) )
  {
    for ( std::vector<const spell_data_t*>::const_iterator i = glyph_list.begin(); i != glyph_list.end(); ++i )
    {
      assert( *i );
      if ( ( *i ) -> id() == g -> id() )
      {
        if ( dbc::get_token( g -> id() ).empty() )
          dbc.add_token( g -> id(), token );
        return g;
      }
    }
  }

  return spell_data_t::not_found();
}

// player_t::find_specialization_spell ======================================

const spell_data_t* player_t::find_specialization_spell( const std::string& name, const std::string& token, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    if ( unsigned spell_id = dbc.specialization_ability_id( _spec, name.c_str() ) )
    {
      const spell_data_t* s = dbc.spell( spell_id );
      if ( ( ( int )s -> level() <= level ) )
      {
        if ( dbc::get_token( spell_id ).empty() )
          dbc.add_token( spell_id, token );

        return s;
      }
    }
  }

  return spell_data_t::not_found();
}

// player_t::find_mastery_spell =============================================

const spell_data_t* player_t::find_mastery_spell( specialization_e s, const std::string& token, uint32_t idx ) const
{
  if ( s != SPEC_NONE && s == _spec )
  {
    if ( unsigned spell_id = dbc.mastery_ability_id( s, idx ) )
    {
      const spell_data_t* s = dbc.spell( spell_id );
      if ( ( int )s -> level() <= level )
      {
        if ( dbc::get_token( spell_id ).empty() )
          dbc.add_token( spell_id, token );

        return dbc.spell( spell_id );
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
      return s;
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
      const spell_data_t* s = dbc.spell( spell_id );
      if ( s -> id() == spell_id && ( int )s -> level() <= level )
      {
        if ( dbc::get_token( spell_id ).empty() )
          dbc.add_token( spell_id, token );

        return s;
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
      return s;
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
    if ( s -> id() && ( int )s -> level() <= level )
    {
      if ( dbc::get_token( id ).empty() )
        dbc.add_token( id, token );
      return s;
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
  virtual double evaluate() { return ( 1 << player.position() ) & mask; }
};
}

// player_t::create_expression ==============================================

expr_t* player_t::create_expression( action_t* a,
                                     const std::string& name_str )
{
  if ( name_str == "level" )
    return make_ref_expr( "level", level );
  if ( name_str == "multiplier" )
  {
    struct multiplier_expr_t : public player_expr_t
    {
      action_t& action;
      multiplier_expr_t( player_t& p, action_t* a ) :
        player_expr_t( "multiplier", p ), action( *a ) { assert( a ); }
      virtual double evaluate() { return player.cache.player_multiplier( action.school ); }
    };
    return new multiplier_expr_t( *this, a );
  }
  if ( name_str == "in_combat" )
    return make_ref_expr( "in_combat", in_combat );
  if ( name_str == "attack_haste" )
    return make_mem_fn_expr( name_str, this-> cache, &player_stat_cache_t::attack_haste );
  if ( name_str == "attack_speed" )
    return make_mem_fn_expr( name_str, this -> cache, &player_stat_cache_t::attack_speed );
  if ( name_str == "spell_haste" )
    return make_mem_fn_expr( name_str, this-> cache, &player_stat_cache_t::spell_speed );
  if ( name_str == "time_to_die" )
    return make_mem_fn_expr( name_str, *this, &player_t::time_to_die );

  if ( name_str == "health_pct" )
    return deprecate_expression( this, a, name_str, "health.pct" );

  if ( name_str == "mana_pct" )
    return deprecate_expression( this, a, name_str, "mana.pct" );

  if ( name_str == "energy_regen" )
    return deprecate_expression( this, a, name_str, "energy.regen" );

  if ( name_str == "focus_regen" )
    return deprecate_expression( this, a, name_str, "focus.regen" );

  if ( name_str == "time_to_max_energy" )
    return deprecate_expression( this, a, name_str, "energy.time_to_max" );

  if ( name_str == "time_to_max_focus" )
    return deprecate_expression( this, a, name_str, "focus.time_to_max" );

  if ( name_str == "max_mana_nonproc" )
    return deprecate_expression( this, a, name_str, "mana.max_nonproc" );

  if ( name_str == "ptr" )
    return make_ref_expr( "ptr", dbc.ptr );

  if ( name_str == "position_front" )
    return new position_expr_t( "position_front", *this,
                                ( 1 << POSITION_FRONT ) | ( 1 << POSITION_RANGED_FRONT ) );
  if ( name_str == "position_back" )
    return new position_expr_t( "position_back", *this,
                                ( 1 << POSITION_BACK ) | ( 1 << POSITION_RANGED_BACK ) );

  if ( name_str == "mastery_value" )
    return  make_mem_fn_expr( name_str, this-> cache, &player_stat_cache_t::mastery_value );

  if ( expr_t* q = create_resource_expression( name_str ) )
    return q;

  // time_to_bloodlust conditional
  if ( name_str == "time_to_bloodlust" )
  {
    struct time_to_bloodlust_expr_t : public expr_t
    {
      player_t* player;

      time_to_bloodlust_expr_t( player_t* p, const std::string& name ) : 
        expr_t( name ), player( p )
      {
      }

      double evaluate()
      {
        return player -> calculate_time_to_bloodlust();
      }
    };

    return new time_to_bloodlust_expr_t( this, name_str );
  }

  // incoming_damage_X expressions
  if ( util::str_in_str_ci( name_str, "incoming_damage_" ) )
  {
    std::vector<std::string> parts = util::string_split( name_str, "_" );
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

        double evaluate()
        {
          return player -> compute_incoming_damage( duration );
        }
      };

      return new inc_dmg_expr_t( this, parts[ 2 ], window_duration );
    }
  }

  std::vector<std::string> splits = util::string_split( name_str, "." );

  // trinket.[12.].(has_|)(stacking_|)proc.<stat>.<buff_expr>
  if ( splits[ 0 ] == "trinket" && splits.size() >= 3 )
  {
    enum proc_expr_e
    {
      PROC_EXISTS,
      PROC_ENABLED
    };

    enum proc_type_e
    {
      PROC_STAT,
      PROC_STACKING_STAT,
      PROC_ICD,
      PROC_STACKING_ICD
    };

    bool trinket1 = false, trinket2 = false;
    int ptype_idx = 1, stat_idx = 2, expr_idx = 3;
    enum proc_expr_e pexprtype = PROC_ENABLED;
    enum proc_type_e ptype = PROC_STAT;
    stat_e stat = STAT_NONE;

    if ( util::is_number( splits[ 1 ] ) )
    {
      if ( splits[ 1 ] == "1" )
        trinket1 = true;
      else if ( splits[ 1 ] == "2" )
        trinket2 = true;
      else
        return sim -> create_expression( a, name_str );
      stat_idx++;
      ptype_idx++;
      expr_idx++;
    }

    // No positional parameter given so check both trinkets
    if ( ! trinket1 && ! trinket2 )
      trinket1 = trinket2 = true;

    if ( util::str_prefix_ci( splits[ ptype_idx ], "has_" ) )
      pexprtype = PROC_EXISTS;

    if ( util::str_in_str_ci( splits[ ptype_idx ], "stacking_" ) )
      ptype = PROC_STACKING_STAT;

    stat = util::parse_stat_type( splits[ stat_idx ] );
    if ( stat == STAT_NONE )
      return sim -> create_expression( a, name_str );

    if ( pexprtype == PROC_ENABLED && splits.size() >= 4 )
    {
      struct trinket_proc_expr_t : public expr_t
      {
        expr_t* bexpr1;
        expr_t* bexpr2;

        trinket_proc_expr_t( action_t* a, stat_e s, bool t1, bool t2, bool stacking, const std::string& expr ) :
          expr_t( "trinket_proc" ), bexpr1( 0 ), bexpr2( 0 )
        {
          if ( t1 )
          {
            const special_effect_t& e = a -> player -> items[ SLOT_TRINKET_1 ].parsed.equip;
            if ( e.stat == s && ( ( ! stacking && e.max_stacks <= 1 ) || ( stacking && e.max_stacks > 1 ) ) )
            {
              buff_t* b1 = buff_t::find( a -> player, e.name_str );
              if ( b1 ) bexpr1 = buff_t::create_expression( b1 -> name(), a, expr, b1 );
            }
          }

          if ( t2 )
          {
            const special_effect_t& e = a -> player -> items[ SLOT_TRINKET_2 ].parsed.equip;
            if ( e.stat == s && ( ( ! stacking && e.max_stacks <= 1 ) || ( stacking && e.max_stacks > 1 ) ) )
            {
              buff_t* b2 = buff_t::find( a -> player, e.name_str );
              if ( b2 ) bexpr2 = buff_t::create_expression( b2 -> name(), a, expr, b2 );
            }
          }
        }

        double evaluate()
        {
          double result = 0;

          if ( bexpr1 )
            result = bexpr1 -> eval();

          if ( bexpr2 )
          {
            double b2result = bexpr2 -> eval();
            if ( b2result > result )
              result = b2result;
          }

          return result;
        }
      };

      return new trinket_proc_expr_t( a, stat, trinket1, trinket2, ptype == PROC_STACKING_STAT, splits[ expr_idx ] );
    }
    else if ( pexprtype == PROC_EXISTS )
    {
      struct trinket_proc_exists_expr_t : public expr_t
      {
        bool has_t1;
        bool has_t2;

        trinket_proc_exists_expr_t( player_t* p, stat_e s, bool t1, bool t2 ) :
          expr_t( "trinket_proc_exists" ), has_t1( false ), has_t2( false )
        {
          if ( t1 )
          {
            const special_effect_t& e = p -> items[ SLOT_TRINKET_1 ].parsed.equip;
            if ( e.stat == s ) has_t1 = true;
          }

          if ( t2 )
          {
            const special_effect_t& e = p -> items[ SLOT_TRINKET_2 ].parsed.equip;
            if ( e.stat == s ) has_t2 = true;
          }
        }

        double evaluate()
        { return has_t1 || has_t2; }
      };

      return new trinket_proc_exists_expr_t( this, stat, trinket1, trinket2 );
    }
  }

  if ( splits[ 0 ] == "race" && splits.size() == 2 )
  {
    struct race_expr_t : public expr_t
    {
      player_t& player;
      std::string race_name;
      race_expr_t( player_t& p, const std::string& n ) :
        expr_t( "race" ), player( p ), race_name( n )
      { }
      virtual double evaluate() { return player.race_str == race_name; }
    };
    return new race_expr_t( *this, splits[ 1 ] );
  }

  if ( splits[ 0 ] == "pet" && splits.size() == 3 )
  {
    struct pet_expr_t : public expr_t
    {
      pet_t& pet;
      pet_expr_t( const std::string& name, pet_t& p ) :
        expr_t( name ), pet( p ) {}
    };

    pet_t* pet = find_pet( splits[ 1 ] );
    if ( ! pet )
    {
      // FIXME: report pet not found?
      return 0;
    }

    if ( splits[ 2 ] == "active" )
    {
      struct pet_active_expr_t : public pet_expr_t
      {
        pet_active_expr_t( pet_t& p ) : pet_expr_t( "pet_active", p ) {}
        virtual double evaluate() { return ! pet.current.sleeping; }
      };
      return new pet_active_expr_t( *pet );
    }

    else if ( splits[ 2 ] == "remains" )
    {
      struct pet_remains_expr_t : public pet_expr_t
      {
        pet_remains_expr_t( pet_t& p ) : pet_expr_t( "pet_remains", p ) {}
        virtual double evaluate()
        {
          if ( pet.expiration && pet.expiration-> remains() > timespan_t::zero() )
            return pet.expiration -> remains().total_seconds();
          else
            return 0;
        }
      };
      return new pet_remains_expr_t( *pet );
    }

    return pet -> create_expression( a, name_str.substr( splits[ 1 ].length() + 5 ) );
  }

  else if ( splits[ 0 ] == "owner" )
  {
    if ( pet_t* pet = dynamic_cast<pet_t*>( this ) )
    {
      if ( pet -> owner )
        return pet -> owner -> create_expression( a, name_str.substr( 6 ) );
    }
    // FIXME: report failure.
  }

  else if ( splits[ 0 ] == "temporary_bonus" && splits.size() == 2 )
  {
    stat_e stat = util::parse_stat_type( splits[ 1 ] );
    switch ( stat )
    {
      case STAT_STRENGTH:
      case STAT_AGILITY:
      case STAT_STAMINA:
      case STAT_INTELLECT:
      case STAT_SPIRIT:
      {
        struct temp_attr_expr_t : public player_expr_t
        {
          attribute_e attr;
          temp_attr_expr_t( const std::string& name, player_t& p, attribute_e a ) :
            player_expr_t( name, p ), attr( a ) {}
          virtual double evaluate()
          { return player.temporary.attribute[ attr ] * player.composite_attribute_multiplier( attr ); }
        };
        return new temp_attr_expr_t( name_str, *this, static_cast<attribute_e>( stat ) );
      }

      case STAT_SPELL_POWER:
      {
        struct temp_sp_expr_t : player_expr_t
        {
          temp_sp_expr_t( const std::string& name, player_t& p ) :
            player_expr_t( name, p ) {}
          virtual double evaluate()
          {
            return ( player.temporary.spell_power +
                     player.temporary.attribute[ ATTR_INTELLECT ] *
                     player.composite_attribute_multiplier( ATTR_INTELLECT ) *
                     player.current.spell_power_per_intellect ) *
                   player.composite_spell_power_multiplier();
          }
        };
        return new temp_sp_expr_t( name_str, *this );
      }

      case STAT_ATTACK_POWER:
      {
        struct temp_ap_expr_t : player_expr_t
        {
          temp_ap_expr_t( const std::string& name, player_t& p ) :
            player_expr_t( name, p ) {}
          virtual double evaluate()
          {
            return ( player.temporary.attack_power +
                     player.temporary.attribute[ ATTR_STRENGTH ] *
                     player.composite_attribute_multiplier( ATTR_STRENGTH ) *
                     player.current.attack_power_per_strength +
                     player.temporary.attribute[ ATTR_AGILITY ] *
                     player.composite_attribute_multiplier( ATTR_AGILITY ) *
                     player.current.attack_power_per_agility ) *
                   player.composite_attack_power_multiplier();
          }
        };
        return new temp_ap_expr_t( name_str, *this );
      }

      case STAT_EXPERTISE_RATING: return make_ref_expr( name_str, temporary.expertise_rating );
      case STAT_HIT_RATING:       return make_ref_expr( name_str, temporary.hit_rating );
      case STAT_CRIT_RATING:      return make_ref_expr( name_str, temporary.crit_rating );
      case STAT_HASTE_RATING:     return make_ref_expr( name_str, temporary.haste_rating );
      case STAT_ARMOR:            return make_ref_expr( name_str, temporary.armor );
      case STAT_DODGE_RATING:     return make_ref_expr( name_str, temporary.dodge_rating );
      case STAT_PARRY_RATING:     return make_ref_expr( name_str, temporary.parry_rating );
      case STAT_BLOCK_RATING:     return make_ref_expr( name_str, temporary.block_rating );
      case STAT_MASTERY_RATING:   return make_ref_expr( name_str, temporary.mastery_rating );
      default: break;
    }

    // FIXME: report error and return?
  }

  else if ( splits[ 0 ] == "stat" && splits.size() == 2 )
  {
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
          virtual double evaluate()
          { return player.cache.get_attribute( attr ); }
        };
        return new attr_expr_t( name_str, *this, static_cast<attribute_e>( stat ) );
      }

      case STAT_SPELL_POWER:
      {
        struct sp_expr_t : player_expr_t
        {
          sp_expr_t( const std::string& name, player_t& p ) :
            player_expr_t( name, p ) {}
          virtual double evaluate()
          {
            return player.cache.spell_power( SCHOOL_MAX ) * player.composite_spell_power_multiplier();
          }
        };
        return new sp_expr_t( name_str, *this );
      }

      case STAT_ATTACK_POWER:
      {
        struct ap_expr_t : player_expr_t
        {
          ap_expr_t( const std::string& name, player_t& p ) :
            player_expr_t( name, p ) {}
          virtual double evaluate()
          {
            return player.cache.attack_power() * player.composite_attack_power_multiplier();
          }
        };
        return new ap_expr_t( name_str, *this );
      }

      case STAT_EXPERTISE_RATING: return make_mem_fn_expr( name_str, *this, &player_t::composite_expertise_rating );
      case STAT_HIT_RATING:       return make_mem_fn_expr( name_str, *this, &player_t::composite_melee_hit_rating );
      case STAT_CRIT_RATING:      return make_mem_fn_expr( name_str, *this, &player_t::composite_melee_crit_rating );
      case STAT_HASTE_RATING:     return make_mem_fn_expr( name_str, *this, &player_t::composite_melee_haste_rating );
      case STAT_ARMOR:            return make_ref_expr( name_str, current.stats.armor );
      case STAT_DODGE_RATING:     return make_mem_fn_expr( name_str, *this, &player_t::composite_dodge_rating );
      case STAT_PARRY_RATING:     return make_mem_fn_expr( name_str, *this, &player_t::composite_parry_rating );
      case STAT_BLOCK_RATING:     return make_mem_fn_expr( name_str, *this, &player_t::composite_block_rating );
      case STAT_MASTERY_RATING:   return make_mem_fn_expr( name_str, *this, &player_t::composite_mastery_rating );
      default: break;
    }

    // FIXME: report error and return?
  }

  else if ( splits.size() == 3 )
  {
    if ( splits[ 0 ] == "buff" || splits[ 0 ] == "debuff" )
    {
      a -> player -> get_target_data( this );
      buff_t* buff = buff_t::find( this, splits[ 1 ], a -> player );
      if ( ! buff ) buff = buff_t::find( this, splits[ 1 ], this ); // Raid debuffs
      if ( buff ) return buff_t::create_expression( splits[ 1 ], a, splits[ 2 ], buff );
    }
    else if ( splits[ 0 ] == "cooldown" )
    {
      cooldown_t* cooldown = get_cooldown( splits[ 1 ] );
      if ( cooldown && splits[ 2 ] == "remains" )
        return make_mem_fn_expr( name_str, *cooldown, &cooldown_t::remains );
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
          virtual double evaluate()
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
        virtual double evaluate() { return player.find_spell( name ) -> ok(); }
      };
      return new spell_exists_expr_t( splits[ 1 ], *this );
    }
  }
  else if ( splits.size() == 2 )
  {
    if ( splits[ 0 ] == "set_bonus" )
      return set_bonus.create_expression( this, splits[ 1 ] );
  }

  if ( splits.size() >= 2 && splits[ 0 ] == "target" )
  {
    std::string rest = splits[1];
    for ( size_t i = 2; i < splits.size(); ++i )
      rest += '.' + splits[ i ];
    return target -> create_expression( a, rest );
  }

  else if ( ( splits.size() == 3 ) && ( ( splits[ 0 ] == "glyph" ) || ( splits[ 0 ] == "talent" ) ) )
  {
    struct s_expr_t : public player_expr_t
    {
      spell_data_t* s;

      s_expr_t( const std::string& name, player_t& p, spell_data_t* sp ) :
        player_expr_t( name, p ), s( sp ) {}
      virtual double evaluate()
      { return ( s && s -> ok() ); }
    };

    if ( splits[ 2 ] != "enabled"  )
    {
      return 0;
    }

    spell_data_t* s;

    if ( splits[ 0 ] == "glyph" )
    {
      s = const_cast< spell_data_t* >( find_glyph_spell( splits[ 1 ] ) );
    }
    else
    {
      s = const_cast< spell_data_t* >( find_talent_spell( splits[ 1 ], std::string(), true ) );
    }

    return new s_expr_t( name_str, *this, s );
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
          virtual double evaluate()
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
          virtual double evaluate()
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

  return sim -> create_expression( a, name_str );
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
        virtual double evaluate()
        { return player.resources.max[ rt ] - player.resources.current[ rt ]; }
      };
      return new resource_deficit_expr_t( name_str, *this, r );
    }

    else if ( splits[ 1 ] == "pct" || splits[ 1 ] == "percent" )
    {
      struct resource_pct_expr_t : public resource_expr_t
      {
        resource_pct_expr_t( const std::string& n, player_t& p, resource_e r  ) :
          resource_expr_t( n, p, r ) {}
        virtual double evaluate()
        { return player.resources.pct( rt ) * 100.0; }
      };
      return new resource_pct_expr_t( name_str, *this, r  );
    }

    else if ( splits[ 1 ] == "max" )
      return make_ref_expr( name_str, resources.max[ r ] );

    else if ( splits[ 1 ] == "max_nonproc" )
      return make_ref_expr( name_str, buffed.resource[ r ] );

    else if ( splits[ 1 ] == "pct_nonproc" )
    {
      struct resource_pct_nonproc_expr_t : public resource_expr_t
      {
        resource_pct_nonproc_expr_t( const std::string& n, player_t& p, resource_e r ) :
          resource_expr_t( n, p, r ) {}
        virtual double evaluate()
        { return player.resources.current[ rt ] / player.buffed.resource[ rt ] * 100.0; }
      };
      return new resource_pct_nonproc_expr_t( name_str, *this, r );
    }
    else if ( splits[ 1 ] == "net_regen" )
    {
      struct resource_net_regen_expr_t : public resource_expr_t
      {
        resource_net_regen_expr_t( const std::string& n, player_t& p, resource_e r ) :
          resource_expr_t( n, p, r ) {}
        virtual double evaluate()
        {
          timespan_t now = player.sim -> current_time;
          if ( now != timespan_t::zero() )
            return ( player.resource_gained[ rt ] - player.resource_lost[ rt ] ) / now.total_seconds();
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
    }

    else if ( splits[ 1 ] == "time_to_max" )
    {
      if ( r == RESOURCE_ENERGY )
      {
        struct time_to_max_energy_expr_t : public resource_expr_t
        {
          time_to_max_energy_expr_t( player_t& p, resource_e r ) :
            resource_expr_t( "time_to_max_energy", p, r ) {}
          virtual double evaluate()
          {
            return ( player.resources.max[ RESOURCE_ENERGY ] -
                     player.resources.current[ RESOURCE_ENERGY ] ) /
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
            resource_expr_t( "time_to_max_focus", p, r ) {}
          virtual double evaluate()
          {
            return ( player.resources.max[ RESOURCE_FOCUS ] -
                     player.resources.current[ RESOURCE_FOCUS ] ) /
                   player.focus_regen_per_second();
          }
        };
        return new time_to_max_focus_expr_t( *this, r );
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
      if ( sim -> current_time - ( *i ).first > interval )
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

    // first, check bloodlust_time.  If it's >0, we just compare to current_time
    // if it's <0, then use time_to_die estimate
    if ( sim -> bloodlust_time > timespan_t::zero() )
      time_to_bl = sim -> bloodlust_time - sim -> current_time;
    else if ( sim -> bloodlust_time < timespan_t::zero() )
      time_to_bl = target -> time_to_die() + sim -> bloodlust_time;

    // check bloodlust_percent, if >0 then we need to estimate time based on time_to_die and health_percentage
    if ( sim -> bloodlust_percent > 0 && target -> health_percentage() > 0 )
      bl_pct_time = ( target -> health_percentage() - sim -> bloodlust_percent ) * target -> time_to_die() / target -> health_percentage();
    
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
  return 3 * sim -> expected_time.total_seconds();
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

bool player_t::create_profile( std::string& profile_str, save_e stype, bool save_html )
{
  std::string term;

  if ( save_html )
    term = "<br>\n";
  else
    term = "\n";

  if ( stype == SAVE_ALL )
  {
    profile_str += "#!./simc " + term + term;
  }

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
    profile_str += "level=" + util::to_string( level ) + term;
    profile_str += "race=" + race_str + term;
    profile_str += "role=";
    profile_str += util::role_type_string( primary_role() ) + term;
    profile_str += "position=" + position_str + term;

    if ( professions_str.size() > 0 )
    {
      profile_str += "professions=" + professions_str + term;
    }

    if ( sim -> tmi_actor_only )
      profile_str += "tmi_self_only=1\n";

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

    if ( glyphs_str.size() > 0 )
    {
      profile_str += "glyphs=" + glyphs_str + term;
    }
  }

  if ( stype == SAVE_ALL )
  {
    profile_str += "spec=";
    profile_str += dbc::specialization_string( specialization() ) + term;
  }

  if ( stype == SAVE_ALL || stype == SAVE_ACTIONS )
  {
    if ( action_list_str.size() > 0 || action_list_default == 1 )
    {
      // If we created a default action list, add comments
      if ( no_action_list_provided )
        profile_str += action_list_information;

      int j = 0;
      std::string alist_str = "";
      for ( size_t i = 0; i < action_list.size(); ++i )
      {
        action_t* a = action_list[ i ];
        if ( a -> signature_str.empty() ) continue;
        if ( a -> action_list != alist_str )
        {
          j = 0;
          alist_str = a -> action_list;
          const action_priority_list_t* alist = get_action_priority_list( alist_str );
          if ( ! alist -> action_list_comment_str.empty() )
            profile_str += term + "# " + alist -> action_list_comment_str + term;
          profile_str += term;
        }

        const std::string& encoded_action = a -> signature -> action_;
        const std::string& encoded_comment = a -> signature -> comment_;

        if ( ! encoded_comment.empty() )
          profile_str += "# " + ( save_html ? util::encode_html( encoded_comment ) : encoded_comment ) + term;
        profile_str += "actions";
        if ( ! a -> action_list.empty() && a -> action_list != "default" )
          profile_str += "." + a -> action_list;
        profile_str += j ? "+=/" : "=";
        if ( save_html )
        {
          util::encode_html( encoded_action );
        }
        profile_str += encoded_action + term;
        j++;
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

    for ( int i = 0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ SLOT_OUT_ORDER[ i ] ];

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
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = total_gear.get_stat( i );
      if ( value != 0 )
      {
        profile_str += "# gear_";
        profile_str += util::stat_type_string( i );
        profile_str += "=" + util::to_string( value, 0 ) + term;
      }
    }
    if ( meta_gem != META_GEM_NONE )
    {
      profile_str += "# meta_gem=";
      profile_str += util::meta_gem_type_string( meta_gem );
      profile_str += term;
    }

    if ( set_bonus.tier13_2pc_caster() ) profile_str += "# tier13_2pc_caster=1" + term;
    if ( set_bonus.tier13_4pc_caster() ) profile_str += "# tier13_4pc_caster=1" + term;
    if ( set_bonus.tier13_2pc_melee()  ) profile_str += "# tier13_2pc_melee=1" + term;
    if ( set_bonus.tier13_4pc_melee()  ) profile_str += "# tier13_4pc_melee=1" + term;
    if ( set_bonus.tier13_2pc_tank()   ) profile_str += "# tier13_2pc_tank=1" + term;
    if ( set_bonus.tier13_4pc_tank()   ) profile_str += "# tier13_4pc_tank=1" + term;
    if ( set_bonus.tier13_2pc_heal()   ) profile_str += "# tier13_2pc_heal=1" + term;
    if ( set_bonus.tier13_4pc_heal()   ) profile_str += "# tier13_4pc_heal=1" + term;

    if ( set_bonus.tier14_2pc_caster() ) profile_str += "# tier14_2pc_caster=1" + term;
    if ( set_bonus.tier14_4pc_caster() ) profile_str += "# tier14_4pc_caster=1" + term;
    if ( set_bonus.tier14_2pc_melee()  ) profile_str += "# tier14_2pc_melee=1" + term;
    if ( set_bonus.tier14_4pc_melee()  ) profile_str += "# tier14_4pc_melee=1" + term;
    if ( set_bonus.tier14_2pc_tank()   ) profile_str += "# tier14_2pc_tank=1" + term;
    if ( set_bonus.tier14_4pc_tank()   ) profile_str += "# tier14_4pc_tank=1" + term;
    if ( set_bonus.tier14_2pc_heal()   ) profile_str += "# tier14_2pc_heal=1" + term;
    if ( set_bonus.tier14_4pc_heal()   ) profile_str += "# tier14_4pc_heal=1" + term;

    if ( set_bonus.tier15_2pc_caster() ) profile_str += "# tier15_2pc_caster=1" + term;
    if ( set_bonus.tier15_4pc_caster() ) profile_str += "# tier15_4pc_caster=1" + term;
    if ( set_bonus.tier15_2pc_melee()  ) profile_str += "# tier15_2pc_melee=1" + term;
    if ( set_bonus.tier15_4pc_melee()  ) profile_str += "# tier15_4pc_melee=1" + term;
    if ( set_bonus.tier15_2pc_tank()   ) profile_str += "# tier15_2pc_tank=1" + term;
    if ( set_bonus.tier15_4pc_tank()   ) profile_str += "# tier15_4pc_tank=1" + term;
    if ( set_bonus.tier15_2pc_heal()   ) profile_str += "# tier15_2pc_heal=1" + term;
    if ( set_bonus.tier15_4pc_heal()   ) profile_str += "# tier15_4pc_heal=1" + term;

    if ( set_bonus.tier16_2pc_caster() ) profile_str += "# tier16_2pc_caster=1" + term;
    if ( set_bonus.tier16_4pc_caster() ) profile_str += "# tier16_4pc_caster=1" + term;
    if ( set_bonus.tier16_2pc_melee()  ) profile_str += "# tier16_2pc_melee=1" + term;
    if ( set_bonus.tier16_4pc_melee()  ) profile_str += "# tier16_4pc_melee=1" + term;
    if ( set_bonus.tier16_2pc_tank()   ) profile_str += "# tier16_2pc_tank=1" + term;
    if ( set_bonus.tier16_4pc_tank()   ) profile_str += "# tier16_4pc_tank=1" + term;
    if ( set_bonus.tier16_2pc_heal()   ) profile_str += "# tier16_2pc_heal=1" + term;
    if ( set_bonus.tier16_4pc_heal()   ) profile_str += "# tier16_4pc_heal=1" + term;

    if ( set_bonus.pvp_2pc_caster() ) profile_str += "# pvp_2pc_caster=1" + term;
    if ( set_bonus.pvp_4pc_caster() ) profile_str += "# pvp_4pc_caster=1" + term;
    if ( set_bonus.pvp_2pc_melee()  ) profile_str += "# pvp_2pc_melee=1" + term;
    if ( set_bonus.pvp_4pc_melee()  ) profile_str += "# pvp_4pc_melee=1" + term;
    if ( set_bonus.pvp_2pc_tank()   ) profile_str += "# pvp_2pc_tank=1" + term;
    if ( set_bonus.pvp_4pc_tank()   ) profile_str += "# pvp_4pc_tank=1" + term;
    if ( set_bonus.pvp_2pc_heal()   ) profile_str += "# pvp_2pc_heal=1" + term;
    if ( set_bonus.pvp_4pc_heal()   ) profile_str += "# pvp_4pc_heal=1" + term;

    for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ SLOT_OUT_ORDER[ i ] ];
      if ( ! item.active() ) continue;
      if ( item.unique || item.parsed.enchant.unique || item.parsed.addon.unique || ! item.encoded_weapon().empty() )
      {
        profile_str += "# ";
        profile_str += item.slot_name();
        profile_str += "=";
        profile_str += item.name();
        if ( item.parsed.data.heroic()                ) profile_str += ",heroic=1";
        if ( item.parsed.data.lfr()                   ) profile_str += ",lfr=1";
        if ( item.parsed.data.flex()                   ) profile_str += ",flex=1";
        if ( item.parsed.data.elite()         ) profile_str += ",elite=1";
        if ( ! item.encoded_weapon().empty()        ) profile_str += ",weapon=" + item.encoded_weapon();
        if ( ! item.parsed.enchant.name_str.empty() ) profile_str += ",enchant=" + item.parsed.enchant.name_str;
        if ( ! item.parsed.addon.name_str.empty()   ) profile_str += ",addon="   + item.parsed.addon.name_str;
        profile_str += term;
      }
    }

    if ( enchant.attribute[ ATTR_STRENGTH  ] != 0 )  profile_str += "enchant_strength="         + util::to_string( enchant.attribute[ ATTR_STRENGTH  ] ) + term;
    if ( enchant.attribute[ ATTR_AGILITY   ] != 0 )  profile_str += "enchant_agility="          + util::to_string( enchant.attribute[ ATTR_AGILITY   ] ) + term;
    if ( enchant.attribute[ ATTR_STAMINA   ] != 0 )  profile_str += "enchant_stamina="          + util::to_string( enchant.attribute[ ATTR_STAMINA   ] ) + term;
    if ( enchant.attribute[ ATTR_INTELLECT ] != 0 )  profile_str += "enchant_intellect="        + util::to_string( enchant.attribute[ ATTR_INTELLECT ] ) + term;
    if ( enchant.attribute[ ATTR_SPIRIT    ] != 0 )  profile_str += "enchant_spirit="           + util::to_string( enchant.attribute[ ATTR_SPIRIT    ] ) + term;
    if ( enchant.spell_power                 != 0 )  profile_str += "enchant_spell_power="      + util::to_string( enchant.spell_power ) + term;
    if ( enchant.attack_power                != 0 )  profile_str += "enchant_attack_power="     + util::to_string( enchant.attack_power ) + term;
    if ( enchant.expertise_rating            != 0 )  profile_str += "enchant_expertise_rating=" + util::to_string( enchant.expertise_rating ) + term;
    if ( enchant.armor                       != 0 )  profile_str += "enchant_armor="            + util::to_string( enchant.armor ) + term;
    if ( enchant.haste_rating                != 0 )  profile_str += "enchant_haste_rating="     + util::to_string( enchant.haste_rating ) + term;
    if ( enchant.hit_rating                  != 0 )  profile_str += "enchant_hit_rating="       + util::to_string( enchant.hit_rating ) + term;
    if ( enchant.crit_rating                 != 0 )  profile_str += "enchant_crit_rating="      + util::to_string( enchant.crit_rating ) + term;
    if ( enchant.mastery_rating              != 0 )  profile_str += "enchant_mastery_rating="   + util::to_string( enchant.mastery_rating ) + term;
    if ( enchant.resource[ RESOURCE_HEALTH ] != 0 )  profile_str += "enchant_health="           + util::to_string( enchant.resource[ RESOURCE_HEALTH ] ) + term;
    if ( enchant.resource[ RESOURCE_MANA   ] != 0 )  profile_str += "enchant_mana="             + util::to_string( enchant.resource[ RESOURCE_MANA   ] ) + term;
    if ( enchant.resource[ RESOURCE_RAGE   ] != 0 )  profile_str += "enchant_rage="             + util::to_string( enchant.resource[ RESOURCE_RAGE   ] ) + term;
    if ( enchant.resource[ RESOURCE_ENERGY ] != 0 )  profile_str += "enchant_energy="           + util::to_string( enchant.resource[ RESOURCE_ENERGY ] ) + term;
    if ( enchant.resource[ RESOURCE_FOCUS  ] != 0 )  profile_str += "enchant_focus="            + util::to_string( enchant.resource[ RESOURCE_FOCUS  ] ) + term;
    if ( enchant.resource[ RESOURCE_RUNIC_POWER  ] != 0 )  profile_str += "enchant_runic="            + util::to_string( enchant.resource[ RESOURCE_RUNIC_POWER  ] ) + term;
  }

  return true;
}


// player_t::copy_from ======================================================

void player_t::copy_from( player_t* source )
{
  origin_str = source -> origin_str;
  level = source -> level;
  race_str = source -> race_str;
  race = source -> race;
  role = source -> role;
  _spec = source -> _spec;
  position_str = source -> position_str;
  professions_str = source -> professions_str;
  source -> recreate_talent_str( TALENT_FORMAT_UNCHANGED );
  parse_talent_url( sim, "talents", source -> talents_str );
  talent_overrides_str = source -> talent_overrides_str;
  glyphs_str = source -> glyphs_str;
  action_list_str = source -> action_list_str;
  alist_map = source -> alist_map;

  meta_gem = source -> meta_gem;
  for ( size_t i = 0; i < items.size(); i++ )
  {
    items[ i ] = source -> items[ i ];
    items[ i ].player = this;
  }

  set_bonus.count = source -> set_bonus.count;
  gear = source -> gear;
  enchant = source -> enchant;
}

// class_modules::create::options ===========================================

void player_t::create_options()
{
  option_t player_options[] =
  {
    // General
    opt_string( "name", name_str ),
    opt_func( "origin", parse_origin ),
    opt_string( "region", region_str ),
    opt_string( "server", server_str ),
    opt_string( "thumbnail", report_information.thumbnail_url ),
    opt_string( "id", id_str ),
    opt_func( "talents", parse_talent_url ),
    opt_func( "talent_override", parse_talent_override ),
    opt_string( "glyphs", glyphs_str ),
    opt_string( "race", race_str ),
    opt_int( "level", level ),
    opt_bool( "ready_trigger", ready_type ),
    opt_func( "role", parse_role_string ),
    opt_string( "target", target_str ),
    opt_float( "skill", base.skill ),
    opt_float( "distance", base.distance ),
    opt_string( "position", position_str ),
    opt_string( "professions", professions_str ),
    opt_string( "actions", action_list_str ),
    opt_append( "actions+", action_list_str ),
    opt_map( "actions.", alist_map ),
    opt_string( "action_list", choose_action_list ),
    opt_bool( "sleeping", initial.sleeping ),
    opt_bool( "quiet", quiet ),
    opt_string( "save", report_information.save_str ),
    opt_string( "save_gear", report_information.save_gear_str ),
    opt_string( "save_talents", report_information.save_talents_str ),
    opt_string( "save_actions", report_information.save_actions_str ),
    opt_string( "comment", report_information.comment_str ),
    opt_bool( "bugs", bugs ),
    opt_func( "world_lag", parse_world_lag ),
    opt_func( "world_lag_stddev", parse_world_lag_stddev ),
    opt_func( "brain_lag", parse_brain_lag ),
    opt_func( "brain_lag_stddev", parse_brain_lag_stddev ),
    opt_bool( "scale_player", scale_player ),
    opt_bool( "tmi_self_only", tmi_self_only ),
    opt_string( "tmi_output", tmi_debug_file_str ),
    opt_func( "spec", parse_specialization ),
    opt_func( "specialization", parse_specialization ),
    opt_func( "stat_timelines", parse_stat_timelines ),

    // Items
    opt_string( "meta_gem",  meta_gem_str ),
    opt_string( "items",     items_str ),
    opt_append( "items+",    items_str ),
    opt_string( "head",      items[ SLOT_HEAD      ].options_str ),
    opt_string( "neck",      items[ SLOT_NECK      ].options_str ),
    opt_string( "shoulders", items[ SLOT_SHOULDERS ].options_str ),
    opt_string( "shoulder",  items[ SLOT_SHOULDERS ].options_str ),
    opt_string( "shirt",     items[ SLOT_SHIRT     ].options_str ),
    opt_string( "chest",     items[ SLOT_CHEST     ].options_str ),
    opt_string( "waist",     items[ SLOT_WAIST     ].options_str ),
    opt_string( "legs",      items[ SLOT_LEGS      ].options_str ),
    opt_string( "leg",       items[ SLOT_LEGS      ].options_str ),
    opt_string( "feet",      items[ SLOT_FEET      ].options_str ),
    opt_string( "foot",      items[ SLOT_FEET      ].options_str ),
    opt_string( "wrists",    items[ SLOT_WRISTS    ].options_str ),
    opt_string( "wrist",     items[ SLOT_WRISTS    ].options_str ),
    opt_string( "hands",     items[ SLOT_HANDS     ].options_str ),
    opt_string( "hand",      items[ SLOT_HANDS     ].options_str ),
    opt_string( "finger1",   items[ SLOT_FINGER_1  ].options_str ),
    opt_string( "finger2",   items[ SLOT_FINGER_2  ].options_str ),
    opt_string( "ring1",     items[ SLOT_FINGER_1  ].options_str ),
    opt_string( "ring2",     items[ SLOT_FINGER_2  ].options_str ),
    opt_string( "trinket1",  items[ SLOT_TRINKET_1 ].options_str ),
    opt_string( "trinket2",  items[ SLOT_TRINKET_2 ].options_str ),
    opt_string( "back",      items[ SLOT_BACK      ].options_str ),
    opt_string( "main_hand", items[ SLOT_MAIN_HAND ].options_str ),
    opt_string( "off_hand",  items[ SLOT_OFF_HAND  ].options_str ),
    opt_string( "tabard",    items[ SLOT_TABARD    ].options_str ),

    // Set Bonus
    opt_bool( "tier13_2pc_caster", set_bonus.count[ SET_T13_2PC_CASTER ] ),
    opt_bool( "tier13_4pc_caster", set_bonus.count[ SET_T13_4PC_CASTER ] ),
    opt_bool( "tier13_2pc_melee",  set_bonus.count[ SET_T13_2PC_MELEE ] ),
    opt_bool( "tier13_4pc_melee",  set_bonus.count[ SET_T13_4PC_MELEE ] ),
    opt_bool( "tier13_2pc_tank",   set_bonus.count[ SET_T13_2PC_TANK ] ),
    opt_bool( "tier13_4pc_tank",   set_bonus.count[ SET_T13_4PC_TANK ] ),
    opt_bool( "tier13_2pc_heal",   set_bonus.count[ SET_T13_2PC_HEAL ] ),
    opt_bool( "tier13_4pc_heal",   set_bonus.count[ SET_T13_4PC_HEAL ] ),
    opt_bool( "tier14_2pc_caster", set_bonus.count[ SET_T14_2PC_CASTER ] ),
    opt_bool( "tier14_4pc_caster", set_bonus.count[ SET_T14_4PC_CASTER ] ),
    opt_bool( "tier14_2pc_melee",  set_bonus.count[ SET_T14_2PC_MELEE ] ),
    opt_bool( "tier14_4pc_melee",  set_bonus.count[ SET_T14_4PC_MELEE ] ),
    opt_bool( "tier14_2pc_tank",   set_bonus.count[ SET_T14_2PC_TANK ] ),
    opt_bool( "tier14_4pc_tank",   set_bonus.count[ SET_T14_4PC_TANK ] ),
    opt_bool( "tier14_2pc_heal",   set_bonus.count[ SET_T14_2PC_HEAL ] ),
    opt_bool( "tier14_4pc_heal",   set_bonus.count[ SET_T14_4PC_HEAL ] ),
    opt_bool( "tier15_2pc_caster", set_bonus.count[ SET_T15_2PC_CASTER ] ),
    opt_bool( "tier15_4pc_caster", set_bonus.count[ SET_T15_4PC_CASTER ] ),
    opt_bool( "tier15_2pc_melee",  set_bonus.count[ SET_T15_2PC_MELEE ] ),
    opt_bool( "tier15_4pc_melee",  set_bonus.count[ SET_T15_4PC_MELEE ] ),
    opt_bool( "tier15_2pc_tank",   set_bonus.count[ SET_T15_2PC_TANK ] ),
    opt_bool( "tier15_4pc_tank",   set_bonus.count[ SET_T15_4PC_TANK ] ),
    opt_bool( "tier15_2pc_heal",   set_bonus.count[ SET_T15_2PC_HEAL ] ),
    opt_bool( "tier15_4pc_heal",   set_bonus.count[ SET_T15_4PC_HEAL ] ),
    opt_bool( "tier16_2pc_caster", set_bonus.count[ SET_T16_2PC_CASTER ] ),
    opt_bool( "tier16_4pc_caster", set_bonus.count[ SET_T16_4PC_CASTER ] ),
    opt_bool( "tier16_2pc_melee",  set_bonus.count[ SET_T16_2PC_MELEE ] ),
    opt_bool( "tier16_4pc_melee",  set_bonus.count[ SET_T16_4PC_MELEE ] ),
    opt_bool( "tier16_2pc_tank",   set_bonus.count[ SET_T16_2PC_TANK ] ),
    opt_bool( "tier16_4pc_tank",   set_bonus.count[ SET_T16_4PC_TANK ] ),
    opt_bool( "tier16_2pc_heal",   set_bonus.count[ SET_T16_2PC_HEAL ] ),
    opt_bool( "tier16_4pc_heal",   set_bonus.count[ SET_T16_4PC_HEAL ] ),
    opt_bool( "pvp_2pc_caster",    set_bonus.count[ SET_PVP_2PC_CASTER ] ),
    opt_bool( "pvp_4pc_caster",    set_bonus.count[ SET_PVP_4PC_CASTER ] ),
    opt_bool( "pvp_2pc_melee",     set_bonus.count[ SET_PVP_2PC_MELEE ] ),
    opt_bool( "pvp_4pc_melee",     set_bonus.count[ SET_PVP_4PC_MELEE ] ),
    opt_bool( "pvp_2pc_tank",      set_bonus.count[ SET_PVP_2PC_TANK ] ),
    opt_bool( "pvp_4pc_tank",      set_bonus.count[ SET_PVP_4PC_TANK ] ),
    opt_bool( "pvp_2pc_heal",      set_bonus.count[ SET_PVP_2PC_HEAL ] ),
    opt_bool( "pvp_4pc_heal",      set_bonus.count[ SET_PVP_4PC_HEAL ] ),

    // Gear Stats
    opt_float( "gear_strength",         gear.attribute[ ATTR_STRENGTH  ] ),
    opt_float( "gear_agility",          gear.attribute[ ATTR_AGILITY   ] ),
    opt_float( "gear_stamina",          gear.attribute[ ATTR_STAMINA   ] ),
    opt_float( "gear_intellect",        gear.attribute[ ATTR_INTELLECT ] ),
    opt_float( "gear_spirit",           gear.attribute[ ATTR_SPIRIT    ] ),
    opt_float( "gear_spell_power",      gear.spell_power ),
    opt_float( "gear_attack_power",     gear.attack_power ),
    opt_float( "gear_expertise_rating", gear.expertise_rating ),
    opt_float( "gear_haste_rating",     gear.haste_rating ),
    opt_float( "gear_hit_rating",       gear.hit_rating ),
    opt_float( "gear_crit_rating",      gear.crit_rating ),
    opt_float( "gear_parry_rating",     gear.parry_rating ),
    opt_float( "gear_dodge_rating",     gear.dodge_rating ),
    opt_float( "gear_health",           gear.resource[ RESOURCE_HEALTH ] ),
    opt_float( "gear_mana",             gear.resource[ RESOURCE_MANA   ] ),
    opt_float( "gear_rage",             gear.resource[ RESOURCE_RAGE   ] ),
    opt_float( "gear_energy",           gear.resource[ RESOURCE_ENERGY ] ),
    opt_float( "gear_focus",            gear.resource[ RESOURCE_FOCUS  ] ),
    opt_float( "gear_runic",            gear.resource[ RESOURCE_RUNIC_POWER  ] ),
    opt_float( "gear_armor",            gear.armor ),
    opt_float( "gear_mastery_rating",   gear.mastery_rating ),

    // Stat Enchants
    opt_float( "enchant_strength",         enchant.attribute[ ATTR_STRENGTH  ] ),
    opt_float( "enchant_agility",          enchant.attribute[ ATTR_AGILITY   ] ),
    opt_float( "enchant_stamina",          enchant.attribute[ ATTR_STAMINA   ] ),
    opt_float( "enchant_intellect",        enchant.attribute[ ATTR_INTELLECT ] ),
    opt_float( "enchant_spirit",           enchant.attribute[ ATTR_SPIRIT    ] ),
    opt_float( "enchant_spell_power",      enchant.spell_power ),
    opt_float( "enchant_attack_power",     enchant.attack_power ),
    opt_float( "enchant_expertise_rating", enchant.expertise_rating ),
    opt_float( "enchant_armor",            enchant.armor ),
    opt_float( "enchant_haste_rating",     enchant.haste_rating ),
    opt_float( "enchant_hit_rating",       enchant.hit_rating ),
    opt_float( "enchant_crit_rating",      enchant.crit_rating ),
    opt_float( "enchant_mastery_rating",   enchant.mastery_rating ),
    opt_float( "enchant_health",           enchant.resource[ RESOURCE_HEALTH ] ),
    opt_float( "enchant_mana",             enchant.resource[ RESOURCE_MANA   ] ),
    opt_float( "enchant_rage",             enchant.resource[ RESOURCE_RAGE   ] ),
    opt_float( "enchant_energy",           enchant.resource[ RESOURCE_ENERGY ] ),
    opt_float( "enchant_focus",            enchant.resource[ RESOURCE_FOCUS  ] ),
    opt_float( "enchant_runic",            enchant.resource[ RESOURCE_RUNIC_POWER  ] ),

    // Regen
    opt_bool( "infinite_energy", resources.infinite_resource[ RESOURCE_ENERGY ] ),
    opt_bool( "infinite_focus",  resources.infinite_resource[ RESOURCE_FOCUS  ] ),
    opt_bool( "infinite_health", resources.infinite_resource[ RESOURCE_HEALTH ] ),
    opt_bool( "infinite_mana",   resources.infinite_resource[ RESOURCE_MANA   ] ),
    opt_bool( "infinite_rage",   resources.infinite_resource[ RESOURCE_RAGE   ] ),
    opt_bool( "infinite_runic",  resources.infinite_resource[ RESOURCE_RUNIC_POWER  ] ),

    // Misc
    opt_string( "skip_actions", action_list_skip ),
    opt_string( "modify_action", modify_action ),
    opt_string( "elixirs", elixirs_str ),
    opt_string( "flask", flask_str ),
    opt_string( "food", food_str ),
    opt_timespan( "reaction_time_mean", reaction_mean ),
    opt_timespan( "reaction_time_stddev", reaction_stddev ),
    opt_timespan( "reaction_time_nu", reaction_nu ),
    opt_timespan( "reaction_time_offset", reaction_offset ),
    opt_bool( "stat_cache", cache.active ),
    opt_null()
  };

  option_t::copy( options, player_options );
}

// player_t::create =========================================================

player_t* player_t::create( sim_t*,
                            const player_description_t& )
{
  return 0;
}

namespace { // UNNAMED NAMESPACE

struct compare_stats_name
{
  bool operator()( stats_t* l, stats_t* r ) const
  {
    return l -> name_str <= r -> name_str;
  }
};

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

  for ( size_t i = 0; i <  buff_list.size(); ++i )
    buff_list[ i ] -> analyze();

  range::sort(  stats_list, compare_stats_name() );

  if (  quiet ) return;
  if (  collected_data.fight_length.mean() == 0 ) return;

  // Pet Chart Adjustment ===================================================
  size_t max_buckets = static_cast<size_t>(  collected_data.fight_length.max() );

  // Make the pet graphs the same length as owner's
  if (  is_pet() )
  {
    player_t* o =  cast_pet() -> owner;
    max_buckets = static_cast<size_t>( o -> collected_data.fight_length.max() );
  }

  // Stats Analysis =========================================================
  std::vector<stats_t*> tmp_stats_list;

  // Append  stats_list to stats_list
  tmp_stats_list.insert( tmp_stats_list.end(),  stats_list.begin(),  stats_list.end() );

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
      stats_t* s = tmp_stats_list[ i ];
      s -> analyze();

      if ( s -> type == STATS_DMG )
        s -> portion_amount =  collected_data.compound_dmg.mean() ? s -> actual_amount.mean() /  collected_data.compound_dmg.mean() : 0 ;
      else if ( s -> type == STATS_HEAL || s -> type == STATS_ABSORB )
        s -> portion_amount =  collected_data.compound_heal.mean() ? s -> actual_amount.mean() /  collected_data.compound_heal.mean() : 0;
    }
  }

  // Actor Lists ============================================================
  if (  !  quiet && !  is_enemy() && !  is_add() && ! (  is_pet() && s.report_pets_separately ) )
  {
    s.players_by_dps.push_back( this );
    s.players_by_hps.push_back( this );
    s.players_by_name.push_back( this );
  }
  if ( !  quiet && (  is_enemy() ||  is_add() ) && ! (  is_pet() && s.report_pets_separately ) )
    s.targets_by_name.push_back( this );

  // Vengeance Timeline
  vengeance.adjust( s.divisor_timeline );

  // Resources & Gains ======================================================

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    resource_lost  [ i ] /= s.iterations;
    resource_gained[ i ] /= s.iterations;
  }

  double rl = resource_lost[  primary_resource() ];
  dpr = ( rl > 0 ) ? (  collected_data.dmg.mean() / rl ) : -1.0;
  hpr = ( rl > 0 ) ? (  collected_data.heal.mean() / rl ) : -1.0;

  rps_loss = resource_lost  [  primary_resource() ] /  collected_data.fight_length.mean();
  rps_gain = resource_gained[  primary_resource() ] /  collected_data.fight_length.mean();

  for ( size_t i = 0; i < gain_list.size(); ++i )
    gain_list[ i ] -> analyze( s );

  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_t* pet =  pet_list[ i ];
    for ( size_t i = 0; i < pet -> gain_list.size(); ++i )
      pet -> gain_list[ i ] -> analyze( s );
  }

  // Damage Timelines =======================================================

  collected_data.timeline_dmg.init( max_buckets );
  for ( size_t i = 0, is_hps = ( primary_role() == ROLE_HEAL ); i < num_stats; i++ )
  {
    stats_t* s = tmp_stats_list[ i ];
    if ( ( s -> type != STATS_DMG ) == is_hps )
    {
      size_t j_max = std::min( max_buckets, s -> timeline_amount.data().size() );
      for ( size_t j = 0; j < j_max; j++ )
        collected_data.timeline_dmg.add( j, s -> timeline_amount.data()[ j ] );
    }
  }


  recreate_talent_str( s.talent_format );

  // Error Convergence ======================================================
  player_convergence( s.convergence_scale, s.confidence_estimator,
                      collected_data.dps,  dps_convergence_error,  sim_t::distribution_mean_error( s, collected_data.dps ),  dps_convergence );

}

// Return sample_data reference over which this player gets scaled ( scale factors, reforge plots, etc. )
// By default this will be his personal dps or hps

player_t::scales_over_t player_t::scales_over()
{
  const std::string& so = sim -> scaling -> scale_over;

  player_t* q = nullptr;
  if ( ! sim -> scaling -> scale_over_player.empty() )
    q = sim -> find_player( sim -> scaling -> scale_over_player );
  if ( !q )
    q = this;

  if ( so == "dmg_taken" ) 
    return q -> collected_data.dmg_taken;

  if ( so == "dps" )
    return q -> collected_data.dps;

  if ( so == "dpse" )
    return q -> collected_data.dpse;

  if ( so == "hpse" )
    return q -> collected_data.hpse;

  if ( so == "htps" )
    return q -> collected_data.htps;

  if ( so == "deaths" )
    return q -> collected_data.deaths;

  if ( so == "theck_meloree_index" || so == "tmi" )
    return q -> collected_data.theck_meloree_index;

  if ( q -> primary_role() == ROLE_HEAL || so == "hps" )
    return q -> collected_data.hps;

  if ( q -> primary_role() == ROLE_TANK || so == "dtps" )
    return q -> collected_data.dtps;

  return q -> collected_data.dps;
}

// Change the player position ( fron/back, etc. ) and update attack hit table

void player_t::change_position( position_e new_pos )
{
  if ( sim -> debug )
    sim -> out_debug.printf( "%s changes position from %s to %s.", name(), util::position_type_string( position() ), util::position_type_string( new_pos ) );

  current.position = new_pos;
}

void player_t::talent_points_t::clear()
{  range::fill( choices, -1 ); }

std::string player_t::talent_points_t::to_string() const
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

// Player Callbacks


// player_t::register_resource_gain_callback ================================

void player_callbacks_t::register_resource_gain_callback( resource_e resource_type,
                                                          action_callback_t* cb )
{
  resource_gain[ resource_type ].push_back( cb );
}

// player_t::register_resource_loss_callback ================================

void player_callbacks_t::register_resource_loss_callback( resource_e resource_type,
                                                          action_callback_t* cb )
{
  resource_loss[ resource_type ].push_back( cb );
}

// player_t::register_attack_callback =======================================

void player_callbacks_t::register_attack_callback( int64_t mask,
                                                   action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      attack[ i ].push_back( cb );
    }
  }
}

// player_t::register_spell_callback ========================================

void player_callbacks_t::register_spell_callback( int64_t mask,
                                                  action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell[ i ].push_back( cb );
      heal[ i ].push_back( cb );
    }
  }
}

// player_t::register_tick_callback =========================================

void player_callbacks_t::register_tick_callback( int64_t mask,
                                                 action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      tick[ i ].push_back( cb );
    }
  }
}

// player_t::register_heal_callback =========================================

void player_callbacks_t::register_heal_callback( int64_t mask,
                                                 action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      heal[ i ].push_back( cb );
    }
  }
}

// player_t::register_absorb_callback =======================================

void player_callbacks_t::register_absorb_callback( int64_t mask,
                                                   action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      absorb[ i ].push_back( cb );
    }
  }
}

// player_t::register_harmful_spell_callback ================================

void player_callbacks_t::register_harmful_spell_callback( int64_t mask,
                                                          action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      harmful_spell[ i ].push_back( cb );
    }
  }
}


// player_t::register_direct_harmful_spell_callback =========================

void player_callbacks_t::register_direct_harmful_spell_callback( int64_t mask,
                                                                 action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      direct_harmful_spell[ i ].push_back( cb );
    }
  }
}

// player_t::register_tick_damage_callback ==================================

void player_callbacks_t::register_tick_damage_callback( int64_t mask,
                                                        action_callback_t* cb )
{
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      tick_damage[ i ].push_back( cb );
    }
  }
}

// player_t::register_direct_damage_callback ================================

void player_callbacks_t::register_direct_damage_callback( int64_t mask,
                                                          action_callback_t* cb )
{
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      direct_damage[ i ].push_back( cb );
    }
  }
}

// player_t::register_direct_crit_callback ==================================

void player_callbacks_t::register_direct_crit_callback( int64_t mask,
                                                        action_callback_t* cb )
{
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      direct_crit[ i ].push_back( cb );
    }
  }
}

// player_t::register_spell_tick_damage_callback ============================

void player_callbacks_t::register_spell_tick_damage_callback( int64_t mask,
                                                              action_callback_t* cb )
{
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell_tick_damage[ i ].push_back( cb );
    }
  }
}

// player_t::register_spell_direct_damage_callback ==========================

void player_callbacks_t::register_spell_direct_damage_callback( int64_t mask,
                                                                action_callback_t* cb )
{
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell_direct_damage[ i ].push_back( cb );
    }
  }
}


// player_t::register_tick_heal_callback ====================================

void player_callbacks_t::register_tick_heal_callback( int64_t mask,
                                                      action_callback_t* cb )
{
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      tick_heal[ i ].push_back( cb );
    }
  }
}

// player_t::register_direct_heal_callback ==================================

void player_callbacks_t::register_direct_heal_callback( int64_t mask,
                                                        action_callback_t* cb )
{
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      direct_heal[ i ].push_back( cb );
    }
  }
}

// player_callbacks_t::register_incoming_attack_callback ====================

void player_callbacks_t::register_incoming_attack_callback( int64_t mask,
                                                            action_callback_t* cb )
{
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      incoming_attack[ i ].push_back( cb );
    }
  }
}

void player_callbacks_t::reset()
{
  action_callback_t::reset( all_callbacks );
}


/* Invalidate ALL stats
 */
void player_stat_cache_t::invalidate()
{
  if ( ! active ) return;

  range::fill( valid, false );
  range::fill( spell_power_valid, false );
  range::fill( player_mult_valid, false );
  range::fill( player_heal_mult_valid, false );
}

/* Helper function to access attribute cache functions by attribute-enumeration
 */
double player_stat_cache_t::get_attribute( attribute_e a )
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

#ifdef SC_STAT_CACHE

// player_stat_cache_t::strength ============================================

double player_stat_cache_t::strength()
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

double player_stat_cache_t::agility()
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

double player_stat_cache_t::stamina()
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

double player_stat_cache_t::intellect()
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

double player_stat_cache_t::spirit()
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

double player_stat_cache_t::spell_power( school_e s )
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

double player_stat_cache_t::attack_power()
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

double player_stat_cache_t::attack_expertise()
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

double player_stat_cache_t::attack_hit()
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

// player_stat_cache_t::attack_crit =========================================

double player_stat_cache_t::attack_crit()
{
  if ( ! active || ! valid[ CACHE_ATTACK_CRIT ] )
  {
    valid[ CACHE_ATTACK_CRIT ] = true;
    _attack_crit = player -> composite_melee_crit();
  }
  else assert( _attack_crit == player -> composite_melee_crit() );
  return _attack_crit;
}

// player_stat_cache_t::attack_haste ========================================

double player_stat_cache_t::attack_haste()
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

double player_stat_cache_t::attack_speed()
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

double player_stat_cache_t::spell_hit()
{
  if ( ! active || ! valid[ CACHE_SPELL_HIT ] )
  {
    valid[ CACHE_SPELL_HIT ] = true;
    _spell_hit = player -> composite_spell_hit();
  }
  else assert( _spell_hit == player -> composite_spell_hit() );
  return _spell_hit;
}

// player_stat_cache_t::spell_crit ==========================================

double player_stat_cache_t::spell_crit()
{
  if ( ! active || ! valid[ CACHE_SPELL_CRIT ] )
  {
    valid[ CACHE_SPELL_CRIT ] = true;
    _spell_crit = player -> composite_spell_crit();
  }
  else assert( _spell_crit == player -> composite_spell_crit() );
  return _spell_crit;
}

// player_stat_cache_t::spell_haste =========================================

double player_stat_cache_t::spell_haste()
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

double player_stat_cache_t::spell_speed()
{
  if ( ! active || ! valid[ CACHE_SPELL_SPEED ] )
  {
    valid[ CACHE_SPELL_SPEED ] = true;
    _spell_speed = player -> composite_spell_speed();
  }
  else assert( _spell_speed == player -> composite_spell_speed() );
  return _spell_speed;
}

double player_stat_cache_t::dodge()
{
  if ( ! active || ! valid[ CACHE_DODGE ] )
  {
    valid[ CACHE_DODGE ] = true;
    _dodge = player -> composite_dodge();
  }
  else assert( _dodge == player -> composite_dodge() );
  return _dodge;
}

double player_stat_cache_t::parry()
{
  if ( ! active || ! valid[ CACHE_PARRY ] )
  {
    valid[ CACHE_PARRY ] = true;
    _parry = player -> composite_parry();
  }
  else assert( _parry == player -> composite_parry() );
  return _parry;
}

double player_stat_cache_t::block()
{
  if ( ! active || ! valid[ CACHE_BLOCK ] )
  {
    valid[ CACHE_BLOCK ] = true;
    _block = player -> composite_block();
  }
  else assert( _block == player -> composite_block() );
  return _block;
}

double player_stat_cache_t::crit_block()
{
  if ( ! active || ! valid[ CACHE_CRIT_BLOCK ] )
  {
    valid[ CACHE_CRIT_BLOCK ] = true;
    _crit_block = player -> composite_crit_block();
  }
  else assert( _crit_block == player -> composite_crit_block() );
  return _crit_block;
}

double player_stat_cache_t::crit_avoidance()
{
  if ( ! active || ! valid[ CACHE_CRIT_AVOIDANCE ] )
  {
    valid[ CACHE_CRIT_AVOIDANCE ] = true;
    _crit_avoidance = player -> composite_crit_avoidance();
  }
  else assert( _crit_avoidance == player -> composite_crit_avoidance() );
  return _crit_avoidance;
}

double player_stat_cache_t::miss()
{
  if ( ! active || ! valid[ CACHE_MISS ] )
  {
    valid[ CACHE_MISS ] = true;
    _miss = player -> composite_miss();
  }
  else assert( _miss == player -> composite_miss() );
  return _miss;
}

double player_stat_cache_t::armor()
{
  if ( ! active || ! valid[ CACHE_ARMOR ] )
  {
    valid[ CACHE_ARMOR ] = true;
    _armor = player -> composite_armor();
  }
  else assert( _armor == player -> composite_armor() );
  return _armor;
}

/* This is composite_mastery * specialization_mastery_coefficient !
 *
 * If you need the pure mastery value, use player_t::composite_mastery
 */
double player_stat_cache_t::mastery_value()
{
  if ( ! active || ! valid[ CACHE_MASTERY ] )
  {
    valid[ CACHE_MASTERY ] = true;
    _mastery_value = player -> composite_mastery_value();
  }
  else assert( _mastery_value == player -> composite_mastery_value() );
  return _mastery_value;
}

// player_stat_cache_t::mastery =============================================

double player_stat_cache_t::player_multiplier( school_e s )
{
  if ( ! active || ! player_mult_valid[ s ] )
  {
    player_mult_valid[ s ] = true;
    _player_mult[ s ] = player -> composite_player_multiplier( s );
  }
  else assert( _player_mult[ s ] == player -> composite_player_multiplier( s ) );
  return _player_mult[ s ];
}

// player_stat_cache_t::mastery =============================================

double player_stat_cache_t::player_heal_multiplier( school_e s )
{
  if ( ! active || ! player_heal_mult_valid[ s ] )
  {
    player_heal_mult_valid[ s ] = true;
    _player_heal_mult[ s ] = player -> composite_player_heal_multiplier( s );
  }
  else assert( _player_heal_mult[ s ] == player -> composite_player_heal_multiplier( s ) );
  return _player_heal_mult[ s ];
}

#endif

/* Start Vengeance
 *
 * Call in combat_begin() when it is active during the whole fight,
 * otherwise in a action/buff ( like Druid Bear Form )
 */

void player_vengeance_t::start( player_t& p )
{
  assert( ! is_started() );

  struct collect_event_t : public event_t
  {
    player_vengeance_t& vengeance;
    collect_event_t( player_t& p, player_vengeance_t& v ) :
      event_t( p, "vengeance_timeline_collect_event_t" ),
      vengeance( v )
    {
      sim().add_event( this, timespan_t::from_seconds( 1 ) );
    }

    virtual void execute()
    {
      assert( vengeance.event == this );
      vengeance.timeline_.add( sim().current_time, p() -> buffs.vengeance -> value() );
      vengeance.event = new ( sim() ) collect_event_t( *p(), vengeance );
    }
  };

  event = new ( *p.sim ) collect_event_t( p, *this ); // start timeline
}

/* Stop Vengeance
 *
 * Is automatically called in player_t::demise()
 * If you have dynamic vengeance activation ( like Druid Bear Form ), call it in the buff expiration/etc.
 */

void player_vengeance_t::stop()
{ core_event_t::cancel( event ); }

player_collected_data_t::action_sequence_data_t::action_sequence_data_t( const action_t* a, const player_t* t, const timespan_t& ts, const player_t* p ) :
  action( a ), target( t ), time( ts )
{
  for ( size_t i = 0; i < p -> buff_list.size(); ++i )
    if ( p -> buff_list[ i ] -> check() && ! p -> buff_list[ i ] -> quiet )
      buff_list.push_back( p -> buff_list[ i ] );

  range::fill( resource_snapshot, -1 );

  for ( resource_e i = RESOURCE_HEALTH; i < RESOURCE_MAX; ++i )
    if ( p -> resources.max[ i ] > 0.0 )
      resource_snapshot[ i ] = p -> resources.current[ i ];
}

player_collected_data_t::player_collected_data_t( const std::string& player_name, sim_t& s ) :
  fight_length( player_name + " Fight Length", s.statistics_level < 2 ),
  waiting_time(), executed_foreground_actions(),
  dmg( player_name + " Damage", s.statistics_level < 2 ),
  compound_dmg( player_name + " Total Damage", s.statistics_level < 2 ),
  dps( player_name + " Damage Per Second", s.statistics_level < 1 ),
  dpse( player_name + " Damage per Second (effective)", s.statistics_level < 2 ),
  dtps( player_name + " Damage Taken Per Second", s.statistics_level < 2 ),
  dmg_taken( player_name + " Damage Taken", s.statistics_level < 2 ),
  timeline_dmg(),
  heal( player_name + " Heal", s.statistics_level < 2 ),
  compound_heal( player_name + " Total Heal", s.statistics_level < 2 ),
  hps( player_name + " Healing Per Second", s.statistics_level < 1 ),
  hpse( player_name + " Healing per Second (effective)", s.statistics_level < 2 ),
  htps( player_name + " Healing taken Per Second", s.statistics_level < 2 ),
  heal_taken( player_name + " Healing Taken", s.statistics_level < 2 ),
  deaths( player_name + " Deaths", s.statistics_level < 2 ),
  theck_meloree_index( player_name + " Theck-Meloree Index", s.statistics_level < 1 ),
  resource_timelines(),
  combat_end_resource( RESOURCE_MAX ),
  stat_timelines(),
  health_changes()
{ }

void player_collected_data_t::reserve_memory( const player_t& p )
{
  fight_length.reserve( p.sim -> iterations );
  // DMG
  dmg.reserve( p.sim -> iterations );
  compound_dmg.reserve( p.sim -> iterations );
  dps.reserve( p.sim -> iterations );
  dpse.reserve( p.sim -> iterations );
  dtps.reserve( p.sim -> iterations );
  // HEAL
  heal.reserve( p.sim -> iterations );
  compound_heal.reserve( p.sim -> iterations );
  hps.reserve( p.sim -> iterations );
  hpse.reserve( p.sim -> iterations );
  htps.reserve( p.sim -> iterations );
  heal_taken.reserve( p.sim -> iterations );
  deaths.reserve( p.sim -> iterations );

  if ( ! p.is_pet() && p.primary_role() == ROLE_TANK )
  {
    theck_meloree_index.reserve( p.sim -> iterations );
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

  assert( resource_timelines.size() == other.resource_timelines.size() );
  for ( size_t i = 0; i < resource_timelines.size(); ++i )
  {
    assert( resource_timelines[ i ].type == other.resource_timelines[ i ].type );
    assert( resource_timelines[ i ].type != RESOURCE_NONE );

    resource_timelines[ i ].timeline.merge ( other.resource_timelines[ i ].timeline );
  }

  health_changes.merged_timeline.merge( other.health_changes.merged_timeline );
}

void player_collected_data_t::analyze( const player_t& p )
{
  fight_length.analyze_all();
  // DMG
  dmg.analyze_all();
  compound_dmg.analyze_all();
  dps.analyze_all();
  dpse.analyze_all();
  dmg_taken.analyze_all();
  dtps.analyze_all();
  timeline_dmg_taken.adjust( p.sim -> divisor_timeline );
  // Heal
  heal.analyze_all();
  compound_heal.analyze_all();
  hps.analyze_all();
  hpse.analyze_all();
  heal_taken.analyze_all();
  htps.analyze_all();
  timeline_healing_taken.adjust( p.sim -> divisor_timeline );
  // Tank
  deaths.analyze_all();
  theck_meloree_index.analyze_all();

  for ( size_t i = 0; i <  resource_timelines.size(); ++i )
  {
    resource_timelines[ i ].timeline.adjust( p.sim -> divisor_timeline );
  }

  for ( size_t i = 0; i < stat_timelines.size(); ++i )
  {
    stat_timelines[ i ].timeline.adjust( p.sim -> divisor_timeline );
  }

  health_changes.merged_timeline.adjust( p.sim -> divisor_timeline );
}


void player_collected_data_t::print_tmi_debug_csv( const sc_timeline_t* ma, const sc_timeline_t* nma, const std::vector<double>& wv, const player_t& p )
{
  if ( ! p.tmi_debug_file_str.empty() )
  {
    io::ofstream f;
    f.open( p.tmi_debug_file_str );
    // write elements to CSV
    f << p.name_str << " TMI data:\n";

    f << "damage,healing,health chg,norm health chg,mov avg,norm mov avg, weighted val\n";

    for ( size_t i = 0; i < health_changes.timeline.data().size(); i++ )
    {
      f.printf( "%f,%f,%f,%f,%f,%f,%f\n", timeline_dmg_taken.data()[ i ],
          timeline_healing_taken.data()[ i ],
          health_changes.timeline.data()[ i ],
          health_changes.timeline_normalized.data()[ i ],
          ma -> data()[ i ],
          nma -> data()[ i ],
          wv[ i ] );
    }
    f << "\n";
  }
};

void player_collected_data_t::collect_data( const player_t& p )
{

  double f_length = p.iteration_fight_length.total_seconds();
  double sim_length = p.sim -> current_time.total_seconds();
  double w_time = p.iteration_waiting_time.total_seconds();
  assert( p.iteration_fight_length <= p.sim -> current_time );

  fight_length.add( f_length );
  waiting_time.add( w_time );

  executed_foreground_actions.add( p.iteration_executed_foreground_actions );


  // Player only dmg/heal
  dmg.add( p.iteration_dmg );
  heal.add( p.iteration_heal );

  double total_iteration_dmg = p.iteration_dmg; // player + pet dmg
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_dmg += p.pet_list[ i ] -> iteration_dmg;
  }
  double total_iteration_heal = p.iteration_heal; // player + pet heal
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_heal += p.pet_list[ i ] -> iteration_heal;
  }

  compound_dmg.add( total_iteration_dmg );
  dps.add( f_length ? total_iteration_dmg / f_length : 0 );
  dpse.add( sim_length ? total_iteration_dmg / sim_length : 0 );

  compound_heal.add( total_iteration_heal );
  hps.add( f_length ? total_iteration_heal / f_length : 0 );
  hpse.add( sim_length ? total_iteration_heal / sim_length : 0 );

  dmg_taken.add( p.iteration_dmg_taken );
  dtps.add( f_length ? p.iteration_dmg_taken / f_length : 0 );

  heal_taken.add( p.iteration_heal_taken );
  htps.add( f_length ? p.iteration_heal_taken / f_length : 0 );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
    combat_end_resource[ i ].add( p.resources.current[ i ] );

  // Health Change Calculations - only needed for tanks
  if ( ! p.is_pet() && p.primary_role() == ROLE_TANK )
  {
    health_changes.merged_timeline.merge( health_changes.timeline );

    // The Theck-Meloree Index is a metric that attempts to quantize the smoothness of damage intake.
    // It performs an exponentially-weighted sum of the moving average of damage intake, with larger
    // damage spikes being weighted more heavily. A formal definition of the metric can be found here:
    // http://www.sacredduty.net/theck-meloree-index-standard-reference-document/
    if ( ! p.is_enemy() ) // Boss TMI is irrelevant, causes problems in iteration #1 
    {
      double tmi = 0; // TMI result
      if ( f_length )
      {
        // define constants and variables
        int window = 6; // window size, this may become user-definable later
        double w0  = 6; // normalized window size
        double hdf = 3; // default health decade factor

        // create sliding average timeline
        sc_timeline_t sliding_average_tl;
        sc_timeline_t sliding_average_normalized_tl;

        // Use half window size of 3, so it's a moving average over 6 seconds
        health_changes.timeline.build_sliding_average_timeline( sliding_average_tl, window );
        health_changes.timeline_normalized.build_sliding_average_timeline( sliding_average_normalized_tl, window );

        // pull the data out of the normalized sliding average timeline
        std::vector<double> weighted_value = sliding_average_normalized_tl.data();

        for ( size_t j = 0, size = weighted_value.size(); j < size; j++ )
        {
          // normalize to the default window size (6-second window)
          weighted_value[ j ] *= w0;
          
          // calculate exponentially-weighted contribution of this data point
          weighted_value[ j ] = std::exp( 10 * std::log( hdf ) * ( weighted_value[ j ] - 1 ) );

          // add to the TMI total
          tmi += weighted_value [ j ];
        }

        // normalize for fight length
        tmi /= f_length;

        // constant multiplicative normalization factors
        // these are estimates at the moment - will be fine-tuned later
        tmi *= 10000;
        tmi *= std::pow( static_cast<double>( window ) , 2 ); // normalizes for window size

        // if an output file has been defined, write to it
        print_tmi_debug_csv( &sliding_average_tl, &sliding_average_normalized_tl, weighted_value, p );
      }
      // normalize by fight length and add to TMI data array
      theck_meloree_index.add( tmi );
    }
    else
      theck_meloree_index.add( 0.0 );
  }
}

std::ostream& player_collected_data_t::data_str( std::ostream& s ) const
{
  fight_length.data_str( s );
  dmg.data_str( s );
  compound_dmg.data_str( s );
  dps.data_str( s );
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

  return s;
}

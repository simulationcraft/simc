// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

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

  virtual void expire()
  {
    buff_t::expire();
    player -> stat_loss( STAT_MAX_MANA, mana_gain );
  }
};

// Player Ready Event =======================================================

struct player_ready_event_t : public event_t
{
  player_ready_event_t( sim_t*    sim,
                        player_t* p,
                        timespan_t delta_time ) :
    event_t( sim, p, "Player-Ready" )
  {
    if ( sim -> debug ) sim -> output( "New Player-Ready Event: %s", p -> name() );
    sim -> add_event( this, delta_time );
  }

  virtual void execute()
  {
    // Player that's checking for off gcd actions to use, cancels that checking when there's a ready event firing.
    if ( player -> off_gcd )
      event_t::cancel( player -> off_gcd );

    if ( ! player -> execute_action() )
    {
      if ( player -> ready_type == READY_POLL )
      {
        timespan_t x = player -> available();

        player -> schedule_ready( x, true );

        // Waiting Debug
        if ( sim -> debug )
          sim -> output( "%s is waiting for %.4f resource=%.2f",
                         player -> name(), x.total_seconds(),
                         player -> resources.current[ player -> primary_resource() ] );
      }
      else player -> started_waiting = sim -> current_time;
    }
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

  std::string::size_type cut_pt;

  if ( url.find( ".battle.net" ) != url.npos )
  {
    if ( ( cut_pt = url.find_first_of( '#' ) ) != url.npos )
    {
      if ( sim -> talent_format == TALENT_FORMAT_UNCHANGED )
        sim -> talent_format = TALENT_FORMAT_ARMORY;
      return p -> parse_talents_armory( url.substr( cut_pt + 1 ) );
    }
  }
  else if ( url.find( "mop.wowhead.com" ) != url.npos )
  {
    if ( ( cut_pt = url.find_first_of( "#" ) ) != url.npos )
    {
      if ( sim -> talent_format == TALENT_FORMAT_UNCHANGED )
        sim -> talent_format = TALENT_FORMAT_WOWHEAD;
      return p -> parse_talents_wowhead( url.substr( cut_pt + 1 ) );
    }
  }
  else
  {
    bool all_digits = true;
    for ( size_t i=0; i < url.size() && all_digits; i++ )
      if ( ! isdigit( url[ i ] ) )
        all_digits = false;

    if ( all_digits )
    {
      if ( sim -> talent_format == TALENT_FORMAT_UNCHANGED )
        sim -> talent_format = TALENT_FORMAT_NUMBERS;
      return p -> parse_talents_numbers( url );
    }
  }

  sim -> errorf( "Unable to decode talent string %s for %s\n", url.c_str(), p -> name() );

  return false;
}

// parse_talent_override =========================================================

bool parse_talent_override( sim_t* sim,
                            const std::string& name,
                            const std::string& override_str )
{
  assert( name == "talent_override" ); ( void )name;

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

// parse_specialization ======================================================

bool parse_specialization( sim_t* sim,
                           const std::string&,
                           const std::string& value )
{
  sim -> active_player -> _spec = util::translate_spec_str( sim -> active_player -> type, value );

  if ( sim -> active_player -> _spec == SPEC_NONE )
    sim->errorf( "\n%s specialization string \"%s\" not valid.\n", sim -> active_player->name(), value.c_str() );

  return true;
}

} // UNNAMED NAMESPACE ===================================================

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
    player_t* target;
    double additional_ignite_dmg;
    timespan_t application_delay;
    action_t* action;

    delay_event_t( sim_t* sim, player_t* t, action_t* a, double dmg ) :
      event_t( sim, a -> player, "Ignite Sampling Event" ), target( t ), additional_ignite_dmg( dmg ),
      action( a )
    {
      // Use same delay as in buff application
      timespan_t delay_duration = sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );

      if ( sim -> debug )
        sim -> output( "New %s Sampling Event: %s ( delta time: %.4f )",
                       action -> name(), player -> name(), delay_duration.total_seconds() );

      sim -> add_event( this, delay_duration );
    }

    virtual void execute()
    {
      assert( action );

      dot_t* dot = action -> get_dot( target );

      double new_total_ignite_dmg = additional_ignite_dmg;

      assert( action -> num_ticks > 0 );

      if ( dot -> ticking )
        new_total_ignite_dmg += action -> base_td * dot -> ticks();

      if ( sim -> debug )
      {
        if ( dot -> ticking )
        {
          sim_t::output( sim, "ignite_delay_event::execute(): additional_damage=%f current_ignite_tick=%f ticks_left=%d new_ignite_dmg=%f",
                         additional_ignite_dmg, action -> base_td, dot -> ticks(), new_total_ignite_dmg );
        }
        else
        {
          sim_t::output( sim, "ignite_delay_event::execute(): additional_damage=%f new_ignite_dmg=%f",
                         additional_ignite_dmg, new_total_ignite_dmg );
        }
      }

      // Pass total amount of damage to the ignite action, and let it divide it by the correct number of ticks!

      // action execute
      {
        action_state_t* s = action -> get_state();
        s -> target = target;
        s -> result = RESULT_HIT;
        action -> snapshot_state( s, action -> snapshot_flags, action -> type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );
        s -> result_amount = new_total_ignite_dmg;
        action -> schedule_travel( s );
        if ( ! action -> dual ) action -> stats -> add_execute( timespan_t::zero() );
      }
    }
  };

  new ( ignite_action -> sim ) delay_event_t( ignite_action -> sim, t, ignite_action, dmg );
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
  double base_power = std::max( a -> player -> composite_attack_power() * a -> player -> composite_attack_power_multiplier() * 0.2,
                                a -> player -> composite_spell_power( a -> school ) * a -> player -> composite_spell_power_multiplier() * 0.3 );
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
    a -> sim -> output( "%s stormlash proc by %s: power=%.3f base_mul=%.3f cast_time=%.3f cast_time_mul=%.3f spell_multiplier=%.3f amount=%.3f %s",
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
    stormlash_aggregate_stat -> add_execute( timespan_t::zero() );
    stormlash_aggregate_stat -> add_result( stormlash_spell -> execute_state -> result_amount,
                                            stormlash_spell -> execute_state -> result_amount,
                                            stormlash_spell -> execute_state -> result_type,
                                            stormlash_spell -> execute_state -> result );
  }
}

stormlash_buff_t::stormlash_buff_t( player_t* p, const spell_data_t* s ) :
  buff_t( buff_creator_t( p, "stormlash", s ).cd( timespan_t::from_seconds( 0.1 ) ) ),
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

void stormlash_buff_t::expire()
{
  buff_t::expire();

  if ( stormlash_cb -> stormlash_aggregate == stormlash_aggregate )
    stormlash_cb -> stormlash_aggregate = 0;
  stormlash_cb -> deactivate();
}

// ==========================================================================
// Player
// ==========================================================================

// player_t::player_t =======================================================

player_t::player_t( sim_t*             s,
                    player_e      t,
                    const std::string& n,
                    race_e        r ) :
  sim( s ),
  type( t ),
  name_str( n ),

  index( -1 ),

  // (static) attributes
  race( r ),
  role( ROLE_HYBRID ),
  level( is_enemy() ? 93 : 90 ),
  party( 0 ), member( 0 ),
  ready_type( READY_POLL ),
  _spec( SPEC_NONE ),
  bugs( true ),
  scale_player( 1 ),
  has_dtr( false ),
  dtr_proc_chance( -1.0 ),
  challenge_mode_power_loss_ratio( 1.0 ),

  simple_actions( false ),

  // dynamic stuff
  target( 0 ),
  active_pets( 0 ),
  initialized( 0 ), potion_used( false ),

  region_str( s -> default_region_str ), server_str( s -> default_server_str ), origin_str( "unknown" ),
  gcd_ready( timespan_t::zero() ), base_gcd( timespan_t::from_seconds( 1.5 ) ), started_waiting( timespan_t::zero() ),
  pet_list( 0 ), invert_scaling( 0 ),
  reaction_offset( timespan_t::from_seconds( 0.1 ) ), reaction_mean( timespan_t::from_seconds( 0.3 ) ), reaction_stddev( timespan_t::zero() ), reaction_nu( timespan_t::from_seconds( 0.25 ) ),
  avg_ilvl( 0 ),
  vengeance( false ),
  // Latency
  world_lag( timespan_t::from_seconds( 0.1 ) ), world_lag_stddev( timespan_t::min() ),
  brain_lag( timespan_t::zero() ), brain_lag_stddev( timespan_t::min() ),
  world_lag_override( false ), world_lag_stddev_override( false ),
  events( 0 ),
  dbc( s -> dbc ),
  autoUnshift( true ),
  talent_list(),
  glyph_list(),
  // Haste
  spell_haste( 1.0 ), attack_haste( 1.0 ),
  // Mastery
  base( base_initial_current_t() ),
  initial( base_initial_current_t() ),
  current( base_initial_current_t() ),
  // Spell Mechanics
  mana_regen_base( 0 ),
  base_energy_regen_per_second( 0 ), base_focus_regen_per_second( 0 ), base_chi_regen_per_second( 0 ),
  last_cast( timespan_t::zero() ),
  // Defense Mechanics
  target_auto_attack( 0 ),
  diminished_dodge_cap( 0 ), diminished_parry_cap( 0 ), diminished_block_cap( 0 ), diminished_kfactor( 0 ),
  armor_coeff( 0 ),
  half_resistance_rating( 0 ),
  // Attacks
  main_hand_attack( 0 ), off_hand_attack( 0 ),
  // Resources
  resources( resources_t() ),
  // Consumables
  elixir_guardian( ELIXIR_NONE ),
  elixir_battle( ELIXIR_NONE ),
  flask( FLASK_NONE ),
  food( FOOD_NONE ),
  // Events
  executing( 0 ), channeling( 0 ), readying( 0 ), off_gcd( 0 ), in_combat( false ), action_queued( false ),
  cast_delay_reaction( timespan_t::zero() ), cast_delay_occurred( timespan_t::zero() ),
  // Actions
  action_list_default( 0 ),
  precombat_action_list( 0 ), active_action_list( 0 ), active_off_gcd_list( 0 ), restore_action_list( 0 ),
  // Reporting
  quiet( 0 ), last_foreground_action( 0 ),
  iteration_fight_length( timespan_t::zero() ), arise_time( timespan_t::min() ),
  fight_length( s -> statistics_level < 2, true ), waiting_time( true ), executed_foreground_actions( s -> statistics_level < 3 ),
  iteration_waiting_time( timespan_t::zero() ), iteration_executed_foreground_actions( 0 ),
  rps_gain( 0 ), rps_loss( 0 ),
  deaths( false ), deaths_error( 0 ),
  buffed( buffed_stats_t() ),
  resource_timeline_count( 0 ),
  // Damage
  iteration_dmg( 0 ), iteration_dmg_taken( 0 ),
  dps_error( 0 ), dpr( 0 ), dtps_error( 0 ),
  dmg( s -> statistics_level < 2 ), compound_dmg( s -> statistics_level < 2 ),
  dps( name_str + " Damage Per Second", s -> statistics_level < 1 ), dpse( s -> statistics_level < 2 ),
  dtps( name_str + " Damage Taken Per Second", s -> statistics_level < 2 ), dmg_taken( s -> statistics_level < 2 ),
  dps_convergence( 0 ),
  // Heal
  iteration_heal( 0 ),iteration_heal_taken( 0 ),
  hps_error( 0 ), hpr( 0 ),
  heal( s -> statistics_level < 2 ), compound_heal( s -> statistics_level < 2 ),
  hps( name_str + " Healing Per Second", s -> statistics_level < 1 ), hpse( s -> statistics_level < 2 ),
  htps( name_str + " Healing taken Per Second", s -> statistics_level < 2 ), heal_taken( s -> statistics_level < 2 ),
  report_information( report_information_t() ),
  // Gear
  sets( 0 ),
  meta_gem( META_GEM_NONE ), matching_gear( false ),
  // Scaling
  scaling_lag( 0 ), scaling_lag_error( 0 ),
  // Movement & Position
  base_movement_speed( 7.0 ), x_position( 0.0 ), y_position( 0.0 ),
  buffs( buffs_t() ),
  potion_buffs( potion_buffs_t() ),
  debuffs( debuffs_t() ),
  gains( gains_t() ),
  procs( procs_t() ),
  rngs( rngs_t() ),
  uptimes( uptimes_t() )
{
  item_cooldown = new cooldown_t( "item_cd", this );
  sim -> actor_list.push_back( this );

  initial.skill = s -> default_skill;
  base.mastery = 8.0;

  if ( !is_enemy() && type != HEALING_ENEMY )
  {
    if ( sim -> debug ) sim -> output( "Creating Player %s", name() );
    sim -> player_list.push_back( this );
    index = ++( sim -> num_players );
  }
  else
  {
    index = - ( ++( sim -> num_enemies ) );
  }

  if ( is_pet() ) current.skill = 1.0;

  range::fill( current.attribute, 0 );
  range::fill( base.attribute, 0 );
  range::fill( initial.attribute, 0 );

  range::fill( spell_resistance, 0 );

  resources.infinite_resource[ RESOURCE_HEALTH ] = true;

  range::fill( resource_lost, 0 );
  range::fill( resource_gained, 0 );

  range::fill( profession, 0 );

  range::fill( scales_with, false );
  range::fill( over_cap, 0 );

  items.resize( SLOT_MAX );
  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
  {
    items[ i ].slot = i;
    items[ i ].sim = sim;
    items[ i ].player = this;
  }

  main_hand_weapon.slot = SLOT_MAIN_HAND;
  off_hand_weapon.slot = SLOT_OFF_HAND;

  if ( reaction_stddev == timespan_t::zero() )
    reaction_stddev = reaction_mean * 0.2;
}

player_t::base_initial_current_t::base_initial_current_t() :
  attribute(),
  mastery( 0 ),
  mastery_rating( 0 ),
  haste_rating( 0 ),
  spell_hit( 0 ),
  spell_crit( 0 ),
  mp5( 0 ),
  attack_power( 0 ),
  attack_hit( 0 ),
  attack_expertise( 0 ),
  attack_crit( 0 ),
  armor( 0 ),
  bonus_armor( 0 ),
  miss( 0 ),
  dodge( 0 ),
  parry( 0 ),
  block( 0 ),
  block_reduction( 0 ),
  resource_reduction(),
  spell_power(),
  spell_power_per_intellect( 0 ),
  spell_crit_per_intellect( 0 ),
  attack_power_per_strength( 0 ),
  attack_power_per_agility( 0 ),
  attack_crit_per_agility( 0 ),
  dodge_per_agility( 0 ),
  parry_rating_per_strength( 0 ),
  mp5_per_spirit( 0 ),
  mp5_from_spirit_multiplier( 0 ),
  health_per_stamina( 0 ),
  skill( 0 ),
  distance( 0 ),
  sleeping( false ),
  attribute_multiplier(),
  spell_power_multiplier( 1.0 ),
  attack_power_multiplier( 1.0 ),
  armor_multiplier( 1.0 ),
  position( POSITION_BACK )
{
  range::fill( attribute_multiplier, 1 );
}

// player_t::~player_t ======================================================

player_t::~player_t()
{
  delete item_cooldown;
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
    sim -> output( "Initializing Auras, Buffs, and De-Buffs." );

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
                                           .default_value( std::fabs( p -> find_spell( 113746 ) -> effectN( 1 ).percent() ) );

    p -> debuffs.weakened_blows          = buff_creator_t( p, "weakened_blows", p -> find_spell( 115798 ) )
                                           .default_value( std::fabs( p -> find_spell( 115798 ) -> effectN( 1 ).percent() ) );
  }

  return true;
}

// init_parties =============================================================

static bool init_parties( sim_t* sim )
{
  // Parties
  if ( sim -> debug )
    sim -> output( "Building Parties." );

  int party_index=0;
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

      std::vector<std::string> player_names;
      size_t num_players = util::string_split( player_names, party_str, ",;/" );
      int member_index=0;

      for ( size_t j = 0; j < num_players; j++ )
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
    sim -> output( "Creating Pets." );

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> create_pets();
  }

  if ( ! init_debuffs( sim ) )
    return false;

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    module_t* m = module_t::get( i );
    if ( m ) m -> init( sim );
  }

  if ( sim -> debug )
    sim -> output( "Initializing Players." );

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

  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_target ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_race ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_talents ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_glyphs ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::replace_spells ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_spells ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_rating ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_racials ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_position ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_professions ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_items ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_base ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_core ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_spell ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_attack ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_defense ) );
  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[ i ];
    p -> init_weapon ( ( p -> main_hand_weapon ) );
  }
  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[ i ];
    p -> init_weapon( ( p -> off_hand_weapon ) );
  }
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_professions_bonus ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_unique_gear ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_enchant ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_scaling ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_buffs ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_values ) );
  range::for_each( sim -> actor_list, std::mem_fn( &player_t::init_actions ) );
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
    sim -> output( "Registering Callbacks." );

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
  if ( sim -> debug ) sim -> output( "Initializing player %s", name() );

  // Ensure the precombat and default lists are the first listed
  get_action_priority_list( "precombat" ) -> used = true;
  get_action_priority_list( "default" );

  for ( std::map<std::string,std::string>::iterator it = alist_map.begin(), end = alist_map.end(); it != end; ++it )
  {
    if ( it -> first == "default" )
      sim -> errorf( "Ignoring action list named default." );
    else
      get_action_priority_list( it -> first ) -> action_list_str = it -> second;
  }
}

// player_t::init_base ======================================================

void player_t::init_base()
{
  if ( sim -> debug ) sim -> output( "Initializing base for player (%s)", name() );

  base.attribute[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_STRENGTH );
  base.attribute[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_AGILITY );
  base.attribute[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_STAMINA );
  base.attribute[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_INTELLECT );
  base.attribute[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_SPIRIT );
  resources.base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_HEALTH );
  resources.base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_MANA );
  base.spell_crit                  = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_SPELL_CRIT );
  base.attack_crit                 = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_MELEE_CRIT );
  initial.spell_crit_per_intellect = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial.attack_crit_per_agility  = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_MELEE_CRIT_PER_AGI );
  base.mp5                         = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_MP5 );
  initial.health_per_stamina = dbc.health_per_stamina( level );
  initial.mp5_per_spirit = dbc.mp5_per_spirit( type, level );

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
      base.attribute[ a ] *= 1.0 + matching_gear_multiplier( a );
      base.attribute[ a ] = util::floor( base.attribute[ a ] );
    }
  }

  if ( world_lag_stddev < timespan_t::zero() ) world_lag_stddev = world_lag * 0.1;
  if ( brain_lag_stddev < timespan_t::zero() ) brain_lag_stddev = brain_lag * 0.1;

  // Collect DTPS data for tanks even for statistics_level == 1
  if ( sim -> statistics_level >= 1 && role == ROLE_TANK )
    dtps.simple = false;
}

// player_t::init_items =====================================================

void player_t::init_items()
{
  if ( is_pet() ) return;

  if ( sim -> debug ) sim -> output( "Initializing items for player (%s)", name() );

  std::vector<std::string> splits;
  util::string_split( splits, items_str, "/" );
  for ( size_t i = 0; i < splits.size(); i++ )
  {
    if ( find_item( splits[ i ] ) )
    {
      sim -> errorf( "Player %s has multiple %s equipped.\n", name(), splits[ i ].c_str() );
    }
    items.push_back( item_t( this, splits[ i ] ) );
  }

  gear_stats_t item_stats;

  bool slots[ SLOT_MAX ];
  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    slots[ i ] = ! util::armor_type_string( type, i );

  unsigned num_ilvl_items = 0;
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

    if ( item.slot != SLOT_SHIRT && item.slot != SLOT_TABARD && item.slot != SLOT_RANGED && item.active() )
    {
      avg_ilvl += item.ilevel;
      num_ilvl_items++;
    }

    slots[ item.slot ] = item.matching_type();

    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
      item_stats.add_stat( j, item.stats.get_stat( j ) );
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

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( gear.get_stat( i ) == 0 )
      gear.add_stat( i, item_stats.get_stat( i ) );
  }

  if ( sim -> challenge_mode && !is_enemy() && this!= sim -> heal_target ) //scale gear to itemlevel 463
  {
    int target_level=463;
    //Calculate power for the itemlevel
    double current_power=0.0394445687657227 * pow( avg_ilvl, 2 ) - 27.9535606063565 *avg_ilvl + 5385.46680173828;
    double target_power=0.0394445687657227 * pow( ( double ) target_level, 2 ) - 27.9535606063565 *target_level + 5385.46680173828;
    challenge_mode_power_loss_ratio = target_power/current_power;

    //reduce primary stats by challenge_mode_power_loss_ratio
    gear.set_stat( STAT_STRENGTH, floor( gear.get_stat( STAT_STRENGTH )*challenge_mode_power_loss_ratio ) );
    gear.set_stat( STAT_AGILITY, floor( gear.get_stat( STAT_AGILITY )*challenge_mode_power_loss_ratio ) );
    gear.set_stat( STAT_STAMINA, floor( gear.get_stat( STAT_STAMINA )*challenge_mode_power_loss_ratio ) );
    gear.set_stat( STAT_INTELLECT, floor( gear.get_stat( STAT_INTELLECT )*challenge_mode_power_loss_ratio ) );

    int old_rating_sum=0;

    old_rating_sum+=( int )gear.get_stat( STAT_SPIRIT );
    old_rating_sum+=( int )gear.get_stat( STAT_EXPERTISE_RATING );
    old_rating_sum+=( int )gear.get_stat( STAT_HIT_RATING );
    old_rating_sum+=( int )gear.get_stat( STAT_CRIT_RATING );
    old_rating_sum+=( int )gear.get_stat( STAT_HASTE_RATING );
    old_rating_sum+=( int )gear.get_stat( STAT_DODGE_RATING );
    old_rating_sum+=( int )gear.get_stat( STAT_PARRY_RATING );
    old_rating_sum+=( int )gear.get_stat( STAT_BLOCK_RATING );
    old_rating_sum+=( int )gear.get_stat( STAT_MASTERY_RATING );

    int target_rating_sum_wo_hit_exp=( int )( floor( old_rating_sum*challenge_mode_power_loss_ratio ) - gear.get_stat( STAT_EXPERTISE_RATING ) - gear.get_stat( STAT_HIT_RATING ) );

    //hit/exp stay the same
    //every secondary stat gets a share of the target_rating_sum according to its previous ratio (without hit/exp)
    int old_rating_sum_wo_hit_exp=( int )( old_rating_sum-gear.get_stat( STAT_EXPERTISE_RATING )-gear.get_stat( STAT_HIT_RATING ) );

    gear.set_stat( STAT_SPIRIT,floor( gear.get_stat( STAT_SPIRIT )/old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    gear.set_stat( STAT_CRIT_RATING,floor( gear.get_stat( STAT_CRIT_RATING )/old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    gear.set_stat( STAT_HASTE_RATING,floor( gear.get_stat( STAT_HASTE_RATING )/old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    gear.set_stat( STAT_DODGE_RATING,floor( gear.get_stat( STAT_DODGE_RATING )/old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    gear.set_stat( STAT_PARRY_RATING,floor( gear.get_stat( STAT_PARRY_RATING )/old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    gear.set_stat( STAT_BLOCK_RATING,floor( gear.get_stat( STAT_BLOCK_RATING )/old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );
    gear.set_stat( STAT_MASTERY_RATING,floor( gear.get_stat( STAT_MASTERY_RATING )/old_rating_sum_wo_hit_exp * target_rating_sum_wo_hit_exp ) );


    //scale AP/SP just by challenge_mode_power_loss_ratio
    gear.set_stat( STAT_ATTACK_POWER,floor( gear.get_stat( STAT_ATTACK_POWER )*challenge_mode_power_loss_ratio ) );
    gear.set_stat( STAT_SPELL_POWER,floor( gear.get_stat( STAT_SPELL_POWER )*challenge_mode_power_loss_ratio ) );


    //FIXME, this just doesn't change a thing. It seems as if the code is not using this data at all.

    //for weapon dps, just use a standard 463 weapon, no need to toy around with rounding errors
    weapon_t *main_hand=items[SLOT_MAIN_HAND].weapon();
    if ( main_hand -> type <= WEAPON_1H ) //any 1h
    {
      main_hand -> dps = 2436.7;
    }
    else if ( main_hand -> type <= WEAPON_RANGED ) //any 2h or ranged
    {
      main_hand -> dps = 3285.4;
    }

    //update min_max according to weapon_speed

    main_hand -> min_dmg = 0.7*main_hand -> dps * main_hand -> swing_time.total_seconds();
    main_hand -> max_dmg = 1.3*main_hand -> dps * main_hand -> swing_time.total_seconds();

    weapon_t *off_hand=items[SLOT_OFF_HAND].weapon();

    if ( off_hand -> type<=WEAPON_1H ) //any 1h
    {
      off_hand -> dps = 2436.7;
    }
    else if ( off_hand -> type<=WEAPON_RANGED ) //any 2h or ranged
    {
      off_hand -> dps = 3285.4;

    }
    off_hand -> min_dmg = 0.7*off_hand -> dps * off_hand -> swing_time.total_seconds();
    off_hand -> max_dmg = 1.3*off_hand -> dps * off_hand -> swing_time.total_seconds();


    //every 463 plate set has 45141 armor
    //so we just give this to any DK,Paladin and Warrior
    //every 463 leather set has 35028 armor
    //so we just give this to any druid, and monk, and rogue
    switch ( type )
    {
    case WARRIOR:
    case DEATH_KNIGHT:
    case PALADIN:
      gear.set_stat( STAT_ARMOR, 45141 );
      break;
    case DRUID:
    case MONK:
    case ROGUE:
      gear.set_stat( STAT_ARMOR,15922 );
    default:
      break;
    }



    //FIXME add gear sets for cloth and mail wearer
    //FIXME what about bonus armor/MP5, neither are on mop gear.....




  }
  if ( sim -> debug )
  {
    sim -> output( "%s gear:", name() );
    gear.print( sim -> output_file );
  }

  set_bonus.init( this );

  // Detect DTR
  if ( find_item( "dragonwrath_tarecgosas_rest" ) )
    has_dtr = true;
}

// player_t::init_meta_gem ==================================================

void player_t::init_meta_gem( gear_stats_t& item_stats )
{
  if ( ! meta_gem_str.empty() ) meta_gem = util::parse_meta_gem_type( meta_gem_str );

  if ( sim -> debug ) sim -> output( "Initializing meta-gem for player (%s)", name() );

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
    unique_gear::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "mystical_skyfire", this, STAT_HASTE_RATING, 1, 320, 0.15, timespan_t::from_seconds( 4.0 ), timespan_t::from_seconds( 45.0 ) );
  }
  else if ( meta_gem == META_INSIGHTFUL_EARTHSTORM )
  {
    unique_gear::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "insightful_earthstorm", this, STAT_MANA, 1, 300, 0.05, timespan_t::zero(), timespan_t::from_seconds( 15.0 ) );
  }
  else if ( meta_gem == META_INSIGHTFUL_EARTHSIEGE )
  {
    unique_gear::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "insightful_earthsiege", this, STAT_MANA, 1, 600, 0.05, timespan_t::zero(), timespan_t::from_seconds( 15.0 ) );
  }
}

// player_t::init_core ======================================================

void player_t::init_core()
{
  if ( sim -> debug ) sim -> output( "Initializing core for player (%s)", name() );

  initial_stats.  hit_rating = gear.  hit_rating + enchant.  hit_rating + ( is_pet() ? 0 : sim -> enchant.  hit_rating );
  initial_stats. crit_rating = gear. crit_rating + enchant. crit_rating + ( is_pet() ? 0 : sim -> enchant. crit_rating );
  initial_stats.haste_rating = gear.haste_rating + enchant.haste_rating + ( is_pet() ? 0 : sim -> enchant.haste_rating );
  initial_stats.mastery_rating = gear.mastery_rating + enchant.mastery_rating + ( is_pet() ? 0 : sim -> enchant.mastery_rating );

  initial.haste_rating   = base.haste_rating + initial_stats.haste_rating;
  initial.mastery_rating = base.mastery_rating + initial_stats.mastery_rating;
  initial.mastery        = base.mastery;

  for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
  {
    initial_stats.attribute[ i ] = gear.attribute[ i ] + enchant.attribute[ i ] + ( is_pet() ? 0 : sim -> enchant.attribute[ i ] );

    current.attribute[ i ] = initial.attribute[ i ] = base.attribute[ i ] + initial_stats.attribute[ i ];
  }
}

// player_t::init_position ==================================================

void player_t::init_position()
{
  if ( sim -> debug )
    sim -> output( "Initializing position for player (%s)", name() );

  if ( position_str.empty() )
  {
    position_str = util::position_type_string( base.position );
  }
  else
  {
    base.position = util::parse_position_type( position_str );
  }

  // default to back when we have an invalid position
  if ( base.position == POSITION_NONE )
  {
    sim -> errorf( "Player %s has an invalid position of %s, defaulting to back.\n", name(), position_str.c_str() );
    base.position = POSITION_BACK;
    position_str = util::position_type_string( base.position );
  }

  initial.position = base.position;
}

// player_t::init_race ======================================================

void player_t::init_race()
{
  if ( sim -> debug )
    sim -> output( "Initializing race for player (%s)", name() );

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

// player_t::init_racials ===================================================

void player_t::init_racials()
{
  if ( sim -> debug )
    sim -> output( "Initializing racials for player (%s)", name() );
}

// player_t::init_spell =====================================================

void player_t::init_spell()
{
  if ( sim -> debug )
    sim -> output( "Initializing spells for player (%s)", name() );

  initial_stats.spell_power = gear.spell_power + enchant.spell_power + ( is_pet() ? 0 : sim -> enchant.spell_power );
  initial_stats.mp5         = gear.mp5         + enchant.mp5         + ( is_pet() ? 0 : sim -> enchant.mp5 );

  initial.spell_power[ SCHOOL_MAX ] = initial_stats.spell_power;

  initial.spell_hit = base.spell_hit + initial_stats.hit_rating / rating.spell_hit;

  initial.spell_crit = base.spell_crit + initial_stats.crit_rating / rating.spell_crit;

  initial.mp5 = base.mp5 + initial_stats.mp5;

  initial.spell_power_multiplier = 1.0;

  if ( !is_enemy() )
    mana_regen_base = dbc.regen_spirit( type, level );

  if ( level >= 61 )
  {
    half_resistance_rating = 150.0 + ( level - 60 ) * ( level - 67.5 );
  }
  else if ( level >= 21 )
  {
    half_resistance_rating = 50.0 + ( level - 20 ) * 2.5;
  }
  else
  {
    half_resistance_rating = 50.0;
  }
}

// player_t::init_attack ====================================================

void player_t::init_attack()
{
  if ( sim -> debug )
    sim -> output( "Initializing attack for player (%s)", name() );

  initial_stats.attack_power     = gear.attack_power     + enchant.attack_power     + ( is_pet() ? 0 : sim -> enchant.attack_power );
  initial_stats.expertise_rating = gear.expertise_rating + enchant.expertise_rating + ( is_pet() ? 0 : sim -> enchant.expertise_rating );

  initial.attack_power     = base.attack_power     + initial_stats.attack_power;
  initial.attack_hit       = base.attack_hit       + initial_stats.hit_rating       / rating.attack_hit;
  initial.attack_crit      = base.attack_crit      + initial_stats.crit_rating      / rating.attack_crit;
  initial.attack_expertise = base.attack_expertise + initial_stats.expertise_rating / rating.expertise;

  initial.attack_power_multiplier = 1.0;

}

// player_t::init_defense ===================================================

void player_t::init_defense()
{
  if ( sim -> debug )
    sim -> output( "Initializing defense for player (%s)", name() );

  initial_stats.armor        = gear.armor        + enchant.armor        + ( is_pet() ? 0 : sim -> enchant.armor );
  initial_stats.bonus_armor  = gear.bonus_armor  + enchant.bonus_armor  + ( is_pet() ? 0 : sim -> enchant.bonus_armor );
  initial_stats.dodge_rating = gear.dodge_rating + enchant.dodge_rating + ( is_pet() ? 0 : sim -> enchant.dodge_rating );
  initial_stats.parry_rating = gear.parry_rating + enchant.parry_rating + ( is_pet() ? 0 : sim -> enchant.parry_rating );
  initial_stats.block_rating = gear.block_rating + enchant.block_rating + ( is_pet() ? 0 : sim -> enchant.block_rating );

  initial.armor           = base.armor       + initial_stats.armor;
  initial.bonus_armor     = base.bonus_armor + initial_stats.bonus_armor;
  initial.miss            = base.miss;
  initial.dodge           = base.dodge       + initial_stats.dodge_rating / rating.dodge;
  initial.parry           = base.parry       + initial_stats.parry_rating / rating.parry;
  initial.block           = base.block       + initial_stats.block_rating / rating.block;
  initial.block_reduction = base.block_reduction;

  if ( !is_enemy()  )
  {
    initial.dodge_per_agility = dbc.dodge_scaling( type, level );
    initial.parry_rating_per_strength = 0.0;
  }

  if ( primary_role() == ROLE_TANK )
    base.position = POSITION_FRONT;

  // Armor Coefficient
  double a,b;
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
  armor_coeff = a * level + b;
}

// player_t::init_weapon ====================================================

void player_t::init_weapon( weapon_t& w )
{
  if ( sim -> debug )
    sim -> output( "Initializing weapon ( type %s ) for player (%s)", util::weapon_type_string( w.type ), name() );

  if ( w.type == WEAPON_NONE ) return;

  if ( w.slot == SLOT_MAIN_HAND ) assert( w.type >= WEAPON_NONE && w.type < WEAPON_RANGED );
  if ( w.slot == SLOT_OFF_HAND  ) assert( w.type >= WEAPON_NONE && w.type < WEAPON_2H );
}

// player_t::init_unique_gear ===============================================

void player_t::init_unique_gear()
{
  if ( sim -> debug ) sim -> output( "Initializing unique gear for player (%s)", name() );

  unique_gear::init( this );
}

// player_t::init_enchant ===================================================

void player_t::init_enchant()
{
  if ( sim -> debug ) sim -> output( "Initializing enchants for player (%s)", name() );

  enchant::init( this );
}

// player_t::init_resources =================================================

void player_t::init_resources( bool force )
{
  if ( sim -> debug ) sim -> output( "Initializing resources for player (%s)", name() );

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

  if ( resource_timeline_count == 0 )
  {
    int size = ( int ) ( sim -> max_time.total_seconds() * ( 1.0 + sim -> vary_combat_length ) );
    if ( size <= 0 ) size = 600; // Default to 10 minutes
    size *= 2;
    size += 3; // Buffer against rounding.

    for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
    {
      if ( resources.max[ i ] > 0 )
      {
        // If you trigger this assert, resource_timelines needs to be bigger.
        assert( resource_timeline_count < resource_timelines.size() );
        resource_timelines[ resource_timeline_count ].type = i;
        resource_timelines[ resource_timeline_count ].timeline.assign( size, 0 );
        ++resource_timeline_count;
      }
    }
  }
}

// player_t::init_professions ===============================================

void player_t::init_professions()
{
  if ( professions_str.empty() ) return;

  if ( sim -> debug ) sim -> output( "Initializing professions for player (%s)", name() );

  std::vector<std::string> splits;
  util::string_split( splits, professions_str, ",/" );

  for ( unsigned int i = 0; i < splits.size(); ++i )
  {
    std::string prof_name;
    int prof_value=0;

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

// player_t::init_professions_bonus =========================================

void player_t::init_professions_bonus()
{
  if ( sim -> debug ) sim -> output( "Initializing professions bonuses for player (%s)", name() );

  // This has to be called after init_attack() and init_core()

  // Miners gain additional stamina
  if      ( profession[ PROF_MINING ] >= 600 ) initial.attribute[ ATTR_STAMINA ] += 480.0;
  else if ( profession[ PROF_MINING ] >= 525 ) initial.attribute[ ATTR_STAMINA ] += 120.0;
  else if ( profession[ PROF_MINING ] >= 450 ) initial.attribute[ ATTR_STAMINA ] +=  60.0;
  else if ( profession[ PROF_MINING ] >= 375 ) initial.attribute[ ATTR_STAMINA ] +=  30.0;
  else if ( profession[ PROF_MINING ] >= 300 ) initial.attribute[ ATTR_STAMINA ] +=  10.0;
  else if ( profession[ PROF_MINING ] >= 225 ) initial.attribute[ ATTR_STAMINA ] +=   7.0;
  else if ( profession[ PROF_MINING ] >= 150 ) initial.attribute[ ATTR_STAMINA ] +=   5.0;
  else if ( profession[ PROF_MINING ] >=  75 ) initial.attribute[ ATTR_STAMINA ] +=   3.0;

  // Skinners gain additional crit rating
  if      ( profession[ PROF_SKINNING ] >= 600 )
  {
    initial.attack_crit += 480.0 / rating.attack_crit;
    initial.spell_crit += 480.0 / rating.spell_crit;
  }
  else if ( profession[ PROF_SKINNING ] >= 525 )
  {
    initial.attack_crit += 80.0 / rating.attack_crit;
    initial.spell_crit += 80.0 / rating.spell_crit;
  }
  else if ( profession[ PROF_SKINNING ] >= 450 )
  {
    initial.attack_crit += 40.0 / rating.attack_crit;
    initial.spell_crit += 40.0 / rating.spell_crit;
  }
  else if ( profession[ PROF_SKINNING ] >= 375 )
  {
    initial.attack_crit += 20.0 / rating.attack_crit;
    initial.spell_crit += 20.0 / rating.spell_crit;
  }
  else if ( profession[ PROF_SKINNING ] >= 300 )
  {
    initial.attack_crit += 12.0 / rating.attack_crit;
    initial.spell_crit += 12.0 / rating.spell_crit;
  }
  else if ( profession[ PROF_SKINNING ] >= 225 )
  {
    initial.attack_crit +=  9.0 / rating.attack_crit;
    initial.spell_crit +=  9.0 / rating.spell_crit;
  }
  else if ( profession[ PROF_SKINNING ] >= 150 )
  {
    initial.attack_crit +=  6.0 / rating.attack_crit;
    initial.spell_crit +=  6.0 / rating.spell_crit;
  }
  else if ( profession[ PROF_SKINNING ] >=  75 )
  {
    initial.attack_crit +=  3.0 / rating.attack_crit;
    initial.spell_crit +=  3.0 / rating.spell_crit;
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

    if ( pet_action -> player -> current.sleeping )
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
    if ( items[ i ].use.active() )
    {
      buffer += "/use_item,name=";
      buffer += items[ i ].name();
      if ( ! append.empty() )
      {
        buffer += append;
      }
    }
  }
  if ( items[ SLOT_HANDS ].use.active() )
  {
    buffer += "/use_item,name=";
    buffer += items[ SLOT_HANDS ].name();
    if ( ! append.empty() )
    {
      buffer += append;
    }
  }

  return buffer;
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

/* Helper function to add actions with spell data of the same name to the action list,
 * and check if that spell data is ok()
 */

void player_t::add_action( std::string action, std::string options, std::string alist )
{
  add_action( find_class_spell( action ), options, alist );
}

// Helper function to add actions to the action list if given spell data is ok()

void player_t::add_action( const spell_data_t* s, std::string options, std::string alist )
{
  std::string *str = ( alist == "default" ) ? &action_list_str : &( get_action_priority_list( alist ) -> action_list_str );
  if ( s -> ok() )
  {
    *str += "/" + dbc_t::get_token( s -> id() );
    if ( ! options.empty() ) *str += "," + options;
  }
}

// player_t::init_actions ===================================================

void player_t::init_actions()
{
  std::string modify_action_options = "";

  if ( ! modify_action.empty() )
  {
    std::string::size_type cut_pt = modify_action.find( "," );

    if ( cut_pt != modify_action.npos )
    {
      modify_action_options = modify_action.substr( cut_pt + 1 );
      modify_action         = modify_action.substr( 0, cut_pt );
    }
  }

  std::vector<std::string> skip_actions;
  if ( ! action_list_skip.empty() )
  {
    if ( sim -> debug )
      sim -> output( "Player %s: action_list_skip=%s", name(), action_list_skip.c_str() );

    util::string_split( skip_actions, action_list_skip, "/" );
  }

  if ( ! action_list_str.empty() ) get_action_priority_list( "default" ) -> action_list_str = action_list_str;

  int j = 0;

  for ( unsigned int alist = 0; alist < action_priority_list.size(); alist++ )
  {
    if ( sim -> debug )
      sim -> output( "Player %s: actions.%s=%s", name(),
                     action_priority_list[ alist ] -> name_str.c_str(),
                     action_priority_list[ alist ] -> action_list_str.c_str() );

    std::vector<std::string> splits;
    size_t num_splits = util::string_split( splits, action_priority_list[ alist ] -> action_list_str, "/" );

    for ( size_t i = 0; i < num_splits; i++ )
    {
      std::string action_name    = splits[ i ];
      std::string action_options = "";

      std::string::size_type cut_pt = action_name.find( "," );

      if ( cut_pt != action_name.npos )
      {
        action_options = action_name.substr( cut_pt + 1 );
        action_name    = action_name.substr( 0, cut_pt );
      }

      action_t* a=0;

      cut_pt = action_name.find( ":" );
      if ( cut_pt != action_name.npos )
      {
        std::string pet_name   = action_name.substr( 0, cut_pt );
        std::string pet_action = action_name.substr( cut_pt + 1 );

        pet_t* pet = find_pet( pet_name );
        if ( pet )
        {
          a =  new execute_pet_action_t( this, pet, pet_action, action_options );
        }
        else
        {
          sim -> errorf( "Player %s refers to unknown pet %s in action: %s\n",
                         name(), pet_name.c_str(), splits[ i ].c_str() );
        }
      }
      else
      {
        if ( action_name == modify_action )
        {
          if ( sim -> debug )
            sim -> output( "Player %s: modify_action=%s", name(), modify_action.c_str() );

          action_options = modify_action_options;
          splits[ i ] = modify_action + "," + modify_action_options;
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

          a -> signature_str = splits[ i ];

          if (  sim -> separate_stats_by_actions > 0 && !is_pet() )
          {
            a -> stats = get_stats( a -> name_str + "__" + a -> marker, a );

            if ( a -> dtr_action )
              a -> dtr_action -> stats = get_stats( a -> name_str + "__" + a -> marker + "_DTR", a );
          }
          j++;
        }
      }
      else
      {
        sim -> errorf( "Player %s unable to create action: %s\n", name(), splits[ i ].c_str() );
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
  report_information.action_sequence.reserve( capacity );
  report_information.action_sequence.clear();
}

void player_t::activate_action_list( action_priority_list_t* a, bool off_gcd )
{
  if ( off_gcd )
    active_off_gcd_list = a;
  else
    active_action_list = a;
  a -> used = true;
}

// player_t::init_rating ====================================================

void player_t::init_rating()
{
  if ( sim -> debug )
    sim -> output( "player_t::init_rating(): level=%d type=%s",
                   level, util::player_type_string( type ) );

  rating.init( sim, dbc, level, type );
}

struct override_talent_t : action_t
{
  override_talent_t( player_t* player ) : action_t( ACTION_OTHER, "override_talent", player )
  {
    background = true;
  }

  virtual bool is_valid_target( player_t* t )
  {
    // This skips the sleeping check, which would have made all targets ineligible at this point
    return ( ( type == ACTION_HEAL && ! t -> is_enemy() ) || ( type != ACTION_HEAL && t -> is_enemy() ) );
  }
};

void player_t::override_talent( std::string override_str )
{
  std::string::size_type cut_pt = override_str.find( "," );

  if ( cut_pt != override_str.npos && override_str.substr( cut_pt + 1, 3 ) == "if=" )
  {
    override_talent_t* dummy_action = new override_talent_t( this );
    expr_t* expr = expr_t::parse( dummy_action, override_str.substr( cut_pt + 4 ) );
    if ( ! expr || ! expr -> success() ) return;
    override_str = override_str.substr( 0, cut_pt );
  }

  unsigned spell_id = dbc.talent_ability_id( type, override_str.c_str() );

  if ( ! spell_id || ! dbc.spell( spell_id ) )
  {
    sim -> errorf( "Override talent %s not found for player %s.\n", override_str.c_str(), name() );
    return;
  }

  for ( unsigned int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( unsigned int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_data_t* td = talent_data_t::find( type, j, i, dbc.ptr );
      if ( td && ( td -> spell_id() == spell_id ) )
      {
        if ( level < ( int )( ( j + 1 ) * 15 ) )
        {
          sim -> errorf( "Override talent %s is too high level for player %s.\n", override_str.c_str(), name() );
          return;
        }
        for ( unsigned int k = 0; k < MAX_TALENT_COLS; k++ )
        {
          if ( k == i )
          {
            if ( sim -> debug && talent_list[ j * MAX_TALENT_COLS + k ] )
              sim -> output( "talent_override - %s for player %s: no action taken, talent was already enabled\n", override_str.c_str(), name() );
            talent_list[ j * MAX_TALENT_COLS + k ] = 1;
          }
          else
          {
            if ( sim -> debug && talent_list[ j * MAX_TALENT_COLS + k ] )
              sim -> output( "talent_override - %s for player %s: talent %d in tier %d replaced\n", override_str.c_str(), name(), k + 1, j + 1 );
            talent_list[ j * MAX_TALENT_COLS + k ] = 0;
          }
        }
      }
    }
  }

}

// player_t::init_talents ====================================================

void player_t::init_talents()
{
  if ( sim -> debug )
    sim -> output( "Initializing talents for player (%s)", name() );

  if ( talents_str.empty() )
  {
    parse_talents_numbers( set_default_talents() );

    if ( glyphs_str.empty() )
      glyphs_str = set_default_glyphs();
  }

  if ( ! talent_overrides_str.empty() )
  {
    std::vector<std::string> splits;
    size_t num_splits = util::string_split( splits, talent_overrides_str, "/" );
    for ( size_t i = 0; i < num_splits; i++ )
    {
      override_talent( splits[ i ] );
    }
  }
}


// player_t::init_glyphs ====================================================

void player_t::init_glyphs()
{
  if ( sim -> debug )
    sim -> output( "Initializing glyphs for player (%s)", name() );

  std::vector<std::string> glyph_names;
  util::string_split( glyph_names, glyphs_str, ",/" );

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
    sim -> output( "Initializing spells for player (%s)", name() );
}

// player_t::init_buffs =====================================================

void player_t::init_buffs()
{
  if ( sim -> debug )
    sim -> output( "Initializing buffs for player (%s)", name() );

  if ( ! is_enemy() )
  {
    buffs.berserking                = haste_buff_creator_t( this, "berserking", find_spell( 26297 ) );
    buffs.body_and_soul             = buff_creator_t( this, "body_and_soul" )
                                      .max_stack( 1 )
                                      .duration( timespan_t::from_seconds( 4.0 ) );
    buffs.grace                     = buff_creator_t( this,  "grace" )
                                      .max_stack( 3 )
                                      .duration( timespan_t::from_seconds( 15.0 ) );
    buffs.heroic_presence           = buff_creator_t( this, "heroic_presence" ).max_stack( 1 );
    buffs.hymn_of_hope              = new hymn_of_hope_buff_t( this, "hymn_of_hope", find_spell( 64904 ) );
    buffs.stoneform                 = buff_creator_t( this, "stoneform", find_spell( 65116 ) );

    buffs.blood_fury = stat_buff_creator_t( this, "blood_fury" ).
                       spell( find_racial_spell( "Blood Fury" ) );

    buffs.stormlash = new stormlash_buff_t( this, find_spell( 120687 ) );

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

    buffs.vengeance = buff_creator_t( this, "vengeance" )
                      .max_stack( 1 )
                      .duration( timespan_t::from_seconds( 20.0 ) )
                      .default_value( 0 );

    // Potions
    struct potions_common_buff_creator : public stat_buff_creator_t
    {
      potions_common_buff_creator( player_t* p,
                                   const std::string& n,
                                   timespan_t d = timespan_t::from_seconds( 25.0 ),
                                   timespan_t cd = timespan_t::from_seconds( 60.0 ) ) :
        stat_buff_creator_t ( p,  n + "_potion" )
      {
        max_stack( 1 );
        duration( d );
        this -> cd( cd );
        // Kludge of the century, version 2
        chance( p -> sim -> allow_potions );
      }
    };

    potion_buffs.speed      = potions_common_buff_creator( this, "speed", timespan_t::from_seconds( 15.0 ) )
                              .add_stat( STAT_HASTE_RATING, 500.0 );

    potion_buffs.volcanic   = potions_common_buff_creator( this, "volcanic" )
                              .add_stat( STAT_INTELLECT, 1200.0 );

    potion_buffs.earthen    = potions_common_buff_creator( this, "earthen" )
                              .add_stat( STAT_ARMOR, 4800.0 );

    potion_buffs.golemblood = potions_common_buff_creator( this, "golemblood" )
                              .add_stat( STAT_STRENGTH, 1200.0 );

    potion_buffs.tolvir     = potions_common_buff_creator( this, "tolvir" )
                              .add_stat( STAT_AGILITY, 1200.0 );

    // New Mop potions

    potion_buffs.jade_serpent = potions_common_buff_creator( this, "jade_serpent" )
                                .add_stat( STAT_INTELLECT, 4000.0 );

    potion_buffs.mountains    = potions_common_buff_creator( this, "mountains" )
                                .add_stat( STAT_ARMOR, 12000.0 );

    potion_buffs.mogu_power   = potions_common_buff_creator( this, "mogu_power" )
                                .add_stat( STAT_STRENGTH, 4000.0 );

    potion_buffs.virmens_bite = potions_common_buff_creator( this, "virmens_bite" )
                                .add_stat( STAT_AGILITY, 4000.0 );


    buffs.mongoose_mh = NULL;
    buffs.mongoose_oh = NULL;
  }

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

// player_t::init_gains =====================================================

void player_t::init_gains()
{
  if ( sim -> debug )
    sim -> output( "Initializing gains for player (%s)", name() );

  gains.arcane_torrent         = get_gain( "arcane_torrent" );
  gains.blessing_of_might      = get_gain( "blessing_of_might" );
  gains.chi_regen              = get_gain( "chi_regen" );
  gains.dark_rune              = get_gain( "dark_rune" );
  gains.energy_regen           = get_gain( "energy_regen" );
  gains.essence_of_the_red     = get_gain( "essence_of_the_red" );
  gains.focus_regen            = get_gain( "focus_regen" );
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
    sim -> output( "Initializing procs for player (%s)", name() );

  procs.hat_donor = get_proc( "hat_donor" );
}

// player_t::init_uptimes ===================================================

void player_t::init_uptimes()
{
  if ( sim -> debug )
    sim -> output( "Initializing uptimes for player (%s)", name() );

  uptimes.primary_resource_cap = get_uptime( util::inverse_tokenize( util::resource_type_string( primary_resource() ) ) +  " Cap" );
}

// player_t::init_benefits ===================================================

void player_t::init_benefits()
{
  if ( sim -> debug )
    sim -> output( "Initializing benefits for player (%s)", name() );
}

// player_t::init_rng =======================================================

void player_t::init_rng()
{
  if ( sim -> debug )
    sim -> output( "Initializing rngs for player (%s)", name() );

  rngs.lag_channel  = get_rng( "lag_channel"  );
  rngs.lag_ability  = get_rng( "lag_ability"  );
  rngs.lag_brain    = get_rng( "lag_brain"    );
  rngs.lag_gcd      = get_rng( "lag_gcd"      );
  rngs.lag_queue    = get_rng( "lag_queue"    );
  rngs.lag_reaction = get_rng( "lag_reaction" );
  rngs.lag_world    = get_rng( "lag_world"    );
}

// player_t::init_stats =====================================================

void player_t::init_stats()
{
  if ( sim -> debug )
    sim -> output( "Initializing stats for player (%s)", name() );


  range::fill( resource_lost, 0 );
  range::fill( resource_gained, 0 );


  for ( size_t i = 0; i < resources.combat_end_resource.size(); ++i )
    resources.combat_end_resource[ i ].reserve( sim -> iterations );

  fight_length.reserve( sim -> iterations );
  waiting_time.reserve( sim -> iterations );
  executed_foreground_actions.reserve( sim -> iterations );

  dmg.reserve( sim -> iterations );
  compound_dmg.reserve( sim -> iterations );
  dps.reserve( sim -> iterations );
  dpse.reserve( sim -> iterations );
  dtps.reserve( sim -> iterations );

  heal.reserve( sim -> iterations );
  compound_heal.reserve( sim -> iterations );
  hps.reserve( sim -> iterations );
  hpse.reserve( sim -> iterations );
  htps.reserve( sim -> iterations );
}

// player_t::init_values ====================================================

void player_t::init_values()
{
  if ( sim -> debug )
    sim -> output( "Initializing values for player (%s)", name() );
}

// player_t::init_scaling ===================================================

void player_t::init_scaling()
{
  if ( sim -> debug )
    sim -> output( "Initializing scaling for player (%s)", name() );

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
    scales_with[ STAT_MP5               ] = false;

    scales_with[ STAT_ATTACK_POWER             ] = attack;
    scales_with[ STAT_EXPERTISE_RATING         ] = attack;
    scales_with[ STAT_EXPERTISE_RATING2        ] = attack && ( position() == POSITION_FRONT );

    scales_with[ STAT_HIT_RATING                ] = true;
    scales_with[ STAT_CRIT_RATING               ] = true;
    scales_with[ STAT_HASTE_RATING              ] = true;
    scales_with[ STAT_MASTERY_RATING            ] = true;

    scales_with[ STAT_WEAPON_DPS   ] = attack;
    scales_with[ STAT_WEAPON_SPEED ] = sim -> weapon_speed_scale_factors ? attack : false;

    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = false;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = false;

    scales_with[ STAT_ARMOR          ] = tank;
    scales_with[ STAT_BONUS_ARMOR    ] = false;
    scales_with[ STAT_DODGE_RATING   ] = tank;
    scales_with[ STAT_PARRY_RATING   ] = tank;

    scales_with[ STAT_BLOCK_RATING ] = tank;

    if ( sim -> scaling -> scale_stat != STAT_NONE && scale_player )
    {
      double v = sim -> scaling -> scale_value;

      switch ( sim -> scaling -> scale_stat )
      {
      case STAT_STRENGTH:  initial.attribute[ ATTR_STRENGTH  ] += v; break;
      case STAT_AGILITY:   initial.attribute[ ATTR_AGILITY   ] += v; break;
      case STAT_STAMINA:   initial.attribute[ ATTR_STAMINA   ] += v; break;
      case STAT_INTELLECT: initial.attribute[ ATTR_INTELLECT ] += v; break;
      case STAT_SPIRIT:    initial.attribute[ ATTR_SPIRIT    ] += v; break;

      case STAT_SPELL_POWER:       initial.spell_power[ SCHOOL_MAX ] += v; break;
      case STAT_MP5:               initial.mp5                       += v; break;

      case STAT_ATTACK_POWER:      initial.attack_power              += v; break;

      case STAT_EXPERTISE_RATING:
      case STAT_EXPERTISE_RATING2:
        initial.attack_expertise   += v / rating.expertise;
        break;

      case STAT_HIT_RATING:
      case STAT_HIT_RATING2:
        initial.attack_hit += v / rating.attack_hit;
        initial.spell_hit  += v / rating.spell_hit;
        break;

      case STAT_CRIT_RATING:
        initial.attack_crit += v / rating.attack_crit;
        initial.spell_crit  += v / rating.spell_crit;
        break;

      case STAT_HASTE_RATING: initial.haste_rating += v; break;
      case STAT_MASTERY_RATING: initial.mastery_rating += v; break;

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

      case STAT_ARMOR:          initial.armor       += v; break;
      case STAT_BONUS_ARMOR:    initial.bonus_armor += v; break;
      case STAT_DODGE_RATING:   initial.dodge       += v / rating.dodge; break;
      case STAT_PARRY_RATING:   initial.parry       += v / rating.parry; break;

      case STAT_BLOCK_RATING:   initial.block       += v / rating.block; break;

      case STAT_MAX: break;

      default: assert( 0 ); break;
      }
    }
  }
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
    r = base_energy_regen_per_second * ( 1.0 / composite_attack_haste() );
  return r;
}

// player_t::focus_regen_per_second =========================================

double player_t::focus_regen_per_second()
{
  double r = 0;
  if ( base_focus_regen_per_second )
    r = base_focus_regen_per_second * ( 1.0 / composite_attack_haste() );
  return r;
}

// player_t::chi_regen_per_second ========================================

double player_t::chi_regen_per_second()
{
  // FIXME: Just assuming it scale with haste right now.
  double r = 0;
  if ( base_chi_regen_per_second )
    r = base_chi_regen_per_second * ( 1.0 / composite_attack_haste() );
  return r;
}

// player_t::composite_attack_haste =========================================

double player_t::composite_attack_haste()
{
  double h = attack_haste;

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

    if ( buffs.mongoose_mh && buffs.mongoose_mh -> up() ) h *= 1.0 / ( 1.0 + 30 / rating.attack_haste );
    if ( buffs.mongoose_oh && buffs.mongoose_oh -> up() ) h *= 1.0 / ( 1.0 + 30 / rating.attack_haste );

    if ( buffs.berserking -> up() )
    {
      h *= 1.0 / ( 1.0 + buffs.berserking -> data().effectN( 1 ).percent() );
    }
  }

  return h;
}

// player_t::composite_attack_speed =========================================

double player_t::composite_attack_speed()
{
  double h = composite_attack_haste();

  if ( race == RACE_GOBLIN )
  {
    h *= 1.0 / ( 1.0 + 0.01 );
  }

  if ( ! is_enemy() && ! is_add() && sim -> auras.attack_haste -> check() )
    h *= 1.0 / ( 1.0 + sim -> auras.attack_haste -> value() );

  return h;
}

// player_t::composite_attack_power =========================================

double player_t::composite_attack_power()
{
  double ap = current.attack_power;

  ap += current.attack_power_per_strength * ( strength() - 10 );
  ap += current.attack_power_per_agility  * ( agility() - 10 );

  if ( vengeance )
    ap += buffs.vengeance->value();

  return ap;
}

// player_t::composite_attack_crit ==========================================

double player_t::composite_attack_crit( weapon_t* /* weapon */ )
{
  double ac = current.attack_crit + ( agility() / current.attack_crit_per_agility / 100.0 );

  if ( ! is_pet() && ! is_enemy() && ! is_add() && sim -> auras.critical_strike -> check() )
    ac += sim -> auras.critical_strike -> value();

  if ( race == RACE_WORGEN )
    ac += 0.01;

  return ac;
}

// player_t::composite_attack_expertise =====================================
double player_t::composite_attack_expertise( weapon_t* weapon )
{
  double m = current.attack_expertise;

  if ( ! weapon )
    return m;

  switch ( race )
  {
  case RACE_ORC:
  {
    switch ( weapon -> type )
    {
    case WEAPON_AXE:
    case WEAPON_AXE_2H:
    case WEAPON_FIST:
    case WEAPON_WAND:
    case WEAPON_NONE:
      m += 0.01;
      break;
    default:
      break;
    }
    break;
  }
  case RACE_TROLL:
  {
    if ( WEAPON_2H < weapon -> type && weapon -> type <= WEAPON_RANGED )
      m += 0.01;
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
      m += 0.01;
      break;
    default:
      break;
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
      m += 0.01;
      break;
    default:
      break;
    }
    break;
  }
  case RACE_GNOME:
  {
    switch ( weapon -> type )
    {
    case WEAPON_DAGGER:
    case WEAPON_SWORD:
      m += 0.01;
      break;
    default:
      break;
    }
    break;
  }
  default:
    break;
  }

  return m;
}

// player_t::composite_attack_hit ===========================================

double player_t::composite_attack_hit()
{
  double ah = current.attack_hit;

  if ( buffs.heroic_presence && buffs.heroic_presence -> up() )
    ah += 0.01;

  return ah;
}

// player_t::composite_armor ================================================

double player_t::composite_armor()
{
  double a = current.armor;

  a *= composite_armor_multiplier();

  a += current.bonus_armor;

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
    a+=0.02;
  }

  return a;
}

// player_t::composite_spell_resistance =====================================

double player_t::composite_spell_resistance( school_e school )
{
  double a = spell_resistance[ school ];

  return a;
}

// player_t::composite_tank_miss ============================================

double player_t::composite_tank_miss( school_e school )
{
  double m = 0;

  if ( school == SCHOOL_PHYSICAL && race == RACE_NIGHT_ELF ) // Quickness
  {
    m += 0.02;
  }

  if      ( m > 1.0 ) m = 1.0;
  else if ( m < 0.0 ) m = 0.0;

  return m;
}

// player_t::composite_tank_block ===========================================

double player_t::composite_tank_block()
{
  double block_by_rating = current.block - base.block;

  double b = base.block;

  if ( block_by_rating>0 )
  {
    //the block by rating gets rounded because that's how blizzard rolls...
    b+= 1/ ( 1 / diminished_block_cap + diminished_kfactor / ( util::round( 12800*block_by_rating )/12800 ) );
  }

  return b;
}

// player_t::composite_tank_dodge ===========================================

double player_t::composite_tank_dodge()
{
  double dodge_by_dodge_rating = current.dodge - base.dodge;
  double dodge_by_agility = ( agility() - base.attribute[ ATTR_AGILITY ] ) * current.dodge_per_agility;

  double d= base.dodge;

  d += base.attribute[ATTR_AGILITY] *current.dodge_per_agility;

  if ( dodge_by_agility > 0 || dodge_by_dodge_rating > 0 )
  {
    d += 1/ ( 1 / diminished_dodge_cap + diminished_kfactor / ( dodge_by_dodge_rating + dodge_by_agility ) );
  }

  return d;
}

// player_t::composite_tank_parry ===========================================

double player_t::composite_tank_parry()
{

  //changed it to match the typical formulation

  double parry_by_parry_rating = current.parry - base.parry;
  double parry_by_strength = ( strength() - base.attribute[ ATTR_STRENGTH ] ) * current.parry_rating_per_strength / rating.parry;
  // these are pre-DR values

  double p = base.parry;

  if ( current.parry_rating_per_strength>0 )
  {
    p += base.attribute[ATTR_STRENGTH]/current.parry_rating_per_strength/rating.parry;
  }

  if ( parry_by_strength >0 || parry_by_parry_rating > 0 )
  {
    p += 1 / ( 1 / diminished_parry_cap + diminished_kfactor / ( parry_by_parry_rating + parry_by_strength ) );
  }
  return p; //this is the post-DR parry value
}



// player_t::composite_tank_block_reduction =================================

double player_t::composite_tank_block_reduction()
{
  double b = current.block_reduction;

  if ( meta_gem == META_ETERNAL_SHADOWSPIRIT  ||
       meta_gem == META_ETERNAL_PRIMAL )
  {
    b += 0.01;
  }

  return b;
}

// player_t::composite_tank_crit_block ======================================

double player_t::composite_tank_crit_block()
{
  return 0;
}

// player_t::composite_tank_crit ============================================

double player_t::composite_tank_crit( school_e /* school */ )
{
  return 0;
}

// player_t::composite_spell_haste ==========================================

double player_t::composite_spell_haste()
{
  double h = spell_haste;

  if ( ! is_pet() && ! is_enemy() )
  {
    if ( buffs.bloodlust -> up() )
    {
      h *= 1.0 / ( 1.0 + buffs.bloodlust -> data().effectN( 1 ).percent() );
    }
    else if ( buffs.power_infusion -> up() )
    {
      h *= 1.0 / ( 1.0 + buffs.power_infusion -> data().effectN( 1 ).percent() );
    }

    if ( buffs.berserking -> up() )
      h *= 1.0 / ( 1.0 + buffs.berserking -> data().effectN( 1 ).percent() );

    if ( sim -> auras.spell_haste -> check() )
    {
      h *= 1.0 / ( 1.0 + sim -> auras.spell_haste -> value() );
    }

    if ( race == RACE_GOBLIN )
    {
      h *= 1.0 / ( 1.0 + 0.01 );
    }
  }

  return h;
}

// player_t::composite_spell_power ==========================================

double player_t::composite_spell_power( school_e school )
{
  double sp = current.spell_power[ school ];

  switch ( school )
  {
  case SCHOOL_FROSTFIRE:
    sp = std::max( current.spell_power[ SCHOOL_FROST ],
                   current.spell_power[ SCHOOL_FIRE  ] );
    break;
  case SCHOOL_SPELLSTORM:
    sp = std::max( current.spell_power[ SCHOOL_NATURE ],
                   current.spell_power[ SCHOOL_ARCANE ] );
    break;
  case SCHOOL_SHADOWFROST:
    sp = std::max( current.spell_power[ SCHOOL_SHADOW ],
                   current.spell_power[ SCHOOL_FROST ] );
    break;
  case SCHOOL_SHADOWFLAME:
    sp = std::max( current.spell_power[ SCHOOL_SHADOW ],
                   current.spell_power[ SCHOOL_FIRE ] );
    break;
  default: break;
  }

  if ( school != SCHOOL_MAX )
    sp += current.spell_power[ SCHOOL_MAX ];


  sp += current.spell_power_per_intellect * ( intellect() - 10 ); // The spellpower is always lower by 10, cata beta build 12803

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
  double sc = current.spell_crit + ( intellect() / current.spell_crit_per_intellect / 100.0 );

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
  double sh = current.spell_hit;

  if ( buffs.heroic_presence && buffs.heroic_presence -> up() )
    sh += 0.01;

  sh += composite_attack_expertise( &main_hand_weapon );

  return sh;
}

// player_t::composite_mp5 ==================================================

double player_t::composite_mp5()
{
  return current.mp5 + spirit() * current.mp5_per_spirit * 5.0 * current.mp5_from_spirit_multiplier;
}

// player_t::composite_mastery ==================================================

double player_t::composite_mastery()
{
  double m = floor( ( current.mastery * 100.0 ) + 0.5 ) / 100.0;

  if ( ! is_pet() && ! is_enemy() && sim -> auras.mastery -> check() )
    m += sim -> auras.mastery -> value() / rating.mastery;

  return m;
}

// player_t::composite_attack_power_multiplier ==============================

double player_t::composite_attack_power_multiplier()
{
  double m = current.attack_power_multiplier;

  if ( ! is_pet() && ! is_enemy() && sim -> auras.attack_power_multiplier -> check() )
    m *= 1.0 + sim -> auras.attack_power_multiplier -> value();

  return m;
}

// player_t::composite_player_multiplier ====================================

double player_t::composite_player_multiplier( school_e school, action_t* /* a */ )
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

// player_t::composite_attribute =================================

double player_t::composite_attribute( attribute_e attr )
{
  double a = current.attribute[ attr ];
  double m = ( ( level >= 50 ) && matching_gear ) ? ( 1.0 + matching_gear_multiplier( attr ) ) : 1.0;

  switch ( attr )
  {
  case ATTR_SPIRIT:
    if ( race == RACE_HUMAN )
      a += ( a - base.attribute[ ATTR_SPIRIT ] ) * 0.03;
    break;
  default:
    break;
  }

  a = util::floor( ( a - base.attribute[ attr ] ) * m ) + base.attribute[ attr ];

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
  default:
    break;
  }

  return m;
}

// player_t::get_attribute() ================================================

double player_t::get_attribute( attribute_e a )
{
  return util::round( composite_attribute( a ) * composite_attribute_multiplier( a ) );
}

// player_t::composite_vulnerability ========================================

double player_t::composite_spell_crit_vulnerability()
{
  return 0.0;
}

// player_t::composite_attack_crit_vulnerability ========================================

double player_t::composite_attack_crit_vulnerability()
{
  return 0.0;
}

// player_t::composite_player_vulnerability ========================================

double player_t::composite_player_vulnerability( school_e school )
{
  double m = 1.0;

  if ( debuffs.magic_vulnerability -> check() &&
       school != SCHOOL_NONE && school != SCHOOL_PHYSICAL && school != SCHOOL_BLEED )
    m *= 1.0 + debuffs.magic_vulnerability -> value();
  else if ( debuffs.physical_vulnerability -> check() &&
            ( school == SCHOOL_PHYSICAL || school == SCHOOL_BLEED ) )
    m *= 1.0 + debuffs.physical_vulnerability -> value();

  if ( debuffs.vulnerable -> check() )
    m *= 1.0 + debuffs.vulnerable -> value();

  return m;
}

// player_t::composite_ranged_attack_player_vulnerability ========================================

double player_t::composite_ranged_attack_player_vulnerability()
{
  // MoP: Increase ranged damage taken by 5%. make sure
  if ( debuffs.ranged_vulnerability -> check() )
    return 1.0 + debuffs.ranged_vulnerability -> value();

  return 1.0;
}

// player_t::combat_begin ===================================================

void player_t::combat_begin()
{
  if ( sim -> debug ) sim -> output( "Combat begins for player %s", name() );

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
          sim -> output( "Combat ends for player %s at time %.4f fight_length=%.4f", name(), sim -> current_time.total_seconds(), iteration_fight_length.total_seconds() );
}

// startpoint for statistical data collection

void player_t::datacollection_begin()
{
  if ( sim -> debug )
    sim -> output( "Data collection begins for player %s", name() );

  iteration_fight_length = timespan_t::zero();
  iteration_waiting_time = timespan_t::zero();
  iteration_executed_foreground_actions = 0;
  iteration_dmg = 0;
  iteration_heal = 0;
  iteration_dmg_taken = 0;
  iteration_heal_taken = 0;

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < stats_list.size(); ++i )
    stats_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < proc_list.size(); ++i )
    proc_list[ i ] -> datacollection_begin();
}

// endpoint for statistical data collection

void player_t::datacollection_end()
{
  for ( size_t i = 0; i < pet_list.size(); ++i )
    pet_list[ i ] -> datacollection_end();

  double f_length = iteration_fight_length.total_seconds();
  double w_time = iteration_waiting_time.total_seconds();

  fight_length.add( f_length );
  waiting_time.add( w_time );

  executed_foreground_actions.add( iteration_executed_foreground_actions );

  for ( size_t i = 0; i < stats_list.size(); ++i )
    stats_list[ i ] -> datacollection_end();

  // DMG
  dmg.add( iteration_dmg );
  if ( ! is_enemy() && ! is_add() )
    sim -> iteration_dmg += iteration_dmg;
  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    iteration_dmg += pet_list[ i ] -> iteration_dmg;
  }
  compound_dmg.add( iteration_dmg );

  dps.add( iteration_fight_length != timespan_t::zero() ? iteration_dmg / iteration_fight_length.total_seconds() : 0 );
  dpse.add( sim -> current_time != timespan_t::zero() ? iteration_dmg / sim -> current_time.total_seconds() : 0 );

  if ( sim -> debug )
    sim -> output( "Data collection ends for player %s at time %.4f fight_length=%.4f", name(), sim -> current_time.total_seconds(), iteration_fight_length.total_seconds() );

  // Heal
  heal.add( iteration_heal );
  if ( ! is_enemy() && ! is_add() )
    sim -> iteration_heal += iteration_heal;
  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    iteration_heal += pet_list[ i ] -> iteration_heal;
  }
  compound_heal.add( iteration_heal );

  hps.add( iteration_fight_length != timespan_t::zero() ? iteration_heal / iteration_fight_length.total_seconds() : 0 );
  hpse.add( sim -> current_time != timespan_t::zero() ? iteration_heal / sim -> current_time.total_seconds() : 0 );

  dmg_taken.add( iteration_dmg_taken );
  dtps.add( iteration_fight_length != timespan_t::zero() ? iteration_dmg_taken / iteration_fight_length.total_seconds() : 0 );

  heal_taken.add( iteration_heal_taken );
  htps.add( iteration_fight_length != timespan_t::zero() ? iteration_heal_taken / iteration_fight_length.total_seconds() : 0 );

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> datacollection_end();

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> datacollection_end( iteration_fight_length );

  for ( size_t i = 0; i < proc_list.size(); ++i )
    proc_list[ i ] -> datacollection_end();

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
    resources.combat_end_resource[ i ].add( resources.current[ i ] );

}

// player_t::merge ==========================================================

void player_t::merge( player_t& other )
{
  fight_length.merge( other.fight_length );
  waiting_time.merge( other.waiting_time );
  executed_foreground_actions.merge( other.executed_foreground_actions );

  dmg.merge( other.dmg );
  compound_dmg.merge( other.compound_dmg );
  dps.merge( other.dps );
  dtps.merge( other.dtps );
  dpse.merge( other.dpse );
  dmg_taken.merge( other.dmg_taken );

  heal.merge( other.heal );
  compound_heal.merge( other.compound_heal );
  hps.merge( other.hps );
  htps.merge( other.htps );
  hpse.merge( other.hpse );
  heal_taken.merge( other.heal_taken );

  deaths.merge( other.deaths );

  assert( resource_timeline_count == other.resource_timeline_count );
  for ( size_t i = 0; i < resource_timeline_count; ++i )
  {
    assert( resource_timelines[ i ].type == other.resource_timelines[ i ].type );
    assert( resource_timelines[ i ].type != RESOURCE_NONE );

    std::vector<double>& mine = resource_timelines[ i ].timeline;
    const std::vector<double>& theirs = other.resource_timelines[ i ].timeline;

    if ( mine.size() < theirs.size() )
      mine.resize( theirs.size() );

    for ( size_t j = 0, num_buckets = std::min( mine.size(), theirs.size() ); j < num_buckets; ++j )
      mine[ j ] += theirs[ j ];
  }

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    resource_lost  [ i ] += other.resource_lost  [ i ];
    resource_gained[ i ] += other.resource_gained[ i ];
  }

  // Buffs
  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t& b = *buff_list[ i ];
    buff_t* otherbuff = NULL;

    std::string initial_source_name; // If the buff has a initial_source != player, save it's name and check against it
    if ( b.initial_source  && b.initial_source != b.player )
      initial_source_name = b.initial_source -> name_str;

    for ( size_t i = 0; i < other.buff_list.size(); i++ )
    {
      buff_t& other_b = *other.buff_list[ i ];

      if ( b.name_str == other_b.name_str )
        if ( initial_source_name.empty() || ( other_b.initial_source && initial_source_name == other_b.initial_source -> name_str ) )
          otherbuff = &other_b;
    }

    if ( otherbuff )
      b.merge( *otherbuff );
    else
    {
#ifndef NDEBUG
      sim -> errorf( "%s player_t::merge can't merge buff %s. initial_source= %s player= %s",
                     name(), b.name(), b.initial_source ? b.initial_source -> name() : "", b.player ? b.player -> name() : "" );
#endif
    }
  }

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

  // Action Map
  for ( size_t i = 0; i < other.action_list.size(); ++i )
    action_list[ i ] -> total_executions += other.action_list[ i ] -> total_executions;
}

// player_t::reset ==========================================================

void player_t::reset()
{
  if ( sim -> debug ) sim -> output( "Resetting player %s", name() );

  last_cast = timespan_t::zero();
  gcd_ready = timespan_t::zero();

  events = 0;

  stats = initial_stats;

  change_position( initial.position );
  // Reset current stats to initial stats
  current = initial;

  current.sleeping = true;
  current.mastery = initial.mastery + initial.mastery_rating / rating.mastery;
  recalculate_haste();

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

  elixir_battle   = ELIXIR_NONE;
  elixir_guardian = ELIXIR_NONE;
  flask           = FLASK_NONE;
  food            = FOOD_NONE;

  callbacks.reset();

  init_resources( true );

  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ] -> reset();

  for ( size_t i = 0; i < cooldown_list.size(); ++i )
    cooldown_list[ i ] -> reset();

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

  item_cooldown -> reset();
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

  if ( sim -> debug ) sim -> output( "%s is triggering ready, interval=%f", name(), ( sim -> current_time - started_waiting ).total_seconds() );

  assert( started_waiting != timespan_t::zero() );

  iteration_waiting_time += sim -> current_time - started_waiting;

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

  started_waiting = timespan_t::zero();

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
        timespan_t ability_lag = rngs.lag_ability -> gauss( last_foreground_action -> ability_lag, last_foreground_action -> ability_lag_stddev );
        timespan_t gcd_lag     = rngs.lag_gcd   -> gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
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
        lag = rngs.lag_channel -> gauss( sim -> channel_lag, sim -> channel_lag_stddev );
      }
      else
      {
        timespan_t   gcd_lag = rngs.lag_gcd   -> gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        timespan_t queue_lag = rngs.lag_queue -> gauss( sim -> queue_lag, sim -> queue_lag_stddev );

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
    last_foreground_action -> stats -> total_execute_time += delta_time;
  }

  readying = new ( sim ) player_ready_event_t( sim, this, delta_time );

  if ( was_executing && was_executing -> gcd() > timespan_t::zero() && ! was_executing -> background && ! was_executing -> proc && ! was_executing -> repeating )
  {
    // Record the last ability use time for cast_react
    cast_delay_occurred = readying -> occurs();
    cast_delay_reaction = rngs.lag_brain -> gauss( brain_lag, brain_lag_stddev );
    if ( sim -> debug )
    {
      sim -> output( "%s %s schedule_ready(): cast_finishes=%f cast_delay=%f",
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
    sim -> output( "%s arises.", name() );

  if ( ! initial.sleeping )
    current.sleeping = false;

  if ( current.sleeping )
    return;

  init_resources( true );

  readying = 0;
  off_gcd = 0;

  arise_time = sim -> current_time;

  if ( unlikely( is_enemy() ) )
    sim -> active_enemies++;
  else
    sim -> active_allies++;

  if ( has_foreground_actions( this ) )
    schedule_ready();
}

// player_t::demise =========================================================

void player_t::demise()
{
  // No point in demising anything if we're not even active
  if ( current.sleeping )
    return;

  if ( sim -> log )
    sim -> output( "%s demises.", name() );

  assert( arise_time >= timespan_t::zero() );
  iteration_fight_length += sim -> current_time - arise_time;
  arise_time = timespan_t::min();

  current.sleeping = true;
  if ( readying )
  {
    event_t::cancel( readying );
    readying = 0;
  }

  if ( unlikely( is_enemy() ) )
    sim -> active_enemies--;
  else
    sim -> active_allies--;

  event_t::cancel( off_gcd );

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> expire();
    // Dead actors speak no lies .. or proc aura delayed buffs
    if ( b -> delay )
      event_t::cancel( b -> delay );
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

  if ( sim -> log ) sim -> output( "%s is interrupted", name() );

  if ( executing  ) executing  -> interrupt_action();
  if ( channeling ) channeling -> interrupt_action();

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
  if ( sim -> log ) sim -> output( "%s is halted", name() );

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
  // FIXME! At the moment we are just clearing DoTs

  if ( sim -> log ) sim -> output( "%s clears debuffs", name() );

  for ( size_t i = 0; i < dot_list.size(); ++i )
  {
    dot_t* dot = dot_list[ i ];
    dot -> cancel();
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
    action -> line_cooldown -> start();
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

  case RESOURCE_CHI:
    base = chi_regen_per_second();
    gain = gains.chi_regen;
    break;

  case RESOURCE_FOCUS:
    base = focus_regen_per_second();
    gain = gains.focus_regen;
    break;

  case RESOURCE_MANA:
    base = composite_mp5() / 5.0;
    gain = gains.mp5_regen;
    break;

  default:
    return;
  }

  if ( gain && base )
    resource_gain( r, base * periodicity.total_seconds(), gain );

}

// player_t::collect_resource_timeline_information ==================================================

void player_t::collect_resource_timeline_information()
{
  unsigned index = static_cast<unsigned>( sim -> current_time.total_seconds() );

  for ( size_t j = 0; j < resource_timeline_count; ++j )
  {
    resource_timelines[ j ].timeline[ index ] +=
      resources.current[ resource_timelines[ j ].type ];
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
    sim -> output( "Player %s loses %.2f (%.2f) %s. health pct: %.2f (%.0f/%.0f)",
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
    sim -> output( "%s gains %.2f (%.2f) %s from %s (%.2f/%.2f)",
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

// player_t::recalculate_resources.max =======================================

void player_t::recalculate_resource_max( resource_e resource_type )
{
  // The first 20pts of intellect/stamina only provide 1pt of mana/health.

  resources.max[ resource_type ] = resources.base[ resource_type ] * resources.base_multiplier[ resource_type ]+
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

role_e player_t::primary_role()
{
  return role;
}

// player_t::primary_tree_name ==============================================

const char* player_t::primary_tree_name()
{
  return util::specialization_string( specialization() ).c_str();
}

// player_t::normalize_by ===================================================

stat_e player_t::normalize_by()
{
  if ( sim -> normalized_stat != STAT_NONE )
    return sim -> normalized_stat;

  role_e role = primary_role();
  if ( role == ROLE_SPELL || role == ROLE_HEAL )
    return STAT_INTELLECT;
  else if ( role == ROLE_TANK )
    return STAT_ARMOR;
  else if ( type == DRUID || type == HUNTER || type == SHAMAN || type == ROGUE )
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
  return reaction_offset + rngs.lag_reaction -> exgauss( reaction_mean, reaction_stddev, reaction_nu );
}

// player_t::stat_gain ======================================================

void player_t::stat_gain( stat_e stat,
                          double    amount,
                          gain_t*   gain,
                          action_t* action,
                          bool      temporary_stat )
{
  if ( amount <= 0 ) return;

  if ( sim -> log ) sim -> output( "%s gains %.2f %s%s", name(), amount, util::stat_type_string( stat ), temporary_stat ? " (temporary)" : "" );

  int temp_value = temporary_stat ? 1 : 0;
  switch ( stat )
  {
  case STAT_STRENGTH:  stats.attribute[ ATTR_STRENGTH  ] += amount; current.attribute[ ATTR_STRENGTH  ] += amount; temporary.attribute[ ATTR_STRENGTH  ] += temp_value * amount; break;
  case STAT_AGILITY:   stats.attribute[ ATTR_AGILITY   ] += amount; current.attribute[ ATTR_AGILITY   ] += amount; temporary.attribute[ ATTR_AGILITY   ] += temp_value * amount; break;
  case STAT_STAMINA:   stats.attribute[ ATTR_STAMINA   ] += amount; current.attribute[ ATTR_STAMINA   ] += amount; temporary.attribute[ ATTR_STAMINA   ] += temp_value * amount; recalculate_resource_max( RESOURCE_HEALTH ); break;
  case STAT_INTELLECT: stats.attribute[ ATTR_INTELLECT ] += amount; current.attribute[ ATTR_INTELLECT ] += amount; temporary.attribute[ ATTR_INTELLECT ] += temp_value * amount; break;
  case STAT_SPIRIT:    stats.attribute[ ATTR_SPIRIT    ] += amount; current.attribute[ ATTR_SPIRIT    ] += amount; temporary.attribute[ ATTR_SPIRIT    ] += temp_value * amount; break;

  case STAT_ALL:
    for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
    {
      stats.attribute[ i ] += amount;
      temporary.attribute[ i ] += temp_value * amount;
      current.attribute[ i ] += amount;
    }
    break;

  case STAT_HEALTH: resource_gain( RESOURCE_HEALTH, amount, gain, action ); break;
  case STAT_MANA:   resource_gain( RESOURCE_MANA,   amount, gain, action ); break;
  case STAT_RAGE:   resource_gain( RESOURCE_RAGE,   amount, gain, action ); break;
  case STAT_ENERGY: resource_gain( RESOURCE_ENERGY, amount, gain, action ); break;
  case STAT_FOCUS:  resource_gain( RESOURCE_FOCUS,  amount, gain, action ); break;
  case STAT_RUNIC:  resource_gain( RESOURCE_RUNIC_POWER,  amount, gain, action ); break;

  case STAT_MAX_HEALTH: resources.max[ RESOURCE_HEALTH ] += amount; resource_gain( RESOURCE_HEALTH, amount, gain, action ); break;
  case STAT_MAX_MANA:   resources.max[ RESOURCE_MANA   ] += amount; resource_gain( RESOURCE_MANA,   amount, gain, action ); break;
  case STAT_MAX_RAGE:   resources.max[ RESOURCE_RAGE   ] += amount; resource_gain( RESOURCE_RAGE,   amount, gain, action ); break;
  case STAT_MAX_ENERGY: resources.max[ RESOURCE_ENERGY ] += amount; resource_gain( RESOURCE_ENERGY, amount, gain, action ); break;
  case STAT_MAX_FOCUS:  resources.max[ RESOURCE_FOCUS  ] += amount; resource_gain( RESOURCE_FOCUS,  amount, gain, action ); break;
  case STAT_MAX_RUNIC:  resources.max[ RESOURCE_RUNIC_POWER  ] += amount; resource_gain( RESOURCE_RUNIC_POWER,  amount, gain, action ); break;

  case STAT_SPELL_POWER:       stats.spell_power       += amount; temporary.spell_power += temp_value * amount; current.spell_power[ SCHOOL_MAX ] += amount; break;
  case STAT_MP5:               stats.mp5               += amount; current.mp5                       += amount; break;

  case STAT_ATTACK_POWER:             stats.attack_power             += amount; temporary.attack_power += temp_value * amount; current.attack_power       += amount;                            break;
  case STAT_EXPERTISE_RATING:         stats.expertise_rating         += amount; temporary.expertise_rating += temp_value * amount; current.attack_expertise   += amount / rating.expertise;         break;

  case STAT_HIT_RATING:
    stats.hit_rating += amount;
    temporary.hit_rating += temp_value * amount;
    current.attack_hit       += amount / rating.attack_hit;
    current.spell_hit        += amount / rating.spell_hit;
    break;

  case STAT_CRIT_RATING:
    stats.crit_rating += amount;
    temporary.crit_rating += temp_value * amount;
    current.attack_crit       += amount / rating.attack_crit;
    current.spell_crit        += amount / rating.spell_crit;
    break;

  case STAT_HASTE_RATING:
  {
    double old_attack_speed = 0;
    if ( main_hand_attack || off_hand_attack )
      old_attack_speed = composite_attack_speed();

    stats.haste_rating += amount;
    temporary.haste_rating += temp_value * amount;
    current.haste_rating       += amount;
    recalculate_haste();

    if ( main_hand_attack )
      main_hand_attack -> reschedule_auto_attack( old_attack_speed );
    if ( off_hand_attack )
      off_hand_attack -> reschedule_auto_attack( old_attack_speed );
    break;
  }
  case STAT_ARMOR:          stats.armor          += amount; temporary.armor += temp_value * amount; current.armor       += amount;                  break;
  case STAT_BONUS_ARMOR:    stats.bonus_armor    += amount; current.bonus_armor += amount;                  break;
  case STAT_DODGE_RATING:   stats.dodge_rating   += amount; temporary.dodge_rating += temp_value * amount; current.dodge       += amount / rating.dodge;   break;
  case STAT_PARRY_RATING:   stats.parry_rating   += amount; temporary.parry_rating += temp_value * amount; current.parry       += amount / rating.parry;   break;

  case STAT_BLOCK_RATING: stats.block_rating += amount; temporary.block_rating += temp_value * amount; current.block       += amount / rating.block; break;

  case STAT_MASTERY_RATING:
    stats.mastery_rating += amount;
    temporary.mastery_rating += temp_value * amount;
    current.mastery += amount / rating.mastery;
    break;

  default: assert( 0 ); break;
  }
}

// player_t::stat_loss ======================================================

void player_t::stat_loss( stat_e stat,
                          double    amount,
                          gain_t*   gain,
                          action_t* action,
                          bool      temporary_buff )
{
  if ( amount <= 0 ) return;

  if ( sim -> log ) sim -> output( "%s loses %.2f %s%s", name(), amount, util::stat_type_string( stat ), ( temporary_buff ) ? " (temporary)" : "" );

  int temp_value = temporary_buff ? 1 : 0;
  switch ( stat )
  {
  case STAT_STRENGTH:  stats.attribute[ ATTR_STRENGTH  ] -= amount; temporary.attribute[ ATTR_STRENGTH  ] -= temp_value * amount; current.attribute[ ATTR_STRENGTH  ] -= amount; break;
  case STAT_AGILITY:   stats.attribute[ ATTR_AGILITY   ] -= amount; temporary.attribute[ ATTR_AGILITY   ] -= temp_value * amount; current.attribute[ ATTR_AGILITY   ] -= amount; break;
  case STAT_STAMINA:   stats.attribute[ ATTR_STAMINA   ] -= amount; temporary.attribute[ ATTR_STAMINA   ] -= temp_value * amount; current.attribute[ ATTR_STAMINA   ] -= amount; stat_loss( STAT_MAX_HEALTH, floor( amount * composite_attribute_multiplier( ATTR_STAMINA ) ) * current.health_per_stamina, gain, action ); break;
  case STAT_INTELLECT: stats.attribute[ ATTR_INTELLECT ] -= amount; temporary.attribute[ ATTR_INTELLECT ] -= temp_value * amount; current.attribute[ ATTR_INTELLECT ] -= amount; break;
  case STAT_SPIRIT:    stats.attribute[ ATTR_SPIRIT    ] -= amount; temporary.attribute[ ATTR_SPIRIT    ] -= temp_value * amount; current.attribute[ ATTR_SPIRIT    ] -= amount; break;

  case STAT_ALL:
    for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
    {
      stats.attribute[ i ] -= amount;
      temporary.attribute[ i ] -= temp_value * amount;
      current.attribute[ i ] -= amount;
    }
    break;

  case STAT_HEALTH: resource_loss( RESOURCE_HEALTH, amount, gain, action ); break;
  case STAT_MANA:   resource_loss( RESOURCE_MANA,   amount, gain, action ); break;
  case STAT_RAGE:   resource_loss( RESOURCE_RAGE,   amount, gain, action ); break;
  case STAT_ENERGY: resource_loss( RESOURCE_ENERGY, amount, gain, action ); break;
  case STAT_FOCUS:  resource_loss( RESOURCE_FOCUS,  amount, gain, action ); break;
  case STAT_RUNIC:  resource_loss( RESOURCE_RUNIC_POWER, amount, gain, action ); break;

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

  case STAT_SPELL_POWER:       stats.spell_power       -= amount; temporary.spell_power -= temp_value * amount; current.spell_power[ SCHOOL_MAX ] -= amount; break;
  case STAT_MP5:               stats.mp5               -= amount; current.mp5                       -= amount; break;

  case STAT_ATTACK_POWER:             stats.attack_power             -= amount; temporary.attack_power -= temp_value * amount; current.attack_power       -= amount;                            break;
  case STAT_EXPERTISE_RATING:         stats.expertise_rating         -= amount; temporary.expertise_rating -= temp_value * amount; current.attack_expertise   -= amount / rating.expertise;         break;

  case STAT_HIT_RATING:
    stats.hit_rating -= amount;
    temporary.hit_rating -= temp_value * amount;
    current.attack_hit       -= amount / rating.attack_hit;
    current.spell_hit        -= amount / rating.spell_hit;
    break;

  case STAT_CRIT_RATING:
    stats.crit_rating -= amount;
    temporary.crit_rating -= temp_value * amount;
    current.attack_crit       -= amount / rating.attack_crit;
    current.spell_crit        -= amount / rating.spell_crit;
    break;

  case STAT_HASTE_RATING:
  {
    double old_attack_speed = 0;
    if ( main_hand_attack || off_hand_attack )
      old_attack_speed = composite_attack_speed();

    stats.haste_rating -= amount;
    temporary.haste_rating -= temp_value * amount;
    current.haste_rating       -= amount;
    recalculate_haste();

    if ( main_hand_attack )
      main_hand_attack -> reschedule_auto_attack( old_attack_speed );
    if ( off_hand_attack )
      off_hand_attack -> reschedule_auto_attack( old_attack_speed );
    break;
  }

  case STAT_ARMOR:          stats.armor          -= amount; temporary.armor -= temp_value * amount; current.armor       -= amount;                  break;
  case STAT_BONUS_ARMOR:    stats.bonus_armor    -= amount; current.bonus_armor -= amount;                  break;
  case STAT_DODGE_RATING:   stats.dodge_rating   -= amount; temporary.dodge_rating -= temp_value * amount; current.dodge       -= amount / rating.dodge;   break;
  case STAT_PARRY_RATING:   stats.parry_rating   -= amount; temporary.parry_rating -= temp_value * amount; current.parry       -= amount / rating.parry;   break;

  case STAT_BLOCK_RATING: stats.block_rating -= amount; temporary.block_rating -= temp_value * amount; current.block       -= amount / rating.block; break;

  case STAT_MASTERY_RATING:
    stats.mastery_rating -= amount;
    temporary.mastery_rating -= temp_value * amount;
    current.mastery -= amount / rating.mastery;
    break;

  default: assert( 0 ); break;
  }
}

// player_t::cost_reduction_gain ============================================

void player_t::cost_reduction_gain( school_e school,
                                    double        amount,
                                    gain_t*       /* gain */,
                                    action_t*     /* action */ )
{
  if ( amount <= 0 ) return;

  if ( sim -> log )
    sim -> output( "%s gains a cost reduction of %.0f on abilities of school %s", name(), amount,
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
    sim -> output( "%s loses a cost reduction %.0f on abilities of school %s", name(), amount,
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

// player_t::assess_damage ==================================================

void player_t::assess_damage( school_e school,
                              dmg_e    type,
                              action_state_t* s )
{
  if ( s -> result_amount <= 0 )
    return;

  if ( ! is_enemy() )
  {
    if ( buffs.pain_supression -> up() )
      s -> result_amount *= 1.0 + buffs.pain_supression -> data().effectN( 1 ).percent();

    if ( buffs.stoneform -> up() )
      s -> result_amount *= 1.0 + buffs.stoneform -> data().effectN( 1 ).percent();
  }

  target_mitigation( school, type, s );

  double absorbed_amount = 0;
  for ( size_t i = 0; i < absorb_buffs.size(); i++ )
  {
    absorb_buff_t* ab = absorb_buffs[ i ];
    if ( ab -> check() )
    {
      double buff_value = ab -> value();
      double value = std::min( s -> result_amount - absorbed_amount, buff_value );
      if ( ab -> absorb_source )
        ab -> absorb_source -> add_result( value, 0, ABSORB, RESULT_HIT );
      absorbed_amount += value;
      if ( sim -> debug ) sim -> output( "%s %s absorbs %.2f",
                                         name(), ab -> name_str.c_str(), value );
      ab -> absorb_used( value );
      if ( value == buff_value )
      {
        ab -> current_value = 0;
        ab -> expire();
      }
      else
      {
        ab -> current_value -= value;
        if ( sim -> debug ) sim -> output( "%s %s absorb remaining %.2f",
                                           name(), ab -> name_str.c_str(), ab -> current_value );
      }
    }
  }

  s -> result_amount -= absorbed_amount;

  iteration_dmg_taken += s -> result_amount;

  double actual_amount = resource_loss( RESOURCE_HEALTH, s -> result_amount, 0, s -> action );

  if ( false && ( this == sim -> target ) )
  {
    if ( sim -> target_death >= 0 )
      if ( resources.current[ RESOURCE_HEALTH ] <= sim -> target_death )
        sim -> iteration_canceled = 1;
  }
  else if ( resources.current[ RESOURCE_HEALTH ] <= 0 && ! is_enemy() && ! resources.is_infinite( RESOURCE_HEALTH ) )
  {
    // This can only save the target, if the damage is less than 200% of the target's health as of 4.0.6
    if ( buffs.guardian_spirit -> check() && actual_amount <= ( resources.max[ RESOURCE_HEALTH] * 2 ) )
    {
      // Just assume that this is used so rarely that a strcmp hack will do
      stats_t* stat = buffs.guardian_spirit -> source ? buffs.guardian_spirit -> source -> get_stats( "guardian_spirit" ) : 0;
      double gs_amount = resources.max[ RESOURCE_HEALTH ] * buffs.guardian_spirit -> data().effectN( 2 ).percent();
      resource_gain( RESOURCE_HEALTH, s -> result_amount );
      if ( stat ) stat -> add_result( gs_amount, gs_amount, HEAL_DIRECT, RESULT_HIT );
      buffs.guardian_spirit -> expire();
    }
    else
    {
      if ( ! current.sleeping )
      {
        deaths.add( sim -> current_time.total_seconds() );
      }
      if ( sim -> log ) sim -> output( "%s has died.", name() );
      demise();
    }
  }


}

// player_t::target_mitigation ==============================================

void player_t::target_mitigation( school_e school,
                                  dmg_e,
                                  action_state_t* s )
{
  if ( s -> result_amount == 0 )
    return;

  if ( s -> result == RESULT_BLOCK )
  {
    s -> result_amount *= ( 1 - composite_tank_block_reduction() );
    if ( s -> result_amount <= 0 ) return;
  }

  if ( s -> result == RESULT_CRIT_BLOCK )
  {
    s -> result_amount *= ( 1 - 2 * composite_tank_block_reduction() );
    if ( s -> result_amount <= 0 ) return;
  }

  if ( school == SCHOOL_PHYSICAL )
  {
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> output( "Damage to %s before armor mitigation is %f", s -> target -> name(), s -> result_amount );

    // Armor
    if ( s -> action )
    {
      double resist = s -> action -> armor() / ( s -> action -> armor() + s -> action -> player -> armor_coeff );

      if ( resist < 0.0 )
        resist = 0.0;
      else if ( resist > 0.75 )
        resist = 0.75;
      s -> result_amount *= 1.0 - resist;
    }

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> output( "Damage to %s after armor mitigation is %f", s -> target -> name(), s -> result_amount );
  }
}

// player_t::assess_heal ====================================================

void player_t::assess_heal( school_e, dmg_e, heal_state_t* s )
{
  if ( buffs.guardian_spirit -> up() )
    s -> result_amount *= 1.0 + buffs.guardian_spirit -> data().effectN( 1 ).percent();

  s -> total_result_amount = s -> result_amount;
  s -> result_amount = resource_gain( RESOURCE_HEALTH, s -> result_amount, 0, s -> action );

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

// player_t::register_callbacks =============================================

void player_t::register_callbacks()
{
}

// player_t::register_resource_gain_callback ================================

void player_t::callbacks_t::register_resource_gain_callback( resource_e resource_type,
                                                             action_callback_t* cb )
{
  resource_gain[ resource_type ].push_back( cb );
}

// player_t::register_resource_loss_callback ================================

void player_t::callbacks_t::register_resource_loss_callback( resource_e resource_type,
                                                             action_callback_t* cb )
{
  resource_loss[ resource_type ].push_back( cb );
}

// player_t::register_attack_callback =======================================

void player_t::callbacks_t::register_attack_callback( int64_t mask,
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

void player_t::callbacks_t::register_spell_callback( int64_t mask,
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

void player_t::callbacks_t::register_tick_callback( int64_t mask,
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

void player_t::callbacks_t::register_heal_callback( int64_t mask,
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

// player_t::register_absorb_callback =========================================

void player_t::callbacks_t::register_absorb_callback( int64_t mask,
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

void player_t::callbacks_t::register_harmful_spell_callback( int64_t mask,
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


// player_t::register_direct_harmful_spell_callback ================================

void player_t::callbacks_t::register_direct_harmful_spell_callback( int64_t mask,
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

void player_t::callbacks_t::register_tick_damage_callback( int64_t mask,
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

void player_t::callbacks_t::register_direct_damage_callback( int64_t mask,
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

// player_t::register_direct_crit_callback ================================

void player_t::callbacks_t::register_direct_crit_callback( int64_t mask,
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

// player_t::register_spell_tick_damage_callback ==================================

void player_t::callbacks_t::register_spell_tick_damage_callback( int64_t mask,
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

// player_t::register_spell_direct_damage_callback ================================

void player_t::callbacks_t::register_spell_direct_damage_callback( int64_t mask,
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

void player_t::callbacks_t::register_tick_heal_callback( int64_t mask,
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

void player_t::callbacks_t::register_direct_heal_callback( int64_t mask,
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

void player_t::callbacks_t::reset()
{
  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    action_callback_t::reset( resource_gain[ i ] );
    action_callback_t::reset( resource_loss[ i ] );
  }
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; i++ )
  {
    action_callback_t::reset( attack[ i ] );
    action_callback_t::reset( spell [ i ] );
    action_callback_t::reset( harmful_spell [ i ] );
    action_callback_t::reset( heal [ i ] );
    action_callback_t::reset( absorb [ i ] );
    action_callback_t::reset( tick  [ i ] );
  }
  for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX; i++ )
  {
    action_callback_t::reset( tick_damage  [ i ] );
    action_callback_t::reset( direct_damage[ i ] );
    action_callback_t::reset( direct_crit  [ i ] );
    action_callback_t::reset( spell_tick_damage  [ i ] );
    action_callback_t::reset( spell_direct_damage[ i ] );
  }
}

// player_t::recalculate_haste ==============================================

void player_t::recalculate_haste()
{
  spell_haste = 1.0 / ( 1.0 + current.haste_rating / rating. spell_haste );
  attack_haste = 1.0 / ( 1.0 + current.haste_rating / rating.attack_haste );
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

// player_t::clear_action_priority_lists() ===========

void player_t::clear_action_priority_lists() const
{
  for ( size_t i = 0; i < action_priority_list.size(); i++ )
  {
    action_priority_list_t* a = action_priority_list[ i ];
    a -> action_list_str.clear();
  }
}

template <typename T>
T* find_vector_member( std::vector<T*> list, const std::string& name )
{
  for ( size_t i = 0; i < list.size(); ++i )
  {
    T* t = list[ i ];
    if ( t -> name_str == name )
      return t;
  }
  return 0;
}

// player_t::find_stats ======================================================

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

rng_t* player_t::find_rng ( const std::string& name )
{ return find_vector_member( rng_list, name ); }

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
    c = new cooldown_t( name, this );

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

// player_t::get_benefit =====================================================

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
    u = new uptime_t(  sim -> statistics_level, sim -> iterations , name );

    uptime_list.push_back( u );
  }

  return u;
}

// player_t::get_rng ========================================================

rng_t* player_t::get_rng( const std::string& n )
{
  assert( sim -> rng );

  if ( ! sim -> separated_rng )
    return sim -> default_rng();

  rng_t* rng = find_rng( n );

  if ( ! rng )
  {
    rng = rng_t::create( n );

    rng_list.push_back( rng );
  }

  return rng;
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

action_priority_list_t* player_t::get_action_priority_list( const std::string& name )
{
  action_priority_list_t* a = find_action_priority_list( name );
  if ( ! a )
  {
    a = new action_priority_list_t( name, this );
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
      sim -> output( "%s starts moving.", player -> name() );

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

    if ( sim -> log ) sim -> output( "%s stops moving.", player -> name() );
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

// Arcane Torrent ===========================================================

struct arcane_torrent_t : public spell_t
{
  resource_e resource;
  double gain;

  arcane_torrent_t( player_t* p, const std::string& options_str ) :
    spell_t( "arcane_torrent", p, p -> find_racial_spell( "Arcane Torrent" ) ),
    resource( RESOURCE_NONE ), gain( 0 )
  {
    parse_options( NULL, options_str );

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

    // FIXME: Remove once 5.1 goes live
    if ( resource == RESOURCE_MANA && player -> dbc.build_level( player -> dbc.ptr ) < 16231 )
      gain /= 3.0; // Hotfix Nerf from 6% to 2%.
  }

  virtual void execute()
  {
    if ( resource == RESOURCE_MANA )
      gain = player -> resources.max [ RESOURCE_MANA ] * data().effectN( 2 ).resource( resource );

    player -> resource_gain( resource, gain, player -> gains.arcane_torrent );

    spell_t::execute();
  }
};

// Berserking ===============================================================

struct berserking_t : public spell_t
{
  berserking_t( player_t* p, const std::string& options_str ) :
    spell_t( "berserking", p, p -> find_racial_spell( "Berserking" ) )
  {
    harmful = false;
    parse_options( NULL, options_str );

    if ( &data() == &spell_data_not_found_t::singleton )
    {
      sim -> errorf( "Player %s attempting to execute action %s without spell data. Setting action to background.\n",
                     player -> name(), name() );
      background = true; // prevent action from being executed
    }
  }

  virtual void execute()
  {
    spell_t::execute();

    player -> buffs.berserking -> trigger();
  }
};

// Blood Fury ===============================================================

struct blood_fury_t : public spell_t
{
  blood_fury_t( player_t* p, const std::string& options_str ) :
    spell_t( "blood_fury", p, p -> find_racial_spell( "Blood Fury" ) )
  {
    harmful = false;
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    spell_t::execute();

    player -> buffs.blood_fury -> trigger();
  }
};

// Rocket Barrage ===========================================================

struct rocket_barrage_t : public spell_t
{
  rocket_barrage_t( player_t* p, const std::string& options_str ) :
    spell_t( "rocket_barrage", p, p -> find_racial_spell( "Rocket Barrage" ) )
  {
    parse_options( NULL, options_str );

    base_spell_power_multiplier  = direct_power_mod;
    base_attack_power_multiplier = data().extra_coeff();
    direct_power_mod             = 1.0;
  }
};

// Stoneform ================================================================

struct stoneform_t : public spell_t
{
  stoneform_t( player_t* p, const std::string& options_str ) :
    spell_t( "stoneform", p, p -> find_racial_spell( "Stoneform" ) )
  {
    harmful = false;
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    spell_t::execute();

    player -> buffs.stoneform -> trigger();
  }
};

// Lifeblood ================================================================

struct lifeblood_t : public spell_t
{
  lifeblood_t( player_t* player, const std::string& options_str ) :
    spell_t( "lifeblood", player )
  {
    if ( player -> profession[ PROF_HERBALISM ] < 1 )
    {
      sim -> errorf( "Player %s attempting to execute action %s without Herbalism.\n",
                     player -> name(), name() );

      background = true; // prevent action from being executed
    }
    parse_options( NULL, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 120 );
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
      { "name", OPT_STRING,  &seq_name_str },
      { NULL,   OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    if ( ! seq )
    {
      for ( size_t i = 0; i < player -> action_list.size() && !seq; ++i )
      {
        action_t* a = player -> action_list[ i ];
        if ( a -> type != ACTION_SEQUENCE )
          continue;

        if ( ! seq_name_str.empty() )
          if ( seq_name_str != a -> name_str )
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

    seq -> restart();
  }

  virtual bool ready()
  {
    if ( seq ) return seq -> can_restart();
    return action_t::ready();
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
      { "mana", OPT_FLT, &mana },
      { NULL, OPT_UNKNOWN, NULL }
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

    if ( sim -> log ) sim -> output( "%s performs %s", p -> name(), name() );

    for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; ++i )
      p -> buffed.attribute[ i ] = floor( p -> get_attribute( i ) );

    p -> buffed.resource     = p -> resources.max;

    p -> buffed.spell_haste  = p -> composite_spell_haste();
    p -> buffed.attack_haste = p -> composite_attack_haste();
    p -> buffed.attack_speed = p -> composite_attack_speed();
    p -> buffed.mastery      = p -> composite_mastery();

    p -> buffed.spell_power  = util::round( p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier() );
    p -> buffed.spell_hit    = p -> composite_spell_hit();
    p -> buffed.spell_crit   = p -> composite_spell_crit();
    p -> buffed.mp5          = p -> composite_mp5();

    p -> buffed.attack_power = p -> composite_attack_power() * p -> composite_attack_power_multiplier();
    p -> buffed.attack_hit   = p -> composite_attack_hit();
    p -> buffed.mh_attack_expertise = p -> composite_attack_expertise( &( p -> main_hand_weapon ) );
    p -> buffed.oh_attack_expertise = p -> composite_attack_expertise( &( p -> off_hand_weapon ) );
    p -> buffed.attack_crit  = p -> composite_attack_crit( &( p -> main_hand_weapon ) );

    p -> buffed.armor        = p -> composite_armor();
    p -> buffed.miss         = p -> composite_tank_miss( SCHOOL_PHYSICAL );
    p -> buffed.dodge        = p -> composite_tank_dodge();
    p -> buffed.parry        = p -> composite_tank_parry();
    p -> buffed.block        = p -> composite_tank_block();
    p -> buffed.crit         = p -> composite_tank_crit( SCHOOL_PHYSICAL );

    role_e role = p -> primary_role();
    int delta_level = sim -> target -> level - p -> level;
    double spell_hit_extra=0, attack_hit_extra=0, expertise_extra=0;

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
      double chance = spell -> miss_chance( spell -> composite_hit(), delta_level );
      if ( chance < 0 ) spell_hit_extra = -chance * p -> rating.spell_hit;
    }

    if ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK )
    {
      attack_t* attack = new melee_attack_t( "snapshot_attack", p );
      attack -> background = true;
      attack -> init();
      double chance = attack -> miss_chance( attack -> composite_hit(), delta_level );
      if ( p -> dual_wield() ) chance += 0.19;
      if ( chance < 0 ) attack_hit_extra = -chance * p -> rating.attack_hit;
      if ( p -> position() != POSITION_FRONT  )
      {
        chance = attack -> dodge_chance( p -> composite_attack_expertise(), delta_level );
        if ( chance < 0 ) expertise_extra = -chance * p -> rating.expertise;
      }
      else if ( p ->position() == POSITION_FRONT )
      {
        chance = attack -> parry_chance( p -> composite_attack_expertise(), delta_level );
        if ( chance < 0 ) expertise_extra = -chance * p -> rating.expertise;
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
  expr_t* time_expr;

  wait_fixed_t( player_t* player, const std::string& options_str ) :
    wait_action_base_t( player, "wait" )
  {
    std::string sec_str = "1.0";

    option_t options[] =
    {
      { "sec", OPT_STRING, &sec_str },
      { NULL,  OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    time_expr = expr_t::parse( this, sec_str );
  }

  virtual timespan_t execute_time()
  {
    timespan_t wait = timespan_t::from_seconds( time_expr -> eval() );
    if ( wait <= timespan_t::zero() ) wait = player -> available();
    return wait;
  }
};

// Wait Until Ready Action ==================================================

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
    std::string item_name;
    option_t options[] =
    {
      { "name", OPT_STRING, &item_name },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( item_name.empty() )
    {
      sim -> errorf( "Player %s has 'use_item' action with no 'name=' option.\n", player -> name() );
      return;
    }

    item = player -> find_item( item_name );
    if ( ! item )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which is not currently equipped.\n", player -> name(), item_name.c_str() );
      return;
    }
    if ( ! item -> use.active() )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which has no 'use=' encoding.\n", player -> name(), item_name.c_str() );
      item = 0;
      return;
    }

    name_str = name_str + "_" + item_name;
    stats = player ->  get_stats( name_str, this );

    item_t::special_effect_t& e = item -> use;

    use_name = e.name_str.empty() ? item_name : e.name_str;

    if ( e.trigger_type )
    {
      if ( e.cost_reduction && e.school && e.discharge_amount )
      {
        trigger = unique_gear::register_cost_reduction_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                             e.school, e.max_stacks, e.discharge_amount,
                                                             e.proc_chance, timespan_t::zero()/*dur*/, timespan_t::zero()/*cd*/, false, e.reverse );
      }
      else if ( e.stat )
      {
        trigger = unique_gear::register_stat_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                   e.stat, e.max_stacks, e.stat_amount,
                                                   e.proc_chance, timespan_t::zero()/*dur*/, timespan_t::zero()/*cd*/, e.tick, e.reverse );
      }
      else if ( e.school )
      {
        trigger = unique_gear::register_discharge_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                        e.max_stacks, e.school, e.discharge_amount, e.discharge_scaling,
                                                        e.proc_chance, timespan_t::zero()/*cd*/, e.aoe, e.override_result_es_mask, e.result_es_mask );
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
             .cd( timespan_t::zero() )
             .chance( e.proc_chance )
             .reverse( e.reverse )
             .add_stat( e.stat, e.stat_amount );
    }
    else assert( false );

    std::string cooldown_name = use_name;
    cooldown_name += "_";
    cooldown_name += item -> slot_name();

    cooldown = player -> get_cooldown( cooldown_name );
    cooldown -> duration = item -> use.cooldown;
    trigger_gcd = timespan_t::zero();

    if ( buff != 0 ) buff -> cooldown = cooldown;
  }

  void lockout( timespan_t duration )
  {
    if ( duration <= timespan_t::zero() ) return;

    player -> item_cooldown -> start( duration );
  }

  virtual void execute()
  {
    if ( discharge )
    {
      discharge -> execute();
    }
    else if ( trigger )
    {
      if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), use_name.c_str() );

      trigger -> activate();

      if ( item -> use.duration != timespan_t::zero() )
      {
        struct trigger_expiration_t : public event_t
        {
          item_t* item;
          action_callback_t* trigger;

          trigger_expiration_t( sim_t* sim, player_t* player, item_t* i, action_callback_t* t ) : event_t( sim, player, i -> name() ), item( i ), trigger( t )
          {
            sim -> add_event( this, item -> use.duration );
          }
          virtual void execute()
          {
            trigger -> deactivate();
          }
        };

        new ( sim ) trigger_expiration_t( sim, player, item, trigger );

        lockout( item -> use.duration );
      }
    }
    else if ( buff )
    {
      if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), use_name.c_str() );
      buff -> trigger();
      lockout( buff -> buff_duration );
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

    if ( player -> item_cooldown -> remains() > timespan_t::zero() ) return false;

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
      { "name", OPT_STRING, &buff_name },
      { NULL,  OPT_UNKNOWN, NULL }
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
    if ( sim -> log ) sim -> output( "%s cancels buff %s", player -> name(), buff -> name_str.c_str() );
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
      { "name", OPT_STRING, &alist_name },
      { NULL,  OPT_UNKNOWN, NULL }
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
    if ( sim -> log ) sim -> output( "%s swaps to action list %s", player -> name(), alist -> name_str.c_str() );
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
    if ( sim -> log ) sim -> output( "%s runs action list %s", player -> name(), alist -> name_str.c_str() );

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
      { "wait",     OPT_TIMESPAN, &wait     },
      { "for_next", OPT_BOOL,     &for_next },
      { "resource", OPT_STRING,   &resource_str },
      { "extra_amount",   OPT_FLT,    &amount },
      { 0,              OPT_UNKNOWN,  0 }
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
      sim -> output( "%s performs %s", player -> name(), name() );
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

// player_t::parse_talents_old_armory ===========================================

bool player_t::parse_talents_old_armory( const std::string& talent_string )
{
  // We don't really care about extracting the actual talents from here. Just the spec and class.
  player_e w_class = PLAYER_NONE;
  specialization_e w_spec = SPEC_NONE;
  uint32_t specidx = 0;

  if ( talent_string.size() < 7 )
  {
    sim -> errorf( "Player %s has malformed Cataclysm battle.net talent string. Empty or too short string.\n", name() );
    return false;
  }

  if ( talent_string[ 1 ] != 'c' || talent_string[ 2 ] != '0' )
  {
    sim -> errorf( "Player %s has malformed Cataclysm battle.net talent string.\n", name() );
    return false;
  }

  // Parse class
  switch ( talent_string[ 0 ] )
  {
  case 'd' : w_class = DEATH_KNIGHT; break;
  case 'U' : w_class = DRUID; break;
  case 'Y' : w_class = HUNTER; break;
  case 'e' : w_class = MAGE; break;
  case 'b' : w_class = PALADIN; break;
  case 'X' : w_class = PRIEST; break;
  case 'c' : w_class = ROGUE; break;
  case 'W' : w_class = SHAMAN; break;
  case 'V' : w_class = WARLOCK; break;
  case 'Z' : w_class = WARRIOR; break;
  default:
  {
    sim -> errorf( "Player %s has malformed Cataclysm battle.net talent string. Invalid class character.\n", name() );
    return false;
  }
  }

  if ( talent_string[ 4 ] < '0' || talent_string[ 4 ] > '2' )
  {
    sim -> errorf( "Player %s has malformed Cataclysm battle.net talent string. Invalid spec character.\n", name() );
    return false;
  }
  specidx = talent_string[ 4 ] - '0';

  if ( w_class != type )
  {
    sim -> errorf( "Player %s has malformed Cataclysm talent string. Talent string class %s does not match player class %s.\n", name(),
                   util::player_type_string( w_class ), util::player_type_string( type ) );
    return false;
  }

  /* Specific override for Druids to detect those Feral Druids with Natural Reaction indicating they're Guardian */
  if ( type == DRUID )
  {
    if ( specidx == 2 )
    {
      specidx = 3;
    }
    else if ( specidx == 1 )
    {
      std::vector<std::string> splits;
      int num_splits = util::string_split( splits, talent_string, "!" );
      if ( num_splits >= 3 )
      {
        if ( splits[ 2 ].size() >= 6 )
        {
          switch ( splits[ 2 ][ 5 ] )
          {
          case 'd':
          case 'g':
          case 'j':
          case 'W':
          case 'T':
          case 'Q':
            specidx = 2;
            break;
          default: break;
          }
        }
      }
    }
  }

  w_spec = dbc.spec_by_idx( type, specidx );

  _spec = w_spec;

  if ( parse_talents_numbers( set_default_talents() ) )
  {
    talents_str = set_default_talents();
    return true;
  }

  create_talents_armory();

  return false;
}

// player_t::parse_talents_numbers ===========================================

bool player_t::parse_talents_numbers( const std::string& talent_string )
{
  std::array<uint32_t,MAX_TALENT_ROWS> encoding;

  size_t i, j;
  size_t i_max = std::min( talent_string.size(),
                           static_cast< size_t >( MAX_TALENT_ROWS ) );

  for ( j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_list[ j * MAX_TALENT_COLS + i ] = 0;
    }
  }

  for ( i = 0; i < i_max; i++ )
  {
    char c = talent_string[ i ];
    if ( c < '0' || c > ( '0' + MAX_TALENT_COLS )  )
    {
      sim -> errorf( "Player %s has illegal character '%c' in talent encoding.\n", name(), c );
      return false;
    }
    encoding[ i ] = c - '0';
  }

  while ( i < encoding.size() )
    encoding[ i++ ] = 0;

  for ( j = 0; j < talent_list.size(); j++ )
  {
    talent_list[ j ] = encoding[ j / MAX_TALENT_COLS ] == ( ( j % MAX_TALENT_COLS ) + 1 );
  }

  create_talents_numbers();

  return true;
}

// player_t::parse_talents_armory ===========================================

bool player_t::parse_talents_armory( const std::string& talent_string )
{
  player_e w_class = PLAYER_NONE;
  specialization_e w_spec = SPEC_NONE;
  uint32_t specidx = 0;
  std::string::size_type cut_pt;

  if ( talent_string.size() < 2 )
  {
    sim -> errorf( "Player %s has malformed MoP battle.net talent string. Empty or too short string.\n", name() );
    return false;
  }

  // Parse class
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
  {
    sim -> errorf( "Player %s has malformed MoP battle.net talent string. Invalid class character.\n", name() );
    return false;
  }
  }

  if ( w_class != type )
  {
    sim -> errorf( "Player %s has malformed Cataclysm talent string. Talent string class %s does not match player class %s.\n", name(),
                   util::player_type_string( w_class ), util::player_type_string( type ) );
    return false;
  }

  size_t i, j;

  for ( j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_list[ j * MAX_TALENT_COLS + i ] = 0;
    }
  }

  std::string spec_string;

  if ( ( cut_pt = talent_string.find_first_of( '!' ) ) == talent_string.npos )
  {
    sim -> errorf( "Player %s has malformed MoP battle.net talent string.\n", name() );
    return false;
  }

  spec_string = talent_string.substr( 1, cut_pt - 1 );
  if ( spec_string.size() != 0 )
  {
    // A spec was specified
    switch ( spec_string[ 0 ] )
    {
    case 'a' : specidx = 0; break;
    case 'Z' : specidx = 1; break;
    case 'b' : specidx = 2; break;
    case 'Y' : specidx = 3; break;
    default:
    {
      sim -> errorf( "Player %s has malformed MoP battle.net talent string. Invalid spec character\n", name() );
      return false;
    }
    }

    w_spec = dbc.spec_by_idx( type, specidx );

    _spec = w_spec;
  }

  std::string t_str = talent_string.substr( cut_pt + 1 );

  if ( t_str.empty() )
  {
    // No talents picked.
    return true;
  }

  if ( t_str.size() < MAX_TALENT_ROWS )
  {
    sim -> errorf( "Player %s has malformed MoP battle.net talent string. Talent list too short.\n", name() );
    return false;
  }

  for ( j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    switch ( t_str[ j ] )
    {
    case '.' : break;
    case '0' : talent_list[ j * MAX_TALENT_COLS + 0 ] = 1; break;
    case '1' : talent_list[ j * MAX_TALENT_COLS + 1 ] = 1; break;
    case '2' : talent_list[ j * MAX_TALENT_COLS + 2 ] = 1; break;
    default:
    {
      sim -> errorf( "Player %s has malformed MoP battle.net talent string. Talent list has invalid characters.\n", name() );
      return false;
    }
    }
  }

  create_talents_armory();

  return true;
}

// player_t::create_talents_wowhead =========================================
void player_t::create_talents_wowhead()
{
  talents_str.clear();
  talents_str += "http://mop.wowhead.com/mists-of-pandaria-talent-calculator#";

  // Class Type
  char c = '\0';
  switch ( type )
  {
  case DEATH_KNIGHT : c = 'k'; break;
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
  default: break;
  }
  talents_str += c;

  // Spec if specified
  uint32_t idx = 0;
  uint32_t cid = 0;
  if ( dbc.spec_idx( _spec, cid, idx ) && ( ( int ) cid == util::class_id( type ) ) )
  {
    switch ( idx )
    {
    case 0 : talents_str += '!'; break;
    case 1 : talents_str += 'x'; break;
    case 2 : talents_str += 'y'; break;
    default: break;
    }
  }

  std::array< int, 9 > key = {{ 48, 32, 16, 12, 8, 4, 2, 1, 0 }};

  bool found_tier = false;
  bool found_this_char = false;

  char v[ 2 ];

  for ( int k = 1; k >= 0; k-- )
  {
    c = '\0';
    v[ k ] = '\0';

    found_this_char = false;
    found_tier = false;

    for ( int j = 2; j >= 0; j-- )
    {
      unsigned int i = 0;

      for ( ; i < MAX_TALENT_COLS; i++ )
      {
        if ( talent_list[ ( ( k * 3 ) + j ) * MAX_TALENT_COLS + i ] )
        {
          found_tier = true;
          found_this_char = true;
          break;
        }
      }
      if ( i >= MAX_TALENT_COLS ) // Wowhead can't actually handle this case properly if it's not a trailing tier.
      {
        if ( found_tier ) // There's a later row that has talents. Set the talent on this tier to the first until Wowhead supports it right.
        {
          c += key[ 8 - ( j * 3 ) ];
        }
      }
      else
      {
        c += key[ 8 - ( ( j * 3 ) + i ) ];
      }
    }

    if ( found_this_char )
      c += '0';

    v[ k ] = c;
  }

  if ( v[ 0 ] == 0 )
  {
    if ( v[ 1 ] > 0 )
    {
      v[ 0 ] = '0';
    }
    else return;
  }
  talents_str += v[ 0 ];
  if ( v[ 1 ] != 0 )
    talents_str += v[ 1 ];
}

void player_t::create_talents_armory()
{
  talents_str.clear();

  talents_str = "http://us.battle.net/wow/en/tool/talent-calculator#";

  switch ( type )
  {
  case DEATH_KNIGHT : talents_str += "d"; break;
  case DRUID        : talents_str += "U"; break;
  case HUNTER       : talents_str += "Y"; break;
  case MAGE         : talents_str += "e"; break;
  case MONK         : talents_str += "f"; break;
  case PALADIN      : talents_str += "b"; break;
  case PRIEST       : talents_str += "X"; break;
  case ROGUE        : talents_str += "c"; break;
  case SHAMAN       : talents_str += "W"; break;
  case WARLOCK      : talents_str += "V"; break;
  case WARRIOR      : talents_str += "Z"; break;
  default:
    talents_str.clear();
    return;
  }

  if ( _spec != SPEC_NONE )
  {
    uint32_t cid, sid;

    if ( ! dbc.spec_idx( _spec, cid, sid ) )
    {
      talents_str.clear();
      return;
    }
    switch ( sid )
    {
    case 0: talents_str += "a"; break;
    case 1: talents_str += "Z"; break;
    case 2: talents_str += "b"; break;
    case 3: talents_str += "Y"; break;
    default:
      talents_str.clear();
      return;
    }
  }

  talents_str += "!";

  for ( unsigned j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    unsigned i = 0;
    for ( ; i < MAX_TALENT_COLS; i++ )
    {
      if ( talent_list[ j * MAX_TALENT_COLS + i ] )
        break;
    }
    if ( i >= MAX_TALENT_COLS )
    {
      talents_str += ".";
    }
    else
    {
      talents_str += util::to_string( i );
    }
  }
}

void player_t::create_talents_numbers()
{
  talents_str.clear();

  // This is necessary because sometimes the talent trees change shape between live/ptr.
  for ( unsigned j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    unsigned i = 0;
    for ( ; i < MAX_TALENT_COLS; i++ )
    {
      if ( talent_list[ j * MAX_TALENT_COLS + i ] )
        break;
    }
    if ( i >= MAX_TALENT_COLS )
      i = 0;
    else
      i++;

    talents_str += util::to_string( i );
  }
}

// player_t::parse_talents_wowhead ==========================================

bool player_t::parse_talents_wowhead( const std::string& talent_string )
{
  player_e w_class = PLAYER_NONE;
  uint32_t w_spec_idx = dbc.specialization_max_per_class();
  std::array< int, MAX_TALENT_SLOTS > encoding;
  uint32_t idx = 0;
  uint32_t encoding_idx = 0;
  std::array< int, 9 > key = {{ 48, 32, 16, 12, 8, 4, 2, 1, 0 }};
  char max_char = 48 + 12 + 2 + '0';

  range::fill( encoding, 0 );

  if ( ! talent_string.size() )
  {
    sim -> errorf( "Player %s has malformed wowhead talent string. Empty string.\n", name() );
    return false;
  }

  // Parse class
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
  default: break;
  }

  if ( w_class == PLAYER_NONE )
  {
    sim -> errorf( "Player %s has malformed wowhead talent string. Empty class value.\n", name() );
    return false;
  }
  if ( w_class != type )
  {
    sim -> errorf( "Player %s has malformed wowhead talent string. Talent string class %s does not match player class %s.\n", name(),
                   util::player_type_string( w_class ), util::player_type_string( type ) );
    return false;
  }

  idx = 1;

  // Parse spec if specified
  switch ( talent_string[ idx ] )
  {
  case '!' : w_spec_idx = 0; break;
  case 'x' : w_spec_idx = 1; break;
  case 'y' : w_spec_idx = 2; break;
  default: break;
  }

  if ( w_spec_idx < dbc.specialization_max_per_class() )
  {
    idx++;

    specialization_e w_spec = dbc.spec_by_idx( type, w_spec_idx );

    _spec = w_spec;
  }

  if ( ( talent_string.size() - idx ) > 2 )
  {
    sim -> errorf( "Player %s has malformed wowhead talent string. String is too long.\n", name() );
    return false;
  }

  while ( idx < talent_string.size() )
  {
    // Process 3 rows of talents per character.
    uint32_t key_idx = 0;
    unsigned char c = talent_string[ idx ];

    if ( ( c < '0' ) || ( c > max_char ) )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string. Character in position %d is invalid.\n", name(), idx );
      return false;
    }

    c -= '0';

    while ( key_idx < key.size() )
    {
      if ( c == 0 && ! ( encoding[ encoding_idx + 1 ] || encoding[ encoding_idx + 2 ] ) )
      {
        encoding[ encoding_idx ] = 1;
      }
      else if ( c > 0 && c >= key[ key_idx ] )
      {
        encoding[ encoding_idx + ( key.size() - key_idx ) - 1 ] = 1;
        c -= key[ key_idx ];
      }
      key_idx++;
    }

    idx++;
    encoding_idx += key.size();
  }

  if ( sim -> debug )
  {
    std::ostringstream str_out;
    for ( size_t i = 0; i < encoding.size(); i++ )
      str_out << encoding[ i ];

    util::fprintf( sim -> output_file, "%s Wowhead talent string translation: %s\n", name(), str_out.str().c_str() );
  }

  for ( uint32_t j = 0; j < talent_list.size(); j++ )
  {
    talent_list[ j ] = encoding[ j ];
  }

  create_talents_wowhead();

  return true;
}

// player_t::replace_spells ======================================================

void player_t::replace_spells()
{
  unsigned id;
  unsigned int class_idx, spec_index;

  if ( ! dbc.spec_idx( _spec, class_idx, spec_index ) )
    return;

  // Search spec spells for spells to replace.
  if ( _spec != SPEC_NONE )
  {
    for ( unsigned int i = 0; i < dbc.specialization_ability_size(); i++ )
    {
      if ( ( id = dbc.specialization_ability( class_idx, spec_index, i ) ) == 0 )
      {
        break;
      }
      if ( dbc.spell( id ) && dbc.spell( id ) -> _replace_spell_id && ( ( int )dbc.spell( id ) -> level() <= level ) )
      {
        // Found a spell we should replace
        dbc.replace_id( dbc.spell( id ) -> _replace_spell_id, id );
      }
    }
  }

  // Search talents for spells to replace.
  for ( unsigned int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( unsigned int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      if ( talent_list[ j * MAX_TALENT_COLS + i ] && ( level >= ( int ) ( ( j + 1 ) * 15 ) ) )
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
      if ( ( id = dbc.glyph_spell( class_idx, j, i ) ) == 0 )
      {
        break;
      }
      if ( dbc.spell( id ) && dbc.spell( id ) -> _replace_spell_id )
      {
        // Found a spell that might need replacing. Check to see if we have that glyph activated
        for ( std::vector<const spell_data_t*>::iterator it = glyph_list.begin(); it != glyph_list.end(); ++it )
        {
          if ( ( *it ) && ( *it ) -> id() == id )
          {
            dbc.replace_id( dbc.spell( id ) -> _replace_spell_id, id );
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
      if ( ( id = dbc.class_ability( class_idx, 0, i ) ) == 0 )
      {
        break;
      }
      if ( dbc.spell( id ) && dbc.spell( id ) -> _replace_spell_id && ( ( int )dbc.spell( id ) -> level() <= level ) )
      {
        // Found a spell we should replace
        dbc.replace_id( dbc.spell( id ) -> _replace_spell_id, id );
      }
    }
  }
}


// player_t::find_talent_spell ====================================================

const spell_data_t* player_t::find_talent_spell( const std::string& n,
                                                 const std::string& token )
{
  unsigned spell_id = dbc.talent_ability_id( type, n.c_str() );

  if ( ! spell_id && token == "" ) spell_id = dbc.get_token_id( n );

  if ( ! spell_id || ! dbc.spell( spell_id ) )
  {
    return ( spell_data_t::not_found() );
  }

  for ( unsigned int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( unsigned int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_data_t* td = talent_data_t::find( type, j, i, dbc.ptr );
      if ( td && ( td -> spell_id() == spell_id ) )
      {
        if ( ! talent_list[ j * MAX_TALENT_COLS + i ] || ( level < ( int )( ( j + 1 ) * 15 ) ) )
        {
          return ( spell_data_t::not_found() );
        }
        // We have that talent enabled.
        dbc_t::add_token( spell_id, token, dbc.ptr );

        return ( dbc.spell( spell_id ) );
      }
    }
  }

  /* Talent not enabled */
  return ( spell_data_t::not_found() );
}

// player_t::find_glyph =====================================================

const spell_data_t* player_t::find_glyph( const std::string& n )
{
  unsigned spell_id = dbc.glyph_spell_id( type, n.c_str() );

  if ( ! spell_id || ! dbc.spell( spell_id ) )
  {
    return ( spell_data_t::not_found() );
  }

  return ( dbc.spell( spell_id ) );
}


// player_t::find_glyph_spell =====================================================

const spell_data_t* player_t::find_glyph_spell( const std::string& n, const std::string& token )
{
  const spell_data_t* g = find_glyph( n );

  if ( ! g )
    return ( spell_data_t::not_found() );

  for ( std::vector<const spell_data_t*>::iterator i = glyph_list.begin(); i != glyph_list.end(); ++i )
  {
    if ( ( *i ) && ( *i ) -> id() == g -> id() )
    {
      dbc_t::add_token( g -> id(), token, dbc.ptr );
      return g;
    }
  }

  return ( spell_data_t::not_found() );
}

// player_t::find_specialization_spell ======================================

const spell_data_t* player_t::find_specialization_spell( const std::string& name, const std::string& token, specialization_e s )
{
  unsigned spell_id = dbc.specialization_ability_id( _spec, name.c_str() );

  if ( s != SPEC_NONE && s != _spec )
    return spell_data_t::not_found();

  if ( ! spell_id || ! dbc.spell( spell_id ) || ( ( int )dbc.spell( spell_id ) -> level() > level ) )
    return ( spell_data_t::not_found() );

  dbc_t::add_token( spell_id, token, dbc.ptr );

  return ( dbc.spell( spell_id ) );
}

// player_t::find_mastery_spell =============================================

const spell_data_t* player_t::find_mastery_spell( specialization_e s, const std::string& token, uint32_t idx )
{
  unsigned spell_id = dbc.mastery_ability_id( s, idx );

  if ( ( s == SPEC_NONE ) || ( s != _spec ) || ! spell_id || ! dbc.spell( spell_id ) || ( ( int )dbc.spell( spell_id ) -> level() > level ) )
    return ( spell_data_t::not_found() );

  dbc_t::add_token( spell_id, token, dbc.ptr );

  return ( dbc.spell( spell_id ) );
}

/*
 * Tries to find spell data by name, by going through various spell lists in following order:
 *
 * class spell, specialization spell, mastery spell, talent spell, glyphs spell,
 * racial spell, pet_spell
 */

const spell_data_t* player_t::find_spell( const std::string& name, const std::string& token, specialization_e s )
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

// player_t::find_racial_spell ============================================

const spell_data_t* player_t::find_racial_spell( const std::string& name, const std::string& token, race_e r )
{
  unsigned spell_id = dbc.race_ability_id( type, ( r != RACE_NONE ) ? r : race, name.c_str() );

  if ( ! spell_id || ! dbc.spell( spell_id ) )
  {
    return spell_data_t::not_found();
  }

  dbc_t::add_token( spell_id, token, dbc.ptr );

  return ( dbc.spell( spell_id ) );
}

// player_t::find_class_spell =============================================

const spell_data_t* player_t::find_class_spell( const std::string& name, const std::string& token, specialization_e s )
{
  unsigned spell_id = dbc.class_ability_id( type, _spec, name.c_str() );

  if ( s != SPEC_NONE && s != _spec )
    return spell_data_t::not_found();

  if ( ! spell_id || ! dbc.spell( spell_id ) || ( ( int )dbc.spell( spell_id ) -> level() > level ) )
  {
    return spell_data_t::not_found();
  }

  dbc_t::add_token( spell_id, token, dbc.ptr );

  return ( dbc.spell( spell_id ) );
}

// player_t::find_pet_spell =============================================

const spell_data_t* player_t::find_pet_spell( const std::string& name, const std::string& token )
{
  unsigned spell_id = dbc.pet_ability_id( type, name.c_str() );

  if ( ! spell_id || ! dbc.spell( spell_id ) )
  {
    return spell_data_t::not_found();
  }

  dbc_t::add_token( spell_id, token, dbc.ptr );

  return ( dbc.spell( spell_id ) );
}

// player_t::find_spell =============================================

const spell_data_t* player_t::find_spell( const unsigned int id, const std::string& token )
{
  if ( ! id || ! dbc.spell( id ) || ! dbc.spell( id ) -> id() || ( ( int )dbc.spell( id ) -> level() > level ) )
    return ( spell_data_t::not_found() );

  dbc_t::add_token( id, token, dbc.ptr );

  return ( dbc.spell( id ) );
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
      virtual double evaluate() { return player.composite_player_multiplier( action.school, &action ); }
    };
    return new multiplier_expr_t( *this, a );
  }
  if ( name_str == "in_combat" )
    return make_ref_expr( "in_combat", in_combat );
  if ( name_str == "attack_haste" )
    return make_mem_fn_expr( name_str, *this, &player_t::composite_attack_haste );
  if ( name_str == "attack_speed" )
    return make_mem_fn_expr( name_str, *this, &player_t::composite_attack_speed );
  if ( name_str == "spell_haste" )
    return make_mem_fn_expr( name_str, *this, &player_t::composite_spell_haste );
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


  if ( expr_t* q = create_resource_expression( name_str ) )
    return q;

  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, name_str, "." );

  if ( splits[ 0 ] == "race" && num_splits == 2 )
  {
    struct race_expr_t : public expr_t
    {
      player_t& player;
      std::string race_name;
      race_expr_t( player_t& p, const std::string& n ) :
        expr_t( "race" ), player( p ), race_name( n )
      {
        util::tokenize( race_name );
      }
      virtual double evaluate() { return player.race_str == race_name; }
    };
    return new race_expr_t( *this, splits[ 1 ] );
  }

  if ( splits[ 0 ] == "pet" && num_splits == 3 )
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

  else if ( splits[ 0 ] == "temporary_bonus" && num_splits == 2 )
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

  else if ( splits[ 0 ] == "stat" && num_splits == 2 )
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
        { return player.current.attribute[ attr ] * player.composite_attribute_multiplier( attr ); }
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
          return player.composite_spell_power( SCHOOL_MAX ) * player.composite_spell_power_multiplier();
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
          return player.composite_attack_power() * player.composite_attack_power_multiplier();
        }
      };
      return new ap_expr_t( name_str, *this );
    }

    case STAT_EXPERTISE_RATING: return make_ref_expr( name_str, stats.expertise_rating );
    case STAT_HIT_RATING:       return make_ref_expr( name_str, stats.hit_rating );
    case STAT_CRIT_RATING:      return make_ref_expr( name_str, stats.crit_rating );
    case STAT_HASTE_RATING:     return make_ref_expr( name_str, current.haste_rating );
    case STAT_ARMOR:            return make_ref_expr( name_str, current.armor );
    case STAT_DODGE_RATING:     return make_ref_expr( name_str, stats.dodge_rating );
    case STAT_PARRY_RATING:     return make_ref_expr( name_str, stats.parry_rating );
    case STAT_BLOCK_RATING:     return make_ref_expr( name_str, stats.block_rating );
    case STAT_MASTERY_RATING:   return make_ref_expr( name_str, current.mastery_rating );
    default: break;
    }

    // FIXME: report error and return?
  }

  else if ( num_splits == 3 )
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
  else if ( num_splits == 2 )
  {
    if ( splits[ 0 ] == "set_bonus" )
      return set_bonus.create_expression( this, splits[ 1 ] );
  }

  if ( num_splits >= 2 && splits[ 0 ] == "target" )
  {
    std::string rest = splits[1];
    for ( int i = 2; i < num_splits; ++i )
      rest += '.' + splits[i];
    return target -> create_expression( a, rest );
  }

  else if ( ( num_splits == 3 ) && ( ( splits[ 0 ] == "glyph" ) || ( splits[ 0 ] == "talent" ) ) )
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
      s = const_cast< spell_data_t* >( find_talent_spell( splits[ 1 ] ) );
    }

    return new s_expr_t( name_str, *this, s );
  }
  else if ( ( num_splits == 3 && splits[ 0 ] == "action" ) || splits[ 0 ] == "in_flight" || splits[ 0 ] == "in_flight_to_target" )
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
              if ( action_list[ i ] -> travel_events.size() > 0 )
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
              for ( size_t j = 0; j < action_list[ i ] -> travel_events.size(); j++ )
              {
                stateless_travel_event_t* te = debug_cast<stateless_travel_event_t*>( action_list[ i ] -> travel_events[ j ] );
                if ( te -> state -> target == action.target ) return true;
              }
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

  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, name_str, "." );
  if ( num_splits <= 0 )
    return 0;

  resource_e r = util::parse_resource_type( splits[ 0 ] );
  if ( r == RESOURCE_NONE )
    return 0;

  if ( num_splits == 1 )
    return make_ref_expr( name_str, resources.current[ r ] );

  if ( num_splits == 2 )
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
    profile_str += "origin=\"" + origin_str + '"' + term;
    profile_str += "level=" + util::to_string( level ) + term;
    profile_str += "race=" + race_str + term;
    profile_str += "spec=";
    profile_str += util::specialization_string( specialization() ) + term;
    profile_str += "role=";
    profile_str += util::role_type_string( primary_role() ) + term;
    profile_str += "position=" + position_str + term;

    if ( professions_str.size() > 0 )
    {
      profile_str += "professions=" + professions_str + term;
    };
  }

  if ( stype == SAVE_ALL || stype == SAVE_TALENTS )
  {
    if ( talents_str.size() > 0 )
    {
      recreate_talent_str( sim -> talent_format );
      profile_str += "talents=" + talents_str + term;
    };

    if ( talent_overrides_str.size() > 0 )
    {
      std::vector<std::string> splits;
      size_t num_splits = util::string_split( splits, talent_overrides_str, "/" );
      for ( size_t i = 0; i < num_splits; i++ )
      {
        profile_str += "talent_override=" + splits[ i ] + term;
      }
    }

    if ( glyphs_str.size() > 0 )
    {
      profile_str += "glyphs=" + glyphs_str + term;
    }
  }

  if ( stype == SAVE_ALL || stype == SAVE_ACTIONS )
  {
    if ( action_list_str.size() > 0 )
    {
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
          profile_str += "\n";
        }
        profile_str += "actions";
        if ( ! a -> action_list.empty() && a -> action_list != "default" )
          profile_str += "." + a -> action_list;
        profile_str += j ? "+=/" : "=";
        std::string encoded_action = a -> signature_str;
        if ( save_html )
          util::encode_html( encoded_action );
        profile_str += encoded_action + term;
        j++;
      }
    }
  }

  if ( stype == SAVE_ALL || stype == SAVE_GEAR )
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
        profile_str += "=" + item.options_str + term;
        if ( sim -> save_gear_comments && ! item.comment_str.empty() ) profile_str += item.comment_str + term;
      }
    }
    if ( ! items_str.empty() )
    {
      profile_str += "items=" + items_str + term;
    }

    profile_str += "\n# Gear Summary" + term;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      double value = initial_stats.get_stat( i );
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
      if ( item.unique || item.unique_enchant || item.unique_addon || ! item.encoded_weapon_str.empty() )
      {
        profile_str += "# ";
        profile_str += item.slot_name();
        profile_str += "=";
        profile_str += item.name();
        if ( item.heroic() ) profile_str += ",heroic=1";
        if ( ! item.encoded_weapon_str.empty() ) profile_str += ",weapon=" + item.encoded_weapon_str;
        if ( item.unique_enchant ) profile_str += ",enchant=" + item.encoded_enchant_str;
        if ( item.unique_addon   ) profile_str += ",addon="   + item.encoded_addon_str;
        profile_str += term;
      }
    }

    if ( enchant.attribute[ ATTR_STRENGTH  ] != 0 )  profile_str += "enchant_strength="         + util::to_string( enchant.attribute[ ATTR_STRENGTH  ] ) + term;
    if ( enchant.attribute[ ATTR_AGILITY   ] != 0 )  profile_str += "enchant_agility="          + util::to_string( enchant.attribute[ ATTR_AGILITY   ] ) + term;
    if ( enchant.attribute[ ATTR_STAMINA   ] != 0 )  profile_str += "enchant_stamina="          + util::to_string( enchant.attribute[ ATTR_STAMINA   ] ) + term;
    if ( enchant.attribute[ ATTR_INTELLECT ] != 0 )  profile_str += "enchant_intellect="        + util::to_string( enchant.attribute[ ATTR_INTELLECT ] ) + term;
    if ( enchant.attribute[ ATTR_SPIRIT    ] != 0 )  profile_str += "enchant_spirit="           + util::to_string( enchant.attribute[ ATTR_SPIRIT    ] ) + term;
    if ( enchant.spell_power                 != 0 )  profile_str += "enchant_spell_power="      + util::to_string( enchant.spell_power ) + term;
    if ( enchant.mp5                         != 0 )  profile_str += "enchant_mp5="              + util::to_string( enchant.mp5 ) + term;
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

  for ( size_t i = 0; i < items.size(); i++ )
  {
    items[ i ] = source -> items[ i ];
    items[ i ].player = this;
  }

  gear = source -> gear;
  enchant = source -> enchant;
}

// class_modules::create::options =================================================

void player_t::create_options()
{
  option_t player_options[] =
  {
    // General
    { "name",                                 OPT_STRING,   &( name_str                               ) },
    { "origin",                               OPT_STRING,   &( origin_str                             ) },
    { "region",                               OPT_STRING,   &( region_str                             ) },
    { "server",                               OPT_STRING,   &( server_str                             ) },
    { "id",                                   OPT_STRING,   &( id_str                                 ) },
    { "talents",                              OPT_FUNC,     ( void* ) ::parse_talent_url                },
    { "talent_override",                      OPT_FUNC,     ( void* ) ::parse_talent_override           },
    { "glyphs",                               OPT_STRING,   &( glyphs_str                             ) },
    { "race",                                 OPT_STRING,   &( race_str                               ) },
    { "level",                                OPT_INT,      &( level                                  ) },
    { "ready_trigger",                        OPT_INT,      &( ready_type                             ) },
    { "role",                                 OPT_FUNC,     ( void* ) ::parse_role_string               },
    { "target",                               OPT_STRING,   &( target_str                             ) },
    { "skill",                                OPT_FLT,      &( initial.skill                          ) },
    { "distance",                             OPT_FLT,      &( current.distance                       ) },
    { "position",                             OPT_STRING,   &( position_str                           ) },
    { "professions",                          OPT_STRING,   &( professions_str                        ) },
    { "actions",                              OPT_STRING,   &( action_list_str                        ) },
    { "actions+",                             OPT_APPEND,   &( action_list_str                        ) },
    { "actions.",                             OPT_MAP,      &( alist_map                              ) },
    { "action_list",                          OPT_STRING,   &( choose_action_list                     ) },
    { "sleeping",                             OPT_BOOL,     &( initial.sleeping                       ) },
    { "quiet",                                OPT_BOOL,     &( quiet                                  ) },
    { "save",                                 OPT_STRING,   &( report_information.save_str            ) },
    { "save_gear",                            OPT_STRING,   &( report_information.save_gear_str       ) },
    { "save_talents",                         OPT_STRING,   &( report_information.save_talents_str    ) },
    { "save_actions",                         OPT_STRING,   &( report_information.save_actions_str    ) },
    { "comment",                              OPT_STRING,   &( report_information.comment_str         ) },
    { "bugs",                                 OPT_BOOL,     &( bugs                                   ) },
    { "world_lag",                            OPT_FUNC,     ( void* ) ::parse_world_lag                 },
    { "world_lag_stddev",                     OPT_FUNC,     ( void* ) ::parse_world_lag_stddev          },
    { "brain_lag",                            OPT_FUNC,     ( void* ) ::parse_brain_lag                 },
    { "brain_lag_stddev",                     OPT_FUNC,     ( void* ) ::parse_brain_lag_stddev          },
    { "scale_player",                         OPT_BOOL,     &( scale_player                           ) },
    { "spec",                                 OPT_FUNC,     ( void* ) ::parse_specialization            },
    { "specialization",                       OPT_FUNC,     ( void* ) ::parse_specialization            },
    // Items
    { "meta_gem",                             OPT_STRING,   &( meta_gem_str                           ) },
    { "items",                                OPT_STRING,   &( items_str                              ) },
    { "items+",                               OPT_APPEND,   &( items_str                              ) },
    { "head",                                 OPT_STRING,   &( items[ SLOT_HEAD      ].options_str    ) },
    { "neck",                                 OPT_STRING,   &( items[ SLOT_NECK      ].options_str    ) },
    { "shoulders",                            OPT_STRING,   &( items[ SLOT_SHOULDERS ].options_str    ) },
    { "shoulder",                             OPT_STRING,   &( items[ SLOT_SHOULDERS ].options_str    ) },
    { "shirt",                                OPT_STRING,   &( items[ SLOT_SHIRT     ].options_str    ) },
    { "chest",                                OPT_STRING,   &( items[ SLOT_CHEST     ].options_str    ) },
    { "waist",                                OPT_STRING,   &( items[ SLOT_WAIST     ].options_str    ) },
    { "legs",                                 OPT_STRING,   &( items[ SLOT_LEGS      ].options_str    ) },
    { "leg",                                  OPT_STRING,   &( items[ SLOT_LEGS      ].options_str    ) },
    { "feet",                                 OPT_STRING,   &( items[ SLOT_FEET      ].options_str    ) },
    { "foot",                                 OPT_STRING,   &( items[ SLOT_FEET      ].options_str    ) },
    { "wrists",                               OPT_STRING,   &( items[ SLOT_WRISTS    ].options_str    ) },
    { "wrist",                                OPT_STRING,   &( items[ SLOT_WRISTS    ].options_str    ) },
    { "hands",                                OPT_STRING,   &( items[ SLOT_HANDS     ].options_str    ) },
    { "hand",                                 OPT_STRING,   &( items[ SLOT_HANDS     ].options_str    ) },
    { "finger1",                              OPT_STRING,   &( items[ SLOT_FINGER_1  ].options_str    ) },
    { "finger2",                              OPT_STRING,   &( items[ SLOT_FINGER_2  ].options_str    ) },
    { "ring1",                                OPT_STRING,   &( items[ SLOT_FINGER_1  ].options_str    ) },
    { "ring2",                                OPT_STRING,   &( items[ SLOT_FINGER_2  ].options_str    ) },
    { "trinket1",                             OPT_STRING,   &( items[ SLOT_TRINKET_1 ].options_str    ) },
    { "trinket2",                             OPT_STRING,   &( items[ SLOT_TRINKET_2 ].options_str    ) },
    { "back",                                 OPT_STRING,   &( items[ SLOT_BACK      ].options_str    ) },
    { "main_hand",                            OPT_STRING,   &( items[ SLOT_MAIN_HAND ].options_str    ) },
    { "off_hand",                             OPT_STRING,   &( items[ SLOT_OFF_HAND  ].options_str    ) },
    { "tabard",                               OPT_STRING,   &( items[ SLOT_TABARD    ].options_str    ) },
    // Set Bonus
    { "tier13_2pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T13_2PC_CASTER ]  ) },
    { "tier13_4pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T13_4PC_CASTER ]  ) },
    { "tier13_2pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T13_2PC_MELEE ]   ) },
    { "tier13_4pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T13_4PC_MELEE ]   ) },
    { "tier13_2pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T13_2PC_TANK ]    ) },
    { "tier13_4pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T13_4PC_TANK ]    ) },
    { "tier13_2pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T13_2PC_HEAL ]    ) },
    { "tier13_4pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T13_4PC_HEAL ]    ) },
    { "tier14_2pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T14_2PC_CASTER ]  ) },
    { "tier14_4pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T14_4PC_CASTER ]  ) },
    { "tier14_2pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T14_2PC_MELEE ]   ) },
    { "tier14_4pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T14_4PC_MELEE ]   ) },
    { "tier14_2pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T14_2PC_TANK ]    ) },
    { "tier14_4pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T14_4PC_TANK ]    ) },
    { "tier14_2pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T14_2PC_HEAL ]    ) },
    { "tier14_4pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T14_4PC_HEAL ]    ) },
    { "tier15_2pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T15_2PC_CASTER ]  ) },
    { "tier15_4pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T15_4PC_CASTER ]  ) },
    { "tier15_2pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T15_2PC_MELEE ]   ) },
    { "tier15_4pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T15_4PC_MELEE ]   ) },
    { "tier15_2pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T15_2PC_TANK ]    ) },
    { "tier15_4pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T15_4PC_TANK ]    ) },
    { "tier15_2pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T15_2PC_HEAL ]    ) },
    { "tier15_4pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T15_4PC_HEAL ]    ) },
    { "tier16_2pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T16_2PC_CASTER ]  ) },
    { "tier16_4pc_caster",                    OPT_BOOL,     &( set_bonus.count[ SET_T16_4PC_CASTER ]  ) },
    { "tier16_2pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T16_2PC_MELEE ]   ) },
    { "tier16_4pc_melee",                     OPT_BOOL,     &( set_bonus.count[ SET_T16_4PC_MELEE ]   ) },
    { "tier16_2pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T16_2PC_TANK ]    ) },
    { "tier16_4pc_tank",                      OPT_BOOL,     &( set_bonus.count[ SET_T16_4PC_TANK ]    ) },
    { "tier16_2pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T16_2PC_HEAL ]    ) },
    { "tier16_4pc_heal",                      OPT_BOOL,     &( set_bonus.count[ SET_T16_4PC_HEAL ]    ) },
    { "pvp_2pc_caster",                       OPT_BOOL,     &( set_bonus.count[ SET_PVP_2PC_CASTER ]  ) },
    { "pvp_4pc_caster",                       OPT_BOOL,     &( set_bonus.count[ SET_PVP_4PC_CASTER ]  ) },
    { "pvp_2pc_melee",                        OPT_BOOL,     &( set_bonus.count[ SET_PVP_2PC_MELEE ]   ) },
    { "pvp_4pc_melee",                        OPT_BOOL,     &( set_bonus.count[ SET_PVP_4PC_MELEE ]   ) },
    { "pvp_2pc_tank",                         OPT_BOOL,     &( set_bonus.count[ SET_PVP_2PC_TANK ]    ) },
    { "pvp_4pc_tank",                         OPT_BOOL,     &( set_bonus.count[ SET_PVP_4PC_TANK ]    ) },
    { "pvp_2pc_heal",                         OPT_BOOL,     &( set_bonus.count[ SET_PVP_2PC_HEAL ]    ) },
    { "pvp_4pc_heal",                         OPT_BOOL,     &( set_bonus.count[ SET_PVP_4PC_HEAL ]    ) },
    // Gear Stats
    { "gear_strength",                        OPT_FLT,  &( gear.attribute[ ATTR_STRENGTH  ]           ) },
    { "gear_agility",                         OPT_FLT,  &( gear.attribute[ ATTR_AGILITY   ]           ) },
    { "gear_stamina",                         OPT_FLT,  &( gear.attribute[ ATTR_STAMINA   ]           ) },
    { "gear_intellect",                       OPT_FLT,  &( gear.attribute[ ATTR_INTELLECT ]           ) },
    { "gear_spirit",                          OPT_FLT,  &( gear.attribute[ ATTR_SPIRIT    ]           ) },
    { "gear_spell_power",                     OPT_FLT,  &( gear.spell_power                           ) },
    { "gear_mp5",                             OPT_FLT,  &( gear.mp5                                   ) },
    { "gear_attack_power",                    OPT_FLT,  &( gear.attack_power                          ) },
    { "gear_expertise_rating",                OPT_FLT,  &( gear.expertise_rating                      ) },
    { "gear_haste_rating",                    OPT_FLT,  &( gear.haste_rating                          ) },
    { "gear_hit_rating",                      OPT_FLT,  &( gear.hit_rating                            ) },
    { "gear_crit_rating",                     OPT_FLT,  &( gear.crit_rating                           ) },
    { "gear_parry_rating",                    OPT_FLT,  &( gear.parry_rating                          ) },
    { "gear_dodge_rating",                    OPT_FLT,  &( gear.dodge_rating                          ) },
    { "gear_health",                          OPT_FLT,  &( gear.resource[ RESOURCE_HEALTH ]           ) },
    { "gear_mana",                            OPT_FLT,  &( gear.resource[ RESOURCE_MANA   ]           ) },
    { "gear_rage",                            OPT_FLT,  &( gear.resource[ RESOURCE_RAGE   ]           ) },
    { "gear_energy",                          OPT_FLT,  &( gear.resource[ RESOURCE_ENERGY ]           ) },
    { "gear_focus",                           OPT_FLT,  &( gear.resource[ RESOURCE_FOCUS  ]           ) },
    { "gear_runic",                           OPT_FLT,  &( gear.resource[ RESOURCE_RUNIC_POWER  ]     ) },
    { "gear_armor",                           OPT_FLT,  &( gear.armor                                 ) },
    { "gear_mastery_rating",                  OPT_FLT,  &( gear.mastery_rating                        ) },
    // Stat Enchants
    { "enchant_strength",                     OPT_FLT,  &( enchant.attribute[ ATTR_STRENGTH  ]        ) },
    { "enchant_agility",                      OPT_FLT,  &( enchant.attribute[ ATTR_AGILITY   ]        ) },
    { "enchant_stamina",                      OPT_FLT,  &( enchant.attribute[ ATTR_STAMINA   ]        ) },
    { "enchant_intellect",                    OPT_FLT,  &( enchant.attribute[ ATTR_INTELLECT ]        ) },
    { "enchant_spirit",                       OPT_FLT,  &( enchant.attribute[ ATTR_SPIRIT    ]        ) },
    { "enchant_spell_power",                  OPT_FLT,  &( enchant.spell_power                        ) },
    { "enchant_mp5",                          OPT_FLT,  &( enchant.mp5                                ) },
    { "enchant_attack_power",                 OPT_FLT,  &( enchant.attack_power                       ) },
    { "enchant_expertise_rating",             OPT_FLT,  &( enchant.expertise_rating                   ) },
    { "enchant_armor",                        OPT_FLT,  &( enchant.armor                              ) },
    { "enchant_haste_rating",                 OPT_FLT,  &( enchant.haste_rating                       ) },
    { "enchant_hit_rating",                   OPT_FLT,  &( enchant.hit_rating                         ) },
    { "enchant_crit_rating",                  OPT_FLT,  &( enchant.crit_rating                        ) },
    { "enchant_mastery_rating",               OPT_FLT,  &( enchant.mastery_rating                     ) },
    { "enchant_health",                       OPT_FLT,  &( enchant.resource[ RESOURCE_HEALTH ]        ) },
    { "enchant_mana",                         OPT_FLT,  &( enchant.resource[ RESOURCE_MANA   ]        ) },
    { "enchant_rage",                         OPT_FLT,  &( enchant.resource[ RESOURCE_RAGE   ]        ) },
    { "enchant_energy",                       OPT_FLT,  &( enchant.resource[ RESOURCE_ENERGY ]        ) },
    { "enchant_focus",                        OPT_FLT,  &( enchant.resource[ RESOURCE_FOCUS  ]        ) },
    { "enchant_runic",                        OPT_FLT,  &( enchant.resource[ RESOURCE_RUNIC_POWER  ]        ) },
    // Regen
    { "infinite_energy",                      OPT_BOOL,   &( resources.infinite_resource[ RESOURCE_ENERGY ]     ) },
    { "infinite_focus",                       OPT_BOOL,   &( resources.infinite_resource[ RESOURCE_FOCUS  ]     ) },
    { "infinite_health",                      OPT_BOOL,   &( resources.infinite_resource[ RESOURCE_HEALTH ]     ) },
    { "infinite_mana",                        OPT_BOOL,   &( resources.infinite_resource[ RESOURCE_MANA   ]     ) },
    { "infinite_rage",                        OPT_BOOL,   &( resources.infinite_resource[ RESOURCE_RAGE   ]     ) },
    { "infinite_runic",                       OPT_BOOL,   &( resources.infinite_resource[ RESOURCE_RUNIC_POWER  ]     ) },
    // Misc
    { "autounshift",                          OPT_BOOL,   &( autoUnshift                              ) },
    { "dtr_proc_chance",                      OPT_FLT,    &( dtr_proc_chance                          ) },
    { "skip_actions",                         OPT_STRING, &( action_list_skip                         ) },
    { "modify_action",                        OPT_STRING, &( modify_action                            ) },
    { "elixirs",                              OPT_STRING, &( elixirs_str                              ) },
    { "flask",                                OPT_STRING, &( flask_str                                ) },
    { "food",                                 OPT_STRING, &( food_str                                 ) },
    { "player_resist_holy",                   OPT_INT,    &( spell_resistance[ SCHOOL_HOLY   ]        ) },
    { "player_resist_shadow",                 OPT_INT,    &( spell_resistance[ SCHOOL_SHADOW ]        ) },
    { "player_resist_arcane",                 OPT_INT,    &( spell_resistance[ SCHOOL_ARCANE ]        ) },
    { "player_resist_frost",                  OPT_INT,    &( spell_resistance[ SCHOOL_FROST  ]        ) },
    { "player_resist_fire",                   OPT_INT,    &( spell_resistance[ SCHOOL_FIRE   ]        ) },
    { "player_resist_nature",                 OPT_INT,    &( spell_resistance[ SCHOOL_NATURE ]        ) },
    { "reaction_time_mean",                   OPT_TIMESPAN, &( reaction_mean                          ) },
    { "reaction_time_stddev",                 OPT_TIMESPAN, &( reaction_stddev                        ) },
    { "reaction_time_nu",                     OPT_TIMESPAN, &( reaction_nu                            ) },
    { "reaction_time_offset",                 OPT_TIMESPAN, &( reaction_offset                        ) },
    { "simple_actions",                       OPT_BOOL,   &( simple_actions                           ) },
    { NULL, OPT_UNKNOWN, NULL }
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
                         sample_data_t& dps,
                         std::vector<double>& dps_convergence_error,
                         double dps_error,
                         double& dps_convergence )
{
  // Error Convergence ======================================================

  int    convergence_iterations = 0;
  double convergence_dps = 0;
  double convergence_min = +1.0E+50;
  double convergence_max = -1.0E+50;
  double convergence_std_dev = 0;

  if ( dps.data().size() > 1 && convergence_scale > 1 && !dps.simple )
  {
    for ( unsigned int i = 0; i < dps.data().size(); i += convergence_scale )
    {
      double i_dps = dps.data()[ i ];
      convergence_dps += i_dps;
      if ( convergence_min > i_dps ) convergence_min = i_dps;
      if ( convergence_max < i_dps ) convergence_max = i_dps;
    }
    convergence_iterations = ( dps.data().size() + convergence_scale - 1 ) / convergence_scale;
    convergence_dps /= convergence_iterations;

    assert( dps_convergence_error.empty() );
    dps_convergence_error.reserve( dps.data().size() );

    double sum_of_squares = 0;

    for ( unsigned int i = 0; i < dps.data().size(); i++ )
    {
      double delta = dps.data()[ i ] - convergence_dps;
      double delta_squared = delta * delta;

      sum_of_squares += delta_squared;

      if ( i > 1 )
        dps_convergence_error.push_back( confidence_estimator * sqrt( sum_of_squares / i ) / sqrt( ( float ) i ) );

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

  // Sample Data Analysis ========================================================

  // sample_data_t::analyze(calc_basics,calc_variance,sort )

  deaths.analyze();

  fight_length.analyze();
  waiting_time.analyze();
  executed_foreground_actions.analyze();

  dmg.analyze();
  compound_dmg.analyze();
  dps.analyze();
  dpse.analyze();

  dmg_taken.analyze();
  dtps.analyze();

  heal.analyze();
  compound_heal.analyze();
  hps.analyze();
  hpse.analyze();

  heal_taken.analyze();
  htps.analyze();

  deaths_error =  deaths.mean_std_dev * s.confidence_estimator;
  dps_error =  dps.mean_std_dev * s.confidence_estimator;
  dtps_error =  dtps.mean_std_dev * s.confidence_estimator;
  hps_error =  hps.mean_std_dev * s.confidence_estimator;

  for ( size_t i = 0; i <  buff_list.size(); ++i )
    buff_list[ i ] -> analyze();

  for ( size_t i = 0; i < benefit_list.size(); ++i )
    benefit_list[ i ] -> analyze();

  for ( size_t i = 0; i < uptime_list.size(); ++i )
    uptime_list[ i ] -> analyze();

  range::sort(  stats_list, compare_stats_name() );

  if (  quiet ) return;
  if (  fight_length.mean == 0 ) return;

  // Pet Chart Adjustment ===================================================
  size_t max_buckets = static_cast<size_t>(  fight_length.max );

  // Make the pet graphs the same length as owner's
  if (  is_pet() )
  {
    player_t* o =  cast_pet() -> owner;
    max_buckets = static_cast<size_t>( o -> fight_length.max );
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
        s -> portion_amount =  compound_dmg.mean ? s -> actual_amount.mean /  compound_dmg.mean : 0 ;
      else
        s -> portion_amount =  compound_heal.mean ? s -> actual_amount.mean /  compound_heal.mean : 0;
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


  // Resources & Gains ======================================================
  for ( size_t i = 0; i <  resource_timeline_count; ++i )
  {
    std::vector<double>& timeline =  resource_timelines[ i ].timeline;
    if ( timeline.size() > max_buckets ) timeline.resize( max_buckets );

    assert( timeline.size() == max_buckets );
    for ( size_t j = 0; j < max_buckets; j++ )
    {
      timeline[ j ] /= s.divisor_timeline[ j ];
    }
  }

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    resource_lost  [ i ] /= s.iterations;
    resource_gained[ i ] /= s.iterations;
  }

  double rl = resource_lost[  primary_resource() ];
  dpr = ( rl > 0 ) ? (  dmg.mean / rl ) : -1.0;
  hpr = ( rl > 0 ) ? (  heal.mean / rl ) : -1.0;

  rps_loss = resource_lost  [  primary_resource() ] /  fight_length.mean;
  rps_gain = resource_gained[  primary_resource() ] /  fight_length.mean;


  for ( size_t i = 0; i < resources.combat_end_resource.size(); ++i )
    resources.combat_end_resource[ i ].analyze();

  for ( size_t i = 0; i < gain_list.size(); ++i )
    gain_list[ i ] -> analyze( s );

  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_t* pet =  pet_list[ i ];
    for ( size_t i = 0; i < pet -> gain_list.size(); ++i )
      pet -> gain_list[ i ] -> analyze( s );
  }

  // Procs ==================================================================

  for ( size_t i = 0; i < proc_list.size(); ++i )
    proc_list[ i ] -> analyze();

  // Damage Timelines =======================================================

  timeline_dmg.assign( max_buckets, 0 );
  for ( size_t i = 0, is_hps = ( primary_role() == ROLE_HEAL ); i < num_stats; i++ )
  {
    stats_t* s = tmp_stats_list[ i ];
    if ( ( s -> type != STATS_DMG ) == is_hps )
    {
      size_t j_max = std::min( max_buckets, s -> timeline_amount.size() );
      for ( size_t j = 0; j < j_max; j++ )
        timeline_dmg[ j ] += s -> timeline_amount[ j ];
    }
  }


  recreate_talent_str( s.talent_format );

  // Error Convergence ======================================================
  player_convergence( s.convergence_scale, s.confidence_estimator,
                      dps,  dps_convergence_error,  dps_error,  dps_convergence );

}

// Return sample_data reference over which this player gets scaled ( scale factors, reforge plots, etc. )
// By default this will be his personal dps or hps

const sample_data_t& player_t::scales_over()
{
  const std::string& so = sim -> scaling -> scale_over;

  if ( so == "raid_dps"       ) return sim -> raid_dps;
  if ( so == "raid_hps"       ) return sim -> raid_hps;

  player_t* q = 0;
  if ( ! sim -> scaling -> scale_over_player.empty() )
    q = sim -> find_player( sim -> scaling -> scale_over_player );
  if ( !q )
    q = this;

  if ( so == "dmg_taken"      ) return q -> dmg_taken;

  if ( so == "dps" )
    return q -> dps;

  if ( so == "dpse" )
    return q -> dpse;

  if ( so == "hpse" )
    return q -> hpse;

  if ( so == "htps" )
    return q -> htps;

  if ( q -> primary_role() == ROLE_HEAL || so =="hps" )
    return q -> hps;

  if ( q -> primary_role() == ROLE_TANK || so =="dtps" )
    return q -> dtps;

  return q -> dps;
}

// Change the player position ( fron/back, etc. ) and update attack hit table

void player_t::change_position( position_e new_pos )
{
  if ( sim -> debug )
    sim -> output( "%s changes position from %s to %s.", name(), util::position_type_string( position() ), util::position_type_string( new_pos ) );

  current.position = new_pos;
}

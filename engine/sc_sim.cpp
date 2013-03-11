// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

// parse_ptr ================================================================

static bool parse_ptr( sim_t*             sim,
                       const std::string& name,
                       const std::string& value )
{
  if ( name != "ptr" ) return false;

  if ( SC_USE_PTR )
    sim -> dbc.ptr = util::str_to_num<int>( value ) != 0;
  else
    sim -> errorf( "SimulationCraft has not been built with PTR data.  The 'ptr=' option is ignored.\n" );

  return true;
}

// parse_active =============================================================

static bool parse_active( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{
  if ( name != "active" ) return false;

  if ( value == "owner" )
  {
    sim -> active_player = sim -> active_player -> cast_pet() -> owner;
  }
  else if ( value == "none" || value == "0" )
  {
    sim -> active_player = 0;
  }
  else
  {
    if ( sim -> active_player )
    {
      sim -> active_player = sim -> active_player -> find_pet( value );
    }
    if ( ! sim -> active_player )
    {
      sim -> active_player = sim -> find_player( value );
    }
    if ( ! sim -> active_player )
    {
      sim -> errorf( "Unable to find player %s to make active.\n", value.c_str() );
      return false;
    }
  }

  return true;
}

// parse_optimal_raid =======================================================

static bool parse_optimal_raid( sim_t*             sim,
                                const std::string& name,
                                const std::string& value )
{
  if ( name != "optimal_raid" ) return false;

  sim -> use_optimal_buffs_and_debuffs( util::str_to_num<int>( value ) );

  return true;
}

// parse_player =============================================================

static bool parse_player( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{

  if ( name[ 0 ] >= '0' && name[ 0 ] <= '9' )
  {
    sim -> errorf( "Invalid actor name %s - name cannot start with a digit.", name.c_str() );
    return false;
  }

  if ( name == "pet" || name == "guardian" )
  {
    std::string::size_type cut_pt = value.find( ',' );
    std::string pet_type( value, 0, cut_pt );

    std::string pet_name;
    if ( cut_pt != value.npos )
      pet_name.assign( value, cut_pt + 1, value.npos );
    else
      pet_name = value;

    if ( ! sim -> active_player )
    {
      sim -> errorf( "Pet profile ( name %s ) needs a player preceding it.", name.c_str() );
      return false;
    }

    sim -> active_player = sim -> active_player -> create_pet( pet_name, pet_type );
  }
  else if ( name == "copy" )
  {
    std::string::size_type cut_pt = value.find( ',' );
    std::string player_name( value, 0, cut_pt );

    player_t* source;
    if ( cut_pt == value.npos )
      source = sim -> active_player;
    else
      source = sim -> find_player( value.substr( cut_pt + 1 ) );

    if ( source == 0 )
    {
      sim -> errorf( "Invalid source for profile copy - format is copy=target[,source], source defaults to active player." );
      return false;
    }

    sim -> active_player = module_t::get( source -> type ) -> create_player( sim, player_name );
    if ( sim -> active_player != 0 ) sim -> active_player -> copy_from ( source );
  }
  else
  {
    sim -> active_player = 0;
    const module_t* module = module_t::get( name );

    if ( ! module || ! module -> valid() )
    {
      sim -> errorf( "\nModule for class %s is currently not available.\n", name.c_str() );
    }
    else
    {
      sim -> active_player = module -> create_player( sim, value );

      if ( ! sim -> active_player )
      {
        sim -> errorf( "\nUnable to create player %s with class %s.\n",
                       value.c_str(), name.c_str() );
      }
    }
  }

  // Create options for player
  if ( sim -> active_player )
    sim -> active_player -> create_options();

  return sim -> active_player != 0;
}

// parse_proxy ==============================================================

static bool parse_proxy( sim_t*             sim,
                         const std::string& /* name */,
                         const std::string& value )
{

  std::vector<std::string> splits = util::string_split( value, "," );

  if ( splits.size() != 3 )
  {
    sim -> errorf( "Expected format is: proxy=type,host,port\n" );
    return false;
  }

  unsigned port = util::str_to_num<unsigned>( splits[ 2 ] );
  if ( splits[ 0 ] == "http" && port > 0 && port < 65536 )
  {
    http::set_proxy( splits[ 0 ], splits[ 1 ], port );
    return true;
  }

  return false;
}

// parse_cache ==============================================================

static bool parse_cache( sim_t*             /* sim */,
                         const std::string& name,
                         const std::string& value )
{
  if ( name == "cache_players" )
  {
    if ( value == "1" ) cache::players( cache::ANY );
    else if ( value == "0" ) cache::players( cache::CURRENT );
    else if ( util::str_compare_ci( value, "only" ) ) cache::players( cache::ONLY );
    else return false;

    return true;
  }

  else if ( name == "cache_items" )
  {
    if ( value == "1" ) cache::items( cache::ANY );
    else if ( value == "0" ) cache::items( cache::CURRENT );
    else if ( util::str_compare_ci( value, "only" ) ) cache::items( cache::ONLY );
    else return false;

    return true;
  }

  else
    return false;


  return true;
}

// parse_talent_format ======================================================

bool parse_talent_format( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{
  if ( name != "talent_format" ) return false;

  if ( util::str_compare_ci( value, "unchanged" ) )
  {
    sim -> talent_format = TALENT_FORMAT_UNCHANGED;
  }
  else if ( util::str_compare_ci( value, "armory" ) )
  {
    sim -> talent_format = TALENT_FORMAT_ARMORY;
  }
  else if ( util::str_compare_ci( value, "wowhead" ) )
  {
    sim -> talent_format = TALENT_FORMAT_WOWHEAD;
  }
  else if ( util::str_compare_ci( value, "numbers" ) || util::str_compare_ci( value, "default" ) )
  {
    sim -> talent_format = TALENT_FORMAT_NUMBERS;
  }

  return true;
}


// parse_armory =============================================================

class names_and_options_t
{
private:
  bool is_valid_region( const std::string& s )
  { return s.size() == 2; }

public:
  struct error {};
  struct option_error : public error {};

  std::vector<std::string> names;
  std::string region;
  std::string server;
  cache::behavior_e cache;

  names_and_options_t( sim_t* sim, const std::string& context,
                       const option_t* client_options, const std::string& input )
  {
    int use_cache = 0;

    option_t base_options[] =
    {
      opt_string( "region", region ),
      opt_string( "server", server ),
      opt_bool( "cache", use_cache ),
      opt_null()
    };

    std::vector<option_t> options;
    option_t::merge( options, base_options, client_options );

    names = util::string_split( input, "," );

    std::vector<std::string> names2 = names;
    size_t count = 0;
    for ( size_t i = 0; i < names.size(); ++i )
    {
      if ( names[ i ].find( '=' ) != std::string::npos )
      {
        if ( unlikely( ! option_t::parse( sim, context.c_str(), options, names[ i ] ) ) )
        {
          throw option_error();
        }
      }
      else
      {
        names2[ count++ ] = names[ i ];
      }
    }

    names2.resize( count );
    names = names2;

    if ( region.empty() )
    {
      if ( names.size() > 2 && is_valid_region( names[ 0 ] ) )
      {
        region = names[ 0 ];
        server = names[ 1 ];
        names.erase( names.begin(), names.begin() + 2 );
      }
      else
      {
        region = sim -> default_region_str;
      }
    }
    if ( server.empty() )
    {
      if ( names.size() > 1 )
      {
        server = names[ 0 ];
        names.erase( names.begin(), names.begin() + 1 );
      }
      else
      {
        server = sim -> default_server_str;
      }
    }

    cache = use_cache ? cache::ANY : cache::players();
  }
};

bool parse_armory( sim_t*             sim,
                   const std::string& name,
                   const std::string& value )
{
  try
  {
    std::string spec = "active";

    option_t options[] =
    {
      opt_string( "spec", spec ),
      opt_null()
    };

    names_and_options_t stuff( sim, name, options, value );

    for ( size_t i = 0; i < stuff.names.size(); ++i )
    {
      // Format: name[|spec]
      std::string& player_name = stuff.names[ i ];
      std::string description = spec;

      if ( player_name[ 0 ] == '!' )
      {
        player_name.erase( 0, 1 );
        description = "inactive";
        sim -> errorf( "Warning: use of \"!%s\" to indicate a player's inactive talent spec is deprecated. Use \"%s,spec=inactive\" instead.\n",
                       player_name.c_str(), player_name.c_str() );
      }

      std::string::size_type pos = player_name.find( '|' );
      if ( pos != player_name.npos )
      {
        description.assign( player_name, pos + 1, player_name.npos );
        player_name.erase( pos );
      }

      player_t* p;
      if ( name == "wowhead" )
      {
        sim -> errorf( "Wowhead profiler currently not support. "
                       "Wowhead profiler does not provide spec, talent or glyph data.\n" );
        return false;

        //p = wowhead::download_player( sim, stuff.region, stuff.server, player_name, description, wowhead::LIVE, stuff.cache );
      }
      else if ( name == "chardev" || name == "mopdev" )
        p = chardev::download_player( sim, player_name, stuff.cache );
      else if ( name == "wowreforge" )
        p = wowreforge::download_player( sim, player_name, stuff.cache );
      else
        p = bcp_api::download_player( sim, stuff.region, stuff.server,
                                      player_name, description, stuff.cache );

      sim -> active_player = p;
      if ( ! p )
        return false;
    }
  }

  catch ( names_and_options_t::error& )
  { return false; }

  // Create options for player
  if ( sim -> active_player )
    sim -> active_player -> create_options();

  return sim -> active_player != 0;
}

bool parse_guild( sim_t*             sim,
                  const std::string& name,
                  const std::string& value )
{
  // Save Raid Summary file when guilds are downloaded
  sim -> save_raid_summary = 1;

  try
  {
    std::string type_str;
    std::string ranks_str;
    int max_rank = 0;

    option_t options[] =
    {
      opt_string( "class", type_str ),
      opt_int( "max_rank", max_rank ),
      opt_string( "ranks", ranks_str ),
      opt_null()
    };

    names_and_options_t stuff( sim, name, options, value );

    std::vector<int> ranks_list;
    if ( ! ranks_str.empty() )
    {
      std::vector<std::string> ranks = util::string_split( ranks_str, "/" );

      for ( size_t i = 0; i < ranks.size(); i++ )
        ranks_list.push_back( atoi( ranks[i].c_str() ) );
    }

    player_e pt = PLAYER_NONE;
    if ( ! type_str.empty() )
      pt = util::parse_player_type( type_str );

    for ( size_t i = 0; i < stuff.names.size(); ++i )
    {
      std::string& guild_name = stuff.names[ i ];
      if ( ! bcp_api::download_guild( sim, stuff.region, stuff.server, guild_name,
                                      ranks_list, pt, max_rank, stuff.cache ) )
        return false;
    }
  }

  catch ( names_and_options_t::error& )
  { return false; }

  return true;
}

// parse_rawr ===============================================================

static bool parse_rawr( sim_t*             sim,
                        const std::string& name,
                        const std::string& value )
{
  if ( name == "rawr" )
  {
    sim -> active_player = rawr::load_player( sim, value );
    if ( ! sim -> active_player )
    {
      sim -> errorf( "Unable to parse Rawr Character Save file '%s'\n", value.c_str() );
    }
  }

  return sim -> active_player != 0;
}

// parse_fight_style ========================================================

static bool parse_fight_style( sim_t*             sim,
                               const std::string& name,
                               const std::string& value )
{
  if ( name != "fight_style" ) return false;

  if ( util::str_compare_ci( value, "Patchwerk" ) )
  {
    sim -> fight_style = "Patchwerk";
    sim -> raid_events_str.clear();
  }
  else if ( util::str_compare_ci( value, "Ultraxion" ) )
  {
    sim -> fight_style = "Ultraxion";
    sim -> max_time    = timespan_t::from_seconds( 366.0 );
    sim -> fixed_time  = 1;
    sim -> vary_combat_length = 0.0;
    sim -> raid_events_str =  "flying,first=0,duration=500,cooldown=500";
    sim -> raid_events_str +=  "/position_switch,first=0,duration=500,cooldown=500";
    sim -> raid_events_str += "/stun,duration=1.0,first=45.0,period=45.0";
    sim -> raid_events_str += "/stun,duration=1.0,first=57.0,period=57.0";
    sim -> raid_events_str += "/damage,first=6.0,period=6.0,last=59.5,amount=44000,type=shadow";
    sim -> raid_events_str += "/damage,first=60.0,period=5.0,last=119.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=120.0,period=4.0,last=179.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=180.0,period=3.0,last=239.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=240.0,period=2.0,last=299.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=300.0,period=1.0,amount=44855,type=shadow";
  }
  else if ( util::str_compare_ci( value, "HelterSkelter" ) || util::str_compare_ci( value, "Helter_Skelter" ) )
  {
    sim -> fight_style = "HelterSkelter";
    sim -> raid_events_str = "casting,cooldown=30,duration=3,first=15";
    sim -> raid_events_str += "/movement,cooldown=30,duration=5";
    sim -> raid_events_str += "/stun,cooldown=60,duration=2";
    sim -> raid_events_str += "/invulnerable,cooldown=120,duration=3";
  }
  else if ( util::str_compare_ci( value, "LightMovement" ) )
  {
    sim -> fight_style = "LightMovement";
    sim -> raid_events_str = "/movement,players_only=1,first=";
    sim -> raid_events_str += util::to_string( int( sim -> max_time.total_seconds() * 0.1 ) );
    sim -> raid_events_str += ",cooldown=85,duration=7,last=";
    sim -> raid_events_str += util::to_string( int( sim -> max_time.total_seconds() * 0.8 ) );
  }
  else if ( util::str_compare_ci( value, "HeavyMovement" ) )
  {
    sim -> fight_style = "HeavyMovement";
    sim -> raid_events_str = "/movement,players_only=1,first=10,cooldown=10,duration=4";
  }
  else if ( util::str_compare_ci( value, "RaidDummy" ) )
  {
    sim -> fight_style = "RaidDummy";
    sim -> overrides.bloodlust = 0;
    sim -> overrides.stormlash = 0;
    sim -> overrides.skull_banner = 0;
    sim -> overrides.target_health = 50000000;
    sim -> target_death_pct = 0;
    sim -> allow_potions = false;
    sim -> vary_combat_length = 0;
    sim -> max_time = timespan_t::from_seconds( 1800 );
    sim -> average_range = false;
    sim -> solo_raid = true;
  }
  else
  {
    sim -> output( "Custom fight style specified: %s", value.c_str() );
    sim -> fight_style = value;
  }

  return true;
}
// parse_override_spell_data ================================================

static bool parse_override_spell_data( sim_t*             sim,
                                       const std::string& /* name */,
                                       const std::string& value )
{
  size_t v_pos = value.find( '=' );

  if ( v_pos == std::string::npos )
    return false;

  std::vector< std::string > splits = util::string_split( value.substr( 0, v_pos ), "." );

  if ( splits.size() != 3 )
    return false;

  unsigned long int id = strtoul( splits[ 1 ].c_str(), 0, 10 );
  if ( id == 0 || id == std::numeric_limits<unsigned long>::max() )
    return false;

  double v = strtod( value.substr( v_pos + 1 ).c_str(), 0 );
  if ( v == std::numeric_limits<double>::min() || v == std::numeric_limits<double>::max() )
    return false;

  if ( util::str_compare_ci( splits[ 0 ], "spell" ) )
  {
    spell_data_t* s = const_cast< spell_data_t* >( sim -> dbc.spell( id ) );
    if ( s == spell_data_t::nil() )
      return false;

    return s -> override( splits[ 2 ], v );
  }
  else if ( util::str_compare_ci( splits[ 0 ], "effect" ) )
  {
    spelleffect_data_t* s = const_cast< spelleffect_data_t* >( sim -> dbc.effect( id ) );
    if ( s == spelleffect_data_t::nil() )
      return false;

    return s -> override( splits[ 2 ], v );
  }
  else
    return false;
}

// parse_spell_query ========================================================

static bool parse_spell_query( sim_t*             sim,
                               const std::string& /* name */,
                               const std::string& value )
{
  std::string sq_str = value;
  size_t lvl_offset = std::string::npos;

  if ( ( lvl_offset = value.rfind( "@" ) ) != std::string::npos )
  {
    std::string lvl_offset_str = value.substr( lvl_offset + 1 );
    int sq_lvl = strtol( lvl_offset_str.c_str(), 0, 10 );
    if ( sq_lvl < 1 || sq_lvl > MAX_LEVEL )
      return 0;

    sim -> spell_query_level = as< unsigned >( sq_lvl );

    sq_str = sq_str.substr( 0, lvl_offset );
  }

  sim -> spell_query = spell_data_expr_t::parse( sim, sq_str );
  return sim -> spell_query != 0;
}

// parse_item_sources =======================================================

// Specifies both the default search order for the various data sources
// and the complete set of valid data source names.
static const char* const default_item_db_sources[] =
{
  "local", "bcpapi", "wowhead", "ptrhead"
};

static bool parse_item_sources( sim_t*             sim,
                                const std::string& /* name */,
                                const std::string& value )
{
  sim -> item_db_sources.clear();

  std::vector<std::string> sources = util::string_split( value, ":/|" );

  for ( size_t j = 0; j < sources.size(); j++ )
  {
    for ( size_t i = 0; i < sizeof_array( default_item_db_sources ); ++i )
    {
      if ( util::str_compare_ci( sources[ j ], default_item_db_sources[ i ] ) )
      {
        sim -> item_db_sources.push_back( default_item_db_sources[ i ] );
        break;
      }
    }
  }

  if ( sim -> item_db_sources.empty() )
  {
    std::string all_known_sources;

    for ( size_t i = 0; i < sizeof_array( default_item_db_sources ); ++i )
    {
      all_known_sources += ' ';
      all_known_sources += default_item_db_sources[ i ];
    }

    sim -> errorf( "Your global data source string \"%s\" contained no valid data sources. Valid identifiers are:%s",
                   value.c_str(), all_known_sources.c_str() );
    return false;
  }

  return true;
}

// Proxy cast ===============================================================

struct proxy_cast_check_t : public event_t
{
  int uses;
  const int& override;
  timespan_t start_time;
  timespan_t cooldown;
  timespan_t duration;

  proxy_cast_check_t( sim_t& s, int u, timespan_t st, timespan_t i, timespan_t cd, timespan_t d, const int& o ) :
    event_t( s, "proxy_cast_check" ),
    uses( u ), override( o ), start_time( st ), cooldown( cd ), duration( d )
  {
    sim.add_event( this, i );
  }

  virtual bool proxy_check() = 0;
  virtual void proxy_execute() = 0;
  virtual proxy_cast_check_t* proxy_schedule( timespan_t interval ) = 0;

  virtual void execute()
  {
    timespan_t interval = timespan_t::from_seconds( 0.25 );

    if ( uses == override && start_time > timespan_t::zero() )
      interval = cooldown - ( sim.current_time - start_time );

    if ( proxy_check() )
    {
      // Cooldown over, reset uses
      if ( uses == override && start_time > timespan_t::zero() && ( sim.current_time - start_time ) >= cooldown )
      {
        start_time = timespan_t::zero();
        uses = 0;
      }

      if ( uses < override )
      {
        proxy_execute();

        uses++;

        if ( start_time == timespan_t::zero() )
          start_time = sim.current_time;

        interval = duration + timespan_t::from_seconds( 1 );

        if ( sim.debug )
          sim.output( "Proxy-Execute uses=%d total=%d start_time=%.3f next_check=%.3f",
                      uses, override, start_time.total_seconds(),
                      ( sim.current_time + interval ).total_seconds() );
      }
    }

    proxy_schedule( interval );
  }
};

struct sim_end_event_t : event_t
{
  sim_end_event_t( sim_t& s, const char* n, timespan_t end_time ) :
    event_t( s, n )
  {
    sim.add_event( this, end_time );
  }
  virtual void execute()
  {
    sim.iteration_canceled = 1;
  }
};

struct resource_timeline_collect_event_t : public event_t
{
  resource_timeline_collect_event_t( sim_t& s ) :
    event_t( s, "resource_timeline_collect_event_t" )
  {
    sim.add_event( this, timespan_t::from_seconds( 1 ) );
  }

  virtual void execute()
  {
    for ( size_t i = 0, actors = sim.actor_list.size(); i < actors; i++ )
    {
      player_t* p = sim.actor_list[ i ];
      if ( p -> current.sleeping ) continue;
      if ( p -> primary_resource() == RESOURCE_NONE ) continue;

      p -> collect_resource_timeline_information();
    }

    new ( sim ) resource_timeline_collect_event_t( sim );
  }
};

struct regen_event_t : public event_t
{
  regen_event_t( sim_t& s ) :
    event_t( s, "Regen Event" )
  {
    if ( sim.debug ) sim.output( "New Regen Event" );

    sim.add_event( this, sim.regen_periodicity );
  }

  virtual void execute()
  {
    for ( size_t i = 0, actors = sim.actor_list.size(); i < actors; i++ )
    {
      player_t* p = sim.actor_list[ i ];
      if ( p -> current.sleeping ) continue;
      if ( p -> primary_resource() == RESOURCE_NONE ) continue;

      p -> regen( sim.regen_periodicity );
    }

    new ( sim ) regen_event_t( sim );
  }
};

} // UNNAMED NAMESPACE ===================================================

// ==========================================================================
// Simulator
// ==========================================================================

// sim_t::sim_t =============================================================

sim_t::sim_t( sim_t* p, int index ) :
  control( 0 ),
  parent( p ),
  initialized( false ),
  target( NULL ),
  heal_target( NULL ),
  target_list( 0 ),
  active_player( 0 ),
  num_players( 0 ),
  num_enemies( 0 ),
  max_player_level( -1 ),
  canceled( 0 ), iteration_canceled( 0 ),
  queue_lag( timespan_t::from_seconds( 0.037 ) ), queue_lag_stddev( timespan_t::zero() ),
  gcd_lag( timespan_t::from_seconds( 0.150 ) ), gcd_lag_stddev( timespan_t::zero() ),
  channel_lag( timespan_t::from_seconds( 0.250 ) ), channel_lag_stddev( timespan_t::zero() ),
  queue_gcd_reduction( timespan_t::from_seconds( 0.032 ) ), strict_gcd_queue( 0 ),
  confidence( 0.95 ), confidence_estimator( 0.0 ),
  world_lag( timespan_t::from_seconds( 0.1 ) ), world_lag_stddev( timespan_t::min() ),
  travel_variance( 0 ), default_skill( 1.0 ), reaction_time( timespan_t::from_seconds( 0.5 ) ),
  regen_periodicity( timespan_t::from_seconds( 0.25 ) ),
  ignite_sampling_delta( timespan_t::from_seconds( 0.2 ) ),
  current_time( timespan_t::zero() ), max_time( timespan_t::from_seconds( 450 ) ),
  expected_time( timespan_t::zero() ), vary_combat_length( 0.2 ),
  fixed_time( 0 ),
  events_remaining( 0 ), max_events_remaining( 0 ),
  events_processed( 0 ), total_events_processed( 0 ),
  seed( 0 ), id( 0 ), iterations( 1000 ), current_iteration( -1 ), current_slot( -1 ),
  armor_update_interval( 20 ), weapon_speed_scale_factors( 0 ),
  optimal_raid( 0 ), log( 0 ), debug( 0 ), save_profiles( 0 ), default_actions( 0 ),
  normalized_stat( STAT_NONE ),
  default_region_str( "us" ),
  save_prefix_str( "save_" ),
  save_talent_str( 0 ),
  talent_format( TALENT_FORMAT_UNCHANGED ),
  auto_ready_trigger( 0 ),
  target_death( 0 ), target_death_pct( 0 ), rel_target_level( 3 ), target_level( -1 ), target_adds( 0 ),
  healer_sim( false ), tank_sim( false ), challenge_mode( false ), scale_to_itemlevel ( -1 ),
  active_enemies( 0 ), active_allies( 0 ),
  deterministic_rng( false ),
  separated_rng( false ), average_range( true ), average_gauss( false ),
  convergence_scale( 2 ),
  recycled_event_list( 0 ), timing_wheel(), wheel_seconds( 0 ), wheel_size( 0 ), wheel_mask( 0 ), timing_slice( 0 ), wheel_granularity( 0.0 ),
  fight_style( "Patchwerk" ), overrides( overrides_t() ), auras( auras_t() ),
  aura_delay( timespan_t::from_seconds( 0.5 ) ), default_aura_delay( timespan_t::from_seconds( 0.3 ) ),
  default_aura_delay_stddev( timespan_t::from_seconds( 0.05 ) ),
  progress_bar( *this ),
  scaling( new scaling_t( this ) ),
  plot( new plot_t( this ) ),
  reforge_plot( new reforge_plot_t( this ) ),
  elapsed_cpu( timespan_t::zero() ), iteration_dmg( 0 ), iteration_heal( 0 ),
  raid_dps( std::string( "Raid Damage Per Second" ) ), total_dmg(), raid_hps( std::string( "Raid Healing Per Second" ) ), total_heal(), simulation_length( false ),
  report_progress( 1 ),
  bloodlust_percent( 25 ), bloodlust_time( timespan_t::from_seconds( 5.0 ) ),
  output_file( stdout ),
  debug_exp( 0 ),
  // Report
  report_precision( 2 ),report_pets_separately( 0 ), report_targets( 1 ), report_details( 1 ), report_raw_abilities( 1 ),
  report_rng( 0 ), hosted_html( 0 ), print_styles( false ), report_overheal( 0 ),
  save_raid_summary( 0 ), save_gear_comments( 0 ), statistics_level( 1 ), separate_stats_by_actions( 0 ), report_raid_summary( 0 ),
  allow_potions( true ),
  allow_food( true ),
  allow_flasks( true ),
  solo_raid( false ),
  global_item_upgrade_level( 0 ),
  report_information( report_information_t() ),
  // Multi-Threading
  threads( 0 ), thread_index( index ),
  spell_query( 0 ), spell_query_level( MAX_LEVEL )
{
  item_db_sources.assign( range::begin( default_item_db_sources ),
                          range::end( default_item_db_sources ) );


  use_optimal_buffs_and_debuffs( 1 );

  create_options();

  if ( parent )
  {
    // Inherit setup
    setup( parent -> control );

    // Inherit 'scaling' settings from parent because these are set outside of the config file
    assert( parent -> scaling );
    scaling -> scale_stat  = parent -> scaling -> scale_stat;
    scaling -> scale_value = parent -> scaling -> scale_value;

    // Inherit reporting directives from parent
    report_progress = parent -> report_progress;
    output_file     = parent -> output_file;

    // Inherit 'plot' settings from parent because are set outside of the config file
    enchant = parent -> enchant;

    seed = parent -> seed;

    // Inherit scale_to_itemlevel
    if ( challenge_mode ) scale_to_itemlevel = 463;
  }
}

// sim_t::~sim_t ============================================================

sim_t::~sim_t()
{
  event_t::release( *this );
  delete scaling;
  delete plot;
  delete reforge_plot;
  delete spell_query;
}

// sim_t::add_event =========================================================

void sim_t::add_event( event_t* e,
                       timespan_t delta_time )
{
  e -> id   = ++id;

  if ( delta_time < timespan_t::zero() )
    delta_time = timespan_t::zero();

  if ( unlikely( delta_time > wheel_time ) )
  {
    e -> time = current_time + wheel_time - timespan_t::from_seconds( 1 );
    e -> reschedule_time = current_time + delta_time;
  }
  else
  {
    e -> time = current_time + delta_time;
    e -> reschedule_time = timespan_t::zero();
  }

  // Determine the timing wheel position to which the event will belong
#ifdef SC_USE_INTEGER_WHEEL_SHIFT
  uint32_t slice = ( uint32_t ) ( e -> time.total_millis() >> SC_USE_INTEGER_WHEEL_SHIFT ) & wheel_mask;
#else
  uint32_t slice = ( uint32_t ) ( e -> time.total_seconds() * wheel_granularity ) & wheel_mask;
#endif

  // Insert event into the event list at the appropriate time
  event_t** prev = &( timing_wheel[ slice ] );
  while ( ( *prev ) && ( *prev ) -> time <= e -> time ) // Find position in the list
  { prev = &( ( *prev ) -> next ); }
  // insert event
  e -> next = *prev;
  *prev = e;

  events_remaining++;
  if ( events_remaining > max_events_remaining ) max_events_remaining = events_remaining;
  if ( e -> player ) e -> player -> events++;

  if ( debug )
  {
    output( "Add Event: %s %.2f %.2f %d %s", e -> name, e -> time.total_seconds(), e -> reschedule_time.total_seconds(), e -> id, e -> player ? e -> player -> name() : "" );
    if ( e -> player ) output( "Actor %s has %d scheduled events", e -> player -> name(), e -> player -> events );
  }
}

// sim_t::reschedule_event ==================================================

void sim_t::reschedule_event( event_t* e )
{
  if ( debug ) output( "Reschedule Event: %s %d", e -> name, e -> id );

  add_event( e, ( e -> reschedule_time - current_time ) );
}

// sim_t::next_event ========================================================

event_t* sim_t::next_event()
{
  if ( events_remaining == 0 ) return 0;

  while ( true )
  {
    event_t*& event_list = timing_wheel[ timing_slice ];
    if ( event_list )
    {
      event_t* e = event_list;
      event_list = e -> next;
      events_remaining--;
      events_processed++;
      return e;
    }

    timing_slice++;
    if ( timing_slice == timing_wheel.size() )
    {
      timing_slice = 0;
      if ( debug ) output( "Time Wheel turns around." );
    }
  }

  return 0;
}

// sim_t::flush_events ======================================================

void sim_t::flush_events()
{
  if ( debug ) output( "Flush Events" );

  for ( size_t i = 0; i < timing_wheel.size(); ++i )
  {
    while ( event_t* e = timing_wheel[ i ] )
    {
      timing_wheel[ i ] = e -> next;
      event_t* null_e = e; // necessary evil
      event_t::cancel( null_e );
      event_t::recycle( e );
    }
  }

  events_remaining = 0;
  events_processed = 0;
  timing_slice = 0;
  id = 0;
}

// sim_t::combat ============================================================

void sim_t::combat( int iteration )
{
  if ( debug ) output( "Starting Simulator" );

  current_iteration = iteration;

  combat_begin();

  while ( event_t* e = next_event() )
  {
    current_time = e -> time;

    // Perform actor event bookkeeping first
    if ( e -> player && ! e -> canceled )
    {
      e -> player -> events--;
      if ( e -> player -> events < 0 )
      {
        errorf( "sim_t::combat assertion error! canceling event %s leaves negative event count for user %s.\n", e -> name, e -> player -> name() );
        assert( 0 );
      }
    }

    if ( unlikely( e -> canceled ) )
    {
      if ( debug ) output( "Canceled event: %s", e -> name );
    }
    else if ( unlikely( e -> reschedule_time > e -> time ) )
    {
      reschedule_event( e );
      continue;
    }
    else
    {
      if ( debug ) output( "Executing event: %s %s", e -> name, e -> player ? e -> player -> name() : "" );
      e -> execute();
    }

    event_t::recycle( e );

    // This should be moved to assess_damage somehow, but it is a little tricky given mixed inheritance of player/enemy.
    if ( target_death >= 0 )
      if ( target -> resources.current[ RESOURCE_HEALTH ] <= target_death )
        iteration_canceled = 1;

    if ( unlikely( iteration_canceled ) )
      break;
  }

  combat_end();
}

// sim_t::reset =============================================================

void sim_t::reset()
{
  if ( debug ) output( "Resetting Simulator" );
  expected_time = max_time * ( 1.0 + vary_combat_length * iteration_adjust() );
  id = 0;
  current_time = timespan_t::zero();

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> reset();

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    t -> reset();
  }
  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    p -> reset();
  }
  raid_event_t::reset( this );
}

// sim_t::combat_begin ======================================================

void sim_t::combat_begin()
{
  if ( debug ) output( "Combat Begin" );

  reset();

  iteration_dmg = iteration_heal = 0;

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    t -> combat_begin();
  }


  if ( overrides.attack_haste            ) auras.attack_haste            -> override();
  if ( overrides.attack_power_multiplier ) auras.attack_power_multiplier -> override();
  if ( overrides.critical_strike         ) auras.critical_strike         -> override();

  if ( overrides.mastery                 )
    auras.mastery -> override( 1, dbc.effect_average( dbc.spell( 116956 ) -> effectN( 1 ).id(), max_player_level ) );

  if ( overrides.spell_haste             ) auras.spell_haste             -> override();
  if ( overrides.spell_power_multiplier  ) auras.spell_power_multiplier  -> override();
  if ( overrides.stamina                 ) auras.stamina                 -> override();
  if ( overrides.str_agi_int             ) auras.str_agi_int             -> override();

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( overrides.slowed_casting         ) t -> debuffs.slowed_casting         -> override();
    if ( overrides.magic_vulnerability    ) t -> debuffs.magic_vulnerability    -> override();
    if ( overrides.ranged_vulnerability   ) t -> debuffs.ranged_vulnerability   -> override();
    if ( overrides.mortal_wounds          ) t -> debuffs.mortal_wounds          -> override();
    if ( overrides.physical_vulnerability ) t -> debuffs.physical_vulnerability -> override();
    if ( overrides.weakened_armor         ) t -> debuffs.weakened_armor         -> override( 3 );
    if ( overrides.weakened_blows         ) t -> debuffs.weakened_blows         -> override();
    if ( overrides.bleeding               ) t -> debuffs.bleeding               -> override();
  }

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    const module_t* m = module_t::get( i );
    if ( m ) m -> combat_begin( this );
  }

  raid_event_t::combat_begin( this );

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    p -> combat_begin();
  }
  new ( *this ) regen_event_t( *this );

  // Always call begin() to ensure various counters are initialized.
  datacollection_begin();

  if ( overrides.bloodlust )
  {
    // Setup a periodic check for Bloodlust

    struct bloodlust_check_t : public event_t
    {
      bloodlust_check_t( sim_t& sim ) :
        event_t( sim, "Bloodlust Check" )
      {
        sim.add_event( this, timespan_t::from_seconds( 1.0 ) );
      }
      virtual void execute()
      {
        player_t* t = sim.target;
        if ( ( sim.bloodlust_percent  > 0                  && t -> health_percentage() <  sim.bloodlust_percent ) ||
             ( sim.bloodlust_time     < timespan_t::zero() && t -> time_to_die()       < -sim.bloodlust_time ) ||
             ( sim.bloodlust_time     > timespan_t::zero() && sim.current_time      >  sim.bloodlust_time ) )
        {
          for ( size_t i = 0; i < sim.player_list.size(); ++i )
          {
            player_t* p = sim.player_list[ i ];
            if ( p -> current.sleeping || p -> buffs.exhaustion -> check() || p -> is_pet() || p -> is_enemy() )
              continue;

            p -> buffs.bloodlust -> trigger();
            p -> buffs.exhaustion -> trigger();
          }
        }
        else
        {
          new ( sim ) bloodlust_check_t( sim );
        }
      }
    };

    new ( *this ) bloodlust_check_t( *this );
  }

  if ( overrides.stormlash )
  {
    struct stormlash_proxy_t : public proxy_cast_check_t
    {
      stormlash_proxy_t( sim_t& s, int u, timespan_t st, timespan_t i ) :
        proxy_cast_check_t( s, u, st, i, timespan_t::from_seconds( 300 ), timespan_t::from_seconds( 10 ), s.overrides.stormlash )
      { }

      // Sync to (reasonably) early proxy-Bloodlust if available
      bool proxy_check()
      {
        return sim.bloodlust_time <= timespan_t::zero() ||
               sim.bloodlust_time >= timespan_t::from_seconds( 30 ) ||
               ( sim.bloodlust_time > timespan_t::zero() && sim.bloodlust_time < timespan_t::from_seconds( 30 ) &&
                 sim.current_time > sim.bloodlust_time + timespan_t::from_seconds( 1 ) );
      }

      void proxy_execute()
      {
        for ( size_t i = 0; i < sim.player_list.size(); ++i )
        {
          player_t* p = sim.player_list[ i ];
          if ( p -> type == PLAYER_GUARDIAN || p -> is_enemy() )
            continue;

          p -> buffs.stormlash -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
        }
      }

      proxy_cast_check_t* proxy_schedule( timespan_t interval )
      { return new ( sim ) stormlash_proxy_t( sim, uses, start_time, interval ); }
    };

    new ( *this ) stormlash_proxy_t( *this, 0, timespan_t::zero(), timespan_t::from_seconds( 0.25 ) );
  }

  if ( overrides.skull_banner )
  {
    struct skull_banner_proxy_t : public proxy_cast_check_t
    {
      skull_banner_proxy_t( sim_t& s, int u, timespan_t st, timespan_t i ) :
        proxy_cast_check_t( s, u, st, i, timespan_t::from_seconds( 180 ), timespan_t::from_seconds( 10 ), s.overrides.skull_banner )
      { }

      // Sync to (reasonably) early proxy-Bloodlust if available
      bool proxy_check()
      {
        return sim.bloodlust_time <= timespan_t::zero() ||
               sim.bloodlust_time >= timespan_t::from_seconds( 30 ) ||
               ( sim.bloodlust_time > timespan_t::zero() && sim.bloodlust_time < timespan_t::from_seconds( 30 ) &&
                 sim.current_time > sim.bloodlust_time + timespan_t::from_seconds( 1 ) );
      }

      void proxy_execute()
      {
        for ( size_t i = 0; i < sim.player_list.size(); ++i )
        {
          player_t* p = sim.player_list[ i ];
          if ( p -> type == PLAYER_GUARDIAN || p -> is_enemy() )
            continue;

          p -> buffs.skull_banner -> trigger();
        }
      }

      proxy_cast_check_t* proxy_schedule( timespan_t interval )
      { return new ( sim ) skull_banner_proxy_t( sim, uses, start_time, interval ); }
    };

    new ( *this ) skull_banner_proxy_t( *this, 0, timespan_t::zero(), timespan_t::from_seconds( 0.25 ) );
  }

  iteration_canceled = 0;

  if ( fixed_time || ( target -> resources.base[ RESOURCE_HEALTH ] == 0 ) )
  {
    new ( *this ) sim_end_event_t( *this, "sim_end_expected_time", expected_time );
    target_death = -1;
  }
  else
  {
    target_death = target -> resources.max[ RESOURCE_HEALTH ] * target_death_pct / 100.0;
  }
  new ( *this ) sim_end_event_t( *this, "sim_end_twice_expected_time", expected_time + expected_time );
}

// sim_t::combat_end ========================================================

void sim_t::combat_end()
{
  if ( debug ) output( "Combat End" );

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> combat_end();
  }

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    const module_t* m = module_t::get( i );
    if ( m ) m -> combat_end( this );
  }

  raid_event_t::combat_end( this );

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    if ( p -> is_pet() ) continue;
    p -> combat_end();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> expire();
  }

  if ( iterations == 1 || current_iteration >= 1 )
    datacollection_end();

  flush_events();

  assert( active_enemies == 0 && active_allies == 0 );
}

// sim_t::combat_begin ======================================================

void sim_t::datacollection_begin()
{
  if ( debug ) output( "Sim Data Collection Begin" );

  iteration_dmg = iteration_heal = 0;

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> datacollection_begin();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    if ( p -> is_pet() ) continue;
    p -> datacollection_begin();
  }
  new ( *this ) resource_timeline_collect_event_t( *this );

}

// sim_t::datacollection_end ========================================================

void sim_t::datacollection_end()
{
  if ( debug ) output( "Sim Data Collection End" );

  iteration_timeline.push_back( current_time );
  simulation_length.add( current_time.total_seconds() );

  total_events_processed += events_processed;

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> datacollection_end();
  }
  raid_event_t::combat_end( this );

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    if ( p -> is_pet() ) continue;
    p -> datacollection_end();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> datacollection_end();
  }

  total_dmg.add( iteration_dmg );
  raid_dps.add( current_time != timespan_t::zero() ? iteration_dmg / current_time.total_seconds() : 0 );
  total_heal.add( iteration_heal );
  raid_hps.add( current_time != timespan_t::zero() ? iteration_heal / current_time.total_seconds() : 0 );
}

// sim_t::init ==============================================================

bool sim_t::init()
{
  if ( initialized )
    return true;

  if ( scaling -> smooth_scale_factors &&
       scaling -> scale_stat != STAT_NONE )
  {
    separated_rng = true;
    average_range = true;
    deterministic_rng = true;
  }

  // Seed RNG
  if ( seed == 0 )
    seed = deterministic_rng ? 31459 : ( int ) time( nullptr );
  rng.seed( seed + thread_index );

  // Timing wheel depth defaults to about 17 minutes with a granularity of 32 buckets per second.
  // This makes wheel_size = 32K and it's fully used.
  if ( wheel_seconds     < 1024 ) wheel_seconds     = 1024; // 2^10 Min to ensure limited wrap-around
  if ( wheel_granularity <=   0 ) wheel_granularity = 32;   // 2^5 Time slices per second

  wheel_time = timespan_t::from_seconds( wheel_seconds );

#ifdef SC_USE_INTEGER_WHEEL_SHIFT
  wheel_size = ( uint32_t ) ( wheel_time.total_millis() >> SC_USE_INTEGER_WHEEL_SHIFT );
#else
  wheel_size = ( uint32_t ) ( wheel_seconds * wheel_granularity );
#endif

  // Round up the wheel depth to the nearest power of 2 to enable a fast "mod" operation.
  for ( wheel_mask = 2; wheel_mask < wheel_size; wheel_mask *= 2 ) { continue; }
  wheel_size = wheel_mask;
  wheel_mask--;

  // The timing wheel represents an array of event lists: Each time slice has an event list.
  timing_wheel.resize( wheel_size );


  if (   queue_lag_stddev == timespan_t::zero() )   queue_lag_stddev =   queue_lag * 0.25;
  if (     gcd_lag_stddev == timespan_t::zero() )     gcd_lag_stddev =     gcd_lag * 0.25;
  if ( channel_lag_stddev == timespan_t::zero() ) channel_lag_stddev = channel_lag * 0.25;
  if ( world_lag_stddev    < timespan_t::zero() ) world_lag_stddev   =   world_lag * 0.1;

  // MoP aura initialization

  // Attack and Ranged haste, value from Swiftblade's Cunning (id=113742) (Rogue)
  auras.attack_haste = buff_creator_t( this, "attack_haste" )
                       .max_stack( 100 )
                       .default_value( dbc.spell( 113742 ) -> effectN( 1 ).percent() );

  // Attack Power Multiplier, value from Trueshot Aura (id=19506) (Hunter)
  auras.attack_power_multiplier = buff_creator_t( this, "attack_power_multiplier" )
                                  .max_stack( 100 )
                                  .default_value( dbc.spell( 19506 ) -> effectN( 1 ).percent() );

  // Critical Strike, value from Arcane Brilliance (id=1459) (Mage)
  auras.critical_strike = buff_creator_t( this, "critical_strike" )
                          .max_stack( 100 )
                          .default_value( dbc.spell( 1459 ) -> effectN( 2 ).percent() );

  // Mastery, value from Grace of Air (id=116956) (Shaman)
  auras.mastery = buff_creator_t( this, "mastery" )
                  .max_stack( 100 )
                  .default_value( dbc.spell( 116956 ) -> effectN( 1 ).base_value() );

  // Spell Haste, value from Mind Quickening (id=49868) (Priest)
  auras.spell_haste = buff_creator_t( this, "spell_haste" )
                      .max_stack( 100 )
                      .default_value( dbc.spell( 49868 ) -> effectN( 1 ).percent() );

  // Spell Power Multiplier, value from Burning Wrath (id=77747) (Shaman)
  auras.spell_power_multiplier = buff_creator_t( this, "spell_power_multiplier" )
                                 .max_stack( 100 )
                                 .default_value( dbc.spell( 77747 ) -> effectN( 1 ).percent() );

  // Stamina, value from fortitude (id=21562) (Priest)
  auras.stamina = buff_creator_t( this, "stamina" )
                  .max_stack( 100 )
                  .default_value( dbc.spell( 21562 ) -> effectN( 1 ).percent() );

  // Strength, Agility, and Intellect, value from Blessing of Kings (id=20217) (Paladin)
  auras.str_agi_int = buff_creator_t( this, "str_agi_int" )
                      .max_stack( 100 )
                      .default_value( dbc.spell( 20217 ) -> effectN( 1 ).percent() );

  // Find Already defined target, otherwise create a new one.
  if ( debug )
    output( "Creating Enemies." );

  if ( !target_list.empty() )
  {
    target = target_list.front();
  }
  else if ( ! main_target_str.empty() )
  {
    player_t* p = find_player( main_target_str );
    if ( p )
      target = p;
  }
  else
    target = module_t::enemy().create_player( this, "Fluffy_Pillow" );

  {
    // Determine whether we have healers or tanks.
    unsigned int healers = 0, tanks = 0;
    for ( size_t i = 0; i < player_list.size(); ++i )
    {
      if ( !player_list[ i ] -> is_pet() && player_list[ i ] -> primary_role() == ROLE_HEAL )
        ++healers;
      if ( !player_list[ i ] -> is_pet() && player_list[ i ] -> primary_role() == ROLE_TANK )
        ++tanks;
    }
    if ( healers > 0 )
      healer_sim = true;
    if ( tanks > 0 )
      tank_sim = true;
  }

  if ( healer_sim )
    heal_target = module_t::heal_enemy().create_player( this, "Healing Target" );


  if ( max_player_level < 0 )
  {
    for ( size_t i = 0; i < player_list.size(); ++i )
    {
      player_t* p = player_list[ i ];
      if ( p -> is_enemy() || p -> is_add() )
        continue;
      if ( max_player_level < p -> level )
        max_player_level = p -> level;
    }
  }

  raid_event_t::init( this );

  if ( ! player_t::init( this ) ) return false;

  // Target overrides 2
  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( ! target_race.empty() )
    {
      t -> race = util::parse_race_type( target_race );
      t -> race_str = util::race_type_string( t -> race );
    }
  }

  if ( report_precision < 0 ) report_precision = 4;

  iteration_timeline.reserve( iterations );
  raid_dps.reserve( iterations );
  total_dmg.reserve( iterations );
  raid_hps.reserve( iterations );
  total_heal.reserve( iterations );
  simulation_length.reserve( iterations );

  initialized = true;

  return canceled ? false : true;
}

// compare_dps ==============================================================

struct compare_dps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> dps.mean > r -> dps.mean;
  }
};

// compare_hps ==============================================================

struct compare_hps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> hps.mean > r -> hps.mean;
  }
};

// compare_name =============================================================

struct compare_name
{
  bool operator()( player_t* l, player_t* r ) const
  {
    if ( l -> type != r -> type )
    {
      return l -> type < r -> type;
    }
    if ( l -> specialization() != r -> specialization() )
    {
      return l -> specialization() < r -> specialization();
    }
    return l -> name_str < r -> name_str;
  }
};


// sim_t::analyze ===========================================================

void sim_t::analyze()
{
  simulation_length.analyze( true, true, true, true );
  if ( simulation_length.mean == 0 ) return;

  // divisor_timeline is necessary because not all iterations go the same length of time
  int max_buckets = ( int ) simulation_length.max + 1;
  divisor_timeline.assign( max_buckets, 0 );

  size_t num_timelines = iteration_timeline.size();
  for ( size_t i = 0; i < num_timelines; i++ )
  {
    int last = ( int ) floor( iteration_timeline[ i ].total_seconds() );
    size_t num_buckets = divisor_timeline.size();
    if ( 1 + last > ( int ) num_buckets ) divisor_timeline.resize( 1 + last, 0 );
    for ( int j = 0; j <= last; j++ ) divisor_timeline[ j ] += 1;
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> analyze();

  total_dmg.analyze();
  raid_dps.analyze();
  total_heal.analyze();
  raid_hps.analyze();

  confidence_estimator = rng::stdnormal_inv( 1.0 - ( 1.0 - confidence ) / 2.0 );

  for ( size_t i = 0; i < actor_list.size(); i++ )
    actor_list[ i ] -> analyze( *this );

  range::sort( players_by_dps, compare_dps() );
  range::sort( players_by_hps, compare_hps() );
  range::sort( players_by_name, compare_name() );
  range::sort( targets_by_name, compare_name() );
}

// progress_bar_t ===========================================================

progress_bar_t::progress_bar_t( sim_t& s ) :
  sim(s), steps(20), updates(100), interval(0), start_time(0) {}

void progress_bar_t::init()
{
  start_time = util::wall_time();
  interval = sim.iterations / updates;
  if ( interval == 0 ) interval = 1;
}

bool progress_bar_t::update()
{
  if ( ! sim.report_progress ) return false;

  if( sim.current_iteration != sim.iterations )
  {
    if ( ! sim.current_iteration ) return false;
    if ( sim.current_iteration % interval ) return false;
  }

  int current, final;
  double pct = sim.progress( &current, &final );
  if ( pct <= 0 ) return false;

  int prev_size = status.size();

  status = "[";
  status.insert( 1, steps, '.' );
  status += "]";

  int length = steps * pct;
  for( int i=1; i < length+1; i++ ) status[ i ] = '=';
  if( length > 0 ) status[ length ] = '>';

  double current_time = util::wall_time() - start_time;
  double total_time = current_time / pct;

  int remaining_sec = int( total_time - current_time );
  int remaining_min = remaining_sec / 60;
  remaining_sec -= remaining_min * 60;

  char buffer[80];
  snprintf( buffer, sizeof(buffer), " %d/%d", current, final );
  status += buffer;

  if( remaining_min > 0 )
  {
    snprintf( buffer, sizeof(buffer), " %dmin", remaining_min );
    status += buffer;
  }

  if( remaining_sec > 0 )
  {
    snprintf( buffer, sizeof(buffer), " %dsec", remaining_sec );
    status += buffer;
  }

  int diff = prev_size - status.size();
  if( diff > 0 ) status.insert( status.end(), diff, ' ' );

  return true;
}

// sim_t::iterate ===========================================================

bool sim_t::iterate()
{
  if ( ! init() ) return false;

  progress_bar.init();

  for ( int i=0; i < iterations; i++ )
  {
    if ( canceled )
    {
      iterations = current_iteration + 1;
      break;
    }

    if ( progress_bar.update() )
    {
      util::fprintf( stdout, "%s %s\r", sim_phase_str.c_str(), progress_bar.status.c_str() );
      fflush( stdout );
    }
    combat( i );
  }

  if ( progress_bar.update() )
  {
    util::fprintf( stdout, "%s %s\r", sim_phase_str.c_str(), progress_bar.status.c_str() );
    fflush( stdout );
  }

  reset();

  return true;
}

// sim_t::merge =============================================================

void sim_t::merge( sim_t& other_sim )
{
  auto_lock_t auto_lock( mutex );

  iterations             += other_sim.iterations;
  total_events_processed += other_sim.total_events_processed;

  simulation_length.merge( other_sim.simulation_length );
  total_dmg.merge( other_sim.total_dmg );
  raid_dps.merge( other_sim.raid_dps );
  total_heal.merge( other_sim.total_heal );
  raid_hps.merge( other_sim.raid_hps );

  if ( max_events_remaining < other_sim.max_events_remaining ) max_events_remaining = other_sim.max_events_remaining;

  range::copy( other_sim.iteration_timeline, std::back_inserter( iteration_timeline ) );

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    if ( buff_t* otherbuff = buff_t::find( &other_sim, buff_list[ i ] -> name_str.c_str() ) )
      buff_list[ i ] -> merge( *otherbuff );
  }

  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[ i ];
    player_t* other_p = other_sim.find_player( p -> index );
    assert( other_p );
    p -> merge( *other_p );
  }
}

// sim_t::merge =============================================================

void sim_t::merge()
{
  if ( children.empty() ) return;

  mutex.unlock();

  for ( size_t i = 0; i < children.size(); i++ )
  {
    sim_t* child = children[ i ];
    child -> wait();
    delete child;
  }

  children.clear();
}

// sim_t::run ===============================================================

void sim_t::run()
{
  iterate();
  parent -> merge( *this );
}

// sim_t::partition =========================================================

void sim_t::partition()
{
  if ( threads <= 1 ) return;
  if ( iterations < threads ) return;

#if defined( NO_THREADS )
  util::fprintf( output_file, "simulationcraft: This executable was built without thread support, please remove 'threads=N' from config file.\n" );
  exit( 0 );
#endif

  mutex.lock(); // parent sim is locked until parent merge() is called

  int remainder = iterations % threads;
  iterations /= threads;

  int num_children = threads - 1;
  children.resize( num_children );

  for ( int i = 0; i < num_children; i++ )
  {
    sim_t* child = children[ i ] = new sim_t( this, i + 1 );
    child -> iterations /= threads;
    if ( remainder )
    {
      child -> iterations += 1;
      remainder--;
    }

    child -> report_progress = 0;
  }

  for ( int i = 0; i < num_children; i++ )
    children[ i ] -> launch();
}

// sim_t::execute ===========================================================

bool sim_t::execute()
{
  int64_t start_time = util::milliseconds();

  partition();
  if ( ! iterate() ) return false;
  merge();
  analyze();

  elapsed_cpu = timespan_t::from_millis( ( util::milliseconds() - start_time ) );

  return true;
}

// sim_t::find_player =======================================================

player_t* sim_t::find_player( const std::string& name )
{
  for ( size_t i = 0, actors = actor_list.size(); i < actors; ++i )
  {
    player_t* p = actor_list[ i ];
    if ( name == p -> name() )
      return p;
  }
  return 0;
}

// sim_t::find_player =======================================================

player_t* sim_t::find_player( int index )
{
  for ( size_t i = 0, actors = actor_list.size(); i < actors; ++i )
  {
    player_t* p = actor_list[ i ];
    if ( index == p -> index )
      return p;
  }
  return 0;
}

// sim_t::get_cooldown ======================================================

cooldown_t* sim_t::get_cooldown( const std::string& name )
{
  cooldown_t* c = 0;

  for ( size_t i = 0; i < cooldown_list.size(); ++i )
  {
    c = cooldown_list[ i ];
    if ( c -> name_str == name )
      return c;
  }

  c = new cooldown_t( name, this );

  cooldown_list.push_back( c );

  return c;
}

// sim_t::use_optimal_buffs_and_debuffs =====================================

void sim_t::use_optimal_buffs_and_debuffs( int value )
{
  optimal_raid = value;

  overrides.attack_haste            = optimal_raid;
  overrides.attack_power_multiplier = optimal_raid;
  overrides.critical_strike         = optimal_raid;
  overrides.mastery                 = optimal_raid;
  overrides.spell_haste             = optimal_raid;
  overrides.spell_power_multiplier  = optimal_raid;
  overrides.stamina                 = optimal_raid;
  overrides.str_agi_int             = optimal_raid;

  overrides.slowed_casting          = optimal_raid;
  overrides.magic_vulnerability     = optimal_raid;
  overrides.ranged_vulnerability    = optimal_raid;
  overrides.mortal_wounds           = optimal_raid;
  overrides.physical_vulnerability  = optimal_raid;
  overrides.weakened_armor          = optimal_raid;
  overrides.weakened_blows          = optimal_raid;
  overrides.bleeding                = optimal_raid;

  overrides.bloodlust               = optimal_raid;
  overrides.stormlash               = ( optimal_raid ) ? 2 : 0;
  overrides.skull_banner            = ( optimal_raid ) ? 2 : 0;
}

// sim_t::time_to_think =====================================================

bool sim_t::time_to_think( timespan_t proc_time )
{
  if ( proc_time == timespan_t::zero() ) return false;
  if ( proc_time < timespan_t::zero() ) return true;
  return current_time - proc_time > reaction_time;
}

// sim_t::gauss =============================================================

double sim_t::gauss( double mean,
                     double stddev )
{
  if ( average_gauss ) return mean;

  return default_rng() -> gauss( mean, stddev );
}

// sim_t::gauss =============================================================

timespan_t sim_t::gauss( timespan_t mean,
                         timespan_t stddev )
{
  return timespan_t::from_native( gauss( ( double ) timespan_t::to_native( mean ), ( double ) timespan_t::to_native( stddev ) ) );
}

// sim_t::get_rng ===========================================================

rng_t* sim_t::get_rng( const std::string& )
{
  if ( ! separated_rng )
    return default_rng();

  rng_t* r = new rng_t;

  rng_list.push_back( r );

  return r;
}

// sim_t::iteration_adjust ==================================================

double sim_t::iteration_adjust()
{
  if ( iterations <= 1 )
    return 0.0;

  if ( current_iteration == 0 )
    return 0.0;

  return ( ( current_iteration % 2 ) ? 1 : -1 ) * current_iteration / ( double ) iterations;
}

// sim_t::create_expression =================================================

expr_t* sim_t::create_expression( action_t* a,
                                  const std::string& name_str )
{
  if ( name_str == "time" )
    return make_ref_expr( name_str, current_time );

  if ( util::str_compare_ci( name_str, "enemies" ) )
    return make_ref_expr( name_str, num_enemies );

  if ( util::str_compare_ci( name_str, "active_enemies" ) )
    return make_ref_expr( name_str, active_enemies );

  if ( util::str_compare_ci( name_str, "active_allies" ) )
    return make_ref_expr( name_str, active_allies );

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.size() == 3 )
  {
    if ( splits[ 0 ] == "aura" )
    {
      buff_t* buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) return 0;
      return buff_t::create_expression( splits[ 1 ], a, splits[ 2 ], buff );
    }
  }
  if ( splits.size() >= 3 && splits[ 0 ] == "actors" )
  {
    player_t* actor = sim_t::find_player( splits[ 1 ] );
    if ( ! target ) return 0;
    std::string rest = splits[ 2 ];
    for ( size_t i = 3; i < splits.size(); ++i )
      rest += '.' + splits[ i ];
    return actor -> create_expression( a, rest );
  }
  if ( splits.size() >= 2 && splits[ 0 ] == "target" )
  {
    std::string rest = splits[1];
    for ( size_t i = 2; i < splits.size(); ++i )
      rest += '.' + splits[ i ];
    return target -> create_expression( a, rest );
  }

  return 0;
}

// sim_t::print_options =====================================================

void sim_t::print_options()
{
  util::fprintf( output_file, "\nWorld of Warcraft Raid Simulator Options:\n" );

  int num_options = ( int ) options.size();

  util::fprintf( output_file, "\nSimulation Engine:\n" );
  for ( int i=0; i < num_options; i++ ) options[ i ].print( output_file );

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    num_options = ( int ) p -> options.size();

    util::fprintf( output_file, "\nPlayer: %s (%s)\n", p -> name(), util::player_type_string( p -> type ) );
    for ( int i=0; i < num_options; i++ ) p -> options[ i ].print( output_file );
  }

  util::fprintf( output_file, "\n" );
  fflush( output_file );
}

// sim_t::create_options ====================================================

void sim_t::create_options()
{
  option_t global_options[] =
  {
    // General
    opt_int( "iterations", iterations ),
    opt_timespan( "max_time", max_time ),
    opt_bool( "fixed_time", fixed_time ),
    opt_float( "vary_combat_length", vary_combat_length ),
    opt_func( "ptr", parse_ptr ),
    opt_int( "threads", threads ),
    opt_float( "confidence", confidence ),
    opt_func( "spell_query", parse_spell_query ),
    opt_string( "spell_query_xml_output_file", spell_query_xml_output_file_str ),
    opt_func( "item_db_source", parse_item_sources ),
    opt_func( "proxy", parse_proxy ),
    opt_int( "auto_ready_trigger", auto_ready_trigger ),
    // Raid buff overrides
    opt_func( "optimal_raid", parse_optimal_raid ),
    opt_int( "override.attack_haste", overrides.attack_haste ),
    opt_int( "override.attack_power_multiplier", overrides.attack_power_multiplier ),
    opt_int( "override.critical_strike", overrides.critical_strike ),
    opt_int( "override.mastery", overrides.mastery ),
    opt_int( "override.spell_haste", overrides.spell_haste ),
    opt_int( "override.spell_power_multiplier", overrides.spell_power_multiplier ),
    opt_int( "override.stamina", overrides.stamina ),
    opt_int( "override.str_agi_int", overrides.str_agi_int ),
    opt_int( "override.slowed_casting", overrides.slowed_casting ),
    opt_int( "override.magic_vulnerability", overrides.magic_vulnerability ),
    opt_int( "override.ranged_vulnerability", overrides.ranged_vulnerability ),
    opt_int( "override.mortal_wounds", overrides.mortal_wounds ),
    opt_int( "override.physical_vulnerability", overrides.physical_vulnerability ),
    opt_int( "override.weakened_armor", overrides.weakened_armor ),
    opt_int( "override.weakened_blows", overrides.weakened_blows ),
    opt_int( "override.bleeding", overrides.bleeding ),
    opt_func( "override.spell_data", parse_override_spell_data ),
    opt_float( "override.target_health", overrides.target_health ),
    opt_int( "override.stormlash", overrides.stormlash ),
    opt_int( "override.skull_banner", overrides.skull_banner ),
    // Lag
    opt_timespan( "channel_lag", channel_lag ),
    opt_timespan( "channel_lag_stddev", channel_lag_stddev ),
    opt_timespan( "gcd_lag", gcd_lag ),
    opt_timespan( "gcd_lag_stddev", gcd_lag_stddev ),
    opt_timespan( "queue_lag", queue_lag ),
    opt_timespan( "queue_lag_stddev", queue_lag_stddev ),
    opt_timespan( "queue_gcd_reduction", queue_gcd_reduction ),
    opt_bool( "strict_gcd_queue", strict_gcd_queue ),
    opt_timespan( "default_world_lag", world_lag ),
    opt_timespan( "default_world_lag_stddev", world_lag_stddev ),
    opt_timespan( "default_aura_delay", default_aura_delay ),
    opt_timespan( "default_aura_delay_stddev", default_aura_delay_stddev ),
    opt_float( "default_skill", default_skill ),
    opt_timespan( "reaction_time", reaction_time ),
    opt_float( "travel_variance", travel_variance ),
    opt_timespan( "ignite_sampling_delta", ignite_sampling_delta ),
    // Output
    opt_bool( "save_profiles", save_profiles ),
    opt_bool( "default_actions", default_actions ),
    opt_bool( "debug", debug ),
    opt_string( "html", html_file_str ),
    opt_bool( "hosted_html", hosted_html ),
    opt_int( "print_styles", print_styles ),
    opt_string( "xml", xml_file_str ),
    opt_string( "xml_style", xml_stylesheet_file_str ),
    opt_bool( "log", log ),
    opt_string( "output", output_file_str ),
    opt_bool( "save_raid_summary", save_raid_summary ),
    opt_bool( "save_gear_comments", save_gear_comments ),
    // Bloodlust
    opt_int( "bloodlust_percent", bloodlust_percent ),
    opt_timespan( "bloodlust_time", bloodlust_time ),
    // Overrides"
    opt_bool( "override.allow_potions", allow_potions ),
    opt_bool( "override.allow_food", allow_food ),
    opt_bool( "override.allow_flasks", allow_flasks ),
    opt_bool( "override.bloodlust", overrides.bloodlust ),
    // Regen
    opt_timespan( "regen_periodicity", regen_periodicity ),
    // RNG
    opt_bool( "separated_rng", separated_rng ),
    opt_bool( "deterministic_rng", deterministic_rng ),
    opt_bool( "average_range", average_range ),
    opt_bool( "average_gauss", average_gauss ),
    opt_int( "convergence_scale", convergence_scale ),
    // Misc
    opt_list( "party", party_encoding ),
    opt_func( "active", parse_active ),
    opt_int( "armor_update_internval", armor_update_interval ),
    opt_timespan( "aura_delay", aura_delay ),
    opt_int( "seed", seed ),
    opt_float( "wheel_granularity", wheel_granularity ),
    opt_int( "wheel_seconds", wheel_seconds ),
    opt_string( "reference_player", reference_player_str ),
    opt_string( "raid_events", raid_events_str ),
    opt_append( "raid_events+", raid_events_str ),
    opt_func( "fight_style", parse_fight_style ),
    opt_int( "debug_exp", debug_exp ),
    opt_bool( "weapon_speed_scale_factors", weapon_speed_scale_factors ),
    opt_string( "main_target", main_target_str ),
    opt_float( "target_death_pct", target_death_pct ),
    opt_int( "target_level", target_level ),
    opt_int( "target_level+", rel_target_level ),
    opt_string( "target_race", target_race ),
    opt_bool( "challenge_mode", challenge_mode ),
    opt_int( "scale_to_itemlevel", scale_to_itemlevel ),
    // Character Creation
    opt_func( "death_knight", parse_player ),
    opt_func( "deathknight", parse_player ),
    opt_func( "druid", parse_player ),
    opt_func( "hunter", parse_player ),
    opt_func( "mage", parse_player ),
    opt_func( "monk", parse_player ),
    opt_func( "priest", parse_player ),
    opt_func( "paladin", parse_player ),
    opt_func( "rogue", parse_player ),
    opt_func( "shaman", parse_player ),
    opt_func( "warlock", parse_player ),
    opt_func( "warrior", parse_player ),
    opt_func( "enemy", parse_player ),
    opt_func( "pet", parse_player ),
    opt_func( "guardian", parse_player ),
    opt_func( "copy", parse_player ),
    opt_func( "armory", parse_armory ),
    opt_func( "guild", parse_guild ),
    opt_func( "wowhead", parse_armory ),
    opt_func( "chardev", parse_armory ),
    opt_func( "mopdev", parse_armory ),
    opt_func( "mophead", parse_armory ),
    opt_func( "rawr", parse_rawr ),
    opt_func( "wowreforge", parse_armory ),
    opt_func( "http_clear_cache", http::clear_cache ),
    opt_func( "cache_items", parse_cache ),
    opt_func( "cache_players", parse_cache ),
    opt_string( "default_region", default_region_str ),
    opt_string( "default_server", default_server_str ),
    opt_string( "save_prefix", save_prefix_str ),
    opt_string( "save_suffix", save_suffix_str ),
    opt_bool( "save_talent_str", save_talent_str ),
    opt_func( "talent_format", parse_talent_format ),
    // Stat Enchants
    opt_float( "default_enchant_strength", enchant.attribute[ ATTR_STRENGTH  ] ),
    opt_float( "default_enchant_agility", enchant.attribute[ ATTR_AGILITY   ] ),
    opt_float( "default_enchant_stamina", enchant.attribute[ ATTR_STAMINA   ] ),
    opt_float( "default_enchant_intellect", enchant.attribute[ ATTR_INTELLECT ] ),
    opt_float( "default_enchant_spirit", enchant.attribute[ ATTR_SPIRIT    ] ),
    opt_float( "default_enchant_spell_power", enchant.spell_power ),
    opt_float( "default_enchant_attack_power", enchant.attack_power ),
    opt_float( "default_enchant_expertise_rating", enchant.expertise_rating ),
    opt_float( "default_enchant_armor", enchant.bonus_armor ),
    opt_float( "default_enchant_dodge_rating", enchant.dodge_rating ),
    opt_float( "default_enchant_parry_rating", enchant.parry_rating ),
    opt_float( "default_enchant_block_rating", enchant.block_rating ),
    opt_float( "default_enchant_haste_rating", enchant.haste_rating ),
    opt_float( "default_enchant_mastery_rating", enchant.mastery_rating ),
    opt_float( "default_enchant_hit_rating", enchant.hit_rating ),
    opt_float( "default_enchant_crit_rating", enchant.crit_rating ),
    opt_float( "default_enchant_health", enchant.resource[ RESOURCE_HEALTH ] ),
    opt_float( "default_enchant_mana", enchant.resource[ RESOURCE_MANA   ] ),
    opt_float( "default_enchant_rage", enchant.resource[ RESOURCE_RAGE   ] ),
    opt_float( "default_enchant_energy", enchant.resource[ RESOURCE_ENERGY ] ),
    opt_float( "default_enchant_focus", enchant.resource[ RESOURCE_FOCUS  ] ),
    opt_float( "default_enchant_runic", enchant.resource[ RESOURCE_RUNIC_POWER  ] ),
    // Report
    opt_int( "report_precision", report_precision ),
    opt_bool( "report_pets_separately", report_pets_separately ),
    opt_bool( "report_targets", report_targets ),
    opt_bool( "report_details", report_details ),
    opt_bool( "report_raw_abilities", report_raw_abilities ),
    opt_bool( "report_rng", report_rng ),
    opt_bool( "report_overheal", report_overheal ),
    opt_int( "statistics_level", statistics_level ),
    opt_bool( "separate_stats_by_actions", separate_stats_by_actions ),
    opt_bool( "report_raid_summary", report_raid_summary ), // Force reporting of raid summary
    opt_string( "reforge_plot_output_file", reforge_plot_output_file_str ),
    opt_int( "global_item_upgrade_level", global_item_upgrade_level ),
    opt_null()
  };

  option_t::copy( options, global_options );
}

// sim_t::parse_option ======================================================

bool sim_t::parse_option( const std::string& name,
                          const std::string& value )
{
  if ( canceled ) return false;

  if ( active_player )
    if ( option_t::parse( this, active_player -> options, name, value ) )
      return true;

  if ( option_t::parse( this, options, name, value ) )
    return true;

  return false;
}

// sim_t::setup =============================================================

bool sim_t::setup( sim_control_t* c )
{
  // Limitation: setup+execute is a one-way action that cannot be repeated or reset

  control = c;

  if ( ! parent ) cache::advance_era();

  // Global Options
  for ( size_t i=0; i < control -> options.size(); i++ )
  {
    option_tuple_t& o = control -> options[ i ];
    if ( o.scope != "global" ) continue;
    if ( ! parse_option( o.name, o.value ) )
    {
      errorf( "Unknown option \"%s\" with value \"%s\"\n", o.name.c_str(), o.value.c_str() );
      return false;
    }
  }

  // Combat
  // Try very hard to limit this to just what would be displayed on the gui.
  // Super-users can use misc options.
  // xyz = control -> combat.xyz;

  // Players
  for ( size_t i=0; i < control -> players.size(); i++ )
  {
    player_t::create( this, control -> players[ i ] );
  }

  // Player Options
  for ( size_t i=0; i < control -> options.size(); i++ )
  {
    option_tuple_t& o = control -> options[ i ];
    if ( o.scope == "global" ) continue;
    player_t* p = find_player( o.scope );
    if ( p )
    {
      if ( ! option_t::parse( this, p -> options, o.name, o.value ) )
        return false;
    }
    else
    {
      errorf( "sim_t::setup: Unable to locate player %s for option %s with value %s\n", o.scope.c_str(), o.name.c_str(), o.value.c_str() );
      return false;
    }
  }

  if ( player_list.empty() && spell_query == NULL )
  {
    errorf( "Nothing to sim!\n" );
    cancel();
    return false;
  }

  if ( parent )
  {
    debug = 0;
    log = 0;
  }
  else if ( ! output_file_str.empty() )
  {
    FILE* f = io::fopen( output_file_str, "w" );
    if ( f )
    {
      output_file = f;
    }
    else
    {
      errorf( "Unable to open output file '%s'\n", output_file_str.c_str() );
      cancel();
      return false;
    }
  }
  if ( debug )
  {
    log = 1;
    print_options();
  }
  if ( log )
  {
    iterations = 1;
    threads = 1;
  }

  return true;
}

// sim_t::cancel ============================================================

void sim_t::cancel()
{
  if ( canceled ) return;

  if ( current_iteration >= 0 )
  {
    errorf( "Simulation has been canceled after %d iterations! (thread=%d)\n", current_iteration+1, thread_index );
  }
  else
  {
    errorf( "Simulation has been canceled during player setup! (thread=%d)\n", thread_index );
  }
  fflush( output_file );

  canceled = 1;

  for ( size_t i = 0; i < children.size(); i++ )
  {
    children[ i ] -> cancel();
  }
}

// sim_t::progress ==========================================================

double sim_t::progress( int* current,
			int* final )
{
  int total_iterations = iterations;
  for ( size_t i = 0; i < children.size(); i++ )
    total_iterations += children[ i ] -> iterations;
  
  int total_current_iterations = current_iteration;
  for ( size_t i = 0; i < children.size(); i++ )
    total_current_iterations += children[ i ] -> current_iteration;
  
  if( current ) *current = total_current_iterations;
  if( final   ) *final   = total_iterations;

  return total_current_iterations / (double) total_iterations;
}

double sim_t::progress( std::string& phase )
{
  if ( canceled )
  {
    phase = "Canceled";
    return 1.0;
  }

  if ( plot -> num_plot_stats > 0 &&
       plot -> remaining_plot_stats > 0 )
  {
    return plot -> progress( phase );
  }
  else if ( scaling -> calculate_scale_factors &&
            scaling -> num_scaling_stats > 0 &&
            scaling -> remaining_scaling_stats > 0 )
  {
    return scaling -> progress( phase );
  }
  else if ( reforge_plot -> num_stat_combos > 0 )
  {
    return reforge_plot -> progress( phase );
  }
  else if ( current_iteration >= 0 )
  {
    phase = "Simulating";
    return progress();
  }
  else if ( current_slot >= 0 )
  {
    phase = current_name;
    return current_slot / ( double ) SLOT_MAX;
  }

  return 0.0;
}

// sim_t::errorf ============================================================

int sim_t::errorf( const char* format, ... )
{
  std::string p_locale = setlocale( LC_CTYPE, NULL );
  setlocale( LC_CTYPE, "" );

  va_list fmtargs;
  va_start( fmtargs, format );

  char buffer[ 1024 ];
  int retcode = vsnprintf( buffer, sizeof( buffer ), format, fmtargs );

  va_end( fmtargs );
  assert( retcode >= 0 );

  if ( buffer[ strlen( buffer ) ] != '\n' && strlen( buffer ) + 2 <= sizeof( buffer ) )
  {
    buffer[ strlen( buffer ) + 1 ] = '\n';
    buffer[ strlen( buffer ) + 2 ] = 0;
  }

  fputs( buffer, output_file );

  setlocale( LC_CTYPE, p_locale.c_str() );

  if ( thread_index == 0 ) error_list.push_back( buffer );
  return retcode;
}

// FIXME! I am not sure if I want these here.

// sim_t::output ============================================================

void sim_t::output( const char* format, ... )
{
  va_list vap;
  char buffer[2048];

  va_start( vap, format );
  vsnprintf( buffer, sizeof( buffer ),  format, vap );
  va_end( vap );

  util::fprintf( output_file, "%-3.3f %s\n", current_time.total_seconds(), buffer );

  //fflush( sim -> output_file );
}

// sim_t::output ============================================================

void sim_t::output( sim_t* sim, const char* format, ... )
{
  va_list vap;
  char buffer[2048];

  va_start( vap, format );
  vsnprintf( buffer, sizeof( buffer ),  format, vap );
  va_end( vap );

  util::fprintf( sim -> output_file, "%-3.3f %s\n", sim -> current_time.total_seconds(), buffer );

  //fflush( sim -> output_file );
}

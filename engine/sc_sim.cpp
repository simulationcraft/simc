// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#ifdef SC_SIGACTION
#include <signal.h>
#endif

namespace { // UNNAMED NAMESPACE ==========================================

#ifdef SC_SIGACTION
// POSIX-only signal handler ================================================

struct sim_signal_handler_t
{
  static sim_t* global_sim;

  static void sigint( int )
  {
    if ( global_sim )
    {
      if ( global_sim -> canceled ) exit( 0 );
      global_sim -> cancel();
    }
  }

  static void callback( int signal )
  {
    if ( global_sim )
    {
      const char* name = strsignal( signal );
      fprintf( stderr, "sim_signal_handler: %s! Seed=%d Iteration=%d\n",
               name, global_sim -> seed, global_sim -> current_iteration );
      fflush( stderr );
    }
    exit( 0 );
  }

  sim_signal_handler_t( sim_t* sim )
  {
    assert ( ! global_sim );
    global_sim = sim;
    struct sigaction sa;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;

    sa.sa_handler = callback;
    sigaction( SIGSEGV, &sa, 0 );

    sa.sa_handler = sigint;
    sigaction( SIGINT,  &sa, 0 );
  }

  ~sim_signal_handler_t()
  { global_sim = 0; }
};
sim_t* sim_signal_handler_t::global_sim = 0;
#else
struct sim_signal_handler_t
{
  sim_signal_handler_t( sim_t* ) {}
};
#endif

// need_to_save_profiles ====================================================

static bool need_to_save_profiles( sim_t* sim )
{
  if ( sim -> save_profiles ) return true;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( ! p -> report_information.save_str.empty() )
      return true;
  }

  return false;
}

// parse_ptr ================================================================

static bool parse_ptr( sim_t*             sim,
                       const std::string& name,
                       const std::string& value )
{
  if ( name != "ptr" ) return false;

  if ( SC_USE_PTR )
    sim -> dbc.ptr = atoi( value.c_str() ) != 0;
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

  sim -> use_optimal_buffs_and_debuffs( atoi( value.c_str() ) );

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
    module_t* module = module_t::get( name );

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

  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, value, "," );

  if ( num_splits != 3 )
  {
    sim -> errorf( "Expected format is: proxy=type,host,port\n" );
    return false;
  }

  int port = atoi( splits[ 2 ].c_str() );
  if ( splits[ 0 ] == "http" && port > 0 && port < 65536 )
  {
    http::proxy.type = splits[ 0 ];
    http::proxy.host = splits[ 1 ];
    http::proxy.port = port;
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
      { "region", OPT_STRING, &region    },
      { "server", OPT_STRING, &server    },
      { "cache",  OPT_BOOL,   &use_cache },
      { NULL,     OPT_UNKNOWN, NULL      }
    };

    std::vector<option_t> options;
    option_t::merge( options, base_options, client_options );

    size_t n = util::string_split( names, input, "," );

    std::vector<std::string> names2 = names;
    size_t count = 0;
    for ( size_t i = 0; i < n; ++i )
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
        server = sim -> default_server_str;
      }
    }
    else if ( server.empty() )
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
      { "spec", OPT_STRING,  &spec },
      { NULL,   OPT_UNKNOWN, NULL }
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

      if ( ! sim -> input_is_utf8 )
        sim -> input_is_utf8 = range::is_valid_utf8( player_name ) && range::is_valid_utf8( stuff.server );

      player_t* p;
      if ( name == "wowhead" )
        p = wowhead::download_player( sim, stuff.region, stuff.server,
                                      player_name, description, wowhead::LIVE, stuff.cache );
      else if ( name == "mophead" )
        p = wowhead::download_player( sim, stuff.region, stuff.server,
                                      player_name, description, wowhead::MOP, stuff.cache );
      else if ( name == "chardev" )
        p = chardev::download_player( sim, player_name, stuff.cache );
      else if ( name == "mopdev" )
        p = chardev::download_player( sim, player_name, stuff.cache, true );
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
      { "class",    OPT_STRING,  &type_str  },
      { "max_rank", OPT_INT,     &max_rank  },
      { "ranks",    OPT_STRING,  &ranks_str },
      { NULL,       OPT_UNKNOWN, NULL }
    };

    names_and_options_t stuff( sim, name, options, value );

    std::vector<int> ranks_list;
    if ( ! ranks_str.empty() )
    {
      std::vector<std::string> ranks;
      int n_ranks = util::string_split( ranks, ranks_str, "/" );
      if ( n_ranks > 0 )
      {
        for ( int i = 0; i < n_ranks; i++ )
          ranks_list.push_back( atoi( ranks[i].c_str() ) );
      }
    }

    player_e pt = PLAYER_NONE;
    if ( ! type_str.empty() )
      pt = util::parse_player_type( type_str );

    for ( size_t i = 0; i < stuff.names.size(); ++i )
    {
      std::string& guild_name = stuff.names[ i ];
      sim -> input_is_utf8 = range::is_valid_utf8( guild_name ) && range::is_valid_utf8( stuff.server );
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
  std::vector< std::string > splits;
  size_t v_pos = value.find( '=' );

  if ( v_pos == std::string::npos )
    return false;

  util::string_split( splits, value.substr( 0, v_pos ), ".", false );

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

    sim -> spell_query_level = static_cast< unsigned >( sq_lvl );

    sq_str = sq_str.substr( 0, lvl_offset );
  }

  sim -> spell_query = spell_data_expr_t::parse( sim, sq_str );
  return sim -> spell_query != 0;
}

// parse_item_sources =======================================================

static bool parse_item_sources( sim_t*             sim,
                                const std::string& /* name */,
                                const std::string& value )
{
  std::vector<std::string> sources;

  util::string_split( sources, value, ":/|", false );

  sim -> item_db_sources.clear();

  for ( unsigned i = 0; i < sources.size(); i++ )
  {
    if ( ! util::str_compare_ci( sources[ i ], "local" ) &&
         ! util::str_compare_ci( sources[ i ], "wowhead" ) &&
         ! util::str_compare_ci( sources[ i ], "ptrhead" ) &&
         ! util::str_compare_ci( sources[ i ], "mophead" ) &&
         ! util::str_compare_ci( sources[ i ], "armory" ) &&
         ! util::str_compare_ci( sources[ i ], "bcpapi" ) )
    {
      continue;
    }
    util::tokenize( sources[ i ] );
    sim -> item_db_sources.push_back( sources[ i ] );
  }

  if ( sim -> item_db_sources.empty() )
  {
    sim -> errorf( "Your global data source string \"%s\" contained no valid data sources. Valid identifiers are: local, mmoc, wowhead, ptrhead and armory.\n",
                   value.c_str() );
    return false;
  }

  return true;
}

struct sim_end_event_t : event_t
{
  sim_end_event_t( sim_t* s, const char* n, timespan_t end_time ) : event_t( s, 0, n )
  {
    sim -> add_event( this, end_time );
  }
  virtual void execute()
  {
    sim -> iteration_canceled = 1;
  }
};

struct resource_timeline_collect_event_t : public event_t
{
  resource_timeline_collect_event_t( sim_t* s ) :
    event_t( s,0, "resource_timeline_collect_event_t" )
  {
    sim -> add_event( this, timespan_t::from_native( 1000 ) );
  }

  virtual void execute()
  {
    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      if ( p -> current.sleeping ) continue;
      if ( p -> primary_resource() == RESOURCE_NONE ) continue;

      p -> collect_resource_timeline_information();
    }

    new ( sim ) resource_timeline_collect_event_t( sim );
  }
};

struct regen_event_t : public event_t
{
  regen_event_t( sim_t* sim ) : event_t( sim, 0, "Regen Event" )
  {
    if ( sim -> debug ) sim -> output( "New Regen Event" );
    sim -> add_event( this, sim -> regen_periodicity );
  }

  virtual void execute()
  {
    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      if ( p -> current.sleeping ) continue;
      if ( p -> primary_resource() == RESOURCE_NONE ) continue;

      p -> regen( sim -> regen_periodicity );
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
  last_event( timespan_t::zero() ), fixed_time( 0 ),
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
  input_is_utf8( false ), auto_ready_trigger( 0 ),
  target_death( 0 ), target_death_pct( 0 ), rel_target_level( 3 ), target_level( -1 ), target_adds( 0 ),
  healer_sim( false ), tank_sim( false ),
  active_enemies( 0 ), active_allies( 0 ),
  default_rng_( 0 ), deterministic_rng( false ),
  rng( 0 ), _deterministic_rng( 0 ), separated_rng( false ), average_range( true ), average_gauss( false ),
  convergence_scale( 2 ),
  timing_wheel( 0 ), wheel_seconds( 0 ), wheel_size( 0 ), wheel_mask( 0 ), timing_slice( 0 ), wheel_granularity( 0.0 ),
  fight_style( "Patchwerk" ), overrides( overrides_t() ), auras( auras_t() ),
  aura_delay( timespan_t::from_seconds( 0.5 ) ), default_aura_delay( timespan_t::from_seconds( 0.3 ) ),
  default_aura_delay_stddev( timespan_t::from_seconds( 0.05 ) ),
  elapsed_cpu( timespan_t::zero() ), iteration_dmg( 0 ), iteration_heal( 0 ),
  raid_dps( std::string( "raid damage per second" ) ), total_dmg(), raid_hps(std::string( "raid healing per second" )), total_heal(), simulation_length( false ),
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
  report_information( report_information_t() ),
  // Multi-Threading
  threads( 0 ), thread_index( index ),
  spell_query( 0 ), spell_query_level( MAX_LEVEL )
{
  // Initialize the default item database source order
  static const char* const dbsources[] = { "local", "bcpapi", "wowhead", "armory", "ptrhead", "mophead" };
  item_db_sources.assign( range::begin( dbsources ), range::end( dbsources ) );

  scaling = new scaling_t( this );
  plot    = new    plot_t( this );
  reforge_plot = new reforge_plot_t( this );

  use_optimal_buffs_and_debuffs( 1 );

  create_options();

  if ( parent )
  {
    // Inherit setup
    setup( parent -> control );

    // Inherit 'scaling' settings from parent because these are set outside of the config file
    scaling -> scale_stat  = parent -> scaling -> scale_stat;
    scaling -> scale_value = parent -> scaling -> scale_value;

    // Inherit reporting directives from parent
    report_progress = parent -> report_progress;
    output_file     = parent -> output_file;

    // Inherit 'plot' settings from parent because are set outside of the config file
    enchant = parent -> enchant;

    seed = parent -> seed;
  }
}

// sim_t::~sim_t ============================================================

sim_t::~sim_t()
{
  flush_events();

  delete rng;
  delete _deterministic_rng;
  delete scaling;
  delete plot;
  delete reforge_plot;

  delete[] timing_wheel;
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

  if ( e -> time > last_event ) last_event = e -> time;

#ifdef SC_USE_INTEGER_WHEEL_SHIFT
  uint32_t slice = ( uint32_t ) ( e -> time.total_millis() >> SC_USE_INTEGER_WHEEL_SHIFT ) & wheel_mask;
#else
  uint32_t slice = ( uint32_t ) ( e -> time.total_seconds() * wheel_granularity ) & wheel_mask;
#endif

  event_t** prev = &( timing_wheel[ slice ] );

  while ( ( *prev ) && ( *prev ) -> time <= e -> time )
  {
    prev = &( ( *prev ) -> next );
  }

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
    if ( timing_slice == wheel_size )
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

  for ( int i=0; i < wheel_size; i++ )
  {
    while ( event_t* e = timing_wheel[ i ] )
    {
      if ( e -> player && ! e -> canceled )
      {
        // Make sure we dont recancel events, although it should
        // not technically matter
        e -> canceled = 1;
        e -> player -> events--;
#ifndef NDEBUG
        if ( e -> player -> events < 0 )
        {
          errorf( "sim_t::flush_events assertion error! flushing event %s leaves negative event count for user %s.\n", e -> name, e -> player -> name() );
          assert( 0 );
        }
#endif
      }
      timing_wheel[ i ] = e -> next;
      delete e;
    }
  }

  events_remaining = 0;
  events_processed = 0;
  timing_slice = 0;
  id = 0;
}

// sim_t::cancel_events =====================================================

void sim_t::cancel_events( player_t* p )
{
  if ( p -> events <= 0 ) return;

  if ( debug ) output( "Canceling events for player %s, events to cancel %d", p -> name(), p -> events );

#ifdef SC_USE_INTEGER_WHEEL_SHIFT
  int end_slice = ( uint32_t ) ( last_event.total_millis() >> SC_USE_INTEGER_WHEEL_SHIFT ) & wheel_mask;
#else
  int end_slice = ( uint32_t ) ( last_event.total_seconds() * wheel_granularity ) & wheel_mask;
#endif

  // Loop only partial wheel, [current_time..last_event], as that's the range where there
  // are events for actors in the sim
  if ( timing_slice <= end_slice )
  {
    for ( int i = timing_slice; i <= end_slice && p -> events > 0; i++ )
    {
      for ( event_t* e = timing_wheel[ i ]; e && p -> events > 0; e = e -> next )
      {
        if ( e -> player == p )
        {
          if ( ! e -> canceled )
            p -> events--;

          e -> canceled = 1;
        }
      }
    }
  }
  // Loop only partial wheel in two places, as the wheel has wrapped around, but simulation
  // current time is still at the tail-end, [begin_slice..wheel_size[ and [0..last_event]
  else
  {
    for ( int i = timing_slice; i < wheel_size && p -> events > 0; i++ )
    {
      for ( event_t* e = timing_wheel[ i ]; e && p -> events > 0; e = e -> next )
      {
        if ( e -> player == p )
        {
          if ( ! e -> canceled )
            p -> events--;

          e -> canceled = 1;
        }
      }
    }

    for ( int i = 0; i <= end_slice && p -> events > 0; i++ )
    {
      for ( event_t* e = timing_wheel[ i ]; e && p -> events > 0; e = e -> next )
      {
        if ( e -> player == p )
        {
          if ( ! e -> canceled )
            p -> events--;

          e -> canceled = 1;
        }
      }
    }
  }

  assert( p -> events == 0 );
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

    delete e;

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
  last_event = timespan_t::zero();

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

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> combat_begin();

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
    module_t* m = module_t::get( i );
    if ( m ) m -> combat_begin( this );
  }

  raid_event_t::combat_begin( this );

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    p -> combat_begin();
  }
  new ( this ) regen_event_t( this );
  new ( this ) resource_timeline_collect_event_t( this );


  if ( overrides.bloodlust )
  {
    // Setup a periodic check for Bloodlust

    struct bloodlust_check_t : public event_t
    {
      bloodlust_check_t( sim_t* sim ) : event_t( sim, 0, "Bloodlust Check" )
      {
        sim -> add_event( this, timespan_t::from_seconds( 1.0 ) );
      }
      virtual void execute()
      {
        player_t* t = sim -> target;
        if ( ( sim -> bloodlust_percent  > 0                  && t -> health_percentage() <  sim -> bloodlust_percent ) ||
             ( sim -> bloodlust_time     < timespan_t::zero() && t -> time_to_die()       < -sim -> bloodlust_time ) ||
             ( sim -> bloodlust_time     > timespan_t::zero() && sim -> current_time      >  sim -> bloodlust_time ) )
        {
          for ( size_t i = 0; i < sim -> player_list.size(); ++i )
          {
            player_t* p = sim -> player_list[ i ];
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

    new ( this ) bloodlust_check_t( this );
  }
  
  if ( overrides.stormlash )
  {
    struct stormlash_check_t : public event_t
    {
      int uses;
      timespan_t start_time;
      stormlash_check_t( sim_t* sim, int u, timespan_t st, timespan_t interval ) : event_t( sim, 0, "Stormlash Check" ),
        uses( u ), start_time( st )
      {
        sim -> add_event( this, interval );
      }

      virtual void execute()
      {
        timespan_t interval = timespan_t::from_seconds( 0.25 );
        if ( uses == sim -> overrides.stormlash && start_time > timespan_t::zero() )
          interval = timespan_t::from_seconds( 300.0 ) - ( sim -> current_time - start_time );

        if ( sim -> bloodlust_time <= timespan_t::zero() || sim -> bloodlust_time >= timespan_t::from_seconds( 30.0 ) || 
             ( sim -> bloodlust_time > timespan_t::zero() && sim -> bloodlust_time < timespan_t::from_seconds( 30.0 ) && sim -> current_time > sim -> bloodlust_time + timespan_t::from_seconds( 1 ) ) )
        {
          if ( uses == sim -> overrides.stormlash && start_time > timespan_t::zero() && 
            ( sim -> current_time - start_time ) >= timespan_t::from_seconds( 300.0 ) )
          {
            start_time = timespan_t::zero();
            uses = 0;
          }
          
          if ( uses < sim -> overrides.stormlash )
          {
            if ( sim -> debug )
              sim -> output( "Proxy-Stormlash performs stormlash_totem uses=%d total=%d start_time=%f interval=%f", uses, sim -> overrides.stormlash, start_time.total_seconds(), (sim -> current_time - start_time).total_seconds() );

            for ( size_t i = 0; i < sim -> player_list.size(); ++i )
            {
              player_t* p = sim -> player_list[ i ];
              if ( p -> type == PLAYER_GUARDIAN )
                continue;

              p -> buffs.stormlash -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, timespan_t::from_seconds( 10.0 ) );
            }

            uses++;

            if ( start_time == timespan_t::zero() )
              start_time = sim -> current_time;

            interval = timespan_t::from_seconds( 11.0 );
          }
        }
        new ( sim ) stormlash_check_t( sim, uses, start_time, interval );
      }
    };

    new ( this ) stormlash_check_t( this, 0, timespan_t::zero(), timespan_t::from_seconds( 0.25 ) );
  }

  iteration_canceled = 0;

  if ( fixed_time || ( target -> resources.base[ RESOURCE_HEALTH ] == 0 ) )
  {
    new ( this ) sim_end_event_t( this, "sim_end_expected_time", expected_time );
    target_death = -1;
  }
  else
  {
    target_death = target -> resources.max[ RESOURCE_HEALTH ] * target_death_pct / 100.0;
  }
  new ( this ) sim_end_event_t( this, "sim_end_twice_expected_time", expected_time + expected_time );
}

// sim_t::combat_end ========================================================

void sim_t::combat_end()
{
  if ( debug ) output( "Combat End" );

  iteration_timeline.push_back( current_time );
  simulation_length.add( current_time.total_seconds() );

  total_events_processed += events_processed;

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> combat_end();
  }

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    module_t* m = module_t::get( i );
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
    b -> combat_end();
  }

  total_dmg.add( iteration_dmg );
  raid_dps.add( current_time != timespan_t::zero() ? iteration_dmg / current_time.total_seconds() : 0 );
  total_heal.add( iteration_heal );
  raid_hps.add( current_time != timespan_t::zero() ? iteration_heal / current_time.total_seconds() : 0 );

  flush_events();
  
  assert( active_enemies == 0 && active_allies == 0 );
}

// sim_t::init ==============================================================

bool sim_t::init()
{
  if ( seed == 0 ) seed = ( int ) time( NULL );

  rng = rng_t::create( "global", RNG_MERSENNE_TWISTER );
  rng -> seed( seed );

  _deterministic_rng = rng_t::create( "global_deterministic", RNG_MERSENNE_TWISTER );
  _deterministic_rng -> seed( 31459 + thread_index );

  if ( scaling -> smooth_scale_factors &&
       scaling -> scale_stat != STAT_NONE )
  {
    separated_rng = true;
    average_range = true;
    deterministic_rng = true;
  }

  default_rng_ = ( deterministic_rng ? _deterministic_rng : rng );

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
  delete[] timing_wheel;
  timing_wheel= new event_t*[wheel_size];
  memset( timing_wheel,0,sizeof( event_t* )*wheel_size );


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
    target = module_t::enemy() -> create_player( this, "Fluffy_Pillow" );

  { // Determine whether we have healers or tanks.
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

  if( healer_sim )
    heal_target = module_t::heal_enemy() -> create_player( this, "Healing Target" );


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

  raid_event_t::init( this );

  if ( report_precision < 0 ) report_precision = 4;

  iteration_timeline.reserve( iterations );
  raid_dps.reserve( iterations );
  total_dmg.reserve( iterations );
  raid_hps.reserve( iterations );
  total_heal.reserve( iterations );
  simulation_length.reserve( iterations );

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
  simulation_length.analyze( true, true, true, 50 );
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

  confidence_estimator = rng -> stdnormal_inv( 1.0 - ( 1.0 - confidence ) / 2.0 );

  for ( size_t i = 0; i < actor_list.size(); i++ )
    actor_list[ i ] -> analyze( *this );

  range::sort( players_by_dps, compare_dps() );
  range::sort( players_by_hps, compare_hps() );
  range::sort( players_by_name, compare_name() );
  range::sort( targets_by_name, compare_name() );
}

// sim_t::iterate ===========================================================

bool sim_t::iterate()
{
  if ( ! init() ) return false;

  int message_interval = iterations/10;
  int message_index = 10;

  for ( int i=0; i < iterations; i++ )
  {
    if ( canceled )
    {
      iterations = current_iteration + 1;
      break;
    }

    if ( report_progress && ( message_interval > 0 ) && ( i % message_interval == 0 ) && ( message_index > 0 ) )
    {
      util::fprintf( stdout, "%d... ", message_index-- );
      fflush( stdout );
    }
    combat( i );
  }
  if ( report_progress ) util::fprintf( stdout, "\n" );

  reset();

  return true;
}

// sim_t::merge =============================================================

void sim_t::merge( sim_t& other_sim )
{
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
  for ( size_t i = 0; i < children.size(); i++ )
  {
    sim_t* child = children[ i ];
    child -> wait();
    merge( *child );
    delete child;
  }

  children.clear();
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
  cooldown_t* c=0;

  for ( size_t i = 0; i < cooldown_list.size(); ++i )
  {
    cooldown_t* c = cooldown_list[ i ];
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

  return default_rng_ -> gauss( mean, stddev );
}

// sim_t::gauss =============================================================

timespan_t sim_t::gauss( timespan_t mean,
                         timespan_t stddev )
{
  return timespan_t::from_native( gauss( ( double ) timespan_t::to_native( mean ), ( double ) timespan_t::to_native( stddev ) ) );
}

// sim_t::get_rng ===========================================================

rng_t* sim_t::get_rng( const std::string& n, int type )
{
  assert( rng );

  if ( type == RNG_GLOBAL ) return rng;
  if ( type == RNG_DETERMINISTIC ) return _deterministic_rng;

  if ( ! separated_rng ) return default_rng_;

  rng_t* r=0;

  for ( size_t i = 0; i < rng_list.size(); ++i )
  {
    rng_t* r = rng_list[ i ];
    if ( r -> name_str == n )
      return r;
  }

  r = rng_t::create( n, static_cast<rng_e> ( type ) );

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

  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, name_str, "." );

  if ( num_splits == 3 )
  {
    if ( splits[ 0 ] == "aura" )
    {
      buff_t* buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) return 0;
      return buff_t::create_expression( splits[ 1 ], a, splits[ 2 ], buff );
    }
  }
  if ( num_splits >= 3 && splits[ 0 ] == "actors" )
  {
    player_t* actor = sim_t::find_player( splits[ 1 ] );
    if ( ! target ) return 0;
    std::string rest = splits[2];
    for ( int i = 3; i < num_splits; ++i )
      rest += '.' + splits[i];
    return actor -> create_expression( a, rest );
  }
  if ( num_splits >= 2 && splits[ 0 ] == "target" )
  {
    std::string rest = splits[1];
    for ( int i = 2; i < num_splits; ++i )
      rest += '.' + splits[i];
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
    { "iterations",                       OPT_INT,    &( iterations                               ) },
    { "max_time",                         OPT_TIMESPAN, &( max_time                               ) },
    { "fixed_time",                       OPT_BOOL,   &( fixed_time                               ) },
    { "vary_combat_length",               OPT_FLT,    &( vary_combat_length                       ) },
    { "ptr",                              OPT_FUNC,   ( void* ) ::parse_ptr                         },
    { "threads",                          OPT_INT,    &( threads                                  ) },
    { "confidence",                       OPT_FLT,    &( confidence                               ) },
    { "spell_query",                      OPT_FUNC,   ( void* ) ::parse_spell_query                 },
    { "item_db_source",                   OPT_FUNC,   ( void* ) ::parse_item_sources                },
    { "proxy",                            OPT_FUNC,   ( void* ) ::parse_proxy                       },
    { "auto_ready_trigger",               OPT_INT,    &( auto_ready_trigger                       ) },
    // Raid buff overrides
    { "optimal_raid",                     OPT_FUNC,   ( void* ) ::parse_optimal_raid                },
    { "override.attack_haste",            OPT_INT,    &( overrides.attack_haste                   ) },
    { "override.attack_power_multiplier", OPT_INT,    &( overrides.attack_power_multiplier        ) },
    { "override.critical_strike",         OPT_INT,    &( overrides.critical_strike                ) },
    { "override.mastery",                 OPT_INT,    &( overrides.mastery                        ) },
    { "override.spell_haste",             OPT_INT,    &( overrides.spell_haste                    ) },
    { "override.spell_power_multiplier",  OPT_INT,    &( overrides.spell_power_multiplier         ) },
    { "override.stamina",                 OPT_INT,    &( overrides.stamina                        ) },
    { "override.str_agi_int",             OPT_INT,    &( overrides.str_agi_int                    ) },
    { "override.slowed_casting",          OPT_INT,    &( overrides.slowed_casting                 ) },
    { "override.magic_vulnerability",     OPT_INT,    &( overrides.magic_vulnerability            ) },
    { "override.ranged_vulnerability",     OPT_INT,   &( overrides.ranged_vulnerability            ) },
    { "override.mortal_wounds",           OPT_INT,    &( overrides.mortal_wounds                  ) },
    { "override.physical_vulnerability",  OPT_INT,    &( overrides.physical_vulnerability         ) },
    { "override.weakened_armor",          OPT_INT,    &( overrides.weakened_armor                 ) },
    { "override.weakened_blows",          OPT_INT,    &( overrides.weakened_blows                 ) },
    { "override.bleeding",                OPT_INT,    &( overrides.bleeding                       ) },
    { "override.spell_data",              OPT_FUNC,   ( void* ) ::parse_override_spell_data         },
    { "override.target_health",           OPT_FLT,    &( overrides.target_health                  ) },
    { "override.stormlash",               OPT_INT,    &( overrides.stormlash                      ) },
    // Lag
    { "channel_lag",                      OPT_TIMESPAN, &( channel_lag                            ) },
    { "channel_lag_stddev",               OPT_TIMESPAN, &( channel_lag_stddev                     ) },
    { "gcd_lag",                          OPT_TIMESPAN, &( gcd_lag                                ) },
    { "gcd_lag_stddev",                   OPT_TIMESPAN, &( gcd_lag_stddev                         ) },
    { "queue_lag",                        OPT_TIMESPAN, &( queue_lag                              ) },
    { "queue_lag_stddev",                 OPT_TIMESPAN, &( queue_lag_stddev                       ) },
    { "queue_gcd_reduction",              OPT_TIMESPAN, &( queue_gcd_reduction                    ) },
    { "strict_gcd_queue",                 OPT_BOOL,   &( strict_gcd_queue                         ) },
    { "default_world_lag",                OPT_TIMESPAN, &( world_lag                              ) },
    { "default_world_lag_stddev",         OPT_TIMESPAN, &( world_lag_stddev                       ) },
    { "default_aura_delay",               OPT_TIMESPAN, &( default_aura_delay                     ) },
    { "default_aura_delay_stddev",        OPT_TIMESPAN, &( default_aura_delay_stddev              ) },
    { "default_skill",                    OPT_FLT,    &( default_skill                            ) },
    { "reaction_time",                    OPT_TIMESPAN, &( reaction_time                          ) },
    { "travel_variance",                  OPT_FLT,    &( travel_variance                          ) },
    { "ignite_sampling_delta",            OPT_TIMESPAN, &( ignite_sampling_delta                  ) },
    // Output
    { "save_profiles",                    OPT_BOOL,   &( save_profiles                            ) },
    { "default_actions",                  OPT_BOOL,   &( default_actions                          ) },
    { "debug",                            OPT_BOOL,   &( debug                                    ) },
    { "html",                             OPT_STRING, &( html_file_str                            ) },
    { "hosted_html",                      OPT_BOOL,   &( hosted_html                              ) },
    { "print_styles",                     OPT_INT,   &( print_styles                             ) },
    { "xml",                              OPT_STRING, &( xml_file_str                             ) },
    { "xml_style",                        OPT_STRING, &( xml_stylesheet_file_str                  ) },
    { "log",                              OPT_BOOL,   &( log                                      ) },
    { "output",                           OPT_STRING, &( output_file_str                          ) },
    { "save_raid_summary",                OPT_BOOL,   &( save_raid_summary                        ) },
    { "save_gear_comments",               OPT_BOOL,   &( save_gear_comments                       ) },
    // Bloodlust
    { "bloodlust_percent",                OPT_INT,    &( bloodlust_percent                        ) },
    { "bloodlust_time",                   OPT_INT,    &( bloodlust_time                           ) },
    // Overrides"
    { "override.allow_potions",           OPT_BOOL,   &( allow_potions                            ) },
    { "override.allow_food",              OPT_BOOL,   &( allow_food                               ) },
    { "override.allow_flasks",            OPT_BOOL,   &( allow_flasks                             ) },
    { "override.bloodlust",               OPT_BOOL,   &( overrides.bloodlust                      ) },
    // Regen
    { "regen_periodicity",                OPT_TIMESPAN, &( regen_periodicity                      ) },
    // RNG
    { "separated_rng",                    OPT_BOOL,   &( separated_rng                            ) },
    { "deterministic_rng",                OPT_BOOL,   &( deterministic_rng                        ) },
    { "average_range",                    OPT_BOOL,   &( average_range                            ) },
    { "average_gauss",                    OPT_BOOL,   &( average_gauss                            ) },
    { "convergence_scale",                OPT_INT,    &( convergence_scale                        ) },
    // Misc
    { "party",                            OPT_LIST,   &( party_encoding                           ) },
    { "active",                           OPT_FUNC,   ( void* ) ::parse_active                      },
    { "armor_update_internval",           OPT_INT,    &( armor_update_interval                    ) },
    { "aura_delay",                       OPT_TIMESPAN, &( aura_delay                             ) },
    { "seed",                             OPT_INT,    &( seed                                     ) },
    { "wheel_granularity",                OPT_FLT,    &( wheel_granularity                        ) },
    { "wheel_seconds",                    OPT_INT,    &( wheel_seconds                            ) },
    { "reference_player",                 OPT_STRING, &( reference_player_str                     ) },
    { "raid_events",                      OPT_STRING, &( raid_events_str                          ) },
    { "raid_events+",                     OPT_APPEND, &( raid_events_str                          ) },
    { "fight_style",                      OPT_FUNC,   ( void* ) ::parse_fight_style                 },
    { "debug_exp",                        OPT_INT,    &( debug_exp                                ) },
    { "weapon_speed_scale_factors",       OPT_BOOL,   &( weapon_speed_scale_factors               ) },
    { "main_target",                      OPT_STRING, &( main_target_str                          ) },
    { "target_death_pct",                 OPT_FLT,    &( target_death_pct                         ) },
    { "target_level",                     OPT_INT,    &( target_level                             ) },
    { "target_level+",                    OPT_INT,    &( rel_target_level                         ) },
    { "target_race",                      OPT_STRING, &( target_race                              ) },
    { "challenge_mode",                   OPT_BOOL,   &( challenge_mode                           ) },
    // Character Creation
    { "death_knight",                     OPT_FUNC,   ( void* ) ::parse_player                      },
    { "deathknight",                      OPT_FUNC,   ( void* ) ::parse_player                      },
    { "druid",                            OPT_FUNC,   ( void* ) ::parse_player                      },
    { "hunter",                           OPT_FUNC,   ( void* ) ::parse_player                      },
    { "mage",                             OPT_FUNC,   ( void* ) ::parse_player                      },
    { "monk",                             OPT_FUNC,   ( void* ) ::parse_player                      },
    { "priest",                           OPT_FUNC,   ( void* ) ::parse_player                      },
    { "paladin",                          OPT_FUNC,   ( void* ) ::parse_player                      },
    { "rogue",                            OPT_FUNC,   ( void* ) ::parse_player                      },
    { "shaman",                           OPT_FUNC,   ( void* ) ::parse_player                      },
    { "warlock",                          OPT_FUNC,   ( void* ) ::parse_player                      },
    { "warrior",                          OPT_FUNC,   ( void* ) ::parse_player                      },
    { "enemy",                            OPT_FUNC,   ( void* ) ::parse_player                      },
    { "pet",                              OPT_FUNC,   ( void* ) ::parse_player                      },
    { "guardian",                              OPT_FUNC,   ( void* ) ::parse_player                      },
    { "copy",                             OPT_FUNC,   ( void* ) ::parse_player                      },
    { "armory",                           OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "guild",                            OPT_FUNC,   ( void* ) ::parse_guild                       },
    { "wowhead",                          OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "chardev",                          OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "mopdev",                           OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "mophead",                          OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "rawr",                             OPT_FUNC,   ( void* ) ::parse_rawr                        },
    { "wowreforge",                       OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "http_clear_cache",                 OPT_FUNC,   ( void* ) ::http::clear_cache                 },
    { "cache_items",                      OPT_FUNC,   ( void* ) ::parse_cache                       },
    { "cache_players",                    OPT_FUNC,   ( void* ) ::parse_cache                       },
    { "default_region",                   OPT_STRING, &( default_region_str                       ) },
    { "default_server",                   OPT_STRING, &( default_server_str                       ) },
    { "save_prefix",                      OPT_STRING, &( save_prefix_str                          ) },
    { "save_suffix",                      OPT_STRING, &( save_suffix_str                          ) },
    { "save_talent_str",                  OPT_BOOL,   &( save_talent_str                          ) },
    { "talent_format",                    OPT_FUNC,   ( void* ) ::parse_talent_format               },
    // Stat Enchants
    { "default_enchant_strength",                 OPT_FLT,  &( enchant.attribute[ ATTR_STRENGTH  ] ) },
    { "default_enchant_agility",                  OPT_FLT,  &( enchant.attribute[ ATTR_AGILITY   ] ) },
    { "default_enchant_stamina",                  OPT_FLT,  &( enchant.attribute[ ATTR_STAMINA   ] ) },
    { "default_enchant_intellect",                OPT_FLT,  &( enchant.attribute[ ATTR_INTELLECT ] ) },
    { "default_enchant_spirit",                   OPT_FLT,  &( enchant.attribute[ ATTR_SPIRIT    ] ) },
    { "default_enchant_spell_power",              OPT_FLT,  &( enchant.spell_power                 ) },
    { "default_enchant_mp5",                      OPT_FLT,  &( enchant.mp5                         ) },
    { "default_enchant_attack_power",             OPT_FLT,  &( enchant.attack_power                ) },
    { "default_enchant_expertise_rating",         OPT_FLT,  &( enchant.expertise_rating            ) },
    { "default_enchant_armor",                    OPT_FLT,  &( enchant.bonus_armor                 ) },
    { "default_enchant_dodge_rating",             OPT_FLT,  &( enchant.dodge_rating                ) },
    { "default_enchant_parry_rating",             OPT_FLT,  &( enchant.parry_rating                ) },
    { "default_enchant_block_rating",             OPT_FLT,  &( enchant.block_rating                ) },
    { "default_enchant_haste_rating",             OPT_FLT,  &( enchant.haste_rating                ) },
    { "default_enchant_mastery_rating",           OPT_FLT,  &( enchant.mastery_rating              ) },
    { "default_enchant_hit_rating",               OPT_FLT,  &( enchant.hit_rating                  ) },
    { "default_enchant_crit_rating",              OPT_FLT,  &( enchant.crit_rating                 ) },
    { "default_enchant_health",                   OPT_FLT,  &( enchant.resource[ RESOURCE_HEALTH ] ) },
    { "default_enchant_mana",                     OPT_FLT,  &( enchant.resource[ RESOURCE_MANA   ] ) },
    { "default_enchant_rage",                     OPT_FLT,  &( enchant.resource[ RESOURCE_RAGE   ] ) },
    { "default_enchant_energy",                   OPT_FLT,  &( enchant.resource[ RESOURCE_ENERGY ] ) },
    { "default_enchant_focus",                    OPT_FLT,  &( enchant.resource[ RESOURCE_FOCUS  ] ) },
    { "default_enchant_runic",                    OPT_FLT,  &( enchant.resource[ RESOURCE_RUNIC_POWER  ] ) },
    // Report
    { "report_precision",                 OPT_INT,    &( report_precision                         ) },
    { "report_pets_separately",           OPT_BOOL,   &( report_pets_separately                   ) },
    { "report_targets",                   OPT_BOOL,   &( report_targets                           ) },
    { "report_details",                   OPT_BOOL,   &( report_details                           ) },
    { "report_raw_abilities",             OPT_BOOL,   &( report_raw_abilities                     ) },
    { "report_rng",                       OPT_BOOL,   &( report_rng                               ) },
    { "report_overheal",                  OPT_BOOL,   &( report_overheal                          ) },
    { "statistics_level",                 OPT_INT,    &( statistics_level                         ) },
    { "separate_stats_by_actions",        OPT_BOOL,   &( separate_stats_by_actions                ) },
    { "report_raid_summary",              OPT_BOOL,   &( report_raid_summary                      ) }, // Force reporting of raid summary
    { NULL, OPT_UNKNOWN, NULL }
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
    FILE* f = fopen( output_file_str.c_str(), "w" );
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
    return current_iteration / ( double ) iterations;
  }
  else if ( current_slot >= 0 )
  {
    phase = current_name;
    return current_slot / ( double ) SLOT_MAX;
  }

  return 0.0;
}

// sim_t::main ==============================================================

int sim_t::main( const std::vector<std::string>& args )
{
  sim_signal_handler_t handler( this );

  http::cache_load();
  dbc_t::init();

  sim_control_t control;

  if ( ! control.options.parse_args( args ) )
  {
    errorf( "ERROR! Incorrect option format..\n" );
    return 0;
  }
  else if ( ! setup( &control ) )
  {
    errorf( "ERROR! Setup failure...\n" );
    return 0;
  }

  if ( canceled ) return 0;

  util::fprintf( output_file, "\nSimulationCraft %s-%s for World of Warcraft %s %s (build level %s)\n",
                 SC_MAJOR_VERSION, SC_MINOR_VERSION, dbc_t::wow_version( dbc.ptr ), ( dbc.ptr ?
#ifdef SC_BETA
                 "BETA"
#else
                 "PTR"
#endif
                 : "Live" ), dbc_t::build_level( dbc.ptr ) );
  fflush( output_file );

  if ( spell_query )
  {
    spell_query -> evaluate();
    report::print_spell_query( this, spell_query_level );
  }
  else if ( need_to_save_profiles( this ) )
  {
    init();
    util::fprintf( stdout, "\nGenerating profiles... \n" ); fflush( stdout );
    report::print_profiles( this );
  }
  else
  {
    if ( max_time <= timespan_t::zero() )
    {
      util::fprintf( output_file, "simulationcraft: One of -max_time or -target_health must be specified.\n" );
      exit( 0 );
    }
    if ( abs( vary_combat_length ) >= 1.0 )
    {
      util::fprintf( output_file, "\n |vary_combat_length| >= 1.0, overriding to 0.0.\n" );
      vary_combat_length = 0.0;
    }
    if ( confidence <= 0.0 || confidence >= 1.0 )
    {
      util::fprintf( output_file, "\nInvalid confidence, reseting to 0.95.\n" );
      confidence = 0.95;
    }

    util::fprintf( output_file,
                   "\nSimulating... ( iterations=%d, max_time=%.0f, vary_combat_length=%0.2f, optimal_raid=%d, fight_style=%s )\n",
                   iterations, max_time.total_seconds(), vary_combat_length, optimal_raid, fight_style.c_str() );
    fflush( output_file );

    util::fprintf( stdout, "\nGenerating baseline... \n" ); fflush( stdout );

    if ( execute() )
    {
      scaling      -> analyze();
      plot         -> analyze();
      reforge_plot -> analyze();
      util::fprintf( stdout, "\nGenerating reports...\n" ); fflush( stdout );
      report::print_suite( this );
    }
  }

  if ( output_file != stdout ) fclose( output_file );

  http::cache_save();
  dbc_t::de_init();

  return 0;
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

  fputs( buffer, output_file );
  fputc( '\n', output_file );

  setlocale( LC_CTYPE, p_locale.c_str() );

  error_list.push_back( buffer );
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

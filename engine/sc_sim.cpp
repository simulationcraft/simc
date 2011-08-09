// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"
#include "utf8.h"

namespace { // ANONYMOUS NAMESPACE ==========================================


// POSIX-only signal handler ================================================

#if SIGACTION
struct sim_signal_handler_t
{
  static sim_t* global_sim;

  static void callback_func( int signal )
  {
    if( signal == SIGSEGV ||
        signal == SIGBUS  )
    {
      const char* name = ( signal == SIGSEGV ) ? "SIGSEGV" : "SIGBUS";
      if( global_sim )
      {
        fprintf( stderr, "sim_signal_handler:  %s!  Seed=%d  Iteration=%d\n", name, global_sim -> seed, global_sim -> current_iteration );
        fflush( stderr );
      }
      exit( 0 );
    }
    else if( signal == SIGINT )
    {
      if( global_sim )
      {
        if( global_sim -> canceled ) exit( 0 );
        global_sim -> cancel();
      }
    }
  }
  static void init( sim_t* sim )
  {
    global_sim = sim;
    struct sigaction sa;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;
    sa.sa_handler = callback_func;
    sigaction( SIGSEGV, &sa, 0 );
    sigaction( SIGINT,  &sa, 0 );
  }
};
sim_t* sim_signal_handler_t::global_sim = 0;
#else
struct sim_signal_handler_t
{
  static void init( sim_t* sim ) {}
};
#endif

// need_to_save_profiles ====================================================

static bool need_to_save_profiles( sim_t* sim )
{
  if ( sim -> save_profiles ) return true;

  for ( player_t* p = sim -> player_list; p; p = p -> next )
    if ( ! p -> save_str.empty() )
      return true;

  return false;
}

// parse_ptr ================================================================

static bool parse_ptr( sim_t*             sim,
                       const std::string& name,
                       const std::string& value )
{
  if ( name != "ptr" ) return false;

#if SC_USE_PTR
  sim -> dbc.ptr = atoi( value.c_str() ) != 0;
#else
  sim -> errorf( "SimulationCraft has not been built with PTR data.  The 'ptr=' option is ignored.\n" );
#endif

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
  if ( name == "player" )
  {
    std::string player_name = value;
    std::string player_options;

    std::string::size_type cut_pt = value.find_first_of( ',' );

    if ( cut_pt != value.npos )
    {
      player_options = value.substr( cut_pt + 1 );
      player_name    = value.substr( 0, cut_pt );
    }

    std::string wowhead;
    std::string region = sim -> default_region_str;
    std::string server = sim -> default_server_str;
    std::string talents = "active";
    int use_cache=0;

    option_t options[] =
    {
      { "wowhead", OPT_STRING, &wowhead   },
      { "region",  OPT_STRING, &region    },
      { "server",  OPT_STRING, &server    },
      { "talents", OPT_STRING, &talents   },
      { "cache",   OPT_BOOL,   &use_cache },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::parse( sim, "player", options, player_options );

    sim -> input_is_utf8 = utf8::is_valid( player_name.begin(), player_name.end() ) && utf8::is_valid( server.begin(), server.end() );
    cache::behavior_t caching = use_cache ? cache::ANY : cache::behavior();

    if ( wowhead.empty() )
    {
      if ( true )
        sim -> active_player = bcp_api::download_player( sim, region, server, player_name, "active", caching );
      else
      {
        if ( region == "cn" )
        {
          sim -> active_player = armory_t::download_player( sim, region, server, player_name, "active", caching );
        }
        else
        {
          sim -> active_player = battle_net_t::download_player( sim, region, server, player_name, "active", caching );
        }
      }
    }
    else
    {
      sim -> active_player = wowhead_t::download_player( sim, wowhead, ( talents == "active" ), caching );

      if ( sim -> active_player && player_name != sim -> active_player -> name() )
        sim -> errorf( "Mismatch between player name '%s' and wowhead name '%s' for id '%s'\n",
                       player_name.c_str(), sim -> active_player -> name(), wowhead.c_str() );
    }
  }
  else if( name == "pet" )
  {
    std::string pet_name = value;
    std::string pet_type = value;

    std::string::size_type cut_pt = value.find_first_of( "," );
    if ( cut_pt != value.npos )
    {
      pet_type = value.substr( 0, cut_pt );
      pet_name = value.substr( cut_pt + 1 );
    }

    sim -> active_player = sim -> active_player -> create_pet( pet_name, pet_type );
  }
  else if ( name == "copy" )
  {
    std::string::size_type cut_pt = value.find_first_of( ',' );

    player_t* source;
    std::string player_name;

    if ( cut_pt == value.npos )
    {
      source = sim -> active_player;
      player_name = value;
    }
    else
    {
      source = sim -> find_player( value.substr( cut_pt + 1 ) );
      player_name = value.substr( 0, cut_pt );
    }

    if ( source == 0 )
    {
      sim -> errorf( "Invalid source for profile copy - format is copy=target[,source], source defaults to active player." );
      return false;
    }

    sim -> active_player = player_t::create( sim, util_t::player_type_string( source -> type ), player_name );
    if ( sim -> active_player != 0 ) sim -> active_player -> copy_from ( source );
  }
  else
  {
    sim -> active_player = player_t::create( sim, name, value );
  }

  return sim -> active_player != 0;
}

// parse_proxy ==============================================================

static bool parse_proxy( sim_t*             sim,
                         const std::string& name,
                         const std::string& value )
{

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, value, "," );

  if ( num_splits != 3 )
  {
    sim -> errorf( "Expected format is: proxy=type,host,port\n" );
    return false;
  }

  int port = atoi( splits[ 2 ].c_str() );
  if ( splits[ 0 ] == "http" && port > 0 && port < 65536 )
  {
    http_t::proxy_type = splits[ 0 ];
    http_t::proxy_host = splits[ 1 ];
    http_t::proxy_port = port;
    return true;
  }

  return false;
}

// parse_armory =============================================================

static bool parse_cache( sim_t*             sim,
                         const std::string& name,
                         const std::string& value )
{
  if ( name != "cache" ) return false;

  if ( value == "1" ) cache::behavior( cache::ANY );
  else if ( value == "0" )cache::behavior( cache::CURRENT );
  else if ( util_t::str_compare_ci( value, "only" ) ) cache::behavior( cache::ONLY );
  else return false;

  return true;
}

// parse_armory =============================================================

static bool parse_armory( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{
  if ( name == "armory" )
  {
    std::vector<std::string> splits;
    int num_splits = util_t::string_split( splits, value, "," );

    if ( num_splits < 3 )
    {
      sim -> errorf( "Expected format is: armory=region,server,player1,player2,...\n" );
      return false;
    }

    const std::string& region = splits[ 0 ];
    const std::string& server = splits[ 1 ];

    for ( int i=2; i < num_splits; i++ )
    {
      std::string player_name = splits[ i ];
      std::string description = "active";
      if ( player_name[ 0 ] == '!' )
      {
        player_name.erase( 0, 1 );
        description = "inactive";
      }
      std::string::size_type pos = player_name.find( '|' );
      if ( pos != player_name.npos )
      {
        description.assign( player_name, pos + 1, player_name.npos );
        player_name.erase( pos );
      }

      if ( ! sim -> input_is_utf8 )
        sim -> input_is_utf8 = utf8::is_valid( player_name.begin(), player_name.end() ) && utf8::is_valid( server.begin(), server.end() );

      if ( true )
      {
        sim -> active_player = bcp_api::download_player( sim, region, server, player_name, description );
      }
      else if ( region == "cn" )
      {
        sim -> active_player = armory_t::download_player( sim, region, server, player_name, description );
      }
      else
      {
        sim -> active_player = battle_net_t::download_player( sim, region, server, player_name, description );
      }
      if ( ! sim -> active_player ) return false;
    }
    return true;
  }
  else if ( name == "guild" )
  {
    std::string guild_name = value;
    std::string guild_options;
    std::vector<int> ranks_list;
    std::vector<std::string> ranks;

    std::string::size_type cut_pt = value.find_first_of( ',' );

    if ( cut_pt != value.npos )
    {
      guild_options = value.substr( cut_pt + 1 );
      guild_name    = value.substr( 0, cut_pt );
    }

    std::string region = sim -> default_region_str;
    std::string server = sim -> default_server_str;
    std::string type_str;
    std::string ranks_str;
    int max_rank = 0;
    int use_cache = 0;

    option_t options[] =
    {
      { "region",   OPT_STRING, &region    },
      { "server",   OPT_STRING, &server    },
      { "class",    OPT_STRING, &type_str  },
      { "max_rank", OPT_INT,    &max_rank  },
      { "ranks",    OPT_STRING, &ranks_str },
      { "cache",    OPT_BOOL,   &use_cache },
      { NULL, OPT_UNKNOWN, NULL }
    };

    if ( ! option_t::parse( sim, "guild", options, guild_options ) )
      return false;

    if ( ! ranks_str.empty() )
    {
      int n_ranks = util_t::string_split( ranks, ranks_str, "/" );
      if ( n_ranks > 0 )
      {
        for ( int i = 0; i < n_ranks; i++ )
          ranks_list.push_back( atoi( ranks[i].c_str() ) );
      }
    }

    sim -> input_is_utf8 = utf8::is_valid( guild_name.begin(), guild_name.end() ) && utf8::is_valid( server.begin(), server.end() );

    int player_type = PLAYER_NONE;
    if ( ! type_str.empty() ) player_type = util_t::parse_player_type( type_str );

    cache::behavior_t caching = use_cache ? cache::ANY : cache::behavior();

    if ( true )
      return bcp_api::download_guild( sim, region, server, guild_name, ranks_list, player_type, max_rank, caching );

    if ( region == "cn" )
    {
      return armory_t::download_guild( sim, region, server, guild_name, ranks_list, player_type, max_rank, caching );
    }
    else
    {
      return battle_net_t::download_guild( sim, region, server, guild_name, ranks_list, player_type, max_rank, caching );
    }
  }

  return false;
}

// parse_wowhead ============================================================

static bool parse_wowhead( sim_t*             sim,
                           const std::string& name,
                           const std::string& value )
{
  if ( name == "wowhead" )
  {
    std::vector<std::string> splits;
    int num_splits = util_t::string_split( splits, value, ",." );

    if ( num_splits == 1 )
    {
      std::string player_id = splits[ 0 ];
      bool active = true;
      if ( player_id[ 0 ] == '!' )
      {
        player_id.erase( 0, 1 );
        active = false;
      }
      sim -> active_player = wowhead_t::download_player( sim, player_id, active );
    }
    else if ( num_splits >= 3 )
    {
      std::string region = splits[ 0 ];
      std::string server = splits[ 1 ];

      for ( int i=2; i < num_splits; i++ )
      {
        std::string player_name = splits[ i ];
        bool active = true;
        if ( player_name[ 0 ] == '!' )
        {
          player_name.erase( 0, 1 );
          active = false;
        }
        sim -> active_player = wowhead_t::download_player( sim, region, server, player_name, active );
        if ( ! sim -> active_player ) return false;
      }
    }
    else
    {
      sim -> errorf( "Expected format is: wowhead=id OR wowhead=region,server,player1,player2,...\n" );
      return false;
    }
  }

  return sim -> active_player != 0;
}

// parse_chardev ============================================================

static bool parse_chardev( sim_t*             sim,
                           const std::string& name,
                           const std::string& value )
{
  if ( name == "chardev" )
  {
    sim -> active_player = chardev_t::download_player( sim, value );
  }

  return sim -> active_player != 0;
}

// parse_rawr ===============================================================

static bool parse_rawr( sim_t*             sim,
                        const std::string& name,
                        const std::string& value )
{
  if ( name == "rawr" )
  {
    sim -> active_player = rawr_t::load_player( sim, value );
    if ( ! sim -> active_player )
    {
      sim -> errorf( "Unable to parse Rawr Character Save file '%s'\n", value.c_str() );
    }
  }

  return sim -> active_player != 0;
}

// parse_bcp_api ============================================================

static bool parse_bcp_api( sim_t*             sim,
                           const std::string& name,
                           const std::string& value )
{
  if ( name != "bcp" ) return false;

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, value, ",." );

  if ( num_splits < 3 )
  {
    sim -> errorf( "Expected format is: bcp=region,server,player1,player2,...\n" );
    return false;
  }

  const std::string& region = splits[ 0 ];
  const std::string& server = splits[ 1 ];

  for ( int i=2; i < num_splits; i++ )
  {
    std::string talents = "active";

    if ( splits[ i ][ 0 ] == '!' )
    {
      splits[ i ].erase( 0, 1 );
      talents = "inactive";
    }

    std::string::size_type pos = splits[ i ].find('|');
    if ( pos != std::string::npos )
    {
      talents.assign( splits[ i ], pos + 1, std::string::npos );
      splits[ i ].erase( pos );
    }

    sim -> active_player = bcp_api::download_player( sim, region, server, splits[ i ], talents );
    if ( ! sim -> active_player ) return false;
  }

  return sim -> active_player != 0;
}

// parse_fight_style ========================================================

static bool parse_fight_style( sim_t*             sim,
                               const std::string& name,
                               const std::string& value )
{
  if ( name != "fight_style" ) return false;

  if ( util_t::str_compare_ci( value, "Patchwerk" ) )
  {
    sim -> fight_style = "Patchwerk";
    sim -> raid_events_str.clear();
  }
  else if ( util_t::str_compare_ci( value, "HelterSkelter" ) )
  {
    sim -> fight_style = "HelterSkelter";
    sim -> raid_events_str = "casting,cooldown=30,duration=3,first=15";
    sim -> raid_events_str += "/movement,cooldown=30,duration=5";
    sim -> raid_events_str += "/stun,cooldown=60,duration=2";
    sim -> raid_events_str += "/invulnerable,cooldown=120,duration=3";
  }
  else if ( util_t::str_compare_ci( value, "LightMovement" ) )
  {
    sim -> fight_style = "LightMovement";
    sim -> raid_events_str = "/movement,players_only=1,first=53,cooldown=85,duration=7,last=360";
  }
  else if ( util_t::str_compare_ci( value, "HeavyMovement" ) )
  {
    sim -> fight_style = "HeavyMovement";
    sim -> raid_events_str = "/movement,players_only=1,first=10,cooldown=10,duration=4";
  }
  else
  {
    log_t::output( sim, "Custom fight style specified: %s", value.c_str() );
    sim -> fight_style = value;
  }

  return true;
}

// parse_spell_query ========================================================

static bool parse_spell_query( sim_t*             sim,
                               const std::string& name,
                               const std::string& value )
{
  sim -> spell_query = spell_data_expr_t::parse( sim, value );
  return sim -> spell_query > 0;
}

// parse_item_sources =======================================================

static bool parse_item_sources( sim_t*             sim,
                                const std::string& name,
                                const std::string& value )
{
  std::vector<std::string> sources;

  util_t::string_split( sources, value, ":/|", false );

  sim -> item_db_sources.clear();

  for ( unsigned i = 0; i < sources.size(); i++ )
  {
    if ( ! util_t::str_compare_ci( sources[ i ], "local" ) &&
         ! util_t::str_compare_ci( sources[ i ], "mmoc" ) &&
         ! util_t::str_compare_ci( sources[ i ], "wowhead" ) &&
         ! util_t::str_compare_ci( sources[ i ], "ptrhead" ) &&
         ! util_t::str_compare_ci( sources[ i ], "armory" ) )
    {
      continue;
    }

    sim -> item_db_sources.push_back( armory_t::format( sources[ i ] ) );
  }

  if ( sim -> item_db_sources.empty() )
  {
    sim -> errorf( "Your global data source string \"%s\" contained no valid data sources. Valid identifiers are: local, mmoc, wowhead, ptrhead and armory.\n",
                   value.c_str() );
    return false;
  }

  return true;
}

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Simulator
// ==========================================================================

// sim_t::sim_t =============================================================

sim_t::sim_t( sim_t* p, int index ) :
  parent( p ),
  free_list( 0 ), target_list( 0 ), player_list( 0 ), active_player( 0 ), num_players( 0 ), num_enemies( 0 ), max_player_level( -1 ), canceled( 0 ),
  queue_lag( 0.037 ), queue_lag_stddev( 0 ),
  gcd_lag( 0.150 ), gcd_lag_stddev( 0 ),
  channel_lag( 0.250 ), channel_lag_stddev( 0 ),
  queue_gcd_reduction( 0.032 ), strict_gcd_queue( 0 ),
  world_lag( 0.1 ), world_lag_stddev( -1.0 ),
  travel_variance( 0 ), default_skill( 1.0 ), reaction_time( 0.5 ), regen_periodicity( 0.25 ),
  current_time( 0 ), max_time( 450 ), expected_time( 0 ), vary_combat_length( 0.2 ),
  last_event( 0 ), fixed_time( 0 ),
  events_remaining( 0 ), max_events_remaining( 0 ),
  events_processed( 0 ), total_events_processed( 0 ),
  seed( 0 ), id( 0 ), iterations( 1000 ), current_iteration( -1 ), current_slot( -1 ),
  armor_update_interval( 20 ), weapon_speed_scale_factors( 0 ),
  optimal_raid( 0 ), log( 0 ), debug( 0 ), save_profiles( 0 ), default_actions( 0 ),
  normalized_stat( STAT_NONE ),
  default_region_str( "us" ),
  save_prefix_str( "save_" ), save_suffix_str( "" ),
  input_is_utf8( false ), main_target_str( "" ),
  dtr_proc_chance( -1.0 ),
  target_death_pct( 0 ), target_level( -1 ), target_race( "" ), target_adds( 0 ),
  rng( 0 ), deterministic_rng( 0 ), rng_list( 0 ),
  smooth_rng( 0 ), deterministic_roll( 0 ), average_range( 1 ), average_gauss( 0 ), convergence_scale( 2 ),
  timing_wheel( 0 ), wheel_seconds( 0 ), wheel_size( 0 ), wheel_mask( 0 ), timing_slice( 0 ), wheel_granularity( 0.0 ),
  fight_style( "Patchwerk" ), buff_list( 0 ), aura_delay( 0.15 ), default_aura_delay( 0.3 ), default_aura_delay_stddev( 0.05 ),
  cooldown_list( 0 ), replenishment_targets( 0 ),
  raid_dps( 0 ), total_dmg( 0 ), raid_hps( 0 ), total_heal( 0 ),
  total_seconds( 0 ), elapsed_cpu_seconds( 0 ),
  report_progress( 1 ),
  bloodlust_percent( 25 ), bloodlust_time( -60 ),
  path_str( "." ), output_file( stdout ),
  armory_throttle( 5 ), current_throttle( 5 ), debug_exp( 0 ),
  // Report
  report_precision( 4 ),report_pets_separately( 0 ), report_targets( 1 ), report_details( 1 ), report_rng( 0 ), hosted_html( 0 ), print_styles( false ),
  // Multi-Threading
  threads( 0 ), thread_handle( 0 ), thread_index( index ),
  spell_query( 0 )
{
  path_str += "|profiles";
  path_str += "|profiles_heal";
  path_str += "|..";
  path_str += DIRECTORY_DELIMITER;
  path_str += "profiles";
  path_str += "|..";
  path_str += DIRECTORY_DELIMITER;
  path_str += "profiles_heal";

  // Initialize the default item database source order
  const char* dbsources[] = { "local", "wowhead", "mmoc", "armory", "ptrhead" };
  item_db_sources = std::vector<std::string>( dbsources, dbsources + sizeof( dbsources ) / sizeof( const char* ) );

  scaling = new scaling_t( this );
  plot    = new    plot_t( this );
  reforge_plot = new reforge_plot_t( this );

  use_optimal_buffs_and_debuffs( 1 );

  create_options();

  if ( parent )
  {
    // Import the config file
    parse_options( parent -> argc, parent -> argv );

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

  while ( player_t* t = target_list )
  {
    target_list = t -> next;
    delete t;
  }

  while ( player_t* p = player_list )
  {
    player_list = p -> next;
    delete p;
  }

  while ( event_t* e = free_list )
  {
    free_list = e -> next;
    event_t::deallocate( e );
  }

  while ( rng_t* r = rng_list )
  {
    rng_list = r -> next;
    delete r;
  }

  while ( buff_t* b = buff_list )
  {
    buff_list = b -> next;
    delete b;
  }

  while ( cooldown_t* d = cooldown_list )
  {
    cooldown_list = d -> next;
    delete d;
  }

  if ( rng     )           delete rng;
  if ( deterministic_rng ) delete deterministic_rng;
  if ( scaling )           delete scaling;
  if ( plot    )           delete plot;
  if ( reforge_plot )      delete reforge_plot;

  int num_events = ( int ) raid_events.size();
  for ( int i=0; i < num_events; i++ )
  {
    delete raid_events[ i ];
  }

  int num_children = ( int ) children.size();
  for ( int i=0; i < num_children; i++ )
  {
    delete children[ i ];
  }
  if ( timing_wheel ) delete[] timing_wheel;

  if ( spell_query ) delete spell_query;
}

// sim_t::add_event ==========================================================

void sim_t::add_event( event_t* e,
                       double   delta_time )
{
  if ( delta_time <= 0 ) delta_time = 0.0000001;

  e -> time = current_time + delta_time;
  e -> id   = ++id;

  if ( ! ( delta_time <= wheel_seconds ) )
  {
    errorf( "sim_t::add_event assertion error! delta_time > wheel_seconds, event %s from %s.\n", e -> name, e -> player ? e -> player -> name() : "no-one" );
    assert( 0 );
  }

  if ( e -> time > last_event ) last_event = e -> time;

  uint32_t slice = ( uint32_t ) ( e -> time * wheel_granularity ) & wheel_mask;

  event_t** prev = &( timing_wheel[ slice ] );

  while ( ( *prev ) && ( *prev ) -> time < e -> time ) prev = &( ( *prev ) -> next );

  e -> next = *prev;
  *prev = e;

  events_remaining++;
  if ( events_remaining > max_events_remaining ) max_events_remaining = events_remaining;
  if ( e -> player ) e -> player -> events++;

  if ( debug )
  {
    log_t::output( this, "Add Event: %s %.2f %d", e -> name, e -> time, e -> id );
    if ( e -> player ) log_t::output( this, "Actor %s has %d scheduled events", e -> player -> name(), e -> player -> events );
  }
}

// sim_t::reschedule_event ====================================================

void sim_t::reschedule_event( event_t* e )
{
  if ( debug ) log_t::output( this, "Reschedule Event: %s %d", e -> name, e -> id );

  add_event( e, ( e -> reschedule_time - current_time ) );

  e -> reschedule_time = 0;
}

// sim_t::next_event ==========================================================

event_t* sim_t::next_event()
{
  if ( events_remaining == 0 ) return 0;

  while ( 1 )
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
    if ( timing_slice == wheel_size ) timing_slice = 0;
  }

  return 0;
}

// sim_t::flush_events ======================================================

void sim_t::flush_events()
{
  if ( debug ) log_t::output( this, "Flush Events" );

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
        assert( e -> player -> events >= 0 );
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

  if ( debug ) log_t::output( this, "Canceling events for player %s, events to cancel %d", p -> name(), p -> events );

  int begin_slice = ( uint32_t ) ( current_time * wheel_granularity ) & wheel_mask,
        end_slice = ( uint32_t ) ( last_event * wheel_granularity ) & wheel_mask;

  // Loop only partial wheel, [current_time..last_event], as that's the range where there
  // are events for actors in the sim
  if ( begin_slice <= end_slice )
  {
    for ( int i = begin_slice; i <= end_slice && p -> events > 0; i++ )
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
    for ( int i = begin_slice; i < wheel_size && p -> events > 0; i++ )
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

// sim_t::combat ==============================================================

void sim_t::combat( int iteration )
{
  if ( debug ) log_t::output( this, "Starting Simulator" );

  current_iteration = iteration;

  combat_begin();

  while ( event_t* e = next_event() )
  {
    current_time = e -> time;

    // Perform actor event bookkeeping first
    if ( e -> player && ! e -> canceled )
    {
      e -> player -> events--;
      assert( e -> player -> events >= 0 );
    }

    if ( fixed_time || ( target -> resource_base[ RESOURCE_HEALTH ] == 0 ) )
    {
      // The first iteration is always time-limited since we do not yet have inferred health
      if ( current_time > expected_time * ( 1 - target_death_pct / 100.0 ) )
      {
        // Set this last event as canceled, so asserts dont fire when odd things happen at the
        // tail-end of the simulation iteration
        e -> canceled = 1;
        delete e;
        break;
      }
    }
    else
    {
      if ( expected_time > 0 && current_time > ( expected_time * 2.0 ) )
      {
        if ( debug ) log_t::output( this, "Target proving tough to kill, ending simulation" );
        // Set this last event as canceled, so asserts dont fire when odd things happen at the
        // tail-end of the simulation iteration
        e -> canceled = 1;
        delete e;
        break;
      }

      if (  target -> resource_current[ RESOURCE_HEALTH ] / target -> resource_max[ RESOURCE_HEALTH ] <= target_death_pct / 100.0 )
      {
        if ( debug ) log_t::output( this, "Target %s has died, ending simulation", target -> name() );
        // Set this last event as canceled, so asserts dont fire when odd things happen at the
        // tail-end of the simulation iteration
        e -> canceled = 1;
        delete e;
        break;
      }
    }

    if ( e -> canceled )
    {
      if ( debug ) log_t::output( this, "Canceled event: %s", e -> name );
    }
    else if ( e -> reschedule_time > e -> time )
    {
      reschedule_event( e );
      continue;
    }
    else
    {
      if ( debug ) log_t::output( this, "Executing event: %s", e -> name );
      if ( e -> player ) e -> player -> current_time = current_time;
      e -> execute();
    }
    delete e;
  }

  combat_end();
}

// sim_t::reset =============================================================

void sim_t::reset()
{
  if ( debug ) log_t::output( this, "Resetting Simulator" );
  expected_time = max_time * ( 1.0 + vary_combat_length * iteration_adjust() );
  id = 0;
  current_time = last_event = 0;
  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> reset();
  }
  for ( player_t* t = target_list; t; t = t -> next )
  {
    t -> reset();
  }
  for ( player_t* p = player_list; p; p = p -> next )
  {
    p -> reset();
  }
  raid_event_t::reset( this );
}

// sim_t::combat_begin ======================================================

void sim_t::combat_begin()
{
  if ( debug ) log_t::output( this, "Combat Begin" );

  reset();

  for ( player_t* t = target_list; t; t = t -> next )
  {
    t -> combat_begin();
  }

  player_t::combat_begin( this );

  raid_event_t::combat_begin( this );

  for ( player_t* p = player_list; p; p = p -> next )
  {
    p -> combat_begin();
  }
  new ( this ) regen_event_t( this );


  if ( overrides.bloodlust )
  {
    // Setup a periodic check for Bloodlust

    struct bloodlust_check_t : public event_t
    {
      bloodlust_check_t( sim_t* sim ) : event_t( sim, 0 )
      {
        name = "Bloodlust Check";
        sim -> add_event( this, 1.0 );
      }
      virtual void execute()
      {
        player_t* t = sim -> target;
        if ( ( sim -> bloodlust_percent  > 0 && t -> health_percentage() <  sim -> bloodlust_percent ) ||
             ( sim -> bloodlust_time     < 0 && t -> time_to_die()       < -sim -> bloodlust_time ) ||
             ( sim -> bloodlust_time     > 0 && t -> current_time        >  sim -> bloodlust_time ) )
        {
          for ( player_t* p = sim -> player_list; p; p = p -> next )
          {
            if ( p -> sleeping || p -> buffs.exhaustion -> check() )
              continue;

            p -> buffs.bloodlust -> trigger();
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
}

// sim_t::combat_end ========================================================

void sim_t::combat_end()
{
  if ( debug ) log_t::output( this, "Combat End" );

  iteration_timeline.push_back( current_time );

  total_seconds += current_time;
  total_events_processed += events_processed;

  flush_events();

  for ( player_t* t = target_list; t; t = t -> next )
  {
    t -> combat_end();
  }
  player_t::combat_end( this );

  raid_event_t::combat_end( this );

  for ( player_t* p = player_list; p; p = p -> next )
  {
    p -> combat_end();
  }

  for( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> expire();
  }
}

// sim_t::init ==============================================================

bool sim_t::init()
{
  if ( seed == 0 ) seed = ( int ) time( NULL );

  if( ! parent ) srand( seed );

  rng = rng_t::create( this, "global", RNG_MERSENNE_TWISTER );

  deterministic_rng = rng_t::create( this, "global_deterministic", RNG_MERSENNE_TWISTER );
  deterministic_rng -> seed( 31459 + thread_index );

  if ( scaling -> smooth_scale_factors &&
       scaling -> scale_stat != STAT_NONE )
  {
    smooth_rng = 1;
    average_range = 1;
    deterministic_roll = 1;
  }

  // Timing wheel depth defaults to about 17 minutes with a granularity of 32 buckets per second.
  // This makes wheel_size = 32K and it's fully used.
  if ( wheel_seconds     <  600 ) wheel_seconds     = 1024; // 2^10  Min of 600 to ensure no wrap-around bugs with Water Shield
  if ( wheel_granularity <=   0 ) wheel_granularity = 32; // 2^5

  wheel_size = ( uint32_t ) ( wheel_seconds * wheel_granularity );

  // Round up the wheel depth to the nearest power of 2 to enable a fast "mod" operation.
  for ( wheel_mask = 2; wheel_mask < wheel_size; wheel_mask *= 2 ) { continue; }
  wheel_size = wheel_mask;
  wheel_mask--;

  // The timing wheel represents an array of event lists: Each time slice has an event list.
  if ( timing_wheel ) delete [] timing_wheel;
  timing_wheel= new event_t*[wheel_size];
  memset( timing_wheel,0,sizeof( event_t* )*wheel_size );

  total_seconds = 0;

  if (   queue_lag_stddev == 0 )   queue_lag_stddev =   queue_lag * 0.25;
  if (     gcd_lag_stddev == 0 )     gcd_lag_stddev =     gcd_lag * 0.25;
  if ( channel_lag_stddev == 0 ) channel_lag_stddev = channel_lag * 0.25;
  if ( world_lag_stddev    < 0 ) world_lag_stddev   =   world_lag * 0.1;

  // Find Already defined target, otherwise create a new one.
  if ( debug )
    log_t::output( this, "Creating Enemys." );

  if ( target_list )
  {
    target = target_list;
  }
  else if ( ! main_target_str.empty() )
  {
    player_t* p = find_player( main_target_str );
    if ( p )
      target = p;
  }
  else
    target = player_t::create( this, "enemy", "Fluffy_Pillow" );

  // Target overrides
  for ( player_t* t = target_list; t; t = t -> next )
  {
    if ( target_level >= 0 )
      t -> level = target_level;
  }

  if ( max_player_level < 0 )
  {
    for ( player_t* p = player_list; p; p = p -> next )
    {
      if ( p -> is_enemy() || p -> is_add() )
        continue;
      if ( max_player_level < p -> level )
        max_player_level = p -> level;
    }
  }

  if ( ! player_t::init( this ) ) return false;

  // Target overrides 2
  for ( player_t* t = target_list; t; t = t -> next )
  {
    if ( ! target_race.empty() )
    {
      t -> race = util_t::parse_race_type( target_race );
      t -> race_str = util_t::race_type_string( t -> race );
    }
  }

  raid_event_t::init( this );

  if ( report_precision < 0 ) report_precision = 3;

  return canceled ? false : true;
}

// compare_dps ==============================================================

struct compare_dps
{
  bool operator()( player_t* l, player_t* r ) SC_CONST
  {
    return l -> dps > r -> dps;
  }
};

// compare_name =============================================================

struct compare_name
{
  bool operator()( player_t* l, player_t* r ) SC_CONST
  {
    if ( l -> type != r -> type )
    {
      return l -> type < r -> type;
    }
    if ( l -> primary_tree() != r -> primary_tree() )
    {
      return l -> primary_tree() < r -> primary_tree();
    }
    return l -> name_str < r -> name_str;
  }
};

// sim_t::analyze_player ====================================================

void sim_t::analyze_player( player_t* p )
{
  p -> pre_analyze_hook();

  for ( buff_t* b = p -> buff_list; b; b = b -> next )
    b -> analyze();

  p -> total_dmg = 0;
  p -> total_seconds /= iterations;
  p -> total_waiting /= iterations;
  p -> total_foreground_actions /= iterations;
  p -> total_dmg_taken /= iterations;

  std::vector<stats_t*> stats_list;

  for ( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    stats_list.push_back( s );
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    for ( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      stats_list.push_back( s );
    }
  }

  int num_stats = ( int ) stats_list.size();
  for ( int i=0; i < num_stats; i++ )
  {
    stats_t* s = stats_list[ i ];
    bool add_stat = ( ( s -> type == STATS_DMG ) && ( p -> primary_role() != ROLE_HEAL ) ) ||
                    ( ( ( s -> type == STATS_HEAL ) || ( s -> type == STATS_ABSORB ) ) && ( p -> primary_role() == ROLE_HEAL ) );

    s -> analyze();
    if ( add_stat & ! s -> quiet )
      p -> total_dmg += s -> total_dmg;
  }

  p -> dps = p -> total_seconds ? p -> total_dmg / p -> total_seconds : 0;

  if ( p -> total_seconds == 0 ) return;

  for ( int i=0; i < num_stats; i++ )
  {
    stats_t* s = stats_list[ i ];

    s -> portion_dmg = s -> compound_dmg / p -> total_dmg;
    s -> portion_dps = s -> portion_dmg * p -> dps;
  }

  if ( ! p -> quiet && ! p -> is_enemy() && ! p -> is_add() )
  {
    players_by_rank.push_back( p );
    players_by_name.push_back( p );
  }
  if ( ! p -> quiet && ( p -> is_enemy() || p -> is_add() ) )
  {
    targets_by_name.push_back( p );
  }

  // Avoid double-counting of pet damage
  if ( ! p -> is_pet() )
  {
    if ( ! p -> is_enemy() && ! p -> is_add() )
    {
      if ( p -> primary_role() == ROLE_HEAL )
        total_heal += p -> total_dmg;
      else
        total_dmg += p -> total_dmg;
    }
  }

  int max_buckets = ( int ) p -> total_seconds;

  // Make the pet graphs the same length as owner's
  if ( p -> is_pet() )
  {
    player_t* o = p -> cast_pet() -> owner;
    max_buckets = ( int ) o -> total_seconds;
  }

  int num_buckets = ( int ) p -> timeline_resource.size();

  if ( num_buckets > max_buckets ) p -> timeline_resource.resize( max_buckets );

  for ( int i=0; i < max_buckets; i++ )
  {
    p -> timeline_resource[ i ] /= divisor_timeline[ i ];
  }

  num_buckets = ( int ) p -> timeline_health.size();

  if ( num_buckets > max_buckets ) p -> timeline_health.resize( max_buckets );

  for ( int i=0; i < max_buckets; i++ )
  {
    p -> timeline_health[ i ] /= divisor_timeline[ i ];
  }

  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    p -> resource_lost  [ i ] /= iterations;
    p -> resource_gained[ i ] /= iterations;
  }

  p -> dpr = p -> total_dmg / p -> resource_lost[ p -> primary_resource() ];

  p -> rps_loss = p -> resource_lost  [ p -> primary_resource() ] / p -> total_seconds;
  p -> rps_gain = p -> resource_gained[ p -> primary_resource() ] / p -> total_seconds;

  for ( gain_t* g = p -> gain_list; g; g = g -> next )
    g -> analyze( this );

  for ( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
    proc -> analyze( this );

  p -> timeline_dmg.clear();
  p -> timeline_dps.clear();

  p -> timeline_dmg.insert( p -> timeline_dmg.begin(), max_buckets, 0 );
  p -> timeline_dps.insert( p -> timeline_dps.begin(), max_buckets, 0 );

  for ( int i=0; i < num_stats; i++ )
  {
    stats_t* s = stats_list[ i ];
    bool add_stat = ( ( s -> type == STATS_DMG ) && ( p -> primary_role() != ROLE_HEAL ) ) ||
                    ( ( ( s -> type == STATS_HEAL ) || ( s -> type == STATS_ABSORB ) ) && ( p -> primary_role() == ROLE_HEAL ) );
    for ( int j=0; ( j < max_buckets ) && ( j < s -> num_buckets ); j++ )
    {
      if ( add_stat )
        p -> timeline_dmg[ j ] += s -> timeline_dmg[ j ];
    }
  }

  for ( int i=0; i < max_buckets; i++ )
  {
    double window_dmg  = p -> timeline_dmg[ i ];
    int    window_size = 1;

    for ( int j=1; ( j <= 10 ) && ( ( i-j ) >=0 ); j++ )
    {
      window_dmg += p -> timeline_dmg[ i-j ];
      window_size++;
    }
    for ( int j=1; ( j <= 10 ) && ( ( i+j ) < max_buckets ); j++ )
    {
      window_dmg += p -> timeline_dmg[ i+j ];
      window_size++;
    }

    p -> timeline_dps[ i ] = window_dmg / window_size;
  }

  assert( p -> iteration_dps.size() >= ( size_t ) iterations );

  p -> dps_min = +1.0E+50;
  p -> dps_max = -1.0E+50;
  p -> dps_std_dev = 0.0;

  p -> dpse = 0.0;
  for ( int i=0; i < iterations; i++ )
  {
    p -> dpse += p -> iteration_dps[ i ];
  }
  p -> dpse /= iterations;

  for ( int i=0; i < iterations; i++ )
  {
    double i_dps = p -> iteration_dps[ i ];
    if ( p -> dps_min > i_dps ) p -> dps_min = i_dps;
    if ( p -> dps_max < i_dps ) p -> dps_max = i_dps;
    double delta = i_dps - p -> dpse;
    p -> dps_std_dev += delta * delta;
  }

  int    convergence_iterations = 0;
  double convergence_dps = 0;
  double convergence_min = +1.0E+50;
  double convergence_max = -1.0E+50;
  double convergence_std_dev = 0;

  if ( iterations > 1 && convergence_scale > 1 )
  {
    for ( int i=0; i < iterations; i++ )
    {
      if ( ( i % convergence_scale ) == 0 )
      {
        convergence_dps += p -> iteration_dps[ i ];
        convergence_iterations++;
      }
    }
    convergence_dps /= convergence_iterations;

    for ( int i=0; i < iterations; i++ )
    {
      if ( ( i % convergence_scale ) == 0 )
      {
        double i_dps = p -> iteration_dps[ i ];
        if ( convergence_min > i_dps ) convergence_min = i_dps;
        if ( convergence_max < i_dps ) convergence_max = i_dps;
        double delta = i_dps - convergence_dps;
        convergence_std_dev += delta * delta;
      }

      p -> dps_convergence_error.push_back( 0 );
      for ( int j=0; j < i; j++ )
      {
        double j_dps = p -> iteration_dps[ j ];
        double delta = j_dps - convergence_dps;
        p -> dps_convergence_error[i] += delta * delta;
      }
      p -> dps_convergence_error[i] /= i;
      p -> dps_convergence_error[i] = sqrt( p -> dps_convergence_error[i] );
      p -> dps_convergence_error[i] = 2.0 * p -> dps_convergence_error[i] / sqrt ( ( float ) i );
    }
  }

  if ( p -> dps_min >= 1.0E+50 ) p -> dps_min = 0.0;
  if ( p -> dps_max < 0.0      ) p -> dps_max = 0.0;

  if ( iterations > 1 ) p -> dps_std_dev /= ( iterations - 1 );
  p -> dps_std_dev = sqrt( p -> dps_std_dev );
  p -> dps_error = 2.0 * p -> dps_std_dev / sqrt( ( float ) iterations );

  convergence_std_dev /= convergence_iterations;
  convergence_std_dev = sqrt( convergence_std_dev );
  double convergence_error = 2.0 * convergence_std_dev / sqrt( ( float ) convergence_iterations );

  if ( convergence_error > 0 )
  {
    p -> dps_convergence = convergence_error / ( p -> dps_error * convergence_scale );
  }

  if ( ( p -> dps_max - p -> dps_min ) > 0 )
  {
    int num_buckets = 50;
    double min = p -> dps_min - 1;
    double max = p -> dps_max + 1;
    double range = max - min;

    p -> distribution_dps.insert( p -> distribution_dps.begin(), num_buckets, 0 );

    for ( int i=0; i < iterations; i++ )
    {
      double i_dps = p -> iteration_dps[ i ];
      int index = ( int ) ( num_buckets * ( i_dps - min ) / range );
      p -> distribution_dps[ index ]++;
    }
  }

  std::sort( p -> iteration_dps.begin(), p -> iteration_dps.end() );

  p -> dps_10_percentile = p -> iteration_dps[ ( int ) floor( 0.1 * p -> iteration_dps.size() ) ];
  p -> dps_90_percentile = p -> iteration_dps[ ( int ) floor( 0.9 * p -> iteration_dps.size() ) ];

  // Death analysis
  double count_death_time = p -> death_time.size();
  assert ( count_death_time == p -> death_count );
  for ( int i = 0; i < count_death_time; i++ )
  {
    if ( p -> death_time[ i ] < p -> min_death_time )
      p -> min_death_time = p -> death_time[ i ];
    p -> avg_death_time += p -> death_time[ i ];
  }
  p -> avg_death_time /= count_death_time;
  p -> death_count_pct = p -> death_count;
  p -> death_count_pct /= iterations;
  p -> death_count_pct *= 100.0;
}

// sim_t::analyze ===========================================================

void sim_t::analyze()
{
  if ( total_seconds == 0 ) return;

  // divisor_timeline is necessary because not all iterations go the same length of time

  int max_buckets = ( int ) floor( total_seconds / iterations ) + 1;
  divisor_timeline.insert( divisor_timeline.begin(), max_buckets, 0 );

  int num_timelines = iteration_timeline.size();
  for( int i=0; i < num_timelines; i++ )
  {
    int last = ( int ) floor( iteration_timeline[ i ] );
    int num_buckets = divisor_timeline.size();
    int delta = 1 + last - num_buckets;
    if( delta > 0 ) divisor_timeline.insert( divisor_timeline.begin() + num_buckets, delta, 0 );
    for( int j=0; j <= last; j++ ) divisor_timeline[ j ] += 1;
  }

  // buff_t::analyze must be called before total_seconds is normalized via iteration count

  for ( buff_t* b = buff_list; b; b = b -> next )
    b -> analyze();

  total_dmg = 0;
  total_heal = 0;
  total_seconds /= iterations;

  for ( unsigned int i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[i];
    analyze_player( p );
  }

  if ( num_timelines > 2 )
  {
    std::sort( iteration_timeline.begin(), iteration_timeline.end() );

    // Throw out the first and last sample.
    int num_buckets = 50;
    double min = iteration_timeline[ 1 ] - 1;
    double max = iteration_timeline[ num_timelines-2 ] + 1;
    double range = max - min;

    distribution_timeline.insert( distribution_timeline.begin(), num_buckets, 0 );

    for ( int i=1; i < num_timelines-1; i++ )
    {
      int index = ( int ) ( num_buckets * ( iteration_timeline[ i ] - min ) / range );
      distribution_timeline[ index ]++;
    }
  }

  std::sort( players_by_rank.begin(), players_by_rank.end(), compare_dps()  );
  std::sort( players_by_name.begin(), players_by_name.end(), compare_name() );
  std::sort( targets_by_name.begin(), targets_by_name.end(), compare_name() );

  raid_dps = total_dmg / total_seconds;
  raid_hps = total_heal / total_seconds;

  chart_t::raid_dps     ( dps_charts,     this );
  chart_t::raid_dpet    ( dpet_charts,    this );
  chart_t::raid_gear    ( gear_charts,    this );
  chart_t::raid_downtime( downtime_chart, this );
  chart_t::raid_timeline( timeline_chart, this );

  for ( unsigned int i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[i];
    for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
    {
      chart_t::action_dpet        ( pet -> action_dpet_chart,               pet );
      chart_t::action_dmg         ( pet -> action_dmg_chart,                pet );
      chart_t::timeline_resource  ( pet -> timeline_resource_chart,         pet );
      chart_t::timeline_health    ( pet -> timeline_resource_health_chart,  pet );
      chart_t::timeline_dps       ( pet -> timeline_dps_chart,              pet );
      chart_t::timeline_dps_error ( pet -> timeline_dps_error_chart,        pet );
      chart_t::dps_error          ( pet -> dps_error_chart,                 pet );
      chart_t::distribution_dps   ( pet -> distribution_dps_chart,          pet );
    }
    if ( p -> quiet ) continue;

    chart_t::action_dpet        ( p -> action_dpet_chart,               p );
    chart_t::action_dmg         ( p -> action_dmg_chart,                p );
    chart_t::timeline_resource  ( p -> timeline_resource_chart,         p );
    chart_t::timeline_health    ( p -> timeline_resource_health_chart,  p );
    chart_t::timeline_dps       ( p -> timeline_dps_chart,              p );
    chart_t::timeline_dps_error ( p -> timeline_dps_error_chart,        p );
    chart_t::dps_error          ( p -> dps_error_chart,                 p );
    chart_t::distribution_dps   ( p -> distribution_dps_chart,          p );
  }
}

// sim_t::iterate ===========================================================

bool sim_t::iterate()
{
  if( ! init() ) return false;

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
      util_t::fprintf( stdout, "%d... ", message_index-- );
      fflush( stdout );
    }
    combat( i );
  }
  if ( report_progress ) util_t::fprintf( stdout, "\n" );

  reset();

  return true;
}

// sim_t::merge =============================================================

void sim_t::merge( sim_t& other_sim )
{
  iterations             += other_sim.iterations;
  total_seconds          += other_sim.total_seconds;
  total_events_processed += other_sim.total_events_processed;

  if ( max_events_remaining < other_sim.max_events_remaining ) max_events_remaining = other_sim.max_events_remaining;

  for ( int i=0; i < other_sim.iterations; i++ )
  {
    iteration_timeline.push_back( other_sim.iteration_timeline[ i ] );
  }

  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> merge( buff_t::find( &other_sim, b -> name() ) );
  }

  for ( unsigned int i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[i];
    player_t* other_p = other_sim.find_player( p -> index );
    assert( other_p );

    p -> total_seconds += other_p -> total_seconds;
    p -> total_waiting += other_p -> total_waiting;
    p -> total_foreground_actions += other_p -> total_foreground_actions;

    for ( int i=0; i < other_sim.iterations; i++ )
    {
      p -> iteration_dps.push_back( other_p -> iteration_dps[ i ] );
    }

    int num_buckets = ( int ) std::min(       p -> timeline_resource.size(),
                                        other_p -> timeline_resource.size() );

    for ( int i=0; i < num_buckets; i++ )
    {
      p -> timeline_resource[ i ] += other_p -> timeline_resource[ i ];
    }

    num_buckets = ( int ) std::min(       p -> timeline_health.size(),
                                    other_p -> timeline_health.size() );

    for ( int i=0; i < num_buckets; i++ )
    {
      p -> timeline_health[ i ] += other_p -> timeline_health[ i ];
    }

    for ( int i=0; i < RESOURCE_MAX; i++ )
    {
      p -> resource_lost  [ i ] += other_p -> resource_lost  [ i ];
      p -> resource_gained[ i ] += other_p -> resource_gained[ i ];
    }

    for ( buff_t* b = p -> buff_list; b; b = b -> next )
    {
      b -> merge( buff_t::find( other_p, b -> name() ) );
    }

    for ( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
    {
      proc -> merge( other_p -> get_proc( proc -> name_str ) );
    }

    for ( gain_t* gain = p -> gain_list; gain; gain = gain -> next )
    {
      gain -> merge( other_p -> get_gain( gain -> name_str ) );
    }

    for ( stats_t* stats = p -> stats_list; stats; stats = stats -> next )
    {
      stats -> merge( other_p -> get_stats( stats -> name_str ) );
    }

    for ( uptime_t* uptime = p -> uptime_list; uptime; uptime = uptime -> next )
    {
      uptime -> merge( other_p -> get_uptime( uptime -> name_str ) );
    }

    // this will likely crash with low iterations (if action maps don't match across threads) :)

    std::map<std::string,int>::const_iterator it1 = p -> action_map.begin();
    std::map<std::string,int>::const_iterator end1 = p -> action_map.end();
    std::map<std::string,int>::const_iterator it2 = other_p -> action_map.begin();

    while ( it1 != end1 )
    {
      p -> action_map[ it1 -> first ] += it2 -> second;
      it1++;
      it2++;
    }
  }
}

// sim_t::merge =============================================================

void sim_t::merge()
{
  int num_children = ( int ) children.size();

  for ( int i=0; i < num_children; i++ )
  {
    sim_t* child = children[ i ];
    thread_t::wait( child );
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
  util_t::fprintf( output_file, "simulationcraft: This executable was built without thread support, please remove 'threads=N' from config file.\n" );
  exit( 0 );
#endif

  iterations /= threads;

  int num_children = threads - 1;
  children.resize( num_children );

  for ( int i=0; i < num_children; i++ )
  {
    sim_t* child = children[ i ] = new sim_t( this, i+1 );
    child -> iterations /= threads;
    child -> report_progress = 0;
  }

  for ( int i=0; i < num_children; i++ )
  {
    thread_t::launch( children[ i ] );
  }
}

// sim_t::execute ===========================================================

bool sim_t::execute()
{
  int64_t start_time = util_t::milliseconds();

  partition();
  if( ! iterate() ) return false;
  merge();
  analyze();

  elapsed_cpu_seconds = ( util_t::milliseconds() - start_time ) / 1000.0;

  return true;
}

// sim_t::find_player =======================================================

player_t* sim_t::find_player( const std::string& name )
{
  for ( unsigned int i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[i];
    if ( name == p -> name() ) return p;
  }
  return 0;
}

// sim_t::find_player =======================================================

player_t* sim_t::find_player( int index )
{
  for ( unsigned int i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[i];
    if ( index == p -> index ) return p;
  }
  return 0;
}

// sim_t::get_cooldown ===================================================

cooldown_t* sim_t::get_cooldown( const std::string& name )
{
  cooldown_t* c=0;

  for ( c = cooldown_list; c; c = c -> next )
  {
    if ( c -> name_str == name )
      return c;
  }

  c = new cooldown_t( name, this );

  cooldown_t** tail = &cooldown_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  c -> next = *tail;
  *tail = c;

  return c;
}

// sim_t::use_optimal_buffs_and_debuffs =====================================

void sim_t::use_optimal_buffs_and_debuffs( int value )
{
  optimal_raid = value;

  overrides.abominations_might     = optimal_raid;
  overrides.arcane_brilliance      = optimal_raid;
  overrides.arcane_tactics         = optimal_raid;
  overrides.battle_shout           = optimal_raid;
  overrides.bleeding               = optimal_raid;
  overrides.blessing_of_kings      = optimal_raid;
  overrides.blessing_of_might      = optimal_raid;
  overrides.blood_frenzy_bleed     = optimal_raid;
  overrides.blood_frenzy_physical  = optimal_raid;
  overrides.bloodlust              = optimal_raid;
  overrides.communion              = optimal_raid;
  overrides.corrosive_spit         = optimal_raid;
  overrides.critical_mass          = optimal_raid;
  overrides.curse_of_elements      = optimal_raid;
  overrides.demonic_pact           = optimal_raid;
  overrides.demoralizing_roar      = optimal_raid;
  overrides.demoralizing_screech   = optimal_raid;
  overrides.demoralizing_shout     = optimal_raid;
  overrides.devotion_aura          = optimal_raid;
  overrides.earth_and_moon         = optimal_raid;
  overrides.ebon_plaguebringer     = optimal_raid;
  overrides.elemental_oath         = optimal_raid;
  overrides.expose_armor           = optimal_raid;
  overrides.faerie_fire            = optimal_raid;
  overrides.fel_intelligence       = optimal_raid;
  overrides.ferocious_inspiration  = optimal_raid;
  overrides.flametongue_totem      = optimal_raid;
  overrides.fortitude              = optimal_raid;
  overrides.hemorrhage             = optimal_raid;
  overrides.honor_among_thieves    = optimal_raid;
  overrides.horn_of_winter         = optimal_raid;
  overrides.hunters_mark           = optimal_raid;
  overrides.improved_icy_talons    = optimal_raid;
  overrides.hunting_party          = optimal_raid;
  overrides.roar_of_courage        = optimal_raid;
  overrides.shadow_and_flame       = optimal_raid;
  overrides.infected_wounds        = optimal_raid;
  overrides.judgements_of_the_just = optimal_raid;
  overrides.leader_of_the_pack     = optimal_raid;
  overrides.lightning_breath       = optimal_raid;
  overrides.mana_spring_totem      = optimal_raid;
  overrides.mangle                 = optimal_raid;
  overrides.mark_of_the_wild       = optimal_raid;
  overrides.master_poisoner        = optimal_raid;
  overrides.moonkin_aura           = optimal_raid;
  overrides.poisoned               = optimal_raid;
  overrides.qiraji_fortitude       = optimal_raid;
  overrides.rampage                = optimal_raid;
  overrides.ravage                 = optimal_raid;
  overrides.replenishment          = optimal_raid;
  overrides.savage_combat          = optimal_raid;
  overrides.scarlet_fever          = optimal_raid;
  overrides.strength_of_earth      = optimal_raid;
  overrides.sunder_armor           = optimal_raid;
  overrides.tailspin               = optimal_raid;
  overrides.tear_armor             = optimal_raid;
  overrides.tendon_rip             = optimal_raid;
  overrides.thunder_clap           = optimal_raid;
  overrides.trueshot_aura          = optimal_raid;
  overrides.unleashed_rage         = optimal_raid;
  overrides.vindication            = optimal_raid;
  overrides.windfury_totem         = optimal_raid;
  overrides.wrath_of_air           = optimal_raid;
}

// sim_t::aura_gain =========================================================

void sim_t::aura_gain( const char* aura_name , int aura_id )
{
  if( log ) log_t::output( this, "Raid gains %s", aura_name );
}

// sim_t::aura_loss =========================================================

void sim_t::aura_loss( const char* aura_name , int aura_id )
{
  if( log ) log_t::output( this, "Raid loses %s", aura_name );
}

// sim_t::time_to_think =====================================================

bool sim_t::time_to_think( double proc_time )
{
  if ( proc_time == 0 ) return false;
  if ( proc_time < 0 ) return true;
  return current_time - proc_time > reaction_time;
}

// sim_t::roll ==============================================================

int sim_t::roll( double chance )
{
  rng_t* r = ( deterministic_roll ? deterministic_rng : rng );

  return r -> roll( chance );
}

// sim_t::range =============================================================

double sim_t::range( double min,
                     double max )
{
  if ( average_range ) return ( min + max ) / 2.0;

  rng_t* r = ( deterministic_roll ? deterministic_rng : rng );

  return r -> range( min, max );
}

// sim_t::gauss =============================================================

double sim_t::gauss( double mean,
                     double stddev )
{
  if ( average_gauss ) return mean;

  rng_t* r = ( deterministic_roll ? deterministic_rng : rng );

  return r -> gauss( mean, stddev );
}

// sim_t::real ==============================================================

double sim_t::real()
{
  rng_t* r = ( deterministic_roll ? deterministic_rng : rng );

  return r -> real();
}

// sim_t::get_rng ===========================================================

rng_t* sim_t::get_rng( const std::string& n, int type )
{
  assert( rng );

  if ( type == RNG_GLOBAL ) return rng;
  if ( type == RNG_DETERMINISTIC ) return deterministic_rng;

  if ( ! smooth_rng ) return ( deterministic_roll ? deterministic_rng : rng );

  rng_t* r=0;

  for ( r = rng_list; r; r = r -> next )
  {
    if ( r -> name_str == n )
      return r;
  }

  r = rng_t::create( this, n, type );
  r -> next = rng_list;
  rng_list = r;

  return r;
}

// sim_t::iteration_adjust ==================================================

double sim_t::iteration_adjust()
{
  if ( iterations <= 1 )
    return 0.0;

  if ( current_iteration == 0 )
    return 0.0;

  return ( 2.0 * current_iteration / ( double ) iterations ) - 1.0;
}

// sim_t::create_expression =================================================

action_expr_t* sim_t::create_expression( action_t* a,
                                         const std::string& name_str )
{
  if ( name_str == "time" )
  {
    struct time_expr_t : public action_expr_t
    {
      time_expr_t( action_t* a ) : action_expr_t( a, "time", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> sim -> current_time;  return TOK_NUM; }
    };
    return new time_expr_t( a );
  }

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, name_str, "." );

  if ( num_splits == 2 )
  {
    if ( splits[ 0 ] == "target" )
    {
      if ( a -> target )
        return a -> target -> create_expression( a, splits[ 1 ] );
      else
        return target -> create_expression( a, splits[ 1 ] );
    }
  }
  else if ( num_splits == 3 )
  {
    if ( splits[ 0 ] == "aura" )
    {
      buff_t* buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) return 0;
      return buff -> create_expression( a, splits[ 2 ] );
    }
    if ( splits[ 0 ] == "target" )
    {
      player_t* target = sim_t::find_player( splits[ 1 ] );
      if ( ! target ) return 0;
      return target -> create_expression( a, splits[ 2 ] );
    }
  }

  return 0;
}

// sim_t::print_options =====================================================

void sim_t::print_options()
{
  util_t::fprintf( output_file, "\nWorld of Warcraft Raid Simulator Options:\n" );

  int num_options = ( int ) options.size();

  util_t::fprintf( output_file, "\nSimulation Engine:\n" );
  for ( int i=0; i < num_options; i++ ) options[ i ].print( output_file );

  for ( player_t* p = player_list; p; p = p -> next )
  {
    num_options = ( int ) p -> options.size();

    util_t::fprintf( output_file, "\nPlayer: %s (%s)\n", p -> name(), util_t::player_type_string( p -> type ) );
    for ( int i=0; i < num_options; i++ ) p -> options[ i ].print( output_file );
  }

  util_t::fprintf( output_file, "\n" );
  fflush( output_file );
}

// sim_t::create_options ====================================================

void sim_t::create_options()
{
  option_t global_options[] =
  {
    // General
    { "iterations",                       OPT_INT,    &( iterations                               ) },
    { "max_time",                         OPT_FLT,    &( max_time                                 ) },
    { "fixed_time",                       OPT_BOOL,   &( fixed_time                               ) },
    { "vary_combat_length",               OPT_FLT,    &( vary_combat_length                       ) },
    { "optimal_raid",                     OPT_FUNC,   ( void* ) ::parse_optimal_raid                },
    { "ptr",                              OPT_FUNC,   ( void* ) ::parse_ptr                         },
    { "threads",                          OPT_INT,    &( threads                                  ) },

    { "spell_query",                      OPT_FUNC,   ( void* ) ::parse_spell_query                 },
    { "item_db_source",                   OPT_FUNC,   ( void* ) ::parse_item_sources                },
    { "proxy",                            OPT_FUNC,   ( void* ) ::parse_proxy                       },
    // Lag
    { "channel_lag",                      OPT_FLT,    &( channel_lag                              ) },
    { "channel_lag_stddev",               OPT_FLT,    &( channel_lag_stddev                       ) },
    { "gcd_lag",                          OPT_FLT,    &( gcd_lag                                  ) },
    { "gcd_lag_stddev",                   OPT_FLT,    &( gcd_lag_stddev                           ) },
    { "queue_lag",                        OPT_FLT,    &( queue_lag                                ) },
    { "queue_lag_stddev",                 OPT_FLT,    &( queue_lag_stddev                         ) },
    { "queue_gcd_reduction",              OPT_FLT,    &( queue_gcd_reduction                      ) },
    { "strict_gcd_queue",                 OPT_BOOL,   &( strict_gcd_queue                         ) },
    { "default_world_lag",                OPT_FLT,    &( world_lag                                ) },
    { "default_world_lag_stddev",         OPT_FLT,    &( world_lag_stddev                         ) },
    { "default_aura_delay",               OPT_FLT,    &( default_aura_delay                       ) },
    { "default_aura_delay_stddev",        OPT_FLT,    &( default_aura_delay_stddev                ) },
    { "default_skill",                    OPT_FLT,    &( default_skill                            ) },
    { "reaction_time",                    OPT_FLT,    &( reaction_time                            ) },
    { "travel_variance",                  OPT_FLT,    &( travel_variance                          ) },
    // Output
    { "save_profiles",                    OPT_BOOL,   &( save_profiles                            ) },
    { "default_actions",                  OPT_BOOL,   &( default_actions                          ) },
    { "debug",                            OPT_BOOL,   &( debug                                    ) },
    { "html",                             OPT_STRING, &( html_file_str                            ) },
    { "hosted_html",                      OPT_BOOL,   &( hosted_html                              ) },
    { "print_styles",                     OPT_BOOL,   &( print_styles                             ) },
    { "xml",                              OPT_STRING, &( xml_file_str                             ) },
    { "log",                              OPT_BOOL,   &( log                                      ) },
    { "output",                           OPT_STRING, &( output_file_str                          ) },
    { "path",                             OPT_STRING, &( path_str                                 ) },
    { "path+",                            OPT_APPEND, &( path_str                                 ) },
    // Overrides"
    { "override.abominations_might",      OPT_BOOL,   &( overrides.abominations_might             ) },
    { "override.arcane_brilliance",       OPT_BOOL,   &( overrides.arcane_brilliance              ) },
    { "override.arcane_tactics",          OPT_BOOL,   &( overrides.arcane_tactics                 ) },
    { "override.battle_shout",            OPT_BOOL,   &( overrides.battle_shout                   ) },
    { "override.bleeding",                OPT_BOOL,   &( overrides.bleeding                       ) },
    { "override.blessing_of_kings",       OPT_BOOL,   &( overrides.blessing_of_kings              ) },
    { "override.blessing_of_might",       OPT_BOOL,   &( overrides.blessing_of_might              ) },
    { "override.blood_frenzy_bleed",      OPT_BOOL,   &( overrides.blood_frenzy_bleed             ) },
    { "override.blood_frenzy_physical",   OPT_BOOL,   &( overrides.blood_frenzy_physical          ) },
    { "override.bloodlust",               OPT_BOOL,   &( overrides.bloodlust                      ) },
    { "bloodlust_percent",                OPT_INT,    &( bloodlust_percent                        ) },
    { "bloodlust_time",                   OPT_INT,    &( bloodlust_time                           ) },
    { "override.communion",               OPT_BOOL,   &( overrides.communion                      ) },
    { "override.corrosive_spit",          OPT_BOOL,   &( overrides.corrosive_spit                 ) },
    { "override.corruption_absolute",     OPT_BOOL,   &( overrides.corruption_absolute            ) },
    { "override.critical_mass",           OPT_BOOL,   &( overrides.critical_mass                  ) },
    { "override.curse_of_elements",       OPT_BOOL,   &( overrides.curse_of_elements              ) },
    { "override.dark_intent",             OPT_BOOL,   &( overrides.dark_intent                    ) },
    { "override.demonic_pact",            OPT_BOOL,   &( overrides.demonic_pact                   ) },
    { "override.demoralizing_roar",       OPT_BOOL,   &( overrides.demoralizing_roar              ) },
    { "override.demoralizing_screech",    OPT_BOOL,   &( overrides.demoralizing_screech           ) },
    { "override.demoralizing_shout",      OPT_BOOL,   &( overrides.demoralizing_shout             ) },
    { "override.devotion_aura",           OPT_BOOL,   &( overrides.devotion_aura                  ) },
    { "override.earth_and_moon",          OPT_BOOL,   &( overrides.earth_and_moon                 ) },
    { "override.ebon_plaguebringer",      OPT_BOOL,   &( overrides.ebon_plaguebringer             ) },
    { "override.elemental_oath",          OPT_BOOL,   &( overrides.elemental_oath                 ) },
    { "override.essence_of_the_red",      OPT_BOOL,   &( overrides.essence_of_the_red             ) },
    { "override.expode_armor",            OPT_BOOL,   &( overrides.expose_armor                   ) },
    { "override.faerie_fire",             OPT_BOOL,   &( overrides.faerie_fire                    ) },
    { "override.ferocious_inspiration",   OPT_BOOL,   &( overrides.ferocious_inspiration          ) },
    { "override.flametongue_totem",       OPT_BOOL,   &( overrides.flametongue_totem              ) },
    { "override.focus_magic",             OPT_BOOL,   &( overrides.focus_magic                    ) },
    { "override.fortitude",               OPT_BOOL,   &( overrides.fortitude                      ) },
    { "override.hemorrhage",              OPT_BOOL,   &( overrides.hemorrhage                     ) },
    { "override.honor_among_thieves",     OPT_BOOL,   &( overrides.honor_among_thieves            ) },
    { "override.horn_of_winter",          OPT_BOOL,   &( overrides.horn_of_winter                 ) },
    { "override.hellscreams_warsong",     OPT_BOOL,   &( overrides.hellscreams_warsong            ) },
    { "override.hunters_mark",            OPT_BOOL,   &( overrides.hunters_mark                   ) },
    { "override.improved_icy_talons",     OPT_BOOL,   &( overrides.improved_icy_talons            ) },
    { "override.hunting_party",           OPT_BOOL,   &( overrides.hunting_party                  ) },
    { "override.shadow_and_flame",        OPT_BOOL,   &( overrides.shadow_and_flame               ) },
    { "override.infected_wounds",         OPT_BOOL,   &( overrides.infected_wounds                ) },
    { "override.judgements_of_the_just",  OPT_BOOL,   &( overrides.judgements_of_the_just         ) },
    { "override.leader_of_the_pack",      OPT_BOOL,   &( overrides.leader_of_the_pack             ) },
    { "override.lightning_breath",        OPT_BOOL,   &( overrides.lightning_breath               ) },
    { "override.mana_spring_totem",       OPT_BOOL,   &( overrides.mana_spring_totem              ) },
    { "override.mangle",                  OPT_BOOL,   &( overrides.mangle                         ) },
    { "override.mark_of_the_wild",        OPT_BOOL,   &( overrides.mark_of_the_wild               ) },
    { "override.master_poisoner",         OPT_BOOL,   &( overrides.master_poisoner                ) },
    { "override.moonkin_aura",            OPT_BOOL,   &( overrides.moonkin_aura                   ) },
    { "override.poisoned",                OPT_BOOL,   &( overrides.poisoned                       ) },
    { "override.qiraji_fortitude",        OPT_BOOL,   &( overrides.qiraji_fortitude               ) },
    { "override.rampage",                 OPT_BOOL,   &( overrides.rampage                        ) },
    { "override.ravage",                  OPT_BOOL,   &( overrides.ravage                         ) },
    { "override.replenishment",           OPT_BOOL,   &( overrides.replenishment                  ) },
    { "override.roar_of_courage",         OPT_BOOL,   &( overrides.roar_of_courage                ) },
    { "override.savage_combat",           OPT_BOOL,   &( overrides.savage_combat                  ) },
    { "override.scarlet_fever",           OPT_BOOL,   &( overrides.scarlet_fever                  ) },
    { "override.strength_of_earth",       OPT_BOOL,   &( overrides.strength_of_earth              ) },
    { "override.strength_of_wrynn",       OPT_BOOL,   &( overrides.strength_of_wrynn              ) },
    { "override.sunder_armor",            OPT_BOOL,   &( overrides.sunder_armor                   ) },
    { "override.tailspin",                OPT_BOOL,   &( overrides.tailspin                       ) },
    { "override.tear_armor",              OPT_BOOL,   &( overrides.tear_armor                     ) },
    { "override.tendon_rip",              OPT_BOOL,   &( overrides.tendon_rip                     ) },
    { "override.thunder_clap",            OPT_BOOL,   &( overrides.thunder_clap                   ) },
    { "override.trueshot_aura",           OPT_BOOL,   &( overrides.trueshot_aura                  ) },
    { "override.unleashed_rage",          OPT_BOOL,   &( overrides.unleashed_rage                 ) },
    { "override.vindication",             OPT_BOOL,   &( overrides.vindication                    ) },
    { "override.windfury_totem",          OPT_BOOL,   &( overrides.windfury_totem                 ) },
    { "override.wrath_of_air",            OPT_BOOL,   &( overrides.wrath_of_air                   ) },
    // Regen
    { "regen_periodicity",                OPT_FLT,    &( regen_periodicity                        ) },
    // RNG
    { "smooth_rng",                       OPT_BOOL,   &( smooth_rng                               ) },
    { "deterministic_roll",               OPT_BOOL,   &( deterministic_roll                       ) },
    { "average_range",                    OPT_BOOL,   &( average_range                            ) },
    { "average_gauss",                    OPT_BOOL,   &( average_gauss                            ) },
    { "convergence_scale",                OPT_INT,    &( convergence_scale                        ) },
    // Misc
    { "party",                            OPT_LIST,   &( party_encoding                           ) },
    { "active",                           OPT_FUNC,   ( void* ) ::parse_active                      },
    { "armor_update_internval",           OPT_INT,    &( armor_update_interval                    ) },
    { "aura_delay",                       OPT_FLT,    &( aura_delay                               ) },
    { "replenishment_targets",            OPT_INT,    &( replenishment_targets                    ) },
    { "seed",                             OPT_INT,    &( seed                                     ) },
    { "wheel_granularity",                OPT_FLT,    &( wheel_granularity                        ) },
    { "wheel_seconds",                    OPT_INT,    &( wheel_seconds                            ) },
    { "armory_throttle",                  OPT_INT,    &( armory_throttle                          ) },
    { "reference_player",                 OPT_STRING, &( reference_player_str                     ) },
    { "raid_events",                      OPT_STRING, &( raid_events_str                          ) },
    { "raid_events+",                     OPT_APPEND, &( raid_events_str                          ) },
    { "fight_style",                      OPT_FUNC,   ( void* ) ::parse_fight_style                 },
    { "debug_exp",                        OPT_INT,    &( debug_exp                                ) },
    { "weapon_speed_scale_factors",       OPT_BOOL,   &( weapon_speed_scale_factors               ) },
    { "main_target",                      OPT_STRING, &( main_target_str                          ) },
    { "default_dtr_proc_chance",          OPT_FLT,    &( dtr_proc_chance                          ) },
    { "target_death_pct",                 OPT_FLT,    &( target_death_pct                         ) },
    { "target_level",                     OPT_INT,    &( target_level                             ) },
    { "target_race",                      OPT_STRING, &( target_race                              ) },
    // Character Creation
    { "death_knight",                     OPT_FUNC,   ( void* ) ::parse_player                      },
    { "deathknight",                      OPT_FUNC,   ( void* ) ::parse_player                      },
    { "druid",                            OPT_FUNC,   ( void* ) ::parse_player                      },
    { "hunter",                           OPT_FUNC,   ( void* ) ::parse_player                      },
    { "mage",                             OPT_FUNC,   ( void* ) ::parse_player                      },
    { "priest",                           OPT_FUNC,   ( void* ) ::parse_player                      },
    { "paladin",                          OPT_FUNC,   ( void* ) ::parse_player                      },
    { "rogue",                            OPT_FUNC,   ( void* ) ::parse_player                      },
    { "shaman",                           OPT_FUNC,   ( void* ) ::parse_player                      },
    { "warlock",                          OPT_FUNC,   ( void* ) ::parse_player                      },
    { "warrior",                          OPT_FUNC,   ( void* ) ::parse_player                      },
    { "enemy",                            OPT_FUNC,   ( void* ) ::parse_player                      },
    { "pet",                              OPT_FUNC,   ( void* ) ::parse_player                      },
    { "player",                           OPT_FUNC,   ( void* ) ::parse_player                      },
    { "copy",                             OPT_FUNC,   ( void* ) ::parse_player                      },
    { "armory",                           OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "guild",                            OPT_FUNC,   ( void* ) ::parse_armory                      },
    { "wowhead",                          OPT_FUNC,   ( void* ) ::parse_wowhead                     },
    { "chardev",                          OPT_FUNC,   ( void* ) ::parse_chardev                     },
    { "rawr",                             OPT_FUNC,   ( void* ) ::parse_rawr                        },
    { "bcp",                              OPT_FUNC,   ( void* ) ::parse_bcp_api                     },
    { "http_clear_cache",                 OPT_FUNC,   ( void* ) ::http_t::clear_cache               },
    { "cache",                            OPT_FUNC,   ( void* ) ::parse_cache                       },
    { "default_region",                   OPT_STRING, &( default_region_str                       ) },
    { "default_server",                   OPT_STRING, &( default_server_str                       ) },
    { "save_prefix",                      OPT_STRING, &( save_prefix_str                          ) },
    { "save_suffix",                      OPT_STRING, &( save_suffix_str                          ) },
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
    { "default_enchant_runic",                    OPT_FLT,  &( enchant.resource[ RESOURCE_RUNIC  ] ) },
    // Report
    { "report_precision",                 OPT_INT,    &( report_precision                         ) },
    { "report_pets_separately",           OPT_BOOL,   &( report_pets_separately                   ) },
    { "report_targets",                   OPT_BOOL,   &( report_targets                           ) },
    { "report_details",                   OPT_BOOL,   &( report_details                           ) },
    { "report_rng",                       OPT_BOOL,   &( report_rng                               ) },
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

// sim_t::parse_options =====================================================

bool sim_t::parse_options( int    _argc,
                           char** _argv )
{
  argc = _argc;
  argv = _argv;

  if ( argc <= 1 ) return false;

  if ( ! parent )
    cache::advance_era();

  for ( int i=1; i < argc; i++ )
  {
    if ( ! option_t::parse_line( this, argv[ i ] ) )
      return false;
  }

  if ( player_list == NULL && spell_query == NULL )
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
  if( canceled ) return;

  if( current_iteration >= 0 )
  {
    errorf( "Simulation has been canceled after %d iterations! (thread=%d)\n", current_iteration+1, thread_index );
  }
  else
  {
    errorf( "Simulation has been canceled during player setup! (thread=%d)\n", thread_index );
  }
  fflush( output_file );

  canceled = 1;

  int num_children = ( int ) children.size();

  for ( int i=0; i < num_children; i++ )
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

  if ( plot -> num_plot_stats > 0 )
  {
    return plot -> progress( phase );
  }
  else if ( scaling -> calculate_scale_factors &&
            scaling -> num_scaling_stats > 0 )
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

int sim_t::main( int argc, char** argv )
{
  sim_signal_handler_t::init( this );

  thread_t::init();

  http_t::cache_load();

  dbc_t::init();

  if ( ! parse_options( argc, argv ) )
  {
    errorf( "ERROR! Incorrect option format..\n" );
    cancel();
  }

  if( canceled ) return 0;

  current_throttle = armory_throttle;

  util_t::fprintf( output_file, "\nSimulationCraft %s-%s for World of Warcraft %s %s (build level %s)\n",
                   SC_MAJOR_VERSION, SC_MINOR_VERSION, dbc_t::wow_version( dbc.ptr ), ( dbc.ptr ? "PTR" : "Live" ), dbc_t::build_level( dbc.ptr ) );
  fflush( output_file );

  if ( spell_query )
  {
    spell_query -> evaluate();
    report_t::print_spell_query( this );
  }
  else if ( need_to_save_profiles( this ) )
  {
    init();
    util_t::fprintf( stdout, "\nGenerating profiles... \n" ); fflush( stdout );
    report_t::print_profiles( this );
  }
  else
  {
    if ( max_time <= 0 )
    {
      util_t::fprintf( output_file, "simulationcraft: One of -max_time or -target_health must be specified.\n" );
      exit( 0 );
    }

    util_t::fprintf( output_file,
                     "\nSimulating... ( iterations=%d, max_time=%.0f, vary_combat_length=%0.2f, optimal_raid=%d, fight_style=%s )\n",
                     iterations, max_time, vary_combat_length, optimal_raid, fight_style.c_str() );
    fflush( output_file );

    util_t::fprintf( stdout, "\nGenerating baseline... \n" ); fflush( stdout );

    if( execute() )
    {
      scaling      -> analyze();
      plot         -> analyze();
      reforge_plot -> analyze();
      util_t::fprintf( stdout, "\nGenerating reports...\n" ); fflush( stdout );
      report_t::print_suite( this );
    }
  }

  if ( output_file != stdout ) fclose( output_file );

  http_t::cache_save();

  sim_signal_handler_t::init( 0 );

  thread_t::de_init();
  dbc_t::de_init();

  return 0;
}

// sim_t::errorf ============================================================

int sim_t::errorf( const char* format, ... )
{
  char buffer_printf[ 1024 ];

  va_list fmtargs;
  va_start( fmtargs, format );
  int retcode = vsnprintf( buffer_printf, sizeof( buffer_printf ), format, fmtargs );
  va_end( fmtargs );
  assert( retcode >= 0 );

  char buffer_locale[ 1024 ];
  char *p_locale = setlocale( LC_CTYPE, NULL );
  if ( p_locale != NULL )
  {
    strncpy( buffer_locale, p_locale, sizeof( buffer_locale ) - 1 );
    buffer_locale[ sizeof( buffer_locale ) - 1 ] = '\0';
  }
  else
  {
    buffer_locale[ 0 ] = '\0';
  }

  setlocale( LC_CTYPE, "" );

  fprintf( output_file, "%s", buffer_printf );
  fprintf( output_file, "\n" );
  error_list.push_back( buffer_printf );

  setlocale( LC_CTYPE, p_locale );

  return retcode;
}

// ==========================================================================
// Utility to make sure memory allocation not happening during iteration.
// ==========================================================================

#if 0
void* operator new ( size_t size )
{
  if ( iterating ) assert( 0 );
  return malloc( size );
}

void operator delete ( void *p )
{
  free( p );
}
#endif

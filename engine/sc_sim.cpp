// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

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
      exit(0);
    }
    else if( signal == SIGINT )
    {
      if( global_sim ) 
      {
	if( global_sim -> canceled ) exit(0);
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

// parse_patch ==============================================================

static bool parse_patch( sim_t*             sim,
                         const std::string& name,
                         const std::string& value )
{
  if ( name != "patch" ) return false;

  int arch, version, revision;

  if ( 3 != util_t::string_split( value, ".", "i i i", &arch, &version, &revision ) )
  {
    sim -> errorf( "Expected format: -patch=#.#.#\n" );
    return false;
  }

  sim -> patch.set( arch, version, revision );

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
    std::string player_options = "";

    std::string::size_type cut_pt = value.find_first_of( "," );

    if ( cut_pt != value.npos )
    {
      player_options = value.substr( cut_pt + 1 );
      player_name    = value.substr( 0, cut_pt );
    }

    std::string wowhead;
    std::string region = sim -> default_region_str;
    std::string server = sim -> default_server_str;
    std::string talents = "active";
    int cache=0;

    option_t options[] =
    {
      { "wowhead", OPT_STRING, &wowhead  },
      { "region",  OPT_STRING, &region   },
      { "server",  OPT_STRING, &server   },
      { "talents", OPT_STRING, &talents  },
      { "cache",   OPT_BOOL,   &cache    },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::parse( sim, "player", options, player_options );

    if ( wowhead.empty() )
    {
      sim -> active_player = armory_t::download_player( sim, region, server, player_name, talents, cache );
    }
    else
    {
      sim -> active_player = wowhead_t::download_player( sim, wowhead, ( talents == "active" ) );

      if ( sim -> active_player )
        if ( player_name != sim -> active_player -> name() )
          sim -> errorf( "Mismatch between player name '%s' and wowhead name '%s' for id '%s'\n",
			 player_name.c_str(), sim -> active_player -> name(), wowhead.c_str() );

    }
  }
  else
  {
    sim -> active_player = player_t::create( sim, name, value );
  }

  return sim -> active_player != 0;
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

    std::string region = splits[ 0 ];
    std::string server = splits[ 1 ];

    for ( int i=2; i < num_splits; i++ )
    {
      std::string player_name = splits[ i ];
      std::string description = "active";
      if ( player_name[ 0 ] == '!' )
      {
        player_name.erase( 0, 1 );
        description = "inactive";
      }
      std::vector<std::string> encoding;
      if ( util_t::string_split( encoding, player_name, "|" ) > 1 )
      {
	player_name = encoding[ 0 ];
	description = encoding[ 1 ];
      }
      sim -> active_player = armory_t::download_player( sim, region, server, player_name, description );
      if ( ! sim -> active_player ) return false;
    }
    return true;
  }
  else if ( name == "guild" )
  {
    std::string guild_name = value;
    std::string guild_options = "";
    std::vector<int> ranks_list;
    std::vector<std::string> ranks;

    std::string::size_type cut_pt = value.find_first_of( "," );

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
    int cache = 0;

    option_t options[] =
    {
      { "region",   OPT_STRING, &region   },
      { "server",   OPT_STRING, &server   },
      { "class",    OPT_STRING, &type_str },
      { "max_rank", OPT_INT,    &max_rank },
      { "ranks",    OPT_STRING, &ranks_str},
      { "cache",    OPT_BOOL,   &cache    },
      { NULL, OPT_UNKNOWN, NULL }
    };

    if ( ! option_t::parse( sim, "guild", options, guild_options ) )
      return false;

    if ( ! ranks_str.empty() )
    {
      int n_ranks = util_t::string_split( ranks, ranks_str, "/" );
      if (n_ranks > 0)
      {
        for (int i = 0; i < n_ranks; i++)
          ranks_list.push_back(atoi(ranks[i].c_str()));
      }
    }

    int player_type = PLAYER_NONE;
    if ( ! type_str.empty() ) player_type = util_t::parse_player_type( type_str );

    return armory_t::download_guild( sim, region, server, guild_name, ranks_list, player_type, max_rank, cache );
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
      int active = 1;
      if ( player_id[ 0 ] == '!' )
      {
        player_id.erase( 0, 1 );
        active = 0;
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
        int active = 1;
        if ( player_name[ 0 ] == '!' )
        {
          player_name.erase( 0, 1 );
          active = 0;
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

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Simulator
// ==========================================================================

// sim_t::sim_t =============================================================

sim_t::sim_t( sim_t* p, int index ) :
    parent( p ), P400( false ),
    free_list( 0 ), player_list( 0 ), active_player( 0 ), num_players( 0 ), canceled( 0 ),
    queue_lag( 0.075 ), queue_lag_stddev( 0 ),
    gcd_lag( 0.150 ), gcd_lag_stddev( 0 ),
    channel_lag( 0.250 ), channel_lag_stddev( 0 ),
    queue_gcd_reduction( 0.075 ), strict_gcd_queue( 1 ),
    travel_variance( 0 ), default_skill( 1.0 ), reaction_time( 0.5 ), regen_periodicity( 1.0 ),
    current_time( 0 ), max_time( 300 ), expected_time( 0 ), vary_combat_length( 0 ),
    events_remaining( 0 ), max_events_remaining( 0 ),
    events_processed( 0 ), total_events_processed( 0 ),
    seed( 0 ), id( 0 ), iterations( 1000 ), current_iteration( -1 ), current_slot( -1 ),
    armor_update_interval( 20 ), weapon_speed_scale_factors( 0 ),
    optimal_raid( 0 ), spell_crit_suppression( 0 ), log( 0 ), debug( 0 ), save_profiles( 0 ),
    normalized_stat( STAT_NONE ),
    default_region_str( "us" ),
    rng( 0 ), deterministic_rng( 0 ), rng_list( 0 ),
    smooth_rng( 0 ), deterministic_roll( 0 ), average_range( 1 ), average_gauss( 0 ),
    timing_wheel( 0 ), wheel_seconds( 0 ), wheel_size( 0 ), wheel_mask( 0 ), timing_slice( 0 ), wheel_granularity( 0.0 ),
    buff_list( 0 ), aura_delay( 0 ), replenishment_targets( 0 ),
    raid_dps( 0 ), total_dmg( 0 ),
    total_seconds( 0 ), elapsed_cpu_seconds( 0 ),
    merge_ignite( 0 ), report_progress( 1 ),
    path_str( "." ), output_file( stdout ), log_file( 0 ), 
    armory_throttle( 2 ), current_throttle( 2 ), debug_exp( 0 ),
    report_precision( 4 ), threads( 0 ), thread_handle( 0 ), thread_index( index )
{
  path_str += "|profiles";
  path_str += "|..";
  path_str += DIRECTORY_DELIMITER;
  path_str += "profiles";

  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    infinite_resource[ i ] = false;
  }
  infinite_resource[ RESOURCE_HEALTH ] = true;

  target  = new  target_t( this );
  scaling = new scaling_t( this );
  plot    = new    plot_t( this );

  use_optimal_buffs_and_debuffs( 1 );

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

  if ( rng     ) delete rng;
  if ( target  ) delete target;
  if ( scaling ) delete scaling;
  if ( plot    ) delete plot;

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
}

// sim_t::add_event ==========================================================

void sim_t::add_event( event_t* e,
                       double   delta_time )
{
  if ( delta_time <= 0 ) delta_time = 0.0000001;

  e -> time = current_time + delta_time;
  e -> id   = ++id;

  assert( delta_time <= wheel_seconds );

  uint32_t slice = ( uint32_t ) ( e -> time * wheel_granularity ) & wheel_mask;

  event_t** prev = &( timing_wheel[ slice ] );

  while ( ( *prev ) && ( *prev ) -> time < e -> time ) prev = &( ( *prev ) -> next );

  e -> next = *prev;
  *prev = e;

  events_remaining++;
  if ( events_remaining > max_events_remaining ) max_events_remaining = events_remaining;

  if ( debug ) log_t::output( this, "Add Event: %s %.2f %d", e -> name, e -> time, e -> id );
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
  for ( int i=0; i < wheel_size; i++ )
  {
    for ( event_t* e = timing_wheel[ i ]; e; e = e -> next )
    {
      if ( e -> player == p )
      {
        e -> canceled = 1;
      }
    }
  }
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

    if ( expected_time > 0 && current_time > ( expected_time * 2.0 ) )
    {
      target -> recalculate_health();
      if ( debug ) log_t::output( this, "Target proving tough to kill, ending simulation" );
      delete e;
      break;
    }
    if ( target -> initial_health != 0 ) 
    {
      if (  target -> current_health <= 0 ) 
      {
	target -> recalculate_health();
	if ( debug ) log_t::output( this, "Target has died, ending simulation" );
	delete e;
	break;
      }
    }
    else // initial_health == 0
    {
      if ( current_time > ( expected_time / 2.0 ) )
      {
	if ( debug ) log_t::output( this, "Initializing target health half-way through simulation" );
	target -> recalculate_health();
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
  if ( debug ) log_t::output( this, "Reseting Simulator" );
  expected_time = max_time * ( 1.0 + vary_combat_length * iteration_adjust() );
  current_time = id = 0;
  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> reset();
  }
  target -> reset();
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

  target -> combat_begin();

  player_t::combat_begin( this );

  raid_event_t::combat_begin( this );

  for ( player_t* p = player_list; p; p = p -> next )
  {
    p -> combat_begin();
  }
  new ( this ) regen_event_t( this );
}

// sim_t::combat_end ========================================================

void sim_t::combat_end()
{
  if ( debug ) log_t::output( this, "Combat End" );

  iteration_timeline.push_back( current_time );

  total_seconds += current_time;
  total_events_processed += events_processed;

  flush_events();

  target -> combat_end();

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

  P400 = patch.after( 4, 0, 0 );

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
  memset( timing_wheel,0,sizeof( event_t* )*( wheel_size ) );

  total_seconds = 0;

  if (   queue_lag_stddev == 0 )   queue_lag_stddev =   queue_lag * 0.25;
  if (     gcd_lag_stddev == 0 )     gcd_lag_stddev =     gcd_lag * 0.25;
  if ( channel_lag_stddev == 0 ) channel_lag_stddev = channel_lag * 0.25;

  target -> init();

  raid_event_t::init( this );

  if ( ! player_t::init( this ) ) return false;

  if ( report_precision < 0 ) report_precision = 3;

  return canceled ? false : true;
}

// sim_t::analyze ============================================================

struct compare_dps
{
  bool operator()( player_t* l, player_t* r ) SC_CONST
  {
    return l -> dps > r -> dps;
  }
};

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

void sim_t::analyze()
{
  if ( total_seconds == 0 ) return;

  // buff_t::analyze must be called before total_seconds is normalized via iteration count

  for ( buff_t* b = buff_list; b; b = b -> next )
    b -> analyze();

  total_dmg = 0;
  total_seconds /= iterations;

  for ( player_t* p = player_list; p; p = p -> next )
  {
    for ( buff_t* b = p -> buff_list; b; b = b -> next )
      b -> analyze();

    p -> total_dmg = 0;
    p -> total_seconds /= iterations;
    p -> total_waiting /= iterations;
    p -> total_foreground_actions /= iterations;
  }

  for ( player_t* p = player_list; p; p = p -> next )
  {
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

      s -> analyze();
      p -> total_dmg += s -> total_dmg;
    }

    p -> dps = p -> total_dmg / p -> total_seconds;

    if ( p -> quiet ) continue;

    for ( int i=0; i < num_stats; i++ )
    {
      stats_t* s = stats_list[ i ];

      s -> portion_dmg = s -> total_dmg / p -> total_dmg;
      s -> portion_dps = s -> portion_dmg * p -> dps;
    }

    players_by_rank.push_back( p );
    players_by_name.push_back( p );

    // Avoid double-counting of pet damage
    if ( ! p -> is_pet() ) total_dmg += p -> total_dmg;

    int max_buckets = ( int ) p -> total_seconds;
    int num_buckets = ( int ) p -> timeline_resource.size();

    if ( num_buckets > max_buckets ) p -> timeline_resource.resize( max_buckets );

    for ( int i=0; i < max_buckets; i++ )
    {
      p -> timeline_resource[ i ] /= iterations;
    }

    for ( int i=0; i < RESOURCE_MAX; i++ )
    {
      p -> resource_lost  [ i ] /= iterations;
      p -> resource_gained[ i ] /= iterations;
    }

    for ( pet_t *pet = p -> pet_list; pet; pet = pet -> next_pet )
    {
      for ( int i=0; i < RESOURCE_MAX; i++ )
      {
        pet -> resource_lost  [ i ] /= iterations;
        pet -> resource_gained[ i ] /= iterations;
      }
    }

    p -> dpr = p -> total_dmg / p -> resource_lost[ p -> primary_resource() ];

    p -> rps_loss = p -> resource_lost  [ p -> primary_resource() ] / p -> total_seconds;
    p -> rps_gain = p -> resource_gained[ p -> primary_resource() ] / p -> total_seconds;

    for ( gain_t* g = p -> gain_list; g; g = g -> next )
      g -> analyze( this );

    for ( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
      proc -> analyze( this );

    for ( pet_t *pet = p -> pet_list; pet; pet = pet -> next_pet )
    {
      for ( gain_t* g = pet -> gain_list; g; g = g -> next )
        g -> analyze( this );

      for ( proc_t* proc = pet -> proc_list; proc; proc = proc -> next )
        proc -> analyze( this );
    }

    p -> timeline_dmg.clear();
    p -> timeline_dps.clear();

    p -> timeline_dmg.insert( p -> timeline_dmg.begin(), max_buckets, 0 );
    p -> timeline_dps.insert( p -> timeline_dps.begin(), max_buckets, 0 );

    for ( int i=0; i < num_stats; i++ )
    {
      stats_t* s = stats_list[ i ];

      for ( int j=0; ( j < max_buckets ) && ( j < s -> num_buckets ); j++ )
      {
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

    p -> dps_min = 0;
    p -> dps_max = 0;
    p -> dps_std_dev = 0;
    for ( int i=0; i < iterations; i++ )
    {
      double i_dps = p -> iteration_dps[ i ];
      if ( p -> dps_min == 0 || p -> dps_min > i_dps ) p -> dps_min = i_dps;
      if ( p -> dps_max == 0 || p -> dps_max < i_dps ) p -> dps_max = i_dps;
      double delta = i_dps - p -> dps;
      p -> dps_std_dev += delta * delta;
    }
    p -> dps_std_dev /= iterations;
    p -> dps_std_dev = sqrt( p -> dps_std_dev );
    p -> dps_error = 2.0 * p -> dps_std_dev / sqrt( ( float ) iterations );

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
  }

  int num_timelines = iteration_timeline.size();
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

  raid_dps = total_dmg / total_seconds;

  chart_t::raid_dps     ( dps_charts,     this );
  chart_t::raid_dpet    ( dpet_charts,    this );
  chart_t::raid_gear    ( gear_charts,    this );
  chart_t::raid_downtime( downtime_chart, this );
  chart_t::raid_timeline( timeline_chart, this );

  for ( player_t* p = player_list; p; p = p -> next )
  {
    if ( p -> quiet ) continue;

    chart_t::action_dpet      ( p -> action_dpet_chart,       p );
    chart_t::action_dmg       ( p -> action_dmg_chart,        p );
    chart_t::gains            ( p -> gains_chart,             p );
    chart_t::timeline_resource( p -> timeline_resource_chart, p );
    chart_t::timeline_dps     ( p -> timeline_dps_chart,      p );
    chart_t::distribution_dps ( p -> distribution_dps_chart,  p );
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

  for ( player_t* p = player_list; p; p = p -> next )
  {
    player_t* other_p = other_sim.find_player( p -> name() );
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
  for ( player_t* p = player_list; p; p = p -> next )
  {
    if ( name == p -> name() ) return p;
  }
  return 0;
}

// sim_t::use_optimal_buffs_and_debuffs =====================================

void sim_t::use_optimal_buffs_and_debuffs( int value )
{
  optimal_raid = value;

  overrides.abominations_might     = optimal_raid;
  overrides.arcane_brilliance      = optimal_raid;
  overrides.arcane_empowerment     = optimal_raid;
  overrides.battle_shout           = optimal_raid;
  overrides.bleeding               = optimal_raid;
  overrides.blessing_of_kings      = optimal_raid;
  overrides.blessing_of_might      = optimal_raid;
  overrides.blessing_of_wisdom     = optimal_raid;
  overrides.blood_frenzy           = optimal_raid;
  overrides.blood_plague           = optimal_raid;
  overrides.bloodlust              = optimal_raid;
  overrides.crypt_fever            = optimal_raid;
  overrides.curse_of_elements      = optimal_raid;
  overrides.devotion_aura          = optimal_raid;
  overrides.divine_spirit          = optimal_raid;
  overrides.earth_and_moon         = optimal_raid;
  overrides.ebon_plaguebringer     = optimal_raid;
  overrides.elemental_oath         = optimal_raid;
  overrides.expose_armor           = optimal_raid;
  overrides.faerie_fire            = optimal_raid;
  overrides.ferocious_inspiration  = optimal_raid;
  overrides.flametongue_totem      = optimal_raid;
  overrides.fortitude              = optimal_raid;
  overrides.frost_fever            = optimal_raid;
  overrides.heart_of_the_crusader  = optimal_raid;
  overrides.hellscreams_warsong    = 0;
  overrides.heroic_presence        = optimal_raid;
  overrides.horn_of_winter         = optimal_raid;
  overrides.hunters_mark           = optimal_raid;
  overrides.improved_faerie_fire   = optimal_raid;
  overrides.improved_icy_talons    = optimal_raid;
  overrides.improved_moonkin_aura  = optimal_raid;
  overrides.improved_scorch        = optimal_raid;
  overrides.improved_shadow_bolt   = optimal_raid;
  overrides.infected_wounds        = optimal_raid;
  overrides.insect_swarm           = optimal_raid;
  overrides.judgement_of_wisdom    = optimal_raid;
  overrides.judgements_of_the_just = optimal_raid;
  overrides.leader_of_the_pack     = optimal_raid;
  overrides.mana_spring_totem      = optimal_raid;
  overrides.mangle                 = optimal_raid;
  overrides.mark_of_the_wild       = optimal_raid;
  overrides.master_poisoner        = optimal_raid;
  overrides.misery                 = optimal_raid;
  overrides.moonkin_aura           = optimal_raid;
  overrides.poisoned               = optimal_raid;
  overrides.rampage                = optimal_raid;
  overrides.replenishment          = optimal_raid;
  overrides.sanctified_retribution = optimal_raid;
  overrides.savage_combat          = optimal_raid;
  overrides.scorpid_sting          = optimal_raid;
  overrides.strength_of_earth      = optimal_raid;
  overrides.strength_of_wrynn      = 0;
  overrides.sunder_armor           = optimal_raid;
  overrides.swift_retribution      = optimal_raid;
  overrides.trauma                 = optimal_raid;
  overrides.thunder_clap           = optimal_raid;
  overrides.totem_of_wrath         = optimal_raid;
  overrides.trueshot_aura          = optimal_raid;
  overrides.unleashed_rage         = optimal_raid;
  overrides.windfury_totem         = optimal_raid;
  overrides.winters_chill          = optimal_raid;
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
  return rng -> roll( chance );
}

// sim_t::range =============================================================

double sim_t::range( double min,
                     double max )
{
  return rng -> range( min, max );
}

// sim_t::gauss =============================================================

double sim_t::gauss( double mean,
                     double stddev )
{
  return rng -> gauss( mean, stddev );
}

// sim_t::get_rng ===========================================================

rng_t* sim_t::get_rng( const std::string& n, int type )
{
  assert( rng );

  if ( ! smooth_rng || type == RNG_GLOBAL ) return rng;

  if ( type == RNG_DETERMINISTIC ) return deterministic_rng;

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
  return 2.0 * ( ( current_iteration / (double) iterations ) - 0.5 );
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

  if ( num_splits == 3 )
  {
    if ( splits[ 0 ] == "aura" || splits[ 0 ] == "debuff" )
    {
      buff_t* buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) return 0;
      return buff -> create_expression( a, splits[ 2 ] );
    }
  }
  else if ( num_splits == 2 )
  {
    if ( splits[ 0 ] == "target" )
    {
      if ( splits[ 1 ] == "time_to_die" )
      {
	struct target_time_to_die_expr_t : public action_expr_t
        {
	  target_time_to_die_expr_t( action_t* a ) : action_expr_t( a, "target_time_to_die", TOK_NUM ) {}
	  virtual int evaluate() { result_num = action -> sim -> target -> time_to_die();  return TOK_NUM; }
	};
	return new target_time_to_die_expr_t( a );
      }
      else if ( splits[ 1 ] == "health_pct" )
      {
	struct target_health_pct_expr_t : public action_expr_t
        {
	  target_health_pct_expr_t( action_t* a ) : action_expr_t( a, "target_health_pct", TOK_NUM ) {}
	  virtual int evaluate() { result_num = action -> sim -> target -> health_percentage();  return TOK_NUM; }
	};
	return new target_health_pct_expr_t( a );
      }
      else if ( splits[ 1 ] == "adds" )
      {
	struct target_adds_expr_t : public action_expr_t
        {
	  target_adds_expr_t( action_t* a ) : action_expr_t( a, "target_adds", TOK_NUM ) {}
	  virtual int evaluate() { result_num = action -> sim -> target -> adds_nearby;  return TOK_NUM; }
	};
	return new target_adds_expr_t( a );
      }
      else if ( splits[ 1 ] == "adds_never" )
      {
	struct target_adds_never_expr_t : public action_expr_t
        {
	  target_adds_never_expr_t( action_t* a ) : action_expr_t( a, "target_adds_never", TOK_NUM ) 
	  {
	    bool adds = a -> sim -> target -> initial_adds_nearby > 0;
	    int num_events = ( int ) a -> sim -> raid_events.size();
	    for ( int i=0; i < num_events; i++ )
	      if ( a -> sim -> raid_events[ i ] -> name_str == "adds" ) 
		adds = true;
	    result_num = adds ? 0.0 : 1.0;
	  }
	};
	return new target_adds_never_expr_t( a );
      }
    }
  }

  return 0;
}

// sim_t::print_options =====================================================

void sim_t::print_options()
{
  util_t::fprintf( output_file, "\nWorld of Warcraft Raid Simulator Options:\n" );

  std::vector<option_t>& options = get_options();
  int num_options = ( int ) options.size();

  util_t::fprintf( output_file, "\nSimulation Engine:\n" );
  for ( int i=0; i < num_options; i++ ) options[ i ].print( output_file );

  for ( player_t* p = player_list; p; p = p -> next )
  {
    std::vector<option_t>& options = p -> get_options();
    int num_options = ( int ) options.size();

    util_t::fprintf( output_file, "\nPlayer: %s (%s)\n", p -> name(), util_t::player_type_string( p -> type ) );
    for ( int i=0; i < num_options; i++ ) options[ i ].print( output_file );
  }

  util_t::fprintf( output_file, "\n" );
  fflush( output_file );
}

// sim_t::get_options =======================================================

std::vector<option_t>& sim_t::get_options()
{
  if ( options.empty() )
  {
    option_t global_options[] =
    {
      // @option_doc loc=global/general title="General"
      { "iterations",                       OPT_INT,    &( iterations                               ) },
      { "max_time",                         OPT_FLT,    &( max_time                                 ) },
      { "vary_combat_length",               OPT_FLT,    &( vary_combat_length                       ) },
      { "optimal_raid",                     OPT_FUNC,   ( void* ) ::parse_optimal_raid                },
      { "patch",                            OPT_FUNC,   ( void* ) ::parse_patch                       },
      { "threads",                          OPT_INT,    &( threads                                  ) },
      // @option_doc loc=global/lag title="Lag"
      { "channel_lag",                      OPT_FLT,    &( channel_lag                              ) },
      { "channel_lag_stddev",               OPT_FLT,    &( channel_lag_stddev                       ) },
      { "gcd_lag",                          OPT_FLT,    &( gcd_lag                                  ) },
      { "gcd_lag_stddev",                   OPT_FLT,    &( gcd_lag_stddev                           ) },
      { "queue_lag",                        OPT_FLT,    &( queue_lag                                ) },
      { "queue_lag_stddev",                 OPT_FLT,    &( queue_lag_stddev                         ) },
      { "queue_gcd_reduction",              OPT_FLT,    &( queue_gcd_reduction                      ) },
      { "strict_gcd_queue",                 OPT_BOOL,   &( strict_gcd_queue                         ) },
      { "default_skill",                    OPT_FLT,    &( default_skill                            ) },
      { "reaction_time",                    OPT_FLT,    &( reaction_time                            ) },
      { "travel_variance",                  OPT_FLT,    &( travel_variance                          ) },
      // @option_doc loc=skip
      { "save_profiles",                    OPT_BOOL,   &( save_profiles                            ) },
      { "combat_log",                       OPT_STRING, &( log_file_str                             ) },
      { "debug",                            OPT_BOOL,   &( debug                                    ) },
      { "html",                             OPT_STRING, &( html_file_str                            ) },
      { "xml",                              OPT_STRING, &( xml_file_str                             ) },
      { "log",                              OPT_BOOL,   &( log                                      ) },
      { "output",                           OPT_STRING, &( output_file_str                          ) },
      { "wiki",                             OPT_STRING, &( wiki_file_str                            ) },
      { "path",                             OPT_STRING, &( path_str                                 ) },
      { "path+",                            OPT_APPEND, &( path_str                                 ) },
      // @option_doc loc=global/overrides title="Buff/Debuff Overrides"
      { "override.abominations_might",      OPT_BOOL,   &( overrides.abominations_might             ) },
      { "override.arcane_brilliance",       OPT_BOOL,   &( overrides.arcane_brilliance              ) },
      { "override.arcane_empowerment",      OPT_BOOL,   &( overrides.arcane_empowerment             ) },
      { "override.battle_shout",            OPT_BOOL,   &( overrides.battle_shout                   ) },
      { "override.bleeding",                OPT_BOOL,   &( overrides.bleeding                       ) },
      { "override.blessing_of_kings",       OPT_BOOL,   &( overrides.blessing_of_kings              ) },
      { "override.blessing_of_might",       OPT_BOOL,   &( overrides.blessing_of_might              ) },
      { "override.blessing_of_wisdom",      OPT_BOOL,   &( overrides.blessing_of_wisdom             ) },
      { "override.blood_frenzy",            OPT_BOOL,   &( overrides.blood_frenzy                   ) },
      { "override.blood_plague",            OPT_BOOL,   &( overrides.blood_plague                   ) },
      { "override.bloodlust",               OPT_BOOL,   &( overrides.bloodlust                      ) },
      { "override.bloodlust_early",         OPT_INT,    &( overrides.bloodlust_early                ) },
      { "override.celerity",                OPT_BOOL,   &( overrides.celerity                       ) },
      { "override.crypt_fever",             OPT_BOOL,   &( overrides.crypt_fever                    ) },
      { "override.curse_of_elements",       OPT_BOOL,   &( overrides.curse_of_elements              ) },
      { "override.devotion_aura",           OPT_BOOL,   &( overrides.devotion_aura                  ) },
      { "override.divine_spirit",           OPT_BOOL,   &( overrides.divine_spirit                  ) },
      { "override.earth_and_moon",          OPT_BOOL,   &( overrides.earth_and_moon                 ) },
      { "override.ebon_plaguebringer",      OPT_BOOL,   &( overrides.ebon_plaguebringer             ) },
      { "override.elemental_oath",          OPT_BOOL,   &( overrides.elemental_oath                 ) },
      { "override.faerie_fire",             OPT_BOOL,   &( overrides.faerie_fire                    ) },
      { "override.ferocious_inspiration",   OPT_BOOL,   &( overrides.ferocious_inspiration          ) },
      { "override.flametongue_totem",       OPT_BOOL,   &( overrides.flametongue_totem              ) },
      { "override.focus_magic",             OPT_BOOL,   &( overrides.focus_magic                    ) },
      { "override.fortitude",               OPT_BOOL,   &( overrides.fortitude                      ) },
      { "override.frost_fever",             OPT_BOOL,   &( overrides.frost_fever                    ) },
      { "override.heart_of_the_crusader",   OPT_BOOL,   &( overrides.heart_of_the_crusader          ) },
      { "override.horn_of_winter",          OPT_BOOL,   &( overrides.horn_of_winter                 ) },
      { "override.hellscreams_warsong",     OPT_BOOL,   &( overrides.hellscreams_warsong            ) },
      { "override.heroic_presence",         OPT_BOOL,   &( overrides.heroic_presence                ) },
      { "override.hunters_mark",            OPT_BOOL,   &( overrides.hunters_mark                   ) },
      { "override.improved_faerie_fire",    OPT_BOOL,   &( overrides.improved_faerie_fire           ) },
      { "override.improved_icy_talons",     OPT_BOOL,   &( overrides.improved_icy_talons            ) },
      { "override.improved_moonkin_aura",   OPT_BOOL,   &( overrides.improved_moonkin_aura          ) },
      { "override.improved_scorch",         OPT_BOOL,   &( overrides.improved_scorch                ) },
      { "override.improved_shadow_bolt",    OPT_BOOL,   &( overrides.improved_shadow_bolt           ) },
      { "override.infected_wounds",         OPT_BOOL,   &( overrides.infected_wounds                ) },
      { "override.insect_swarm",            OPT_BOOL,   &( overrides.insect_swarm                   ) },
      { "override.judgement_of_wisdom",     OPT_BOOL,   &( overrides.judgement_of_wisdom            ) },
      { "override.judgements_of_the_just",  OPT_BOOL,   &( overrides.judgements_of_the_just         ) },
      { "override.leader_of_the_pack",      OPT_BOOL,   &( overrides.leader_of_the_pack             ) },
      { "override.mana_spring_totem",       OPT_BOOL,   &( overrides.mana_spring_totem              ) },
      { "override.mangle",                  OPT_BOOL,   &( overrides.mangle                         ) },
      { "override.mark_of_the_wild",        OPT_BOOL,   &( overrides.mark_of_the_wild               ) },
      { "override.master_poisoner",         OPT_BOOL,   &( overrides.master_poisoner                ) },
      { "override.misery",                  OPT_BOOL,   &( overrides.misery                         ) },
      { "override.moonkin_aura",            OPT_BOOL,   &( overrides.moonkin_aura                   ) },
      { "override.poisoned",                OPT_BOOL,   &( overrides.poisoned                       ) },
      { "override.rampage",                 OPT_BOOL,   &( overrides.rampage                        ) },
      { "override.replenishment",           OPT_BOOL,   &( overrides.replenishment                  ) },
      { "override.sanctified_retribution",  OPT_BOOL,   &( overrides.sanctified_retribution         ) },
      { "override.savage_combat",           OPT_BOOL,   &( overrides.savage_combat                  ) },
      { "override.scorpid_sting",           OPT_BOOL,   &( overrides.scorpid_sting                  ) },
      { "override.strength_of_earth",       OPT_BOOL,   &( overrides.strength_of_earth              ) },
      { "override.strength_of_wrynn",       OPT_BOOL,   &( overrides.strength_of_wrynn              ) },
      { "override.sunder_armor",            OPT_BOOL,   &( overrides.sunder_armor                   ) },
      { "override.swift_retribution",       OPT_BOOL,   &( overrides.swift_retribution              ) },
      { "override.thunder_clap",            OPT_BOOL,   &( overrides.thunder_clap                   ) },
      { "override.totem_of_wrath",          OPT_BOOL,   &( overrides.totem_of_wrath                 ) },
      { "override.trauma",                  OPT_BOOL,   &( overrides.trauma                         ) },
      { "override.trueshot_aura",           OPT_BOOL,   &( overrides.trueshot_aura                  ) },
      { "override.unleashed_rage",          OPT_BOOL,   &( overrides.unleashed_rage                 ) },
      { "override.windfury_totem",          OPT_BOOL,   &( overrides.windfury_totem                 ) },
      { "override.winters_chill",           OPT_BOOL,   &( overrides.winters_chill                  ) },
      { "override.wrath_of_air",            OPT_BOOL,   &( overrides.wrath_of_air                   ) },
      // @option_doc loc=global/regen title="Regen"
      { "infinite_energy",                  OPT_BOOL,   &( infinite_resource[ RESOURCE_ENERGY ]     ) },
      { "infinite_focus",                   OPT_BOOL,   &( infinite_resource[ RESOURCE_FOCUS  ]     ) },
      { "infinite_health",                  OPT_BOOL,   &( infinite_resource[ RESOURCE_HEALTH ]     ) },
      { "infinite_mana",                    OPT_BOOL,   &( infinite_resource[ RESOURCE_MANA   ]     ) },
      { "infinite_rage",                    OPT_BOOL,   &( infinite_resource[ RESOURCE_RAGE   ]     ) },
      { "infinite_runic",                   OPT_BOOL,   &( infinite_resource[ RESOURCE_RUNIC  ]     ) },
      { "regen_periodicity",                OPT_FLT,    &( regen_periodicity                        ) },
      // @option_doc loc=global/rng title="Smooth RNG"
      { "smooth_rng",                       OPT_BOOL,   &( smooth_rng                               ) },
      { "deterministic_roll",               OPT_BOOL,   &( deterministic_roll                       ) },
      { "average_range",                    OPT_BOOL,   &( average_range                            ) },
      { "average_gauss",                    OPT_BOOL,   &( average_gauss                            ) },
      // @option_doc loc=global/party title="Party Composition"
      { "party",                            OPT_LIST,   &( party_encoding                           ) },
      // @option_doc loc=skip
      { "active",                           OPT_FUNC,   ( void* ) ::parse_active                      },
      { "armor_update_internval",           OPT_INT,    &( armor_update_interval                    ) },
      { "aura_delay",                       OPT_FLT,    &( aura_delay                               ) },
      { "merge_ignite",                     OPT_BOOL,   &( merge_ignite                             ) },
      { "replenishment_targets",            OPT_INT,    &( replenishment_targets                    ) },
      { "seed",                             OPT_INT,    &( seed                                     ) },
      { "wheel_granularity",                OPT_FLT,    &( wheel_granularity                        ) },
      { "wheel_seconds",                    OPT_INT,    &( wheel_seconds                            ) },
      { "armory_throttle",                  OPT_INT,    &( armory_throttle                          ) },
      { "reference_player",                 OPT_STRING, &( reference_player_str                     ) },
      { "raid_events",                      OPT_STRING, &( raid_events_str                          ) },
      { "raid_events+",                     OPT_APPEND, &( raid_events_str                          ) },
      { "debug_exp",                        OPT_INT,    &( debug_exp                                ) },
      { "report_precision",                 OPT_INT,    &( report_precision                         ) },
      { "weapon_speed_scale_factors",       OPT_BOOL,   &( weapon_speed_scale_factors               ) },
      // @option_doc loc=skip
      { "death_knight",                     OPT_FUNC,   ( void* ) ::parse_player                      },
      { "druid",                            OPT_FUNC,   ( void* ) ::parse_player                      },
      { "hunter",                           OPT_FUNC,   ( void* ) ::parse_player                      },
      { "mage",                             OPT_FUNC,   ( void* ) ::parse_player                      },
      { "priest",                           OPT_FUNC,   ( void* ) ::parse_player                      },
      { "paladin",                          OPT_FUNC,   ( void* ) ::parse_player                      },
      { "rogue",                            OPT_FUNC,   ( void* ) ::parse_player                      },
      { "shaman",                           OPT_FUNC,   ( void* ) ::parse_player                      },
      { "warlock",                          OPT_FUNC,   ( void* ) ::parse_player                      },
      { "warrior",                          OPT_FUNC,   ( void* ) ::parse_player                      },
      { "pet",                              OPT_FUNC,   ( void* ) ::parse_player                      },
      { "player",                           OPT_FUNC,   ( void* ) ::parse_player                      },
      { "armory",                           OPT_FUNC,   ( void* ) ::parse_armory                      },
      { "guild",                            OPT_FUNC,   ( void* ) ::parse_armory                      },
      { "wowhead",                          OPT_FUNC,   ( void* ) ::parse_wowhead                     },
      { "rawr",                             OPT_FUNC,   ( void* ) ::parse_rawr                        },
      { "http_clear_cache",                 OPT_FUNC,   ( void* ) ::http_t::clear_cache               },
      { "default_region",                   OPT_STRING, &( default_region_str                       ) },
      { "default_server",                   OPT_STRING, &( default_server_str                       ) },
      { "spell_crit_suppression",           OPT_BOOL,   &( spell_crit_suppression                   ) },
      // @option_doc loc=player/all/enchant/stats title="Stat Enchants"
      { "default_enchant_strength",                 OPT_FLT,  &( enchant.attribute[ ATTR_STRENGTH  ] ) },
      { "default_enchant_agility",                  OPT_FLT,  &( enchant.attribute[ ATTR_AGILITY   ] ) },
      { "default_enchant_stamina",                  OPT_FLT,  &( enchant.attribute[ ATTR_STAMINA   ] ) },
      { "default_enchant_intellect",                OPT_FLT,  &( enchant.attribute[ ATTR_INTELLECT ] ) },
      { "default_enchant_spirit",                   OPT_FLT,  &( enchant.attribute[ ATTR_SPIRIT    ] ) },
      { "default_enchant_spell_power",              OPT_FLT,  &( enchant.spell_power                 ) },
      { "default_enchant_mp5",                      OPT_FLT,  &( enchant.mp5                         ) },
      { "default_enchant_attack_power",             OPT_FLT,  &( enchant.attack_power                ) },
      { "default_enchant_expertise_rating",         OPT_FLT,  &( enchant.expertise_rating            ) },
      { "default_enchant_armor_penetration_rating", OPT_FLT,  &( enchant.armor_penetration_rating    ) },
      { "default_enchant_armor",                    OPT_FLT,  &( enchant.bonus_armor                 ) },
      { "default_enchant_defense_rating",           OPT_FLT,  &( enchant.defense_rating              ) },
      { "default_enchant_dodge_rating",             OPT_FLT,  &( enchant.dodge_rating                ) },
      { "default_enchant_parry_rating",             OPT_FLT,  &( enchant.parry_rating                ) },
      { "default_enchant_block_rating",             OPT_FLT,  &( enchant.block_rating                ) },
      { "default_enchant_block_value",              OPT_FLT,  &( enchant.block_value                 ) },
      { "default_enchant_haste_rating",             OPT_FLT,  &( enchant.haste_rating                ) },
      { "default_enchant_hit_rating",               OPT_FLT,  &( enchant.hit_rating                  ) },
      { "default_enchant_crit_rating",              OPT_FLT,  &( enchant.crit_rating                 ) },
      { "default_enchant_health",                   OPT_FLT,  &( enchant.resource[ RESOURCE_HEALTH ] ) },
      { "default_enchant_mana",                     OPT_FLT,  &( enchant.resource[ RESOURCE_MANA   ] ) },
      { "default_enchant_rage",                     OPT_FLT,  &( enchant.resource[ RESOURCE_RAGE   ] ) },
      { "default_enchant_energy",                   OPT_FLT,  &( enchant.resource[ RESOURCE_ENERGY ] ) },
      { "default_enchant_focus",                    OPT_FLT,  &( enchant.resource[ RESOURCE_FOCUS  ] ) },
      { "default_enchant_runic",                    OPT_FLT,  &( enchant.resource[ RESOURCE_RUNIC  ] ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, global_options );

    target  -> get_options( options );
    scaling -> get_options( options );
    plot    -> get_options( options );
  }

  return options;
}

// sim_t::parse_option ======================================================

bool sim_t::parse_option( const std::string& name,
                          const std::string& value )
{
  if ( canceled ) return false;

  if ( active_player )
    if ( option_t::parse( this, active_player -> get_options(), name, value ) )
      return true;

  return option_t::parse( this, get_options(), name, value );
}

// sim_t::parse_options =====================================================

bool sim_t::parse_options( int    _argc,
                           char** _argv )
{
  argc = _argc;
  argv = _argv;

  if ( argc <= 1 ) return false;

  for ( int i=1; i < argc; i++ )
  {
    if ( ! option_t::parse_line( this, argv[ i ] ) )
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
    }
  }
  if ( ! log_file_str.empty() )
  {
    log_file = fopen( log_file_str.c_str(), "w" );
    if ( ! log_file )
    {
      errorf( "Unable to open combat log file '%s'\n", log_file_str.c_str() );
      cancel();
    }
    log = 1;
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
  else if ( current_iteration >= 0 )
  {
    phase = "Simulating";
    return current_iteration / (double) iterations;
  }
  else if ( current_slot >= 0 )
  {
    phase = current_name;
    return current_slot / (double) SLOT_MAX;
  }

  return 0.0;
}

// sim_t::main ==============================================================

int sim_t::main( int argc, char** argv )
{
  int arch = 0, version = 0, revision = 0;

  sim_signal_handler_t::init( this );

  thread_t::init();

  http_t::cache_load();

  if ( ! parse_options( argc, argv ) )
  {
    errorf( "ERROR! Incorrect option format..\n" );
    cancel();
  }

  if( canceled ) return 0;

  current_throttle = armory_throttle;

  patch.decode(&arch, &version, &revision);
  util_t::fprintf( output_file,
                   "\nSimulationCraft for World of Warcraft release %d.%d.%d ( iterations=%d, max_time=%.0f, vary_combat_length=%.3f, optimal_raid=%d, smooth_rng=%d )\n",
                   arch, version, revision, iterations, max_time, vary_combat_length, optimal_raid, smooth_rng );
  fflush( output_file );

  if ( need_to_save_profiles( this ) )
  {
    init();
    util_t::fprintf( stdout, "\nGenerating profiles... \n" ); fflush( stdout );
    report_t::print_profiles( this );
  }
  else
  {
    if ( max_time <= 0 && target -> fixed_health <= 0 )
    {
      util_t::fprintf( output_file, "simulationcraft: One of -max_time or -target_health must be specified.\n" );
      exit( 0 );
    }

    util_t::fprintf( stdout, "\nGenerating baseline... \n" ); fflush( stdout );

    if( execute() )
    {
      scaling -> analyze();
      plot    -> analyze();
      util_t::fprintf( stdout, "\nGenerating reports...\n" ); fflush( stdout );
      report_t::print_suite( this );
    }
  }

  if ( output_file != stdout ) fclose( output_file );
  if ( log_file ) fclose( log_file );

  http_t::cache_save();

  sim_signal_handler_t::init( 0 );

  return 0;
}

// sim_t::errorf ============================================================

int sim_t::errorf( const char* format, ... )
{
  va_list fmtargs;
  int retcode = 0;
  char *p_locale = NULL;
  char buffer_locale[ 1024 ];
  char buffer_printf[ 1024 ];

  p_locale = setlocale( LC_CTYPE, NULL );
  if ( p_locale != NULL )
  {
    strncpy( buffer_locale, p_locale, 1023 );
    buffer_locale[1023] = '\0';
  }
  else
  {
    buffer_locale[0] = '\0';
  }

  setlocale( LC_CTYPE, "" );

  va_start( fmtargs, format );
  retcode = vsnprintf( buffer_printf, 1023, format, fmtargs );
  va_end( fmtargs );

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

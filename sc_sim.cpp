// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Signal Handler
// ==========================================================================

#if SIGACTION
#include <signal.h>
#endif

struct sim_signal_handler_t
{
  static int seed;
  static int iteration;
  static void callback_func( int signal )
  {
    fprintf( stderr, "sim_signal_handler:  Seed=%d  Iteration=%d\n", seed, iteration );
    fflush( stderr );
    exit(0);
  }
  static void init( sim_t* sim )
  {
    seed = sim -> seed;
#if SIGACTION
    struct sigaction sa;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;
    sa.sa_handler = callback_func;
    sigaction( SIGSEGV, &sa, 0 );
#endif
  }
};

int sim_signal_handler_t::seed = 0;
int sim_signal_handler_t::iteration = 0;

// ==========================================================================
// Timing Wheel
// ==========================================================================

// timing_wheel_t::timing_wheel_t ===========================================

timing_wheel_t::timing_wheel_t() : 
  time_horizon( 600.0 ), granularity( 0.01 ),
  size( 0 ), events( 0 )
{
}

// timing_wheel_t::push =====================================================

void timing_wheel_t::push( event_t* e )
{
  assert( false );
}

// timing_wheel_t::top ======================================================

event_t* timing_wheel_t::top()
{
  assert( false );
}

// timing_wheel_t::pop ======================================================

void timing_wheel_t::pop()
{
  assert( false );
}

// timing_wheel_t::empty ====================================================

bool timing_wheel_t::empty()
{
  assert( false );
}

// ==========================================================================
// Timing List
// ==========================================================================

// timing_list_t::timing_list_t =============================================

timing_list_t::timing_list_t() : 
  events( 0 )
{
}

// timing_list_t::push ======================================================

void timing_list_t::push( event_t* e )
{
  event_t** prev = &events;

  while( (*prev) && (*prev) -> time < e -> time )
    prev = &( (*prev) -> next );

  e -> next = *prev;
  *prev = e;
}

// timing_list_t::top =======================================================

event_t* timing_list_t::top()
{
  return events;
}

// timing_list_t::pop =======================================================

void timing_list_t::pop()
{
  // Requires event deletion to occur AFTER pop().
  events = events -> next;
}

// timing_list_t::empty =====================================================

bool timing_list_t::empty()
{
  return events != 0;
}

// ==========================================================================
// Simulator
// ==========================================================================

bool sim_t::WotLK = false;

// sim_t::sim_t =============================================================

sim_t::sim_t() : 
  method( SIM_LIST ), player_list(0), active_player(0), target(0),
  lag(0), reaction_time(0.5), regen_periodicity(1.0), current_time(0), max_time(0),
  events_remaining(0), max_events_remaining(0), 
  events_processed(0), total_events_processed(0),
  seed(0), id(0), iterations(1),
  potion_sickness(0), average_dmg(1), log(0), debug(0), timestamp(1), 
  uptime_list(0), output_file(stdout), html_file(stdout), 
  report(0), raid_dps(0), total_dmg(0), total_seconds(0), elapsed_cpu_seconds(0), merge_ignite(0)
{
  for( int i=0; i < RESOURCE_MAX; i++ ) 
  {
    infinite_resource[ i ] = 0;
  }
  target = new target_t( this );
  report = new report_t( this );
}

// sim_t::add_event ==========================================================

void sim_t::add_event( event_t* e,
		       double   delta_time )
{
   assert( delta_time >= 0 );
   e -> time = current_time + delta_time;
   e -> id   = ++id;
   switch( method )
   {
   case SIM_LIST:      timing_list.push( e );      break;
   case SIM_WHEEL:     timing_wheel.push( e );     break;
   case SIM_PRIORITYQ: timing_priorityq.push( e ); break;
   default: assert( false );
   }
   events_remaining++;
   if( events_remaining > max_events_remaining ) max_events_remaining = events_remaining;
   if( debug ) report_t::log( this, "Add Event: %s %.2f %d", e -> name, e -> time, e -> id );
}

// sim_t::reschedule_event ==========================================================

void sim_t::reschedule_event( event_t* e )
{
   if( debug ) report_t::log( this, "Reschedule Event: %s %d", e -> name, e -> id );
   add_event( e, ( e -> reschedule_time - current_time ) );
   e -> reschedule_time = 0;
}

// sim_t::next_event ========================================================

event_t* sim_t::next_event()
{
  event_t* e=0;

  if( method == SIM_LIST )
  {
    e = timing_list.top();
    if( e ) timing_list.pop();
  }
  else if( method == SIM_WHEEL )
  {
    e = timing_wheel.top();
    if( e ) timing_wheel.pop();
  }
  else if( method == SIM_PRIORITYQ )
  {
    if( ! timing_priorityq.empty() )
    {
      e = timing_priorityq.top();
      timing_priorityq.pop();
    }
  }
  else assert( false );

  return e;
}

// sim_t::execute ===========================================================

bool sim_t::execute()
{
  if( debug ) report_t::log( this, "Starting Simulator" );

  while( event_t* e = next_event() )
  {
    events_remaining--;
    events_processed++;

    current_time = e -> time;

    if( max_time > 0 && current_time > max_time ) 
    {
      if( debug ) report_t::log( this, "MaxTime reached, ending simulation" );     
      delete e;
      break;
    }
    if( target -> initial_health > 0 && target -> current_health <= 0 )
    {
      if( debug ) report_t::log( this, "Target has died, ending simulation" );     
      delete e;
      break;
    }
    if( target -> initial_health == 0 && current_time > ( max_time / 2.0 ) )
    {
      target -> recalculate_health();
    }
    if( e -> canceled ) 
    {
      if( debug ) report_t::log( this, "Canceled event: %s", e -> name );     
    }
    else if( e -> reschedule_time > e -> time )
    {
      reschedule_event( e );
      continue;
    }
    else
    {
      if( debug ) report_t::log( this, "Executing event: %s", e -> name );     
      e -> execute();
      
      if( e -> player ) e -> player -> last_action = current_time;
    }
    delete e;
  }
  total_seconds += current_time;
  total_events_processed += events_processed;

  for( player_t* p = player_list; p; p = p -> next )
  {
    p -> total_seconds += p -> last_action;
  }

  return true;
}

// sim_t::flush_events ======================================================

void sim_t::flush_events()
{
   if( debug ) report_t::log( this, "Flush Events" );
   while( event_t* e = next_event() )
     delete e;
   events_remaining = 0;
   events_processed = 0;
   id = 0;
}

// sim_t::reset =============================================================

void sim_t::reset()
{
  flush_events();
  if( debug ) report_t::log( this, "Reseting Simulator" );
  current_time = id = 0;
  stats_t::last_execute = 0;
  target -> reset();
  for( player_t* p = player_list; p; p = p -> next )
    p -> reset();
  new regen_event_t( this );
}

// sim_t::init ==============================================================

bool sim_t::init()
{
  if( debug ) log = 1;

  total_seconds = 0;

  if( seed == 0 ) seed = time( NULL );
  rand_t::init( seed );

  sim_signal_handler_t::init( this );

  if( ! method_str.empty() )
  {
    if     ( method_str == "list"      || method_str == "LIST"      ) method = SIM_LIST;
    else if( method_str == "wheel"     || method_str == "WHEEL"     ) method = SIM_WHEEL;
    else if( method_str == "priorityq" || method_str == "PRIORITYQ" ) method = SIM_PRIORITYQ;
    else assert( false );
  }

  if( ! patch_str.empty() )
  {
    int8_t arch, version, revision;
    if( 3 != util_t::string_split( patch_str, ".", "i8 i8 i8", &arch, &version, &revision ) )
    {
      fprintf( output_file, "util_t::sim: Expected format: -patch=#.#.#\n" );
      return false;
    }
    patch.set( arch, version, revision );

    WotLK = patch.after( 3, 0, 0 );
  }

  target -> init();

  bool too_quiet = true;

  for( player_t* p = player_list; p; p = p -> next )
  {
    p -> init();
    if( ! p -> quiet ) too_quiet = false;
  }

  if( too_quiet && ! debug ) exit(0);

  return true;
}

// sim_t::analyze ============================================================

struct compare_dps {
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> dps > r -> dps;
  }
};

struct compare_name {
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> name_str < r -> name_str;
  }
};

void sim_t::analyze()
{
  if( total_seconds == 0 ) return;

  total_dmg = 0;
  total_seconds /= iterations;

  for( player_t* p = player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;

    players_by_rank.push_back( p );
    players_by_name.push_back( p );

    p -> total_seconds /= iterations;
    p -> total_waiting /= iterations;
    p -> total_dmg = 0;

    std::vector<stats_t*> stats_list;

    for( stats_t* s = p -> stats_list; s; s = s -> next )
    {
      stats_list.push_back( s );
    }

    for( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
    {
      for( stats_t* s = pet -> stats_list; s; s = s -> next )
      {
	stats_list.push_back( s );
      }
    }

    int num_stats = stats_list.size();

    for( int i=0; i < num_stats; i++ )
    {
      stats_t* s = stats_list[ i ];

      s -> analyze();
      p -> total_dmg += s -> total_dmg;
    }

    p -> dps = p -> total_dmg / p -> total_seconds;

    // Avoid double-counting of pet damage
    if( p -> type != PLAYER_PET ) total_dmg += p -> total_dmg;

    for( int i=0; i < RESOURCE_MAX; i++ )
    {
      p -> resource_lost  [ i ] /= iterations;
      p -> resource_gained[ i ] /= iterations;
    }

    p -> dpr = p -> total_dmg / p -> resource_lost[ p -> primary_resource() ];

    p -> rps_loss = p -> resource_lost  [ p -> primary_resource() ] / p -> total_seconds;
    p -> rps_gain = p -> resource_gained[ p -> primary_resource() ] / p -> total_seconds;

    for( gain_t* g = p -> gain_list; g; g = g -> next )
    {
      g -> amount /= iterations;
    }

    for( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
    {
      if( proc -> count > 0 ) 
      {
	proc -> count /= iterations;
	proc -> frequency = p -> total_seconds / proc -> count;
      }
    }

    int max_buckets = (int) p -> total_seconds;
    
    p -> timeline_dmg.clear();
    p -> timeline_dps.clear();

    p -> timeline_dmg.insert( p -> timeline_dmg.begin(), max_buckets, 0 );
    p -> timeline_dps.insert( p -> timeline_dps.begin(), max_buckets, 0 );

    for( int i=0; i < num_stats; i++ )
    {
      stats_t* s = stats_list[ i ];

      for( int j=0; ( j < max_buckets ) && ( j < s -> num_buckets ); j++ )
      {
	p -> timeline_dmg[ j ] += s -> timeline_dmg[ j ];
      }
    }

    for( int i=0; i < max_buckets; i++ )
    {
      double window_dmg  = p -> timeline_dmg[ i ];
      int    window_size = 1;

      for( int j=1; ( j <= 10 ) && ( (i-j) >=0 ); j++ )
      {
	window_dmg += p -> timeline_dmg[ i-j ];
	window_size++;
      }
      for( int j=1; ( j <= 10 ) && ( (i+j) < max_buckets ); j++ )
      {
	window_dmg += p -> timeline_dmg[ i+j ];
	window_size++;
      }

      p -> timeline_dps[ i ] = window_dmg / window_size;
    }
  }

  std::sort( players_by_rank.begin(), players_by_rank.end(), compare_dps()  );
  std::sort( players_by_name.begin(), players_by_name.end(), compare_name() );

  raid_dps = total_dmg / total_seconds;
}

// sim_t::get_uptime ========================================================

uptime_t* sim_t::get_uptime( const std::string& name )
{
  uptime_t* u=0;

  for( u = uptime_list; u; u = u -> next )
  {
    if( u -> name_str == name )
      return u;
  }

  u = new uptime_t( name );

  uptime_t** tail = &uptime_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// sim_t::parse_option ======================================================

bool sim_t::parse_option( const std::string& name,
			  const std::string& value )
{
  option_t options[] =
  {
    { "average_dmg",             OPT_INT8,   &( average_dmg                          ) },
    { "debug",                   OPT_INT8,   &( debug                                ) },
    { "infinite_energy",         OPT_INT8,   &( infinite_resource[ RESOURCE_ENERGY ] ) },
    { "infinite_focus",          OPT_INT8,   &( infinite_resource[ RESOURCE_FOCUS  ] ) },
    { "infinite_health",         OPT_INT8,   &( infinite_resource[ RESOURCE_HEALTH ] ) },
    { "infinite_mana",           OPT_INT8,   &( infinite_resource[ RESOURCE_MANA   ] ) },
    { "infinite_rage",           OPT_INT8,   &( infinite_resource[ RESOURCE_RAGE   ] ) },
    { "infinite_runic",          OPT_INT8,   &( infinite_resource[ RESOURCE_RUNIC  ] ) },
    { "iterations",              OPT_INT32,  &( iterations                           ) },
    { "lag",                     OPT_FLT,    &( lag                                  ) },
    { "merge_ignite",            OPT_INT8,   &( merge_ignite                         ) },
    { "reaction_time",           OPT_FLT,    &( reaction_time                        ) },
    { "regen_periodicity",       OPT_FLT,    &( regen_periodicity                    ) },
    { "log",                     OPT_INT8,   &( log                                  ) },
    { "max_time",                OPT_FLT,    &( max_time                             ) },
    { "method"  ,                OPT_STRING, &( method_str                           ) },
    { "patch",                   OPT_STRING, &( patch_str                            ) },
    { "potion_sickness",         OPT_INT8,   &( potion_sickness                      ) },
    { "seed",                    OPT_INT32,  &( seed                                 ) },
    { "timestamp",               OPT_INT8,   &( timestamp                            ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( this, options );
    return false;
  }

  if( target -> parse_option( name, value ) ) return true;
  if( report -> parse_option( name, value ) ) return true;

  if( active_player && active_player -> parse_option( name, value ) ) return true;

  return option_t::parse( this, options, name, value );
}

// sim_t::print_options =====================================================

void sim_t::print_options()
{
  fprintf( output_file, "\nWorld of Warcraft Raid Simulator Options:\n" );

  fprintf( output_file, "\nSimulation Engine:\n" );
  parse_option( std::string(), std::string() );

  fprintf( output_file, "\nTarget: %s\n", target -> name() );
  target -> parse_option( std::string(), std::string() );

  for( player_t* p = player_list; p; p = p -> next )
  {
    fprintf( output_file, "\nPlayer: %s (%s)\n", p -> name(), util_t::player_type_string( p -> type ) );
    p -> parse_option( std::string(), std::string() );
  }

  fprintf( output_file, "\n" );
  fflush( output_file );
}

// ==========================================================================
// MAIN 
// ==========================================================================

int main( int argc, char **argv )
{
  sim_t sim;

  if( ! option_t::parse( &sim, argc, argv ) )
  {
    fprintf( sim.output_file, "ERROR! Incorrect option format..\n" );
    return -1;
  }

  if( ! sim.init() )
  {
    fprintf( sim.output_file, "ERROR! Unable to initialize.\n" );
    return -1;
  }
   
  time_t start_time = time(0);

  for( int i=0; i < sim.iterations; i++ )
  {
    sim_signal_handler_t::iteration = i;
    sim.reset();
    sim.execute();
  }
  sim.elapsed_cpu_seconds = time(0) - start_time;
  
  sim.reset();
  sim.analyze();

  sim.report -> print();
  sim.report -> chart();

  if( sim.output_file != stdout ) fclose( sim.output_file );
  if( sim.  html_file != stdout ) fclose( sim.  html_file );
  
  return 0;
}

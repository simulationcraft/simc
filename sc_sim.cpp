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

struct wow_sim_signal_handler_t
{
  static int seed;
  static int iteration;
  static void callback_func( int signal )
  {
    fprintf( stderr, "wow_sim_signal_handler:  Seed=%d  Iteration=%d\n", seed, iteration );
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

int wow_sim_signal_handler_t::seed = 0;
int wow_sim_signal_handler_t::iteration = 0;

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

// sim_t::sim_t =============================================================

sim_t::sim_t() : 
  method( SIM_LIST ), player_list(0), active_player(0), target(0),
  current_time(0), max_time(0), lag(0), total_seconds(0), 
  events_remaining(0), max_events_remaining(0), 
  events_processed(0), total_events_processed(0),
  seed(0), id(0), iterations(1),
  average_dmg(1), log(0), debug(0), timestamp(1), report(0), uptime_list(0)
{
  for( int i=0; i < RESOURCE_MAX; i++ ) 
  {
    infinite_resource[ i ] = 0;
  }
  target = new target_t( this );
  report = new report_t( this );
}

// sim_t::add_event ==========================================================

void sim_t::add_event( event_t* e )
{
   assert( e -> time >= 0 );
   e -> time += current_time;
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
   report_t::debug( this, "Add Event: %s %.2f %d", e -> name, e -> time, e -> id );
}

// sim_t::reschedule_event ==========================================================

void sim_t::reschedule_event( event_t* e )
{
   report_t::debug( this, "Reschedule Event: %s %d", e -> name, e -> id );
   e -> time = ( e -> reschedule_time - e -> time );
   e -> reschedule_time = 0;
   add_event( e );
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
  report_t::debug( this, "Starting Simulator" );

  while( event_t* e = next_event() )
  {
    events_remaining--;
    events_processed++;

    current_time = e -> time;

    if( max_time > 0 && current_time > max_time ) 
    {
      report_t::debug( this, "MaxTime reached, ending simulation" );     
      delete e;
      break;
    }
    if( target -> initial_health > 0 && target -> current_health <= 0 )
    {
      report_t::debug( this, "Target has died, ending simulation" );     
      delete e;
      break;
    }
    if( target -> initial_health == 0 && current_time > ( max_time / 2.0 ) )
    {
      target -> recalculate_health();
    }
    if( e -> invalid ) 
    {
      report_t::debug( this, "Invalid event: %s", e -> name );     
    }
    else if( e -> reschedule_time > e -> time )
    {
      reschedule_event( e );
      continue;
    }
    else
    {
      report_t::debug( this, "Executing event: %s", e -> name );     
      e -> execute();
    }
    delete e;
  }
  total_seconds += current_time;
  total_events_processed += events_processed;
  
  return true;
}

// sim_t::flush_events ======================================================

void sim_t::flush_events()
{
   report_t::debug( this, "Flush Events" );
   while( event_t* e = next_event() )
     delete e;
   events_remaining = 0;
   events_processed = 0;
   id = 0;
}

// sim_t::reset =============================================================

void sim_t::reset()
{
  report_t::debug( this, "Reseting Simulator" );
  flush_events();
  current_time = id = 0;
  target -> reset();
  for( player_t* p = player_list; p; p = p -> next )
    p -> reset();
  new regen_event_t( this );
  stats_t::last_execute = 0;
}

// sim_t::init ==============================================================

bool sim_t::init()
{
  if( debug ) log = 1;

  total_seconds = 0;

  if( seed == 0 ) seed = time( NULL );
  wow_random_init( seed );

  wow_sim_signal_handler_t::init( this );

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
    if( 3 != wow_string_split( patch_str, ".", "i8 i8 i8", &arch, &version, &revision ) )
    {
      printf( "wow_sim: Expected format: -patch=#.#.#\n" );
      return false;
    }
    patch.set( arch, version, revision );
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
  u -> next = uptime_list;
  uptime_list = u;

  return u;
}

// sim_t::parse_option ======================================================

bool sim_t::parse_option( const std::string& name,
			  const std::string& value )
{
  option_t options[] =
  {
    { "method"  ,                OPT_STRING, &( method_str                           ) },
    { "patch",                   OPT_STRING, &( patch_str                            ) },
    { "max_time",                OPT_FLT,    &( max_time                             ) },
    { "lag",                     OPT_FLT,    &( lag                                  ) },
    { "seed",                    OPT_INT32,  &( seed                                 ) },
    { "iterations",              OPT_INT32,  &( iterations                           ) },
    { "infinite_mana",           OPT_INT8,   &( infinite_resource[ RESOURCE_MANA   ] ) },
    { "infinite_health",         OPT_INT8,   &( infinite_resource[ RESOURCE_HEALTH ] ) },
    { "infinite_rage",           OPT_INT8,   &( infinite_resource[ RESOURCE_RAGE   ] ) },
    { "infinite_energy",         OPT_INT8,   &( infinite_resource[ RESOURCE_ENERGY ] ) },
    { "infinite_focus",          OPT_INT8,   &( infinite_resource[ RESOURCE_FOCUS  ] ) },
    { "infinite_runic",          OPT_INT8,   &( infinite_resource[ RESOURCE_RUNIC  ] ) },
    { "average_dmg",             OPT_INT8,   &( average_dmg                          ) },
    { "log",                     OPT_INT8,   &( log                                  ) },
    { "debug",                   OPT_INT8,   &( debug                                ) },
    { "timestamp",               OPT_INT8,   &( timestamp                            ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( options );
    return false;
  }

  if( target -> parse_option( name, value ) ) return true;
  if( report -> parse_option( name, value ) ) return true;

  if( active_player && active_player -> parse_option( name, value ) ) return true;

  return option_t::parse( options, name, value );
}

// sim_t::print_options =====================================================

void sim_t::print_options()
{
  printf( "\nWorld of Warcraft Raid Simulator Options:\n" );

  printf( "\nSimulation Engine:\n" );
  parse_option( std::string(), std::string() );

  printf( "\nTarget: %s\n", target -> name() );
  target -> parse_option( std::string(), std::string() );

  for( player_t* p = player_list; p; p = p -> next )
  {
    printf( "\nPlayer: %s (%s)\n", p -> name(), wow_player_type_string( p -> type ) );
    p -> parse_option( std::string(), std::string() );
  }

  printf( "\n" );
  fflush( stdout );
}

// ==========================================================================
// MAIN 
// ==========================================================================

int main( int argc, char **argv )
{
   sim_t sim;

   if( ! option_t::parse( argc, argv, &sim ) )
   {
      printf( "ERROR! Incorrect option format..\n" );
      return -1;
   }

   if( ! sim.init() )
   {
      printf( "ERROR! Unable to initialize.\n" );
      return -1;
   }
   
   for( int i=0; i < sim.iterations; i++ )
   {
     wow_sim_signal_handler_t::iteration = i;
     sim.reset();
     sim.execute();
   }
   sim.reset();
   sim.report -> print();
   
   return 0;
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Simulator
// ==========================================================================

// sim_t::sim_t =============================================================

sim_t::sim_t( sim_t* p ) : 
  parent(p), P309(true), rng(0), free_list(0), player_list(0), active_player(0), num_players(0),
  queue_lag(0.075), queue_lag_range(0), 
  gcd_lag(0.150), gcd_lag_range(0), 
  channel_lag(0.250), channel_lag_range(0),
  travel_variance(0),
  reaction_time(0.5), regen_periodicity(1.0), 
  current_time(0), max_time(0),
  events_remaining(0), max_events_remaining(0), 
  events_processed(0), total_events_processed(0),
  seed(0), id(0), iterations(1000), current_iteration(0), threads(0),
  armor_update_interval(20), potion_sickness(1),
  optimal_raid(0), average_dmg(1),
  log(0), debug(0), sfmt(1),
  jow_chance(0), jow_ppm(15.0),
  timing_wheel(0), wheel_seconds(0), wheel_size(0), wheel_mask(0), timing_slice(0), wheel_granularity(0.0),
  replenishment_targets(0),
  raid_dps(0), total_dmg(0), 
  total_seconds(0), elapsed_cpu_seconds(0), 
  merge_ignite(0), report_progress(1),
  output_file(stdout), log_file(0), thread_handle(0)
{

  for( int i=0; i < RESOURCE_MAX; i++ ) 
  {
    infinite_resource[ i ] = false;
  }

  target  = new  target_t( this );
  scaling = new scaling_t( this );

  if( parent ) 
  {
    // Import the config file
    option_t::parse( this, parent -> argc, parent -> argv );

    // Inherit 'delta_gear' settings from parent because these may be set outside of the config file
    gear_delta = parent -> gear_delta;
  }
}

// sim_t::~sim_t ============================================================

sim_t::~sim_t()
{
  flush_events();

  while( player_t* p = player_list )
  {
    player_list = p -> next;
    delete p;
  }

  while( event_t* e = free_list )
  {
    free_list = e -> next;
    event_t::deallocate( e );
  }

  if( rng     ) delete rng;
  if( target  ) delete target;
  if( scaling ) delete scaling;
  
  int num_children = children.size();
  for( int i=0; i < num_children; i++ ) 
  {
    delete children[ i ];
  }
  if (timing_wheel) delete timing_wheel;
}

// sim_t::add_event ==========================================================

void sim_t::add_event( event_t* e,
                       double   delta_time )
{
  if( delta_time <= 0 ) delta_time = 0.0000001;

  e -> time = current_time + delta_time;
  e -> id   = ++id;

  uint32_t slice = ( (uint32_t) ( e -> time * wheel_granularity ) ) & wheel_mask;
  
  event_t** prev = &( timing_wheel[ slice ] );

  while( (*prev) && (*prev) -> time < e -> time ) prev = &( (*prev) -> next );

  e -> next = *prev;
  *prev = e;

  events_remaining++;
  if( events_remaining > max_events_remaining ) max_events_remaining = events_remaining;

  if( debug ) log_t::output( this, "Add Event: %s %.2f %d", e -> name, e -> time, e -> id );
}

// sim_t::reschedule_event ====================================================

void sim_t::reschedule_event( event_t* e )
{
   if( debug ) log_t::output( this, "Reschedule Event: %s %d", e -> name, e -> id );

   add_event( e, ( e -> reschedule_time - current_time ) );

   e -> reschedule_time = 0;
}

// sim_t::next_event ==========================================================

event_t* sim_t::next_event()
{
  if( events_remaining == 0 ) return 0;

  while( 1 )
  {
    event_t*& event_list = timing_wheel[ timing_slice ];
    
    if( event_list )
    {
      event_t* e = event_list;
      event_list = e -> next;
      events_remaining--;
      events_processed++;
      return e;
    }

    timing_slice++;
    if( timing_slice == wheel_size ) timing_slice = 0;
  }

  return 0;
}

// sim_t::flush_events ======================================================

void sim_t::flush_events()
{
   if( debug ) log_t::output( this, "Flush Events" );

   for( int i=0; i < wheel_size; i++ )
   {
     while( event_t* e = timing_wheel[ i ] )
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
  for( int i=0; i < wheel_size; i++ )
  {
    for( event_t* e = timing_wheel[ i ]; e; e = e -> next )
    {
      if( e -> player == p ) 
      {
        e -> canceled = 1;
      }
    }
  }
}

// sim_t::combat ==============================================================

void sim_t::combat( int iteration )
{
  if( debug ) log_t::output( this, "Starting Simulator" );

  current_iteration = iteration;

  combat_begin();

  while( event_t* e = next_event() )
  {
    current_time = e -> time;

    if( max_time > 0 && current_time > max_time ) 
    {
      if( debug ) log_t::output( this, "MaxTime reached, ending simulation" );     
      delete e;
      break;
    }
    if( target -> initial_health > 0 && target -> current_health <= 0 )
    {
      target -> recalculate_health();
      if( debug ) log_t::output( this, "Target has died, ending simulation" );     
      delete e;
      break;
    }
    if( target -> initial_health == 0 && current_time > ( max_time / 2.0 ) )
    {
      target -> recalculate_health();
    }
    if( e -> canceled ) 
    {
      if( debug ) log_t::output( this, "Canceled event: %s", e -> name );     
    }
    else if( e -> reschedule_time > e -> time )
    {
      reschedule_event( e );
      continue;
    }
    else
    {
      if( debug ) log_t::output( this, "Executing event: %s", e -> name );     
      e -> execute();
      
      if( e -> player ) e -> player -> last_action_time = current_time;
    }
    delete e;
  }

  combat_end();
}

// sim_t::reset =============================================================

void sim_t::reset()
{
  if( debug ) log_t::output( this, "Reseting Simulator" );
  current_time = id = 0;
  auras.reset();
  expirations.reset();
  target -> reset();
  for( player_t* p = player_list; p; p = p -> next )
  {
    p -> reset();
  }
}

// sim_t::combat_begin ======================================================

void sim_t::combat_begin()
{
  if( debug ) log_t::output( this, "Combat Begin" );

  reset();

  if( overrides.improved_moonkin_aura  ) auras.improved_moonkin = 1;
  if( overrides.leader_of_the_pack     ) auras.leader_of_the_pack = 1;
  if( overrides.moonkin_aura           ) auras.moonkin = 1;
  if( overrides.sanctified_retribution ) auras.sanctified_retribution = 1;
  if( overrides.swift_retribution      ) auras.swift_retribution = 1;
  if( overrides.trueshot_aura          ) auras.trueshot = 1;

  target -> combat_begin();

  for( player_t* p = player_list; p; p = p -> next )
  {
    p -> combat_begin();
  }
  new ( this ) regen_event_t( this );
}

// sim_t::combat_end ========================================================

void sim_t::combat_end()
{
  if( debug ) log_t::output( this, "Combat End" );

  total_seconds += current_time;
  total_events_processed += events_processed;

  flush_events();

  target -> combat_end();

  for( player_t* p = player_list; p; p = p -> next )
  {
    p -> combat_end();
  }
}

// sim_t::init ==============================================================

bool sim_t::init()
{
  P309 = patch.before( 3, 1, 0 );

  rng = rng_t::init( sfmt );

  // Timing wheel depth defaults to 10 minutes with a granularity of 10 buckets per second.
  if( wheel_seconds     <= 0 ) wheel_seconds     = 600;
  if( wheel_granularity <= 0 ) wheel_granularity = 10;
  
  wheel_size = (uint32_t) ( wheel_seconds * wheel_granularity );

  // Round up the wheel depth to the nearest power of 2 to enable a fast "mod" operation.
  for( wheel_mask = 2; wheel_mask < wheel_size; wheel_mask *= 2 );
  wheel_size = wheel_mask;
  wheel_mask--;

  // The timing wheel represents an array of event lists: Each time slice has an event list.
  if (timing_wheel) delete timing_wheel;
  timing_wheel= new event_t*[wheel_size+1];
  memset(timing_wheel,0,sizeof(event_t*)*(wheel_size+1)); 

  total_seconds = 0;

  if(   queue_lag_range == 0 )   queue_lag_range =   queue_lag * 0.25;
  if(     gcd_lag_range == 0 )     gcd_lag_range =     gcd_lag * 0.25;
  if( channel_lag_range == 0 ) channel_lag_range = channel_lag * 0.25;

  target -> init();

  bool too_quiet = true;

  for( player_t* p = player_list; p; p = p -> next )
  {
    p -> init();
    if( ! p -> quiet ) too_quiet = false;
  }

  if( too_quiet && ! debug ) exit(0);

  // Defer party creation after player_t::init() calls to handle any pets created there.

  int party_index=0;
  for( unsigned i=0; i < party_encoding.size(); i++ )
  {
    std::string& party_str = party_encoding[ i ];

    if( party_str == "reset" )
    {
      party_index = 0;
      for( player_t* p = player_list; p; p = p -> next ) p -> party = 0;
    }
    else if( party_str == "all" )
    {
      int member_index = 0;
      for( player_t* p = player_list; p; p = p -> next ) 
      {
        p -> party = 1;
        p -> member = member_index++;
      }
    }
    else
    {
      party_index++;

      std::vector<std::string> player_names;
      int num_players = util_t::string_split( player_names, party_str, ",;/" );
      int member_index=0;

      for( int j=0; j < num_players; j++ )
      {
        player_t* p = find_player( player_names[ j ] );
        if( ! p ) printf( "simcraft: ERROR! Unable to find player %s\n", player_names[ j ].c_str() );
        assert( p );
        p -> party = party_index;
        p -> member = member_index++;
        for( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet ) 
        {
          pet -> party = party_index;
          pet -> member = member_index++;
        }
      }
    }
  }

  for( player_t* p = player_list; p; p = p -> next )
  {
    p -> register_callbacks();
  }

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
    p -> total_dmg = 0;
    p -> total_seconds /= iterations;
    p -> total_waiting /= iterations;
  }

  for( player_t* p = player_list; p; p = p -> next )
  {
    if( p -> quiet ) continue;

    players_by_rank.push_back( p );
    players_by_name.push_back( p );

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
    if( ! p -> is_pet() ) total_dmg += p -> total_dmg;

    int max_buckets = (int) p -> total_seconds;
    int num_buckets = p -> timeline_resource.size();

    if( num_buckets > max_buckets ) p -> timeline_resource.resize( max_buckets );

    for( int i=0; i < max_buckets; i++ )
    {
      p -> timeline_resource[ i ] /= iterations;
    }

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
      g -> actual   /= iterations;
      g -> overflow /= iterations;
    }

    for( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
    {
      if( proc -> count > 0 ) 
      {
        proc -> count /= iterations;
        proc -> frequency = p -> total_seconds / proc -> count;
      }
    }

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

    assert( p -> iteration_dps.size() == (size_t) iterations );

    p -> dps_min = 0;
    p -> dps_max = 0;
    p -> dps_std_dev = 0;
    for( int i=0; i < iterations; i++ )
    {
      double i_dps = p -> iteration_dps[ i ];
      if( p -> dps_min == 0 || p -> dps_min > i_dps ) p -> dps_min = i_dps;
      if( p -> dps_max == 0 || p -> dps_max < i_dps ) p -> dps_max = i_dps;
      double delta = i_dps - p -> dps;
      p -> dps_std_dev += delta * delta;
    }
    p -> dps_std_dev /= iterations;
    p -> dps_std_dev = sqrt( p -> dps_std_dev );
    p -> dps_error = 2.0 * p -> dps_std_dev / sqrt( (float) iterations );

    if( ( p -> dps_max - p -> dps_min ) > 0 )
    {
      int num_buckets = 50;
      double min = p -> dps_min - 1;
      double max = p -> dps_max + 1;
      double range = max - min;

      p -> distribution_dps.insert( p -> distribution_dps.begin(), num_buckets, 0 );

      for( int i=0; i < iterations; i++ )
      {
        double i_dps = p -> iteration_dps[ i ];
        int index = (int) ( num_buckets * ( i_dps - min ) / range );
        p -> distribution_dps[ index ]++;
      }
    }
  }

  std::sort( players_by_rank.begin(), players_by_rank.end(), compare_dps()  );
  std::sort( players_by_name.begin(), players_by_name.end(), compare_name() );

  raid_dps = total_dmg / total_seconds;

  chart_t::raid_dps     ( dps_charts,     this );
  chart_t::raid_dpet    ( dpet_charts,    this );
  chart_t::raid_gear    ( gear_charts,    this );
  chart_t::raid_downtime( downtime_chart, this );
  chart_t::raid_uptimes ( uptimes_chart,  this );

  for( player_t* p = player_list; p; p = p -> next )
  {
    if( p -> quiet ) continue;

    chart_t::action_dpet      ( p -> action_dpet_chart,       p );
    chart_t::action_dmg       ( p -> action_dmg_chart,        p );
    chart_t::gains            ( p -> gains_chart,             p );
    chart_t::uptimes_and_procs( p -> uptimes_and_procs_chart, p );
    chart_t::timeline_resource( p -> timeline_resource_chart, p );
    chart_t::timeline_dps     ( p -> timeline_dps_chart,      p );
    chart_t::distribution_dps ( p -> distribution_dps_chart,  p );
  }
}

// sim_t::iterate ===========================================================

void sim_t::iterate()
{
  init();

  int message_interval = iterations/10;
  int message_index = 10;

  for( int i=0; i < iterations; i++ )
  {
    if( report_progress && ( message_interval > 0 ) && ( i % message_interval == 0 ) )
    {
      fprintf( stdout, "%d... ", message_index-- );
      fflush( stdout );
    }
    combat( i );
  }
  if( report_progress ) fprintf( stdout, "\n" );

  reset();
}

// sim_t::merge =============================================================

void sim_t::merge( sim_t& other_sim )
{
  iterations             += other_sim.iterations;
  total_seconds          += other_sim.total_seconds;
  total_events_processed += other_sim.total_events_processed;

  if( max_events_remaining < other_sim.max_events_remaining ) max_events_remaining = other_sim.max_events_remaining;

  for( player_t* p = player_list; p; p = p -> next )
  {
    player_t* other_p = other_sim.find_player( p -> name() );
    assert( other_p );

    p -> total_seconds += other_p -> total_seconds;
    p -> total_waiting += other_p -> total_waiting;

    for( int i=0; i < other_sim.iterations; i++ )
    {
      p -> iteration_dps.push_back( other_p -> iteration_dps[ i ] );
    }

    int num_buckets = std::min(       p -> timeline_resource.size(), 
                                other_p -> timeline_resource.size() );

    for( int i=0; i < num_buckets; i++ )
    {
      p -> timeline_resource[ i ] += other_p -> timeline_resource[ i ];
    }

    for( int i=0; i < RESOURCE_MAX; i++ )
    {
      p -> resource_lost  [ i ] += other_p -> resource_lost  [ i ];
      p -> resource_gained[ i ] += other_p -> resource_gained[ i ];
    }

    for( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
    {
      proc -> merge( other_p -> get_proc( proc -> name_str ) );
    }    

    for( gain_t* gain = p -> gain_list; gain; gain = gain -> next )
    {
      gain -> merge( other_p -> get_gain( gain -> name_str ) );
    }    

    for( stats_t* stats = p -> stats_list; stats; stats = stats -> next )
    {
      stats -> merge( other_p -> get_stats( stats -> name_str ) );
    }    

    for( uptime_t* uptime = p -> uptime_list; uptime; uptime = uptime -> next )
    {
      uptime -> merge( other_p -> get_uptime( uptime -> name_str ) );
    }    
  }  
}

// sim_t::merge =============================================================

void sim_t::merge()
{
  int num_children = children.size();
  
  for( int i=0; i < num_children; i++ )
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
  if( threads <= 1 ) return;
  if( iterations < threads ) return;

#if defined( NO_THREADS )
  fprintf( output_file, "simcraft: This executable was built without thread support, please remove 'threads=N' from config file.\n" );
  exit(0);
#endif

  iterations /= threads;

  int num_children = threads - 1;
  children.resize( num_children );

  for( int i=0; i < num_children; i++ )
  {
    sim_t* child = children[ i ] = new sim_t( this );
    child -> iterations /= threads;
    child -> report_progress = 0;
  }

  for( int i=0; i < num_children; i++ )
  {
    thread_t::launch( children[ i ] );
  }
}

// sim_t::execute ===========================================================

void sim_t::execute()
{
  int start_time = util_t::miliseconds();

  partition();
  iterate();
  merge();
  analyze();

  elapsed_cpu_seconds = ( util_t::miliseconds() - start_time )/1000.0;
}

// sim_t::find_player =======================================================

player_t* sim_t::find_player( const std::string& name )
{
  for( player_t* p = player_list; p; p = p -> next )
  {
    if( name == p -> name() ) return p;
  }
  return 0;
}

// sim_t::parse_option ======================================================

bool sim_t::parse_option( const std::string& name,
                          const std::string& value )
{
  option_t options[] =
  {
    // Main
    { "iterations",                       OPT_INT,    &( iterations                               ) },
    { "max_time",                         OPT_FLT,    &( max_time                                 ) },
    { "threads",                          OPT_INT,    &( threads                                  ) },
    // Party Composition
    { "party",                            OPT_LIST,   &( party_encoding                           ) },
    // Lag
    { "channel_lag",                      OPT_FLT,    &( channel_lag                              ) },
    { "channel_lag_range",                OPT_FLT,    &( channel_lag_range                        ) },
    { "gcd_lag",                          OPT_FLT,    &( gcd_lag                                  ) },
    { "gcd_lag_range",                    OPT_FLT,    &( gcd_lag_range                            ) },
    { "queue_lag",                        OPT_FLT,    &( queue_lag                                ) },
    { "queue_lag_range",                  OPT_FLT,    &( queue_lag_range                          ) },
    { "reaction_time",                    OPT_FLT,    &( reaction_time                            ) },
    { "travel_variance",                  OPT_FLT,    &( travel_variance                          ) },
    // Gear Defaults
    { "default_strength",                 OPT_FLT,    &( gear_default.attribute[ ATTR_STRENGTH  ] ) },
    { "default_agility",                  OPT_FLT,    &( gear_default.attribute[ ATTR_AGILITY   ] ) },
    { "default_stamina",                  OPT_FLT,    &( gear_default.attribute[ ATTR_STAMINA   ] ) },
    { "default_intellect",                OPT_FLT,    &( gear_default.attribute[ ATTR_INTELLECT ] ) },
    { "default_spirit",                   OPT_FLT,    &( gear_default.attribute[ ATTR_SPIRIT    ] ) },
    { "default_spell_power",              OPT_FLT,    &( gear_default.spell_power                 ) },
    { "default_attack_power",             OPT_FLT,    &( gear_default.attack_power                ) },
    { "default_expertise_rating",         OPT_FLT,    &( gear_default.expertise_rating            ) },
    { "default_armor_penetration_rating", OPT_FLT,    &( gear_default.armor_penetration_rating    ) },
    { "default_hit_rating",               OPT_FLT,    &( gear_default.hit_rating                  ) },
    { "default_crit_rating",              OPT_FLT,    &( gear_default.crit_rating                 ) },
    { "default_haste_rating",             OPT_FLT,    &( gear_default.haste_rating                ) },
    { "default_health",                   OPT_FLT,    &( gear_default.resource[ RESOURCE_HEALTH ] ) },
    { "default_mana",                     OPT_FLT,    &( gear_default.resource[ RESOURCE_MANA   ] ) },
    { "default_rage",                     OPT_FLT,    &( gear_default.resource[ RESOURCE_RAGE   ] ) },
    { "default_energy",                   OPT_FLT,    &( gear_default.resource[ RESOURCE_ENERGY ] ) },
    { "default_focus",                    OPT_FLT,    &( gear_default.resource[ RESOURCE_FOCUS  ] ) },
    { "default_runic",                    OPT_FLT,    &( gear_default.resource[ RESOURCE_RUNIC  ] ) },
    { "default_armor",                    OPT_FLT,    &( gear_default.armor                       ) },
    // Gear Deltas for Scale Factors
    { "delta_strength",                   OPT_FLT,    &( gear_delta.attribute[ ATTR_STRENGTH  ]   ) },
    { "delta_agility",                    OPT_FLT,    &( gear_delta.attribute[ ATTR_AGILITY   ]   ) },
    { "delta_stamina",                    OPT_FLT,    &( gear_delta.attribute[ ATTR_STAMINA   ]   ) },
    { "delta_intellect",                  OPT_FLT,    &( gear_delta.attribute[ ATTR_INTELLECT ]   ) },
    { "delta_spirit",                     OPT_FLT,    &( gear_delta.attribute[ ATTR_SPIRIT    ]   ) },
    { "delta_spell_power",                OPT_FLT,    &( gear_delta.spell_power                   ) },
    { "delta_attack_power",               OPT_FLT,    &( gear_delta.attack_power                  ) },
    { "delta_expertise_rating",           OPT_FLT,    &( gear_delta.expertise_rating              ) },
    { "delta_armor_penetration_rating",   OPT_FLT,    &( gear_delta.armor_penetration_rating      ) },
    { "delta_hit_rating",                 OPT_FLT,    &( gear_delta.hit_rating                    ) },
    { "delta_crit_rating",                OPT_FLT,    &( gear_delta.crit_rating                   ) },
    { "delta_haste_rating",               OPT_FLT,    &( gear_delta.haste_rating                  ) },
    { "delta_health",                     OPT_FLT,    &( gear_delta.resource[ RESOURCE_HEALTH ]   ) },
    { "delta_mana",                       OPT_FLT,    &( gear_delta.resource[ RESOURCE_MANA   ]   ) },
    { "delta_rage",                       OPT_FLT,    &( gear_delta.resource[ RESOURCE_RAGE   ]   ) },
    { "delta_energy",                     OPT_FLT,    &( gear_delta.resource[ RESOURCE_ENERGY ]   ) },
    { "delta_focus",                      OPT_FLT,    &( gear_delta.resource[ RESOURCE_FOCUS  ]   ) },
    { "delta_runic",                      OPT_FLT,    &( gear_delta.resource[ RESOURCE_RUNIC  ]   ) },
    { "delta_armor",                      OPT_FLT,    &( gear_delta.armor                         ) },
    // Regen
    { "infinite_energy",                  OPT_INT,    &( infinite_resource[ RESOURCE_ENERGY ]     ) },
    { "infinite_focus",                   OPT_INT,    &( infinite_resource[ RESOURCE_FOCUS  ]     ) },
    { "infinite_health",                  OPT_INT,    &( infinite_resource[ RESOURCE_HEALTH ]     ) },
    { "infinite_mana",                    OPT_INT,    &( infinite_resource[ RESOURCE_MANA   ]     ) },
    { "infinite_rage",                    OPT_INT,    &( infinite_resource[ RESOURCE_RAGE   ]     ) },
    { "infinite_runic",                   OPT_INT,    &( infinite_resource[ RESOURCE_RUNIC  ]     ) },
    { "regen_periodicity",                OPT_FLT,    &( regen_periodicity                        ) },
    // Output 
    { "debug",                            OPT_INT,    &( debug                                    ) },
    { "html",                             OPT_STRING, &( html_file_str                            ) },
    { "log",                              OPT_INT,    &( log                                      ) },
    { "wiki",                             OPT_STRING, &( wiki_file_str                            ) },
    // Overrides - Buffs/Debuffs
    { "abominations_might",               OPT_INT,    &( overrides.abominations_might             ) },
    { "affliction_effects",               OPT_INT,    &( overrides.affliction_effects             ) },
    { "arcane_brilliance",                OPT_INT,    &( overrides.arcane_brilliance              ) },
    { "battle_shout",                     OPT_INT,    &( overrides.battle_shout                   ) },
    { "bleeding",                         OPT_INT,    &( overrides.bleeding                       ) },
    { "blessing_of_kings",                OPT_INT,    &( overrides.blessing_of_kings              ) },
    { "blessing_of_might",                OPT_INT,    &( overrides.blessing_of_might              ) },
    { "blessing_of_wisdom",               OPT_INT,    &( overrides.blessing_of_wisdom             ) },
    { "blood_frenzy",                     OPT_INT,    &( overrides.blood_frenzy                   ) },
    { "bloodlust",                        OPT_INT,    &( overrides.bloodlust                      ) },
    { "bloodlust_early",                  OPT_INT,    &( overrides.bloodlust_early                ) },
    { "crypt_fever",                      OPT_INT,    &( overrides.crypt_fever                    ) },
    { "curse_of_elements",                OPT_INT,    &( overrides.curse_of_elements              ) },
    { "divine_spirit",                    OPT_INT,    &( overrides.divine_spirit                  ) },
    { "earth_and_moon",                   OPT_INT,    &( overrides.earth_and_moon                 ) },
    { "faerie_fire",                      OPT_INT,    &( overrides.faerie_fire                    ) },
    { "ferocious_inspiration",            OPT_INT,    &( overrides.ferocious_inspiration          ) },
    { "fortitude",                        OPT_INT,    &( overrides.fortitude                      ) },
    { "hunters_mark",                     OPT_INT,    &( overrides.hunters_mark                   ) },
    { "improved_divine_spirit",           OPT_INT,    &( overrides.improved_divine_spirit         ) },
    { "improved_moonkin_aura",            OPT_INT,    &( overrides.improved_moonkin_aura          ) },
    { "improved_scorch",                  OPT_INT,    &( overrides.improved_scorch                ) },
    { "improved_shadow_bolt",             OPT_INT,    &( overrides.improved_shadow_bolt           ) },
    { "judgement_of_wisdom",              OPT_INT,    &( overrides.judgement_of_wisdom            ) },
    { "leader_of_the_pack",               OPT_INT,    &( overrides.leader_of_the_pack             ) },
    { "mana_spring",                      OPT_INT,    &( overrides.mana_spring                    ) },
    { "mangle",                           OPT_INT,    &( overrides.mangle                         ) },
    { "mark_of_the_wild",                 OPT_INT,    &( overrides.mark_of_the_wild               ) },
    { "master_poisoner",                  OPT_INT,    &( overrides.master_poisoner                ) },
    { "misery",                           OPT_INT,    &( overrides.misery                         ) },
    { "moonkin_aura",                     OPT_INT,    &( overrides.moonkin_aura                   ) },
    { "poisoned",                         OPT_INT,    &( overrides.poisoned                       ) },
    { "razorice",                         OPT_INT,    &( overrides.razorice                       ) },
    { "replenishment",                    OPT_INT,    &( overrides.replenishment                  ) },
    { "sanctified_retribution",           OPT_INT,    &( overrides.sanctified_retribution         ) },
    { "savage_combat",                    OPT_INT,    &( overrides.savage_combat                  ) },
    { "snare",                            OPT_INT,    &( overrides.snare                          ) },
    { "strength_of_earth",                OPT_INT,    &( overrides.strength_of_earth              ) },
    { "sunder_armor",                     OPT_INT,    &( overrides.sunder_armor                   ) },
    { "swift_retribution",                OPT_INT,    &( overrides.swift_retribution              ) },
    { "thunder_clap",                     OPT_INT,    &( overrides.thunder_clap                   ) },
    { "totem_of_wrath",                   OPT_INT,    &( overrides.totem_of_wrath                 ) },
    { "totem_of_wrath",                   OPT_INT,    &( overrides.totem_of_wrath                 ) },
    { "trauma",                           OPT_INT,    &( overrides.trauma                         ) },
    { "trueshot_aura",                    OPT_INT,    &( overrides.trueshot_aura                  ) },
    { "unleashed_rage",                   OPT_INT,    &( overrides.unleashed_rage                 ) },
    { "windfury_totem",                   OPT_INT,    &( overrides.windfury_totem                 ) },
    { "winters_chill",                    OPT_INT,    &( overrides.winters_chill                  ) },
    { "wrath_of_air",                     OPT_INT,    &( overrides.wrath_of_air                   ) },
    // Unique Mechanics
    { "jow_chance",                       OPT_FLT,    &( jow_chance                               ) },
    { "jow_ppm",                          OPT_FLT,    &( jow_ppm                                  ) },
    { "merge_ignite",                     OPT_INT,    &( merge_ignite                             ) },
    { "replenishment_targets",            OPT_INT,    &( replenishment_targets                    ) },
    { "potion_sickness",                  OPT_INT,    &( potion_sickness                          ) },
    // Misc
    { "armor_update_internval",           OPT_INT,    &( armor_update_interval                    ) },
    { "average_dmg",                      OPT_INT,    &( average_dmg                              ) },
    { "seed",                             OPT_INT,    &( seed                                     ) },
    { "sfmt",                             OPT_INT,    &( sfmt                                     ) },
    { "wheel_granularity",                OPT_FLT,    &( wheel_granularity                        ) },
    { "wheel_seconds",                    OPT_INT,    &( wheel_seconds                            ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( this, options );
    return false;
  }

  if( target  -> parse_option( name, value ) ) return true;
  if( scaling -> parse_option( name, value ) ) return true;

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

int main( int argc, char** argv )
{
  sim_t sim;

  if( ! option_t::parse( &sim, argc, argv ) )
  {
    fprintf( sim.output_file, "ERROR! Incorrect option format..\n" );
    exit( 0 );
  }

  if( sim.seed == 0 ) sim.seed = (int32_t) time( NULL );
  srand( sim.seed );

  int arch, version, revision;
  sim.patch.decode( &arch, &version, &revision );

  fprintf( sim.output_file, 
           "\nSimulationCraft for World of Warcraft build %d.%d.%d (iterations=%d, max_time=%.0f, optimal_raid=%d)\n",
           arch, version, revision, sim.iterations, sim.max_time, sim.optimal_raid );
  fflush( sim.output_file );

  fprintf( stdout, "\nGenerating baseline... \n" ); fflush( stdout );

  sim.execute();

  sim.scaling -> analyze();

  fprintf( stdout, "\nGenerating reports...\n" );

  report_t::print_suite( &sim );

  if( sim.output_file != stdout ) fclose( sim.output_file );
  if( sim.log_file ) fclose( sim.log_file );
  
  return 0;
}

// ==========================================================================
// Utility to make sure memory allocation not happening during iteration.
// ==========================================================================

#if 0
void* operator new (size_t size)
{
  if( iterating ) assert(0);
  return malloc(size); 
}

void operator delete (void *p)
{
 free(p); 
}
#endif

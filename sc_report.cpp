// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Report
// ==========================================================================

// report_t::report_t =======================================================

report_t::report_t( sim_t* s ) :
  sim( s ),
  report_actions(1),
  report_attack_stats(1),
  report_core_stats(1),
  report_dpm(1),
  report_dps(1),
  report_gains(1),
  report_miss(1),
  report_mps(1),
  report_name(1),
  report_pet(0),
  report_performance(1),
  report_procs(1),
  report_raid_dps(1),
  report_spell_stats(1),
  report_tag(1),
  report_uptime(1)
{
}

// report_t::parse_option ===================================================

bool report_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    { "report_actions",          OPT_INT8,   &( report_actions          ) },
    { "report_attack_stats",     OPT_INT8,   &( report_attack_stats     ) },
    { "report_core_stats",       OPT_INT8,   &( report_core_stats       ) },
    { "report_dpm",              OPT_INT8,   &( report_dpm              ) },
    { "report_dps",              OPT_INT8,   &( report_dps              ) },
    { "report_gains",            OPT_INT8,   &( report_gains            ) },
    { "report_miss",             OPT_INT8,   &( report_miss             ) },
    { "report_mps",              OPT_INT8,   &( report_mps              ) },
    { "report_name",             OPT_INT8,   &( report_name             ) },
    { "report_pet",              OPT_INT8,   &( report_pet              ) },
    { "report_performance",      OPT_INT8,   &( report_performance      ) },
    { "report_procs",            OPT_INT8,   &( report_procs            ) },
    { "report_raid_dps",         OPT_INT8,   &( report_raid_dps         ) },
    { "report_spell_stats",      OPT_INT8,   &( report_spell_stats      ) },
    { "report_tag",              OPT_INT8,   &( report_tag              ) },
    { "report_uptime",           OPT_INT8,   &( report_uptime           ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( options );
    return false;
  }

  return option_t::parse( options, name, value );
}

// report_t::print_action ====================================================

void report_t::print_action( stats_t* s )
{
  if( s -> num_executes == 0 ) return;

  static std::string action_name;
  double total_dmg;

  if( s -> player -> type == PLAYER_PET )
  {
    action_name = s -> player -> name_str + "-" + s -> name;
    total_dmg   = s -> player -> cast_pet() -> owner ->  total_dmg;
  }
  else 
  {
    action_name = s -> name;
    total_dmg   = s -> player -> total_dmg;
  }

  printf( "    %-20s  Count=%5.1f|%4.1fsec  DPS=%6.1f  DPE=%4.0f|%2.0f%%  DPET=%4.0f", 
	  action_name.c_str(),
	  s -> num_executes,
	  s -> sim -> total_seconds / s -> num_executes,
	  s -> dps, s -> dpe, s -> total_dmg * 100.0 / total_dmg, s -> dpet );

  if( report_miss ) printf( "  Miss=%.1f%%", s -> execute_results[ RESULT_MISS ].count * 100.0 / s -> num_executes );
      
  if( s -> execute_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    printf( "  Hit=%4.0f", s -> execute_results[ RESULT_HIT ].avg_dmg );
  }
  if( s -> execute_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    printf( "  CritHit=%4.0f|%4.0f|%.1f%%", 
	    s -> execute_results[ RESULT_CRIT ].avg_dmg, 
	    s -> execute_results[ RESULT_CRIT ].max_dmg, 
	    s -> execute_results[ RESULT_CRIT ].count * 100.0 / s -> num_executes );
  }

  if( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    printf( "  Tick=%.0f", s -> tick_results[ RESULT_HIT ].avg_dmg );
  }
  if( s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    printf( "  CritTick=%.0f|%.0f|%.1f%%", 
	    s -> tick_results[ RESULT_CRIT ].avg_dmg, 
	    s -> tick_results[ RESULT_CRIT ].max_dmg, 
	    s -> tick_results[ RESULT_CRIT ].count * 100.0 / s -> num_ticks );
  }

  printf( "\n" );
}

// report_t::print_actions ===================================================

void report_t::print_actions( player_t* p )
{
  printf( "  Actions:\n" );

  for( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    if( s -> total_dmg > 0 )
    {
      print_action( s );
    }
  }

  for( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    for( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if( s -> total_dmg > 0 )
      {
	print_action( s );
      }
    }
  }
}

// report_t::print_gains ====================================================

void report_t::print_gains( player_t* p )
{
  if( p -> gain_list.size() == 0 ) return;

  printf( "    Gains: " );
  
  for( gain_list_t::iterator i = p -> gain_list.begin(); i != p -> gain_list.end(); ++i )
  {
    printf( "  %s=%.1f", 
	    i -> first.c_str(), 
	    i -> second / sim -> iterations );
  }

  printf( "\n" );
}

// report_t::print_procs ====================================================

void report_t::print_procs( player_t* p )
{
  if( p -> proc_list.size() == 0 ) return;

  printf( "    Procs: " );
  
  for( proc_list_t::iterator i = p -> proc_list.begin(); i != p -> proc_list.end(); ++i )
  {
    printf( "  %s=%d|%.1fsec", 
	    i -> first.c_str(), 
	    i -> second / sim -> iterations,
	    sim -> iterations * sim -> total_seconds / (double) i -> second );
  }

  printf( "\n" );
}

// report_t::print_core_stats =================================================

void report_t::print_core_stats( player_t* p )
{
  printf( "%s  %s%.0f  %s%.0f  %s%.0f  %s%.0f  %s%.0f  %s%.0f  %s%.0f\n", 
	  report_tag ? "  Core Stats:" : "", 
	  report_tag ? "strength=" : "",
	  p -> strength(),
	  report_tag ? "agility=" : "",
	  p -> agility(),
	  report_tag ? "stamina=" : "",
	  p -> stamina(),
	  report_tag ? "intellect=" : "",
	  p -> intellect(),
	  report_tag ? "spirit=" : "",
	  p -> spirit(),
	  report_tag ? "health=" : "",
	  p -> resource_max[ RESOURCE_HEALTH ], 
	  report_tag ? "mana=" : "",
	  p -> resource_max[ RESOURCE_MANA ] );
}

// report_t::print_spell_stats ================================================

void report_t::print_spell_stats( player_t* p )
{
  printf( "%s  %s%.0f  %s%.1f%%  %s%.1f%%  %s%.0f  %s%.1f%%  %s%.0f\n", 
	  report_tag ? "  Spell Stats:" : "", 
	  report_tag ? "power="       : "", p -> composite_spell_power( SCHOOL_MAX ),
	  report_tag ? "hit="         : "", p -> composite_spell_hit()  * 100.0, 
	  report_tag ? "crit="        : "", p -> composite_spell_crit() * 100.0,
	  report_tag ? "penetration=" : "", p -> composite_spell_penetration(),
	  report_tag ? "haste="       : "", ( 1.0 - p -> haste ) * 100.0,
	  report_tag ? "mp5="         : "", p -> initial_mp5 );
}

// report_t::print_attack_stats ===============================================

void report_t::print_attack_stats( player_t* p )
{
  printf( "%s  %s%.0f  %s%.1f%%  %s%.1f%%  %s%.1f  %s%.0f  %s%.1f%%\n", 
	  report_tag ? "  Attack Stats:" : "",
	  report_tag ? "power="       : "", p -> composite_attack_power(),
	  report_tag ? "hit="         : "", p -> composite_attack_hit()       * 100.0, 
	  report_tag ? "crit="        : "", p -> composite_attack_crit()      * 100.0, 
	  report_tag ? "expertise="   : "", p -> composite_attack_expertise() * 100.0,
	  report_tag ? "penetration=" : "", p -> composite_attack_penetration(),
	  report_tag ? "haste="       : "", ( 1.0 - p -> haste ) * 100.0 );
}

// report_t::print_uptime =====================================================

void report_t::print_uptime()
{
  printf( "Up-Times:\n" );

  for( uptime_t* u = sim -> uptime_list; u; u = u -> next )
  {
    if( u -> up > 0 ) 
    {
      printf( "  %s=%.1f%%\n", u -> name(), u -> percentage() );
    }
  }
}

// report_t::print_performance ================================================

void report_t::print_performance()
{
  printf( "Performance:\n"
	  "  TotalEvents   = %d\n"
	  "  MaxEventQueue = %d\n"
	  "  SimSeconds    = %.0f\n"
	  "  CpuSeconds    = %.0f\n"
	  "  SpeedUp       = %.0f\n", 
	  sim -> total_events_processed,
	  sim -> max_events_remaining,
	  sim -> iterations * sim -> total_seconds,
	  sim -> elapsed_cpu_seconds,
	  sim -> iterations * sim -> total_seconds / sim -> elapsed_cpu_seconds );
}

// report_t::print ============================================================

void report_t::print()
{
  if( sim -> total_seconds == 0 ) return;
  sim -> total_seconds /= sim -> iterations;

  double raid_dps = 0;

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;

    if( p -> type == PLAYER_PET && ! report_pet )
      continue;

    p -> total_dmg = 0;

    for( stats_t* s = p -> stats_list; s; s = s -> next )
    {
      s -> analyze();
      p -> total_dmg += s -> total_dmg;
    }

    for( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
    {
      for( stats_t* s = pet -> stats_list; s; s = s -> next )
      {
	s -> analyze();
	p -> total_dmg += s -> total_dmg;
      }
    }

    double dps = p -> total_dmg / sim -> total_seconds;

    // Avoid double-counting of pet damage.
    if( p -> type != PLAYER_PET ) raid_dps += dps;

    if( report_name ) printf( "%s%s ",   report_tag ? "Player=" : "", p -> name() );
    if( report_dps  ) printf( "%s%.1f ", report_tag ? "DPS="    : "", dps );

    double mana_loss = p -> resource_lost[ RESOURCE_MANA ];

    if( mana_loss > 0 )
    {
      mana_loss /= sim -> iterations;

      if( report_dpm  ) printf( "%s%.1f ", report_tag ? "DPM="    : "", p -> total_dmg / mana_loss );
      if( report_mps  ) printf( "%s%.1f ", report_tag ? "MPS="    : "", mana_loss / sim -> total_seconds );
    }

    if( report_name ) printf( "\n" );

    if( report_core_stats   ) print_core_stats  ( p );
    if( report_spell_stats  ) print_spell_stats ( p );
    if( report_attack_stats ) print_attack_stats( p );

    if( report_actions ) print_actions( p );
    if( report_gains   ) print_gains  ( p );
    if( report_procs   ) print_procs  ( p );

  }
  if( report_raid_dps ) printf( "%s%.1f\n", report_tag ? "RDPS=" : "", raid_dps );

  if( report_uptime ) print_uptime();

  if( report_performance ) print_performance();

  printf( "\n" );
}

// report_t::timestamp ======================================================

void report_t::timestamp( sim_t* sim )
{
  if( sim -> timestamp ) 
  {
    printf( "%-8.2f ", sim -> current_time );
  }
}

// report_t::va_printf ======================================================

void report_t::va_printf( sim_t*      sim, 
			  const char* format,
			  va_list     vap )
{
  timestamp( sim );
  vprintf( format, vap );
  printf( "\n" );
  fflush( stdout );
}


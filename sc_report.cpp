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
  report_uptime(1),
  report_waiting(1)
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
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}

// report_t::print_action ====================================================

void report_t::print_action( stats_t* s )
{
  if( s -> total_dmg == 0 ) return;

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

  fprintf( sim -> output_file, 
	   "    %-20s  Count=%5.1f|%4.1fsec  DPS=%6.1f  DPE=%4.0f|%2.0f%%  DPET=%4.0f", 
	   action_name.c_str(),
	   s -> num_executes,
	   s -> player -> total_seconds / s -> num_executes,
	   s -> dps, s -> dpe, s -> total_dmg * 100.0 / total_dmg, s -> dpet );

  if( report_miss ) fprintf( sim -> output_file, "  Miss=%.1f%%", s -> execute_results[ RESULT_MISS ].count * 100.0 / s -> num_executes );
      
  if( s -> execute_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    fprintf( sim -> output_file, "  Hit=%4.0f", s -> execute_results[ RESULT_HIT ].avg_dmg );
  }
  if( s -> execute_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    fprintf( sim -> output_file, 
	     "  CritHit=%4.0f|%4.0f|%.1f%%", 
	     s -> execute_results[ RESULT_CRIT ].avg_dmg, 
	     s -> execute_results[ RESULT_CRIT ].max_dmg, 
	     s -> execute_results[ RESULT_CRIT ].count * 100.0 / s -> num_executes );
  }

  if( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    fprintf( sim -> output_file, 
	     "  Tick=%.0f", s -> tick_results[ RESULT_HIT ].avg_dmg );
  }
  if( s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    fprintf( sim -> output_file, 
	     "  CritTick=%.0f|%.0f|%.1f%%", 
	     s -> tick_results[ RESULT_CRIT ].avg_dmg, 
	     s -> tick_results[ RESULT_CRIT ].max_dmg, 
	     s -> tick_results[ RESULT_CRIT ].count * 100.0 / s -> num_ticks );
  }

  fprintf( sim -> output_file, "\n" );
}

// report_t::print_actions ===================================================

void report_t::print_actions( player_t* p )
{
  fprintf( sim -> output_file, "  Actions:\n" );

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

// report_t::print_core_stats =================================================

void report_t::print_core_stats( player_t* p )
{
  fprintf( sim -> output_file, 
	   "%s  %s%.0f  %s%.0f  %s%.0f  %s%.0f  %s%.0f  %s%.0f  %s%.0f\n", 
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
  fprintf( sim -> output_file, 
	   "%s  %s%.0f  %s%.1f%%  %s%.1f%%  %s%.0f  %s%.1f%%  %s%.0f\n", 
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
  fprintf( sim -> output_file, 
	   "%s  %s%.0f  %s%.1f%%  %s%.1f%%  %s%.1f  %s%.0f  %s%.1f%%\n", 
	   report_tag ? "  Attack Stats:" : "",
	   report_tag ? "power="       : "", p -> composite_attack_power(),
	   report_tag ? "hit="         : "", p -> composite_attack_hit()       * 100.0, 
	   report_tag ? "crit="        : "", p -> composite_attack_crit()      * 100.0, 
	   report_tag ? "expertise="   : "", p -> composite_attack_expertise() * 100.0,
	   report_tag ? "penetration=" : "", p -> composite_attack_penetration(),
	   report_tag ? "haste="       : "", ( 1.0 - p -> haste ) * 100.0 );
}

// report_t::print_gains =====================================================

void report_t::print_gains()
{
  fprintf( sim -> output_file, "\nGains:\n" );

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;

    bool first=true;
    for( gain_t* g = p -> gain_list; g; g = g -> next )
    {
      if( g -> amount > 0 ) 
      {
	if( first )
        {
	  fprintf( sim -> output_file, "\n    %s:\n", p -> name() );
	  first = false;
	}
	fprintf( sim -> output_file, "        %s=%.1f\n", g -> name(), g -> amount / sim -> iterations );
      }
    }
  }
}

// report_t::print_procs =====================================================

void report_t::print_procs()
{
  fprintf( sim -> output_file, "\nProcs:\n" );

  for( player_t* player = sim -> player_list; player; player = player -> next )
  {
    if( player -> quiet ) 
      continue;

    bool first=true;
    for( proc_t* p = player -> proc_list; p; p = p -> next )
    {
      if( p -> count > 0 ) 
      {
	if( first )
        {
	  fprintf( sim -> output_file, "\n    %s:\n", player -> name() );
	  first = false;
	}
	fprintf( sim -> output_file, "        %s=%d|%.1fsec\n", 
		 p -> name(),
		 p -> count / sim -> iterations,
		 sim -> iterations * player -> total_seconds / p -> count );
      }
    }
  }
}

// report_t::print_uptime =====================================================

void report_t::print_uptime()
{
  fprintf( sim -> output_file, "\nUp-Times:\n" );

  bool first=true;
  for( uptime_t* u = sim -> uptime_list; u; u = u -> next )
  {
    if( u -> up > 0 ) 
    {
      if( first )
      {
	fprintf( sim -> output_file, "\n    Global:\n" );
	first = false;
      }
      fprintf( sim -> output_file, "        %4.1f%% : %s\n", u -> percentage(), u -> name() );
    }
  }

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;

    first=true;
    for( uptime_t* u = p -> uptime_list; u; u = u -> next )
    {
      if( u -> up > 0 ) 
      {
	if( first )
        {
	  fprintf( sim -> output_file, "\n    %s:\n", p -> name() );
	  first = false;
	}
	fprintf( sim -> output_file, "        %4.1f%% : %s\n", u -> percentage(), u -> name() );
      }
    }
  }
}

// report_t::print_waiting =====================================================

void report_t::print_waiting()
{
  fprintf( sim -> output_file, "\nWaiting:\n" );

  bool nobody_waits = true;

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) 
      continue;
    
    if( p -> total_waiting )
    {
      nobody_waits = false;
      p -> total_waiting /= sim -> iterations;
      fprintf( sim -> output_file, "    %4.1f%% : %s\n", 100.0 * p -> total_waiting / p -> total_seconds,  p -> name() );
    }
  }

  if( nobody_waits ) fprintf( sim -> output_file, "    All players active 100%% of the time.\n" );
}

// report_t::print_performance ================================================

void report_t::print_performance()
{
  fprintf( sim -> output_file, 
	   "\nPerformance:\n"
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

    p -> total_seconds /= sim -> iterations;
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

    double dps = p -> total_dmg / p -> total_seconds;

    // Avoid double-counting of pet damage.
    if( p -> type != PLAYER_PET ) raid_dps += dps;

    if( report_tag  ) fprintf( sim -> output_file, "\n" );
    if( report_name ) fprintf( sim -> output_file, "%s%s ",   report_tag ? "Player=" : "", p -> name() );
    if( report_dps  ) fprintf( sim -> output_file, "%s%.1f ", report_tag ? "DPS="    : "", dps );

    double mana_loss = p -> resource_lost  [ RESOURCE_MANA ];
    double mana_gain = p -> resource_gained[ RESOURCE_MANA ];

    if( mana_loss > 0 )
    {
      mana_loss /= sim -> iterations;
      mana_gain /= sim -> iterations;

      if( report_dpm  ) fprintf( sim -> output_file, "%s%.1f ",      report_tag ? "DPM="    : "", p -> total_dmg / mana_loss );
      if( report_mps  ) fprintf( sim -> output_file, "%s%.1f/%.1f ", report_tag ? "MPS="    : "", 
				 mana_loss / p -> total_seconds, mana_gain / p -> total_seconds );
    }

    if( report_name ) fprintf( sim -> output_file, "\n" );

    if( report_core_stats   ) print_core_stats  ( p );
    if( report_spell_stats  ) print_spell_stats ( p );
    if( report_attack_stats ) print_attack_stats( p );

    if( report_actions ) print_actions( p );

  }
  if( report_raid_dps ) fprintf( sim -> output_file, "%s%.1f\n", report_tag ? "\nRDPS=" : "", raid_dps );

  if( report_gains   ) print_gains();
  if( report_procs   ) print_procs();
  if( report_uptime  ) print_uptime();
  if( report_waiting ) print_waiting();

  if( report_performance ) print_performance();

  fprintf( sim -> output_file, "\n" );
}

// report_t::timestamp ======================================================

void report_t::timestamp( sim_t* sim )
{
  if( sim -> timestamp ) 
  {
    fprintf( sim -> output_file, "%-8.2f ", sim -> current_time );
  }
}

// report_t::va_printf ======================================================

void report_t::va_printf( sim_t*      sim, 
			  const char* format,
			  va_list     vap )
{
  timestamp( sim );
  vfprintf( sim -> output_file, format, vap );
  fprintf( sim -> output_file, "\n" );
  fflush( sim -> output_file );
}


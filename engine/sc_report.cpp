// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// player_type_string ========================================================

static const char* player_type_string( player_t* p )
{
  switch( p -> type )
  {
  case DEATH_KNIGHT:    return "DeathKnight";
  case DRUID:           return "Druid";
  case HUNTER:          return "Hunter";
  case MAGE:            return "Mage";
  case PALADIN:         return "Paladin";
  case PRIEST:          return "Priest";
  case ROGUE:           return "Rogue";
  case SHAMAN:          return "Shaman";
  case WARLOCK:         return "Warlock";
  case WARRIOR:         return "Warrior";
  }
  assert(0);
  return 0;
}

// simplify_html =============================================================

static void simplify_html( std::string& buffer )
{
  for ( std::string::size_type pos = buffer.find( "&lt;", 0 ); pos != std::string::npos; pos = buffer.find( "&lt;", pos ) )
  {
    buffer.replace( pos, 4, "<" );
  }
  for ( std::string::size_type pos = buffer.find( "&gt;", 0 ); pos != std::string::npos; pos = buffer.find( "&gt;", pos ) )
  {
    buffer.replace( pos, 4, ">" );
  }
  for ( std::string::size_type pos = buffer.find( "&amp;", 0 ); pos != std::string::npos; pos = buffer.find( "&amp;", pos ) )
  {
    buffer.replace( pos, 5, "&" );
  }
}

// wiki_player_reference =====================================================

static std::string wiki_player_reference( player_t* p )
{
  std::string buffer = p -> name();

  for ( std::string::size_type pos = buffer.find( "_", 0 ); pos != std::string::npos; pos = buffer.find( "_", pos ) )
  {
    buffer.replace( pos, 1, "" );
  }

  return buffer;
}

// wiki_player_anchor ========================================================

static std::string wiki_player_anchor( player_t* p )
{
  std::string buffer = wiki_player_reference( p );

  // GoogleCode Wiki is giving me headaches......
  int size = buffer.size();
  if ( isdigit( buffer[ size-1 ] ) )
  {
    buffer.insert( 0, "!" );
  }

  return buffer;
}

// print_action ==============================================================

static void print_action( FILE* file, stats_t* s, int max_name_length=0 )
{
  if ( s -> total_dmg == 0 ) return;

  if( max_name_length == 0 ) max_name_length = 20;

  util_t::fprintf( file,
                   "    %-*s  Count=%5.1f|%5.2fsec  DPE=%6.0f|%2.0f%%  DPET=%6.0f  DPR=%6.1f  pDPS=%4.0f",
		   max_name_length,
                   s -> name_str.c_str(),
                   s -> num_executes,
                   s -> frequency,
                   s -> dpe,
                   s -> portion_dmg * 100.0,
                   s -> dpet,
                   s -> dpr,
                   s -> portion_dps );

  double miss_pct = s -> num_executes > 0 ? s -> execute_results[ RESULT_MISS ].count / s -> num_executes : 0.0;

  util_t::fprintf( file, "  Miss=%.2f%%", 100.0 * miss_pct );

  if ( s -> execute_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file, "  Hit=%4.0f|%4.0f|%4.0f", s -> execute_results[ RESULT_HIT ].avg_dmg, s -> execute_results[ RESULT_HIT ].min_dmg, s -> execute_results[ RESULT_HIT ].max_dmg );
  }
  if ( s -> execute_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Crit=%5.0f|%5.0f|%5.0f|%.1f%%",
                     s -> execute_results[ RESULT_CRIT ].avg_dmg,
                     s -> execute_results[ RESULT_CRIT ].min_dmg,
                     s -> execute_results[ RESULT_CRIT ].max_dmg,
                     s -> execute_results[ RESULT_CRIT ].count * 100.0 / s -> num_executes );
  }
  if ( s -> execute_results[ RESULT_GLANCE ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Glance=%4.0f|%.1f%%",
                     s -> execute_results[ RESULT_GLANCE ].avg_dmg,
                     s -> execute_results[ RESULT_GLANCE ].count * 100.0 / s -> num_executes );
  }
  if ( s -> execute_results[ RESULT_DODGE ].count > 0 )
  {
    util_t::fprintf( file,
                     "  Dodge=%.1f%%",
                     s -> execute_results[ RESULT_DODGE ].count * 100.0 / s -> num_executes );
  }
  if ( s -> execute_results[ RESULT_PARRY ].count > 0 )
  {
    util_t::fprintf( file,
                     "  Parry=%.1f%%",
                     s -> execute_results[ RESULT_PARRY ].count * 100.0 / s -> num_executes );
  }

  if ( s -> num_ticks > 0 ) util_t::fprintf( file, "  TickCount=%.0f", s -> num_ticks );

  double tick_miss_pct = s -> num_ticks > 0 ? s -> tick_results[ RESULT_MISS ].count / s -> num_ticks : 0.0;

  if ( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 || s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file, "  MissTick=%.1f%%", 100.0 * tick_miss_pct );
  }

  if ( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Tick=%.0f|%.0f|%.0f", s -> tick_results[ RESULT_HIT ].avg_dmg, s -> tick_results[ RESULT_HIT ].min_dmg, s -> tick_results[ RESULT_HIT ].max_dmg );
  }
  if ( s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  CritTick=%.0f|%.0f|%.0f|%.1f%%",
                     s -> tick_results[ RESULT_CRIT ].avg_dmg,
                     s -> tick_results[ RESULT_CRIT ].min_dmg,
                     s -> tick_results[ RESULT_CRIT ].max_dmg,
                     s -> tick_results[ RESULT_CRIT ].count * 100.0 / s -> num_ticks );
  }

  util_t::fprintf( file, "\n" );
}

// print_actions =============================================================

static void print_actions( FILE* file, player_t* p )
{
  util_t::fprintf( file, "  Glyphs: %s\n", p -> glyphs_str.c_str() );

  util_t::fprintf( file, "  Priorities:\n" );

  std::vector<std::string> action_list;
  int num_actions = util_t::string_split( action_list, p -> action_list_str, "/" );
  int length = 0;
  for ( int i=0; i < num_actions; i++ )
  {
    if ( length > 80 || ( length > 0 && ( length + action_list[ i ].size() ) > 80 ) )
    {
      util_t::fprintf( file, "\n" );
      length = 0;
    }
    util_t::fprintf( file, "%s%s", ( ( length > 0 ) ? "/" : "    " ), action_list[ i ].c_str() );
    length += ( int ) action_list[ i ].size();
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "  Actions:\n" );

  int max_length=0;
  for ( stats_t* s = p -> stats_list; s; s = s -> next )
    if ( s -> total_dmg > 0 )
      if( max_length < (int) s -> name_str.length() )
	max_length = ( int ) s -> name_str.length();

  for ( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    if ( s -> total_dmg > 0 )
    {
      print_action( file, s, max_length );
    }
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    bool first=true;
    for ( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if ( s -> total_dmg > 0 )
      {
        if ( first )
        {
          util_t::fprintf( file, "   %s  (DPS=%.1f)\n", pet -> name_str.c_str(), pet -> dps );
          first = false;
        }
        print_action( file, s, max_length );
      }
    }
  }
}

// print_buffs ===============================================================

static void print_buffs( FILE* file, player_t* p )
{
  bool first=true;
  char prefix = ' ';
  int total_length = 100;
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count )
      continue;

    if ( b -> constant )
    {
      int length = ( int ) strlen( b -> name() );
      if( ( total_length + length ) > 100 )
      {
	if ( ! first ) util_t::fprintf( file, "\n" ); first=false;
	util_t::fprintf( file, "  Constant Buffs:" );
	prefix = ' ';
	total_length = 0;
      }
      util_t::fprintf( file, "%c%s", prefix, b -> name() );
      prefix = '/';
      total_length += length;
    }
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "  Dynamic Buffs:\n" );

  int max_length = 0;
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( ! b -> quiet && b -> start_count && ! b -> constant )
    {
      int length = ( int ) strlen( b -> name() );
      if ( length > max_length ) max_length = length;
    }
  }

  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count )
      continue;

    if ( ! b -> constant )
    {
      util_t::fprintf( file, "    %-*s : start=%-4.1f  refresh=%-5.1f  interval=%5.1f|%-5.1f  uptime=%2.0f%%",
                       max_length, b -> name(), b -> avg_start, b -> avg_refresh, 
		       b -> avg_start_interval, b -> avg_trigger_interval, b -> uptime_pct );

      if( b -> benefit_pct > 0 &&
	  b -> benefit_pct < 100 )
      {
	util_t::fprintf( file, "  benefit=%2.0f%%", b -> benefit_pct );
      }

      if ( b -> trigger_pct > 0 && 
	   b -> trigger_pct < 100 ) 
      {
	util_t::fprintf( file, "  trigger=%2.0f%%", b -> trigger_pct );
      }

      util_t::fprintf( file, "\n" );
    }
  }
}

// print_buffs ===============================================================

static void print_buffs( FILE* file, sim_t* sim )
{
  util_t::fprintf( file, "\nAuras and De-Buffs:" );
  char prefix = ' ';
  int total_length = 100;
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count )
      continue;

    if ( b -> constant )
    {
      int length = ( int ) strlen( b -> name() );
      if( ( total_length + length ) > 100 )
      {
	util_t::fprintf( file, "\n  Constant:" );
	prefix = ' ';
	total_length = 0;
      }
      util_t::fprintf( file, "%c%s", prefix, b -> name() );
      prefix = '/';
      total_length += length;
    }
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "  Dynamic:\n" );

  int max_length = 0;
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( ! b -> quiet && b -> start_count && ! b -> constant )
    {
      int length = ( int ) strlen( b -> name() );
      if ( length > max_length ) max_length = length;
    }
  }

  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count )
      continue;

    if ( ! b -> constant )
    {
      util_t::fprintf( file, "    %-*s : start=%-4.1f  refresh=%-5.1f  interval=%5.1f|%-5.1f  uptime=%2.0f%%",
                       max_length, b -> name(), b -> avg_start, b -> avg_refresh, 
		       b -> avg_start_interval, b -> avg_trigger_interval, b -> uptime_pct );

      if( b -> benefit_pct > 0 &&
	  b -> benefit_pct < 100 )
      {
	util_t::fprintf( file, "  benefit=%2.0f%%", b -> benefit_pct );
      }

      if ( b -> trigger_pct > 0 && 
	   b -> trigger_pct < 100 ) 
      {
	util_t::fprintf( file, "  trigger=%2.0f%%", b -> trigger_pct );
      }

      util_t::fprintf( file, "\n" );
    }
  }
}

// print_core_stats ===========================================================

static void print_core_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Core Stats:  strength=%.0f|%.0f(%.0f)  agility=%.0f|%.0f(%.0f)  stamina=%.0f|%.0f(%.0f)  intellect=%.0f|%.0f(%.0f)  spirit=%.0f|%.0f(%.0f)  health=%.0f|%.0f  mana=%.0f|%.0f\n",
                   p -> attribute_buffed[ ATTR_STRENGTH  ], p -> strength(),  p -> stats.attribute[ ATTR_STRENGTH  ],
                   p -> attribute_buffed[ ATTR_AGILITY   ], p -> agility(),   p -> stats.attribute[ ATTR_AGILITY   ],
                   p -> attribute_buffed[ ATTR_STAMINA   ], p -> stamina(),   p -> stats.attribute[ ATTR_STAMINA   ],
                   p -> attribute_buffed[ ATTR_INTELLECT ], p -> intellect(), p -> stats.attribute[ ATTR_INTELLECT ],
                   p -> attribute_buffed[ ATTR_SPIRIT    ], p -> spirit(),    p -> stats.attribute[ ATTR_SPIRIT    ],
                   p -> resource_buffed[ RESOURCE_HEALTH ], p -> resource_max[ RESOURCE_HEALTH ], 
		   p -> resource_buffed[ RESOURCE_MANA   ], p -> resource_max[ RESOURCE_MANA   ] );
}

// print_spell_stats ==========================================================

static void print_spell_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Spell Stats:  power=%.0f|%.0f(%.0f)  hit=%.2f%%|%.2f%%(%.0f)  crit=%.2f%%|%.2f%%(%.0f)  penetration=%.0f|%.0f(%.0f)  haste=%.2f%%|%.2f%%(%.0f)  mp5=%.0f|%.0f(%.0f)\n",
                   p -> buffed_spell_power, p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier(), p -> stats.spell_power,
                   100 * p -> buffed_spell_hit,          100 * p -> composite_spell_hit(),          p -> stats.hit_rating,
                   100 * p -> buffed_spell_crit,         100 * p -> composite_spell_crit(),         p -> stats.crit_rating,
                   100 * p -> buffed_spell_penetration,  100 * p -> composite_spell_penetration(),  p -> stats.spell_penetration,
                   100 * ( 1 / p -> buffed_spell_haste - 1 ), 100 * ( 1 / p -> spell_haste - 1 ), p -> stats.haste_rating,
                   p -> buffed_mp5, p -> composite_mp5(), p -> stats.mp5 );
}

// print_attack_stats =========================================================

static void print_attack_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Attack Stats  power=%.0f|%.0f(%.0f)  hit=%.2f%%|%.2f%%(%.0f)  crit=%.2f%%|%.2f%%(%.0f)  expertise=%.2f|%.2f(%.0f)  penetration=%.2f%%|%.2f%%(%.0f)  haste=%.2f%%|%.2f%%(%.0f)\n",
                   p -> buffed_attack_power, p -> composite_attack_power() * p -> composite_attack_power_multiplier(), p -> stats.attack_power,
                   100 * p -> buffed_attack_hit,         100 * p -> composite_attack_hit(),         p -> stats.hit_rating,
                   100 * p -> buffed_attack_crit,        100 * p -> composite_attack_crit(),        p -> stats.crit_rating,
                   100 * p -> buffed_attack_expertise,   100 * p -> composite_attack_expertise(),   p -> stats.expertise_rating,
                   100 * p -> buffed_attack_penetration, 100 * p -> composite_attack_penetration(), p -> stats.armor_penetration_rating,
                   100 * ( 1 / p -> buffed_attack_haste - 1 ), 100 * ( 1 / p -> attack_haste - 1 ), p -> stats.haste_rating );
}

// print_defense_stats =======================================================

static void print_defense_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Defense Stats:  armor=%.0f|%.0f(%.0f)  blockv=%.0f|%.0f(%.0f)  defense=%.0f|%.0f(%.0f)  miss=%.2f%%|%.2f%%  dodge=%.2f%%|%.2f%%(%.0f)  parry=%.2f%%|%.2f%%(%.0f)  block=%.2f%%|%.2f%%(%.0f) crit=%.2f%%|%.2f%%\n",
                   p -> buffed_armor,       p -> composite_armor(), ( p -> stats.armor + p -> stats.bonus_armor ),
                   p -> buffed_block_value, p -> composite_block_value(), p -> stats.block_value,
                   p -> buffed_defense,     p -> composite_defense(), p -> stats.defense_rating,
                   100 * p -> buffed_miss,  100 * ( p -> composite_tank_miss( SCHOOL_PHYSICAL ) - p -> diminished_miss( SCHOOL_PHYSICAL ) ),
                   100 * p -> buffed_dodge, 100 * ( p -> composite_tank_dodge() - p -> diminished_dodge() ), p -> stats.dodge_rating,
                   100 * p -> buffed_parry, 100 * ( p -> composite_tank_parry() - p -> diminished_parry() ), p -> stats.parry_rating,
                   100 * p -> buffed_block, 100 * p -> composite_tank_block(), p -> stats.block_rating,
		   100 * p -> buffed_crit,  100 * p -> composite_tank_crit( SCHOOL_PHYSICAL ) );
}

// print_gains ===============================================================

static void print_gains( FILE* file, player_t* p )
{
  int max_length = 0;
  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 )
    {
      int length = ( int ) strlen( g -> name() );
      if ( length > max_length ) max_length = length;
    }
  }
  if( max_length == 0 ) return;

  util_t::fprintf( file, "  Gains:\n" );

  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 )
    {
      util_t::fprintf( file, "    %7.1f : %-*s", g -> actual, max_length, g -> name() );
      double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
      if ( overflow_pct > 1.0 ) util_t::fprintf( file, "  (overflow=%.1f%%)", overflow_pct );
      util_t::fprintf( file, "\n" );
    }
  }
}

// print_pet_gains ===============================================================

static void print_pet_gains( FILE* file, player_t* p )
{
  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    if ( pet -> total_dmg <= 0 ) continue;

    int max_length = 0;
    for ( gain_t* g = pet -> gain_list; g; g = g -> next )
    {
      if ( g -> actual > 0 )
      {
        int length = ( int ) strlen( g -> name() );
        if ( length > max_length ) max_length = length;
      }
    }
    if( max_length > 0 )
    {
      util_t::fprintf( file, "  Pet \"%s\" Gains:\n", pet -> name_str.c_str() );

      for ( gain_t* g = pet -> gain_list; g; g = g -> next )
      {
	if ( g -> actual > 0 )
        {
	  util_t::fprintf( file, "    %7.1f : %-*s", g -> actual, max_length, g -> name() );
	  double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
	  if ( overflow_pct > 1.0 ) util_t::fprintf( file, "  (overflow=%.1f%%)", overflow_pct );
	  util_t::fprintf( file, "\n" );
	}
      }
    }
  }
}

// print_procs ================================================================

static void print_procs( FILE* file, player_t* p )
{
  bool first=true;

  for ( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
  {
    if ( proc -> count > 0 )
    {
      if ( first ) util_t::fprintf( file, "  Procs:\n" ); first = false;
      util_t::fprintf( file, "    %5.1f | %6.2fsec : %s\n",
                       proc -> count, proc -> frequency, proc -> name() );
    }
  }
}

// print_uptime ===============================================================

static void print_uptime( FILE* file, player_t* p )
{
  bool first=true;

  for ( uptime_t* u = p -> uptime_list; u; u = u -> next )
  {
    if ( u -> percentage() > 0 )
    {
      if( first ) util_t::fprintf( file, "  Up-Times:\n" ); first = false;
      util_t::fprintf( file, "    %5.1f%% : %-30s\n", u -> percentage(), u -> name() );
    }
  }
}

// print_waiting ===============================================================

static void print_waiting( FILE* file, sim_t* sim )
{
  util_t::fprintf( file, "\nWaiting:\n" );

  bool nobody_waits = true;

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet )
      continue;

    if ( p -> total_waiting )
    {
      nobody_waits = false;
      util_t::fprintf( file, "    %4.1f%% : %s\n", 100.0 * p -> total_waiting / p -> total_seconds,  p -> name() );
    }
  }

  if ( nobody_waits ) util_t::fprintf( file, "    All players active 100%% of the time.\n" );
}

// print_performance ==========================================================

static void print_performance( FILE* file, sim_t* sim )
{
  util_t::fprintf( file,
                   "\nBaseline Performance:\n"
                   "  TotalEvents   = %ld\n"
                   "  MaxEventQueue = %ld\n"
                   "  TargetHealth  = %.0f\n"
                   "  SimSeconds    = %.0f\n"
                   "  CpuSeconds    = %.3f\n"
                   "  SpeedUp       = %.0f\n\n",
                   (long) sim -> total_events_processed,
                   (long) sim -> max_events_remaining,
                   sim -> target -> initial_health,
                   sim -> iterations * sim -> total_seconds,
                   sim -> elapsed_cpu_seconds,
                   sim -> iterations * sim -> total_seconds / sim -> elapsed_cpu_seconds );

  sim -> rng -> report( file );
}

// print_scale_factors ========================================================

static void print_scale_factors( FILE* file, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  util_t::fprintf( file, "\nScale Factors:\n" );

  int num_players = ( int ) sim -> players_by_name.size();
  int max_length=0;

  if ( sim -> report_precision < 0 )
    sim -> report_precision = 2;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    int length = ( int ) strlen( p -> name() );
    if ( length > max_length ) max_length = length;
  }

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    util_t::fprintf( file, "  %-*s", max_length, p -> name() );

    gear_stats_t& sf = ( sim -> scaling -> normalize_scale_factors ) ? p -> normalized_scaling : p -> scaling;

    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( p -> scales_with[ j ] != 0 )
      {
        util_t::fprintf( file, "  %s=%.*f", util_t::stat_type_abbrev( j ), sim -> report_precision, sf.get_stat( j ) );
      }
    }

    if ( sim -> scaling -> normalize_scale_factors )
    {
      util_t::fprintf( file, "  DPS/%s=%.*f", util_t::stat_type_abbrev( p -> normalize_by() ), sim -> report_precision, p -> scaling.get_stat( p -> normalize_by() ) );
    }

    if ( sim -> scaling -> scale_lag ) util_t::fprintf( file, "  Lag=%.*f", sim -> report_precision, p -> scaling_lag );

    util_t::fprintf( file, "\n" );
  }
}

// print_scale_factors ========================================================

static void print_scale_factors( FILE* file, player_t* p )
{
  if ( ! p -> sim -> scaling -> has_scale_factors() ) return;

  if ( p -> sim -> report_precision < 0 )
    p -> sim -> report_precision = 2;

  util_t::fprintf( file, "  Scale Factors:\n" );

  gear_stats_t& sf = ( p -> sim -> scaling -> normalize_scale_factors ) ? p -> normalized_scaling : p -> scaling;
  
  util_t::fprintf( file, "    Weights :" );
  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( p -> scales_with[ i ] != 0 )
    {
      util_t::fprintf( file, "  %s=%.*f", util_t::stat_type_abbrev( i ), p -> sim -> report_precision, sf.get_stat( i ) );
    }
  }
  if ( p -> sim -> scaling -> normalize_scale_factors )
  {
    util_t::fprintf( file, "  DPS/%s=%.*f", util_t::stat_type_abbrev( p -> normalize_by() ), p -> sim -> report_precision, p -> scaling.get_stat( p -> normalize_by() ) );
  }
  util_t::fprintf( file, "\n" );

  std::string lootrank = p -> gear_weights_lootrank_link;
  std::string wowhead  = p -> gear_weights_wowhead_link;
  std::string pawn_std = p -> gear_weights_pawn_std_string;
  std::string pawn_alt = p -> gear_weights_pawn_alt_string;

  simplify_html( lootrank );
  simplify_html( wowhead  );
  simplify_html( pawn_std );
  simplify_html( pawn_alt );

  util_t::fprintf( file, "    Wowhead : %s\n", wowhead.c_str() );
}

// print_dps_plots ============================================================

static void print_dps_plots( FILE* file, player_t* p )
{
  sim_t* sim = p -> sim;

  if ( sim -> plot -> dps_plot_stat_str.empty() ) return;

  int range = sim -> plot -> dps_plot_points / 2;

  double min = -range * sim -> plot -> dps_plot_step;
  double max = +range * sim -> plot -> dps_plot_step;

  int points = 1 + range * 2;

  util_t::fprintf( file, "  DPS Plot Data ( min=%.1f max=%.1f points=%d )\n", min, max, points );

  for( int i=0; i < STAT_MAX; i++ )
  {
    std::vector<double>& pd = p -> dps_plot_data[ i ];

    if ( ! pd.empty() )
    {
      util_t::fprintf( file, "    DPS(%s)=", util_t::stat_type_abbrev( i ) );
      int num_points = pd.size();
      for( int j=0; j < num_points; j++ )
      {
	util_t::fprintf( file, "%s%.0f", (j?"|":""), pd[ j ] );
      }
      util_t::fprintf( file, "\n" );
    }
  }
}

// print_reference_dps ========================================================

static void print_reference_dps( FILE* file, sim_t* sim )
{
  if ( sim -> reference_player_str.empty() ) return;

  if ( sim -> report_precision < 0 )
    sim -> report_precision = 2;

  util_t::fprintf( file, "\nReference DPS:\n" );

  player_t* ref_p = sim -> find_player( sim -> reference_player_str );

  if ( ! ref_p )
  {
    sim -> errorf( "Unable to locate reference player: %s\n", sim -> reference_player_str.c_str() );
    return;
  }

  int num_players = ( int ) sim -> players_by_rank.size();
  int max_length=0;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_rank[ i ];
    int length = ( int ) strlen( p -> name() );
    if ( length > max_length ) max_length = length;
  }

  util_t::fprintf( file, "  %-*s", max_length, ref_p -> name() );
  util_t::fprintf( file, "  %.0f", ref_p -> dps );

  if ( sim -> scaling -> has_scale_factors() )
  {
    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( ref_p -> scales_with[ j ] != 0 )
      {
        util_t::fprintf( file, "  %s=%.*f", util_t::stat_type_abbrev( j ), sim -> report_precision, ref_p -> scaling.get_stat( j ) );
      }
    }
  }

  util_t::fprintf( file, "\n" );

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_rank[ i ];

    if ( p != ref_p )
    {
      util_t::fprintf( file, "  %-*s", max_length, p -> name() );

      bool over = ( p -> dps > ref_p -> dps );

      double ratio = 100.0 * fabs( p -> dps - ref_p -> dps ) / ref_p -> dps;

      util_t::fprintf( file, "  %c%.0f%%", ( over ? '+' : '-' ), ratio );

      if ( sim -> scaling -> has_scale_factors() )
      {
        for ( int j=0; j < STAT_MAX; j++ )
        {
          if ( ref_p -> scales_with[ j ] != 0 )
          {
            double ref_sf = ref_p -> scaling.get_stat( j );
            double     sf =     p -> scaling.get_stat( j );

            over = ( sf > ref_sf );

            ratio = 100.0 * fabs( sf - ref_sf ) / ref_sf;

            util_t::fprintf( file, "  %s=%c%.0f%%", util_t::stat_type_abbrev( j ), ( over ? '+' : '-' ), ratio );
          }
        }
      }

      util_t::fprintf( file, "\n" );
    }
  }
}

struct compare_hat_donor_interval
{
  bool operator()( player_t* l, player_t* r ) SC_CONST
  {
    return( l -> procs.hat_donor -> frequency < 
	    r -> procs.hat_donor -> frequency );
  }
};

static void print_hat_donors( FILE* file, sim_t* sim )
{
  std::vector<player_t*> hat_donors;

  int num_players = ( int ) sim -> players_by_name.size();
  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    if( p -> procs.hat_donor -> count )
      hat_donors.push_back( p );
  }

  int num_donors = ( int ) hat_donors.size();
  if( num_donors )
  {
    std::sort( hat_donors.begin(), hat_donors.end(), compare_hat_donor_interval()  );

    util_t::fprintf( file, "\nHonor Among Thieves Donor Report:\n" );

    for( int i=0; i < num_donors; i++ )
    {
      player_t* p = hat_donors[ i ];
      proc_t* proc = p -> procs.hat_donor;
      util_t::fprintf( file, "  %.2fsec | %.3fcps : %s\n", proc -> frequency, ( 1.0 / proc -> frequency ), p -> name() );
    }
  }
}

// print_html_contents =======================================================

static void print_html_contents( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "<h1>Table of Contents</h1>\n" );
  util_t::fprintf( file, "<ul>\n" );
  util_t::fprintf( file, " <li> <a href=\"#raid_summary\"> Raid Summary </a> </li>\n" );
  if ( sim -> scaling -> has_scale_factors() )
  {
    util_t::fprintf( file, " <li> <a href=\"#scale_factors\"> Scale Factors </a> </li>\n" );
  }
  util_t::fprintf( file, " <li> <a href=\"#auras_debuffs\"> Auras and Debuffs </a> </li>\n" );
  int num_players = ( int ) sim -> players_by_name.size();
  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    util_t::fprintf( file, " <li> <a href=\"#%s\"> %s </a> </li>\n", p -> name(), p -> name() );
  }
  util_t::fprintf( file, "</ul>\n" );
  util_t::fprintf( file, "<br /><hr />\n" );
}

// print_html_raid_summary ===================================================

static void print_html_raid_summary( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "<a name=\"raid_summary\" /><h1>Raid Summary</h1>\n" );

  util_t::fprintf( file, "<style type=\"text/css\">\n  table.charts td, table.charts th { padding: 4px; border: 1px inset; }\n  table.charts { border: 1px outset; }</style>\n" );

  assert( sim ->  dps_charts.size() ==
          sim -> gear_charts.size() );

  util_t::fprintf( file, "<table class=\"charts\">\n" );
  int count = ( int ) sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file, " <tr> <td><img src=\"%s\" /></td> <td><img src=\"%s\" /></td> </tr>\n", 
		     sim -> dps_charts[ i ].c_str(), sim -> gear_charts[ i ].c_str() );
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"charts\">\n" );
  count = ( int ) sim -> dpet_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file, " <tr> <td><img src=\"%s\" /></td> </tr>\n", sim -> dpet_charts[ i ].c_str() );
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<img src=\"%s\" /> <br />\n", sim -> timeline_chart.c_str() );
}

// print_html_scale_factors ===================================================

static void print_html_scale_factors( FILE*  file, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  if ( sim -> report_precision < 0 )
    sim -> report_precision = 2;

  util_t::fprintf( file, "<a name=\"scale_factors\" /><h1>DPS Scale Factors (dps increase per unit stat)</h1>\n" );

  util_t::fprintf( file, "<style type=\"text/css\">\n  table.scale_factors td, table.scale_factors th { padding: 4px; border: 1px inset; }\n  table.scale_factors { border: 1px outset; }</style>\n" );

  util_t::fprintf( file, "<table class=\"scale_factors\">\n" );

  std::string buffer;
  int num_players = ( int ) sim -> players_by_name.size();
  int prev_type = PLAYER_NONE;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if( p -> type != prev_type )
    {
      prev_type = p -> type;

      util_t::fprintf( file, "  <tr> <th>profile</th>" );
      for ( int i=0; i < STAT_MAX; i++ )
      {
	if ( sim -> scaling -> stats.get_stat( i ) != 0 )
        {
	  util_t::fprintf( file, " <th>%s</th>", util_t::stat_type_abbrev( i ) );
	}
      }
      util_t::fprintf( file, " <th>wowhead</th> <th>lootrank</th> </tr>\n" );
    }

    util_t::fprintf( file, "  <tr>\n    <td>%s</td>\n", p -> name() );
    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        util_t::fprintf( file, "    <td>%.*f</td>\n", sim -> report_precision, p -> scaling.get_stat( j ) );
      }
    }
    util_t::fprintf( file, " <td><a href=\"%s\"> wowhead </a></td> <td><a href=\"%s\"> lootrank</a></td> </tr>\n",
		     p -> gear_weights_wowhead_link.c_str(), p -> gear_weights_lootrank_link.c_str() );
  }
  util_t::fprintf( file, "</table> <br />\n" );
}

// print_html_auras_debuffs ==================================================

static void print_html_auras_debuffs( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "<a name=\"auras_debuffs\" /><h1>Auras and Debuffs</h1>\n" );

  util_t::fprintf( file, "<style type=\"text/css\">\n  table.auras td, table.auras th { padding: 4px; border: 1px inset; }\n  table.charts { border: 1px outset; }</style>\n" );

  util_t::fprintf( file, "<table class=\"auras\">\n  <tr> <th>Dynamic</th> <th>Start</th> <th>Refresh</th> <th>Interval</th> <th>Trigger</th> <th>Up-Time</th> <th>Benefit</th> </tr>\n" );
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || b -> constant )
      continue;

    util_t::fprintf( file, "  <tr> <td>%s</td> <td>%.1f</td> <td>%.1f</td> <td>%.1fsec</td> <td>%.1fsec</td> <td>%.0f%%</td> <td>%.0f%%</td> </tr>\n", 
		     b -> name(), b -> avg_start, b -> avg_refresh, 
		     b -> avg_start_interval, b -> avg_trigger_interval, 
		     b -> uptime_pct, b -> benefit_pct > 0 ? b -> benefit_pct : b -> uptime_pct );
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"auras\">\n  <tr> <th>Constant</th> </tr>\n" );
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || ! b -> constant )
      continue;

    util_t::fprintf( file, "  <tr> <td>%s</td> </tr>\n", b -> name() );
  }
  util_t::fprintf( file, "</table> <br />\n" );

}

// print_html_action =========================================================

static void print_html_action( FILE* file, stats_t* s )
{
  double executes_divisor = s -> num_executes;
  double    ticks_divisor = s -> num_ticks;

  if ( executes_divisor <= 0 ) executes_divisor = 1;
  if (    ticks_divisor <= 0 )    ticks_divisor = 1;

  util_t::fprintf( file,
		   " <tr>"
		   " <td>%s</td> <td>%.0f</td> <td>%.1f%%</td> <td>%.1f</td> <td>%.2fsec</td>"
		   " <td>%.0f</td> <td>%.0f</td> <td>%.1f</td> <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> <td>%.1f%%</td>"
		   " <td>%.1f%%</td> <td>%.1f%%</td> <td>%.1f%%</td> <td>%.1f%%</td>"
		   " <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> <td>%.1f%%</td> <td>%.1f%%</td>"
		   " </tr>\n",
		   s -> name_str.c_str(), s -> portion_dps, s -> portion_dmg * 100, 
		   s -> num_executes, s -> frequency,
		   s -> dpe, s -> dpet, s -> dpr,
		   s -> execute_results[ RESULT_HIT  ].avg_dmg,
		   s -> execute_results[ RESULT_CRIT ].avg_dmg,
		   s -> execute_results[ RESULT_CRIT ].max_dmg,
		   s -> execute_results[ RESULT_CRIT ].count * 100 / executes_divisor,
		   s -> execute_results[ RESULT_MISS ].count * 100 / executes_divisor,
		   s -> execute_results[ RESULT_DODGE  ].count * 100.0 / executes_divisor,
		   s -> execute_results[ RESULT_PARRY  ].count * 100.0 / executes_divisor,
		   s -> execute_results[ RESULT_GLANCE ].count * 100.0 / executes_divisor,
		   s -> num_ticks,
		   s -> tick_results[ RESULT_HIT  ].avg_dmg,
		   s -> tick_results[ RESULT_CRIT ].avg_dmg,
		   s -> tick_results[ RESULT_CRIT ].count * 100.0 / ticks_divisor,
		   s -> tick_results[ RESULT_MISS ].count * 100.0 / ticks_divisor );
}

// print_html_player =========================================================

static void print_html_player( FILE* file, player_t* p )
{
  char buffer[ 4096 ];

  util_t::fprintf( file, "<a name=\"%s\" /><h1>%s&nbsp;:&nbsp;%.0fdps</h1>\n", p -> name(), p -> name(), p -> dps );

  util_t::fprintf( file, "<style type=\"text/css\">\n  table.player td, table.player th { padding: 4px; border: 1px inset; }\n  table.player { border: 1px outset; }</style>\n" );

  util_t::fprintf( file, 
		   "<table class=\"player\">\n"
		   "  <tr> <th>Name</th> <th>Race</th> <th>Class</th> <th>Tree</th> <th>Level</th> </tr>\n"
		   "  <tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> <td>%d</td>\n"
		   "</table><br />\n",
		   p -> name(), p -> race_str.c_str(), 
		   util_t::player_type_string( p -> type ),
		   util_t::talent_tree_string( p -> primary_tree() ), p -> level );

  util_t::fprintf( file, 
		   "<table class=\"player\">\n"
		   "  <tr> <th>DPS</th> <th>Error</th> <th>Range</th> <th>DPR</th> <th>RPS-Out</th> <th>RPS-In</th> <th>Resource</th> <th>Waiting</th> <th>ApM</th> </tr>\n"
		   "  <tr> <td>%.0f</td> <td>%.1f%%</td> <td>%.1f%%</td> <td>%.1f</td> <td>%.1f</td> <td>%.1f</td> <td>%s</td> <td>%.1f%%</td> <td>%.1f</td>\n"
		   "</table><br />\n",
		   p -> dps, p -> dps_error * 100 / p -> dps, ( ( p -> dps_max - p -> dps_min ) / 2 ) * 100 / p -> dps,
		   p -> dpr, p -> rps_loss, p -> rps_gain, util_t::resource_type_string( p -> primary_resource() ),
		   100.0 * p -> total_waiting / p -> total_seconds, 
		   60.0 * p -> total_foreground_actions / p -> total_seconds  );

  util_t::fprintf( file, 
		   "<table class=\"player\">\n"
		   "  <tr> <th>Origin</th> <td><a href=\"%s\">%s</a></td>\n"
		   "</table><br />\n",
		   p -> origin_str.c_str(), p -> origin_str.c_str() );

  util_t::fprintf( file, 
		   "<table class=\"player\">\n"
		   "  <tr> <th>Talents</th> <td><a href=\"%s\">%s</a></td>\n"
		   "</table><br />\n",
		   p -> talents_str.c_str(), p -> talents_str.c_str() );

  util_t::fprintf( file, "<table class=\"player\"> <tr> <th>Glyphs</th>" );
  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, p -> glyphs_str, ",/" );
  for ( int i=0; i < num_glyphs; i++ )
  {
    util_t::fprintf( file, " <td>%s</td>", glyph_names[ i ].c_str() );
  }
  util_t::fprintf( file, " </tr> </table> <br />\n" );

  if ( p -> sim -> scaling -> has_scale_factors() )
  {
    util_t::fprintf( file, "<table class=\"player\">\n" );

    util_t::fprintf( file, " <tr> <th></th>" );
    for ( int i=0; i < STAT_MAX; i++ )
      if ( p -> scales_with[ i ] )
	util_t::fprintf( file, " <th>%s</th>", util_t::stat_type_abbrev( i ) );
    util_t::fprintf( file, " </tr>\n" );

    util_t::fprintf( file, " <tr> <th>Scale Factors</th>" );
    for ( int i=0; i < STAT_MAX; i++ )
      if ( p -> scales_with[ i ] )
	util_t::fprintf( file, " <td>%.*f</td>", p -> sim -> report_precision, p -> scaling.get_stat( i ) );
    util_t::fprintf( file, " </tr>\n" );

    util_t::fprintf( file, " <tr> <th>Normalized</th>" );
    for ( int i=0; i < STAT_MAX; i++ )
      if ( p -> scales_with[ i ] )
	util_t::fprintf( file, " <td>%.*f</td>", p -> sim -> report_precision, p -> normalized_scaling.get_stat( i ) );
    util_t::fprintf( file, " </tr>\n" );

    util_t::fprintf( file, "</table>\n" );
    util_t::fprintf( file, "<i>DPS gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>DPS loss</b> per unit stat <b>decrease</b></i><p>\n" );

    util_t::fprintf( file, "<table class=\"player\">\n" );
    util_t::fprintf( file, " <tr> <th>Gear Ranking</th> <td><a href=\"%s\">wowhead</a></td> <td><a href=\"%s\">lootrank</a></td> </tr>\n",
		     p -> gear_weights_wowhead_link.c_str(), p -> gear_weights_lootrank_link.c_str() );
    util_t::fprintf( file, "</table> <br />\n" );
  }

  std::string action_dpet_str       = "empty";
  std::string action_dmg_str        = "empty";
  std::string gains_str             = "empty";
  std::string timeline_resource_str = "empty";
  std::string timeline_dps_str      = "empty";
  std::string distribution_dps_str  = "empty";

  if ( ! p -> action_dpet_chart.empty() )
  {
    snprintf( buffer, sizeof( buffer ), "<img name=\"chart_action_dpet\" src=\"%s\" />\n", p -> action_dpet_chart.c_str() );
    action_dpet_str = buffer;
  }
  if ( ! p -> action_dmg_chart.empty() )
  {
    snprintf( buffer, sizeof( buffer ), "<img name=\"chart_action_dmg\" src=\"%s\" />\n", p -> action_dmg_chart.c_str() );
    action_dmg_str = buffer;
  }
  if ( ! p -> gains_chart.empty() )
  {
    snprintf( buffer, sizeof( buffer ), "<img name=\"chart_gains\" src=\"%s\" />\n", p -> gains_chart.c_str() );
    gains_str = buffer;
  }
  if ( ! p -> timeline_resource_chart.empty() )
  {
    snprintf( buffer, sizeof( buffer ), "<img name=\"chart_timeline_resource\" src=\"%s\" />\n", p -> timeline_resource_chart.c_str() );
    timeline_resource_str = buffer;
  }
  if ( ! p -> timeline_dps_chart.empty() )
  {
    snprintf( buffer, sizeof( buffer ), "<img name=\"chart_timeline_dps\" src=\"%s\" />\n", p -> timeline_dps_chart.c_str() );
    timeline_dps_str = buffer;
  }
  if ( ! p -> distribution_dps_chart.empty() )
  {
    snprintf( buffer, sizeof( buffer ), "<img name=\"chart_distribution_dps\" src=\"%s\" />\n", p -> distribution_dps_chart.c_str() );
    distribution_dps_str = buffer;
  }

  util_t::fprintf( file, 
		   "<table class=\"player\">\n"
		   "  <tr> <td>%s</td> <td>%s</td> </tr>\n"
		   "  <tr> <td>%s</td> <td>%s</td> </tr>\n"
		   "  <tr> <td>%s</td> <td>%s</td> </tr>\n",
		   action_dpet_str.c_str(), action_dmg_str.c_str(),
		   gains_str.c_str(), timeline_resource_str.c_str(),
		   timeline_dps_str.c_str(), distribution_dps_str.c_str() );

  if ( ( ! p -> scaling_dps_chart.empty() ) || ( ! p -> scale_factors_chart.empty() ) )
  {
    util_t::fprintf( file, "<tr>\n" );
    if( ! p -> scaling_dps_chart.empty() )
    {
       util_t::fprintf( file, "  <td><img name=\"chart_scaling_dps\" src=\"%s\" /></td>\n",
                        p -> scaling_dps_chart.c_str() );
    }
    if( ! p -> scale_factors_chart.empty() )
    {
       util_t::fprintf( file, "  <td><img name=\"scale_factors\" src=\"%s\" /></td>\n",
                        p -> scale_factors_chart.c_str() );
    }
  }
  util_t::fprintf( file, "</tr></table> <br />\n" );

  util_t::fprintf( file, 
		   "<table class=\"player\">\n"
		   " <tr>"
		   " <th>Ability</th> <th>DPS</th> <th>DPS%%</th> <th>Count</th> <th>Interval</th>"
		   " <th>DPE</th> <th>DPET</th> <th>DPR</th> <th>Hit</th> <th>Crit</th> <th>Max</th> <th>Crit%%</th>"
		   " <th>M%%</th> <th>D%%</th> <th>P%%</th> <th>G%%</th>"
		   " <th>Ticks</th> <th>T-Hit</th> <th>T-Crit</th> <th>T-Crit%%</th> <th>T-M%%</th>"
		   " </tr>\n" );

  util_t::fprintf( file, " <tr> <th>%s</th> <th>%.0f</th> </tr>\n", p -> name(), p -> dps );

  for ( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    if ( s -> total_dmg > 0 )
    {
      print_html_action( file, s );
    }
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    bool first=true;

    for ( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if ( s -> total_dmg > 0 )
      {
        if ( first )
        {
          first = false;
	  util_t::fprintf( file, " <tr> <th>pet - %s</th> <th>%.0f</th> </tr>\n", pet -> name_str.c_str(), pet -> dps );
        }
        print_html_action( file, s );
      }
    }
  }

  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"player\">\n  <tr> <th>Dynamic Buffs</th> <th>Start</th> <th>Refresh</th> <th>Interval</th> <th>Trigger</th> <th>Up-Time</th> <th>Benefit</th> </tr>\n" );
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || b -> constant )
      continue;

    util_t::fprintf( file, "  <tr> <td>%s</td> <td>%.1f</td> <td>%.1f</td> <td>%.1fsec</td> <td>%.1fsec</td> <td>%.0f%%</td> <td>%.0f%%</td> </tr>\n", 
		     b -> name(), b -> avg_start, b -> avg_refresh, 
		     b -> avg_start_interval, b -> avg_trigger_interval, 
		     b -> uptime_pct, b -> benefit_pct > 0 ? b -> benefit_pct : b -> uptime_pct );
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"player\">\n  <tr> <th>Constant Buffs</th> </tr>\n" );
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || ! b -> constant )
      continue;

    util_t::fprintf( file, "  <tr> <td>%s</td> </tr>\n", b -> name() );
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"player\">\n  <tr> <th>Up-Times</th> <th>%%</th> </tr>\n" );
  for ( uptime_t* u = p -> uptime_list; u; u = u -> next )
  {
    if ( u -> percentage() > 0 ) 
    {
      util_t::fprintf( file, "  <tr> <td>%s</td> <td>%.1f%%</td> </tr>\n", u -> name(), u -> percentage() );
    }
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"player\">\n  <tr> <th>Procs</th> <th>Count</th> <th>Interval</th> </tr>\n" );
  for ( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
  {
    if ( proc -> count > 0 )
    {
      util_t::fprintf( file, "  <tr> <td>%s</td> <td>%.1f</td> <td>%.1fsec</td> </tr>\n", proc -> name(), proc -> count, proc -> frequency );
    }
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"player\">\n  <tr> <th>Gains</th> <th>%s</th> <th>Overflow</th> </tr>\n", util_t::resource_type_string( p -> primary_resource() ) );
  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 )
    {
      double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
      util_t::fprintf( file, "  <tr> <td>%s</td> <td>%.1f</td> <td>%.1f%%</td> </tr>\n", g -> name(), g -> actual, overflow_pct );
    }
  }
  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    if ( pet -> total_dmg <= 0 ) continue;
    bool first = true;
    for ( gain_t* g = pet -> gain_list; g; g = g -> next )
    {
      if ( g -> actual > 0 )
      {
	if ( first )
	{
	  first = false;
	  util_t::fprintf( file, "  <tr> <th>pet - %s</th> <th>%s</th> </tr>\n", pet -> name_str.c_str(), util_t::resource_type_string( pet -> primary_resource() ) );
	}
	double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
	util_t::fprintf( file, "  <tr> <td>%s</td> <td>%.1f</td> <td>%.1f%%</td> </tr>\n", g -> name(), g -> actual, overflow_pct );
      }
    }
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"player\">\n  <tr> <th></th> <th>Action Priority List</th> </tr>\n" );
  std::vector<std::string> action_names;
  int num_actions = util_t::string_split( action_names, p -> action_list_str, "/" );
  for ( int i=0; i < num_actions; i++ )
  {
    util_t::fprintf( file, "  <tr> <th>%d</th> <td>%s</td> </tr>\n", i, action_names[ i ].c_str() );
  }
  util_t::fprintf( file, "</table> <br />\n" );

  util_t::fprintf( file, "<table class=\"player\">\n  <tr> <th>Stat</th> <th>Raid-Buffed</th> <th>Un-Buffed</th> <th>Gear Amount</th> </tr>\n" );

  util_t::fprintf( file, " <tr> <th>Strength</th>  <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n", p -> attribute_buffed[ ATTR_STRENGTH  ], p -> strength(),  p -> stats.attribute[ ATTR_STRENGTH  ] );
  util_t::fprintf( file, " <tr> <th>Agility</th>   <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n", p -> attribute_buffed[ ATTR_AGILITY   ], p -> agility(),   p -> stats.attribute[ ATTR_AGILITY   ] );
  util_t::fprintf( file, " <tr> <th>Stamina</th>   <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n", p -> attribute_buffed[ ATTR_STAMINA   ], p -> stamina(),   p -> stats.attribute[ ATTR_STAMINA   ] );
  util_t::fprintf( file, " <tr> <th>Intellect</th> <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n", p -> attribute_buffed[ ATTR_INTELLECT ], p -> intellect(), p -> stats.attribute[ ATTR_INTELLECT ] );
  util_t::fprintf( file, " <tr> <th>Spirit</th>    <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n", p -> attribute_buffed[ ATTR_SPIRIT    ], p -> spirit(),    p -> stats.attribute[ ATTR_SPIRIT    ] );
  util_t::fprintf( file, " <tr> <th>Health</th>    <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n", p -> resource_buffed[ RESOURCE_HEALTH ], p -> resource_max[ RESOURCE_HEALTH ], 0.0 );
  util_t::fprintf( file, " <tr> <th>Mana</th>      <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n", p -> resource_buffed[ RESOURCE_MANA   ], p -> resource_max[ RESOURCE_MANA   ], 0.0 );

  util_t::fprintf( file, " <tr> <th>Spell Power</th>       <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n",     p -> buffed_spell_power, p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier(), p -> stats.spell_power );
  util_t::fprintf( file, " <tr> <th>Spell Hit</th>         <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_spell_hit,          100 * p -> composite_spell_hit(),          p -> stats.hit_rating  );
  util_t::fprintf( file, " <tr> <th>Spell Crit</th>        <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_spell_crit,         100 * p -> composite_spell_crit(),         p -> stats.crit_rating );
  util_t::fprintf( file, " <tr> <th>Spell Haste</th>       <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * ( 1 / p -> buffed_spell_haste - 1 ), 100 * ( 1 / p -> spell_haste - 1 ), p -> stats.haste_rating );
  util_t::fprintf( file, " <tr> <th>Spell Penetration</th> <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n",     100 * p -> buffed_spell_penetration,  100 * p -> composite_spell_penetration(),  p -> stats.spell_penetration );
  util_t::fprintf( file, " <tr> <th>Mana Per 5</th>        <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n",     p -> buffed_mp5, p -> composite_mp5(), p -> stats.mp5 );

  util_t::fprintf( file, " <tr> <th>Attack Power</th>      <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n",     p -> buffed_attack_power, p -> composite_attack_power() * p -> composite_attack_power_multiplier(), p -> stats.attack_power );
  util_t::fprintf( file, " <tr> <th>Melee Hit</th>         <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_attack_hit,         100 * p -> composite_attack_hit(),         p -> stats.hit_rating );
  util_t::fprintf( file, " <tr> <th>Melee Crit</th>        <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_attack_crit,        100 * p -> composite_attack_crit(),        p -> stats.crit_rating );
  util_t::fprintf( file, " <tr> <th>Melee Haste</th>       <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * ( 1 / p -> buffed_attack_haste - 1 ), 100 * ( 1 / p -> attack_haste - 1 ), p -> stats.haste_rating );
  util_t::fprintf( file, " <tr> <th>Expertise</th>         <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_attack_expertise,   100 * p -> composite_attack_expertise(),   p -> stats.expertise_rating );
  util_t::fprintf( file, " <tr> <th>Armor Penetration</th> <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_attack_penetration, 100 * p -> composite_attack_penetration(), p -> stats.armor_penetration_rating );

  util_t::fprintf( file, " <tr> <th>Armor</th>       <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n",     p -> buffed_armor,       p -> composite_armor(), ( p -> stats.armor + p -> stats.bonus_armor ) );
  util_t::fprintf( file, " <tr> <th>Block Value</th> <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n",     p -> buffed_block_value, p -> composite_block_value(), p -> stats.block_value );
  util_t::fprintf( file, " <tr> <th>Defense</th>     <td>%.0f</td> <td>%.0f</td> <td>%.0f</td> </tr>\n",     p -> buffed_defense,     p -> composite_defense(), p -> stats.defense_rating );
  util_t::fprintf( file, " <tr> <th>Tank-Miss</th>   <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_miss,  100 * ( p -> composite_tank_miss( SCHOOL_PHYSICAL ) - p -> diminished_miss( SCHOOL_PHYSICAL ) ), 0.0  );
  util_t::fprintf( file, " <tr> <th>Tank-Dodge</th>  <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_dodge, 100 * ( p -> composite_tank_dodge() - p -> diminished_dodge() ), p -> stats.dodge_rating );
  util_t::fprintf( file, " <tr> <th>Tank-Parry</th>  <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_parry, 100 * ( p -> composite_tank_parry() - p -> diminished_parry() ), p -> stats.parry_rating );
  util_t::fprintf( file, " <tr> <th>Tank-Block</th>  <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_block, 100 * p -> composite_tank_block(), p -> stats.block_rating );
  util_t::fprintf( file, " <tr> <th>Tank-Crit</th>   <td>%.2f%%</td> <td>%.2f%%</td> <td>%.0f</td> </tr>\n", 100 * p -> buffed_crit,  100 * p -> composite_tank_crit( SCHOOL_PHYSICAL ), 0.0 );

  util_t::fprintf( file, "</table> <br />\n" );

  if ( p -> sim -> scaling -> has_scale_factors() )
  {
    util_t::fprintf( file, "<table class=\"player\">\n" );
    util_t::fprintf( file, " <tr> <th>Pawn Standard</th> <td>%s</td> </tr>\n",      p -> gear_weights_pawn_std_string.c_str() );
    util_t::fprintf( file, " <tr> <th>Zero Hit/Expertise</th> <td>%s</td> </tr>\n", p -> gear_weights_pawn_alt_string.c_str() );
    util_t::fprintf( file, "</table> <br />\n" );

    std::string rhada_std = p -> gear_weights_pawn_std_string;  
    std::string rhada_alt = p -> gear_weights_pawn_alt_string;  

    if ( rhada_std.size() > 10 ) rhada_std.replace( 2, 8, "RhadaTip" );
    if ( rhada_alt.size() > 10 ) rhada_alt.replace( 2, 8, "RhadaTip" );

    util_t::fprintf( file, "<table class=\"player\">\n" );
    util_t::fprintf( file, " <tr> <th>RhadaTip Standard</th> <td>%s</td> </tr>\n",  rhada_std.c_str() );
    util_t::fprintf( file, " <tr> <th>Zero Hit/Expertise</th> <td>%s</td> </tr>\n", rhada_alt.c_str() );
    util_t::fprintf( file, "</table> <br />\n" );
  }

  util_t::fprintf( file, "<hr />\n" );
}

// print_wiki_preamble =======================================================

static void print_wiki_preamble( FILE* file, sim_t* sim )
{
  int arch = 0, version = 0, revision = 0;
  sim -> patch.decode( &arch, &version, &revision );
  util_t::fprintf( file, "= !SimulationCraft %s.%s for World of Warcraft release %d.%d.%d =\n", SC_MAJOR_VERSION, SC_MINOR_VERSION, arch, version, revision );

  time_t rawtime;
  time ( &rawtime );

  util_t::fprintf( file, " * Timestamp: %s", ctime( &rawtime ) );
  util_t::fprintf( file, " * Iterations: %d\n", sim -> iterations );
  util_t::fprintf( file, " * Fight Length: %.0f\n", sim -> max_time );
  util_t::fprintf( file, " * Smooth RNG: %s\n", ( sim -> smooth_rng ? "true" : "false" ) );

  util_t::fprintf( file, "\n----\n\n<br>\n\n" );
}

// print_wiki_contents =======================================================

static void print_wiki_contents( FILE* file, sim_t* sim, const std::string& wiki_name, int player_type )
{
  util_t::fprintf( file, "= Table of Contents =\n" );
  util_t::fprintf( file, " * [%s#Raid_Summary Raid Summary ]\n", wiki_name.c_str() );
  if ( player_type == PLAYER_NONE )
  { 
    if ( sim -> scaling -> has_scale_factors() )
    {
      util_t::fprintf( file, " * [%s#DPS_Scale_Factors DPS Scale Factors ]\n", wiki_name.c_str() );
    }
    util_t::fprintf( file, " * [%s#Auras_and_Debuffs Auras and Debuffs ]\n", wiki_name.c_str() );
  }
  int num_players = ( int ) sim -> players_by_name.size();
  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if ( player_type != PLAYER_NONE && 
	 player_type != p -> type ) 
      continue;

    std::string anchor_name = wiki_player_reference( p );

    util_t::fprintf( file, " * [%s%s#%s %s]\n", wiki_name.c_str(), player_type_string( p ), anchor_name.c_str(), p -> name() );
  }
  util_t::fprintf( file, "\n----\n\n<br>\n\n" );
}

// print_wiki_raid_summary ===================================================

static void print_wiki_raid_summary( FILE* file, sim_t* sim )
{
  util_t::fprintf( file, "= Raid Summary =\n" );

  assert( sim ->  dps_charts.size() ==
          sim -> gear_charts.size() );

  int count = ( int ) sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    std::string  dps_link = sim ->  dps_charts[ i ]; simplify_html(  dps_link );
    std::string gear_link = sim -> gear_charts[ i ]; simplify_html( gear_link );

    util_t::fprintf( file, "|| %s&dummy=dummy.png || %s&dummy=dummy.png ||\n", dps_link.c_str(), gear_link.c_str() );
  }

  count = ( int ) sim -> dpet_charts.size();
  for ( int i=0; i < count; i++ )
  {
    std::string link = sim -> dpet_charts[ i ]; simplify_html( link );
    util_t::fprintf( file, "|| %s&dummy=dummy.png || \n", link.c_str() );
  }
  std::string link = sim -> timeline_chart; simplify_html( link );
  util_t::fprintf( file, "|| %s&dummy=dummy.png || \n", link.c_str() );

  util_t::fprintf( file, "\n----\n\n<br>\n\n" );
}

// print_wiki_scale_factors ===================================================

static void print_wiki_scale_factors( FILE*  file, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  if ( sim -> report_precision < 0 ) sim -> report_precision = 2;

  util_t::fprintf( file, "=DPS Scale Factors=\n" );

  std::string buffer;
  int num_players = ( int ) sim -> players_by_name.size();
  int prev_type = PLAYER_NONE;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if( p -> type != prev_type )
    {
      prev_type = p -> type;

      util_t::fprintf( file, "|| *profile* ||" );
      for ( int i=0; i < STAT_MAX; i++ )
      {
	if ( sim -> scaling -> stats.get_stat( i ) != 0 )
        {
	  util_t::fprintf( file, " *%s* ||", util_t::stat_type_abbrev( i ) );
	}
      }
      util_t::fprintf( file, " *wowhead* || *lootrank* ||\n" );
    }

    util_t::fprintf( file, "|| *!%s* ||", p -> name() );
    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        util_t::fprintf( file, " %.*f ||", sim -> report_precision, p -> scaling.get_stat( j ) );
      }
    }

    std::string lootrank = p -> gear_weights_lootrank_link; simplify_html( lootrank );
    std::string wowhead  = p -> gear_weights_wowhead_link;  simplify_html( wowhead  );

    util_t::fprintf( file, " [%s wowhead] || [%s lootrank] ||\n", wowhead.c_str(), lootrank.c_str() );
  }

  util_t::fprintf( file, "\n----\n\n<br>\n\n" );
}

// print_wiki_auras_debuffs ==================================================

static void print_wiki_auras_debuffs( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "= Auras and Debuffs =\n" );

  util_t::fprintf( file, "|| *Dynamic* || *Start* || *Refresh* || *Interval* || *Trigger* || *Up-Time* || *Benefit* ||\n" );
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || b -> constant )
      continue;

    util_t::fprintf( file, "|| %s || %.1f || %.1f || %.1fsec || %.1fsec || %.0f%% || %.0f%% ||\n", 
		     b -> name(), b -> avg_start, b -> avg_refresh, 
		     b -> avg_start_interval, b -> avg_trigger_interval, 
		     b -> uptime_pct, b -> benefit_pct > 0 ? b -> benefit_pct : b -> uptime_pct );
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| *Constant* ||\n" );
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || ! b -> constant )
      continue;

    util_t::fprintf( file, "|| %s ||\n", b -> name() );
  }
  util_t::fprintf( file, "\n----\n\n<br>\n\n" );

}

// print_wiki_action =========================================================

static void print_wiki_action( FILE* file, stats_t* s )
{
  double executes_divisor = s -> num_executes;
  double    ticks_divisor = s -> num_ticks;

  if ( executes_divisor <= 0 ) executes_divisor = 1;
  if (    ticks_divisor <= 0 )    ticks_divisor = 1;

  util_t::fprintf( file,
		   "||%s||%.0f||%.1f%%||%.1f||%.2fsec||%.0f||%.0f||%.1f||%.0f||%.0f||%.0f||%.1f%%||%.1f%%||%.1f%%||%.1f%%||%.1f%%||%.0f||%.0f||%.0f||%.1f%%||%.1f%%||\n",
		   s -> name_str.c_str(), s -> portion_dps, s -> portion_dmg * 100, 
		   s -> num_executes, s -> frequency,
		   s -> dpe, s -> dpet, s -> dpr,
		   s -> execute_results[ RESULT_HIT  ].avg_dmg,
		   s -> execute_results[ RESULT_CRIT ].avg_dmg,
		   s -> execute_results[ RESULT_CRIT ].max_dmg,
		   s -> execute_results[ RESULT_CRIT ].count * 100 / executes_divisor,
		   s -> execute_results[ RESULT_MISS ].count * 100 / executes_divisor,
		   s -> execute_results[ RESULT_DODGE  ].count * 100.0 / executes_divisor,
		   s -> execute_results[ RESULT_PARRY  ].count * 100.0 / executes_divisor,
		   s -> execute_results[ RESULT_GLANCE ].count * 100.0 / executes_divisor,
		   s -> num_ticks,
		   s -> tick_results[ RESULT_HIT  ].avg_dmg,
		   s -> tick_results[ RESULT_CRIT ].avg_dmg,
		   s -> tick_results[ RESULT_CRIT ].count * 100.0 / ticks_divisor,
		   s -> tick_results[ RESULT_MISS ].count * 100.0 / ticks_divisor );
}

// print_wiki_player =========================================================

static void print_wiki_player( FILE* file, player_t* p )
{
  std::string anchor_name = wiki_player_anchor( p );

  util_t::fprintf( file, "= %s =\n", anchor_name.c_str() );

  util_t::fprintf( file, 
		   "|| *Name* || *Race* || *Class* || *Tree* || *Level* ||\n"
		   "|| *!%s*  || %s     || %s      || %s     || %d      ||\n"
		   "\n",
		   p -> name(), p -> race_str.c_str(), 
		   util_t::player_type_string( p -> type ),
		   util_t::talent_tree_string( p -> primary_tree() ), p -> level );

  util_t::fprintf( file, 
		   "|| *DPS*  || *Error* || *Range* || *DPR* || *RPS-Out* || *RPS-In* || *Resource* || *Waiting* || *ApM* ||\n"
		   "|| *%.0f* || %.1f%%  || %.1f%%  || %.1f  || %.1f      || %.1f     || %s         || %.1f%%    || %.1f  ||\n"
		   "\n",
		   p -> dps, p -> dps_error * 100 / p -> dps, ( ( p -> dps_max - p -> dps_min ) / 2 ) * 100 / p -> dps,
		   p -> dpr, p -> rps_loss, p -> rps_gain, util_t::resource_type_string( p -> primary_resource() ),
		   100.0 * p -> total_waiting / p -> total_seconds, 
		   60.0 * p -> total_foreground_actions / p -> total_seconds  );

  util_t::fprintf( file, "|| *Origin* || [%s] ||\n\n", p -> origin_str.c_str() );

  util_t::fprintf( file, "|| *Talents* || [%s] ||\n\n", p -> talents_str.c_str() );


  util_t::fprintf( file, "|| *Glyphs* ||" );
  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, p -> glyphs_str, ",/" );
  for ( int i=0; i < num_glyphs; i++ )
  {
    util_t::fprintf( file, " %s ||", glyph_names[ i ].c_str() );
  }
  util_t::fprintf( file, " \n\n" );

  if ( p -> sim -> scaling -> has_scale_factors() )
  {
    util_t::fprintf( file, "|| ||" );
    for ( int i=0; i < STAT_MAX; i++ )
      if ( p -> scales_with[ i ] )
	util_t::fprintf( file, " *%s* ||", util_t::stat_type_abbrev( i ) );
    util_t::fprintf( file, "\n" );

    util_t::fprintf( file, "|| *Scale Factors* ||" );
    for ( int i=0; i < STAT_MAX; i++ )
      if ( p -> scales_with[ i ] )
	util_t::fprintf( file, " %.*f ||", p -> sim -> report_precision, p -> scaling.get_stat( i ) );
    util_t::fprintf( file, "\n" );

    util_t::fprintf( file, "|| *Normalized* ||" );
    for ( int i=0; i < STAT_MAX; i++ )
      if ( p -> scales_with[ i ] )
	util_t::fprintf( file, " %.*f ||", p -> sim -> report_precision, p -> normalized_scaling.get_stat( i ) );
    util_t::fprintf( file, "\n" );

    util_t::fprintf( file, "_DPS gain per unit stat increase except for *Hit/Expertise* which represent *DPS loss* per unit stat *decrease*_<p>\n" );

    std::string lootrank = p -> gear_weights_lootrank_link; simplify_html( lootrank );
    std::string wowhead  = p -> gear_weights_wowhead_link;  simplify_html( wowhead  );

    util_t::fprintf( file, "|| *Gear Ranking* || [%s wowhead] || [%s lootrank] ||\n\n", wowhead.c_str(), lootrank.c_str() );
  }

  std::string action_dpet_str       = "empty";
  std::string action_dmg_str        = "empty";
  std::string gains_str             = "empty";
  std::string timeline_resource_str = "empty";
  std::string timeline_dps_str      = "empty";
  std::string distribution_dps_str  = "empty";

  if ( ! p -> action_dpet_chart.empty() )
  {
    action_dpet_str = p -> action_dpet_chart;
    action_dpet_str += "&dummy=dummy.png";
    simplify_html( action_dpet_str );
  }
  if ( ! p -> action_dmg_chart.empty() )
  {
    action_dmg_str = p -> action_dmg_chart.c_str();
    action_dmg_str += "&dummy=dummy.png";
    simplify_html( action_dmg_str );
  }
  if ( ! p -> gains_chart.empty() )
  {
    gains_str = p -> gains_chart.c_str();
    gains_str += "&dummy=dummy.png";
    simplify_html( gains_str );
  }
  if ( ! p -> timeline_resource_chart.empty() )
  {
    timeline_resource_str = p -> timeline_resource_chart.c_str();
    timeline_resource_str += "&dummy=dummy.png";
    simplify_html( timeline_resource_str );
  }
  if ( ! p -> timeline_dps_chart.empty() )
  {
    timeline_dps_str = p -> timeline_dps_chart.c_str();
    timeline_dps_str += "&dummy=dummy.png";
    simplify_html( timeline_dps_str );
  }
  if ( ! p -> distribution_dps_chart.empty() )
  {
    distribution_dps_str = p -> distribution_dps_chart;
    distribution_dps_str += "&dummy=dummy.png";
    simplify_html( distribution_dps_str );
  }

  util_t::fprintf( file, 
		   "|| %s || %s ||\n"
		   "|| %s || %s ||\n"
		   "|| %s || %s ||\n"
		   "\n",
		   action_dpet_str.c_str(), action_dmg_str.c_str(),
		   gains_str.c_str(), timeline_resource_str.c_str(),
		   timeline_dps_str.c_str(), distribution_dps_str.c_str() );


  util_t::fprintf( file, 
		   "||*Ability*||*DPS*||*DPS%%*||*Count*||*Interval*||*DPE*||*DPET*||*DPR*||*Hit*||*Crit*||*Max*||*Crit%%*"
		   "||*M%%*||*D%%*||*P%%*||*G%%*||*Ticks*||*T-Hit*||*T-Crit*||*T-Crit%%*||*T-M%%*||\n" );

  util_t::fprintf( file, "|| *!%s* || *%.0f ||\n", p -> name(), p -> dps );

  for ( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    if ( s -> total_dmg > 0 )
    {
      print_wiki_action( file, s );
    }
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    bool first=true;

    for ( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if ( s -> total_dmg > 0 )
      {
        if ( first )
        {
          first = false;
	  util_t::fprintf( file, "|| *pet - %s* || *%.0f* ||\n", pet -> name_str.c_str(), pet -> dps );
        }
        print_wiki_action( file, s );
      }
    }
  }

  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| *Dynamic Buffs* || **Start* || *Refresh* || *Interval* || *Trigger* || *Up-Time* || *Benefit* ||\n" );
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || b -> constant )
      continue;

    util_t::fprintf( file, "|| %s || %.1f || %.1f || %.1fsec || %.1fsec || %.0f%% || %.0f%% ||\n", 
		     b -> name(), b -> avg_start, b -> avg_refresh, 
		     b -> avg_start_interval, b -> avg_trigger_interval, 
		     b -> uptime_pct, b -> benefit_pct > 0 ? b -> benefit_pct : b -> uptime_pct );
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| *Constant Buffs* ||\n" );
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || ! b -> constant )
      continue;

    util_t::fprintf( file, "|| %s ||\n", b -> name() );
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| *Up-Times* || *%%* ||\n" );
  for ( uptime_t* u = p -> uptime_list; u; u = u -> next )
  {
    if ( u -> percentage() > 0 ) 
    {
      util_t::fprintf( file, "|| %s || %.1f%% ||\n", u -> name(), u -> percentage() );
    }
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| *Procs* || *Count* || *Interval* ||\n" );
  for ( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
  {
    if ( proc -> count > 0 )
    {
      util_t::fprintf( file, "|| %s || %.1f || %.1fsec ||\n", proc -> name(), proc -> count, proc -> frequency );
    }
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| *Gains* || *%s* || *Overflow* ||\n", util_t::resource_type_string( p -> primary_resource() ) );
  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 )
    {
      double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
      util_t::fprintf( file, "|| %s || %.1f || %.1f%% ||\n", g -> name(), g -> actual, overflow_pct );
    }
  }
  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    if ( pet -> total_dmg <= 0 ) continue;
    bool first = true;
    for ( gain_t* g = pet -> gain_list; g; g = g -> next )
    {
      if ( g -> actual > 0 )
      {
	if ( first )
	{
	  first = false;
	  util_t::fprintf( file, "|| *pet - %s* || *%s* ||\n", pet -> name_str.c_str(), util_t::resource_type_string( pet -> primary_resource() ) );
	}
	double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
	util_t::fprintf( file, "|| %s || %.1f || %.1f%% ||\n", g -> name(), g -> actual, overflow_pct );
      }
    }
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| || *Action Priority List* ||\n" );
  std::vector<std::string> action_names;
  int num_actions = util_t::string_split( action_names, p -> action_list_str, "/" );
  for ( int i=0; i < num_actions; i++ )
  {
    util_t::fprintf( file, "|| *%d* || %s ||\n", i, action_names[ i ].c_str() );
  }
  util_t::fprintf( file, "\n" );

  util_t::fprintf( file, "|| *Stat* || *Raid-Buffed* || *Un-Buffed* || *Gear Amount* ||\n" );

  util_t::fprintf( file, "|| *Strength*  || %.0f || %.0f || %.0f ||\n", p -> attribute_buffed[ ATTR_STRENGTH  ], p -> strength(),  p -> stats.attribute[ ATTR_STRENGTH  ] );
  util_t::fprintf( file, "|| *Agility*   || %.0f || %.0f || %.0f ||\n", p -> attribute_buffed[ ATTR_AGILITY   ], p -> agility(),   p -> stats.attribute[ ATTR_AGILITY   ] );
  util_t::fprintf( file, "|| *Stamina*   || %.0f || %.0f || %.0f ||\n", p -> attribute_buffed[ ATTR_STAMINA   ], p -> stamina(),   p -> stats.attribute[ ATTR_STAMINA   ] );
  util_t::fprintf( file, "|| *Intellect* || %.0f || %.0f || %.0f ||\n", p -> attribute_buffed[ ATTR_INTELLECT ], p -> intellect(), p -> stats.attribute[ ATTR_INTELLECT ] );
  util_t::fprintf( file, "|| *Spirit*    || %.0f || %.0f || %.0f ||\n", p -> attribute_buffed[ ATTR_SPIRIT    ], p -> spirit(),    p -> stats.attribute[ ATTR_SPIRIT    ] );
  util_t::fprintf( file, "|| *Health*    || %.0f || %.0f || %.0f ||\n", p -> resource_buffed[ RESOURCE_HEALTH ], p -> resource_max[ RESOURCE_HEALTH ], 0.0 );
  util_t::fprintf( file, "|| *Mana*      || %.0f || %.0f || %.0f ||\n", p -> resource_buffed[ RESOURCE_MANA   ], p -> resource_max[ RESOURCE_MANA   ], 0.0 );

  util_t::fprintf( file, "|| *Spell Power*       || %.0f || %.0f || %.0f ||\n",     p -> buffed_spell_power, p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier(), p -> stats.spell_power );
  util_t::fprintf( file, "|| *Spell Hit*         || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_spell_hit,          100 * p -> composite_spell_hit(),          p -> stats.hit_rating  );
  util_t::fprintf( file, "|| *Spell Crit*        || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_spell_crit,         100 * p -> composite_spell_crit(),         p -> stats.crit_rating );
  util_t::fprintf( file, "|| *Spell Haste*       || %.2f%% || %.2f%% || %.0f ||\n", 100 * ( 1 / p -> buffed_spell_haste - 1 ), 100 * ( 1 / p -> spell_haste - 1 ), p -> stats.haste_rating );
  util_t::fprintf( file, "|| *Spell Penetration* || %.0f || %.0f || %.0f ||\n",     100 * p -> buffed_spell_penetration,  100 * p -> composite_spell_penetration(),  p -> stats.spell_penetration );
  util_t::fprintf( file, "|| *Mana Per 5*        || %.0f || %.0f || %.0f ||\n",     p -> buffed_mp5, p -> composite_mp5(), p -> stats.mp5 );

  util_t::fprintf( file, "|| *Attack Power*      || %.0f || %.0f || %.0f ||\n",     p -> buffed_attack_power, p -> composite_attack_power() * p -> composite_attack_power_multiplier(), p -> stats.attack_power );
  util_t::fprintf( file, "|| *Melee Hit*         || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_attack_hit,         100 * p -> composite_attack_hit(),         p -> stats.hit_rating );
  util_t::fprintf( file, "|| *Melee Crit*        || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_attack_crit,        100 * p -> composite_attack_crit(),        p -> stats.crit_rating );
  util_t::fprintf( file, "|| *Melee Haste*       || %.2f%% || %.2f%% || %.0f ||\n", 100 * ( 1 / p -> buffed_attack_haste - 1 ), 100 * ( 1 / p -> attack_haste - 1 ), p -> stats.haste_rating );
  util_t::fprintf( file, "|| *Expertise*         || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_attack_expertise,   100 * p -> composite_attack_expertise(),   p -> stats.expertise_rating );
  util_t::fprintf( file, "|| *Armor Penetration* || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_attack_penetration, 100 * p -> composite_attack_penetration(), p -> stats.armor_penetration_rating );

  util_t::fprintf( file, "|| *Armor*       || %.0f || %.0f || %.0f ||\n",     p -> buffed_armor,       p -> composite_armor(), ( p -> stats.armor + p -> stats.bonus_armor ) );
  util_t::fprintf( file, "|| *Block Value* || %.0f || %.0f || %.0f ||\n",     p -> buffed_block_value, p -> composite_block_value(), p -> stats.block_value );
  util_t::fprintf( file, "|| *Defense*     || %.0f || %.0f || %.0f ||\n",     p -> buffed_defense,     p -> composite_defense(), p -> stats.defense_rating );
  util_t::fprintf( file, "|| *Tank-Miss*   || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_miss,  100 * ( p -> composite_tank_miss( SCHOOL_PHYSICAL ) - p -> diminished_miss( SCHOOL_PHYSICAL ) ), 0.0  );
  util_t::fprintf( file, "|| *Tank-Dodge*  || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_dodge, 100 * ( p -> composite_tank_dodge() - p -> diminished_dodge() ), p -> stats.dodge_rating );
  util_t::fprintf( file, "|| *Tank-Parry*  || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_parry, 100 * ( p -> composite_tank_parry() - p -> diminished_parry() ), p -> stats.parry_rating );
  util_t::fprintf( file, "|| *Tank-Block*  || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_block, 100 * p -> composite_tank_block(), p -> stats.block_rating );
  util_t::fprintf( file, "|| *Tank-Crit*   || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_crit,  100 * p -> composite_tank_crit( SCHOOL_PHYSICAL ), 0.0 );

  util_t::fprintf( file, "\n" );

  if ( p -> sim -> scaling -> has_scale_factors() )
  {
    util_t::fprintf( file, "|| *Pawn Standard*      || %s ||\n", p -> gear_weights_pawn_std_string.c_str() );
    util_t::fprintf( file, "|| *Zero Hit/Expertise* || %s ||\n", p -> gear_weights_pawn_alt_string.c_str() );
    util_t::fprintf( file, "\n" );
  }

  util_t::fprintf( file, "\n----\n\n<br>\n\n" );
}

// print_wiki_legend =========================================================

static void print_wiki_legend( FILE* file )
{
  util_t::fprintf( file, "= Legend =\n" );
  util_t::fprintf( file, " * *Origin*: link to player profile from which the simulation script was generated\n" );
  util_t::fprintf( file, " * *DPS*: average damage-per-second\n" );
  util_t::fprintf( file, " * *Error*: ( 2 x dps_stddev / sqrt( iterations ) ) / dps_avg \n" );
  util_t::fprintf( file, " * *Range*: ( dps_max - dps_min ) / ( 2 x dps_avg )\n" );
  util_t::fprintf( file, " * *DPR*: average damager-per-resource\n" );
  util_t::fprintf( file, " * *RPS-Out*: resource-per-second being consumed\n" );
  util_t::fprintf( file, " * *RPS-In*: resource-per-second being generated\n" );
  util_t::fprintf( file, " * *Waiting*: percentage of player time waiting for something in the action priority list to be ready, non-zero values are often due to resource constraints\n" );
  util_t::fprintf( file, " * *ApM*: average number of actions executed per minute\n" );
  util_t::fprintf( file, " * *DPS%%*: percentage of total dps contributed by one particular action\n" );
  util_t::fprintf( file, " * *Count*: average number of times this action is executed per iteration\n" );
  util_t::fprintf( file, " * *Interval*: average time between executions of a particular action\n" );
  util_t::fprintf( file, " * *DPE*: average damage-per-execute\n" );
  util_t::fprintf( file, " * *DPET*: average damage-per-execute-time, the amount of damage generated divided by the time taken to execute the action, including time spent in the GCD\n" );
  util_t::fprintf( file, " * *Hit*: average non-crit damage\n" );
  util_t::fprintf( file, " * *Crit*: average crit damage\n" );
  util_t::fprintf( file, " * *Max*: max crit damage over all iterations\n" );
  util_t::fprintf( file, " * *Crit%%*: percentage of executes that resulted in critical strikes\n" );
  util_t::fprintf( file, " * *M%%*: percentage of executes that resulted in misses\n" );
  util_t::fprintf( file, " * *D%%*: percentage of executes that resulted in dodges\n" );
  util_t::fprintf( file, " * *M%%*: percentage of executes that resulted in parrys\n" );
  util_t::fprintf( file, " * *M%%*: percentage of executes that resulted in glances\n" );
  util_t::fprintf( file, " * *Ticks*: average number of ticks per iteration\n" );
  util_t::fprintf( file, " * *T-Hit*: average non-crit tick damage\n" );
  util_t::fprintf( file, " * *T-Crit*: average crit tick damage\n" );
  util_t::fprintf( file, " * *T-Crit%%*: percentage of ticks that resulted in critical strikes\n" );
  util_t::fprintf( file, " * *T-M%%*: percentage of ticks that resulted in misses\n" );
  util_t::fprintf( file, " * *Constant Buffs*: buffs received prior to combat and present the entire fight\n" );
  util_t::fprintf( file, " * *Dynamic Buffs*: temporary buffs received during combat, perhaps multiple times\n" );
}

// print_xml_raid ===========================================================

static void print_xml_raid( FILE*  file, sim_t* sim )
{
  assert( sim ->  dps_charts.size() == sim -> gear_charts.size() );

  int count = ( int ) sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file, "    <chart name=\"DPS Ranking\" url=\"%s\" />\n", sim -> dps_charts[ i ].c_str() );

    util_t::fprintf( file, "    <chart name=\"Gear Overview\" url=\"%s\" />\n", sim -> gear_charts[ i ].c_str() );
  }

  if ( ! sim -> downtime_chart.empty() )
  {
    util_t::fprintf( file, "    <chart name=\"Raid Downtime\" url=\"%s\" />\n", sim -> downtime_chart.c_str() );
  }

  count = ( int ) sim -> dpet_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file, "    <chart name=\"Raid Damage Per Execute Time\" url=\"%s\" />\n", sim -> dpet_charts[ i ].c_str() );
  }
}

// print_xml_player =========================================================

static void print_xml_player( FILE* file, player_t* p )
{
  if ( ! p -> action_dpet_chart.empty() )
  {
    util_t::fprintf( file, "      <chart name=\"Damage Per Execute Time\" type=\"chart_dpet\" url=\"%s\" />\n", p -> action_dpet_chart.c_str() );
  }

  if ( ! p -> action_dmg_chart.empty() )
  {
    util_t::fprintf( file, "      <chart name=\"Damage Sources\" type=\"chart_sources\" url=\"%s\" />\n", p -> action_dmg_chart.c_str() );
  }

  if ( ! p -> gains_chart.empty() )
  {
    util_t::fprintf( file, "      <chart name=\"Resource Gains\" type=\"chart_gains\" url=\"%s\" />\n", p -> gains_chart.c_str() );
  }

  util_t::fprintf( file, "      <chart name=\"Resource Timeline\" type=\"chart_resource_timeline\" url=\"%s\" />\n", p -> timeline_resource_chart.c_str() );

  util_t::fprintf( file, "      <chart name=\"DPS Timeline\" type=\"chart_dps_timeline\" url=\"%s\" />\n", p -> timeline_dps_chart.c_str() );

  util_t::fprintf( file, "      <chart name=\"DPS Distribution\" type=\"chart_dps_distribution\" url=\"%s\" />\n", p -> distribution_dps_chart.c_str() );

  if ( ! p -> scaling_dps_chart.empty() )
  {
    util_t::fprintf( file, "      <chart name=\"DPS Scaling\" type=\"chart_dps_scaling\" url=\"%s\" />\n", p -> distribution_dps_chart.c_str() );
  }
}

// print_xml_scale_factors ===================================================

static void print_xml_player_scale_factors( FILE*  file, sim_t* sim, player_t* p )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  if ( sim -> report_precision < 0 )
    sim -> report_precision = 2;

  util_t::fprintf( file, "      <scale_factors>\n" );

  for ( int j=0; j < STAT_MAX; j++ )
  {
    if ( sim -> scaling -> stats.get_stat( j ) != 0 )
    {
      util_t::fprintf( file, "        <scale_factor name=\"%s\" value=\"%.*f\" />\n", util_t::stat_type_abbrev( j ), sim -> report_precision, p -> scaling.get_stat( j ) );
    }
  }

  util_t::fprintf( file, "        <scale_factor name=\"Lag\" value=\"%.*f\" />\n", sim -> report_precision, p -> scaling_lag );
  util_t::fprintf( file, "        <scale_factor_link type=\"lootrank\" url=\"%s\" />\n", p -> gear_weights_lootrank_link.c_str() );
  util_t::fprintf( file, "        <scale_factor_link type=\"wowhead\" url=\"%s\" />\n", p -> gear_weights_wowhead_link.c_str() );
  util_t::fprintf( file, "        <pawn_std_string>%s</pawn_string>\n", p -> gear_weights_pawn_std_string.c_str() );
  util_t::fprintf( file, "        <pawn_alt_string>%s</pawn_string>\n", p -> gear_weights_pawn_std_string.c_str() );

  util_t::fprintf( file, "      </scale_factors>\n" );
}

// print_xml_text ===========================================================

static void print_xml_text( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "    <raw_text>\n<![CDATA[\n" );
  report_t::print_text( file, sim );
  util_t::fprintf( file, "    ]]>\n</raw_text>\n" );
}

} // ANONYMOUS NAMESPACE =====================================================

// ===========================================================================
// Report
// ===========================================================================

// report_t::print_text ======================================================

void report_t::print_text( FILE* file, sim_t* sim, bool detail )
{
  if ( sim -> total_seconds == 0 ) return;

  int num_players = ( int ) sim -> players_by_rank.size();

  if ( detail )
  {
    util_t::fprintf( file, "\nDPS Ranking:\n" );
    util_t::fprintf( file, "%7.0f 100.0%%  Raid\n", sim -> raid_dps );
    for ( int i=0; i < num_players; i++ )
    {
      player_t* p = sim -> players_by_rank[ i ];
      util_t::fprintf( file, "%7.0f  %4.1f%%  %s\n", p -> dps, 100 * p -> total_dmg / sim -> total_dmg, p -> name() );
    }
  }

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    util_t::fprintf( file, "\nPlayer: %s %s %s %s %d\n",
                     p -> name(), p -> race_str.c_str(), 
		     util_t::player_type_string( p -> type ),
		     util_t::talent_tree_string( p -> primary_tree() ), p -> level );

    util_t::fprintf( file, "  DPS: %.1f  Error=%.1f  Range=%.0f",
                     p -> dps, p -> dps_error, ( p -> dps_max - p -> dps_min ) / 2.0 );

    if ( p -> rps_loss > 0 )
    {
      util_t::fprintf( file, "  DPR=%.1f  RPS=%.1f/%.1f  (%s)",
                       p -> dpr, p -> rps_loss, p -> rps_gain,
                       util_t::resource_type_string( p -> primary_resource() ) );
    }

    util_t::fprintf( file, "\n" );
    util_t::fprintf( file, "  Origin: %s\n", p -> origin_str.c_str() );

    print_core_stats   ( file, p );
    print_spell_stats  ( file, p );
    print_attack_stats ( file, p );
    print_defense_stats( file, p );
    print_actions      ( file, p );

    if ( detail )
    {
      print_buffs        ( file, p );
      print_uptime       ( file, p );
      print_procs        ( file, p );
      print_gains        ( file, p );
      print_pet_gains    ( file, p );
      print_scale_factors( file, p );
      print_dps_plots    ( file, p );
    }
  }

  if ( detail )
  {
    print_buffs        ( file, sim );
    print_hat_donors   ( file, sim );
    print_waiting      ( file, sim );
    print_performance  ( file, sim );
    print_scale_factors( file, sim );
    print_reference_dps( file, sim );
  }

  util_t::fprintf( file, "\n" );
}

// report_t::print_html ======================================================

void report_t::print_html( sim_t* sim )
{
  int num_players = ( int ) sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> total_seconds == 0 ) return;
  if ( sim -> html_file_str.empty() ) return;

  FILE* file = fopen( sim -> html_file_str.c_str(), "w" );
  if ( ! file )
  {
    sim -> errorf( "Unable to open html file '%s'\n", sim -> html_file_str.c_str() );
    return;
  }

  util_t::fprintf( file, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n" );
  util_t::fprintf( file, "<html>\n" );

  util_t::fprintf( file, "<head>\n" );
  util_t::fprintf( file, "<title>Simulationcraft Results</title>\n" );

  util_t::fprintf( file, "</head>\n" );
  util_t::fprintf( file, "<body>\n" );

  if( ! sim -> error_list.empty() )
  {
    util_t::fprintf( file, "<pre>\n" );
    int num_errors = sim -> error_list.size();
    for( int i=0; i < num_errors; i++ ) util_t::fprintf( file, "%s\n", sim -> error_list[ i ].c_str() );
    util_t::fprintf( file, "</pre>\n" );
  }

  int arch = 0, version = 0, revision = 0;
  sim -> patch.decode( &arch, &version, &revision );
  util_t::fprintf( file, "<h1>SimulationCraft %s.%s for World of Warcraft release %d.%d.%d</h1>\n", SC_MAJOR_VERSION, SC_MINOR_VERSION, arch, version, revision );

  time_t rawtime;
  time ( &rawtime );

  util_t::fprintf( file, "<ul>\n" );
  util_t::fprintf( file, "  <li>Timestamp: %s</li>\n", ctime( &rawtime ) );
  util_t::fprintf( file, "  <li>Iterations: %d</li>\n", sim -> iterations );
  util_t::fprintf( file, "  <li>Fight Length: %.0f</li>\n", sim -> max_time );
  util_t::fprintf( file, "  <li>Smooth RNG: %s</li>\n", ( sim -> smooth_rng ? "true" : "false" ) );
  util_t::fprintf( file, "</ul>\n" );

  util_t::fprintf( file, "<hr />\n" );

  if ( num_players > 1 ) 
  {
    print_html_contents( file, sim );
    print_html_raid_summary( file, sim );
    print_html_scale_factors( file, sim );
    util_t::fprintf( file, "<hr />\n" );
  }

  for ( int i=0; i < num_players; i++ )
  {
    print_html_player( file, sim -> players_by_name[ i ] );
  }

  if ( num_players == 1 ) 
  {
    util_t::fprintf( file, "<img src=\"%s\" /> <br />\n", sim -> timeline_chart.c_str() );
  }

  print_html_auras_debuffs( file, sim );

  util_t::fprintf( file, "</body>\n" );
  util_t::fprintf( file, "</html>" );

  fclose( file );
}

// report_t::print_wiki ======================================================

void report_t::print_wiki( sim_t* sim )
{
  int num_players = ( int ) sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> total_seconds == 0 ) return;
  if ( sim -> wiki_file_str.empty() ) return;

  FILE* file = fopen( sim -> wiki_file_str.c_str(), "w" );
  if ( ! file )
  {
    sim -> errorf( "Unable to open wiki file '%s'\n", sim -> wiki_file_str.c_str() );
    return;
  }

  std::string wiki_name = sim -> wiki_file_str;
  std::string::size_type pos = wiki_name.rfind( ".wiki" );
  if ( pos != std::string::npos ) wiki_name = wiki_name.substr( 0, pos );

  print_wiki_preamble( file, sim );
  print_wiki_contents( file, sim, wiki_name, PLAYER_NONE );
  print_wiki_raid_summary( file, sim );
  print_wiki_scale_factors( file, sim );
  print_wiki_auras_debuffs( file, sim );

  fclose( file );

  FILE* player_files[ PLAYER_MAX ];
  memset( (void*) player_files, 0x00, sizeof( FILE* ) * PLAYER_MAX );
  
  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    file = player_files[ p -> type ];

    if ( ! file )
    {
      std::string type_name = player_type_string( p );
      std::string file_name = wiki_name + type_name + ".wiki";

      file = fopen( file_name.c_str(), "w" );
      if ( ! file )
      {
	util_t::fprintf( stderr, "simulationcraft: Unable to open wiki player file '%s'\n", file_name.c_str() );
	exit( 0 );
      }

      print_wiki_preamble( file, sim );
      print_wiki_contents( file, sim, wiki_name, p -> type );

      player_files[ p -> type ] = file;
    }

    print_wiki_player( file, p );
  }

  for ( int i=0; i < PLAYER_MAX; i++ ) 
  {
    file = player_files[ i ];
    if ( file )
    {
      print_wiki_legend( file );
      fclose ( file );
    }
  }
}

// report_t::print_xml ======================================================

void report_t::print_xml( sim_t* sim )
{
  int num_players = ( int ) sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> total_seconds == 0 ) return;
  if ( sim -> xml_file_str.empty() ) return;

  FILE* file = fopen( sim -> xml_file_str.c_str(), "w" );
  if ( ! file )
  {
    util_t::fprintf( stderr, "simulationcraft: Unable to open xml file '%s'\n", sim -> xml_file_str.c_str() );
    exit( 0 );
  }

  // Start the XML file
  util_t::fprintf( file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
  util_t::fprintf( file, "<xml>\n" );

  // Add the overall raid summary data
  if ( num_players > 1 )
  {
    util_t::fprintf( file, "  <raid_summary>\n" );
    print_xml_raid( file, sim );
    util_t::fprintf( file, "  </raid_summary>\n" );
  }

  // Loop over the players in the simulation, and print each's simulation results
  util_t::fprintf( file, "  <players>\n" );
  for ( int i=0; i < num_players; i++ )
  {
    util_t::fprintf( file, "    <player name=\"%s\" talent_url=\"%s\">\n",
                     sim -> players_by_name[ i ] -> name(),
                     sim -> players_by_name[ i ] -> talents_str.c_str() // TODO: These talent URLs should have their ampersands escaped
                   );

    // Print the player results
    print_xml_player( file, sim -> players_by_name[ i ] );

    // Add the scale factors for the player
    print_xml_player_scale_factors( file, sim, sim -> players_by_name[ i ] );

    util_t::fprintf( file, "    </player>\n" );
  }
  util_t::fprintf( file, "  </players>\n" );


  // Print the final, raw simulation response
  util_t::fprintf( file, "  <simulation_details>\n" );
  print_xml_text( file, sim );
  util_t::fprintf( file, "  </simulation_details>\n" );


  util_t::fprintf( file, "</xml>" );

  fclose( file );
}

// report_t::print_profiles ==================================================

void report_t::print_profiles( sim_t* sim )
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> is_pet() ) continue;

    FILE* file = NULL;

    if ( !p -> save_gear_str.empty() ) // Save gear
    {
      file = fopen( p -> save_gear_str.c_str(), "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save gear profile %s for player %s\n", p -> save_gear_str.c_str(), p -> name() );
      }
      else
      {
	std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_GEAR );
	fprintf( file, "%s", profile_str.c_str() );
        fclose( file );
      }
    }

    if ( !p -> save_talents_str.empty() ) // Save talents
    {
      file = fopen( p -> save_talents_str.c_str(), "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save talents profile %s for player %s\n", p -> save_talents_str.c_str(), p -> name() );
      }
      else
      {
	std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_TALENTS );
	fprintf( file, "%s", profile_str.c_str() );
        fclose( file );
      }
    }

    if ( !p -> save_actions_str.empty() ) // Save actions
    {
      file = fopen( p -> save_actions_str.c_str(), "w" );
      if ( ! file )
      {
        sim -> errorf( "Unable to save actions profile %s for player %s\n", p -> save_actions_str.c_str(), p -> name() );
      }
      else
      {
	std::string profile_str = "";
        p -> create_profile( profile_str, SAVE_ACTIONS );
	fprintf( file, "%s", profile_str.c_str() );
        fclose( file );
      }
    }

    std::string file_name = p -> save_str;

    if ( file_name.empty() && sim -> save_profiles )
    {
      file_name  = "save_";
      file_name += p -> name_str;
      file_name += ".simc";
    }

    if ( file_name.empty() ) continue;

    file = fopen( file_name.c_str(), "w" );
    if ( ! file )
    {
      sim -> errorf( "Unable to save profile %s for player %s\n", file_name.c_str(), p -> name() );
      continue;
    }

    std::string profile_str = "";
    p -> create_profile( profile_str );
    fprintf( file, "%s", profile_str.c_str() );
    fclose( file );
  }
}

// report_t::print_suite =====================================================

void report_t::print_suite( sim_t* sim )
{
  report_t::print_text( sim -> output_file, sim );
  report_t::print_html( sim );
  report_t::print_wiki( sim );
  report_t::print_xml( sim );
  report_t::print_profiles( sim );
}

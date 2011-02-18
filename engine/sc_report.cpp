// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

#if SC_BETA
// beta warning messages
static const char* beta_warnings[] =
{
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  "All classes are supported.",
  "Some class models still need tweaking.",
  "Some class action lists need tweaking.",
  "Some class BiS gear setups need tweaking.",
  "Some trinkets not yet implemented.",
  "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  0
};
#endif

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
  default: break;
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

// encode_html =============================================================

static void encode_html ( std::string& buffer )
{
  for ( std::string::size_type pos = buffer.find( "&", 0 ); pos != std::string::npos; pos = buffer.find( "&", pos ) )
  {
    buffer.replace( pos, 1, "&amp;" );
    pos+=2;
  }
  for ( std::string::size_type pos = buffer.find( "<", 0 ); pos != std::string::npos; pos = buffer.find( "<", pos ) )
  {
    buffer.replace( pos, 1, "&lt;" );
    pos+=2;
  }
  for ( std::string::size_type pos = buffer.find( ">", 0 ); pos != std::string::npos; pos = buffer.find( ">", pos ) )
  {
    buffer.replace( pos, 1, "&gt;" );
    pos+=2;
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

// wiki_chart_url =====================================================

static void wiki_chart_url( std::string& buffer )
{
  for ( std::string::size_type pos = buffer.find( " ", 0 ); pos != std::string::npos; pos = buffer.find( " ", pos ) )
  {
    buffer.replace( pos, 1, "+" );
  }
}

// wiki_player_anchor ========================================================

static std::string wiki_player_anchor( player_t* p )
{
  std::string buffer = wiki_player_reference( p );
  // GoogleCode Wiki is giving me headaches......
  int size = buffer.size();
  if ( isdigit( buffer[ size-1 ] ) || islower( buffer[ size-1 ] ) )
  {
    buffer.insert( 0, "!" );
  }
  return buffer;
}

// print_text_action ==============================================================

static void print_text_action( FILE* file, stats_t* s, int max_name_length=0 )
{
  if ( s -> num_executes == 0 && s -> total_dmg == 0 ) return;

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

  if ( s -> num_direct_results > 0 )
  {
    util_t::fprintf( file, "  Miss=%.2f%%", s -> direct_results[ RESULT_MISS ].pct );
  }

  if ( s -> direct_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file, "  Hit=%4.0f|%4.0f|%4.0f", 
		     s -> direct_results[ RESULT_HIT ].avg_dmg, 
		     s -> direct_results[ RESULT_HIT ].min_dmg, 
		     s -> direct_results[ RESULT_HIT ].max_dmg );
  }
  if ( s -> direct_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Crit=%5.0f|%5.0f|%5.0f|%.1f%%",
                     s -> direct_results[ RESULT_CRIT ].avg_dmg,
                     s -> direct_results[ RESULT_CRIT ].min_dmg,
                     s -> direct_results[ RESULT_CRIT ].max_dmg,
                     s -> direct_results[ RESULT_CRIT ].pct );
  }
  if ( s -> direct_results[ RESULT_GLANCE ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Glance=%4.0f|%.1f%%",
                     s -> direct_results[ RESULT_GLANCE ].avg_dmg,
                     s -> direct_results[ RESULT_GLANCE ].pct );
  }
  if ( s -> direct_results[ RESULT_DODGE ].count > 0 )
  {
    util_t::fprintf( file,
                     "  Dodge=%.1f%%",
                     s -> direct_results[ RESULT_DODGE ].pct );
  }
  if ( s -> direct_results[ RESULT_PARRY ].count > 0 )
  {
    util_t::fprintf( file,
                     "  Parry=%.1f%%",
                     s -> direct_results[ RESULT_PARRY ].pct );
  }

  if ( s -> num_ticks > 0 ) util_t::fprintf( file, "  TickCount=%.0f", s -> num_ticks );

  if ( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 || s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file, "  MissTick=%.1f%%", s -> tick_results[ RESULT_MISS ].pct );
  }

  if ( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Tick=%.0f|%.0f|%.0f", 
                     s -> tick_results[ RESULT_HIT ].avg_dmg, 
                     s -> tick_results[ RESULT_HIT ].min_dmg, 
                     s -> tick_results[ RESULT_HIT ].max_dmg );
  }
  if ( s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  CritTick=%.0f|%.0f|%.0f|%.1f%%",
                     s -> tick_results[ RESULT_CRIT ].avg_dmg,
                     s -> tick_results[ RESULT_CRIT ].min_dmg,
                     s -> tick_results[ RESULT_CRIT ].max_dmg,
                     s -> tick_results[ RESULT_CRIT ].pct );
  }

  if ( s -> total_tick_time > 0 )
  {
    util_t::fprintf( file, "  UpTime=%.1f%%", 100.0 * s -> total_tick_time / s -> player -> total_seconds  );
  }

  util_t::fprintf( file, "\n" );
}

// print_text_actions =============================================================

static void print_text_actions( FILE* file, player_t* p )
{
  if ( ! p -> glyphs_str.empty() ) util_t::fprintf( file, "  Glyphs: %s\n", p -> glyphs_str.c_str() );

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
    if ( s -> num_executes > 1 || s -> compound_dmg > 0 )
    {
      print_text_action( file, s, max_length );
    }
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    bool first=true;
    for ( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if ( s -> num_executes || s -> compound_dmg > 0 )
      {
        if ( first )
        {
          util_t::fprintf( file, "   %s  (DPS=%.1f)\n", pet -> name_str.c_str(), pet -> dps );
          first = false;
        }
        print_text_action( file, s, max_length );
      }
    }
  }
}

// print_text_buffs ===============================================================

static void print_text_buffs( FILE* file, player_t* p )
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
        if ( ! first )
          util_t::fprintf( file, "\n" );
        first=false;
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

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    for ( buff_t* b = pet -> buff_list; b; b = b -> next )
    {
      if ( ! b -> quiet && b -> start_count && ! b -> constant )
      {
        int length = ( int ) strlen( b -> name() ) + ( int ) pet -> name_str.size() + 1;
        if ( length > max_length ) max_length = length;
      }
    }
  }

  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count )
      continue;

    if ( ! b -> constant )
    {
      util_t::fprintf( file, "    %-*s : start=%-4.1f refresh=%-5.1f interval=%5.1f trigger=%-5.1f uptime=%2.0f%%",
                       max_length, b -> name(), b -> avg_start, b -> avg_refresh,
                       b -> avg_start_interval, b -> avg_trigger_interval, b -> uptime_pct );

      if( b -> benefit_pct > 0 &&
          b -> benefit_pct < 100 )
      {
        util_t::fprintf( file, "  benefit=%2.0f%%", b -> benefit_pct );
      }

      util_t::fprintf( file, "\n" );
    }
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    for ( buff_t* b = pet -> buff_list; b; b = b -> next )
    {
      if ( b -> quiet || ! b -> start_count )
        continue;

      if ( ! b -> constant )
      {
        std::string full_name = pet -> name_str + "-" + b -> name_str;

        util_t::fprintf( file, "    %-*s : start=%-4.1f refresh=%-5.1f interval=%5.1f trigger=%-5.1f uptime=%2.0f%%",
                         max_length, full_name.c_str(), b -> avg_start, b -> avg_refresh,
                         b -> avg_start_interval, b -> avg_trigger_interval, b -> uptime_pct );

        if( b -> benefit_pct > 0 &&
            b -> benefit_pct < 100 )
        {
          util_t::fprintf( file, "  benefit=%2.0f%%", b -> benefit_pct );
        }

        util_t::fprintf( file, "\n" );
      }
    }
  }
}

// print_text_buffs ===============================================================

static void print_text_buffs( FILE* file, sim_t* sim )
{
  util_t::fprintf( file, "\nAuras:" );
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

      if( b -> benefit_pct > 0 && b -> benefit_pct < 100 )
      {
        util_t::fprintf( file, "  benefit=%2.0f%%", b -> benefit_pct );
      }

      if ( b -> trigger_pct > 0 && b -> trigger_pct < 100 )
      {
        util_t::fprintf( file, "  trigger=%2.0f%%", b -> trigger_pct );
      }

      util_t::fprintf( file, "\n" );
    }
  }
}

// print_text_core_stats ===========================================================

static void print_text_core_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Core Stats:  strength=%.0f|%.0f(%.0f)  agility=%.0f|%.0f(%.0f)  stamina=%.0f|%.0f(%.0f)  intellect=%.0f|%.0f(%.0f)  spirit=%.0f|%.0f(%.0f)  mastery=%.2f|%.2f(%.0f)  health=%.0f|%.0f  mana=%.0f|%.0f\n",
                   p -> attribute_buffed[ ATTR_STRENGTH  ], p -> strength(),  p -> stats.attribute[ ATTR_STRENGTH  ],
                   p -> attribute_buffed[ ATTR_AGILITY   ], p -> agility(),   p -> stats.attribute[ ATTR_AGILITY   ],
                   p -> attribute_buffed[ ATTR_STAMINA   ], p -> stamina(),   p -> stats.attribute[ ATTR_STAMINA   ],
                   p -> attribute_buffed[ ATTR_INTELLECT ], p -> intellect(), p -> stats.attribute[ ATTR_INTELLECT ],
                   p -> attribute_buffed[ ATTR_SPIRIT    ], p -> spirit(),    p -> stats.attribute[ ATTR_SPIRIT    ],
                   p -> buffed_mastery , p -> composite_mastery(), p -> stats.mastery_rating,
                   p -> resource_buffed[ RESOURCE_HEALTH ], p -> resource_max[ RESOURCE_HEALTH ],
                   p -> resource_buffed[ RESOURCE_MANA   ], p -> resource_max[ RESOURCE_MANA   ] );
}

// print_text_spell_stats ==========================================================

static void print_text_spell_stats( FILE* file, player_t* p )
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

// print_text_attack_stats =========================================================

static void print_text_attack_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Attack Stats  power=%.0f|%.0f(%.0f)  hit=%.2f%%|%.2f%%(%.0f)  crit=%.2f%%|%.2f%%(%.0f)  expertise=%.2f|%.2f(%.0f)  haste=%.2f%%|%.2f%%(%.0f)\n",
                   p -> buffed_attack_power, p -> composite_attack_power() * p -> composite_attack_power_multiplier(), p -> stats.attack_power,
                   100 * p -> buffed_attack_hit,         100 * p -> composite_attack_hit(),         p -> stats.hit_rating,
                   100 * p -> buffed_attack_crit,        100 * p -> composite_attack_crit(),        p -> stats.crit_rating,
                   100 * p -> buffed_attack_expertise,   100 * p -> composite_attack_expertise(),   p -> stats.expertise_rating,
                   100 * ( 1 / p -> buffed_attack_haste - 1 ), 100 * ( 1 / p -> attack_haste - 1 ), p -> stats.haste_rating );
}

// print_text_defense_stats =======================================================

static void print_text_defense_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Defense Stats:  armor=%.0f|%.0f(%.0f) miss=%.2f%%|%.2f%%  dodge=%.2f%%|%.2f%%(%.0f)  parry=%.2f%%|%.2f%%(%.0f)  block=%.2f%%|%.2f%%(%.0f) crit=%.2f%%|%.2f%%\n",
                   p -> buffed_armor,       p -> composite_armor(), ( p -> stats.armor + p -> stats.bonus_armor ),
                   100 * p -> buffed_miss,  100 * ( p -> composite_tank_miss( SCHOOL_PHYSICAL ) ),
                   100 * p -> buffed_dodge, 100 * ( p -> composite_tank_dodge() - p -> diminished_dodge() ), p -> stats.dodge_rating,
                   100 * p -> buffed_parry, 100 * ( p -> composite_tank_parry() - p -> diminished_parry() ), p -> stats.parry_rating,
                   100 * p -> buffed_block, 100 * p -> composite_tank_block(), p -> stats.block_rating,
                   100 * p -> buffed_crit,  100 * p -> composite_tank_crit( SCHOOL_PHYSICAL ) );
}

// print_text_gains ===============================================================

static void print_text_gains( FILE* file, player_t* p )
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
      util_t::fprintf( file, "    %8.1f : %-*s", g -> actual, max_length, g -> name() );
      double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
      if ( overflow_pct > 1.0 ) util_t::fprintf( file, "  (overflow=%.1f%%)", overflow_pct );
      util_t::fprintf( file, "\n" );
    }
  }
}

// print_text_pet_gains ===============================================================

static void print_text_pet_gains( FILE* file, player_t* p )
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
          if ( overflow_pct > 1.0 )
            util_t::fprintf( file, "  (overflow=%.1f%%)", overflow_pct );
          util_t::fprintf( file, "\n" );
        }
      }
    }
  }
}

// print_text_procs ================================================================

static void print_text_procs( FILE* file, player_t* p )
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

// print_text_uptime ===============================================================

static void print_text_uptime( FILE* file, player_t* p )
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

// print_text_waiting ===============================================================

static void print_text_waiting( FILE* file, sim_t* sim )
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

// print_text_performance ==========================================================

static void print_text_performance( FILE* file, sim_t* sim )
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

// print_text_scale_factors ========================================================

static void print_text_scale_factors( FILE* file, sim_t* sim )
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

    if ( sim -> scaling -> scale_lag ) util_t::fprintf( file, "  ms Lag=%.*f", sim -> report_precision, p -> scaling_lag );

    util_t::fprintf( file, "\n" );
  }
}

// print_text_scale_factors ========================================================

static void print_text_scale_factors( FILE* file, player_t* p )
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
  if ( p -> sim -> scaling -> scale_lag ) util_t::fprintf( file, "  ms Lag=%.*f", p -> sim -> report_precision, p -> scaling_lag );

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

// print_text_dps_plots ============================================================

static void print_text_dps_plots( FILE* file, player_t* p )
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

// print_text_reference_dps ========================================================

static void print_text_reference_dps( FILE* file, sim_t* sim )
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
    return( l -> procs.hat_donor -> frequency < r -> procs.hat_donor -> frequency );
  }
};

static void print_text_hat_donors( FILE* file, sim_t* sim )
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

// print_text_player =========================================================

static void print_text_player( FILE* file, sim_t* sim, player_t* p, int j )
{
  util_t::fprintf( file, "\n%s: %s %s %s %s %d\n",
                   p -> is_enemy() ? "Target" : p -> is_add() ? "Add" : "Player",
                   p -> name(), p -> race_str.c_str(),
                   util_t::player_type_string( p -> type ),
                   util_t::talent_tree_string( p -> primary_tree() ), p -> level );

  util_t::fprintf( file, "  DPS: %.1f  Error=%.1f/%.1f%%  Range=%.0f/%.1f%%",
                   p -> dps, p -> dps_error, p -> dps ? p -> dps_error * 100 / p -> dps : 0, ( p -> dps_max - p -> dps_min ) / 2.0 , p -> dps ? ( ( p -> dps_max - p -> dps_min ) / 2 ) * 100 / p -> dps : 0);

  if ( p -> rps_loss > 0 )
  {
    util_t::fprintf( file, "  DPR=%.1f  RPS-Out=%.1f RPS-In=%.1f  Resource=(%s) Waiting=%.1f ApM=%.1f",
                     p -> dpr, p -> rps_loss, p -> rps_gain,
                     util_t::resource_type_string( p -> primary_resource() ), 100.0 * p -> total_waiting / p -> total_seconds, 60.0 * p -> total_foreground_actions / p -> total_seconds  );
  }

  util_t::fprintf( file, "\n" );
  if ( p -> origin_str.compare("unknown") ) util_t::fprintf( file, "  Origin: %s\n", p -> origin_str.c_str() );
  if ( ! p -> talents_str.empty() )util_t::fprintf( file, "  Talents: %s\n",p -> talents_str.c_str() );
  print_text_core_stats   ( file, p );
  print_text_spell_stats  ( file, p );
  print_text_attack_stats ( file, p );
  print_text_defense_stats( file, p );
  print_text_actions      ( file, p );

  print_text_buffs        ( file, p );
  print_text_uptime       ( file, p );
  print_text_procs        ( file, p );
  print_text_gains        ( file, p );
  print_text_pet_gains    ( file, p );
  print_text_scale_factors( file, p );
  print_text_dps_plots    ( file, p );

}

// print_html_contents =======================================================

static void print_html_contents( FILE*  file, sim_t* sim )
{
  int i;         // reusable counter
  double n;      // number of columns
  double c = 2;  // total number of TOC entries
  int pi = 0;    // player counter
  int ci = 0;    // in-column counter
  int cs = 0;    // column size
  int ab = 0;    // auras and debuffs link added yet?
  std::string toc_class; // css class
  char buffer[ 1024 ];

  if ( sim -> scaling -> has_scale_factors() )
  {
    c++;
  }
  int num_players = ( int ) sim -> players_by_name.size();
  c += num_players;
  for ( i=0; i < num_players; i++ )
  {
    if ( sim -> report_pets_separately )
    {
      for ( pet_t* pet = sim -> players_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
      {
        if ( pet -> summoned )
        {
          c++;
        }
      }
    }
  }
  if ( sim -> report_targets )
  {
    c += (int) sim -> targets_by_name.size();
  }

  util_t::fprintf( file,
    "    <div id=\"table-of-contents\" class=\"section section-open\">\n"
    "      <h2 class=\"toggle open\">Table of Contents</h2>\n"
    "      <div class=\"toggle-content\">\n" );

  // set number of columns
  if ( c < 6 )
  {
    n = 1;
    toc_class = "toc-wide";
  }
  else if ( sim -> report_pets_separately )
  {
    n = 2;
    toc_class = "toc-wide";
  }
  else if ( c < 9 )
  {
    n = 2;
    toc_class = "toc-narrow";
  }
  else
  {
    n = 3;
    toc_class = "toc-narrow";
  }
  std::string cols [ 3 ];
  for ( i=0; i < n; i++ )
  {
    if ( i == 0 )
    {
      cs = (int) ceil( 1.0 * c / n);
    }
    else if ( i == 1 )
    {
      if ( n == 2 )
      {
        cs = (int) ( c - ceil( 1.0 * c / n ) );
      }
      else
      {
        cs = (int) ceil( 1.0 * c / n);
      }
    }
    else
    {
      cs = (int) ( c - 2 * ceil( 1.0 * c / n) );
    }
    ci = 1;
    snprintf( buffer, sizeof( buffer ),
      "        <ul class=\"toc %s\">\n",
	  toc_class.c_str() );
	cols[i] += buffer;
    if ( i == 0 )
    {
      cols[i] += "          <li><a href=\"#raid-summary\">Raid Summary</a></li>\n";
      ci++;
      if ( sim -> scaling -> has_scale_factors() )
      {
	    cols[i] += "          <li><a href=\"#raid-scale-factors\">Scale Factors</a></li>\n";
	    ci++;
      }
    }
    while ( ci <= cs )
    {
      if ( pi < (int) sim -> players_by_name.size() )
      {
        player_t* p = sim -> players_by_name[ pi ];
        snprintf( buffer, sizeof( buffer ),
          "          <li><a href=\"#%s\">%s</a>",
	      p -> name(),
	      p -> name() );
	    cols[i] += buffer;
	    ci++; 
        if ( sim -> report_pets_separately )
        {
	      cols[i] += "\n            <ul>\n";
	      for ( pet_t* pet = sim -> players_by_name[ pi ] -> pet_list; pet; pet = pet -> next_pet )
	      {
		    if ( pet -> summoned )
		    {
		      snprintf( buffer, sizeof( buffer ),
		        "              <li><a href=\"#%s\">%s</a></li>\n",
		        pet -> name(),
		        pet -> name() );
		      cols[i] += buffer;
		      ci++;
		    }
	      }
	      cols[i] += "            </ul>\n";
	    }
	    pi++;
	  }
      if ( pi == (int) sim -> players_by_name.size() )
      {
        if ( ab == 0 )
        {
          cols[i] += "            <li><a href=\"#auras\">Auras</a></li>\n";
          ab = 1;
        }
        ci++;
      }
      if ( sim -> report_targets && ab > 0 )
      {
        if ( ab == 1 )
        {
          pi = 0;
          ab = 2;
        }
        while ( ci <= cs )
        {
          if ( pi < (int) sim -> targets_by_name.size() )
          {
            player_t* p = sim -> targets_by_name[ pi ];
            snprintf( buffer, sizeof( buffer ),
              "          <li><a href=\"#%s\">%s</a>",
	          p -> name(),
	          p -> name() );
	        cols[i] += buffer;
	      }
          ci++; 
          pi++;
	    }
	  }
    }
    cols[i] += "          </ul>\n";
  }
  for ( i=0; i < n; i++ )
  {
    util_t::fprintf( file, cols[i].c_str() );
  }

  util_t::fprintf( file,
    "        <div class=\"clear\"></div>\n"
    "      </div>\n\n"
    "    </div>\n\n" );
}

// print_html_raid_summary ===================================================

static void print_html_raid_summary( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file,
    "    <div id=\"raid-summary\" class=\"section section-open\">\n\n" );
  util_t::fprintf( file,
    "      <h2 class=\"toggle open\">Raid Summary</h2>\n" );
  util_t::fprintf( file,
    "      <div class=\"toggle-content\">\n" );

  assert( sim ->  dps_charts.size() ==
          sim -> gear_charts.size() );

  // Left side charts: dps, gear, timeline, raid events
  util_t::fprintf( file,
    "        <div class=\"charts\">\n" );
  int count = ( int ) sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file,
      "          <a href=\"#help-dps\" class=\"help\"><img src=\"%s\" alt=\"DPS Chart\" /></a>\n",
      sim -> dps_charts[ i ].c_str() );
  }
  count = ( int ) sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file,
      "          <img src=\"%s\" alt=\"Gear Chart\" />\n",
      sim -> gear_charts[ i ].c_str() );
  }
  if ( sim -> iterations > 1 )
  {
    util_t::fprintf( file,
      "          <a href=\"#help-timeline-distribution\" class=\"help\"><img src=\"%s\" alt=\"Timeline Distribution Chart\" /></a>\n",
      sim -> timeline_chart.c_str() );
  }
  if ( ! sim -> raid_events_str.empty() )
  {
    util_t::fprintf( file,
      "          <table>\n"
      "            <tr>\n"
      "              <th></th>\n"
      "              <th class=\"left\">Raid Event List</th>\n"
      "            </tr>\n" );
    std::vector<std::string> raid_event_names;
    int num_raid_events = util_t::string_split( raid_event_names, sim -> raid_events_str, "/" );
    for ( int i=0; i < num_raid_events; i++ )
    {
      util_t::fprintf( file,
        "            <tr" );
      if ( ( i & 1 ) )
      {
        util_t::fprintf( file, " class=\"odd\"" );
      }
      util_t::fprintf( file, ">\n" );
      util_t::fprintf( file,
        "              <th class=\"right\">%d</th>\n"
        "              <td class=\"left\">%s</td>\n"
        "            </tr>\n",
        i,
        raid_event_names[ i ].c_str() );
    }
    util_t::fprintf( file,
      "          </table>\n" );
  }
  util_t::fprintf( file,
    "        </div>\n" );

  // Right side charts: dpet
  util_t::fprintf( file,
    "        <div class=\"charts\">\n" );
  count = ( int ) sim -> dpet_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file,
      "          <img src=\"%s\" alt=\"DPET Chart\" />\n",
      sim -> dpet_charts[ i ].c_str() );
  }
  util_t::fprintf( file,
    "        </div>\n" );

  // closure
  util_t::fprintf( file,
    "        <div class=\"clear\"></div>\n"
    "      </div>\n"
    "    </div>\n\n" );

}

// print_html_scale_factors ===================================================

static void print_html_scale_factors( FILE*  file, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  if ( sim -> report_precision < 0 )
    sim -> report_precision = 2;

  util_t::fprintf( file,
    "    <div id=\"raid-scale-factors\" class=\"section grouped-first\">\n\n"
    "      <h2 class=\"toggle\">DPS Scale Factors (dps increase per unit stat)</h2>\n"
    "      <div class=\"toggle-content\">\n" );

  util_t::fprintf( file,
    "        <table class=\"sc\">\n" );

  std::string buffer;
  int num_players = ( int ) sim -> players_by_name.size();
  int prev_type = PLAYER_NONE;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if( p -> type != prev_type )
    {
      prev_type = p -> type;

      util_t::fprintf( file,
        "          <tr>\n"
        "            <th class=\"left\">Profile</th>\n" );
      for ( int i=0; i < STAT_MAX; i++ )
      {
  if ( sim -> scaling -> stats.get_stat( i ) != 0 )
        {
    util_t::fprintf( file,
      "            <th>%s</th>\n",
      util_t::stat_type_abbrev( i ) );
  }
      }
      util_t::fprintf( file,
        "            <th>wowhead</th>\n"
        "            <th>lootrank</th>\n"
        "          </tr>\n" );
    }

    util_t::fprintf( file,
      "          <tr" );
    if ( ( i & 1 ) )
    {
      util_t::fprintf( file, " class=\"odd\"" );
    }
    util_t::fprintf( file, ">\n" );
    util_t::fprintf( file,
      "            <td class=\"left\">%s</td>\n",
      p -> name() );
    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        util_t::fprintf( file,
          "            <td>%.*f</td>\n",
          sim -> report_precision,
          p -> scaling.get_stat( j ) );
      }
    }
    util_t::fprintf( file,
      "            <td><a href=\"%s\"> wowhead </a></td>\n"
      "            <td><a href=\"%s\"> lootrank</a></td>\n"
      "          </tr>\n",
      p -> gear_weights_wowhead_link.c_str(),
      p -> gear_weights_lootrank_link.c_str() );
  }
  util_t::fprintf( file,
    "        </table>\n" );
  if ( sim -> iterations < 10000 )
    util_t::fprintf( file, "<br/><h3>Warning: Scale Factors generated using less than 10,000 iterations will vary from run to run.</h3>");
  util_t::fprintf( file,
    "      </div>\n"
    "    </div>\n\n" );
}

// print_html_auras_debuffs ==================================================

static void print_html_auras_debuffs( FILE*  file, sim_t* sim )
{
  int num_players = ( int ) sim -> players_by_name.size();
  int show_dyn = 0;
  int show_con = 0;
  int i;
  for ( buff_t* b = sim -> buff_list; b; b = b -> next )
  {
    if ( b -> constant )
    {
      show_con++;
    }
    else
    {
      show_dyn++;
    }
  }

  util_t::fprintf( file,
    "        <div id=\"auras\" class=\"section" );
  if ( num_players == 1 )
  {
    util_t::fprintf( file, " grouped-first" );
  }
  if ( !sim -> report_targets )
  {
    util_t::fprintf ( file, " final grouped-last" );
  }
  util_t::fprintf ( file, "\">\n" );
  util_t::fprintf ( file,
    "          <h2 class=\"toggle\">Auras</h2>\n"
    "            <div class=\"toggle-content\">\n" );

  if ( show_dyn > 0 )
  {
    i = 0;
    util_t::fprintf( file,
      "              <table class=\"sc mb\">\n"
      "                <tr>\n"
      "                  <th class=\"left\">Dynamic Buff</th>\n"
      "                  <th>Start</th>\n"
      "                  <th>Refresh</th>\n"
      "                  <th>Interval</th>\n"
      "                  <th>Trigger</th>\n"
      "                  <th>Up-Time</th>\n"
      "                  <th>Benefit</th>\n"
      "                </tr>\n" );
    for ( buff_t* b = sim -> buff_list; b; b = b -> next )
    {
      if ( b -> quiet || ! b -> start_count || b -> constant )
        continue;

      util_t::fprintf( file,
        "                <tr" );
      if ( ( i & 1 ) ) {
        util_t::fprintf( file, " class=\"odd\"" );
      }
      util_t::fprintf( file, ">\n" );
      util_t::fprintf( file,
        "                  <td class=\"left\">%s</td>\n"
        "                  <td class=\"right\">%.1f</td>\n"
        "                  <td class=\"right\">%.1f</td>\n"
        "                  <td class=\"right\">%.1fsec</td>\n"
        "                  <td class=\"right\">%.1fsec</td>\n"
        "                  <td class=\"right\">%.0f%%</td>\n"
        "                  <td class=\"right\">%.0f%%</td>\n"
        "                </tr>\n",
        b -> name(),
        b -> avg_start,
        b -> avg_refresh,
        b -> avg_start_interval,
        b -> avg_trigger_interval,
        b -> uptime_pct,
        b -> benefit_pct > 0 ? b -> benefit_pct : b -> uptime_pct );
      i++;
    }
    util_t::fprintf( file,
      "              </table>\n" );
  }
  
  if ( show_con > 0 )
  {
    i = 0;
    util_t::fprintf( file,
      "              <table class=\"sc\">\n"
      "                <tr>\n"
      "                  <th class=\"left\">Constant Buff</th>\n"
      "                </tr>\n" );
    for ( buff_t* b = sim -> buff_list; b; b = b -> next )
    {
      if ( b -> quiet || ! b -> start_count || ! b -> constant )
        continue;

      util_t::fprintf( file,
        "                <tr class=\"left" );
      if ( ( i & 1 ) ) {
        util_t::fprintf( file, " odd" );
      }
      util_t::fprintf( file, "\">\n" );
      util_t::fprintf( file,
        "                  <td>%s</td>\n"
        "                </tr>\n",
        b -> name() );
      i++;
    }
    util_t::fprintf( file,
      "              </table>\n" );
  }
  
  util_t::fprintf( file,
    "            </div>\n"
    "          </div>\n\n" );

}

// print_html_help_boxes =====================================================

static void print_html_help_boxes( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file,
    "    <!-- Help Boxes -->\n"

    "    <div id=\"help-apm\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>APM</h3>\n"
    "        <p>Average number of actions executed per minute.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-constant-buffs\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Constant Buffs</h3>\n"
    "        <p>Buffs received prior to combat and present the entire fight.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-count\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Count</h3>\n"
    "        <p>Average number of times an action is executed per iteration.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-crit\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Crit</h3>\n"
    "        <p>Average crit damage.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-crit-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Crit%%</h3>\n"
    "        <p>Percentage of executes that resulted in critical strikes.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-dodge-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Dodge%%</h3>\n"
    "        <p>Percentage of executes that resulted in dodges.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-dpe\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>DPE</h3>\n"
    "        <p>Average damage per execution of an individual action.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-dpet\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>DPET</h3>\n"
    "        <p>Average damage per execute time of an individual action; the amount of damage generated, divided by the time taken to execute the action, including time spent in the GCD.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-dpr\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>DPR</h3>\n"
    "        <p>Average damage per resource point spent.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-dps\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>DPS</h3>\n"
    "        <p>Average damage per second.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-dps-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>DPS%%</h3>\n"
    "        <p>Percentage of total DPS contributed by a particular action.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-dynamic-buffs\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Dynamic Buffs</h3>\n"
    "        <p>Temporary buffs received during combat, perhaps multiple times.</p>\n"
    "      </div>\n"
    "    </div>\n"

    // Plain English needed
    "    <div id=\"help-error\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Error</h3>\n"
    "        <p>( 2 * dps_stddev / sqrt( iterations ) ) / dps_avg</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-glance-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>G%%</h3>\n"
    "        <p>Percentage of executes that resulted in glancing blows.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-block-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>G%%</h3>\n"
    "        <p>Percentage of executes that resulted in blocking blows.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-hit\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Hit</h3>\n"
    "        <p>Average non-crit damage.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-interval\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Interval</h3>\n"
    "        <p>Average time between executions of a particular action.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-max\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Max</h3>\n"
    "        <p>Maximum crit damage over all iterations.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-miss-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>M%%</h3>\n"
    "        <p>Percentage of executes that resulted in misses.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-origin\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Origin</h3>\n"
    "        <p>The player profile from which the simulation script was generated. The profile must be copied into the same directory as this HTML file in order for the link to work.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-parry-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Parry%%</h3>\n"
    "        <p>Percentage of executes that resulted in parries.</p>\n"
    "      </div>\n"
    "    </div>\n"

    // Plain English needed
    "    <div id=\"help-range\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Range</h3>\n"
    "        <p>( dps_max - dps_min ) / ( 2 * dps_avg )</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-rps-in\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>RPS In</h3>\n"
    "        <p>Average resource points generated per second.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-rps-out\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>RPS Out</h3>\n"
    "        <p>Average resource points consumed per second.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-scale-factors\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Scale Factors</h3>\n"
    "        <p>DPS gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>DPS loss</b> per unit stat <b>decrease</b>.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-ticks\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Ticks</h3>\n"
    "        <p>Average number of periodic ticks per iteration. Spells that do not have a damage-over-time component will have zero ticks.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-ticks-crit\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>T-Crit</h3>\n"
    "        <p>Average crit tick damage.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-ticks-crit-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>T-Crit%%</h3>\n"
    "        <p>Percentage of ticks that resulted in critical strikes.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-ticks-hit\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>T-Hit</h3>\n"
    "        <p>Average non-crit tick damage.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-ticks-miss-pct\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>T-M%%</h3>\n"
    "        <p>Percentage of ticks that resulted in misses.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-ticks-uptime\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>UpTime%%</h3>\n"
    "        <p>Percentage of total time that DoT is ticking on target.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-timeline-distribution\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Timeline Distribution</h3>\n"
    "        <p>The simulated encounter's duration can vary based on the health of the target and variation in the raid DPS. This chart shows how often the duration of the encounter varied by how much time.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <div id=\"help-waiting\">\n"
    "      <div class=\"help-box\">\n"
    "        <h3>Waiting</h3>\n"
    "        <p>This is the percentage of time in which no action can be taken other than autoattacks. This can be caused by resource starvation, lockouts, and timers.</p>\n"
    "      </div>\n"
    "    </div>\n"

    "    <!-- End Help Boxes -->\n" );
}

// print_html_action =========================================================

static void print_html_action( FILE* file, stats_t* s, player_t* p, int j )
{
  int id = 0;

  util_t::fprintf( file,
    "              <tr" );
  if ( j & 1 )
  {
    util_t::fprintf( file, " class=\"odd\"" );
  }
  util_t::fprintf( file, ">\n" );

  for ( action_t* a = s -> player -> action_list; a; a = a -> next )
  {
    if ( a -> stats != s ) continue;
    id = a -> id;
    if ( ! a -> background ) break;
  }

  util_t::fprintf( file,
    "                <td class=\"left small\">" );
  if ( p -> sim -> report_details )
  util_t::fprintf( file,
    "<a href=\"#\" class=\"toggle-details\" rel=\"spell=%i\">%s</a></td>\n",
    id,
    s -> name_str.c_str() );
  else
  util_t::fprintf( file,
    "%s</td>\n",
    s -> name_str.c_str());
  util_t::fprintf( file,
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f</td>\n"
    "                <td class=\"right small\">%.2fsec</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.1f</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.0f</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "                <td class=\"right small\">%.1f%%</td>\n"
    "              </tr>\n",
    s -> portion_dps,
    s -> portion_dmg * 100,
    s -> resource_portion * 100,
    s -> num_executes,
    s -> frequency,
    s -> dpe,
    s -> dpet,
    s -> dpr,
    s -> rpe,
    s -> direct_results[ RESULT_HIT  ].avg_dmg,
    s -> direct_results[ RESULT_CRIT ].avg_dmg,
    s -> direct_results[ RESULT_CRIT ].max_dmg ? s -> direct_results[ RESULT_CRIT ].max_dmg : s -> direct_results[ RESULT_HIT ].max_dmg,
    s -> direct_results[ RESULT_CRIT ].pct,
    s -> direct_results[ RESULT_MISS ].pct,
    s -> direct_results[ RESULT_DODGE  ].pct,
    s -> direct_results[ RESULT_PARRY  ].pct,
    s -> direct_results[ RESULT_GLANCE ].pct,
    s -> direct_results[ RESULT_BLOCK  ].pct,
    s -> num_ticks,
    s -> tick_results[ RESULT_HIT  ].avg_dmg,
    s -> tick_results[ RESULT_CRIT ].avg_dmg,
    s -> tick_results[ RESULT_CRIT ].pct,
    s -> tick_results[ RESULT_MISS ].pct,
    100 * s -> total_tick_time / s -> player -> total_seconds );

  if ( p -> sim -> report_details )
  {
  util_t::fprintf( file,
    "              <tr class=\"details hide\">\n"
    "                <td colspan=\"25\" class=\"filler\">\n" );

  // Stat Details
  util_t::fprintf (file,
    "                  <h4>Stats details: %s </h4>\n", s -> name_str.c_str() );

  util_t::fprintf (file,
    "                  <div class=\"float\">\n"
    "                    <ul>\n" );
  util_t::fprintf (file,
      "                      <li><span class=\"label\">executes:</span>%.2f</li>\n"
      "                      <li><span class=\"label\">direct results:</span>%.2f</li>\n"
      "                      <li><span class=\"label\">ticks:</span>%.2f</li>\n"
      "                      <li><span class=\"label\">tick results:</span>%.2f</li>\n",
      s -> num_executes,
      s -> num_direct_results,
      s -> num_ticks,
      s -> num_tick_results );
  util_t::fprintf (file,
    "                    </ul></div>\n" );

  util_t::fprintf (file,
    "                  <div class=\"clear\"></div>\n" );

  util_t::fprintf (file,
    "                  <table class=\"sc\">\n");
  if ( s -> num_direct_results > 0 )
  {
  // Direct Damage
  util_t::fprintf (file,
    "                    <tr>\n"
    "                      <th class=\"small\">Direct Results</th>\n"
    "                      <th class=\"small\">Count</th>\n"
    "                      <th class=\"small\">Pct</th>\n"
    "                      <th class=\"small\">Average</th>\n"
    "                      <th class=\"small\">Min</th>\n"
    "                      <th class=\"small\">Max</th>\n"
    "                      <th class=\"small\">Total Damage</th>\n"
    "                    </tr>\n" );
  for ( int i=RESULT_MAX-1; i >= RESULT_NONE; i-- )
  {
  if ( s -> direct_results[ i  ].count)
  {
    util_t::fprintf( file,
    "                    <tr" );
    if ( i & 1 )
    {
      util_t::fprintf( file, " class=\"odd\"" );
    }
    util_t::fprintf( file, ">\n" );
    util_t::fprintf (file,
    "                      <td class=\"left small\">%s</td>\n"
    "                      <td class=\"right small\">%.1f</td>\n"
    "                      <td class=\"right small\">%.2f%%</td>\n"
    "                      <td class=\"right small\">%.2f</td>\n"
    "                      <td class=\"right small\">%.0f</td>\n"
    "                      <td class=\"right small\">%.0f</td>\n"
    "                      <td class=\"right small\">%.0f</td>\n"
    "                    </tr>\n",
    util_t::result_type_string( i ),
    s -> direct_results[ i  ].count,
    s -> direct_results[ i  ].pct,
    s -> direct_results[ i  ].avg_dmg,
    s -> direct_results[ i  ].min_dmg,
    s -> direct_results[ i  ].max_dmg,
    s -> direct_results[ i  ].total_dmg);
  }
  }

  }

  if ( s -> num_tick_results > 0 )
  {
    // Tick Damage
    util_t::fprintf (file,
      "                    <tr>\n"
      "                      <th class=\"small\">Tick Results</th>\n"
      "                      <th class=\"small\">Count</th>\n"
      "                      <th class=\"small\">Pct</th>\n"
      "                      <th class=\"small\">Average</th>\n"
      "                      <th class=\"small\">Min</th>\n"
      "                      <th class=\"small\">Max</th>\n"
      "                      <th class=\"small\">Total Damage</th>\n"
      "                    </tr>\n" );
    for ( int i=RESULT_MAX-1; i >= RESULT_NONE; i-- )
    {
    if ( s -> tick_results[ i  ].count)
    {
      util_t::fprintf( file,
      "                    <tr" );
      if ( i & 1 )
      {
        util_t::fprintf( file, " class=\"odd\"" );
      }
      util_t::fprintf( file, ">\n" );
      util_t::fprintf (file,
      "                      <td class=\"left small\">%s</td>\n"
      "                      <td class=\"right small\">%.1f</td>\n"
      "                      <td class=\"right small\">%.2f%%</td>\n"
      "                      <td class=\"right small\">%.2f</td>\n"
      "                      <td class=\"right small\">%.0f</td>\n"
      "                      <td class=\"right small\">%.0f</td>\n"
      "                      <td class=\"right small\">%.0f</td>\n"
      "                    </tr>\n",
      util_t::result_type_string( i ),
      s -> tick_results[ i  ].count,
      s -> tick_results[ i  ].pct,
      s -> tick_results[ i  ].avg_dmg,
      s -> tick_results[ i  ].min_dmg,
      s -> tick_results[ i  ].max_dmg,
      s -> tick_results[ i  ].total_dmg);
    }
    }


  }
  util_t::fprintf (file,
    "                  </table>\n" );

  util_t::fprintf (file,
    "                  <div class=\"clear\"></div>\n" );
  // Action Details
  std::vector<std::string> processed_actions;
  for ( action_t* a = s -> player -> action_list; a; a = a -> next )
  {
    if ( a -> stats != s ) continue;

    bool found = false;
    for ( int i=processed_actions.size()-1; i >= 0 && !found; i-- )
      if ( processed_actions[ i ] == a -> name() )
        found = true;
    if( found ) continue;
    processed_actions.push_back( a -> name() );

    util_t::fprintf (file,
      "                  <h4>Action details: %s </h4>\n", a -> name() );
    util_t::fprintf (file,
      "                  <div class=\"float\">\n"
      "                    <h5>Static Values</h5>\n"
      "                    <ul>\n"
      "                      <li><span class=\"label\">id:</span>%i</li>\n"
      "                      <li><span class=\"label\">school:</span>%s</li>\n"
      "                      <li><span class=\"label\">resource:</span>%s</li>\n"
      "                      <li><span class=\"label\">tree:</span>%s</li>\n"
      "                      <li><span class=\"label\">range:</span>%.1f</li>\n"
      "                      <li><span class=\"label\">travel_speed:</span>%.4f</li>\n"
      "                      <li><span class=\"label\">trigger_gcd:</span>%.4f</li>\n"
      "                      <li><span class=\"label\">base_cost:</span>%.1f</li>\n"
      "                      <li><span class=\"label\">cooldown:</span>%.2f</li>\n"
      "                      <li><span class=\"label\">base_execute_time:</span>%.2f</li>\n"
      "                      <li><span class=\"label\">base_crit:</span>%.2f</li>\n"
      "                      <li><span class=\"label\">target:</span>%s</li>\n"
      "                      <li><span class=\"label\">tooltip:</span><span class=\"tooltip\">%s</span></li>\n"
      "                      <li><span class=\"label\">description:</span><span class=\"tooltip\">%s</span></li>\n"
      "                    </ul>\n"
      "                  </div>\n"
      "                  <div class=\"float\">\n",
      a -> id,
      util_t::school_type_string( a-> school ),
      util_t::resource_type_string( a -> resource ),
      util_t::talent_tree_string( a -> tree ),
      a -> range,
      a -> travel_speed,
      a -> trigger_gcd,
      a -> base_cost,
      a -> cooldown -> duration,
      a -> base_execute_time,
      a -> base_crit,
      a -> target ? a -> target -> name() : "",
      a -> tooltip(),
      a -> desc() );
    if( a -> direct_power_mod || a -> base_dd_min || a -> base_dd_max )
    {
      util_t::fprintf (file,
        "                    <h5>Direct Damage</h5>\n"
        "                    <ul>\n"
        "                      <li><span class=\"label\">may_crit:</span>%s</li>\n"
        "                      <li><span class=\"label\">direct_power_mod:</span>%.6f</li>\n"
        "                      <li><span class=\"label\">base_dd_min:</span>%.2f</li>\n"
        "                      <li><span class=\"label\">base_dd_max:</span>%.2f</li>\n"
        "                    </ul>\n",
        a -> may_crit?"true":"false",
        a -> direct_power_mod,
        a -> base_dd_min,
        a -> base_dd_max );
    }
    if( a -> num_ticks )
    {
      util_t::fprintf (file,
        "                    <h5>Damage Over Time</h5>\n"
        "                    <ul>\n"
        "                      <li><span class=\"label\">tick_may_crit:</span>%s</li>\n"
        "                      <li><span class=\"label\">tick_zero:</span>%s</li>\n"
        "                      <li><span class=\"label\">tick_power_mod:</span>%.6f</li>\n"
        "                      <li><span class=\"label\">base_td:</span>%.2f</li>\n"
        "                      <li><span class=\"label\">num_ticks:</span>%i</li>\n"
        "                      <li><span class=\"label\">base_tick_time:</span>%.2f</li>\n"
        "                      <li><span class=\"label\">hasted_ticks:</span>%s</li>\n"
        "                      <li><span class=\"label\">dot_behavior:</span>%s</li>\n"
        "                    </ul>\n",
        a -> tick_may_crit?"true":"false",
        a -> tick_zero?"true":"false",
        a -> tick_power_mod,
        a -> base_td,
        a -> num_ticks,
        a -> base_tick_time,
        a -> hasted_ticks?"true":"false",
        a -> dot_behavior==DOT_REFRESH?"DOT_REFRESH":a -> dot_behavior==DOT_CLIP?"DOT_CLIP":"DOT_WAIT" );
    }
    if( a -> weapon )
    {
      util_t::fprintf (file,
        "                    <h5>Weapon</h5>\n"
        "                    <ul>\n"
        "                      <li><span class=\"label\">normalized:</span>%s</li>\n"
        "                      <li><span class=\"label\">weapon_power_mod:</span>%.6f</li>\n"
        "                      <li><span class=\"label\">weapon_multiplier:</span>%.2f</li>\n"
        "                    </ul>\n",
        a -> normalize_weapon_speed ? "true" : "false",
        a -> weapon_power_mod,
        a -> weapon_multiplier );
    }
    util_t::fprintf (file,
      "                  </div>\n"
      "                  <div class=\"clear\"></div>\n" );
  }


  util_t::fprintf( file,
    "                </td>\n"
    "              </tr>\n" );
  }
}

// print_html_gear ============================================================

static void print_html_gear (FILE* file, player_t* a )
{
  if ( a -> total_seconds > 0 )
  {
    util_t::fprintf( file,
      "            <div class=\"player-section gear\">\n"
      "              <h3 class=\"toggle\">Gear</h3>\n"
      "              <div class=\"toggle-content\">\n"
      "                <table class=\"sc\">\n"
      "                  <tr>\n"
      "                    <th></th>\n"
      "                    <th>Encoded</th>\n"
      "                  </tr>\n" );

    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = a -> items[ i ];

      util_t::fprintf( file,
        "                  <tr>\n"
        "                    <th class=\"left\">%s</th>\n"
        "                    <td class=\"left\">%s</td>\n"
        "                  </tr>\n",
        item.slot_name(),
        item.active() ? item.options_str.c_str() : "empty" );
    }

    util_t::fprintf( file,
      "                </table>\n"
      "              </div>\n"
      "            </div>\n" );
  }
}

// print_html_profile ============================================================

static void print_html_profile (FILE* file, player_t* a )
{
  if ( a -> total_seconds > 0 )
  {
    std::string profile_str;
    a -> create_profile( profile_str, SAVE_ALL, true );

    util_t::fprintf( file,
      "            <div class=\"player-section profile\">\n"
      "              <h3 class=\"toggle\">Profile</h3>\n"
      "              <div class=\"toggle-content no-wrap\">\n"
      "                <p>%s</p>\n"
      "              </div>\n"
      "            </div>\n",
      profile_str.c_str() );
  }
}


// print_html_stats ============================================================

static void print_html_stats (FILE* file, player_t* a )
{
  std::string n = a -> name();
  util_t::format_text( n, a -> sim -> input_is_utf8 );

  if ( a -> total_seconds > 0 )
  {
    util_t::fprintf( file,
      "            <div class=\"player-section stats\">\n"
      "              <h3 class=\"toggle\">Stats</h3>\n"
      "              <div class=\"toggle-content\">\n"
      "                <table class=\"sc\">\n"
      "                  <tr>\n"
      "                    <th></th>\n"
      "                    <th>Raid-Buffed</th>\n"
      "                    <th>Unbuffed</th>\n"
      "                    <th>Gear Amount</th>\n"
      "                  </tr>\n" );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Strength</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> attribute_buffed[ ATTR_STRENGTH  ],
      a -> strength(),
      a -> stats.attribute[ ATTR_STRENGTH  ] );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Agility</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> attribute_buffed[ ATTR_AGILITY   ],
      a -> agility(),
      a -> stats.attribute[ ATTR_AGILITY   ] );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Stamina</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> attribute_buffed[ ATTR_STAMINA   ],
      a -> stamina(),
      a -> stats.attribute[ ATTR_STAMINA   ] );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Intellect</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> attribute_buffed[ ATTR_INTELLECT ],
      a -> intellect(),
      a -> stats.attribute[ ATTR_INTELLECT ] );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Spirit</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> attribute_buffed[ ATTR_SPIRIT    ],
      a -> spirit(),
      a -> stats.attribute[ ATTR_SPIRIT    ] );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Health</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> resource_buffed[ RESOURCE_HEALTH ],
      a -> resource_max[ RESOURCE_HEALTH ],
      0.0 );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Mana</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> resource_buffed[ RESOURCE_MANA   ],
      a -> resource_max[ RESOURCE_MANA   ],
      0.0 );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Spell Power</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> buffed_spell_power,
      a -> composite_spell_power( SCHOOL_MAX ) * a -> composite_spell_power_multiplier(),
      a -> stats.spell_power );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Spell Hit</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_spell_hit,
      100 * a -> composite_spell_hit(),
      a -> stats.hit_rating  );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Spell Crit</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_spell_crit,
      100 * a -> composite_spell_crit(),
      a -> stats.crit_rating );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Spell Haste</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * ( 1 / a -> buffed_spell_haste - 1 ),
      100 * ( 1 / a -> spell_haste - 1 ),
      a -> stats.haste_rating );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Spell Penetration</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_spell_penetration,
      100 * a -> composite_spell_penetration(),
      a -> stats.spell_penetration );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Mana Per 5</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> buffed_mp5,
      a -> composite_mp5(),
      a -> stats.mp5 );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Attack Power</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> buffed_attack_power,
      a -> composite_attack_power() * a -> composite_attack_power_multiplier(),
      a -> stats.attack_power );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Melee Hit</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_attack_hit,
      100 * a -> composite_attack_hit(),
      a -> stats.hit_rating );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Melee Crit</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_attack_crit,
      100 * a -> composite_attack_crit(),
      a -> stats.crit_rating );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Melee Haste</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * ( 1 / a -> buffed_attack_haste - 1 ),
      100 * ( 1 / a -> attack_haste - 1 ),
      a -> stats.haste_rating );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Expertise</th>\n"
      "                    <td class=\"right\">%.2f</td>\n"
      "                    <td class=\"right\">%.2f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_attack_expertise,
      100 * a -> composite_attack_expertise(),
      a -> stats.expertise_rating );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Armor</th>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> buffed_armor,
      a -> composite_armor(),
       ( a -> stats.armor + a -> stats.bonus_armor ) );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Tank-Miss</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_miss,
      100 * ( a -> composite_tank_miss( SCHOOL_PHYSICAL ) ),
      0.0  );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Tank-Dodge</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_dodge,
      100 * ( a -> composite_tank_dodge() - a -> diminished_dodge() ),
      a -> stats.dodge_rating );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Tank-Parry</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_parry,
      100 * ( a -> composite_tank_parry() - a -> diminished_parry() ),
      a -> stats.parry_rating );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Tank-Block</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_block,
      100 * a -> composite_tank_block(),
      a -> stats.block_rating );

    util_t::fprintf( file,
      "                  <tr>\n"
      "                    <th class=\"left\">Tank-Crit</th>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.2f%%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      100 * a -> buffed_crit,
      100 * a -> composite_tank_crit( SCHOOL_PHYSICAL ),
      0.0 );

    util_t::fprintf( file,
      "                  <tr class=\"odd\">\n"
      "                    <th class=\"left\">Mastery</th>\n"
      "                    <td class=\"right\">%.2f% </td>\n"
      "                    <td class=\"right\">%.2f%</td>\n"
      "                    <td class=\"right\">%.0f</td>\n"
      "                  </tr>\n",
      a -> buffed_mastery,
      a -> composite_mastery(),
      a -> stats.mastery_rating );

    util_t::fprintf( file,
      "                </table>\n"
      "              </div>\n"
      "            </div>\n" );
  }
}


// print_html_talents_player ======================================================

static void print_html_talents( FILE* file, player_t* p )
{
  std::string n = p -> name();
  util_t::format_text( n, p -> sim -> input_is_utf8 );

  if ( p -> total_seconds > 0 )
  {
    util_t::fprintf( file,
      "            <div class=\"player-section talents\">\n"
      "              <h3 class=\"toggle\">Talents</h3>\n"
      "              <div class=\"toggle-content\">\n" );

    for ( int i = 0; i < MAX_TALENT_TREES; i++ )
    {
      int tree_size = p -> talent_trees[ i ].size();

      if ( tree_size == 0 )
        continue;

      util_t::fprintf( file,
          "                <div class=\"float\">\n"
          "                  <table class=\"sc\">\n"
          "                    <tr>\n"
          "                      <th class=\"left\">%s</th>\n"
          "                      <th>Rank</th>\n"
          "                    </tr>\n",
          util_t::talent_tree_string( p -> tree_type[ i ], false ) );



      for ( int j=0; j < tree_size; j++ )
      {
        talent_t* t = p -> talent_trees[ i ][ j ];

        util_t::fprintf( file, "                    <tr%s>\n", ( (j&1) ? " class=\"odd\"" : "" ) );
        util_t::fprintf( file, "                      <td class=\"left\">%s</td>\n", t -> t_data -> name );
        util_t::fprintf( file, "                      <td>%d</td>\n", t -> rank() );
        util_t::fprintf( file, "                    </tr>\n" );
      }
      util_t::fprintf( file,
          "                  </table>\n"
          "                </div>\n" );
    }

    util_t::fprintf( file,
        "                <div class=\"clear\"></div>\n" );

    util_t::fprintf( file,
      "              </div>\n"
      "            </div>\n" );
  }
}

// print_html_player =========================================================

static void print_html_player( FILE* file, sim_t* sim, player_t* p, int j )
{
  char buffer[ 4096 ];
  std::string n = p -> name();
  util_t::format_text( n, sim -> input_is_utf8 );
  int num_players = ( int ) sim -> players_by_name.size();
  int i;
  
  util_t::fprintf( file,
    "    <div id=\"%s\" class=\"player section",
    n.c_str() );
  if ( num_players > 1 && j == 0 && ! sim -> scaling -> has_scale_factors() && p -> type != ENEMY && p -> type != ENEMY_ADD )
  {
    util_t::fprintf( file, " grouped-first" );
  }
  else if (( p -> type == ENEMY || p -> type == ENEMY_ADD ) && j == (int) sim -> targets_by_name.size() - 1 )
  {
    util_t::fprintf( file, " final grouped-last" );
  }
  else if ( num_players == 1 )
  {
    util_t::fprintf( file, " section-open" );
  }
  util_t::fprintf( file, "\">\n" );
  util_t::fprintf( file,
    "      <h2 class=\"toggle" );
  if ( num_players == 1 )
  {
    util_t::fprintf( file, " open" );
  }

  util_t::fprintf( file, "\">%s&nbsp;:&nbsp;%.0fdps</h2>\n",
    n.c_str(),
    p -> dps );
  
  util_t::fprintf( file,
    "      <div class=\"toggle-content\">\n" );

  util_t::fprintf( file,
    "        <ul class=\"params\">\n"
    "          <li><b>Race:</b> %s</li>\n"
    "          <li><b>Class:</b> %s</li>\n",
    p -> race_str.c_str(),
    p -> is_pet() ? util_t::pet_type_string( p -> cast_pet() -> pet_type ) :util_t::player_type_string( p -> type )
    );

  if ( p -> primary_tree() != TREE_NONE )
    util_t::fprintf( file,
    "          <li><b>Tree:</b> %s</li>\n",
    util_t::talent_tree_string( p -> primary_tree() ) );

  util_t::fprintf( file,
    "          <li><b>Level:</b> %d</li>\n"
    "          <li><b>Role:</b> %s</li>\n"
    "        </ul>\n"
    "        <div class=\"clear\"></div>\n",
    p -> level, util_t::role_type_string( p -> primary_role() ) );

  // Main player table
  util_t::fprintf( file,
    "        <div class=\"player-section results-spec-gear mt\">\n" );
  if ( p -> is_pet() )
  {
  util_t::fprintf( file,
    "          <h3 class=\"toggle open\">Results</h3>\n" );
  }
  else
  {
  util_t::fprintf( file,
    "          <h3 class=\"toggle open\">Results, Spec and Gear</h3>\n" );
  }
  util_t::fprintf( file,
    "          <div class=\"toggle-content\">\n"
    "            <table class=\"sc\">\n"
    "              <tr>\n"
    "                <th><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
    "                <th><a href=\"#help-error\" class=\"help\">Error</a></th>\n"
    "                <th><a href=\"#help-range\" class=\"help\">Range</a></th>\n"
    "                <th><a href=\"#help-dpr\" class=\"help\">DPR</a></th>\n"
    "                <th><a href=\"#help-rps-out\" class=\"help\">RPS Out</a></th>\n"
    "                <th><a href=\"#help-rps-in\" class=\"help\">RPS In</a></th>\n"
    "                <th>Resource</th>\n"
    "                <th><a href=\"#help-waiting\" class=\"help\">Waiting</a></th>\n"
    "                <th><a href=\"#help-apm\" class=\"help\">APM</a></th>\n"
    "              </tr>\n"
    "              <tr>\n"
    "                <td>%.1f</td>\n"
    "                <td>%.1f / %.1f%%</td>\n"
    "                <td>%.1f / %.1f%%</td>\n"
    "                <td>%.1f</td>\n"
    "                <td>%.1f</td>\n"
    "                <td>%.1f</td>\n"
    "                <td>%s</td>\n"
    "                <td>%.2f%%</td>\n"
    "                <td>%.1f</td>\n"
    "              </tr>\n"
    "            </table>\n",
    p -> dps,
    p -> dps_error,
    p -> dps ? p -> dps_error * 100 / p -> dps : 0,
    ( ( p -> dps_max - p -> dps_min ) / 2 ),
    p -> dps ? ( ( p -> dps_max - p -> dps_min ) / 2 ) * 100 / p -> dps : 0,
    p -> dpr,
    p -> rps_loss,
    p -> rps_gain,
    util_t::resource_type_string( p -> primary_resource() ),
    p -> total_seconds ? 100.0 * p -> total_waiting / p -> total_seconds : 0,
    p -> total_seconds ? 60.0 * p -> total_foreground_actions / p -> total_seconds : 0 );

  // Spec and gear
  if ( !p -> is_pet() )
  {
    util_t::fprintf( file,
      "            <table class=\"sc mt\">\n" );
    if ( p -> origin_str.compare("unknown") )
    {
      std::string  enc_url = p -> origin_str; encode_html(  enc_url );
      util_t::fprintf( file,
        "              <tr class=\"left\">\n"
        "                <th><a href=\"#help-origin\" class=\"help\">Origin</a></th>\n"
        "                <td><a href=\"%s\" rel=\"_blank\">%s</a></td>\n"
        "              </tr>\n",
        p -> origin_str.c_str(),
        enc_url.c_str() );
    }
    if ( !p -> talents_str.empty() )
    {
      std::string  enc_url = p -> talents_str; encode_html(  enc_url );
      util_t::fprintf( file,
        "              <tr class=\"left\">\n"
        "                <th>Talents</th>\n"
        "                <td><a href=\"%s\" rel=\"_blank\">%s</a></td>\n"
        "              </tr>\n",
        enc_url.c_str(),
        enc_url.c_str() );
    }
    std::vector<std::string> glyph_names;
    int num_glyphs = util_t::string_split( glyph_names, p -> glyphs_str, ",/" );
    if ( num_glyphs )
    {
      util_t::fprintf( file,
        "              <tr class=\"left\">\n"
        "                <th>Glyphs</th>\n"
        "                <td>\n"
        "                  <ul class=\"float\">\n");
      for ( int i=0; i < num_glyphs; i++ )
      {
        util_t::fprintf( file,
          "                    <li>%s</li>\n",
          glyph_names[ i ].c_str() );
      }
      util_t::fprintf( file,
        "                  </ul>\n"
        "                </td>\n"
        "              </tr>\n" );
    }

    util_t::fprintf( file,
      "            </table>\n" );
  }

  // Scale factors
  if ( !p -> is_pet() )
  {
    if ( p -> sim -> scaling -> has_scale_factors() )
    {
      int colspan = 0;
      util_t::fprintf( file,
        "            <table class=\"sc mt\">\n" );
      util_t::fprintf( file,
        "              <tr>\n"
        "                <th><a href=\"#help-scale-factors\" class=\"help\">?</a></th>\n" );
      for ( int i=0; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
        {
          util_t::fprintf( file,
            "                <th>%s</th>\n",
            util_t::stat_type_abbrev( i ) );
          colspan++;
        }
      if ( p -> sim -> scaling -> scale_lag )
      {
        util_t::fprintf( file,
          "                <th>ms Lag</th>\n" );
        colspan++;
    }
      util_t::fprintf( file,
        "              </tr>\n" );
      util_t::fprintf( file,
        "              <tr>\n"
        "                <th class=\"left\">Scale Factors</th>\n" );
      for ( int i=0; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          util_t::fprintf( file,
            "                <td>%.*f</td>\n",
            p -> sim -> report_precision,
            p -> scaling.get_stat( i ) );
      if ( p -> sim -> scaling -> scale_lag )
        util_t::fprintf( file,
          "                <td>%.*f</td>\n",
          p -> sim -> report_precision,
          p -> scaling_lag );
      util_t::fprintf( file,
        "              </tr>\n" );
      util_t::fprintf( file,
        "              <tr>\n"
        "                <th class=\"left\">Normalized</th>\n" );
      for ( int i=0; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          util_t::fprintf( file,
            "                <td>%.*f</td>\n",
            p -> sim -> report_precision,
            p -> normalized_scaling.get_stat( i ) );
      util_t::fprintf( file,
        "              </tr>\n" );
      util_t::fprintf( file,
        "              <tr>\n"
        "                <th class=\"left\">Scale Deltas</th>\n" );
      for ( int i=0; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          util_t::fprintf( file,
            "                <td>%.0f</td>\n",
            p -> sim -> scaling -> stats.get_stat( i ) );
      util_t::fprintf( file,
        "              </tr>\n" );
      util_t::fprintf( file,
        "              <tr class=\"left\">\n"
        "                <th>Gear Ranking</th>\n"
        "                <td colspan=\"%i\" class=\"filler\">\n"
        "                  <ul class=\"float\">\n"
        "                    <li><a href=\"%s\" rel=\"_blank\">wowhead</a></li>\n"
        "                    <li><a href=\"%s\" rel=\"_blank\">lootrank</a></li>\n"
        "                  </ul>\n"
        "                </td>\n"
        "              </tr>\n",
        colspan,
        p -> gear_weights_wowhead_link.c_str(),
        p -> gear_weights_lootrank_link.c_str() );
      util_t::fprintf( file,
      "            </table>\n" );
      if ( sim -> iterations < 10000 )
        util_t::fprintf( file, "<br/><h3>Warning: Scale Factors generated using less than 10,000 iterations will vary from run to run.</h3>");
    }
  }
  util_t::fprintf( file,
    "          </div>\n"
    "        </div>\n" );

  // Check for healer's in the raid
  bool healer_in_the_raid = false;
  for ( player_t* q = sim -> player_list; q; q = q -> next )
  {
    if ( q -> primary_role() == ROLE_HEAL )
    {
      healer_in_the_raid = true;
      break;
    }
  }

  std::string action_dpet_str                     = "";
  std::string action_dmg_str                      = "";
  std::string gains_str                           = "";
  std::string scaling_dps_str                     = "";
  std::string scale_factors_str                   = "";
  std::string timeline_resource_str               = "";
  std::string timeline_resource_health_str        = "";
  std::string timeline_dps_str                    = "";
  std::string distribution_dps_str                = "";
  std::string distribution_encounter_timeline_str = "";

  if ( ! p -> action_dpet_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Action DPET Chart\" />\n", p -> action_dpet_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-action-dpet\" title=\"Action DPET Chart\">%s</span>\n", p -> action_dpet_chart.c_str() );
    }
    action_dpet_str = buffer;
  }
  if ( ! p -> action_dmg_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Action Damage Chart\" />\n", p -> action_dmg_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-action-dmg\" title=\"Action Damage Chart\">%s</span>\n", p -> action_dmg_chart.c_str() );
    }
    action_dmg_str = buffer;
  }
  if ( ! p -> gains_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Resource Gains Chart\" />\n", p -> gains_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-gains\" title=\"Resource Gains Chart\">%s</span>\n", p -> gains_chart.c_str() );
    }
    gains_str = buffer;
  }
  if ( ! p -> scaling_dps_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Scaling DPS Chart\" />\n", p -> scaling_dps_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-scaling-dps\" title=\"Scaling DPS Chart\">%s</span>\n", p -> scaling_dps_chart.c_str() );
    }
    scaling_dps_str = buffer;
  }
  if ( ! p -> timeline_resource_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Resource Timeline Chart\" />\n", p -> timeline_resource_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-timeline-resource\" title=\"Resource Timeline Chart\">%s</span>\n", p -> timeline_resource_chart.c_str() );
    }
    timeline_resource_str = buffer;
  }
  if ( ! p -> timeline_resource_health_chart.empty() && healer_in_the_raid )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Health Timeline Chart\" />\n", p -> timeline_resource_health_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-health-timeline-resource\" title=\"Health Timeline Chart\">%s</span>\n", p -> timeline_resource_health_chart.c_str() );
    }
    timeline_resource_health_str = buffer;
  }
  if ( ! p -> timeline_dps_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"DPS Timeline Chart\" />\n", p -> timeline_dps_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-timeline-dps\" title=\"DPS Timeline Chart\">%s</span>\n", p -> timeline_dps_chart.c_str() );
    }
    timeline_dps_str = buffer;
  }
  if ( ! p -> distribution_dps_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"DPS Distribution Chart\" />\n", p -> distribution_dps_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-distribution-dps\" title=\"DPS Distribution Chart\">%s</span>\n", p -> distribution_dps_chart.c_str() );
    }
    distribution_dps_str = buffer;
  }
  if ( ! p -> scale_factors_chart.empty() )
  {
    if ( num_players == 1)
    {
      snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Scale Factors Chart\" />\n", p -> scale_factors_chart.c_str() );
    }
    else
    {
      snprintf( buffer, sizeof( buffer ), "<span class=\"chart-scale-factors\" title=\"Scale Factors Chart\">%s</span>\n", p -> scale_factors_chart.c_str() );
    }
    scale_factors_str = buffer;
  }
  if ( num_players == 1 )
  {
    snprintf( buffer, sizeof( buffer ), "<img src=\"%s\" alt=\"Encounter Timeline Distribution Chart\" />\n", sim -> timeline_chart.c_str() );
    distribution_encounter_timeline_str = buffer;
  }

  util_t::fprintf( file,
    "        <div class=\"player-section\">\n"
    "          <h3 class=\"toggle open\">Charts</h3>\n"
    "          <div class=\"toggle-content\">\n"
    "            <div class=\"charts\">\n"
    "              %s"
    "              %s"
    "              %s"
    "              %s"
    "            </div>\n"
    "            <div class=\"charts\">\n"
    "              %s"
    "              %s"
    "              %s"
    "              %s"
    "              %s"
    "              %s"
    "            </div>\n"
    "            <div class=\"clear\"></div>\n"
    "          </div>\n"
    "        </div>\n",
    action_dpet_str.c_str(),
    action_dmg_str.c_str(),
    gains_str.c_str(),
    scaling_dps_str.c_str(),
    scale_factors_str.c_str(),
    timeline_resource_str.c_str(),
    timeline_resource_health_str.c_str(),
    timeline_dps_str.c_str(),
    distribution_dps_str.c_str(),
    distribution_encounter_timeline_str.c_str() );

  util_t::fprintf( file,
    "        <div class=\"player-section\">\n"
    "          <h3 class=\"toggle open\">Abilities</h3>\n"
    "          <div class=\"toggle-content\">\n"
    "            <table class=\"sc\">\n"
    "              <tr>\n"
    "                <th class=\"small\"></th>\n"
    "                <th class=\"small\"><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-dps-pct\" class=\"help\">DPS%%</a></th>\n"
    "                <th class=\"small\">Res%%</th>\n"
    "                <th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-dpe\" class=\"help\">DPE</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-dpet\" class=\"help\">DPET</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-dpr\" class=\"help\">DPR</a></th>\n"
    "                <th class=\"small\">RPE</th>\n"
    "                <th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-max\" class=\"help\">Max</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-miss-pct\" class=\"help\">M%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-dodge-pct\" class=\"help\">D%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-parry-pct\" class=\"help\">P%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-glance-pct\" class=\"help\">G%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-block-pct\" class=\"help\">B%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-ticks\" class=\"help\">Ticks</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-ticks-hit\" class=\"help\">T-Hit</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-ticks-crit\" class=\"help\">T-Crit</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-ticks-crit-pct\" class=\"help\">T-Crit%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-ticks-miss-pct\" class=\"help\">T-M%%</a></th>\n"
    "                <th class=\"small\"><a href=\"#help-ticks-uptime\" class=\"help\">Up%%</a></th>\n"
    "              </tr>\n" );

  util_t::fprintf( file,
    "              <tr>\n"
    "                <th class=\"left small\">%s</th>\n"
    "                <th class=\"right small\">%.0f</th>\n"
    "                <td colspan=\"23\" class=\"filler\"></td>\n"
    "              </tr>\n",
    n.c_str(),
    p -> dps );

  i = 0;
  for ( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    if ( s -> num_executes > 1 || s -> compound_dmg > 0 )
    {
      print_html_action( file, s, p, i );
      i++;
    }
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    bool first=true;

    i = 0;
    for ( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if ( s -> num_executes || s -> compound_dmg > 0 )
      {
        if ( first )
        {
          first = false;
    util_t::fprintf( file,
      "              <tr>\n"
      "                <th class=\"left small\">pet - %s</th>\n"
      "                <th class=\"right small\">%.0f</th>\n"
      "                <td colspan=\"23\" class=\"filler\"></td>\n"
      "              </tr>\n",
      pet -> name_str.c_str(),
      pet -> dps );
        }
        print_html_action( file, s, p, i );
        i++;
      }
    }
  }

  util_t::fprintf( file,
    "            </table>\n"
    "          </div>\n"
    "        </div>\n" );

  util_t::fprintf( file,
    "        <div class=\"player-section buffs\">\n"
    "          <h3 class=\"toggle open\">Buffs</h3>\n"
    "          <div class=\"toggle-content\">\n" );

  // Dynamic Buffs table
  util_t::fprintf( file,
    "            <table class=\"sc mb\">\n"
    "              <tr>\n"
    "                <th class=\"left\"><a href=\"#help-dynamic-buffs\" class=\"help\">Dynamic Buffs</a></th>\n"
    "                <th>Start</th>\n"
    "                <th>Refresh</th>\n"
    "                <th>Interval</th>\n"
    "                <th>Trigger</th>\n"
    "                <th>Up-Time</th>\n"
    "                <th>Benefit</th>\n"
    "              </tr>\n",
    n.c_str(),
    n.c_str() );

  std::vector<buff_t*> dynamic_buffs;
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
    if ( ! b -> quiet && b -> start_count && ! b -> constant )
      dynamic_buffs.push_back( b );
  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
    for ( buff_t* b = pet -> buff_list; b; b = b -> next )
      if ( ! b -> quiet && b -> start_count && ! b -> constant )
        dynamic_buffs.push_back( b );

  for ( i=0; i < (int) dynamic_buffs.size(); i++ )
  {
    buff_t* b = dynamic_buffs[ i ];

    std::string buff_name = "";
    if( b -> player -> is_pet() )
    {
      buff_name += b -> player -> name_str + "-";
    }
    buff_name += b -> name();

    util_t::fprintf( file,
      "              <tr" );
    if ( i & 1 )
    {
      util_t::fprintf( file, " class=\"odd\"" );
    }
    util_t::fprintf( file, ">\n" );
    if ( p -> sim -> report_details )
    util_t::fprintf( file,
      "                <td class=\"left\"><a href=\"#\" class=\"toggle-details\">%s</a></td>\n",
      buff_name.c_str() );
    else
    util_t::fprintf( file,
      "                <td class=\"left\">%s</td>\n",
      buff_name.c_str() );
    util_t::fprintf( file,
      "                <td class=\"right\">%.1f</td>\n"
      "                <td class=\"right\">%.1f</td>\n"
      "                <td class=\"right\">%.1fsec</td>\n"
      "                <td class=\"right\">%.1fsec</td>\n"
      "                <td class=\"right\">%.0f%%</td>\n"
      "                <td class=\"right\">%.0f%%</td>\n"
      "              </tr>\n",
      b -> avg_start,
      b -> avg_refresh,
      b -> avg_start_interval,
      b -> avg_trigger_interval,
      b -> uptime_pct,
     ( b -> benefit_pct > 0 ? b -> benefit_pct : b -> uptime_pct ) );

    if ( p -> sim -> report_details )
    {
    util_t::fprintf( file,
      "              <tr class=\"details hide\">\n"
      "                <td colspan=\"7\" class=\"filler\">\n"
      "                  <h4>Database details</h4>\n"
      "                  <ul>\n"
      "                    <li><span class=\"label\">id:</span>%.i</li>\n"
      "                    <li><span class=\"label\">cooldown name:</span>%s</li>\n"
      "                    <li><span class=\"label\">tooltip:</span><span class=\"tooltip-wider\">%s</span></li>\n"
      "                    <li><span class=\"label\">max_stacks:</span>%.i</li>\n"
      "                    <li><span class=\"label\">duration:</span>%.2f</li>\n"
      "                    <li><span class=\"label\">cooldown:</span>%.2f</li>\n"
      "                    <li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
      "                  </ul>\n"
      "                </td>\n"
      "              </tr>\n",
      b -> s_id,
      b -> cooldown -> name_str.c_str(),
      b -> tooltip(),
      b -> max_stack,
      b -> buff_duration,
      b -> cooldown -> duration,
      b -> default_chance * 100 );
    }
  }
  util_t::fprintf( file,
    "            </table>\n" );

  // constant buffs
  if ( !p -> is_pet() )
  {
    util_t::fprintf( file,
      "              <table class=\"sc\">\n"
      "                <tr>\n"
      "                  <th class=\"left\"><a href=\"#help-constant-buffs\" class=\"help\">Constant Buffs</a></th>\n"
      "                </tr>\n" );
    i = 1;
    for ( buff_t* b = p -> buff_list; b; b = b -> next )
    {
      if ( b -> quiet || ! b -> start_count || ! b -> constant )
        continue;

      util_t::fprintf( file,
        "              <tr" );
      if ( !( i & 1 ) )
      {
        util_t::fprintf( file, " class=\"odd\"" );
      }
      util_t::fprintf( file, ">\n" );
      if ( p -> sim -> report_details )
      {
      util_t::fprintf( file,
        "                  <td class=\"left\"><a href=\"#\" class=\"toggle-details\">%s</a></td>\n"
        "                </tr>\n",
        b -> name() );


      util_t::fprintf( file,
        "                <tr class=\"details hide\">\n"
        "                  <td>\n"
        "                    <h4>Database details</h4>\n"
        "                    <ul>\n"
        "                      <li><span class=\"label\">id:</span>%.i</li>\n"
        "                      <li><span class=\"label\">cooldown name:</span>%s</li>\n"
        "                      <li><span class=\"label\">tooltip:</span><span class=\"tooltip\">%s</span></li>\n"
        "                      <li><span class=\"label\">max_stacks:</span>%.i</li>\n"
        "                      <li><span class=\"label\">duration:</span>%.2f</li>\n"
        "                      <li><span class=\"label\">cooldown:</span>%.2f</li>\n"
        "                      <li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
        "                    </ul>\n"
        "                  </td>\n"
        "                </tr>\n",
        b -> s_id,
        b -> cooldown -> name_str.c_str(),
        b -> tooltip(),
        b -> max_stack,
        b -> buff_duration,
        b -> cooldown -> duration,
        b -> default_chance * 100 );
      }
      else
      util_t::fprintf( file,
        "                  <td class=\"left\">%s</td>\n"
        "                </tr>\n",
        b -> name() );

      i++;
    }
    util_t::fprintf( file,
      "              </table>\n" );
  }


  util_t::fprintf( file,
    "            </div>\n"
    "          </div>\n" );

  util_t::fprintf( file,
    "          <div class=\"player-section uptimes\">\n"
    "            <h3 class=\"toggle\">Uptimes</h3>\n"
    "            <div class=\"toggle-content\">\n"
    "              <table class=\"sc\">\n"
    "                <tr>\n"
    "                  <th></th>\n"
    "                  <th>%%</th>\n"
    "                </tr>\n" );
  i = 1;
  for ( uptime_t* u = p -> uptime_list; u; u = u -> next )
  {
    if ( u -> percentage() > 0 )
    {
      util_t::fprintf( file,
        "                <tr" );
      if ( !( i & 1 ) ) {
        util_t::fprintf( file, " class=\"odd\"" );
      }
      util_t::fprintf( file, ">\n" );
      util_t::fprintf( file,
        "                  <td class=\"left\">%s</td>\n"
        "                  <td class=\"right\">%.1f%%</td>\n"
        "                </tr>\n",
        u -> name(),
        u -> percentage() );
      i++;
    }
  }
  util_t::fprintf( file,
    "              </table>\n"
    "            </div>\n"
    "          </div>\n" );

  util_t::fprintf( file,
    "          <div class=\"player-section procs\">\n"
    "            <h3 class=\"toggle\">Procs</h3>\n"
    "            <div class=\"toggle-content\">\n"
    "              <table class=\"sc\">\n"
    "                <tr>\n"
    "                  <th></th>\n"
    "                  <th>Count</th>\n"
    "                  <th>Interval</th>\n"
    "                </tr>\n",
    n.c_str(),
    n.c_str());
  i = 1;
  for ( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
  {
    if ( proc -> count > 0 )
    {
      util_t::fprintf( file,
        "                <tr" );
      if ( !( i & 1 ) ) {
        util_t::fprintf( file, " class=\"odd\"" );
      }
      util_t::fprintf( file, ">\n" );
      util_t::fprintf( file,
        "                  <td class=\"left\">%s</td>\n"
        "                  <td class=\"right\">%.1f</td>\n"
        "                  <td class=\"right\">%.1fsec</td>\n"
        "                </tr>\n",
        proc -> name(),
        proc -> count,
        proc -> frequency );
      i++;
    }
  }
  util_t::fprintf( file,
    "              </table>\n"
    "            </div>\n"
    "          </div>\n" );

  util_t::fprintf( file,
    "          <div class=\"player-section gains\">\n"
    "            <h3 class=\"toggle\">Gains</h3>\n"
    "            <div class=\"toggle-content\">\n"
    "              <table class=\"sc\">\n"
    "                <tr>\n"
    "                  <th></th>\n"
    "                  <th>Count</th>\n"
    "                  <th>%s</th>\n"
    "                  <th>Average</th>\n"
    "                  <th>Overflow</th>\n"
    "                </tr>\n",
    util_t::resource_type_string( p -> primary_resource() ) );
  i = 1;
  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 )
    {
      double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
      util_t::fprintf( file,
        "                <tr" );
      if ( !( i & 1 ) ) {
        util_t::fprintf( file, " class=\"odd\"" );
      }
      util_t::fprintf( file, ">\n" );
      util_t::fprintf( file,
        "                  <td class=\"left\">%s</td>\n"
        "                  <td class=\"right\">%.1f</td>\n"
        "                  <td class=\"right\">%.1f</td>\n"
        "                  <td class=\"right\">%.1f</td>\n"
        "                  <td class=\"right\">%.1f%%</td>\n"
        "                </tr>\n",
        g -> name(),
        g -> count,
        g -> actual,
        g -> actual / g -> count,
        overflow_pct );
      i++;
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
          util_t::fprintf( file,
            "                <tr>\n"
            "                  <th>pet - %s</th>\n"
            "                  <th>%s</th>\n"
            "                </tr>\n",
            pet -> name_str.c_str(),
            util_t::resource_type_string( pet -> primary_resource() ) );
        }
        double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
        util_t::fprintf( file,
          "                <tr>\n"
          "                  <td>%s</td>\n"
          "                  <td>%.1f</td>\n"
          "                  <td>%.1f%%</td>\n"
          "                </tr>\n",
          g -> name(),
          g -> actual,
          overflow_pct );
      }
    }
  }
  util_t::fprintf( file,
    "                </table>\n"
    "              </div>\n"
    "            </div>\n" );

  util_t::fprintf( file,
    "            <div class=\"player-section action-priority-list\">\n"
    "              <h3 class=\"toggle\">Action Priority List</h3>\n"
    "              <div class=\"toggle-content\">\n"
    "                <table class=\"sc\">\n"
    "                  <tr>\n"
    "                    <th class=\"right\">#</th>\n"
    "                    <th class=\"left\">action,conditions</th>\n"
    "                  </tr>\n" );
  i = 1;
  for ( action_t* a = p -> action_list; a; a = a -> next )
  {
    if ( a -> signature_str.empty() || ! a -> marker ) continue;
    util_t::fprintf( file,
      "                <tr" );
    if ( !( i & 1 ) ) {
      util_t::fprintf( file, " class=\"odd\"" );
    }
    util_t::fprintf( file, ">\n" );
    std::string enc_action = a -> signature_str; encode_html( enc_action );
    util_t::fprintf( file,
      "                    <th class=\"right\">%c</th>\n"
      "                    <td class=\"left\">%s</td>\n"
      "                  </tr>\n",
      a -> marker,
      enc_action.c_str() );
    i++;
  }
  util_t::fprintf( file,
    "                </table>\n" );

  if( ! p -> action_sequence.empty() )
  {
    std::string& seq = p -> action_sequence;
    if ( seq.size() > 0 )
    {
      util_t::fprintf( file,
        "                <h4 class=\"mt\">Sample Sequence</h4>\n"
        "                <div class=\"sample-sequence\">%s</div>\n",
        seq.c_str() );
    }
  }

  util_t::fprintf( file,
    "              </div>\n"
    "            </div>\n" );


  print_html_stats( file, p );

  print_html_gear( file, p );

  print_html_talents( file, p );

  print_html_profile( file, p );


  if ( p -> sim -> scaling -> has_scale_factors() && !p -> is_pet() )
  {
    util_t::fprintf( file,
      "            <div class=\"player-section gear-weights\">\n"
      "              <h3 class=\"toggle\">Gear Weights</h3>\n"
      "              <div class=\"toggle-content\">\n"
      "                <table class=\"sc mb\">\n"
      "                  <tr class=\"left\">\n"
      "                    <th>Pawn Standard</th>\n"
      "                    <td>%s</td>\n"
      "                  </tr>\n"
      "                  <tr class=\"left\">\n"
      "                    <th>Zero Hit/Expertise</th>\n"
      "                    <td>%s</td>\n"
      "                  </tr>\n"
      "                </table>\n",
      p -> gear_weights_pawn_std_string.c_str(),
      p -> gear_weights_pawn_alt_string.c_str() );

    std::string rhada_std = p -> gear_weights_pawn_std_string;
    std::string rhada_alt = p -> gear_weights_pawn_alt_string;

    if ( rhada_std.size() > 10 ) rhada_std.replace( 2, 8, "RhadaTip" );
    if ( rhada_alt.size() > 10 ) rhada_alt.replace( 2, 8, "RhadaTip" );

    util_t::fprintf( file,
      "                <table class=\"sc\">\n"
      "                  <tr class=\"left\">\n"
      "                    <th>RhadaTip Standard</th>\n"
      "                    <td>%s</td>\n"
      "                  </tr>\n"
      "                  <tr class=\"left\">\n"
      "                    <th>Zero Hit/Expertise</th>\n"
      "                    <td>%s</td>\n"
      "                  </tr>\n"
      "                </table>\n",
      rhada_std.c_str(),
      rhada_alt.c_str() );
    util_t::fprintf( file,
      "              </div>\n"
      "            </div>\n" );
  }

  util_t::fprintf( file,
    "          </div>\n"
    "        </div>\n\n" );
}



static void print_wiki_beta_message( FILE * file )
{
#if SC_BETA
  util_t::fprintf( file, "\n= Beta Release =\n" );
  int i = 0;
  while ( beta_warnings[ i ] )
  {
    util_t::fprintf( file, " * %s\n", beta_warnings[ i ] );
    i++;
  }
  util_t::fprintf( file, "\n----\n\n<br>\n\n" );
#endif
}

static void print_wiki_raid_events( FILE * file, sim_t* sim )
{
  if ( ! sim -> raid_events_str.empty() )
  {
    util_t::fprintf( file, "\n= Raid Events =\n" );
    util_t::fprintf( file, "|| || *Raid Event List* ||\n" );
    std::vector<std::string> raid_event_names;
    int num_raid_events = util_t::string_split( raid_event_names, sim -> raid_events_str, "/" );
    for ( int i=0; i < num_raid_events; i++ )
    {
      util_t::fprintf( file, "|| *%d* || %s ||\n", i, raid_event_names[ i ].c_str() );
    }
    util_t::fprintf( file, "\n" );
  }
}

// print_wiki_preamble =======================================================

static void print_wiki_preamble( FILE* file, sim_t* sim )
{
  util_t::fprintf( file, "= !SimulationCraft %s-%s for World of Warcraft %s %s (build level %s) =\n", 
                   SC_MAJOR_VERSION, SC_MINOR_VERSION, ( dbc_t::get_ptr() ? "4.0.6" : "4.0.6" ), ( dbc_t::get_ptr() ? "PTR" : "Live" ), dbc_t::build_level() );

  time_t rawtime;
  time ( &rawtime );

  util_t::fprintf( file, " * Timestamp: %s", ctime( &rawtime ) );
  util_t::fprintf( file, " * Iterations: %d\n", sim -> iterations );
  util_t::fprintf( file, " * Fight Length: %.0f\n", sim -> max_time );
  if ( sim -> vary_combat_length > 0.0 )
  {
    util_t::fprintf( file, " * Vary Combat Length: %.2f\n", sim -> vary_combat_length );
  }
  util_t::fprintf( file, " * Smooth RNG: %s\n", ( sim -> smooth_rng ? "true" : "false" ) );

  util_t::fprintf( file, "\n----\n\n<br>\n\n" );
  print_wiki_beta_message( file );
  print_wiki_raid_events( file, sim );
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

    if ( player_type != PLAYER_NONE && player_type != p -> type )
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
  util_t::fprintf( file,
                   "||%s||%.0f||%.1f%%||%.1f||%.2fsec||%.0f||%.0f||%.1f||%.0f||%.0f||%.0f||%.1f%%||%.1f%%||%.1f%%||%.1f%%||%.1f%%||%.0f||%.0f||%.0f||%.1f%%||%.1f%%||\n",
                   s -> name_str.c_str(), s -> portion_dps, s -> portion_dmg * 100,
                   s -> num_executes, s -> frequency,
                   s -> dpe, s -> dpet, s -> dpr,
                   s -> direct_results[ RESULT_HIT  ].avg_dmg,
                   s -> direct_results[ RESULT_CRIT ].avg_dmg,
                   s -> direct_results[ RESULT_CRIT ].max_dmg,
                   s -> direct_results[ RESULT_CRIT ].pct,
                   s -> direct_results[ RESULT_MISS ].pct,
                   s -> direct_results[ RESULT_DODGE  ].pct,
                   s -> direct_results[ RESULT_PARRY  ].pct,
                   s -> direct_results[ RESULT_GLANCE ].pct,
                   s -> num_ticks,
                   s -> tick_results[ RESULT_HIT  ].avg_dmg,
                   s -> tick_results[ RESULT_CRIT ].avg_dmg,
                   s -> tick_results[ RESULT_CRIT ].pct,
                   s -> tick_results[ RESULT_MISS ].pct );
}

// print_wiki_player =========================================================

static void print_wiki_player( FILE* file, player_t* p )
{
  std::string anchor_name = wiki_player_anchor( p );

  util_t::fprintf( file, "= %s =\n", anchor_name.c_str() );

  util_t::fprintf( file,
                   "|| *Name* || *Race* || *Class* || *Tree* || *Level* ||\n"
                   "|| *%s*  || %s     || %s      || %s     || %d      ||\n"
                   "\n",
                   p -> name(), p -> race_str.c_str(),
                   util_t::player_type_string( p -> type ),
                   util_t::talent_tree_string( p -> primary_tree() ), p -> level );

  util_t::fprintf( file,
                   "|| *DPS*  || *Error* || *Range* || *DPR* || *RPS-Out* || *RPS-In* || *Resource* || *Waiting* || *ApM* ||\n"
                   "|| *%.0f* || %.1f / %.1f%%  || %.1f / %.1f%%  || %.1f  || %.1f      || %.1f     || %s         || %.1f%%    || %.1f  ||\n"
                   "\n",
                   p -> dps, p -> dps_error, p -> dps_error * 100 / p -> dps, ( ( p -> dps_max - p -> dps_min ) / 2 ), ( ( p -> dps_max - p -> dps_min ) / 2 ) * 100 / p -> dps,
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
    wiki_chart_url( action_dpet_str );
  }
  if ( ! p -> action_dmg_chart.empty() )
  {
    action_dmg_str = p -> action_dmg_chart.c_str();
    action_dmg_str += "&dummy=dummy.png";
    simplify_html( action_dmg_str );
    wiki_chart_url( action_dmg_str );
  }
  if ( ! p -> gains_chart.empty() )
  {
    gains_str = p -> gains_chart.c_str();
    gains_str += "&dummy=dummy.png";
    simplify_html( gains_str );
    wiki_chart_url( gains_str );
  }
  if ( ! p -> timeline_resource_chart.empty() )
  {
    timeline_resource_str = p -> timeline_resource_chart.c_str();
    timeline_resource_str += "&dummy=dummy.png";
    simplify_html( timeline_resource_str );
    wiki_chart_url( timeline_resource_str );
  }
  if ( ! p -> timeline_dps_chart.empty() )
  {
    timeline_dps_str = p -> timeline_dps_chart.c_str();
    timeline_dps_str += "&dummy=dummy.png";
    simplify_html( timeline_dps_str );
    wiki_chart_url( timeline_dps_str );
  }
  if ( ! p -> distribution_dps_chart.empty() )
  {
    distribution_dps_str = p -> distribution_dps_chart;
    distribution_dps_str += "&dummy=dummy.png";
    simplify_html( distribution_dps_str );
    wiki_chart_url( distribution_dps_str );
  }

  util_t::fprintf( file,
                   "|| %s || %s ||\n"
                   "|| %s || %s ||\n"
                   "|| %s || %s ||\n"
                   "\n",
                   action_dpet_str.c_str(), action_dmg_str.c_str(),
                   timeline_resource_str.c_str(), gains_str.c_str(),
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
  util_t::fprintf( file, "|| *Expertise*         || %.2f || %.2f || %.0f ||\n", 100 * p -> buffed_attack_expertise,   100 * p -> composite_attack_expertise(),   p -> stats.expertise_rating );

  util_t::fprintf( file, "|| *Armor*       || %.0f || %.0f || %.0f ||\n",     p -> buffed_armor,       p -> composite_armor(), ( p -> stats.armor + p -> stats.bonus_armor ) );
  util_t::fprintf( file, "|| *Tank-Miss*   || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_miss,  100 * ( p -> composite_tank_miss( SCHOOL_PHYSICAL ) ), 0.0  );
  util_t::fprintf( file, "|| *Tank-Dodge*  || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_dodge, 100 * ( p -> composite_tank_dodge() - p -> diminished_dodge() ), p -> stats.dodge_rating );
  util_t::fprintf( file, "|| *Tank-Parry*  || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_parry, 100 * ( p -> composite_tank_parry() - p -> diminished_parry() ), p -> stats.parry_rating );
  util_t::fprintf( file, "|| *Tank-Block*  || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_block, 100 * p -> composite_tank_block(), p -> stats.block_rating );
  util_t::fprintf( file, "|| *Tank-Crit*   || %.2f%% || %.2f%% || %.0f ||\n", 100 * p -> buffed_crit,  100 * p -> composite_tank_crit( SCHOOL_PHYSICAL ), 0.0 );

  util_t::fprintf( file, "|| *Mastery*  || %.2f% || %.2f% || %.0f ||\n", p -> buffed_mastery, p -> composite_mastery(), p -> stats.mastery_rating );

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

#if SC_BETA
  util_t::fprintf( file, "\n\n*** Beta Release ***\n" );
  int i = 0;
  while ( beta_warnings[ i ] )
  {
    util_t::fprintf( file, " * %s\n", beta_warnings[ i ] );
    i++;
  }
  util_t::fprintf( file, "\n" );
#endif

  if ( ! sim -> raid_events_str.empty() )
  {
    util_t::fprintf( file, "\n\nRaid Events:\n" );
    std::vector<std::string> raid_event_names;
    int num_raid_events = util_t::string_split( raid_event_names, sim -> raid_events_str, "/" );
    if ( num_raid_events )
      util_t::fprintf( file, "  raid_event=/%s\n", raid_event_names[ 0 ].c_str() );
    for ( int i=1; i < num_raid_events; i++ )
    {
      util_t::fprintf( file, "  raid_event+=/%s\n", raid_event_names[ i ].c_str() );
    }
    util_t::fprintf( file, "\n" );
  }

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

  // Report Players
  for ( int i=0; i < num_players; i++ )
  {
    print_text_player( file, sim, sim -> players_by_name[ i ], i );

    // Pets
    if ( sim -> report_pets_separately )
    {
      for ( pet_t* pet = sim -> players_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
      {
        if ( pet -> summoned )
          print_text_player( file, sim, pet, 1 );
      }
    }
  }

  // Report Targets
  if ( sim -> report_targets )
  {
    util_t::fprintf( file, "\n\n *** Targets *** \n\n" );

    for ( int i=0; i < (int) sim -> targets_by_name.size(); i++ )
    {
      print_text_player( file, sim, sim -> targets_by_name[ i ], i );

      // Adds
      if ( sim -> targets_by_name[i] -> is_enemy() )
      {
        for ( add_t* add = sim -> targets_by_name[ i ] -> cast_target() -> add_list; add; add = add -> next_add )
        {
          if ( add -> summoned )
            print_text_player( file, sim, add, 1 );
        }
      }
    }
  }

  if ( detail )
  {
    print_text_buffs        ( file, sim );
    print_text_hat_donors   ( file, sim );
    print_text_waiting      ( file, sim );
    print_text_performance  ( file, sim );
    print_text_scale_factors( file, sim );
    print_text_reference_dps( file, sim );
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
  
  util_t::fprintf( file,
    "<!DOCTYPE html>\n\n" );
  util_t::fprintf( file,
    "<html>\n\n" );
  
  util_t::fprintf( file,
    "  <head>\n\n" );
  util_t::fprintf( file,
    "    <title>Simulationcraft Results</title>\n\n" );
  util_t::fprintf( file,
    "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n\n");

  // Styles
  // If file is being hosted on simulationcraft.org, link to the local
  // stylesheet; otherwise, embed the styles.
  util_t::fprintf( file,
    "    <style type=\"text/css\" media=\"all\">\n" );
  if ( sim -> hosted_html )
  {
    util_t::fprintf( file,
    "      @import url('http://www.simulationcraft.org/css/styles.css');\n" );
  }
  else
  {
    util_t::fprintf( file,
      "      * { border: none; margin: 0; padding: 0; }\n"
      "      body { padding: 5px 25px 25px 25px; font-family: \"Lucida Grande\", Arial, sans-serif; font-size: 14px; background-color: #f9f9f9; color: #333; text-align: center; }\n"
      "      p { margin-top: 1em; }\n"
      "      h1, h2, h3, h4, h5, h6 { color: #777; margin-top: 1em; margin-bottom: 0.5em; }\n"
      "      h1, h2 { margin: 0; padding: 2px 2px 0 2px; }\n"
      "      h1 { font-size: 24px; }\n"
      "      h2 { font-size: 18px; }\n"
      "      h3 { margin: 0 0 4px 0; font-size: 16px; }\n"
      "      a { color: #666688; text-decoration: none; }\n"
      "      a:hover, a:active { color: #333; }\n"
      "      ul, ol { padding-left: 20px; }\n"
      "      ul.float, ol.float { padding: 0; }\n"
      "      ul.float li, ol.float li { display: inline; float: left; padding-right: 6px; margin-right: 6px; list-style-type: none; border-right: 2px solid #eee; }\n"
      "      .clear { clear: both; }\n"
      "      .hide, .charts span { display: none; }\n"
      "      .float { float: left; }\n"
      "      .no-wrap { max-width: 1200px; outline: 1px solid #ccc; margin: 10px 0 0 16px; padding: 8px; word-wrap: no-wrap; overflow-x: scroll; }\n"
      "      .mt { margin-top: 20px; }\n"
      "      .mb { margin-bottom: 20px; }\n"
      "      .toggle { cursor: pointer; }\n"
      "      h2.toggle { padding-left: 16px; background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAeCAIAAACT/LgdAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD1SURBVHja7JQ9CoQwFIT9LURQG3vBwyh4XsUjWFtb2IqNCmIhkp1dd9dsfIkeYKdKHl+G5CUTvaqqrutM09Tk2rYtiiIrjuOmaeZ5VqBBEADVGWPTNJVlOQwDyYVhmKap4zgGJp7nJUmCpQoOY2Mv+b6PkkDz3IGevQUOeu6VdxrHsSgK27azLOM5AoVwPqCu6wp1ApXJ0G7rjx5oXdd4YrfQtm3xFJdluUYRBFypghb32ve9jCaOJaPpDpC0tFmg8zzn46nq6/rSd2opAo38IHMXrmeOdgWHACKVFx3Y/c7cjys+JkSP9HuLfYR/Dg1icj0EGACcXZ/44V8+SgAAAABJRU5ErkJggg==) 0 -10px no-repeat; }\n"
      "      h2.open { margin-bottom: 10px; background-position: 0 9px; }\n"
      "      h3.toggle { padding-left: 16px; background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAaCAIAAAAMmCo2AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAEfSURBVHjazJPLjkRAGIXbdSM8ACISvWeDNRYeGuteuL2EdMSGWLrOmdExaCO9nLOq+vPV+S9VRTwej6IoGIYhCOK21zzPfd/f73da07TiRxRFbTkQ4zjKsqyqKoFN27ZhGD6fT5ZlV2IYBkVRXNflOI5ESBAEz/NEUYT5lnAcBwQi307L6aZpoiiqqgprSZJwbCF2EFTXdRAENE37vr8SR2jhAPE8vw0eoVORtw/0j6Fpmi7afEFlWeZ5jhu9grqui+M4SZIrCO8Eg86y7JT7LXx5TODSNL3qDhw6eOeOIyBJEuUj6ZY7mRNmAUvQa4Q+EEiHJizLMgzj3AkeMLBte0vsoCULPHRd//NaUK9pmu/EywDCv0M7+CTzmb4EGADS4Lwj+N6gZgAAAABJRU5ErkJggg==) 0 -11px no-repeat; }\n"
      "      h3.open { background-position: 0 7px; }\n"
      "      h4.toggle { margin: 0 0 8px 0; padding-left: 12px; background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAVCAIAAADw0OikAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD8SURBVHjavJHLjkRAGIUbRaxd3oAQ8QouifDSFmysPICNIBZ2EhuJuM6ZMdFR3T3LOYtKqk79/3/qKybLsrZteZ5/3DXPs67rxLbtvu+bprluHMexrqumaZZlMdhM05SmaVVVhBBst20zDMN1XRR822erJEnKsmQYxjRNz/M4jsM5ORsKguD7/r7vqHAc5/Sg3+orDsuyGHGd3OxXsY8/9R92XdfjOH60i6IAODzsvQ0sgApw1I0nAZACVGAAPlEU6WigDaLoEcfxleNN8mEY8Id0c2hZFlmWgyDASlefXhiGqqrS0eApihJFkSRJt0nHj/I877rueNGXAAMAKcaTc/aCM/4AAAAASUVORK5CYII=) 0 -8px no-repeat; }\n"
      "      h4.open { background-position: 0 6px; }\n"
      "      a.toggle-details { margin: 0 0 8px 0; padding-left: 12px; background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAkAAAAXCAYAAADZTWX7AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAADiSURBVHjaYvz//z/DrFmzGBkYGLqBeG5aWtp1BjTACFIEAkCFZ4AUNxC7ARU+RlbEhMT+BMQaQLwOqEESlyIYMIEqlMenCAQsgLiakKILQNwF47AgSfyH0leA2B/o+EfYTOID4gdA7IusAK4IGk7ngNgPqOABut3I1uUDFfzA5kB4YOIDTAxEgOGtiAUY2vlA2hCIf2KRZwXie6AQPwzEFUAsgUURSGMQEzAqQHFmB8R30BS8BWJXoPw2sJuAjNug2Afi+1AFH4A4DCh+GMXhQIEboHQExKeAOAbI3weTAwgwAIZTQ9CyDvuYAAAAAElFTkSuQmCC) 0 4px no-repeat; }\n"
      "      a.open { background-position: 0 -11px; }\n"
      "      td.small a.toggle-details { background-position: 0 2px; }\n"
      "      td.small a.open { background-position: 0 -13px; }\n"
      "      .toggle-content { display: none; }\n"
      "      #active-help, .help-box { display: none; }\n"
      "      #active-help { position: absolute; width: 350px; padding: 3px; background: #fff; z-index: 10; }\n"
      "      #active-help-dynamic { padding: 6px 6px 18px 6px; background: #eeeef5; outline: 1px solid #ddd; font-size: 13px; }\n"
      "      #active-help .close { position: absolute; right: 10px; bottom: 4px; }\n"
      "      .help-box h3 { margin: 0 0 5px 0; font-size: 16px; }\n"
      "      .help-box { border: 1px solid #ccc; background-color: #fff; padding: 10px; }\n"
      "      a.help { cursor: help; }\n"
      "      .section { position: relative; max-width: 1260px; padding: 8px; margin-left: auto; margin-right: auto; margin-bottom: -1px; border: 1px solid #ccc; background-color: #fff; -moz-box-shadow: 4px 4px 4px #bbb; -webkit-box-shadow: 4px 4px 4px #bbb; box-shadow: 4px 4px 4px #bbb; text-align: left; }\n"
      "      .section-open { margin-top: 25px; margin-bottom: 25px; -moz-border-radius: 10px; -khtml-border-radius: 10px; -webkit-border-radius: 10px; border-radius: 10px; }\n"
      "      .grouped-first { -moz-border-radius-topright: 10px; -moz-border-radius-topleft: 10px; -khtml-border-top-right-radius: 10px; -khtml-border-top-left-radius: 10px; -webkit-border-top-right-radius: 10px; -webkit-border-top-left-radius: 10px; border-top-right-radius: 10px; border-top-left-radius: 10px; }\n"
      "      .grouped-last, .final { -moz-border-radius-bottomright: 10px; -moz-border-radius-bottomleft: 10px; -khtml-border-bottom-right-radius: 10px; -khtml-border-bottom-left-radius: 10px; -webkit-border-bottom-right-radius: 10px; -webkit-border-bottom-left-radius: 10px; border-bottom-right-radius: 10px; border-bottom-left-radius: 10px; }\n"
      "      .section .toggle-content { padding: 0 0 20px 16px; }\n"
      "      #raid-summary .toggle-content { padding-bottom: 0px; }\n"
      "      ul.params { padding: 0; }\n"
      "      ul.params li { float: left; padding: 2px 10px 2px 10px; margin-right: 10px; list-style-type: none; background: #eeeef5; font-family: \"Lucida Grande\", Arial, sans-serif; font-size: 12px; }\n"
      "      .player h2 { margin: 0; }\n"
      "      .player ul.params { position: relative; top: 2px; }\n"
      "      #masthead h2 { margin: 10px 0 5px 0; }\n"
      "      #notice { border: 1px solid #ddbbbb; background: #ffdddd; font-size: 12px; }\n"
      "      #notice h2 { margin-bottom: 10px; }\n"
      "      .toc { float: left; padding: 0; }\n"
      "      .toc-wide { width: 560px; }\n"
      "      .toc-narrow { width: 375px; }\n"
      "      .toc li { margin-bottom: 10px; list-style-type: none; }\n"
      "      .toc li ul { padding-left: 10px; }\n"
      "      .toc li ul li { margin: 0; list-style-type: none; font-size: 13px; }\n"
      "      .charts { margin: 10px 60px 0 4px; float: left; width: 550px; text-align: center; }\n"
      "      .charts img { padding: 8px; margin: 0 auto; margin-bottom: 20px; border: 1px solid #ccc; -moz-border-radius: 6px; -khtml-border-radius: 6px; -webkit-border-radius: 6px; border-radius: 6px; -moz-box-shadow: inset 1px 1px 4px #ccc; -webkit-box-shadow: inset 1px 1px 4px #ccc; box-shadow: inset 1px 1px 4px #ccc; }\n"
      "      .talents div.float { width: auto; margin-right: 50px; }\n"
      "      table.sc { border: 0; background-color: #eee; }\n"
      "      table.sc tr { background-color: #fff; }\n"
      "      table.sc tr.head { background-color: #aaa; color: #fff; }\n"
      "      table.sc tr.odd { background-color: #f3f3f3; }\n"
      "      table.sc th { padding: 2px 4px; text-align: center; background-color: #aaa; color: #fcfcfc; }\n"
      "      table.sc th.small { padding: 2px 2px; font-size: 12px; }\n"
      "      table.sc th a { color: #fff; text-decoration: underline; }\n"
      "      table.sc th a:hover, table.sc th a:active { color: #f1f1ff; }\n"
      "      table.sc td { padding: 2px 4px; text-align: center; font-size: 13px; }\n"
      "      table.sc td.small { padding: 2px 2px; font-size: 11px; }\n"
      "      table.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td { text-align: left; padding-right: 6px; }\n"
      "      table.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td { text-align: right; padding-right: 6px; }\n"
      "      table.sc tr.details td { padding: 0 0 15px 15px; text-align: left; background-color: #fff; }\n"
      "      table.sc tr.details td ul { padding: 0; margin: 4px 0 8px 0; }\n"
      "      table.sc tr.details td ul li { clear: both; padding: 2px; list-style-type: none; }\n"
      "      table.sc tr.details td ul li span.label { display: block; float: left; width: 150px; margin-right: 4px; background: #f3f3f3; }\n"
      "      table.sc tr.details td ul li span.tooltip { display: block; float: left; width: 190px; }\n"
      "      table.sc tr.details td ul li span.tooltip-wider { display: block; float: left; width: 350px; }\n"
      "      table.sc tr.details td div.float { width: 350px; }\n"
      "      table.sc tr.details td div.float h5 { margin-top: 4px; }\n"
      "      table.sc tr.details td div.float ul { margin: 0 0 12px 0; }\n"
      "      table.sc td.filler { background-color: #ccc; }\n"
      "      table.sc .dynamic-buffs tr.details td ul li span.label { width: 80px; }\n"
      "      .sample-sequence { width: 500px; word-wrap: break-word; outline: 1px solid #ddd; background: #fcfcfc; padding: 6px; font-family: \"Lucida Console\", Monaco, monospace; font-size: 12px; }\n" );
  }

  util_t::fprintf( file,
    "    </style>\n\n"
    "  </head>\n\n" );
          
        util_t::fprintf( file,
          "  <body>\n\n" );
        
        if( ! sim -> error_list.empty() )
        {
                util_t::fprintf( file,
                  "    <pre>\n" );
                int num_errors = sim -> error_list.size();
                for( int i=0; i < num_errors; i++ )
                  util_t::fprintf( file,
                    "      %s\n", sim -> error_list[ i ].c_str() );
                util_t::fprintf( file,
                  "    </pre>\n\n" );
        }
        
        // Prints div wrappers for help popups
        util_t::fprintf( file,
          "    <div id=\"active-help\">\n"
          "      <div id=\"active-help-dynamic\">\n"
          "        <div class=\"help-box\">\n"
          "        </div>\n"
          "        <a href=\"#\" class=\"close\">close</a>\n"
          "      </div>\n"
          "    </div>\n\n" );
        
        // Begin masthead section
        util_t::fprintf( file,
          "    <div id=\"masthead\" class=\"section section-open\">\n\n" );
        
        util_t::fprintf( file,
          "      <h1>SimulationCraft %s-%s</h1>\n"
          "      <h2>for World of Warcraft %s %s (build level %s)</h2>\n\n",
          SC_MAJOR_VERSION, SC_MINOR_VERSION, ( dbc_t::get_ptr() ? "4.0.6" : "4.0.6" ), ( dbc_t::get_ptr() ? "PTR" : "Live" ), dbc_t::build_level() );
        
        time_t rawtime;
        time ( &rawtime );
        
        util_t::fprintf( file,
          "      <ul class=\"params\">\n" );
        util_t::fprintf( file,
          "        <li><b>Timestamp:</b> %s</li>\n",
          ctime( &rawtime ) );
        util_t::fprintf( file,
          "        <li><b>Iterations:</b> %d</li>\n",
          sim -> iterations );
        util_t::fprintf( file,
          "        <li><b>Fight Length:</b> %.0f</li>\n",
          sim -> max_time );
        if ( sim -> vary_combat_length > 0.0 )
        {
          util_t::fprintf( file,
            "        <li><b>Vary Combat Length:</b> %.2f</li>\n",
            sim -> vary_combat_length );
        }
        util_t::fprintf( file,
          "        <li><b>Smooth RNG:</b> %s</li>\n",
          ( sim -> smooth_rng ? "true" : "false" ) );
        util_t::fprintf( file,
          "      </ul>\n" );
        util_t::fprintf( file,
          "      <div class=\"clear\"></div>\n\n"
          "    </div>\n\n" );
        // End masthead section
        
        if ( num_players > 1 )
        {
                print_html_contents( file, sim );
        }
        
#if SC_BETA
    util_t::fprintf( file,
      "    <div id=\"notice\" class=\"section section-open\">\n" );
        util_t::fprintf( file,
          "      <h2>Beta Release</h2>\n" );
        int ii = 0;
        if ( beta_warnings[ 0 ] )
          util_t::fprintf( file,
        "      <ul>\n" );
        while ( beta_warnings[ ii ] )
        {
          util_t::fprintf( file,
                "        <li>%s</li>\n",
                beta_warnings[ ii ] );
      ii++;
        }
        if ( beta_warnings[ 0 ] )
          util_t::fprintf( file,
            "      </ul>\n" );
        util_t::fprintf( file,
          "    </div>\n\n" );
#endif
        
        if ( num_players > 1 )
        {
                print_html_raid_summary( file, sim );
                print_html_scale_factors( file, sim );
        }
        
        // Players
        for ( int i=0; i < num_players; i++ )
        {
          print_html_player( file, sim, sim -> players_by_name[ i ], i );

          // Pets
          if ( sim -> report_pets_separately )
          {
            for ( pet_t* pet = sim -> players_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
            {
              if ( pet -> summoned )
                print_html_player( file, sim, pet, 1 );
            }
          }
        }

        // Auras
        print_html_auras_debuffs( file, sim );

        // Report Targets
        if ( sim -> report_targets )
        {
          for ( int i=0; i < (int) sim -> targets_by_name.size(); i++ )
          {
            print_html_player( file, sim, sim -> targets_by_name[ i ], i );

            // Adds
            if ( sim -> targets_by_name[i] )
            {
              for ( add_t* add = sim -> targets_by_name[ i ] -> cast_target() -> add_list; add; add = add -> next_add )
              {
                if ( add -> summoned )
                  print_html_player( file, sim, add, 1 );
              }
            }
          }
        }
        
        print_html_help_boxes( file, sim );
        
        
        // jQuery
        util_t::fprintf ( file,
          "    <script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.2/jquery.min.js\"></script>\n" );
        
    // Toggles, image load-on-demand, etc. Load from simulationcraft.org if
    // hosted_html=1, otherwise embed
    if ( sim -> hosted_html )
    {
      util_t::fprintf( file,
        "    <script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/ga.js\"></script>\n"
        "    <script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/rep.js\"></script>\n" );
    }
    else
    {
      util_t::fprintf( file,
        "    <script type=\"text/javascript\">\n"
        "      jQuery.noConflict();\n"
        "      jQuery(document).ready(function($) {\n"
        "        function open_anchor(anchor) {\n"
        "          var img_id = '';\n"
        "          var src = '';\n"
        "          var target = '';\n"
        "          anchor.addClass('open');\n"
        "          var section = anchor.parent('.section');\n"
        "          section.addClass('section-open');\n"
        "          section.removeClass('grouped-first');\n"
        "          section.removeClass('grouped-last');\n"
        "          if (!(section.next().hasClass('section-open'))) {\n"
        "            section.next().addClass('grouped-first');\n"
        "          }\n"
        "          if (!(section.prev().hasClass('section-open'))) {\n"
        "            section.prev().addClass('grouped-last');\n"
        "          }\n"
        "          anchor.next('.toggle-content').show(150);\n"
        "          anchor.next('.toggle-content').find('.charts').each(function() {\n"
        "            $(this).children('span').each(function() {\n"
        "              img_class = $(this).attr('class');\n"
        "              img_alt = $(this).attr('title');\n"
        "              img_src = $(this).html().replace(/&amp;/g, \"&\");\n"
        "              var img = new Image();\n"
        "              $(img).attr('class', img_class);\n"
        "              $(img).attr('src', img_src);\n"
        "              $(img).attr('alt', img_alt);\n"
        "              $(this).replaceWith(img);\n"
        "              $(this).load();\n"
        "            });\n"
        "          })\n"
        "          setTimeout('window.scrollTo(0, anchor.position().top', 500);\n"
        "        }\n"
        "        var anchor_check = document.location.href.split('#');\n"
        "        if (anchor_check.length > 1) {\n"
        "        	var anchor = anchor_check[anchor_check.length - 1];\n"
        "        }\n"
        "        var pcol = document.location.protocol;\n"
        "        if (pcol != 'file:') {\n"
        "          var whtt = document.createElement(\"script\");\n"
        "          whtt.src = pcol + \"//static.wowhead.com/widgets/power.js\";\n"
        "          $('body').append(whtt);\n"
        "        }\n"
        "        $('a[ rel=\"_blank\"]').each(function() {\n"
        "          $(this).attr('target', '_blank');\n"
        "        });\n"
        "        $('.toggle-content, .help-box').hide();\n"
        "        $('.open').next('.toggle-content').show();\n"
        "        $('.toggle').click(function(e) {\n"
        "          var img_id = '';\n"
        "          var src = '';\n"
        "          var target = '';\n"
        "          e.preventDefault();\n"
        "          $(this).toggleClass('open');\n"
        "          var section = $(this).parent('.section');\n"
        "          if (section.attr('id') != 'masthead') {\n"
        "            section.toggleClass('section-open');\n"
        "          }\n"
        "          if (section.attr('id') != 'masthead' && section.hasClass('section-open')) {\n"
        "            section.removeClass('grouped-first');\n"
        "            section.removeClass('grouped-last');\n"
        "            if (!(section.next().hasClass('section-open'))) {\n"
        "              section.next().addClass('grouped-first');\n"
        "            }\n"
        "            if (!(section.prev().hasClass('section-open'))) {\n"
        "              section.prev().addClass('grouped-last');\n"
        "            }\n"
        "          } else if (section.attr('id') != 'masthead') {\n"
        "            if (section.hasClass('final') || section.next().hasClass('section-open')) {\n"
        "              section.addClass('grouped-last');\n"
        "            } else {\n"
        "              section.next().removeClass('grouped-first');\n"
        "            }\n"
        "            if (section.prev().hasClass('section-open')) {\n"
        "              section.addClass('grouped-first');\n"
        "            } else {\n"
        "              section.prev().removeClass('grouped-last');\n"
        "            }\n"
        "          }\n"
        "          $(this).next('.toggle-content').toggle(150);\n"
        "          $(this).next('.toggle-content').find('.charts').each(function() {\n"
        "            $(this).children('span').each(function() {\n"
        "              img_class = $(this).attr('class');\n"
        "              img_alt = $(this).attr('title');\n"
        "              img_src = $(this).html().replace(/&amp;/g, \"&\");\n"
        "              var img = new Image();\n"
        "              $(img).attr('class', img_class);\n"
        "              $(img).attr('src', img_src);\n"
        "              $(img).attr('alt', img_alt);\n"
        "              $(this).replaceWith(img);\n"
        "              $(this).load();\n"
        "            });\n"
        "          });\n"
        "        });\n"
        "        $('.toggle-details').click(function(e) {\n"
        "          e.preventDefault();\n"
        "          $(this).toggleClass('open');\n"
        "          $(this).parents().next('.details').toggleClass('hide');\n"
        "        });\n"
        "        $('.toggle-db-details').click(function(e) {\n"
        "          e.preventDefault();\n"
        "          $(this).toggleClass('open');\n"
        "          $(this).parent().next('.toggle-content').toggle(150);\n"
        "        });\n"
        "        $('.help').click(function(e) {\n"
        "          e.preventDefault();\n"
        "          var target = $(this).attr('href') + ' .help-box';\n"
        "          var content = $(target).html();\n"
        "          $('#active-help-dynamic .help-box').html(content);\n"
        "          $('#active-help .help-box').show();\n"
        "          var t = e.pageY - 20;\n"
        "          var l = e.pageX - 20;\n"
        "          $('#active-help').css({top:t,left:l});\n"
        "          $('#active-help').toggle(250);\n"
        "        });\n"
        "        $('#active-help a.close').click(function(e) {\n"
        "          e.preventDefault();\n"
        "          $('#active-help').toggle(250);\n"
        "        });\n"
        "          if (anchor) {\n"
        "            anchor = '#' + anchor;\n"
        "            target = $(anchor).children('h2:first');\n"
        "            open_anchor(target);\n"
        "          }\n"
        "          $('ul.toc li a').click(function(e) {\n"
        "            anchor = $(this).attr('href');\n"
        "            target = $(anchor).children('h2:first');\n"
        "            open_anchor(target);\n"
        "          });\n"
        "      });\n"
        "    </script>\n\n" );
    }
    util_t::fprintf( file,
      "  </body>\n\n"
      "</html>\n" );
        
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
  std::string wiki_file_name = wiki_name;
  pos = wiki_name.rfind( DIRECTORY_DELIMITER );
  if ( pos != std::string::npos ) wiki_name = wiki_name.substr( pos + 1, wiki_name.length() - pos );

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
      std::string file_name = wiki_file_name + type_name + ".wiki";

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
      file_name  = sim -> save_prefix_str;
      file_name += p -> name_str;
      file_name += ".simc";
      util_t::urlencode( util_t::format_text( file_name, sim -> input_is_utf8 ) );
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

// report_t::print_spell_query ===============================================

void report_t::print_spell_query( sim_t* sim )
{
  spell_data_expr_t* sq = sim -> spell_query;
  assert( sq );

  for ( std::vector<uint32_t>::const_iterator i = sq -> result_spell_list.begin(); i != sq -> result_spell_list.end(); i++ )
  {
    if ( sq -> data_type == DATA_TALENT )
    {
      util_t::fprintf( sim -> output_file, "%s", spell_info_t::talent_to_str( sim, sim -> sim_data.m_talents_index[ *i ] ).c_str() );
    }
    else if ( sq -> data_type == DATA_EFFECT )
    {
      std::ostringstream sqs;
      if ( sim -> sim_data.m_spells_index[ sim -> sim_data.m_effects_index[ *i ] -> spell_id ] )
      {
        spell_info_t::effect_to_str( sim, 
                                     sim -> sim_data.m_spells_index[ sim -> sim_data.m_effects_index[ *i ] -> spell_id ], 
                                     sim -> sim_data.m_effects_index[ *i ],
                                     sqs );
      }
      util_t::fprintf( sim -> output_file, "%s", sqs.str().c_str() );
    }
    else
    {
      util_t::fprintf( sim -> output_file, "%s", spell_info_t::to_str( sim, sim -> sim_data.m_spells_index[ *i ] ).c_str() );
    }
  }
}

// report_t::print_suite =====================================================

void report_t::print_suite( sim_t* sim )
{
  report_t::print_text( sim -> output_file, sim, sim -> report_details );
  report_t::print_html( sim );
  report_t::print_wiki( sim );
  report_t::print_xml( sim );
  report_t::print_profiles( sim );
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace   // ANONYMOUS NAMESPACE ==========================================
{

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

// print_action ==============================================================

static void print_action( FILE* file, stats_t* s, int max_name_length=0 )
{
  if ( s -> total_dmg == 0 ) return;

  if( max_name_length == 0 ) max_name_length = 20;

  util_t::fprintf( file,
                   "    %-*s  Count=%5.1f|%4.1fsec  DPE=%6.0f|%2.0f%%  DPET=%6.0f  DPR=%6.1f  pDPS=%4.0f",
		   max_name_length,
                   s -> name_str.c_str(),
                   s -> num_executes,
                   s -> frequency,
                   s -> dpe,
                   s -> portion_dmg * 100.0,
                   s -> dpet,
                   s -> dpr,
                   s -> portion_dps );

  double miss_pct = ( s -> execute_results[ RESULT_MISS ].count +
                      s ->    tick_results[ RESULT_MISS ].count ) / ( s -> num_executes + s -> num_ticks );


  util_t::fprintf( file, "  Miss=%.1f%%", 100.0 * miss_pct );

  if ( s -> execute_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file, "  Hit=%4.0f", s -> execute_results[ RESULT_HIT ].avg_dmg );
  }
  if ( s -> execute_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Crit=%5.0f|%5.0f|%.1f%%",
                     s -> execute_results[ RESULT_CRIT ].avg_dmg,
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

  if ( s -> num_ticks > 0 ) util_t::fprintf( file, "  TickCount=%.0f", s -> num_ticks );

  if ( s -> tick_results[ RESULT_HIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  Tick=%.0f", s -> tick_results[ RESULT_HIT ].avg_dmg );
  }
  if ( s -> tick_results[ RESULT_CRIT ].avg_dmg > 0 )
  {
    util_t::fprintf( file,
                     "  CritTick=%.0f|%.0f|%.1f%%",
                     s -> tick_results[ RESULT_CRIT ].avg_dmg,
                     s -> tick_results[ RESULT_CRIT ].max_dmg,
                     s -> tick_results[ RESULT_CRIT ].count * 100.0 / s -> num_ticks );
  }

  util_t::fprintf( file, "\n" );
}

// print_actions =============================================================

static void print_actions( FILE* file, player_t* p )
{
  util_t::fprintf( file, "  Glyphs: %s\n", p -> glyphs_str.c_str() );

  if ( p -> action_list_default )
  {
    util_t::fprintf( file, "  Priorities:\n" );

    std::vector<std::string> action_list;
    int num_actions = util_t::string_split( action_list, p -> action_list_str, "/" );
    int length = 0;
    for ( int i=0; i < num_actions; i++ )
    {
      if ( length > 80 )
      {
        util_t::fprintf( file, "\n" );
        length = 0;
      }
      util_t::fprintf( file, "%s%s", ( ( length > 0 ) ? "/" : "    " ), action_list[ i ].c_str() );
      length += action_list[ i ].size();
    }
    util_t::fprintf( file, "\n" );
  }

  util_t::fprintf( file, "  Actions:\n" );

  int max_length=0;
  for ( stats_t* s = p -> stats_list; s; s = s -> next )
    if ( s -> total_dmg > 0 )
      if( max_length < (int) s -> name_str.length() )
	max_length = s -> name_str.length();

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
      int length = strlen( b -> name() );
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
      int length = strlen( b -> name() );
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
      int length = strlen( b -> name() );
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
      int length = strlen( b -> name() );
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
                   "  Core Stats:  strength=%.0f(%.0f)  agility=%.0f(%.0f)  stamina=%.0f(%.0f)  intellect=%.0f(%.0f)  spirit=%.0f(%.0f)  health=%.0f  mana=%.0f\n",
                   p -> strength(),  p -> stats.attribute[ ATTR_STRENGTH  ],
                   p -> agility(),   p -> stats.attribute[ ATTR_AGILITY   ],
                   p -> stamina(),   p -> stats.attribute[ ATTR_STAMINA   ],
                   p -> intellect(), p -> stats.attribute[ ATTR_INTELLECT ],
                   p -> spirit(),    p -> stats.attribute[ ATTR_SPIRIT    ],
                   p -> resource_max[ RESOURCE_HEALTH ], p -> resource_max[ RESOURCE_MANA ] );
}

// print_spell_stats ==========================================================

static void print_spell_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Spell Stats:  power=%.0f(%.0f)  hit=%.2f%%(%.0f)  crit=%.2f%%(%.0f)  penetration=%.0f(%.0f)  haste=%.2f%%(%.0f)  mp5=%.0f\n",
                   p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier(), p -> stats.spell_power,
                   p -> composite_spell_hit()  * 100.0,    p -> stats.hit_rating,
                   p -> composite_spell_crit() * 100.0,    p -> stats.crit_rating,
                   p -> composite_spell_penetration(),     p -> stats.spell_penetration,
                   ( 1.0 / p -> spell_haste - 1 ) * 100.0, p -> stats.haste_rating,
                   p -> initial_mp5 );
}

// print_attack_stats =========================================================

static void print_attack_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Attack Stats  power=%.0f(%.0f)  hit=%.2f%%(%.0f)  crit=%.2f%%(%.0f)  expertise=%.2f%%(%.0f)  penetration=%.2f%%(%.0f)  haste=%.2f%%(%.0f)\n",
                   p -> composite_attack_power() * p -> composite_attack_power_multiplier(), p -> stats.attack_power,
                   p -> composite_attack_hit()         * 100.0, p -> stats.hit_rating,
                   p -> composite_attack_crit()        * 100.0, p -> stats.crit_rating,
                   p -> composite_attack_expertise()   * 100.0, p -> stats.expertise_rating,
                   p -> composite_attack_penetration() * 100.0, p -> stats.armor_penetration_rating,
                   ( 1.0 / p -> attack_haste - 1 )     * 100.0, p -> stats.haste_rating );
}

// print_defense_stats =======================================================

static void print_defense_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Defense Stats:  armor=%.0f(%.0f)\n",
                   p -> composite_armor(), p -> stats.armor );
}

// print_gains ===============================================================

static void print_gains( FILE* file, player_t* p )
{
  int max_length = 0;
  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 )
    {
      int length = strlen( g -> name() );
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
                   "  TotalEvents   = %d\n"
                   "  MaxEventQueue = %d\n"
                   "  TargetHealth  = %.0f\n"
                   "  SimSeconds    = %.0f\n"
                   "  CpuSeconds    = %.3f\n"
                   "  SpeedUp       = %.0f\n\n",
                   sim -> total_events_processed,
                   sim -> max_events_remaining,
                   sim -> target -> initial_health,
                   sim -> iterations * sim -> total_seconds,
                   sim -> elapsed_cpu_seconds,
                   sim -> iterations * sim -> total_seconds / sim -> elapsed_cpu_seconds );

  sim -> rng -> report( file );
}

// print_scale_factors ========================================================

static void print_scale_factors( FILE* file, sim_t* sim )
{
  if ( ! sim -> scaling -> calculate_scale_factors ) return;

  util_t::fprintf( file, "\nScale Factors:\n" );

  int num_players = sim -> players_by_name.size();
  int max_length=0;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    int length = strlen( p -> name() );
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
        util_t::fprintf( file, "  %s=%.2f", util_t::stat_type_abbrev( j ), sf.get_stat( j ) );
      }
    }

    if ( sim -> scaling -> normalize_scale_factors )
    {
      util_t::fprintf( file, "  DPS/%s=%.2f", util_t::stat_type_abbrev( p -> normalized_to ), p -> scaling.get_stat( p -> normalized_to ) );
    }

    util_t::fprintf( file, "  Lag=%.2f", p -> scaling_lag );

    util_t::fprintf( file, "\n" );
  }
}

// print_scale_factors ========================================================

static void print_scale_factors( FILE* file, player_t* p )
{
  if ( ! p -> sim -> scaling -> calculate_scale_factors ) return;

  util_t::fprintf( file, "  Scale Factors:\n" );

  gear_stats_t& sf = ( p -> sim -> scaling -> normalize_scale_factors ) ? p -> normalized_scaling : p -> scaling;
  
  util_t::fprintf( file, "    Weights :" );
  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( p -> scales_with[ i ] != 0 )
    {
      util_t::fprintf( file, "  %s=%.2f", util_t::stat_type_abbrev( i ), sf.get_stat( i ) );
    }
  }
  if ( p -> sim -> scaling -> normalize_scale_factors )
  {
    util_t::fprintf( file, "  DPS/%s=%.2f", util_t::stat_type_abbrev( p -> normalized_to ), p -> scaling.get_stat( p -> normalized_to ) );
  }
  util_t::fprintf( file, "\n" );

  std::string lootrank = p -> gear_weights_lootrank_link;
  std::string wowhead  = p -> gear_weights_wowhead_link;
  std::string pawn     = p -> gear_weights_pawn_string;

  simplify_html( lootrank );
  simplify_html( wowhead  );
  simplify_html( pawn     );

  util_t::fprintf( file, "    Wowhead : %s\n", wowhead.c_str() );
}

// print_reference_dps ========================================================

static void print_reference_dps( FILE* file, sim_t* sim )
{
  if ( sim -> reference_player_str.empty() ) return;

  util_t::fprintf( file, "\nReference DPS:\n" );

  player_t* ref_p = sim -> find_player( sim -> reference_player_str );

  if ( ! ref_p )
  {
    util_t::fprintf( file, "Unable to locate reference player: %s\n", sim -> reference_player_str.c_str() );
    return;
  }

  int num_players = sim -> players_by_rank.size();
  int max_length=0;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_rank[ i ];
    int length = strlen( p -> name() );
    if ( length > max_length ) max_length = length;
  }

  util_t::fprintf( file, "  %-*s", max_length, ref_p -> name() );
  util_t::fprintf( file, "  %.0f", ref_p -> dps );

  if ( sim -> scaling -> calculate_scale_factors )
  {
    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( ref_p -> scales_with[ j ] != 0 )
      {
        util_t::fprintf( file, "  %s=%.2f", util_t::stat_type_abbrev( j ), ref_p -> scaling.get_stat( j ) );
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

      if ( sim -> scaling -> calculate_scale_factors )
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

  int num_players = sim -> players_by_name.size();
  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    if( p -> procs.hat_donor -> count )
      hat_donors.push_back( p );
  }

  int num_donors = hat_donors.size();
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

// print_html_menu_definition ================================================

static void print_html_menu_definition( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "<script type=\"text/javascript\">\n"
                   "function hideElement(el) {if (el) el.style.display='none';}\n"
                   "function showElement(el) {if (el) el.style.display='';}\n"
                   "function hideElements(els) {if (els) {"
                   "for (var i = 0; i < els.length; i++) hideElement(els[i]);"
                   "}}\n"
                   "function showElements(els) {if (els) {"
                   "for (var i = 0; i < els.length; i++) showElement(els[i]);"
                   "}}\n"
                   "</script>\n" );
}

// print_html_menu_triggers ==================================================

static void print_html_menu_triggers( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "<a href=\"javascript:hideElement(document.getElementById('trigger_menu'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElement(document.getElementById('trigger_menu'));\">+</a> " );
  util_t::fprintf( file, "Menu\n" );

  util_t::fprintf( file, "<div id=\"trigger_menu\" style=\"display:none;\">\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementById('players').getElementsByTagName('img'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementById('players').getElementsByTagName('img'));\">+</a> " );
  util_t::fprintf( file, "All Charts<br />\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementsByName('chart_dpet'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_dpet'));\">+</a> " );
  util_t::fprintf( file, "DamagePerExecutionTime<br />\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementsByName('chart_uptimes'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_uptimes'));\">+</a> " );
  util_t::fprintf( file, "Up-Times and Procs<br />\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementsByName('chart_sources'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_sources'));\">+</a> " );
  util_t::fprintf( file, "Damage Sources<br />\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementsByName('chart_gains'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_gains'));\">+</a> " );
  util_t::fprintf( file, "Resource Gains<br />\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementsByName('chart_dps_timeline'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_dps_timeline'));\">+</a> " );
  util_t::fprintf( file, "DPS Timeline<br />\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementsByName('chart_dps_distribution'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_dps_distribution'));\">+</a> " );
  util_t::fprintf( file, "DPS Distribution<br />\n" );

  util_t::fprintf( file, "  <a href=\"javascript:hideElements(document.getElementsByName('chart_resource_timeline'));\">-</a> " );
  util_t::fprintf( file, "<a href=\"javascript:showElements(document.getElementsByName('chart_resource_timeline'));\">+</a> " );
  util_t::fprintf( file, "Resource Timeline<br />\n" );

  util_t::fprintf( file, "</div>\n" );//trigger_menu

  util_t::fprintf( file, "<hr />\n" );
}

// print_html_raid ===========================================================

static void print_html_raid( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "<h1>Raid</h1>\n" );

  assert( sim ->  dps_charts.size() ==
          sim -> gear_charts.size() );

  int count = sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file, "\n<!-- DPS Ranking: -->\n" );
    util_t::fprintf( file, "<img src=\"%s\" />\n", sim -> dps_charts[ i ].c_str() );

    util_t::fprintf( file, "\n<!-- Gear Overview: -->\n" );
    util_t::fprintf( file, "<img src=\"%s\" />\n", sim -> gear_charts[ i ].c_str() );
  }

  if ( ! sim -> downtime_chart.empty() )
  {
    util_t::fprintf( file, "\n<!-- Raid Downtime: -->\n" );
    util_t::fprintf( file, "<img src=\"%s\" />\n", sim -> downtime_chart.c_str() );
  }

  if ( ! sim -> uptimes_chart.empty() )
  {
    util_t::fprintf( file, "\n<!-- Global Up-Times: -->\n" );
    util_t::fprintf( file, "<img src=\"%s\" />\n", sim -> uptimes_chart.c_str() );
  }

  count = sim -> dpet_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file, "\n<!-- Raid Damage Per Execute Time: -->\n" );
    util_t::fprintf( file, "<img src=\"%s\" />\n", sim -> dpet_charts[ i ].c_str() );
  }
  util_t::fprintf( file, "<hr />\n" );
}

// print_html_scale_factors ===================================================

static void print_html_scale_factors( FILE*  file, sim_t* sim )
{
  if ( ! sim -> scaling -> calculate_scale_factors ) return;

  util_t::fprintf( file, "<h1>DPS Scale Factors (dps increase per unit stat)</h1>\n" );

  util_t::fprintf( file, "<style type=\"text/css\">\n  table.scale_factors td, table.scale_factors th { padding: 4px; border: 1px inset; }\n  table.scale_factors { border: 1px outset; }</style>\n" );

  util_t::fprintf( file, "<table class=\"scale_factors\">\n" );
  util_t::fprintf( file, "  <tr>\n    <th>profile</th>\n" );
  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( sim -> scaling -> stats.get_stat( i ) != 0 )
    {
      util_t::fprintf( file, "    <th>%s</th>\n", util_t::stat_type_abbrev( i ) );
    }
  }
  util_t::fprintf( file, "    <th>Lag</th>\n      </tr>\n" );

  std::string buffer;
  int num_players = sim -> players_by_name.size();

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    util_t::fprintf( file, "  <tr>\n    <td>%s</td>\n", p -> name() );

    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        util_t::fprintf( file, "    <td>%.2f</td>\n", p -> scaling.get_stat( j ) );
      }
    }

    util_t::fprintf( file, "    <td>%.2f</td>\n", p -> scaling_lag );
    util_t::fprintf( file, "  </tr>\n" );
  }
  util_t::fprintf( file, "</table>\n" );
  util_t::fprintf( file, "<hr />\n" );

  util_t::fprintf( file, "<table class=\"scale_factors\">\n" );
  util_t::fprintf( file, "  <tr>\n    <th>profile</th>\n    <th>lootrank</th>\n    <th>wowhead</th>\n    <th>pawn</th>\n  </tr>\n" );

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    util_t::fprintf( file, "  <tr>\n    <td>%s</td>\n", p -> name() );
    util_t::fprintf( file, "    <td><a href=\"%s\"> lootrank</a></td>\n", p -> gear_weights_lootrank_link.c_str() );
    util_t::fprintf( file, "    <td><a href=\"%s\"> wowhead </a></td>\n", p -> gear_weights_wowhead_link.c_str() );
    util_t::fprintf( file,
                     "    <td>\n      <div style=\"margin:1px; margin-top:1px\">\n"
                     "        <pre class=\"alt2\" dir=\"ltr\" style=\""
                     "margin: 0px;"
                     "padding: 3px;"
                     "border: 1px inset;"
                     "width: 512px;"
                     "height: 30px;"
                     "text-align: left;"
                     "overflow: auto\">%s"
                     "</pre>\n"
                     "      </div>\n    </td>\n",
                     p -> gear_weights_pawn_string.c_str() );

    util_t::fprintf( file, "  </tr>\n" );
  }
  util_t::fprintf( file, "</table>\n" );
  util_t::fprintf( file, "<hr />\n" );
}

// print_html_player =========================================================

static void print_html_player( FILE* file, player_t* p )
{
  util_t::fprintf( file, "<h1>%s</h1>\n", p -> name() );
  util_t::fprintf( file, "<ul><li><a href=\"%s\">Talents</a><li><a href=\"%s\">Origin</a></ul>\n", p -> talents_str.c_str(), p -> origin_str.c_str() );

  if ( ! p -> action_dpet_chart.empty() )
  {
    util_t::fprintf( file, "\n<!-- %s Damage Per Execute Time: -->\n", p -> name() );
    util_t::fprintf( file, "<img name=\"chart_dpet\" src=\"%s\" />\n", p -> action_dpet_chart.c_str() );
  }

  if ( ! p -> uptimes_and_procs_chart.empty() )
  {
    util_t::fprintf( file, "\n<!-- %s Up-Times and Procs: -->\n", p -> name() );
    util_t::fprintf( file, "<img name=\"chart_uptimes\" src=\"%s\" />\n", p -> uptimes_and_procs_chart.c_str() );
  }
  util_t::fprintf( file, "<br />\n" );

  if ( ! p -> action_dmg_chart.empty() )
  {
    util_t::fprintf( file, "\n<!-- %s Damage Sources: -->\n", p -> name() );
    util_t::fprintf( file, "<img name=\"chart_sources\" src=\"%s\" />\n", p -> action_dmg_chart.c_str() );
  }

  if ( ! p -> gains_chart.empty() )
  {
    util_t::fprintf( file, "\n<!-- %s Resource Gains: -->\n", p -> name() );
    util_t::fprintf( file, "<img name=\"chart_gains\" src=\"%s\" />\n", p -> gains_chart.c_str() );
  }
  util_t::fprintf( file, "<br />\n" );

  util_t::fprintf( file, "\n<!-- %s DPS Timeline: -->\n", p -> name() );
  util_t::fprintf( file, "<img name=\"chart_dps_timeline\" src=\"%s\" />\n", p -> timeline_dps_chart.c_str() );

  util_t::fprintf( file, "\n<!-- %s DPS Distribution: -->\n", p -> name() );
  util_t::fprintf( file, "<img name=\"chart_dps_distribution\" src=\"%s\" />\n", p -> distribution_dps_chart.c_str() );

  util_t::fprintf( file, "\n<!-- %s Resource Timeline: -->\n", p -> name() );
  util_t::fprintf( file, "<img name=\"chart_resource_timeline\" src=\"%s\" />\n", p -> timeline_resource_chart.c_str() );

  util_t::fprintf( file, "<hr />\n" );
}

// print_html_text ===========================================================

static void print_html_text( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "<h1>Raw Text Output</h1>\n" );

  util_t::fprintf( file, "%s",
                   "<ul>\n"
                   " <li><b>DPS=Num:</b> <i>Num</i> is the <i>damage per second</i></li>\n"
                   " <li><b>DPR=Num:</b> <i>Num</i> is the <i>damage per resource</i></li>\n"
                   " <li><b>RPS=Num1/Num2:</b> <i>Num1</i> is the <i>resource consumed per second</i> and <i>Num2</i> is the <i>resource regenerated per second</i></li>\n"
                   " <li><b>Count=Num|Time:</b> <i>Num</i> is number of casts per fight and <i>Time</i> is average time between casts</li>\n"
                   " <li><b>DPE=Num:</b> <i>Num</i> is the <i>damage per execute</i></li>\n"
                   " <li><b>DPET=Num:</b> <i>Num</i> is the <i>damage per execute</i> divided by the <i>time to execute</i> (this value includes GCD costs and Lag in the calculation of <i>time to execute</i>)</li>\n"
                   " <li><b>Hit=Num:</b> <i>Num</i> is the average damage per non-crit hit</li>\n"
                   " <li><b>Crit=Num1|Num2|Pct:</b> <i>Num1</i> is average crit damage, <i>Num2</i> is the max crit damage, and <i>Pct</i> is the percentage of crits <i>per execute</i> (not <i>per hit</i>)</li>\n"
                   " <li><b>Tick=Num:</b> <i>Num</i> is the average tick of damage for the <i>damage over time</i> portion of actions</li>\n"
                   " <li><b>Up-Time:</b> This is <i>not</i> the percentage of time the buff/debuff is present, but rather the ratio of <i>actions it affects</i> over <i>total number of actions it could affect</i>.  If spell S is cast 10 times and buff B is present for 3 of those casts, then buff B has an up-time of 30%.</li>\n"
                   " <li><b>Waiting</b>: This is percentage of total time not doing anything (except auto-attack in the case of physical dps classes).  This can occur because the player is resource constrained (Mana, Energy, Rage) or cooldown constrained (as in the case of Enhancement Shaman).</li>\n"
                   "</ul>\n" );

  util_t::fprintf( file, "<pre>\n" );
  report_t::print_text( file, sim );
  util_t::fprintf( file, "</pre>\n" );
}

// print_xml_raid ===========================================================

static void print_xml_raid( FILE*  file, sim_t* sim )
{
  assert( sim ->  dps_charts.size() == sim -> gear_charts.size() );

  int count = sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    util_t::fprintf( file, "    <chart name=\"DPS Ranking\" url=\"%s\" />\n", sim -> dps_charts[ i ].c_str() );

    util_t::fprintf( file, "    <chart name=\"Gear Overview\" url=\"%s\" />\n", sim -> gear_charts[ i ].c_str() );
  }

  if ( ! sim -> downtime_chart.empty() )
  {
    util_t::fprintf( file, "    <chart name=\"Raid Downtime\" url=\"%s\" />\n", sim -> downtime_chart.c_str() );
  }

  if ( ! sim -> uptimes_chart.empty() )
  {
    util_t::fprintf( file, "    <chart name=\"Global Up-Times\" url=\"%s\" />\n", sim -> uptimes_chart.c_str() );
  }

  count = sim -> dpet_charts.size();
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

  if ( ! p -> uptimes_and_procs_chart.empty() )
  {
    util_t::fprintf( file, "      <chart name=\"Up-Times and Procs\" type=\"chart_uptimes\" url=\"%s\" />\n", p -> uptimes_and_procs_chart.c_str() );
  }

  if ( ! p -> action_dmg_chart.empty() )
  {
    util_t::fprintf( file, "      <chart name=\"Damage Sources\" type=\"chart_sources\" url=\"%s\" />\n", p -> action_dmg_chart.c_str() );
  }

  if ( ! p -> gains_chart.empty() )
  {
    util_t::fprintf( file, "      <chart name=\"Resource Gains\" type=\"chart_gains\" url=\"%s\" />\n", p -> gains_chart.c_str() );
  }

  util_t::fprintf( file, "      <chart name=\"DPS Timeline\" type=\"chart_dps_timeline\" url=\"%s\" />\n", p -> timeline_dps_chart.c_str() );

  util_t::fprintf( file, "      <chart name=\"DPS Distribution\" type=\"chart_dps_distribution\" url=\"%s\" />\n", p -> distribution_dps_chart.c_str() );

  util_t::fprintf( file, "      <chart name=\"Resource Timeline\" type=\"chart_resource_timeline\" url=\"%s\" />\n", p -> timeline_resource_chart.c_str() );
}

// print_xml_scale_factors ===================================================

static void print_xml_player_scale_factors( FILE*  file, sim_t* sim, player_t* p )
{
  if ( ! sim -> scaling -> calculate_scale_factors ) return;

  util_t::fprintf( file, "      <scale_factors>\n" );

  for ( int j=0; j < STAT_MAX; j++ )
  {
    if ( sim -> scaling -> stats.get_stat( j ) != 0 )
    {
      util_t::fprintf( file, "        <scale_factor name=\"%s\" value=\"%.2f\" />\n", util_t::stat_type_abbrev( j ), p -> scaling.get_stat( j ) );
    }
  }

  util_t::fprintf( file, "        <scale_factor name=\"Lag\" value=\"%.2f\" />\n", p -> scaling_lag );
  util_t::fprintf( file, "        <scale_factor_link type=\"lootrank\" url=\"%s\" />\n", p -> gear_weights_lootrank_link.c_str() );
  util_t::fprintf( file, "        <scale_factor_link type=\"wowhead\" url=\"%s\" />\n", p -> gear_weights_wowhead_link.c_str() );
  util_t::fprintf( file, "        <pawn_string>%s</pawn_string>\n", p -> gear_weights_pawn_string.c_str() );

  util_t::fprintf( file, "      </scale_factors>\n" );
}

// print_xml_text ===========================================================

static void print_xml_text( FILE*  file, sim_t* sim )
{
  util_t::fprintf( file, "    <raw_text>\n<![CDATA[\n" );
  report_t::print_text( file, sim );
  util_t::fprintf( file, "    ]]>\n</raw_text>\n" );
}

// print_wiki_raid ===========================================================

static void print_wiki_raid( FILE*  file,
                             sim_t* sim )
{
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "= Raid Charts =\n" );

  assert( sim ->  dps_charts.size() ==
          sim -> gear_charts.size() );

  int count = sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    std::string  dps_chart = sim ->  dps_charts[ i ];
    std::string gear_chart = sim -> gear_charts[ i ];

    simplify_html(  dps_chart );
    simplify_html( gear_chart );

    util_t::fprintf( file, "|| %s&dummy=dummy.png || %s&dummy=dummy.png ||\n", dps_chart.c_str(), gear_chart.c_str() );
  }

  std::string raid_downtime = "No chart for Raid Down-Time";
  std::string raid_uptimes  = "No chart for Raid Up-Times";

  if ( ! sim -> downtime_chart.empty() )
  {
    raid_downtime = sim -> downtime_chart + "&dummy=dummy.png";
    simplify_html( raid_downtime );
  }
  if ( ! sim -> uptimes_chart.empty() )
  {
    raid_uptimes = sim -> uptimes_chart + "&dummy=dummy.png";
    simplify_html( raid_uptimes );
  }

  util_t::fprintf( file, "|| %s || %s ||\n", raid_downtime.c_str(), raid_uptimes.c_str() );

  count = sim -> dpet_charts.size();
  for ( int i=0; i < count; i++ )
  {
    std::string raid_dpet = sim -> dpet_charts[ i ] + "&dummy=dummy.png";
    simplify_html( raid_dpet );
    util_t::fprintf( file, "|| %s ", raid_dpet.c_str() );
    if ( ++i < count )
    {
      raid_dpet = sim -> dpet_charts[ i ] + "&dummy=dummy.png";
      simplify_html( raid_dpet );
      util_t::fprintf( file, "|| %s ", raid_dpet.c_str() );
    }
    util_t::fprintf( file, "||\n" );
  }

  util_t::fprintf( file, "\n" );
}

// print_wiki_scale_factors ===================================================

static void print_wiki_scale_factors( FILE*  file,
                                      sim_t* sim )
{
  if ( ! sim -> scaling -> calculate_scale_factors ) return;

  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "== DPS Scale Factors (dps increase per unit stat) ==\n" );

  util_t::fprintf( file, "|| profiles ||" );
  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( sim -> scaling -> stats.get_stat( i ) != 0 )
    {
      util_t::fprintf( file, " %s ||", util_t::stat_type_abbrev( i ) );
    }
  }
  util_t::fprintf( file, "lag ||\n" );

  std::string buffer;
  int num_players = sim -> players_by_name.size();

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    util_t::fprintf( file, "|| %-25s ||", p -> name() );

    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        util_t::fprintf( file, " %.2f ||", p -> scaling.get_stat( j ) );
      }
    }
    util_t::fprintf( file, " %.2f ||", p -> scaling_lag );
    util_t::fprintf( file, "\n" );
  }

  util_t::fprintf( file, "\n----\n\n" );

  util_t::fprintf( file, "|| profiles || lootrank || wowhead || pawn ||\n" );

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    util_t::fprintf( file, "|| %-25s ||", p -> name() );

    std::string lootrank = p -> gear_weights_lootrank_link;
    std::string wowhead  = p -> gear_weights_wowhead_link;
    std::string pawn     = p -> gear_weights_pawn_string;

    simplify_html( lootrank );
    simplify_html( wowhead  );
    simplify_html( pawn     );

    util_t::fprintf( file, " [%s lootrank] ||", lootrank.c_str() );
    util_t::fprintf( file, " [%s wowhead] ||",  wowhead.c_str() );
    util_t::fprintf( file, " {{{ %s }}} ||",            pawn.c_str() );
    util_t::fprintf( file, "\n" );
  }
}

// print_wiki_player =========================================================

static void print_wiki_player( FILE*     file,
                               player_t* p )
{
  std::string action_dpet       = "No chart for Damage Per Execute Time";
  std::string uptimes_and_procs = "No chart for Up-Times and Procs";
  std::string action_dmg        = "No chart for Damage Sources";
  std::string gains             = "No chart for Resource Gains";
  std::string timeline_dps      = "No chart for DPS Timeline";
  std::string distribution_dps  = "No chart for DPS Distribution";
  std::string timeline_resource = "No chart for Resource Timeline";

  if ( ! p -> action_dpet_chart.empty() )
  {
    action_dpet = p -> action_dpet_chart;
    action_dpet += "&dummy=dummy.png";
    simplify_html( action_dpet );
  }
  if ( ! p -> uptimes_and_procs_chart.empty() )
  {
    uptimes_and_procs = p -> uptimes_and_procs_chart;
    uptimes_and_procs += "&dummy=dummy.png";
    simplify_html( uptimes_and_procs );
  }
  if ( ! p -> action_dmg_chart.empty() )
  {
    action_dmg = p -> action_dmg_chart;
    action_dmg += "&dummy=dummy.png";
    simplify_html( action_dmg );
  }
  if ( ! p -> gains_chart.empty() )
  {
    gains = p -> gains_chart;
    gains += "&dummy=dummy.png";
    simplify_html( gains );
  }
  if ( ! p -> timeline_dps_chart.empty() )
  {
    timeline_dps = p -> timeline_dps_chart;
    timeline_dps += "&dummy=dummy.png";
    simplify_html( timeline_dps );
  }
  if ( ! p -> distribution_dps_chart.empty() )
  {
    distribution_dps = p -> distribution_dps_chart;
    distribution_dps += "&dummy=dummy.png";
    simplify_html( distribution_dps );
  }
  if ( ! p -> timeline_resource_chart.empty() )
  {
    timeline_resource = p -> timeline_resource_chart;
    timeline_resource += "&dummy=dummy.png";
    simplify_html( timeline_resource );
  }

  util_t::fprintf( file, "\n" );
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "----\n" );
  util_t::fprintf( file, "= %s =\n", p -> name() );
  util_t::fprintf( file, " * [%s Talents]\n * [%s Origin]\n", p -> talents_str.c_str(), p -> origin_str.c_str() );
  util_t::fprintf( file, "\n" );
  util_t::fprintf( file, "|| %s || %s ||\n", action_dpet.c_str(), uptimes_and_procs.c_str() );
  util_t::fprintf( file, "|| %s || %s ||\n", action_dmg.c_str(),  gains.c_str() );
  util_t::fprintf( file, "|| %s || %s ||\n", timeline_dps.c_str(), distribution_dps.c_str() );
  util_t::fprintf( file, "|| %s || ||\n", timeline_resource.c_str() );
}

// print_wiki_text ===========================================================

static void print_wiki_text( FILE*  file,
                             sim_t* sim )
{
  util_t::fprintf( file, "%s",
                   "\n"
                   "= Raw Text Output =\n"
                   "*Super-Secret Decoder Ring (remember that these values represent the average over all iterations)*\n"
                   " * *DPS=Num:* _Num_ is the _damage per second_\n"
                   " * *DPR=Num:* _Num_ is the _damage per resource_\n"
                   " * *RPS=Num1/Num2:* _Num1_ is the _resource consumed per second_ and _Num2_ is the _resource regenerated per second_\n"
                   " * *Count=Num|Time:* _Num_ is number of casts per fight and _Time_ is average time between casts\n"
                   " * *DPE=Num:* _Num_ is the _damage per execute_\n"
                   " * *DPET=Num:* _Num_ is the _damage per execute_ divided by the _time to execute_ (this value includes GCD costs and Lag in the calculation of _time to execute_)\n"
                   " * *Hit=Num:* _Num_ is the average damage per non-crit hit\n"
                   " * *Crit=Num1|Num2|Pct:* _Num1_ is average crit damage, _Num2_ is the max crit damage, and _Pct_ is the percentage of crits _per execute_ (not _per hit_)\n"
                   " * *Tick=Num:* _Num_ is the average tick of damage for the _damage over time_ portion of actions\n"
                   " * *Up-Time:* This is _not_ the percentage of time the buff/debuff is present, but rather the ratio of _actions it affects_ over _total number of actions it could affect_.  If spell S is cast 10 times and buff B is present for 3 of those casts, then buff B has an up-time of 30%.\n"
                   " * *Waiting*: This is percentage of total time not doing anything (except auto-attack in the case of physical dps classes).  This can occur because the player is resource constrained (Mana, Energy, Rage) or cooldown constrained (as in the case of Enhancement Shaman).\n"
                   "\n" );

  util_t::fprintf( file, "{{{\n" );
  report_t::print_text( file, sim );
  util_t::fprintf( file, "}}}\n" );
}

} // ANONYMOUS NAMESPACE =====================================================

// ===========================================================================
// Report
// ===========================================================================

// report_t::print_text ======================================================

void report_t::print_text( FILE* file, sim_t* sim, bool detail )
{
  if ( sim -> total_seconds == 0 ) return;

  int num_players = sim -> players_by_rank.size();

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

    util_t::fprintf( file, "\nPlayer=%s (%s)  DPS=%.1f (Error=+/-%.1f Range=+/-%.0f)",
                     p -> name(), util_t::talent_tree_string( p -> primary_tree() ),
                     p -> dps, p -> dps_error, ( p -> dps_max - p -> dps_min ) / 2.0 );

    if ( p -> rps_loss > 0 )
    {
      util_t::fprintf( file, "  DPR=%.1f  RS=%.1f/%.1f  (%s)",
                       p -> dpr, p -> rps_loss, p -> rps_gain,
                       util_t::resource_type_string( p -> primary_resource() ) );
    }

    util_t::fprintf( file, "\n" );
    util_t::fprintf( file, "  Origin: %s\n", p -> origin_str.c_str() );
    util_t::fprintf( file, "  Race: %s\n",   p -> race_str.c_str() );
    util_t::fprintf( file, "  Level: %d\n",  p -> level );

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
      print_scale_factors( file, p );
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
  int num_players = sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> total_seconds == 0 ) return;
  if ( sim -> html_file_str.empty() ) return;

  FILE* file = fopen( sim -> html_file_str.c_str(), "w" );
  if ( ! file )
  {
    util_t::fprintf( stderr, "simcraft: Unable to open html file '%s'\n", sim -> html_file_str.c_str() );
    exit( 0 );
  }

  util_t::fprintf( file, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n" );
  util_t::fprintf( file, "<html>\n" );

  util_t::fprintf( file, "<head>\n" );
  util_t::fprintf( file, "<title>Simulationcraft Results</title>\n" );

  if ( num_players > 1 ) print_html_menu_definition( file, sim );

  util_t::fprintf( file, "</head>\n" );
  util_t::fprintf( file, "<body>\n" );

  if ( num_players > 1 ) print_html_raid( file, sim );

  print_html_scale_factors( file, sim );

  if ( num_players > 1 ) print_html_menu_triggers( file, sim );

  util_t::fprintf( file, "<div id=\"players\">\n" );

  for ( int i=0; i < num_players; i++ )
  {
    print_html_player( file, sim -> players_by_name[ i ] );
  }

  util_t::fprintf( file, "</div>\n" );

  print_html_text( file, sim );

  util_t::fprintf( file, "</body>\n" );
  util_t::fprintf( file, "</html>" );

  fclose( file );
}

// report_t::print_wiki ======================================================

void report_t::print_wiki( sim_t* sim )
{
  int num_players = sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> total_seconds == 0 ) return;
  if ( sim -> wiki_file_str.empty() ) return;

  FILE* file = fopen( sim -> wiki_file_str.c_str(), "w" );
  if ( ! file )
  {
    util_t::fprintf( stderr, "simcraft: Unable to open wiki file '%s'\n", sim -> wiki_file_str.c_str() );
    exit( 0 );
  }

  if ( num_players > 1 ) print_wiki_raid( file, sim );

  print_wiki_scale_factors( file, sim );

  for ( int i=0; i < num_players; i++ )
  {
    print_wiki_player( file, sim -> players_by_name[ i ] );
  }

  print_wiki_text( file, sim );

  fclose( file );
}

// report_t::print_xml ======================================================

void report_t::print_xml( sim_t* sim )
{
  int num_players = sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> total_seconds == 0 ) return;
  if ( sim -> xml_file_str.empty() ) return;

  FILE* file = fopen( sim -> xml_file_str.c_str(), "w" );
  if ( ! file )
  {
    util_t::fprintf( stderr, "simcraft: Unable to open xml file '%s'\n", sim -> xml_file_str.c_str() );
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
        util_t::printf( "simcraft: Unable to save gear profile %s for player %s\n", p -> save_gear_str.c_str(), p -> name() );
      }
      else
      {
        p -> save( file, SAVE_GEAR );
        fclose( file );
      }
    }

    if ( !p -> save_talents_str.empty() ) // Save talents
    {
      file = fopen( p -> save_talents_str.c_str(), "w" );
      if ( ! file )
      {
        util_t::printf( "simcraft: Unable to save talents profile %s for player %s\n", p -> save_talents_str.c_str(), p -> name() );
      }
      else
      {
        p -> save( file, SAVE_TALENTS );
        fclose( file );
      }
    }

    if ( !p -> save_actions_str.empty() ) // Save actions
    {
      file = fopen( p -> save_actions_str.c_str(), "w" );
      if ( ! file )
      {
        util_t::printf( "simcraft: Unable to save actions profile %s for player %s\n", p -> save_actions_str.c_str(), p -> name() );
      }
      else
      {
        p -> save( file, SAVE_ACTIONS );
        fclose( file );
      }
    }

    std::string file_name = p -> save_str;

    if ( file_name.empty() && sim -> save_profiles )
    {
      file_name  = "save_";
      file_name += p -> name_str;
      file_name += ".simcraft";
    }

    if ( file_name.empty() ) continue;

    file = fopen( file_name.c_str(), "w" );
    if ( ! file )
    {
      util_t::printf( "simcraft: Unable to save profile %s for player %s\n", file_name.c_str(), p -> name() );
      continue;
    }

    p -> save( file );

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

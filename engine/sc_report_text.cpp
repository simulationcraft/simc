// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

void simplify_html( std::string& buffer )
{
  util_t::replace_all( buffer, "&lt;", '<' );
  util_t::replace_all( buffer, "&gt;", '>' );
  util_t::replace_all( buffer, "&amp;", '&' );
}

// print_text_action ========================================================

static void print_text_action( FILE* file, stats_t* s, int max_name_length=0 )
{
  if ( s -> num_executes == 0 && s -> total_amount.mean == 0 ) return;

  if ( max_name_length == 0 ) max_name_length = 20;

  util_t::fprintf( file,
                   "    %-*s  Count=%5.1f|%5.2fsec  DPE=%6.0f|%2.0f%%  DPET=%6.0f  DPR=%6.1f  pDPS=%4.0f",
                   max_name_length,
                   s -> name_str.c_str(),
                   s -> num_executes,
                   s -> frequency,
                   s -> ape,
                   s -> portion_amount * 100.0,
                   s -> apet,
                   s -> apr,
                   s -> portion_aps.mean );

  if ( s -> num_direct_results > 0 )
  {
    util_t::fprintf( file, "  Miss=%.2f%%", s -> direct_results[ RESULT_MISS ].pct );
  }

  if ( s -> direct_results[ RESULT_HIT ].actual_amount.mean > 0 )
  {
    util_t::fprintf( file, "  Hit=%4.0f|%4.0f|%4.0f",
                     s -> direct_results[ RESULT_HIT ].actual_amount.mean,
                     s -> direct_results[ RESULT_HIT ].actual_amount.min,
                     s -> direct_results[ RESULT_HIT ].actual_amount.max );
  }
  if ( s -> direct_results[ RESULT_CRIT ].actual_amount.mean > 0 )
  {
    util_t::fprintf( file,
                     "  Crit=%5.0f|%5.0f|%5.0f|%.1f%%",
                     s -> direct_results[ RESULT_CRIT ].actual_amount.mean,
                     s -> direct_results[ RESULT_CRIT ].actual_amount.min,
                     s -> direct_results[ RESULT_CRIT ].actual_amount.max,
                     s -> direct_results[ RESULT_CRIT ].pct );
  }
  if ( s -> direct_results[ RESULT_GLANCE ].actual_amount.mean > 0 )
  {
    util_t::fprintf( file,
                     "  Glance=%4.0f|%.1f%%",
                     s -> direct_results[ RESULT_GLANCE ].actual_amount.mean,
                     s -> direct_results[ RESULT_GLANCE ].pct );
  }
  if ( s -> direct_results[ RESULT_DODGE ].count.mean > 0 )
  {
    util_t::fprintf( file,
                     "  Dodge=%.1f%%",
                     s -> direct_results[ RESULT_DODGE ].pct );
  }
  if ( s -> direct_results[ RESULT_PARRY ].count.mean > 0 )
  {
    util_t::fprintf( file,
                     "  Parry=%.1f%%",
                     s -> direct_results[ RESULT_PARRY ].pct );
  }

  if ( s -> num_ticks > 0 ) util_t::fprintf( file, "  TickCount=%.0f", s -> num_ticks );

  if ( s -> tick_results[ RESULT_HIT ].actual_amount.mean > 0 || s -> tick_results[ RESULT_CRIT ].actual_amount.mean > 0 )
  {
    util_t::fprintf( file, "  MissTick=%.1f%%", s -> tick_results[ RESULT_MISS ].pct );
  }

  if ( s -> tick_results[ RESULT_HIT ].avg_actual_amount.mean > 0 )
  {
    util_t::fprintf( file,
                     "  Tick=%.0f|%.0f|%.0f",
                     s -> tick_results[ RESULT_HIT ].actual_amount.mean,
                     s -> tick_results[ RESULT_HIT ].actual_amount.min,
                     s -> tick_results[ RESULT_HIT ].actual_amount.max );
  }
  if ( s -> tick_results[ RESULT_CRIT ].avg_actual_amount.mean > 0 )
  {
    util_t::fprintf( file,
                     "  CritTick=%.0f|%.0f|%.0f|%.1f%%",
                     s -> tick_results[ RESULT_CRIT ].actual_amount.mean,
                     s -> tick_results[ RESULT_CRIT ].actual_amount.min,
                     s -> tick_results[ RESULT_CRIT ].actual_amount.max,
                     s -> tick_results[ RESULT_CRIT ].pct );
  }

  if ( s -> total_tick_time > 0 )
  {
    util_t::fprintf( file, "  UpTime=%.1f%%", 100.0 * s -> total_tick_time / s -> player -> fight_length.mean  );
  }

  util_t::fprintf( file, "\n" );
}

// print_text_actions =======================================================

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
    if ( s -> total_amount.mean > 0 )
      if ( max_length < ( int ) s -> name_str.length() )
        max_length = ( int ) s -> name_str.length();

  for ( stats_t* s = p -> stats_list; s; s = s -> next )
  {
    if ( s -> num_executes > 1 || s -> compound_amount > 0 )
    {
      print_text_action( file, s, max_length );
    }
  }

  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    bool first=true;
    for ( stats_t* s = pet -> stats_list; s; s = s -> next )
    {
      if ( s -> num_executes || s -> compound_amount > 0 )
      {
        if ( first )
        {
          util_t::fprintf( file, "   %s  (DPS=%.1f)\n", pet -> name_str.c_str(), pet -> dps.mean );
          first = false;
        }
        print_text_action( file, s, max_length );
      }
    }
  }
}

// print_text_buffs =========================================================

static inline bool buff_comp( const buff_t* i, const buff_t* j )
{
  // Aura&Buff / Pet
  if ( ( ! i -> player || ! i -> player -> is_pet() ) && j -> player && j -> player -> is_pet() )
    return true;
  // Pet / Aura&Buff
  else if ( i -> player && i -> player -> is_pet() && ( ! j -> player || ! j -> player -> is_pet() ) )
    return false;
  // Pet / Pet
  else if ( i -> player && i -> player -> is_pet() && j -> player && j -> player -> is_pet() )
  {
    if ( i -> player -> name_str.compare( j -> player -> name_str ) == 0 )
      return ( i -> name_str.compare( j -> name_str ) < 0 );
    else
      return ( i -> player -> name_str.compare( j -> player -> name_str ) < 0 );
  }

  return ( i -> name_str.compare( j -> name_str ) < 0 );
}
  
static void print_text_buffs( FILE* file, player_t* p )
{
  bool first=true;
  char prefix = ' ';
  int total_length = 100;
  std::vector< buff_t* > buff_list;
  std::string full_name;
  
  for ( buff_t* b = p -> sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || ! b -> constant )
      continue;
    
    buff_list.push_back( b );
  }
  
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || ! b -> constant )
      continue;
    
    buff_list.push_back( b );
  }
  
  std::sort( buff_list.begin(), buff_list.end(), buff_comp );
  
  for ( std::vector< buff_t* >::const_iterator b = buff_list.begin();
       b < buff_list.end(); b++ )
  {
    int length = ( int ) ( *b ) -> name_str.length();
    if ( ( total_length + length ) > 100 )
    {
      if ( ! first )
        util_t::fprintf( file, "\n" );
      first=false;
      util_t::fprintf( file, "  Constant Buffs:" );
      prefix = ' ';
      total_length = 0;
    }
    util_t::fprintf( file, "%c%s", prefix, ( *b ) -> name() );
    prefix = '/';
    total_length += length;
  }

  util_t::fprintf( file, "\n" );

  buff_list.clear();
  
  int max_length = 0;

  // Consolidate player buffs, first auras
  for ( buff_t* b = p -> sim -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || b -> constant )
      continue;
    
    buff_list.push_back( b );
  }

  // Then player buffs
  for ( buff_t* b = p -> buff_list; b; b = b -> next )
  {
    if ( b -> quiet || ! b -> start_count || b -> constant )
      continue;

    buff_list.push_back( b );
  }
  
  // Then pet buffs
  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    for ( buff_t* b = pet -> buff_list; b; b = b -> next )
    {
      if ( b -> quiet || ! b -> start_count || b -> constant )
        continue;
      
      buff_list.push_back( b );
    }
  }

  for ( std::vector< buff_t* >::const_iterator b = buff_list.begin();
       b < buff_list.end(); b++ )
  {
    if ( ( *b ) -> player && ( *b ) -> player -> is_pet() )
      full_name = ( *b ) -> player -> name_str + "-" + ( *b ) -> name_str;
    else
      full_name = ( *b ) -> name_str;
    
    int length = ( int ) full_name.length();
    if ( length > max_length ) max_length = length;
  }

  std::sort( buff_list.begin(), buff_list.end(), buff_comp );

  if ( buff_list.size() > 0 )
    util_t::fprintf( file, "  Dynamic Buffs:\n" );
  
  for ( std::vector< buff_t* >::const_iterator b = buff_list.begin();
       b < buff_list.end(); b++ )
  {
    if ( ( *b ) -> player && ( *b ) -> player -> is_pet() )
      full_name = ( *b ) -> player -> name_str + "-" + ( *b ) -> name_str;
    else
      full_name = ( *b ) -> name_str;

    util_t::fprintf( file, "    %-*s : start=%-4.1f refresh=%-5.1f interval=%5.1f trigger=%-5.1f uptime=%2.0f%%",
                    max_length, full_name.c_str(), ( *b ) -> avg_start, ( *b ) -> avg_refresh,
                    ( *b ) -> avg_start_interval, ( *b ) -> avg_trigger_interval, ( *b ) -> uptime_pct.mean );
    
    if ( ( *b ) -> benefit_pct > 0 && ( *b ) -> benefit_pct < 100 )
      util_t::fprintf( file, "  benefit=%2.0f%%", ( *b ) -> benefit_pct );
    
    util_t::fprintf( file, "\n" );
  }
}

// print_text_core_stats ====================================================

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

// print_text_spell_stats ===================================================

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

// print_text_attack_stats ==================================================

static void print_text_attack_stats( FILE* file, player_t* p )
{
  util_t::fprintf( file,
                   "  Attack Stats  power=%.0f|%.0f(%.0f)  hit=%.2f%%|%.2f%%(%.0f)  crit=%.2f%%|%.2f%%(%.0f)  expertise=%.2f|%.2f(%.0f)  haste=%.2f%%|%.2f%%(%.0f)  speed=%.2f%%|%.2f%%(%.0f)\n",
                   p -> buffed_attack_power, p -> composite_attack_power() * p -> composite_attack_power_multiplier(), p -> stats.attack_power,
                   100 * p -> buffed_attack_hit,         100 * p -> composite_attack_hit(),         p -> stats.hit_rating,
                   100 * p -> buffed_attack_crit,        100 * p -> composite_attack_crit(),        p -> stats.crit_rating,
                   100 * p -> buffed_attack_expertise,   100 * p -> composite_attack_expertise(),   p -> stats.expertise_rating,
                   100 * ( 1 / p -> buffed_attack_haste - 1 ), 100 * ( 1 / p -> composite_attack_haste() - 1 ), p -> stats.haste_rating,
                   100 * ( 1 / p -> buffed_attack_speed - 1 ), 100 * ( 1 / p -> composite_attack_speed() - 1 ), p -> stats.haste_rating );
}

// print_text_defense_stats =================================================

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

// print_text_gains =========================================================

static void print_text_gains( FILE* file, player_t* p )
{
  int max_length = 0;
  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 || g -> overflow > 0 )
    {
      int length = ( int ) strlen( g -> name() );
      if ( length > max_length ) max_length = length;
    }
  }
  if ( max_length == 0 ) return;

  util_t::fprintf( file, "  Gains:\n" );

  for ( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if ( g -> actual > 0 || g -> overflow > 0 )
    {
      util_t::fprintf( file, "    %8.1f : %-*s", g -> actual, max_length, g -> name() );
      double overflow_pct = 100.0 * g -> overflow / ( g -> actual + g -> overflow );
      if ( overflow_pct > 1.0 ) util_t::fprintf( file, "  (overflow=%.1f%%)", overflow_pct );
      util_t::fprintf( file, "\n" );
    }
  }
}

// print_text_pet_gains =====================================================

static void print_text_pet_gains( FILE* file, player_t* p )
{
  for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    if ( pet -> dmg.mean <= 0 ) continue;

    int max_length = 0;
    for ( gain_t* g = pet -> gain_list; g; g = g -> next )
    {
      if ( g -> actual > 0 || g -> overflow > 0 )
      {
        int length = ( int ) strlen( g -> name() );
        if ( length > max_length ) max_length = length;
      }
    }
    if ( max_length > 0 )
    {
      util_t::fprintf( file, "  Pet \"%s\" Gains:\n", pet -> name_str.c_str() );

      for ( gain_t* g = pet -> gain_list; g; g = g -> next )
      {
        if ( g -> actual > 0 || g -> overflow > 0 )
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

// print_text_procs =========================================================

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

// print_text_uptime ========================================================

static void print_text_uptime( FILE* file, player_t* p )
{
  bool first=true;

  for ( benefit_t* u = p -> benefit_list; u; u = u -> next )
  {
    if ( u -> ratio > 0 )
    {
      if ( first ) util_t::fprintf( file, "  Benefits:\n" ); first = false;
      util_t::fprintf( file, "    %5.1f%% : %-30s\n", u -> ratio * 100.0, u -> name() );
    }
  }

  first=true;
  for ( uptime_t* u = p -> uptime_list; u; u = u -> next )
  {
    if ( u -> uptime > 0 )
    {
      if ( first ) util_t::fprintf( file, "  Up-Times:\n" ); first = false;
      util_t::fprintf( file, "    %5.1f%% : %-30s\n", u -> uptime * 100.0, u -> name() );
    }
  }
}

// print_text_waiting =======================================================

static void print_text_waiting( FILE* file, sim_t* sim )
{
  util_t::fprintf( file, "\nWaiting:\n" );

  bool nobody_waits = true;

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet )
      continue;

    if ( p -> waiting_time.mean )
    {
      nobody_waits = false;
      util_t::fprintf( file, "    %4.1f%% : %s\n", 100.0 * p -> waiting_time.mean / p -> fight_length.mean,  p -> name() );
    }
  }

  if ( nobody_waits ) util_t::fprintf( file, "    All players active 100%% of the time.\n" );
}

// print_text_performance ===================================================

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
                   ( long ) sim -> total_events_processed,
                   ( long ) sim -> max_events_remaining,
                   sim -> target -> resource_base[ RESOURCE_HEALTH ],
                   sim -> iterations * sim -> simulation_length.mean,
                   sim -> elapsed_cpu_seconds,
                   sim -> iterations * sim -> simulation_length.mean / sim -> elapsed_cpu_seconds );

  sim -> rng -> report( file );
}

// print_text_scale_factors =================================================

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

    gear_stats_t& sf = ( sim -> scaling -> normalize_scale_factors ) ? p -> scaling_normalized : p -> scaling;

    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( p -> scales_with[ j ] != 0 )
      {
        util_t::fprintf( file, "  %s=%.*f(%.*f)", util_t::stat_type_abbrev( j ),
                         sim -> report_precision, sf.get_stat( j ),
                         sim -> report_precision, p -> scaling_error.get_stat( j ) );
      }
    }

    if ( sim -> scaling -> normalize_scale_factors )
    {
      util_t::fprintf( file, "  DPS/%s=%.*f", util_t::stat_type_abbrev( p -> normalize_by() ), sim -> report_precision, p -> scaling.get_stat( p -> normalize_by() ) );
    }

    if ( p -> sim -> scaling -> scale_lag ) util_t::fprintf( file, "  ms Lag=%.*f(%.*f)", p -> sim -> report_precision, p -> scaling_lag, p -> sim -> report_precision, p -> scaling_lag_error );

    util_t::fprintf( file, "\n" );
  }
}

// print_text_scale_factors =================================================

static void print_text_scale_factors( FILE* file, player_t* p )
{
  if ( ! p -> sim -> scaling -> has_scale_factors() ) return;

  if ( p -> sim -> report_precision < 0 )
    p -> sim -> report_precision = 2;

  util_t::fprintf( file, "  Scale Factors:\n" );

  gear_stats_t& sf = ( p -> sim -> scaling -> normalize_scale_factors ) ? p -> scaling_normalized : p -> scaling;

  util_t::fprintf( file, "    Weights :" );
  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( p -> scales_with[ i ] != 0 )
    {
      util_t::fprintf( file, "  %s=%.*f(%.*f)", util_t::stat_type_abbrev( i ),
                       p -> sim -> report_precision, sf.get_stat( i ),
                       p -> sim -> report_precision, p -> scaling_error.get_stat( i ) );;
    }
  }
  if ( p -> sim -> scaling -> normalize_scale_factors )
  {
    util_t::fprintf( file, "  DPS/%s=%.*f", util_t::stat_type_abbrev( p -> normalize_by() ), p -> sim -> report_precision, p -> scaling.get_stat( p -> normalize_by() ) );
  }
  if ( p -> sim -> scaling -> scale_lag ) util_t::fprintf( file, "  ms Lag=%.*f(%.*f)", p -> sim -> report_precision, p -> scaling_lag, p -> sim -> report_precision, p -> scaling_lag_error );

  util_t::fprintf( file, "\n" );


  std::string lootrank   = p -> gear_weights_lootrank_link;
  std::string wowhead    = p -> gear_weights_wowhead_link;
  std::string wowreforge = p -> gear_weights_wowreforge_link;
  std::string pawn_std   = p -> gear_weights_pawn_std_string;
  std::string pawn_alt   = p -> gear_weights_pawn_alt_string;

  simplify_html( lootrank   );
  simplify_html( wowhead    );
  simplify_html( wowreforge );
  simplify_html( pawn_std   );
  simplify_html( pawn_alt   );

  util_t::fprintf( file, "    Wowhead : %s\n", wowhead.c_str() );
}

// print_text_dps_plots =====================================================

static void print_text_dps_plots( FILE* file, player_t* p )
{
  sim_t* sim = p -> sim;

  if ( sim -> plot -> dps_plot_stat_str.empty() ) return;

  int range = sim -> plot -> dps_plot_points / 2;

  double min = -range * sim -> plot -> dps_plot_step;
  double max = +range * sim -> plot -> dps_plot_step;

  int points = 1 + range * 2;

  util_t::fprintf( file, "  DPS Plot Data ( min=%.1f max=%.1f points=%d )\n", min, max, points );

  for ( int i=0; i < STAT_MAX; i++ )
  {
    std::vector<double>& pd = p -> dps_plot_data[ i ];

    if ( ! pd.empty() )
    {
      util_t::fprintf( file, "    DPS(%s)=", util_t::stat_type_abbrev( i ) );
      size_t num_points = pd.size();
      for ( size_t j=0; j < num_points; j++ )
      {
        util_t::fprintf( file, "%s%.0f", ( j?"|":"" ), pd[ j ] );
      }
      util_t::fprintf( file, "\n" );
    }
  }
}

// print_text_reference_dps =================================================

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

  int num_players = ( int ) sim -> players_by_dps.size();
  int max_length=0;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_dps[ i ];
    int length = ( int ) strlen( p -> name() );
    if ( length > max_length ) max_length = length;
  }

  util_t::fprintf( file, "  %-*s", max_length, ref_p -> name() );
  util_t::fprintf( file, "  %.0f", ref_p -> dps.mean );

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
    player_t* p = sim -> players_by_dps[ i ];

    if ( p != ref_p )
    {
      util_t::fprintf( file, "  %-*s", max_length, p -> name() );

      bool over = ( p -> dps.mean > ref_p -> dps.mean );

      double ratio = 100.0 * fabs( p -> dps.mean - ref_p -> dps.mean ) / ref_p -> dps.mean;

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

namespace {

struct compare_hat_donor_interval
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return ( l -> procs.hat_donor -> frequency < r -> procs.hat_donor -> frequency );
  }
};

}

static void print_text_hat_donors( FILE* file, sim_t* sim )
{
  std::vector<player_t*> hat_donors;

  int num_players = ( int ) sim -> players_by_name.size();
  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    if ( p -> procs.hat_donor -> count )
      hat_donors.push_back( p );
  }

  int num_donors = ( int ) hat_donors.size();
  if ( num_donors )
  {
    range::sort( hat_donors, compare_hat_donor_interval()  );

    util_t::fprintf( file, "\nHonor Among Thieves Donor Report:\n" );

    for ( int i=0; i < num_donors; i++ )
    {
      player_t* p = hat_donors[ i ];
      proc_t* proc = p -> procs.hat_donor;
      util_t::fprintf( file, "  %.2fsec | %.3fcps : %s\n", proc -> frequency, ( 1.0 / proc -> frequency ), p -> name() );
    }
  }
}

// print_text_player ========================================================

static void print_text_player( FILE* file, player_t* p )
{
  util_t::fprintf( file, "\n%s: %s %s %s %s %d\n",
                   p -> is_enemy() ? "Target" : p -> is_add() ? "Add" : "Player",
                   p -> name(), p -> race_str.c_str(),
                   util_t::player_type_string( p -> type ),
                   util_t::talent_tree_string( p -> primary_tree() ), p -> level );

  util_t::fprintf( file, "  DPS: %.1f  DPS-Error=%.1f/%.1f%% HPS: %.1f HPS-Error=%.1f/%.1f%% DPS-Range=%.0f/%.1f%%  DPS-Convergence=%.1f%%",
                   p -> dps.mean,
                   p -> dps_error, p -> dps.mean ? p -> dps_error * 100 / p -> dps.mean : 0,
                   p -> hps.mean,
                   p -> hps_error, p -> hps.mean ? p -> hps_error * 100 / p -> hps.mean : 0,
                   ( p -> dps.max - p -> dps.min ) / 2.0 , p -> dps.mean ? ( ( p -> dps.max - p -> dps.min ) / 2 ) * 100 / p -> dps.mean : 0,
                   p -> dps_convergence * 100 );

  if ( p -> rps_loss > 0 )
  {
    util_t::fprintf( file, "  DPR=%.1f  RPS-Out=%.1f RPS-In=%.1f  Resource=(%s) Waiting=%.1f ApM=%.1f",
                     p -> dpr, p -> rps_loss, p -> rps_gain,
                     util_t::resource_type_string( p -> primary_resource() ), 100.0 * p -> waiting_time.mean / p -> fight_length.mean, 60.0 * p -> executed_foreground_actions.mean / p -> fight_length.mean  );
  }

  util_t::fprintf( file, "\n" );
  if ( p -> origin_str.compare( "unknown" ) ) util_t::fprintf( file, "  Origin: %s\n", p -> origin_str.c_str() );
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

} // ANONYMOUS NAMESPACE ====================================================



// report_t::print_text =====================================================

void report_t::print_text( FILE* file, sim_t* sim, bool detail )
{
  if ( sim -> simulation_length.mean == 0 ) return;

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

  int num_players = ( int ) sim -> players_by_dps.size();

  if ( detail )
  {
    util_t::fprintf( file, "\nDPS Ranking:\n" );
    util_t::fprintf( file, "%7.0f 100.0%%  Raid\n", sim -> raid_dps.mean );
    for ( int i=0; i < num_players; i++ )
    {
      player_t* p = sim -> players_by_dps[ i ];
      if ( p -> dps.mean <= 0 ) continue;
      util_t::fprintf( file, "%7.0f  %4.1f%%  %s\n", p -> dps.mean, sim -> raid_dps.mean ? 100 * p -> dpse.mean / sim -> raid_dps.mean : 0, p -> name() );
    }

    if ( sim -> players_by_hps.size() > 0 )
    {
      util_t::fprintf( file, "\nHPS Ranking:\n" );
      util_t::fprintf( file, "%7.0f 100.0%%  Raid\n", sim -> raid_hps.mean );
      for ( size_t i=0; i < sim -> players_by_hps.size(); i++ )
      {
        player_t* p = sim -> players_by_hps[ i ];
        if ( p -> hps.mean <= 0 ) continue;
        util_t::fprintf( file, "%7.0f  %4.1f%%  %s\n", p -> hps.mean, sim -> raid_hps.mean ? 100 * p -> hpse.mean / sim -> raid_hps.mean : 0, p -> name() );
      }
    }
  }

  // Report Players
  for ( int i=0; i < num_players; i++ )
  {
    print_text_player( file, sim -> players_by_name[ i ] );

    // Pets
    if ( sim -> report_pets_separately )
    {
      for ( pet_t* pet = sim -> players_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
      {
        if ( pet -> summoned )
          print_text_player( file, pet );
      }
    }
  }

  // Report Targets
  if ( sim -> report_targets )
  {
    util_t::fprintf( file, "\n\n *** Targets *** \n\n" );

    for ( int i=0; i < ( int ) sim -> targets_by_name.size(); i++ )
    {
      print_text_player( file, sim -> targets_by_name[ i ] );

      // Pets
      if ( sim -> report_pets_separately )
      {
        for ( pet_t* pet = sim -> players_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
        {
          if ( pet -> summoned )
            print_text_player( file, pet );
        }
      }
    }
  }

  if ( detail )
  {
    print_text_hat_donors   ( file, sim );
    print_text_waiting      ( file, sim );
    print_text_performance  ( file, sim );
    print_text_scale_factors( file, sim );
    print_text_reference_dps( file, sim );
  }

  util_t::fprintf( file, "\n" );
}

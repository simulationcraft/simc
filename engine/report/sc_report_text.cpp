// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

struct compare_hat_donor_interval
{
  bool operator()( const player_t* l, const player_t* r ) const
  {
    return ( l -> procs.hat_donor -> interval_sum.mean() < r -> procs.hat_donor -> interval_sum.mean() );
  }
};

void simplify_html( std::string& buffer )
{
  util::replace_all( buffer, "&lt;", "<" );
  util::replace_all( buffer, "&gt;", ">" );
  util::replace_all( buffer, "&amp;", "&" );
}

// print_text_action ========================================================

void print_text_action( FILE* file, stats_t* s, int max_name_length, int max_dpe, int max_dpet, int max_dpr, int max_pdps )
{
  if ( s -> num_executes.sum() == 0 && s -> total_amount.sum() == 0 ) return;

  if ( max_name_length == 0 ) max_name_length = 20;

  util::fprintf( file,
                 "    %-*s  Count=%5.1f|%6.2fsec  DPE=%*.0f|%2.0f%%  DPET=%*.0f  DPR=%*.1f  pDPS=%*.0f",
                 max_name_length,
                 s -> name_str.c_str(),
                 s -> num_executes.mean(),
                 s -> total_intervals.pretty_mean(),
                 max_dpe,
                 s -> ape,
                 s -> portion_amount * 100.0,
                 max_dpet,
                 s -> apet,
                 ( max_dpr + 2 ),
                 s -> apr[ s -> player -> primary_resource() ],
                 max_pdps,
                 s -> portion_aps.mean() );

  if ( s -> num_direct_results.mean() > 0 )
  {
    util::fprintf( file, "  Miss=%.2f%%", s -> direct_results[ RESULT_MISS ].pct );
  }

  if ( s -> direct_results[ RESULT_HIT ].actual_amount.sum() > 0 )
  {
    util::fprintf( file, "  Hit=%4.0f|%4.0f|%4.0f",
                   s -> direct_results[ RESULT_HIT ].actual_amount.mean(),
                   s -> direct_results[ RESULT_HIT ].actual_amount.min(),
                   s -> direct_results[ RESULT_HIT ].actual_amount.max() );
  }
  if ( s -> direct_results[ RESULT_CRIT ].actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  Crit=%5.0f|%5.0f|%5.0f|%.1f%%",
                   s -> direct_results[ RESULT_CRIT ].actual_amount.mean(),
                   s -> direct_results[ RESULT_CRIT ].actual_amount.min(),
                   s -> direct_results[ RESULT_CRIT ].actual_amount.max(),
                   s -> direct_results[ RESULT_CRIT ].pct );
  }
  if ( s -> direct_results[ RESULT_MULTISTRIKE ].actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  Mult=%.1f|%5.0f|%5.0f|%5.0f|%.1f%%",
                   s -> direct_results[ RESULT_MULTISTRIKE ].count.mean(),
                   s -> direct_results[ RESULT_MULTISTRIKE ].actual_amount.mean(),
                   s -> direct_results[ RESULT_MULTISTRIKE ].actual_amount.min(),
                   s -> direct_results[ RESULT_MULTISTRIKE ].actual_amount.max(),
                   s -> direct_results[ RESULT_MULTISTRIKE ].pct );
  }
  if ( s -> direct_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  MultCrit=%.1f|%5.0f|%5.0f|%5.0f|%.1f%%",
                   s -> direct_results[ RESULT_MULTISTRIKE_CRIT ].count.mean(),
                   s -> direct_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.mean(),
                   s -> direct_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.min(),
                   s -> direct_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.max(),
                   s -> direct_results[ RESULT_MULTISTRIKE_CRIT ].pct );
  }
  if ( s -> direct_results[ RESULT_GLANCE ].actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  Glance=%4.0f|%.1f%%",
                   s -> direct_results[ RESULT_GLANCE ].actual_amount.pretty_mean(),
                   s -> direct_results[ RESULT_GLANCE ].pct );
  }
  if ( s -> direct_results[ RESULT_DODGE ].count.sum() > 0 )
  {
    util::fprintf( file,
                   "  Dodge=%.1f%%",
                   s -> direct_results[ RESULT_DODGE ].pct );
  }
  if ( s -> direct_results[ RESULT_PARRY ].count.sum() > 0 )
  {
    util::fprintf( file,
                   "  Parry=%.1f%%",
                   s -> direct_results[ RESULT_PARRY ].pct );
  }

  if ( s -> num_ticks.sum() > 0 ) util::fprintf( file, "  TickCount=%.0f", s -> num_ticks.mean() );

  if ( s -> tick_results[ RESULT_HIT ].actual_amount.sum() > 0 || s -> tick_results[ RESULT_CRIT ].actual_amount.sum() > 0 )
  {
    util::fprintf( file, "  MissTick=%.1f%%", s -> tick_results[ RESULT_MISS ].pct );
  }

  if ( s -> tick_results[ RESULT_HIT ].avg_actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  Tick=%.0f|%.0f|%.0f",
                   s -> tick_results[ RESULT_HIT ].actual_amount.mean(),
                   s -> tick_results[ RESULT_HIT ].actual_amount.min(),
                   s -> tick_results[ RESULT_HIT ].actual_amount.max() );
  }
  if ( s -> tick_results[ RESULT_CRIT ].avg_actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  CritTick=%.0f|%.0f|%.0f|%.1f%%",
                   s -> tick_results[ RESULT_CRIT ].actual_amount.mean(),
                   s -> tick_results[ RESULT_CRIT ].actual_amount.min(),
                   s -> tick_results[ RESULT_CRIT ].actual_amount.max(),
                   s -> tick_results[ RESULT_CRIT ].pct );
  }
  if ( s -> tick_results[ RESULT_MULTISTRIKE ].actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  MultTick=%5.0f|%5.0f|%5.0f|%.1f%%",
                   s -> tick_results[ RESULT_MULTISTRIKE ].actual_amount.mean(),
                   s -> tick_results[ RESULT_MULTISTRIKE ].actual_amount.min(),
                   s -> tick_results[ RESULT_MULTISTRIKE ].actual_amount.max(),
                   s -> tick_results[ RESULT_MULTISTRIKE ].pct );
  }
  if ( s -> tick_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.sum() > 0 )
  {
    util::fprintf( file,
                   "  MultCritTick=%5.0f|%5.0f|%5.0f|%.1f%%",
                   s -> tick_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.mean(),
                   s -> tick_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.min(),
                   s -> tick_results[ RESULT_MULTISTRIKE_CRIT ].actual_amount.max(),
                   s -> tick_results[ RESULT_MULTISTRIKE_CRIT ].pct );
  }

  if ( s -> total_tick_time.sum() > 0.0 )
  {
    util::fprintf( file, "  UpTime=%.1f%%", 100.0 * s -> total_tick_time.mean() / s -> player -> collected_data.fight_length.mean()  );
  }

  util::fprintf( file, "\n" );
}

// print_text_actions =======================================================

void print_text_actions( FILE* file, player_t* p )
{
  if ( ! p -> glyphs_str.empty() ) util::fprintf( file, "  Glyphs: %s\n", p -> glyphs_str.c_str() );

  for ( unsigned int idx = 0; idx < p -> action_priority_list.size(); idx++ )
  {
    action_priority_list_t* alist = p -> action_priority_list[ idx ];

    if ( alist -> used )
    {
      util::fprintf( file, "  Priorities%s:\n", ( alist -> name_str == "default" ) ? "" : ( " (actions." + alist -> name_str + ")" ).c_str() );

      int length = 0;
      for ( size_t i = 0; i < alist -> action_list.size(); i++ )
      {
        if ( length > 80 || ( length > 0 && ( length + alist -> action_list[ i ].action_.size() ) > 80 ) )
        {
          util::fprintf( file, "\n" );
          length = 0;
        }
        util::fprintf( file, "%s%s", ( ( length > 0 ) ? "/" : "    " ), alist -> action_list[ i ].action_.c_str() );
        length += ( int ) alist -> action_list[ i ].action_.size();
      }
      util::fprintf( file, "\n" );
    }
  }

  util::fprintf( file, "  Actions:\n" );

  int max_length = 0;
  int max_dpe = 0, max_dpet = 0, max_dpr = 0, max_pdps = 0;
  std::vector<stats_t*> tmp_stats_list = p -> stats_list;
  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
    tmp_stats_list.insert( tmp_stats_list.end(), p -> pet_list[ i ] -> stats_list.begin(), p -> pet_list[ i ] -> stats_list.end() );
  for ( size_t i = 0; i < tmp_stats_list.size(); ++i )
  {
    stats_t* s = tmp_stats_list[ i ];
    if ( s -> total_amount.mean() > 0 )
    {
      if ( max_length < ( int ) s -> name_str.length() )
        max_length = ( int ) s -> name_str.length();

      if ( max_dpe < util::numDigits( static_cast<int32_t>( s -> ape ) ) )
        max_dpe = util::numDigits( static_cast<int32_t>( s -> ape ) ) ;

      if ( max_dpet < util::numDigits( static_cast<int32_t>( s -> apet ) )  )
        max_dpet = util::numDigits( static_cast<int32_t>( s -> apet ) ) ;

      if ( max_dpr < util::numDigits( static_cast<int32_t>( s -> apr[ s -> player -> primary_resource() ] ) ) )
        max_dpr = util::numDigits( static_cast<int32_t>( s -> apr[ s -> player -> primary_resource() ] ) );

      if ( max_pdps < util::numDigits( static_cast<int32_t>( s -> portion_aps.mean() ) ) )
        max_pdps = util::numDigits( static_cast<int32_t>( s -> portion_aps.mean() ) );
    }
  }

  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    if ( s -> num_executes.mean() > 1 || s -> compound_amount > 0 )
    {
      print_text_action( file, s, max_length, max_dpe, max_dpet, max_dpr, max_pdps );
    }
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    bool first = true;
    for ( size_t j = 0; j < pet -> stats_list.size(); ++j )
    {
      stats_t* s = pet -> stats_list[ j ];
      if ( s -> num_executes.mean() > 1 || s -> compound_amount > 0 )
      {
        if ( first )
        {
          util::fprintf( file, "   %s  (DPS=%.1f)\n", pet -> name_str.c_str(), pet -> collected_data.dps.mean() );
          first = false;
        }
        print_text_action( file, s, max_length, max_dpe, max_dpet, max_dpr, max_pdps );
      }
    }
  }
}

// print_text_buffs =========================================================


void print_text_buffs( FILE* file, player_processed_report_information_t& ri )
{
  bool first = true;
  char prefix = ' ';
  int total_length = 100;
  std::string full_name;

  for ( size_t i = 0; i < ri.constant_buffs.size(); i++ )
  {
    buff_t* b = ri.constant_buffs[ i ];
    int length = ( int ) b -> name_str.length();
    if ( ( total_length + length ) > 100 )
    {
      if ( ! first )
        util::fprintf( file, "\n" );
      first = false;
      util::fprintf( file, "  Constant Buffs:" );
      prefix = ' ';
      total_length = 0;
    }
    util::fprintf( file, "%c%s", prefix, b -> name_str.c_str() );
    prefix = '/';
    total_length += length;
  }

  util::fprintf( file, "\n" );


  int max_length = 0;


  for ( size_t i = 0; i < ri.dynamic_buffs.size(); i++ )
  {
    buff_t* b = ri.dynamic_buffs[ i ];
    if ( b -> player && b -> player -> is_pet() )
      full_name = b -> player -> name_str + "-" + b -> name_str;
    else
      full_name = b -> name_str;

    int length = ( int ) full_name.length();
    if ( length > max_length ) max_length = length;
  }


  if ( ri.dynamic_buffs.size() > 0 )
    util::fprintf( file, "  Dynamic Buffs:\n" );

  for ( size_t i = 0; i < ri.dynamic_buffs.size(); i++ )
  {
    buff_t* b = ri.dynamic_buffs[ i ];
    if ( b -> player && b -> player -> is_pet() )
      full_name = b -> player -> name_str + "-" + b -> name_str;
    else
      full_name = b -> name_str;

    util::fprintf( file, "    %-*s : start=%4.1f refresh=%5.1f interval=%5.1f trigger=%5.1f uptime=%2.2f%%",
                   max_length, full_name.c_str(), b -> avg_start.mean(), b -> avg_refresh.mean(),
                   b -> start_intervals.pretty_mean(), b -> trigger_intervals.pretty_mean(), b -> uptime_pct.pretty_mean() );

    if ( b -> benefit_pct.sum() > 0 && b -> benefit_pct.mean() < 100 )
      util::fprintf( file, "  benefit=%2.0f%%", b -> benefit_pct.mean() );

    util::fprintf( file, "\n" );
  }
}

// print_text_core_stats ====================================================

void print_text_core_stats( FILE* file, player_t* p )
{
  player_collected_data_t::buffed_stats_t& buffed_stats = p -> collected_data.buffed_stats_snapshot;

  util::fprintf( file,
                 "  Core Stats:    strength=%.0f|%.0f(%.0f)  agility=%.0f|%.0f(%.0f)  stamina=%.0f|%.0f(%.0f)  intellect=%.0f|%.0f(%.0f)  spirit=%.0f|%.0f(%.0f)  mastery=%.2f%%|%.2f%%(%.0f)  multistrike=%.2f%%|%.2f%%(%.0f) health=%.0f|%.0f  mana=%.0f|%.0f\n",
                 buffed_stats.attribute[ ATTR_STRENGTH  ], p -> strength(),  p -> initial.stats.get_stat( STAT_STRENGTH  ),
                 buffed_stats.attribute[ ATTR_AGILITY   ], p -> agility(),   p -> initial.stats.get_stat( STAT_AGILITY   ),
                 buffed_stats.attribute[ ATTR_STAMINA   ], p -> stamina(),   p -> initial.stats.get_stat( STAT_STAMINA   ),
                 buffed_stats.attribute[ ATTR_INTELLECT ], p -> intellect(), p -> initial.stats.get_stat( STAT_INTELLECT ),
                 buffed_stats.attribute[ ATTR_SPIRIT    ], p -> spirit(),    p -> initial.stats.get_stat( STAT_SPIRIT    ),
                 100.0 * buffed_stats.mastery_value , 100.0 * p -> cache.mastery_value(), p -> initial.stats.get_stat( STAT_MASTERY_RATING ),
                 100.0 * buffed_stats.multistrike , 100.0 * p -> cache.multistrike(), p -> initial.stats.get_stat( STAT_MULTISTRIKE_RATING ),
                 buffed_stats.resource[ RESOURCE_HEALTH ], p -> resources.max[ RESOURCE_HEALTH ],
                 buffed_stats.resource[ RESOURCE_MANA   ], p -> resources.max[ RESOURCE_MANA   ] );
}

// print_text_spell_stats ===================================================

void print_text_spell_stats( FILE* file, player_t* p )
{
  player_collected_data_t::buffed_stats_t& buffed_stats = p -> collected_data.buffed_stats_snapshot;

  util::fprintf( file,
                 "  Spell Stats:   power=%.0f|%.0f(%.0f)  hit=%.2f%%|%.2f%%(%.0f)  crit=%.2f%%|%.2f%%(%.0f)  haste=%.2f%%|%.2f%%(%.0f)  speed=%.2f%%|%.2f%%(%.0f)  versatility=%.2f%%|%.2f%%(%.0f)  manareg=%.0f|%.0f(%d)\n",
                 buffed_stats.spell_power, p -> composite_spell_power( SCHOOL_MAX ) * p -> composite_spell_power_multiplier(), p -> initial.stats.spell_power,
                 100 * buffed_stats.spell_hit,          100 * p -> composite_spell_hit(),          p -> initial.stats.hit_rating,
                 100 * buffed_stats.spell_crit,         100 * p -> composite_spell_crit(),         p -> initial.stats.crit_rating,
                 100 * ( 1 / buffed_stats.spell_haste - 1 ), 100 * ( 1 / p -> composite_spell_speed() - 1 ), p -> initial.stats.haste_rating,
                 100 * ( 1 / buffed_stats.spell_speed - 1 ), 100 * ( 1 / p -> cache.spell_speed() - 1 ), p -> initial.stats.haste_rating,
                 100 * buffed_stats.damage_versatility, 100 * p -> composite_damage_versatility(), p -> initial.stats.versatility_rating,
                 buffed_stats.manareg_per_second, p -> mana_regen_per_second(), 0 );
}

// print_text_attack_stats ==================================================

void print_text_attack_stats( FILE* file, player_t* p )
{
  player_collected_data_t::buffed_stats_t& buffed_stats = p -> collected_data.buffed_stats_snapshot;

  if ( p -> dual_wield() )
    util::fprintf( file,
                   "  Attack Stats:  power=%.0f|%.0f(%.0f)  hit=%.2f%%|%.2f%%(%.0f)  crit=%.2f%%|%.2f%%(%.0f)  expertise=%.2f%%/%.2f%%|%.2f%%/%.2f%%(%.0f)  haste=%.2f%%|%.2f%%(%.0f)  speed=%.2f%%|%.2f%%(%.0f)  versatility=%.2f%%|%.2f%%(%.0f)\n",
                   buffed_stats.attack_power, p -> composite_melee_attack_power() * p -> composite_attack_power_multiplier(), p -> initial.stats.attack_power,
                   100 * buffed_stats.attack_hit,         100 * p -> composite_melee_hit(),         p -> initial.stats.hit_rating,
                   100 * buffed_stats.attack_crit,        100 * p -> composite_melee_crit(),        p -> initial.stats.crit_rating,
                   100 * buffed_stats.mh_attack_expertise,   100 * p -> composite_melee_expertise( &( p -> main_hand_weapon ) ),
                   100 * buffed_stats.oh_attack_expertise,   100 * p -> composite_melee_expertise( &( p -> off_hand_weapon ) ),
                   p -> current.stats.expertise_rating,
                   100 * ( 1 / buffed_stats.attack_haste - 1 ), 100 * ( 1 / p -> composite_melee_haste() - 1 ), p -> initial.stats.haste_rating,
                   100 * ( 1 / buffed_stats.attack_speed - 1 ), 100 * ( 1 / p -> composite_melee_speed() - 1 ), p -> initial.stats.haste_rating,
                   100 * buffed_stats.damage_versatility, 100 * p -> composite_damage_versatility(), p -> initial.stats.versatility_rating );
  else
    util::fprintf( file,
                   "  Attack Stats:  power=%.0f|%.0f(%.0f)  hit=%.2f%%|%.2f%%(%.0f)  crit=%.2f%%|%.2f%%(%.0f)  expertise=%.2f%%|%.2f%%(%.0f)  haste=%.2f%%|%.2f%%(%.0f)  speed=%.2f%%|%.2f%%(%.0f)  versatility=%.2f%%|%.2f%%(%.0f)\n",
                   buffed_stats.attack_power, p -> composite_melee_attack_power() * p -> composite_attack_power_multiplier(), p -> initial.stats.attack_power,
                   100 * buffed_stats.attack_hit,         100 * p -> composite_melee_hit(),         p -> initial.stats.hit_rating,
                   100 * buffed_stats.attack_crit,        100 * p -> composite_melee_crit(),        p -> initial.stats.crit_rating,
                   100 * buffed_stats.mh_attack_expertise,   100 * p -> composite_melee_expertise( &( p -> main_hand_weapon ) ),
                   p -> current.stats.expertise_rating,
                   100 * ( 1 / buffed_stats.attack_haste - 1 ), 100 * ( 1 / p -> composite_melee_haste() - 1 ), p -> initial.stats.haste_rating,
                   100 * ( 1 / buffed_stats.attack_speed - 1 ), 100 * ( 1 / p -> composite_melee_speed() - 1 ), p -> initial.stats.haste_rating,
                   100 * buffed_stats.damage_versatility, 100 * p -> composite_damage_versatility(), p -> initial.stats.versatility_rating );
}

// print_text_defense_stats =================================================

void print_text_defense_stats( FILE* file, player_t* p )
{
  player_collected_data_t::buffed_stats_t& buffed_stats = p -> collected_data.buffed_stats_snapshot;

  util::fprintf( file,
                 "  Defense Stats: armor=%.0f|%.0f(%.0f) miss=%.2f%%|%.2f%%  dodge=%.2f%%|%.2f%%(%.0f)  parry=%.2f%%|%.2f%%(%.0f)  block=%.2f%%|%.2f%%(%.0f) crit=%.2f%%|%.2f%%  versatility=%.2f%%|%.2f%%(%.0f)\n",
                 buffed_stats.armor,       p -> composite_armor(), p -> initial.stats.armor,
                 100 * buffed_stats.miss,  100 * ( p -> cache.miss() ),
                 100 * buffed_stats.dodge, 100 * ( p -> cache.dodge() ), p -> initial.stats.dodge_rating,
                 100 * buffed_stats.parry, 100 * ( p -> cache.parry() ), p -> initial.stats.parry_rating,
                 100 * buffed_stats.block, 100 * p -> composite_block(), p -> initial.stats.block_rating,
                 100 * buffed_stats.crit,  100 * p -> cache.crit_avoidance(),
                 100 * buffed_stats.mitigation_versatility, 100 * p -> composite_mitigation_versatility(), p -> initial.stats.versatility_rating );
}

void print_text_gains( FILE* file, gain_t* g, int max_length )
{

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( g -> actual[ i ] > 0 || g -> overflow[ i ] > 0 )
    {
      util::fprintf( file, "    %8.1f : %-*s (%s)", g -> actual[ i ], max_length, g -> name(),  util::resource_type_string( i ) );
      double overflow_pct = 100.0 * g -> overflow[ i ] / ( g -> actual[ i ] + g -> overflow[ i ] );
      if ( overflow_pct > 1.0 ) util::fprintf( file, "  (overflow=%.1f%%)", overflow_pct );
      util::fprintf( file, "\n" );
    }
  }
}
// print_text_gains =========================================================

void print_text_player_gains( FILE* file, player_t* p )
{
  int max_length = 0;
  for ( size_t i = 0; i < p -> gain_list.size(); ++i )
  {
    gain_t* g = p -> gain_list[ i ];
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; r++ )
    {
      if ( g -> actual[ r ] > 0 || g -> overflow[ r ] > 0 )
      {
        int length = ( int ) strlen( g -> name() );
        if ( length > max_length ) max_length = length;
      }
    }
  }
  if ( max_length == 0 ) return;

  util::fprintf( file, "  Gains:\n" );

  for ( size_t i = 0; i < p -> gain_list.size(); ++i )
  {
    gain_t* g = p -> gain_list[ i ];
    print_text_gains( file, g, max_length );
  }
}

// print_text_pet_gains =====================================================

void print_text_pet_gains( FILE* file, player_t* p )
{
  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    if ( pet -> collected_data.dmg.mean() <= 0 ) continue;

    int max_length = 0;
    for ( size_t j = 0; j < pet -> gain_list.size(); ++j)
    {
      gain_t* g = pet -> gain_list[ j ];
      for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; r++ )
      {
        if ( g -> actual[ r ] > 0 || g -> overflow[ r ] > 0 )
        {
          int length = ( int ) strlen( g -> name() );
          if ( length > max_length ) max_length = length;
        }
      }
    }
    if ( max_length > 0 )
    {
      util::fprintf( file, "    Pet \"%s\" Gains:\n", pet -> name_str.c_str() );

      for ( size_t m = 0; m < pet -> gain_list.size(); ++m )
      {
        gain_t* g = pet -> gain_list[ m ];
        print_text_gains( file, g, max_length );
      }
    }
  }
}

// print_text_procs =========================================================

void print_text_procs( FILE* file, player_t* p )
{
  bool first = true;

  for ( size_t i = 0; i < p -> proc_list.size(); ++i )
  {
    proc_t* proc = p -> proc_list[ i ];
    if ( proc -> count.sum() > 0 )
    {
      if ( first ) util::fprintf( file, "  Procs:\n" ); first = false;
      util::fprintf( file, "    %5.1f | %6.2fsec : %s\n",
                     proc -> count.mean(), proc -> interval_sum.pretty_mean(), proc -> name() );
    }
  }
}

// print_text_uptime ========================================================

void print_text_uptime( FILE* file, player_t* p )
{
  bool first = true;

  for ( size_t j = 0; j < p -> benefit_list.size(); ++j )
  {
    benefit_t* u = p -> benefit_list[ j ];
    if ( u -> ratio.mean() > 0 )
    {
      if ( first ) util::fprintf( file, "  Benefits:\n" ); first = false;
      util::fprintf( file, "    %5.1f%% : %-30s\n", u -> ratio.mean(), u -> name() );
    }
  }

  first = true;
  for ( size_t j = 0; j < p -> uptime_list.size(); ++j )
  {
    uptime_t* u = p -> uptime_list[ j ];
    if ( u -> uptime_sum.mean() > 0 )
    {
      if ( first ) util::fprintf( file, "  Up-Times:\n" ); first = false;
      util::fprintf( file, "    %5.1f%% : %-30s\n", u -> uptime_sum.mean() * 100.0, u -> name() );
    }
  }
}

// print_text_waiting ==========================================================
void print_text_waiting( FILE* file, player_t* p )
{
  util::fprintf( file, "  Waiting: %4.2f%%", 100.0* p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean() );
}

// print_text_waiting_all =======================================================

void print_text_waiting_all( FILE* file, sim_t* sim )
{
  util::fprintf( file, "\nWaiting:\n" );

  bool nobody_waits = true;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( p -> quiet )
      continue;

    if ( p -> collected_data.waiting_time.mean() )
    {
      nobody_waits = false;
      util::fprintf( file, "    %4.1f%% : %s\n", 100.0 * p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean(),  p -> name() );
    }
  }

  if ( nobody_waits ) util::fprintf( file, "    All players active 100%% of the time.\n" );
}

// print_text_performance ===================================================

void print_text_performance( FILE* file, sim_t* sim )
{
  std::time_t cur_time = std::time( NULL );
  std::string date_str = std::asctime(std::localtime(&cur_time));
  if ( date_str.size() > 0 && *(date_str.end() - 1) == '\n' )
    date_str = date_str.substr( 0, date_str.size() - 1 );
  util::fprintf( file,
                 "\nBaseline Performance:\n"
#if !defined( SC_WINDOWS )
                 "  TotalEvents   = %llu\n"
#else
                 "  TotalEvents   = %I64u\n"
#endif
                 "  MaxEventQueue = %ld\n"
                 "  TargetHealth  = %.0f\n"
                 "  SimSeconds    = %.0f\n"
                 "  CpuSeconds    = %.3f\n"
                 "  WallSeconds   = %.3f\n"
                 "  SpeedUp       = %.0f\n"
                 "  EndTime       = %s (%ld)\n\n",
                 sim -> total_events_processed,
                 ( long ) sim -> max_events_remaining,
                 sim -> target -> resources.base[ RESOURCE_HEALTH ],
                 sim -> iterations * sim -> simulation_length.mean(),
                 sim -> elapsed_cpu,
                 sim -> elapsed_time,
                 sim -> iterations * sim -> simulation_length.mean() / sim -> elapsed_cpu,
                 date_str.c_str(),
                 as<long int>(cur_time) );
}

// print_text_scale_factors =================================================

void print_text_scale_factors( FILE* file, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  util::fprintf( file, "\nScale Factors:\n" );

  int num_players = ( int ) sim -> players_by_name.size();
  int max_length = 0;

  for ( int i = 0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    int length = ( int ) strlen( p -> name() );
    if ( length > max_length ) max_length = length;
  }

  for ( int i = 0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    util::fprintf( file, "  %-*s", max_length, p -> name() );
    
    scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
    gear_stats_t& sf = ( sim -> scaling -> normalize_scale_factors ) ? p -> scaling_normalized[ sm ] : p -> scaling[ sm ];

    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( p -> scales_with[ j ] )
      {
        util::fprintf( file, "  %s=%.*f(%.*f)", util::stat_type_abbrev( j ),
                       sim -> report_precision, sf.get_stat( j ),
                       sim -> report_precision, p -> scaling_error[ sm ].get_stat( j ) );
      }
    }

    if ( sim -> scaling -> normalize_scale_factors )
    {
      scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
      util::fprintf( file, "  DPS/%s=%.*f", util::stat_type_abbrev( p -> normalize_by() ), sim -> report_precision, p -> scaling[ sm ].get_stat( p -> normalize_by() ) );
    }

    if ( p -> sim -> scaling -> scale_lag ) util::fprintf( file, "  ms Lag=%.*f(%.*f)", p -> sim -> report_precision, p -> scaling_lag[ sm ], p -> sim -> report_precision, p -> scaling_lag_error[ sm ] );

    util::fprintf( file, "\n" );
  }
}

// print_text_scale_factors =================================================

void print_text_scale_factors( FILE* file, player_t* p, player_processed_report_information_t& ri )
{
  if ( ! p -> sim -> scaling -> has_scale_factors() ) return;

  if ( p -> sim -> report_precision < 0 )
    p -> sim -> report_precision = 2;

  util::fprintf( file, "  Scale Factors:\n" );
  
  scale_metric_e sm = p -> sim -> scaling -> scaling_metric;
  gear_stats_t& sf = ( p -> sim -> scaling -> normalize_scale_factors ) ? p -> scaling_normalized[ sm ] : p -> scaling[ sm ];

  util::fprintf( file, "    Weights :" );
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( p -> scales_with[ i ] )
    {
      util::fprintf( file, "  %s=%.*f(%.*f)", util::stat_type_abbrev( i ),
                     p -> sim -> report_precision, sf.get_stat( i ),
                     p -> sim -> report_precision, p -> scaling_error[ sm ].get_stat( i ) );
    }
  }
  if ( p -> sim -> scaling -> normalize_scale_factors )
  {
    util::fprintf( file, "  DPS/%s=%.*f", util::stat_type_abbrev( p -> normalize_by() ), p -> sim -> report_precision, p -> scaling[ sm ].get_stat( p -> normalize_by() ) );
  }
  if ( p -> sim -> scaling -> scale_lag ) util::fprintf( file, "  ms Lag=%.*f(%.*f)", p -> sim -> report_precision, p -> scaling_lag[ sm ], p -> sim -> report_precision, p -> scaling_lag_error[ sm ] );

  util::fprintf( file, "\n" );

  std::string lootrank       = ri.gear_weights_lootrank_link;
  std::string wowhead_std    = ri.gear_weights_wowhead_std_link;

  simplify_html( lootrank    );
  simplify_html( wowhead_std );

  util::fprintf( file, "    Wowhead : %s\n", wowhead_std.c_str() );
}

// print_text_dps_plots =====================================================

void print_text_dps_plots( FILE* file, player_t* p )
{
  sim_t* sim = p -> sim;

  if ( sim -> plot -> dps_plot_stat_str.empty() ) return;

  int range = sim -> plot -> dps_plot_points / 2;

  double min = -range * sim -> plot -> dps_plot_step;
  double max = +range * sim -> plot -> dps_plot_step;

  int points = 1 + range * 2;

  util::fprintf( file, "  DPS Plot Data ( min=%.1f max=%.1f points=%d )\n", min, max, points );

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];

    if ( ! pd.empty() )
    {
      util::fprintf( file, "    DPS(%s)=", util::stat_type_abbrev( i ) );
      size_t num_points = pd.size();
      for ( size_t j = 0; j < num_points; j++ )
      {
        util::fprintf( file, "%s%.0f", ( j ? "|" : "" ), pd[ j ].value );
      }
      util::fprintf( file, "\n" );
    }
  }
}

// print_text_reference_dps =================================================

void print_text_reference_dps( FILE* file, sim_t* sim )
{
  if ( sim -> reference_player_str.empty() ) return;

  util::fprintf( file, "\nReference DPS:\n" );

  player_t* ref_p = sim -> find_player( sim -> reference_player_str );

  if ( ! ref_p )
  {
    sim -> errorf( "Unable to locate reference player: %s\n", sim -> reference_player_str.c_str() );
    return;
  }

  scale_metric_e sm = sim -> scaling -> scaling_metric;

  int num_players = ( int ) sim -> players_by_dps.size();
  int max_length = 0;

  for ( int i = 0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_dps[ i ];
    int length = ( int ) strlen( p -> name() );
    if ( length > max_length ) max_length = length;
  }

  util::fprintf( file, "  %-*s", max_length, ref_p -> name() );
  util::fprintf( file, "  %.0f", ref_p -> collected_data.dps.mean() );

  if ( sim -> scaling -> has_scale_factors() )
  {
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( ref_p -> scales_with[ j ] )
      {
        util::fprintf( file, "  %s=%.*f", util::stat_type_abbrev( j ), sim -> report_precision, ref_p -> scaling[ sm ].get_stat( j ) );
      }
    }
  }

  util::fprintf( file, "\n" );

  for ( int i = 0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_dps[ i ];

    if ( p != ref_p )
    {
      util::fprintf( file, "  %-*s", max_length, p -> name() );

      bool over = ( p -> collected_data.dps.mean() > ref_p -> collected_data.dps.mean() );

      double ratio = 100.0 * fabs( p -> collected_data.dps.mean() - ref_p -> collected_data.dps.mean() ) / ref_p -> collected_data.dps.mean();

      util::fprintf( file, "  %c%.0f%%", ( over ? '+' : '-' ), ratio );

      if ( sim -> scaling -> has_scale_factors() )
      {
        for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
        {
          if ( ref_p -> scales_with[ j ] )
          {
            double ref_sf = ref_p -> scaling[ sm ].get_stat( j );
            double     sf =     p -> scaling[ sm ].get_stat( j );

            over = ( sf > ref_sf );

            ratio = 100.0 * fabs( sf - ref_sf ) / ref_sf;

            util::fprintf( file, "  %s=%c%.0f%%", util::stat_type_abbrev( j ), ( over ? '+' : '-' ), ratio );
          }
        }
      }

      util::fprintf( file, "\n" );
    }
  }
}

void print_text_hat_donors( FILE* file, sim_t* sim )
{
  std::vector<player_t*> hat_donors;

  int num_players = ( int ) sim -> players_by_name.size();
  for ( int i = 0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    if ( p -> procs.hat_donor -> count.mean() )
      hat_donors.push_back( p );
  }

  int num_donors = ( int ) hat_donors.size();
  if ( num_donors )
  {
    range::sort( hat_donors, compare_hat_donor_interval() );

    util::fprintf( file, "\nHonor Among Thieves Donor Report:\n" );

    for ( int i = 0; i < num_donors; i++ )
    {
      player_t* p = hat_donors[ i ];
      proc_t* proc = p -> procs.hat_donor;
      util::fprintf( file, "  %.2fsec | %.3fcps : %s\n", proc -> interval_sum.mean(), ( 1.0 / proc -> interval_sum.mean() ), p -> name() );
    }
  }
}
struct sort_by_event_stopwatch {
  bool operator()(player_t* l, player_t* r )
  { return l -> event_stopwatch.current() > r -> event_stopwatch.current(); }
};
void print_text_monitor_cpu( FILE* file, sim_t* sim )
{
  if ( ! sim -> monitor_cpu ) return;

  util::fprintf( file, "\nEvent Monitor CPU Report:\n" );

  util::fprintf( file, "%10.2fsec : Global Events\n", sim -> event_stopwatch.current() );

  std::vector<player_t*> sorted_p = sim -> player_list.data();

  range::sort( sorted_p, sort_by_event_stopwatch() );
  for ( size_t i = 0; i < sorted_p.size(); ++i )
  {
    player_t* p = sorted_p[ i ];

    util::fprintf( file, "%10.2fsec : %s\n", p -> event_stopwatch.current(), p -> name() );
  }
}

// print_text_player ========================================================

void print_text_player( FILE* file, player_t* p )
{
  report::generate_player_buff_lists( p, p->report_information );

  const player_collected_data_t& cd = p -> collected_data;

  util::fprintf( file, "\n%s: %s %s %s %s %d\n",
                 p -> is_enemy() ? "Target" : p -> is_add() ? "Add" : "Player",
                 p -> name(), p -> race_str.c_str(),
                 util::player_type_string( p -> type ),
                 dbc::specialization_string( p -> specialization() ).c_str(), p -> level );

  double dps_error = sim_t::distribution_mean_error( *p -> sim, p -> collected_data.dps );
  util::fprintf( file, "  DPS: %.1f  DPS-Error=%.1f/%.1f%%  DPS-Range=%.0f/%.1f%%  DPS-Convergence=%.1f%%\n",
                 p -> collected_data.dps.mean(),
                 dps_error, cd.dps.mean() ? dps_error * 100 / cd.dps.mean() : 0,
                 ( cd.dps.max() - cd.dps.min() ) / 2.0 , cd.dps.mean() ? ( ( cd.dps.max() - cd.dps.min() ) / 2 ) * 100 / cd.dps.mean() : 0,
                 p -> dps_convergence * 100 );

  double hps_error = sim_t::distribution_mean_error( *p -> sim, p -> collected_data.hps );
  util::fprintf( file, "  HPS: %.1f HPS-Error=%.1f/%.1f%%\n",
                 cd.hps.mean(),
                 hps_error, cd.hps.mean() ? hps_error * 100 / cd.hps.mean() : 0 );

  if ( p -> rps_loss > 0 )
  {
    util::fprintf( file, "  DPR=%.1f  RPS-Out=%.1f RPS-In=%.1f  Resource=(%s) Waiting=%.1f ApM=%.1f",
                   p -> dpr, p -> rps_loss, p -> rps_gain,
                   util::resource_type_string( p -> primary_resource() ),
                   100.0 * p -> collected_data.waiting_time.mean() / p -> collected_data.fight_length.mean(),
                   60.0 * p -> collected_data.executed_foreground_actions.mean() / p -> collected_data.fight_length.mean()  );
  }

  util::fprintf( file, "\n" );

  if ( p -> primary_role() == ROLE_TANK && p -> type != ENEMY )
  {
    double dtps_error = sim_t::distribution_mean_error( *p -> sim, p -> collected_data.dtps );
    util::fprintf( file, "  DTPS: %.1f  DTPS-error=%.1f/%.1f%% \n",
                    p -> collected_data.dtps.mean(), dtps_error,
                    p -> collected_data.dtps.mean() ? dtps_error * 100 / p -> collected_data.dtps.mean() : 0.0 );

    double tmi_error = sim_t::distribution_mean_error( *p -> sim, p -> collected_data.theck_meloree_index );
    util::fprintf( file, "  TMI: %.1f  TMI-error=%.1f/%.1f%%  TMI-min=%.1f  TMI-max=%.1f \n",
                    p -> collected_data.theck_meloree_index.mean(),
                    tmi_error,
                    p -> collected_data.theck_meloree_index.mean() ? tmi_error * 100 / p -> collected_data.theck_meloree_index.mean() : 0.0,
                    p -> collected_data.theck_meloree_index.min(),
                    p -> collected_data.theck_meloree_index.max() );
  }

  if ( ! p -> origin_str.empty() )  util::fprintf( file, "  Origin: %s\n", p -> origin_str.c_str() );
  if ( ! p -> talents_str.empty() ) util::fprintf( file, "  Talents: %s\n", p -> talents_str.c_str() );
  print_text_core_stats   ( file, p );
  print_text_spell_stats  ( file, p );
  print_text_attack_stats ( file, p );
  print_text_defense_stats( file, p );
  print_text_actions      ( file, p );

  print_text_buffs        ( file, p -> report_information );
  print_text_uptime       ( file, p );
  print_text_procs        ( file, p );
  print_text_player_gains ( file, p );
  print_text_pet_gains    ( file, p );
  print_text_scale_factors( file, p, p -> report_information );
  print_text_dps_plots    ( file, p );
  print_text_waiting      ( file, p );

}

} // UNNAMED NAMESPACE ====================================================


namespace report {

void print_text( sim_t* sim, bool detail )
{
  std::flush( *sim -> out_std.get_stream() );
  FILE* file = stdout;
  io::cfile report_f( sim -> output_file_str, "a" );
  if ( report_f )
    file = report_f;


  if ( sim -> simulation_length.mean() == 0 ) return;

#if SC_BETA
  util::fprintf( file, "\n" );
  for ( size_t i = 0; i < sizeof_array( beta_warnings ); ++i )
    util::fprintf( file, " * %s\n", beta_warnings[ i ] );
#endif

  if ( ! sim -> raid_events_str.empty() )
  {
    util::fprintf( file, "\n\nRaid Events:\n" );
    std::vector<std::string> raid_event_names = util::string_split( sim -> raid_events_str, "/" );
    if ( ! raid_event_names.empty() )
      util::fprintf( file, "  raid_event=/%s\n", raid_event_names[ 0 ].c_str() );
    for ( size_t i = 1; i < raid_event_names.size(); i++ )
    {
      util::fprintf( file, "  raid_event+=/%s\n", raid_event_names[ i ].c_str() );
    }
    util::fprintf( file, "\n" );
  }

  int num_players = ( int ) sim -> players_by_dps.size();

  if ( detail )
  {
    util::fprintf( file, "\nDPS Ranking:\n" );
    util::fprintf( file, "%7.0f 100.0%%  Raid\n", sim -> raid_dps.mean() );
    for ( int i = 0; i < num_players; i++ )
    {
      player_t* p = sim -> players_by_dps[ i ];
      if ( p -> collected_data.dps.mean() <= 0 ) continue;
      util::fprintf( file, "%7.0f  %4.1f%%  %s\n", p -> collected_data.dps.mean(), sim -> raid_dps.mean() ? 100 * p -> collected_data.dpse.mean() / sim -> raid_dps.mean() : 0, p -> name() );
    }

    if ( ! sim -> players_by_hps.empty() )
    {
      util::fprintf( file, "\nHPS Ranking:\n" );
      util::fprintf( file, "%7.0f 100.0%%  Raid\n", sim -> raid_hps.mean() + sim -> raid_aps.mean() );
      for ( size_t i = 0; i < sim -> players_by_hps.size(); i++ )
      {
        player_t* p = sim -> players_by_hps[ i ];
        if ( p -> collected_data.hps.mean() <= 0 && p -> collected_data.aps.mean() <= 0 ) continue;
        util::fprintf( file, "%7.0f  %4.1f%%  %s\n", p -> collected_data.hps.mean() + p -> collected_data.aps.mean(), sim -> raid_hps.mean() ? 100 * p -> collected_data.hpse.mean() / sim -> raid_hps.mean() : 0, p -> name() );
      }
    }
  }

  // Report Players
  for ( int i = 0; i < num_players; i++ )
  {
    print_text_player( file, sim -> players_by_name[ i ] );

    // Pets
    if ( sim -> report_pets_separately )
    {
      std::vector<pet_t*>& pl = sim -> players_by_name[ i ] -> pet_list;
      for ( size_t j = 0; j < pl.size(); ++j )
      {
        pet_t* pet = pl[ j ];
        if ( pet -> summoned && !pet -> quiet )
          print_text_player( file, pet );
      }
    }
  }

  // Report Targets
  if ( sim -> report_targets )
  {
    util::fprintf( file, "\n\n *** Targets *** \n\n" );

    for ( int i = 0; i < ( int ) sim -> targets_by_name.size(); i++ )
    {
      print_text_player( file, sim -> targets_by_name[ i ] );

      // Pets
      if ( sim -> report_pets_separately )
      {
        std::vector<pet_t*>& pl = sim -> targets_by_name[ i ] -> pet_list;
        for ( size_t j = 0; j < pl.size(); ++j )
        {
          pet_t* pet = pl[ j ];
          if ( pet -> summoned )
            print_text_player( file, pet );
        }
      }
    }
  }

  if ( detail )
  {
    print_text_hat_donors   ( file, sim );
    print_text_waiting_all  ( file, sim );
    print_text_performance  ( file, sim );
    print_text_scale_factors( file, sim );
    print_text_reference_dps( file, sim );
    print_text_monitor_cpu  ( file, sim );
  }

  util::fprintf( file, "\n" );
}

} // END report NAMESPACE

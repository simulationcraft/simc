// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_report.hpp"
#include "simulationcraft.hpp"
#include "util/fmt/time.h"

namespace
{  // UNNAMED NAMESPACE ==========================================

void simplify_html( std::string& buffer )
{
  util::replace_all( buffer, "&lt;", "<" );
  util::replace_all( buffer, "&gt;", ">" );
  util::replace_all( buffer, "&amp;", "&" );
}

// print_text_action ========================================================

void print_action( FILE* file, stats_t* s, size_t max_name_length,
                        int max_dpe, int max_dpet, int max_dpr, int max_pdps )
{
  if ( max_name_length == 0 )
    max_name_length = 20;

  fmt::print( file, "    {:<{}}  Count={:6.1f}|{:7.3f}sec  DPE={:{}.0f}|{:5.2f}%  "
                    "DPET={:{}.0f}  DPR={:{}.0f}  pDPS={:{}.0f}",
                    s->name_str,
                    max_name_length,
                    s->num_executes.mean(),
                    s->total_intervals.mean(),
                    s->ape,
                    max_dpe,
                    s->portion_amount * 100.0,
                    s->apet,
                    max_dpet,
                    s->apr[ s->player->primary_resource() ],
                    ( max_dpr + 2 ),
                    s->portion_aps.mean(),
                    max_pdps );

  if ( s->num_direct_results.mean() > 0 )
  {
    fmt::print( file, "  Miss={:5.2f}%",
        s->direct_results[ FULLTYPE_MISS ].pct );
  }

  if ( s->direct_results[ FULLTYPE_HIT ].actual_amount.sum() > 0 )
  {
    fmt::print( file, "  Hit={:6.0f}|{:6.0f}|{:6.0f}",
        s->direct_results[ FULLTYPE_HIT ].actual_amount.mean(),
        s->direct_results[ FULLTYPE_HIT ].actual_amount.min(),
        s->direct_results[ FULLTYPE_HIT ].actual_amount.max() );
  }
  if ( s->direct_results[ FULLTYPE_CRIT ].actual_amount.sum() > 0 )
  {
    fmt::print( file, "  Crit={:6.0f}|{:6.0f}|{:6.0f}|{:5.2f}%",
        s->direct_results[ FULLTYPE_CRIT ].actual_amount.mean(),
        s->direct_results[ FULLTYPE_CRIT ].actual_amount.min(),
        s->direct_results[ FULLTYPE_CRIT ].actual_amount.max(),
        s->direct_results[ FULLTYPE_CRIT ].pct );
  }
  if ( s->direct_results[ FULLTYPE_GLANCE ].actual_amount.sum() > 0 )
  {
    fmt::print( file, "  Glance={:6.0f}|{:5.2f}%",
        s->direct_results[ FULLTYPE_GLANCE ].actual_amount.mean(),
        s->direct_results[ FULLTYPE_GLANCE ].pct );
  }
  if ( s->direct_results[ FULLTYPE_DODGE ].count.sum() > 0 )
  {
    fmt::print( file, "  Dodge={:5.2f}%",
                   s->direct_results[ FULLTYPE_DODGE ].pct );
  }
  if ( s->direct_results[ FULLTYPE_PARRY ].count.sum() > 0 )
  {
    fmt::print( file, "  Parry={:5.2f}%",
                   s->direct_results[ FULLTYPE_PARRY ].pct );
  }

  if ( s->num_ticks.sum() > 0 )
    fmt::print( file, "  TickCount={:6.0f}", s->num_ticks.mean() );

  if ( s->tick_results[ RESULT_HIT ].actual_amount.sum() > 0 ||
       s->tick_results[ RESULT_CRIT ].actual_amount.sum() > 0 )
  {
    fmt::print( file, "  MissTick={:5.2f}%",
                   s->tick_results[ RESULT_MISS ].pct );
  }

  if ( s->tick_results[ RESULT_HIT ].avg_actual_amount.sum() > 0 )
  {
    fmt::print( file, "  Tick={:6.0f}|{:6.0f}|{:6.0f}",
                   s->tick_results[ RESULT_HIT ].actual_amount.mean(),
                   s->tick_results[ RESULT_HIT ].actual_amount.min(),
                   s->tick_results[ RESULT_HIT ].actual_amount.max() );
  }
  if ( s->tick_results[ RESULT_CRIT ].avg_actual_amount.sum() > 0 )
  {
    fmt::print( file, "  CritTick={:6.0f}|{:6.0f}|{:6.0f}|{:5.2f}%",
                   s->tick_results[ RESULT_CRIT ].actual_amount.mean(),
                   s->tick_results[ RESULT_CRIT ].actual_amount.min(),
                   s->tick_results[ RESULT_CRIT ].actual_amount.max(),
                   s->tick_results[ RESULT_CRIT ].pct );
  }

  if ( s->total_tick_time.sum() > 0.0 )
  {
    fmt::print( file, "  UpTime={:6.2f}%",
        100.0 * s->total_tick_time.mean() / s->player->collected_data.fight_length.mean() );
  }

  fmt::print( file, "\n" );
}

void print_player_actions( FILE* file, const player_t& p )
{
  for ( auto& alist : p.action_priority_list )
  {
    if ( alist->used )
    {
      fmt::print( file, "  Priorities{}:\n",
          ( " (actions." + alist->name_str + ")" ) );

      size_t length = 0;
      constexpr size_t max_length = 140;
      for ( auto& ap : alist->action_list )
      {
        if ( length > max_length || ( length > 0 && ( length + ap.action_.size() ) > max_length ) )
        {
          fmt::print( file, "\n" );
          length = 0;
        }
        fmt::print( file, "{}{}",
            ( ( length > 0 ) ? "/" : "    " ),
            ap.action_ );
        length += ap.action_.size();
      }
      fmt::print( file, "\n" );
    }
  }

  fmt::print( file, "  Actions:\n" );

  size_t max_length = 0;
  int max_dpe = 0, max_dpet = 0, max_dpr = 0, max_pdps = 0;
  std::vector<stats_t*> tmp_stats_list = p.stats_list;
  for ( auto& pet : p.pet_list )
  {
    tmp_stats_list.insert( tmp_stats_list.end(),
                           pet->stats_list.begin(),
                           pet->stats_list.end() );
  }
  for ( auto& s : tmp_stats_list )
  {
      if ( max_length < s->name_str.length() )
        max_length = s->name_str.length();

    if ( s->total_amount.mean() > 0 )
    {
      if ( max_dpe < util::numDigits( static_cast<int32_t>( s->ape ) ) )
        max_dpe = util::numDigits( static_cast<int32_t>( s->ape ) );

      if ( max_dpet < util::numDigits( static_cast<int32_t>( s->apet ) ) )
        max_dpet = util::numDigits( static_cast<int32_t>( s->apet ) );

      if ( max_dpr < util::numDigits( static_cast<int32_t>(
                         s->apr[ s->player->primary_resource() ] ) ) )
        max_dpr = util::numDigits(
            static_cast<int32_t>( s->apr[ s->player->primary_resource() ] ) );

      if ( max_pdps <
           util::numDigits( static_cast<int32_t>( s->portion_aps.mean() ) ) )
        max_pdps =
            util::numDigits( static_cast<int32_t>( s->portion_aps.mean() ) );
    }
  }

  for ( auto& s : p.stats_list )
  {
    if ( s->num_executes.mean() > 1 || s->compound_amount > 0 )
    {
      print_action( file, s, max_length, max_dpe, max_dpet, max_dpr,
                         max_pdps );
    }
  }

  for ( auto& pet : p.pet_list )
  {
    bool first = true;
    for ( auto& s : pet->stats_list )
    {
      if ( s->num_executes.mean() > 1 || s->compound_amount > 0 )
      {
        if ( first )
        {
          fmt::print( file, "   {} (DPS={})\n",
              pet->name_str,
              pet->collected_data.dps.mean() );
          first = false;
        }
        print_action( file, s, max_length, max_dpe, max_dpet, max_dpr,
                           max_pdps );
      }
    }
  }
}

void print_constant_buffs( FILE* file, const player_processed_report_information_t& ri )
{
  bool first       = true;
  char prefix      = ' ';
  size_t total_length = 0;
  constexpr size_t max_line_length = 140;

  for ( auto& b : ri.constant_buffs )
  {
    if ( first )
    {
      fmt::print( file, "\n  Constant Buffs:\n   " );
      first = false;
    }
    auto length = b->name_str.length();
    if ( ( total_length + length ) > max_line_length )
    {
      fmt::print( file, "\n   " );
      prefix       = ' ';
      total_length = 0;
    }
    fmt::print( file, "{}{}", prefix, b->name_str );
    prefix = '/';
    total_length += length;
  }
}

void print_dynamic_buffs( FILE* file, const player_processed_report_information_t& ri )
{
  // Get max name length
  size_t max_name_length = 0;
  for ( auto& b : ri.dynamic_buffs )
  {
    std::string full_name;
    if ( b->player && b->player->is_pet() )
      full_name = b->player->name_str + "-" + b->name_str;
    else
      full_name = b->name_str;

    auto length = full_name.length();
    if ( length > max_name_length )
      max_name_length = length;
  }

  if ( !ri.dynamic_buffs.empty() )
    fmt::print( file, "  Dynamic Buffs:\n" );

  for ( auto& b : ri.dynamic_buffs )
  {
    std::string full_name;
    if ( b->player && b->player->is_pet() )
      full_name = b->player->name_str + "-" + b->name_str;
    else
      full_name = b->name_str;

    fmt::print( file, "    {:<{}} : start={:5.1f} refresh={:5.1f} interval={:5.1f} trigger={:5.1f} uptime={:6.2f}%",
        full_name,
        max_name_length,
        b->avg_start.mean(),
        b->avg_refresh.mean(),
        b->start_intervals.mean(),
        b->trigger_intervals.mean(),
        b->uptime_pct.mean() );

    if ( b->benefit_pct.sum() > 0 && b->benefit_pct.mean() < 100 )
    {
      fmt::print( file, "  benefit={:6.2f}%", b->benefit_pct.mean() );
    }

    fmt::print( file, "\n" );
  }
}

void print_player_buffs( FILE* file, const player_processed_report_information_t& ri )
{
  print_constant_buffs( file, ri );
  fmt::print( file, "\n" );
  print_dynamic_buffs( file, ri );
}

void print_core_stats( FILE* file, const player_t& p )
{
  auto& buffed_stats = p.collected_data.buffed_stats_snapshot;

  fmt::print( file,
                 "  Core Stats:    strength={:.0f}|{:.0f}({:.0f})  "
                 "agility={:.0f}|{:.0f}({:.0f})  stamina={:.0f}|{:.0f}({:.0f})  "
                 "intellect={:.0f}|{:.0f}({:.0f})  spirit={:.0f}|{:.0f}({:.0f})  "
                 "health={:.0f}|{:.0f}  mana={:.0f}|{:.0f}\n",
                 buffed_stats.attribute[ ATTR_STRENGTH ], p.strength(),
                 p.initial.stats.get_stat( STAT_STRENGTH ),
                 buffed_stats.attribute[ ATTR_AGILITY ], p.agility(),
                 p.initial.stats.get_stat( STAT_AGILITY ),
                 buffed_stats.attribute[ ATTR_STAMINA ], p.stamina(),
                 p.initial.stats.get_stat( STAT_STAMINA ),
                 buffed_stats.attribute[ ATTR_INTELLECT ], p.intellect(),
                 p.initial.stats.get_stat( STAT_INTELLECT ),
                 buffed_stats.attribute[ ATTR_SPIRIT ], p.spirit(),
                 p.initial.stats.get_stat( STAT_SPIRIT ),
                 buffed_stats.resource[ RESOURCE_HEALTH ],
                 p.resources.max[ RESOURCE_HEALTH ],
                 buffed_stats.resource[ RESOURCE_MANA ],
                 p.resources.max[ RESOURCE_MANA ] );
}

void print_generic_stats( FILE* file, const player_t& p )
{
  auto& buffed_stats = p.collected_data.buffed_stats_snapshot;

  fmt::print(
      file,
      "  Generic Stats: "
      "mastery={:.2f}%|{:.2f}%({:.0f})  "
      "versatility={:.2f}%|{:.2f}%({:.0f})  "
      "leech={:.2f}%|{:.2f}%({:.0f})  "
      "runspeed={:.2f}%|{:.2f}%({:.0f})\n",
      100.0 * buffed_stats.mastery_value, 100.0 * p.cache.mastery_value(),
      p.composite_mastery_rating(), 100 * buffed_stats.damage_versatility,
      100 * p.composite_damage_versatility(),
      p.composite_damage_versatility_rating(), 100 * buffed_stats.leech,
      100 * p.composite_leech(), p.composite_leech_rating(),
      buffed_stats.run_speed, p.composite_movement_speed(), p.composite_speed_rating() );
}

void print_spell_stats( FILE* file, const player_t& p )
{
  auto& buffed_stats = p.collected_data.buffed_stats_snapshot;

  fmt::print(
      file,
      "  Spell Stats:   power={:.0f}|{:.0f}({:.0f})  hit={:.2f}%|{:.2f}%({:.0f})  "
      "crit={:.2f}%|{:.2f}%({:.0f})  haste={:.2f}%|{:.2f}%({:.0f})  "
      "speed={:.2f}%|{:.2f}%  manareg={:.0f}|{:.0f}({})\n",
      buffed_stats.spell_power,
      p.composite_spell_power( SCHOOL_MAX ) * p.composite_spell_power_multiplier(),
      p.initial.stats.spell_power,
      100 * buffed_stats.spell_hit,
      100 * p.composite_spell_hit(),
      p.composite_spell_hit_rating(),
      100 * buffed_stats.spell_crit_chance,
      100 * p.composite_spell_crit_chance(),
      p.composite_spell_crit_rating(),
      100 * ( 1 / buffed_stats.spell_haste - 1 ),
      100 * ( 1 / p.composite_spell_haste() - 1 ),
      p.composite_spell_haste_rating(),
      100 * ( 1 / buffed_stats.spell_speed - 1 ),
      100 * ( 1 / p.cache.spell_speed() - 1 ),
      buffed_stats.manareg_per_second,
      p.resource_regen_per_second( RESOURCE_MANA ),
      0 );
}

void print_attack_stats( FILE* file, const player_t& p )
{
  auto& buffed_stats = p.collected_data.buffed_stats_snapshot;

  if ( p.dual_wield() )
  {
    fmt::print(
        file,
        "  Attack Stats:  power={:.0f}|{:.0f}({:.0f})  hit={:.2f}%|{:.2f}%({:.0f})  "
        "crit={:.2f}%|{:.2f}%({:.0f})  expertise={:.2f}%/{:.2f}%|{:.2f}%/{:.2f}%({:.0f}) "
        " haste={:.2f}%|{:.2f}%({:.0f})  speed={:.2f}%|{:.2f}%\n",
        buffed_stats.attack_power,
        p.composite_melee_attack_power() * p.composite_attack_power_multiplier(),
        p.initial.stats.attack_power,
        100 * buffed_stats.attack_hit,
        100 * p.composite_melee_hit(),
        p.composite_melee_hit_rating(),
        100 * buffed_stats.attack_crit_chance,
        100 * p.composite_melee_crit_chance(),
        p.composite_melee_crit_rating(),
        100 * buffed_stats.mh_attack_expertise,
        100 * p.composite_melee_expertise( &( p.main_hand_weapon ) ),
        100 * buffed_stats.oh_attack_expertise,
        100 * p.composite_melee_expertise( &( p.off_hand_weapon ) ),
        p.composite_expertise_rating(),
        100 * ( 1 / buffed_stats.attack_haste - 1 ),
        100 * ( 1 / p.composite_melee_haste() - 1 ),
        p.composite_melee_haste_rating(),
        100 * ( 1 / buffed_stats.attack_speed - 1 ),
        100 * ( 1 / p.composite_melee_speed() - 1 ) );
  }
  else
  {
    fmt::print(
        file,
        "  Attack Stats:  power={:.0f}|{:.0f}({:.0f})  hit={:.2f}%|{:.2f}%({:.0f})  "
        "crit={:.2f}%|{:.2f}%({:.0f})  expertise={:.2f}%|{:.2f}%({:.0f})  "
        "haste={:.2f}%|{:.2f}%({:.0f})  speed={:.2f}%|{:.2f}%\n",
        buffed_stats.attack_power,
        p.composite_melee_attack_power() * p.composite_attack_power_multiplier(),
        p.initial.stats.attack_power,
        100 * buffed_stats.attack_hit,
        100 * p.composite_melee_hit(),
        p.composite_melee_hit_rating(),
        100 * buffed_stats.attack_crit_chance,
        100 * p.composite_melee_crit_chance(),
        p.composite_melee_crit_rating(),
        100 * buffed_stats.mh_attack_expertise,
        100 * p.composite_melee_expertise( &( p.main_hand_weapon ) ),
        p.current.stats.expertise_rating,
        100 * ( 1 / buffed_stats.attack_haste - 1 ),
        100 * ( 1 / p.composite_melee_haste() - 1 ),
        p.composite_melee_haste_rating(),
        100 * ( 1 / buffed_stats.attack_speed - 1 ),
        100 * ( 1 / p.composite_melee_speed() - 1 ) );
  }
}

void print_defense_stats( FILE* file, const player_t& p )
{
  auto& buffed_stats = p.collected_data.buffed_stats_snapshot;

  fmt::print(
      file,
      "  Defense Stats: armor={:.0f}|{:.0f}({:.0f}) miss={:.2f}%|{:.2f}%  "
      "dodge={:.2f}%|{:.2f}%({:.0f})  parry={:.2f}%|{:.2f}%({:.0f})  "
      "block={:.2f}%|{:.2f}%({:.0f}) crit={:.2f}%|{:.2f}%  "
      "versatility={:.2f}%|{:.2f}%({:.0f})\n",
      buffed_stats.armor,
      p.composite_armor(),
      p.initial.stats.armor,
      100 * buffed_stats.miss,
      100 * ( p.cache.miss() ),
      100 * buffed_stats.dodge,
      100 * ( p.cache.dodge() ),
      p.initial.stats.dodge_rating,
      100 * buffed_stats.parry,
      100 * ( p.cache.parry() ),
      p.initial.stats.parry_rating,
      100 * buffed_stats.block,
      100 * p.composite_block(),
      p.initial.stats.block_rating,
      100 * buffed_stats.crit,
      100 * p.cache.crit_avoidance(),
      100 * buffed_stats.mitigation_versatility,
      100 * p.composite_mitigation_versatility(),
      p.composite_mitigation_versatility_rating() );
}

void print_gain( FILE* file, const gain_t& g, int max_name_length )
{
  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( g.actual[ i ] > 0 || g.overflow[ i ] > 0 )
    {
      fmt::print( file, "    {:8.1f} : {:<{}} {:<15}",
          g.actual[ i ],
          g.name(),
          max_name_length,
          fmt::format("({})", util::resource_type_string( i ) ));

      double overflow_pct = 100.0 * g.overflow[ i ] / ( g.actual[ i ] + g.overflow[ i ] );

      if ( overflow_pct > 1.0 )
      {
        fmt::print( file, "  (overflow={:.2f}%)", overflow_pct );
      }
      fmt::print( file, "\n" );
    }
  }
}

void gain_name_length( const std::vector<gain_t*>& gain_list, size_t& max_length )
{
  for ( auto& g : gain_list )
  {
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; r++ )
    {
      if ( g->actual[ r ] > 0 || g->overflow[ r ] > 0 )
      {
        auto length = std::strlen( g->name() );
        if ( length > max_length )
          max_length = length;
      }
    }
  }
}

void print_player_gains( FILE* file, const player_t& p )
{
  // Get max gain name length
  size_t max_name_length = 0;
  gain_name_length( p.gain_list, max_name_length );
  for ( auto& pet : p.pet_list )
  {
    if ( pet->collected_data.dmg.sum() <= 0 )
      continue;
    gain_name_length( pet->gain_list, max_name_length );
  }
  if ( max_name_length == 0 )
    return;

  fmt::print( file, "  Gains:\n" );

  for ( auto& g : p.gain_list )
  {
    print_gain( file, *g, max_name_length );
  }

  for ( auto& pet : p.pet_list )
  {
    if ( pet->collected_data.dmg.sum() <= 0 )
      continue;

    fmt::print( file, "    Pet \"{}\" Gains:\n", pet->name_str );

    for ( auto& g : pet->gain_list )
    {
      print_gain( file, *g, max_name_length );
    }
  }
}

void print_procs( FILE* file, const player_t& p )
{
  bool first = true;

  for ( auto& proc : p.proc_list )
  {
    if ( proc->count.sum() > 0.0 )
    {
      if ( first )
      {
        fmt::print( file, "  Procs:\n" );
        first = false;
      }
      fmt::print( file, "    {} | {}sec : {}\n",
          proc->count.mean(),
          proc->interval_sum.pretty_mean(),
          proc->name() );
    }
  }
}

void print_uptimes_benefits( FILE* file, const player_t& p )
{
  bool first = true;

  for ( auto& b : p.benefit_list )
  {
    if ( b->ratio.mean() > 0 )
    {
      if ( first )
      {
        fmt::print( file, "  Benefits:\n" );
        first = false;
      }
      fmt::print( file, "    {:6.2f}% : {:<30}\n",
          b->ratio.mean(),
          b->name() );
    }
  }

  first = true;
  for ( auto& u : p.uptime_list )
  {
    if ( u->uptime_sum.mean() > 0 )
    {
      if ( first )
      {
        fmt::print( file, "  Up-Times:\n" );
        first = false;
      }
      fmt::print( file, "    {:6.2f}% : {:<30}\n",
          u->uptime_sum.mean() * 100.0,
          u->name() );
    }
  }
}

// print_text_waiting ==========================================================
void print_waiting_player( FILE* file, const player_t& p )
{
  double wait_time = 0;
  if ( p.collected_data.fight_length.mean() > 0.0 )
  {
    wait_time = p.collected_data.waiting_time.mean() /
                p.collected_data.fight_length.mean();
  }

  fmt::print( file, "  Waiting: {:5.2f}%\n", 100.0 * wait_time );
}

// print_text_waiting_all
// =======================================================

void print_waiting_all( FILE* file, const sim_t& sim )
{
  fmt::print( file, "\nWaiting:\n" );

  bool nobody_waits = true;

  for ( auto& p : sim.player_list )
  {
    if ( p->quiet )
      continue;

    if ( p->collected_data.waiting_time.mean() > 0.0)
    {
      nobody_waits = false;
      fmt::print( file, "    {:5.2f}% : {}\n",
          100.0 * p->collected_data.waiting_time.mean() / p->collected_data.fight_length.mean(),
          p->name() );
    }
  }

  if ( nobody_waits )
  {
    fmt::print( file, "    All players active 100% of the time.\n" );
  }
}

// print_text_iteration_data ================================================

void print_iteration_data( FILE* file, const sim_t& sim )
{
  if ( !sim.deterministic || sim.report_iteration_data == 0 )
  {
    return;
  }

  size_t n_spacer = ( sim.target_list.size() - 1 ) * 10 + ( sim.target_list.size() - 2 ) * 2 + 2;
  std::string spacer_str_1( n_spacer, '-' ), spacer_str_2( n_spacer, ' ' );

  fmt::print( file, "\nIteration data:\n" );
  if ( sim.low_iteration_data.size() && sim.high_iteration_data.size() )
  {
    fmt::print(
        file,
        ".--------------------------------------------------------{}. "
        ".--------------------------------------------------------{}.\n",
        spacer_str_1, spacer_str_1 );
    fmt::print(
        file,
        "| Low Iteration Data                                     {}| | High "
        "Iteration Data                                    {}|\n",
        spacer_str_2, spacer_str_2 );
    fmt::print(
        file,
        "+--------+-----------+----------------------+------------{}+ "
        "+--------+-----------+----------------------+------------{}+\n",
        spacer_str_1, spacer_str_1 );
    fmt::print( file,
                   "|  Iter# |    Metric |                 Seed |  {}Health(s) "
                   "| |  Iter# |    "
                   "Metric |                 Seed |  {}Health(s) |\n",
                   spacer_str_2, spacer_str_2 );
    fmt::print(
        file,
        "+--------+-----------+----------------------+------------{}+ "
        "+--------+-----------+----------------------+------------{}+\n",
        spacer_str_1, spacer_str_1 );

    for ( size_t i = 0; i < sim.low_iteration_data.size(); i++ )
    {
      const iteration_data_entry_t& low_data  = sim.low_iteration_data[ i ];
      const iteration_data_entry_t& high_data = sim.high_iteration_data[ i ];
      std::ostringstream low_health_s, high_health_s;
      for ( size_t health_idx = 0; health_idx < low_data.target_health.size();
            ++health_idx )
      {
        low_health_s << std::setw( 10 ) << std::right
                     << low_data.target_health[ health_idx ];
        high_health_s << std::setw( 10 ) << std::right
                      << high_data.target_health[ health_idx ];

        if ( health_idx < low_data.target_health.size() - 1 )
        {
          low_health_s << ", ";
          high_health_s << ", ";
        }
      }

      fmt::print( file, "| {:6} | {:9.1f} | {:20} | {} | | {:6} | {:9.1f} | {:20} | {} |\n",
          sim.low_iteration_data[ i ].iteration,
          sim.low_iteration_data[ i ].metric,
          sim.low_iteration_data[ i ].seed, low_health_s.str(),
          sim.high_iteration_data[ i ].iteration,
          sim.high_iteration_data[ i ].metric,
          sim.high_iteration_data[ i ].seed, high_health_s.str() );
    }
    fmt::print(
        file,
        "'--------+-----------+----------------------+------------{}' "
        "'--------+-----------+----------------------+------------{}'\n",
        spacer_str_1, spacer_str_1 );
  }
  else
  {
    fmt::print( file, ".--------------------------------------------------------{}.\n",
                   spacer_str_1 );
    fmt::print( file, "| Iteration Data                                         {}|\n",
                   spacer_str_2 );
    fmt::print( file, "+--------+-----------+----------------------+------------{}+\n",
                   spacer_str_1 );
    fmt::print( file, "|  Iter# |    Metric |                 Seed |  {}Health(s) |\n",
                   spacer_str_2 );
    fmt::print( file, "+--------+-----------+----------------------+------------{}+\n",
                   spacer_str_1 );

    for ( auto& data : sim.iteration_data )
    {
      std::ostringstream health_s;
      for ( size_t health_idx = 0; health_idx < data.target_health.size();
            ++health_idx )
      {
        health_s << std::setw( 10 ) << std::right
                 << data.target_health[ health_idx ];

        if ( health_idx < data.target_health.size() - 1 )
        {
          health_s << ", ";
        }
      }

      fmt::print( file, "| {:6} | {:9.1f} | {:20} | {} |\n",
          data.iteration,
          data.metric,
          data.seed, health_s.str().c_str() );
    }
    fmt::print( file, "'--------+-----------+----------------------+------------{}'\n",
                   spacer_str_1 );
  }
}

void sim_summary_performance( FILE* file, sim_t* sim )
{
  std::time_t cur_time = std::time( nullptr );
  auto date_str = fmt::format("{:%Y-%m-%d %H:%M:%S%z}", *std::localtime(&cur_time) );

  std::stringstream iterations_str;
  if ( sim -> threads > 1 )
  {
    iterations_str << " (";
    for ( size_t i = 0; i < sim -> work_per_thread.size(); ++i )
    {
      iterations_str << sim -> work_per_thread[ i ];

      if ( i < sim -> work_per_thread.size() - 1 )
      {
        iterations_str << ", ";
      }
    }
    iterations_str << ")";
  }

  fmt::print(
      file,
      "\n\nBaseline Performance:\n"
      "  RNG Engine    = {}{}\n"
      "  Iterations    = {}{}\n"
      "  TotalEvents   = {:n}\n"
      "  MaxEventQueue = {}\n"
#ifdef EVENT_QUEUE_DEBUG
      "  AllocEvents   = {}\n"
      "  EndInsert     = {} ({:.3f}%)\n"
      "  MaxTravDepth  = {}\n"
      "  AvgTravDepth  = {}\n"
#endif
      "  TargetHealth  = {:.0f}\n"
      "  SimSeconds    = {}\n"
      "  CpuSeconds    = {}\n"
      "  WallSeconds   = {}\n"
      "  InitSeconds   = {}\n"
      "  MergeSeconds  = {}\n"
      "  AnalyzeSeconds= {}\n"
      "  SpeedUp       = {}\n"
      "  EndTime       = {} ({})\n\n",
      sim->rng().name(), sim->deterministic ? " (deterministic)" : "",
      sim->iterations,
      sim -> threads > 1 ? iterations_str.str().c_str() : "",
      sim->event_mgr.total_events_processed,
      sim->event_mgr.max_events_remaining,
#ifdef EVENT_QUEUE_DEBUG
      sim->event_mgr.n_allocated_events, sim->event_mgr.n_end_insert,
      100.0 * static_cast<double>( sim->event_mgr.n_end_insert ) /
          sim->event_mgr.events_added,
      sim->event_mgr.max_queue_depth,
      static_cast<double>( sim->event_mgr.events_traversed ) /
          sim->event_mgr.events_added,
#endif
      sim->target->resources.base[ RESOURCE_HEALTH ],
      sim->simulation_length.sum(), sim->elapsed_cpu,
      sim->elapsed_time,
      sim->init_time,
      sim->merge_time,
      sim->analyze_time,
      sim->iterations * sim->simulation_length.mean() / sim->elapsed_cpu,
      date_str, cur_time );
#ifdef EVENT_QUEUE_DEBUG
  double total_p = 0;

  for ( unsigned i = 0; i < sim->event_mgr.event_queue_depth_samples.size(); ++i )
  {
    auto& sample = sim->event_mgr.event_queue_depth_samples[ i ];
    if ( sample.first == 0 )
    {
      continue;
    }

    double p = 100.0 * static_cast<double>( sample.first ) / sim->event_mgr.events_added;
    double p2 = 100.0 * static_cast<double>( sample.second ) / sample.first;
    total_p += p;
    fmt::print( file, "Depth: {:4} Samples: {:9} ({:6.3f}% / {:7.3f}%) tail-inserts: {:9} ({:7.3f}%)\n",
        i,
        sample.first, p, total_p,
        sample.second, p2 );


  }
  fmt::print( file, "Total: {:.3f}% Samples: {}\n",
      total_p,
      sim->event_mgr.events_added );

  fmt::print( file, "\nEvent Queue Allocation:\n" );
  double total_a = 0;
  for ( size_t i = 0; i < sim->event_mgr.event_requested_size_count.size();
        ++i )
  {
    auto& count = sim->event_mgr.event_requested_size_count[ i ];
    if ( count == 0 )
    {
      continue;
    }

    double p = 100.0 * static_cast<double>( count ) / sim->event_mgr.n_requested_events;
    fmt::print( file, "Alloc-Size: {:4} Samples: {:7} ({:6.3f}%)\n",
        i,
        count,
        p );

    total_a += p;
  }

  fmt::print( file, "Total: {:.3f}% Alloc Samples: {}\n",
      total_p,
      sim->event_mgr.n_requested_events );
  fmt::print( file, "Alloc size used for event_t: {}\n",
      util::next_power_of_two( 2 * sizeof( event_t ) ) );
#endif
}

void print_raid_scale_factors( FILE* file, sim_t* sim )
{
  if ( !sim->scaling->has_scale_factors() )
    return;

  fmt::print( file, "\nScale Factors:\n" );

  size_t max_name_length  = 0;

  for ( auto& p : sim->players_by_name )
  {
    auto length  = std::strlen( p->name() );
    if ( length > max_name_length )
      max_name_length = length;
  }

  for ( auto& p : sim->players_by_name )
  {
    fmt::print( file, "  {:<{}}", p->name(), max_name_length );

    scale_metric_e sm = p->sim->scaling->scaling_metric;
    gear_stats_t& sf  = ( sim->scaling->normalize_scale_factors )
                           ? p->scaling->scaling_normalized[ sm ]
                           : p->scaling->scaling[ sm ];

    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( p->scaling->scales_with[ j ] )
      {
        fmt::print( file, "  {}={:f}({:f})",
            util::stat_type_abbrev( j ),
            sf.get_stat( j ),
            p->scaling->scaling_error[ sm ].get_stat( j ));
      }
    }

    if ( sim->scaling->normalize_scale_factors )
    {
      fmt::print( file, "  DPS/{}={:f}",
          util::stat_type_abbrev( p->normalize_by() ),
          p->scaling->scaling[ sm ].get_stat( p->normalize_by() ) );
    }

    if ( p->sim->scaling->scale_lag )
    {
      fmt::print( file, "  ms Lag={:f}({:f})",
          p->scaling->scaling_lag[ sm ],
          p->scaling->scaling_lag_error[ sm ] );
    }

    fmt::print( file, "\n" );
  }
}

// print_text_scale_factors =================================================

void print_player_scale_factors( FILE* file, const player_t& p,
                               const player_processed_report_information_t& ri )
{
  if ( !p.sim->scaling->has_scale_factors() )
    return;

  if ( p.scaling == nullptr )
    return;

  fmt::print( file, "  Scale Factors:\n" );

  scale_metric_e sm = p.sim->scaling->scaling_metric;
  gear_stats_t& sf  = ( p.sim->scaling->normalize_scale_factors )
                         ? p.scaling->scaling_normalized[ sm ]
                         : p.scaling->scaling[ sm ];

  fmt::print( file, "    Weights :" );
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( p.scaling->scales_with[ i ] )
    {
      fmt::print( file, "  {}={}({})",
          util::stat_type_abbrev( i ),
          sf.get_stat( i ),
          p.scaling->scaling_error[ sm ].get_stat( i ) );
    }
  }
  if ( p.sim->scaling->normalize_scale_factors )
  {
    fmt::print( file, "  DPS/{}={}",
        util::stat_type_abbrev( p.normalize_by() ),
        p.scaling->scaling[ sm ].get_stat( p.normalize_by() ) );
  }
  if ( p.sim->scaling->scale_lag )
  {
    fmt::print( file, "  ms Lag={}({})",
        p.scaling->scaling_lag[ sm ],
        p.scaling->scaling_lag_error[ sm ] );
  }

  fmt::print( file, "\n" );

  std::string wowhead_std = ri.gear_weights_wowhead_std_link[ sm ];
  simplify_html( wowhead_std );

  fmt::print( file, "    Wowhead : {}\n", wowhead_std );
}

// print_text_dps_plots =====================================================

void print_dps_plots( FILE* file, const player_t& p )
{
  sim_t& sim = *p.sim;

  if ( sim.plot->dps_plot_stat_str.empty() )
    return;

  int range = sim.plot->dps_plot_points / 2;

  double min = -range * sim.plot->dps_plot_step;
  double max = +range * sim.plot->dps_plot_step;

  int points = 1 + range * 2;

  fmt::print( file, "  DPS Plot Data ( min={} max={} points={} )\n",
      min,
      max,
      points );

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    const auto& pd = p.dps_plot_data[ i ];

    if ( !pd.empty() )
    {
      fmt::print( file, "    DPS({})=", util::stat_type_abbrev( i ) );
      size_t num_points = pd.size();
      for ( size_t j = 0; j < num_points; j++ )
      {
        fmt::print( file, "{}{}", ( j ? "|" : "" ), pd[ j ].value );
      }
      fmt::print( file, "\n" );
    }
  }
}

// print_text_reference_dps =================================================

void print_reference_dps( FILE* file, sim_t& sim )
{
  if ( sim.reference_player_str.empty() )
    return;

  fmt::print( file, "\nReference DPS:\n" );

  const player_t* ref_p = sim.find_player( sim.reference_player_str );

  if ( !ref_p )
  {
    sim.error(fmt::format("Unable to locate reference player: {}.",
                 sim.reference_player_str) );
    return;
  }

  scale_metric_e sm = sim.scaling->scaling_metric;

  size_t max_length  = 0;

  for ( auto& p : sim.players_by_dps )
  {
    auto length  = std::strlen( p->name() );
    if ( length > max_length )
      max_length = length;
  }

  fmt::print( file, "  {:<{}}", ref_p->name(), max_length );
  fmt::print( file, "  {}", ref_p->collected_data.dps.mean() );

  if ( sim.scaling->has_scale_factors() )
  {
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( ref_p->scaling->scales_with[ j ] )
      {
        fmt::print( file, "  {}={:{}.0f}",
            util::stat_type_abbrev( j ),
            ref_p->scaling->scaling[ sm ].get_stat( j ),
            sim.report_precision );
      }
    }
  }

  fmt::print( file, "\n" );

  for ( auto& p : sim.players_by_dps )
  {
    if ( p != ref_p )
    {
      fmt::print( file, "  {:<{}}", p->name(), max_length );

      bool over = ( p->collected_data.dps.mean() > ref_p->collected_data.dps.mean() );

      double ratio = 100.0 * std::fabs( p->collected_data.dps.mean() - ref_p->collected_data.dps.mean() ) /
                     ref_p->collected_data.dps.mean();

      fmt::print( file, "  {}{:5.2f}%", ( over ? '+' : '-' ), ratio );

      if ( sim.scaling->has_scale_factors() )
      {
        for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
        {
          if ( ref_p->scaling->scales_with[ j ] )
          {
            double ref_sf = ref_p->scaling->scaling[ sm ].get_stat( j );
            double sf     = p->scaling->scaling[ sm ].get_stat( j );

            over = ( sf > ref_sf );

            ratio = 100.0 * std::fabs( sf - ref_sf ) / ref_sf;

            fmt::print( file, "  {}={}{:5.2f}%",
                util::stat_type_abbrev( j ),
                ( over ? '+' : '-' ),
                ratio );
          }
        }
      }

      fmt::print( file, "\n" );
    }
  }
}

struct sort_by_event_stopwatch
{
  bool operator()( player_t* l, player_t* r )
  {
    return l->event_stopwatch.current() > r->event_stopwatch.current();
  }
};

void event_manager_infos( FILE* file, const sim_t& sim )
{
#if defined( ACTOR_EVENT_BOOKKEEPING )
  if ( !sim.event_mgr.monitor_cpu )
    return;

  fmt::print( file, "\nEvent Manager CPU Report:\n" );
  std::vector<player_t*> sorted_p = sim.player_list.data();

  double total_event_time = sim.event_mgr.event_stopwatch.current();
  for ( const auto& player : sorted_p )
  {
    total_event_time += player->event_stopwatch.current();
  }
  fmt::print( file, "{:10.2f}sec / {:5.2f}% : Global Events\n",
      sim.event_mgr.event_stopwatch.current(),
      sim.event_mgr.event_stopwatch.current() / total_event_time * 100.0 );

  range::sort( sorted_p, sort_by_event_stopwatch() );
  for ( const auto& p : sorted_p )
  {
    fmt::print( file, "{:10.3f}sec / {:5.2f}% : {}\n",
        p->event_stopwatch.current(),
        p->event_stopwatch.current() / total_event_time * 100.0,
        p->name() );
  }
#endif  // ACTOR_EVENT_BOOKKEEPING
}

void print_collected_amount( FILE* file, const player_t& p, std::string name, const extended_sample_data_t& sd )
{
  if ( sd.sum() <= 0.0 )
    return;

  double error =
      sim_t::distribution_mean_error( *p.sim, sd );
  fmt::print( file, "  {}={} {}-Error={}/{:.2f}% {}-Range={}/{:.2f}%\n",
      name, sd.mean(),
      name, error, error * 100 / sd.mean(),
      name, ( sd.max() - sd.min() ) / 2.0, ( ( sd.max() - sd.min() ) / 2 ) * 100 / sd.mean() );
}

void print_player( FILE* file, player_t& p )
{
  report::generate_player_buff_lists( p, p.report_information );
  report::generate_player_charts( p, p.report_information ); // For WoWhead/Pawn String

  const player_collected_data_t& cd = p.collected_data;

  fmt::print( file, "\n{}: {} {} {} {} {}\n",
      p.is_enemy() ? "Target" : p.is_add() ? "Add" : "Player",
      p.name(),
      p.race_str,
      util::player_type_string( p.type ),
      dbc::specialization_string( p.specialization() ),
      p.true_level );

  print_collected_amount(file, p, "DPS", cd.dps );
  print_collected_amount(file, p, "HPS", cd.hps );
  print_collected_amount(file, p, "DTPS", cd.dtps );
  print_collected_amount(file, p, "TMI", cd.theck_meloree_index );

  if ( p.rps_loss > 0.0 )
  {
    fmt::print( file, "  DPR={} RPS-Out={} RPS-In={} Resource={} Waiting={} ApM={}",
        p.dpr,
        p.rps_loss,
        p.rps_gain,
        util::resource_type_string( p.primary_resource() ),
        100.0 * p.collected_data.waiting_time.mean() / p.collected_data.fight_length.mean(),
        60.0 * p.collected_data.executed_foreground_actions.mean() / p.collected_data.fight_length.mean() );
  }

  fmt::print( file, "\n" );

  if ( !p.origin_str.empty() )
    fmt::print( file, "  Origin: {}\n", p.origin_str );
  if ( !p.talents_str.empty() )
    fmt::print( file, "  Talents: {}\n", p.talents_str );
  if ( p.artifact && !p.artifact->artifact_option_string().empty() )
    fmt::print( file, "  Artifact: {}\n", p.artifact->crucible_option_string() );
  if ( p.artifact && !p.artifact->crucible_option_string().empty() )
    fmt::print( file, "  Crucible: {}\n", p.artifact->crucible_option_string() );
  print_core_stats( file, p );
  print_generic_stats( file, p );
  print_spell_stats( file, p );
  print_attack_stats( file, p );
  print_defense_stats( file, p );
  print_player_actions( file, p );

  print_player_buffs( file, p.report_information );
  print_uptimes_benefits( file, p );
  print_procs( file, p );
  print_player_gains( file, p );
  print_player_scale_factors( file, p, p.report_information );
  print_dps_plots( file, p );
  print_waiting_player( file, p );
}

void print_player_sequence( FILE* file, sim_t* sim, std::vector<player_t*> players, bool detail )
{
  (void) detail;
  for ( auto& player : players )
  {
    print_player( file, *player );

    // Pets
    if ( sim->report_pets_separately )
    {
      for ( auto& pet : player->pet_list )
      {
        if ( pet->summoned && !pet->quiet )
        {
          print_player( file, *pet );
        }
      }
    }
  }
}

void print_text_report( FILE* file, sim_t* sim, bool detail )
{
#if SC_BETA
  fmt::print( file, "\n" );
  auto beta_warnings = report::beta_warnings();
  for ( const auto& line : beta_warnings )
  {
    fmt::print( file, " * {} \n", line );
  }
#endif

  // Raid Events
  if ( !sim->raid_events_str.empty() )
  {
    fmt::print( file, "\n\nRaid Events:\n" );
    auto raid_event_names = util::string_split( sim->raid_events_str, "/" );
    if ( !raid_event_names.empty() )
    {
      fmt::print( file, "  raid_event=/{}\n", raid_event_names[ 0 ] );
    }
    for ( size_t i = 1; i < raid_event_names.size(); i++ )
    {
      fmt::print( file, "  raid_event+=/{}\n", raid_event_names[ i ] );
    }
    fmt::print( file, "\n" );
  }

  // DPS & HPS Rankings
  if ( detail )
  {
    fmt::print( file, "\nDPS Ranking:\n" );
    fmt::print( file, "{:7.0f} 100.0%%  Raid\n", sim->raid_dps.mean() );
    for ( auto& player : sim->players_by_dps )
    {
      if ( player->collected_data.dps.mean() <= 0 )
        continue;

      fmt::print( file, "{:7.0f}  {:4.1f}%  {}\n", player->collected_data.dps.mean(),
          sim->raid_dps.mean() ? 100 * player->collected_data.dpse.mean() / sim->raid_dps.mean() : 0.0,
          player->name() );
    }

    if ( !sim->players_by_hps.empty() )
    {
      fmt::print( file, "\nHPS Ranking:\n" );
      fmt::print( file, "{:7.0f} 100.0%%  Raid\n",
                     sim->raid_hps.mean() + sim->raid_aps.mean() );
      for ( auto& p : sim->players_by_hps )
      {
        if ( p->collected_data.hps.mean() <= 0 && p->collected_data.aps.mean() <= 0 )
          continue;

        fmt::print( file, "{:7.0f}  {:4.1f}%  {}\n",
            p->collected_data.hps.mean() + p->collected_data.aps.mean(),
            sim->raid_hps.mean()
                ? 100 * p->collected_data.hpse.mean() / sim->raid_hps.mean()
                : 0,
            p->name() );
      }
    }
  }

  // Report Players
  print_player_sequence( file, sim, sim->players_by_name, detail );


  // Report Targets
  if ( sim->report_targets )
  {
    fmt::print( file, "\n\n *** Targets *** \n" );

    print_player_sequence( file, sim, sim->targets_by_name, detail );
  }

  sim -> profilesets.output( *sim, file );

  sim_summary_performance( file, sim );

  if ( detail )
  {
    print_waiting_all( file, *sim );
    print_iteration_data( file, *sim );
    print_raid_scale_factors( file, sim );
    print_reference_dps( file, *sim );
    event_manager_infos( file, sim );
  }

  fmt::print( file, "\n" );
}
}  // UNNAMED NAMESPACE ====================================================

namespace report
{
void print_text( sim_t* sim, bool detail )
{
  FILE* text_out = stdout;
  io::cfile file;
  if ( !sim->output_file_str.empty() )
  {
    file = io::fopen( sim->output_file_str, "a" );
    if ( !file )
    {
      sim->errorf( "Failed to open text output file '%s'.\nUsing stdout.",
                   sim->output_file_str.c_str() );
    }
  }
  if ( file )
  {
    text_out = file;
  }

  if ( sim->simulation_length.sum() == 0.0 )
    return;

  try
  {
    Timer t( "text report" );
    if ( ! sim -> profileset_enabled )
    {
      t.start();
    }

    print_text_report( text_out, sim, detail );
  }
  catch ( const std::exception& e )
  {
    sim->errorf( "Failed to print text output! %s", e.what() );
#ifndef NDEBUG
    throw;
#endif
  }
}

}  // END report NAMESPACE

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

// print_html_contents ======================================================

void print_html_contents( report::sc_html_stream& os, sim_t* sim )
{
  size_t c = 2;     // total number of TOC entries
  if ( sim -> scaling -> has_scale_factors() )
    ++c;

  size_t num_players = sim -> players_by_name.size();
  c += num_players;
  if ( sim -> report_targets )
    c += sim -> targets_by_name.size();

  if ( sim -> report_pets_separately )
  {
    for ( size_t i = 0; i < num_players; i++ )
    {
      for ( size_t j = 0; j < sim -> players_by_name[ i ] -> pet_list.size(); ++j )
      {
        pet_t* pet = sim -> players_by_name[ i ] -> pet_list[ j ];

        if ( pet -> summoned && !pet -> quiet )
          ++c;
      }
    }
  }

  os << "\t\t<div id=\"table-of-contents\" class=\"section grouped-first grouped-last\">\n"
     << "\t\t\t<h2 class=\"toggle\">Table of Contents</h2>\n"
     << "\t\t\t<div class=\"toggle-content hide\">\n";

  // set number of columns
  int n;         // number of columns
  const char* toc_class; // css class
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

  int pi = 0;    // player counter
  int ab = 0;    // auras and debuffs link added yet?
  for ( int i=0; i < n; i++ )
  {
    int cs;    // column size
    if ( i == 0 )
    {
      cs = ( int ) ceil( 1.0 * c / n );
    }
    else if ( i == 1 )
    {
      if ( n == 2 )
      {
        cs = ( int ) ( c - ceil( 1.0 * c / n ) );
      }
      else
      {
        cs = ( int ) ceil( 1.0 * c / n );
      }
    }
    else
    {
      cs = ( int ) ( c - 2 * ceil( 1.0 * c / n ) );
    }

    os << "\t\t\t\t<ul class=\"toc " << toc_class << "\">\n";

    int ci = 1;    // in-column counter
    if ( i == 0 )
    {
      os << "\t\t\t\t\t<li><a href=\"#raid-summary\">Raid Summary</a></li>\n";
      ci++;
      if ( sim -> scaling -> has_scale_factors() )
      {
        os << "\t\t\t\t\t<li><a href=\"#raid-scale-factors\">Scale Factors</a></li>\n";
        ci++;
      }
    }
    while ( ci <= cs )
    {
      if ( pi < static_cast<int>( sim -> players_by_name.size() ) )
      {
        player_t* p = sim -> players_by_name[ pi ];
        std::string html_name = p -> name();
        util::encode_html( html_name );
        os << "\t\t\t\t\t<li><a href=\"#player" << p -> index << "\">" << html_name << "</a>";
        ci++;
        if ( sim -> report_pets_separately )
        {
          os << "\n\t\t\t\t\t\t<ul>\n";
          for ( size_t k = 0; k < sim -> players_by_name[ pi ] -> pet_list.size(); ++k )
          {
            pet_t* pet = sim -> players_by_name[ pi ] -> pet_list[ k ];
            if ( pet -> summoned )
            {
              html_name = pet -> name();
              util::encode_html( html_name );
              os << "\t\t\t\t\t\t\t<li><a href=\"#player" << pet -> index << "\">" << html_name << "</a></li>\n";
              ci++;
            }
          }
          os << "\t\t\t\t\t\t</ul>";
        }
        os << "</li>\n";
        pi++;
      }
      if ( pi == static_cast<int>( sim -> players_by_name.size() ) )
      {
        if ( ab == 0 )
        {
          os << "\t\t\t\t\t<li><a href=\"#auras-buffs\">Auras/Buffs</a></li>\n";
          ab = 1;
        }
        ci++;
        os << "\t\t\t\t\t<li><a href=\"#sim-info\">Simulation Information</a></li>\n";
        ci++;
        if ( sim -> report_raw_abilities )
        {
          os << "\t\t\t\t\t<li><a href=\"#raw-abilities\">Raw Ability Summary</a></li>\n";
          ci++;
        }
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
          if ( pi < static_cast<int>( sim -> targets_by_name.size() ) )
          {
            player_t* p = sim -> targets_by_name[ pi ];
            os << "\t\t\t\t\t<li><a href=\"#player" << p -> index << "\">"
               << util::encode_html( p -> name() ) << "</a></li>\n";
          }
          ci++;
          pi++;
        }
      }
    }
    os << "\t\t\t\t</ul>\n";
  }

  os << "\t\t\t\t<div class=\"clear\"></div>\n"
     << "\t\t\t</div>\n\n"
     << "\t\t</div>\n\n";
}

// print_html_sim_summary ===================================================

void print_html_sim_summary( report::sc_html_stream& os, sim_t* sim, sim_t::report_information_t& ri )
{
  os << "\t\t\t\t<div id=\"sim-info\" class=\"section\">\n";

  os << "\t\t\t\t\t<h2 class=\"toggle\">Simulation & Raid Information</h2>\n"
     << "\t\t\t\t\t\t<div class=\"toggle-content hide\">\n";

  os << "\t\t\t\t\t\t<table class=\"sc mt\">\n";

  os << "\t\t\t\t\t\t\t<tr class=\"left\">\n"
     << "\t\t\t\t\t\t\t\t<th>Iterations:</th>\n"
     << "\t\t\t\t\t\t\t\t<td>" << sim -> iterations << "</td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  os << "\t\t\t\t\t\t\t<tr class=\"left\">\n"
     << "\t\t\t\t\t\t\t\t<th>Threads:</th>\n"
     << "\t\t\t\t\t\t\t\t<td>" << ( ( sim -> threads < 1 ) ? 1 : sim -> threads ) << "</td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  os << "\t\t\t\t\t\t\t<tr class=\"left\">\n"
     << "\t\t\t\t\t\t\t\t<th>Confidence:</th>\n"
     << "\t\t\t\t\t\t\t\t<td>" << sim -> confidence * 100.0 << "%</td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  os.printf( "\t\t\t\t\t\t\t<tr class=\"left\">\n"
             "\t\t\t\t\t\t\t\t<th>Fight Length:</th>\n"
             "\t\t\t\t\t\t\t\t<td>%.0f - %.0f ( %.1f )</td>\n"
             "\t\t\t\t\t\t\t</tr>\n",
             sim -> simulation_length.min,
             sim -> simulation_length.max,
             sim -> simulation_length.mean );

  os << "\t\t\t\t\t\t\t<tr class=\"left\">\n"
     << "\t\t\t\t\t\t\t\t<th><h2>Performance:</h2></th>\n"
     << "\t\t\t\t\t\t\t\t<td></td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  os.printf(
    "\t\t\t\t\t\t\t<tr class=\"left\">\n"
    "\t\t\t\t\t\t\t\t<th>Total Events Processed:</th>\n"
    "\t\t\t\t\t\t\t\t<td>%ld</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    ( long ) sim -> total_events_processed );

  os.printf(
    "\t\t\t\t\t\t\t<tr class=\"left\">\n"
    "\t\t\t\t\t\t\t\t<th>Max Event Queue:</th>\n"
    "\t\t\t\t\t\t\t\t<td>%ld</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    ( long ) sim -> max_events_remaining );

  os.printf(
    "\t\t\t\t\t\t\t<tr class=\"left\">\n"
    "\t\t\t\t\t\t\t\t<th>Sim Seconds:</th>\n"
    "\t\t\t\t\t\t\t\t<td>%.0f</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    sim -> iterations * sim -> simulation_length.mean );
  os.printf(
    "\t\t\t\t\t\t\t<tr class=\"left\">\n"
    "\t\t\t\t\t\t\t\t<th>CPU Seconds:</th>\n"
    "\t\t\t\t\t\t\t\t<td>%.4f</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    sim -> elapsed_cpu.total_seconds() );
  os.printf(
    "\t\t\t\t\t\t\t<tr class=\"left\">\n"
    "\t\t\t\t\t\t\t\t<th>Speed Up:</th>\n"
    "\t\t\t\t\t\t\t\t<td>%.0f</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    sim -> iterations * sim -> simulation_length.mean / sim -> elapsed_cpu.total_seconds() );

  os << "\t\t\t\t\t\t\t<tr class=\"left\">\n"
     << "\t\t\t\t\t\t\t\t<th><h2>Settings:</h2></th>\n"
     << "\t\t\t\t\t\t\t\t<td></td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  os.printf(
    "\t\t\t\t\t\t\t<tr class=\"left\">\n"
    "\t\t\t\t\t\t\t\t<th>World Lag:</th>\n"
    "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    ( double )sim -> world_lag.total_millis(), ( double )sim -> world_lag_stddev.total_millis() );
  os.printf(
    "\t\t\t\t\t\t\t<tr class=\"left\">\n"
    "\t\t\t\t\t\t\t\t<th>Queue Lag:</th>\n"
    "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    ( double )sim -> queue_lag.total_millis(), ( double )sim -> queue_lag_stddev.total_millis() );

  if ( sim -> strict_gcd_queue )
  {
    os.printf(
      "\t\t\t\t\t\t\t<tr class=\"left\">\n"
      "\t\t\t\t\t\t\t\t<th>GCD Lag:</th>\n"
      "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
      "\t\t\t\t\t\t\t</tr>\n",
      ( double )sim -> gcd_lag.total_millis(), ( double )sim -> gcd_lag_stddev.total_millis() );
    os.printf(
      "\t\t\t\t\t\t\t<tr class=\"left\">\n"
      "\t\t\t\t\t\t\t\t<th>Channel Lag:</th>\n"
      "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
      "\t\t\t\t\t\t\t</tr>\n",
      ( double )sim -> channel_lag.total_millis(), ( double )sim -> channel_lag_stddev.total_millis() );
    os.printf(
      "\t\t\t\t\t\t\t<tr class=\"left\">\n"
      "\t\t\t\t\t\t\t\t<th>Queue GCD Reduction:</th>\n"
      "\t\t\t\t\t\t\t\t<td>%.0f ms</td>\n"
      "\t\t\t\t\t\t\t</tr>\n",
      ( double )sim -> queue_gcd_reduction.total_millis() );
  }

  os << "\t\t\t\t\t\t</table>\n";
  report::print_html_sample_data( os, sim, sim -> simulation_length, "Simulation Length" );


  // Left side charts: dps, gear, timeline, raid events

  os << "\t\t\t\t<div class=\"charts charts-left\">\n";
  // Timeline Distribution Chart
  if ( sim -> iterations > 1 && ! ri.timeline_chart.empty() )
  {
    os.printf(
      "\t\t\t\t\t<a href=\"#help-timeline-distribution\" class=\"help\"><img src=\"%s\" alt=\"Timeline Distribution Chart\" /></a>\n",
      ri.timeline_chart.c_str() );
  }

  // Gear Charts
  for ( size_t i = 0; i < ri.gear_charts.size(); i++ )
  {
    os << "\t\t\t\t\t<img src=\"" << ri.gear_charts[ i ] << "\" alt=\"Gear Chart\" />\n";
  }

  // Raid Downtime Chart
  if ( !  ri.downtime_chart.empty() )
  {
    os.printf(
      "\t\t\t\t\t<img src=\"%s\" alt=\"Raid Downtime Chart\" />\n",
      ri.downtime_chart.c_str() );
  }

  os << "\t\t\t\t</div>\n";

  // Right side charts: dpet
  os << "\t\t\t\t<div class=\"charts\">\n";

  for ( size_t i = 0; i < ri.dpet_charts.size(); i++ )
  {
    os.printf(
      "\t\t\t\t\t<img src=\"%s\" alt=\"DPET Chart\" />\n",
      ri.dpet_charts[ i ].c_str() );
  }

  os << "\t\t\t\t</div>\n";


  // closure
  os << "\t\t\t\t<div class=\"clear\"></div>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n\n";
}


// print_html_raw_ability_summary ===================================================

double aggregate_damage( std::vector<stats_t::stats_results_t> result )
{
  double total = 0;
  for ( size_t i = 0; i < result.size(); i++ )
  {
    total  += result[ i ].fight_actual_amount.mean;
  }
  return total;
}

int find_id( stats_t* s )
{
  int id = 0;
  /*
  for ( size_t i = 0; i < s -> player -> action_list.size(); ++i )
    {
      action_t* a = s -> player -> action_list[ i ];
      if ( a -> stats != s ) continue;
      id = a -> id;
      if ( ! a -> background ) break;
    }
  */
  for ( size_t i = 0; i < s -> action_list.size(); i++ )
  {
    if ( s -> action_list[ i ] -> id != 0 )
    {
      id = s -> action_list[ i ] -> id;
      break;
    }
  }
  return id;
}

void print_html_raw_action_damage( report::sc_html_stream& os, stats_t* s, player_t* p, int j, sim_t* sim )
{
  if ( s -> num_executes.mean == 0 && s -> compound_amount == 0 && !sim -> debug )
    return;

  os << "\t\t\t<tr";
  if ( j & 1 )
    os << " class=\"odd\"";
  os << ">\n";

  int id = find_id( s );

  char format[] =
    "\t\t\t\t\t<td class=\"left  small\">%s</td>\n"
    "\t\t\t\t\t<td class=\"left  small\">%s</td>\n"
    "\t\t\t\t\t<td class=\"left  small\">%s%s</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%d</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.2fsec</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.2fsec</td>\n"
    "\t\t\t\t</tr>\n";

  double direct_total = aggregate_damage( s -> direct_results );
  double tick_total = aggregate_damage( s -> tick_results );
  if ( direct_total > 0.0 || tick_total <= 0.0 )
    os.printf(
      format,
      util::encode_html( p -> name() ).c_str(),
      util::encode_html( s -> player -> name() ).c_str(),
      s -> name_str.c_str(), "",
      id,
      direct_total,
      direct_total / s -> player -> fight_length.mean,
      s -> num_direct_results.mean / ( s -> player -> fight_length.mean / 60.0 ),
      s -> direct_results[ RESULT_HIT  ].actual_amount.mean,
      s -> direct_results[ RESULT_CRIT ].actual_amount.mean,
      s -> num_executes.mean,
      s -> num_direct_results.mean,
      s -> direct_results[ RESULT_CRIT ].pct,
      s -> direct_results[ RESULT_MISS ].pct + s -> direct_results[ RESULT_DODGE  ].pct + s -> direct_results[ RESULT_PARRY  ].pct,
      s -> direct_results[ RESULT_GLANCE ].pct,
      s -> direct_results[ RESULT_BLOCK  ].pct,
      s -> total_intervals.mean,
      s -> total_amount.mean,
      s -> player -> fight_length.mean );

  if ( tick_total > 0.0 )
    os.printf(
      format,
      util::encode_html( p -> name() ).c_str(),
      util::encode_html( s -> player -> name() ).c_str(),
      s -> name_str.c_str(), " ticks",
      -id,
      tick_total,
      tick_total / sim -> max_time.total_seconds(),
      s -> num_ticks.mean / sim -> max_time.total_minutes(),
      s -> tick_results[ RESULT_HIT  ].actual_amount.mean,
      s -> tick_results[ RESULT_CRIT ].actual_amount.mean,
      s -> num_executes.mean,
      s -> num_ticks.mean,
      s -> tick_results[ RESULT_CRIT ].pct,
      s -> tick_results[ RESULT_MISS ].pct + s -> tick_results[ RESULT_DODGE  ].pct + s -> tick_results[ RESULT_PARRY  ].pct,
      s -> tick_results[ RESULT_GLANCE ].pct,
      s -> tick_results[ RESULT_BLOCK  ].pct,
      s -> total_intervals.mean,
      s -> total_amount.mean,
      s -> player -> fight_length.mean );

  for ( size_t i = 0, num_children = s -> children.size(); i < num_children; i++ )
  {
    print_html_raw_action_damage( os, s -> children[ i ], p, j, sim );
  }
}

void print_html_raw_ability_summary( report::sc_html_stream& os, sim_t* sim )
{
  os << "\t\t<div id=\"raw-abilities\" class=\"section\">\n\n";
  os << "\t\t\t<h2 class=\"toggle\">Raw Ability Summary</h2>\n"
     << "\t\t\t<div class=\"toggle-content hide\">\n";

  // Abilities Section
  os << "\t\t\t<table class=\"sc\">\n"
     << "\t\t\t\t<tr>\n"
     << "\t\t\t\t\t<th class=\"left small\">Character</th>\n"
     << "\t\t\t\t\t<th class=\"left small\">Unit</th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ability\" class=\"help\">Ability</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-id\" class=\"help\">Id</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-total\" class=\"help\">Total</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ipm\" class=\"help\">Imp/Min</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-direct-results\" class=\"help\">Impacts</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-miss-pct\" class=\"help\">Avoid%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-glance-pct\" class=\"help\">G%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-block-pct\" class=\"help\">B%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-combined\" class=\"help\">Combined</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-duration\" class=\"help\">Duration</a></th>\n"
     << "\t\t\t\t</tr>\n";

  int count = 0;
  for ( size_t player_i = 0; player_i < sim -> players_by_name.size(); player_i++ )
  {
    player_t* p = sim -> players_by_name[ player_i ];
    for ( size_t i = 0; i < p -> stats_list.size(); ++i )
    {
      stats_t* s = p -> stats_list[ i ];
      if ( s -> parent == NULL )
        print_html_raw_action_damage( os, s, p, count++, sim );
    }

    for ( size_t pet_i = 0; pet_i < p -> pet_list.size(); ++pet_i )
    {
      pet_t* pet = p -> pet_list[ pet_i ];
      for ( size_t i = 0; i < pet -> stats_list.size(); ++i )
      {
        stats_t* s = pet -> stats_list[ i ];
        if ( s -> parent == NULL )
          print_html_raw_action_damage( os, s, p, count++, sim );
      }
    }
  }

  // closure
  os << "\t\t\t\t</table>\n";
  os << "\t\t\t\t<div class=\"clear\"></div>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n\n";
}

// print_html_raid_summary ==================================================

void print_html_raid_summary( report::sc_html_stream& os, sim_t* sim, sim_t::report_information_t& ri )
{
  os << "\t\t<div id=\"raid-summary\" class=\"section section-open\">\n\n";

  os << "\t\t\t<h2 class=\"toggle open\">Raid Summary</h2>\n";

  os << "\t\t\t<div class=\"toggle-content\">\n";

  os << "\t\t\t<ul class=\"params\">\n";

  os.printf(
    "\t\t\t\t<li><b>Raid Damage:</b> %.0f</li>\n",
    sim -> total_dmg.mean );
  os.printf(
    "\t\t\t\t<li><b>Raid DPS:</b> %.0f</li>\n",
    sim -> raid_dps.mean );
  if ( sim -> total_heal.mean > 0 )
  {
    os.printf(
      "\t\t\t\t<li><b>Raid Heal:</b> %.0f</li>\n",
      sim -> total_heal.mean );
    os.printf(
      "\t\t\t\t<li><b>Raid HPS:</b> %.0f</li>\n",
      sim -> raid_hps.mean );
  }
  os << "\t\t\t</ul><p>&nbsp;</p>\n";

  // Left side charts: dps, raid events
  os << "\t\t\t\t<div class=\"charts charts-left\">\n";

  for ( size_t i = 0; i < ri.dps_charts.size(); i++ )
  {
    os.printf(
      "\t\t\t\t\t<map id='DPSMAP%d' name='DPSMAP%d'></map>\n", ( int )i, ( int )i );
    os.printf(
      "\t\t\t\t\t<img id='DPSIMG%d' src=\"%s\" alt=\"DPS Chart\" />\n",
      ( int )i, ri.dps_charts[ i ].c_str() );
  }

  if ( ! sim -> raid_events_str.empty() )
  {
    os << "\t\t\t\t\t<table>\n"
       << "\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t<th></th>\n"
       << "\t\t\t\t\t\t\t<th class=\"left\">Raid Event List</th>\n"
       << "\t\t\t\t\t\t</tr>\n";

    std::vector<std::string> raid_event_names;
    size_t num_raid_events = util::string_split( raid_event_names, sim -> raid_events_str, "/" );
    for ( size_t i = 0; i < num_raid_events; i++ )
    {
      os << "\t\t\t\t\t\t<tr";
      if ( ( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";

      os.printf(
        "\t\t\t\t\t\t\t<th class=\"right\">%d</th>\n"
        "\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
        "\t\t\t\t\t\t</tr>\n",
        ( int )i,
        raid_event_names[ i ].c_str() );
    }
    os << "\t\t\t\t\t</table>\n";
  }
  os << "\t\t\t\t</div>\n";

  // Right side charts: hps
  os << "\t\t\t\t<div class=\"charts\">\n";

  for ( size_t i = 0; i < ri.hps_charts.size(); i++ )
  {
    os.printf(  "\t\t\t\t\t<map id='HPSMAP%d' name='HPSMAP%d'></map>\n", ( int )i, ( int )i );
    os.printf(
      "\t\t\t\t\t<img id='HPSIMG%d' src=\"%s\" alt=\"HPS Chart\" />\n",
      ( int )i, ri.hps_charts[ i ].c_str() );
  }

  // RNG chart
  if ( sim -> report_rng )
  {
    os << "\t\t\t\t\t<ul>\n";
    for ( size_t i = 0; i < sim -> players_by_name.size(); i++ )
    {
      player_t* p = sim -> players_by_name[ i ];
      double range = ( p -> dps.percentile( 0.95 ) - p -> dps.percentile( 0.05 ) ) / 2.0;
      os.printf(
        "\t\t\t\t\t\t<li>%s: %.1f / %.1f%%</li>\n",
        util::encode_html( p -> name() ).c_str(),
        range,
        p -> dps.mean ? ( range * 100 / p -> dps.mean ) : 0 );
    }
    os << "\t\t\t\t\t</ul>\n";
  }

  os << "\t\t\t\t</div>\n";

  // closure
  os << "\t\t\t\t<div class=\"clear\"></div>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n\n";

}

// print_html_raid_imagemaps ==================================================

void print_html_raid_imagemap( report::sc_html_stream& os, sim_t* sim, int num, bool dps )
{
  std::vector<player_t*> player_list = ( dps ) ? sim -> players_by_dps : sim -> players_by_hps;
  size_t start = num * MAX_PLAYERS_PER_CHART;
  size_t end = start + MAX_PLAYERS_PER_CHART;

  for ( size_t i = 0; i < player_list.size(); i++ )
  {
    player_t* p = player_list[ i ];
    if ( dps ? p -> dps.mean <= 0 : p -> hps.mean <=0 )
    {
      player_list.resize( i );
      break;
    }
  }

  if ( end > player_list.size() ) end = static_cast<unsigned>( player_list.size() );

  os << "\t\t\tn = [";
  for ( int i = ( int )end-1; i >= ( int )start; i-- )
  {
    os << "\"" << util::encode_html( player_list[i] -> name() ) << "\"";
    if ( i != ( int )start ) os << ", ";
  }
  os << "];\n";

  char imgid[32];
  util::snprintf( imgid, sizeof( imgid ), "%sIMG%d", ( dps ) ? "DPS" : "HPS", num );
  char mapid[32];
  util::snprintf( mapid, sizeof( mapid ), "%sMAP%d", ( dps ) ? "DPS" : "HPS", num );

  os.printf(
    "\t\t\tu = document.getElementById('%s').src;\n"
    "\t\t\tgetMap(u, n, function(mapStr) {\n"
    "\t\t\t\tdocument.getElementById('%s').innerHTML += mapStr;\n"
    "\t\t\t\t$j('#%s').attr('usemap','#%s');\n"
    "\t\t\t\t$j('#%s area').click(function(e) {\n"
    "\t\t\t\t\tanchor = $j(this).attr('href');\n"
    "\t\t\t\t\ttarget = $j(anchor).children('h2:first');\n"
    "\t\t\t\t\topen_anchor(target);\n"
    "\t\t\t\t});\n"
    "\t\t\t});\n\n",
    imgid, mapid, imgid, mapid, mapid );
}

void print_html_raid_imagemaps( report::sc_html_stream& os, sim_t* sim, sim_t::report_information_t& ri )
{

  os << "\t\t<script type=\"text/javascript\">\n"
     << "\t\t\tvar $j = jQuery.noConflict();\n"
     << "\t\t\tfunction getMap(url, names, mapWrite) {\n"
     << "\t\t\t\t$j.getJSON(url + '&chof=json&callback=?', function(jsonObj) {\n"
     << "\t\t\t\t\tvar area = false;\n"
     << "\t\t\t\t\tvar chart = jsonObj.chartshape;\n"
     << "\t\t\t\t\tvar mapStr = '';\n"
     << "\t\t\t\t\tfor (var i = 0; i < chart.length; i++) {\n"
     << "\t\t\t\t\t\tarea = chart[i];\n"
     << "\t\t\t\t\t\tarea.coords[2] = 523;\n"
     << "\t\t\t\t\t\tmapStr += \"\\n  <area name='\" + area.name + \"' shape='\" + area.type + \"' coords='\" + area.coords.join(\",\") + \"' href='#\" + names[i] + \"'  title='\" + names[i] + \"'>\";\n"
     << "\t\t\t\t\t}\n"
     << "\t\t\t\t\tmapWrite(mapStr);\n"
     << "\t\t\t\t});\n"
     << "\t\t\t}\n\n";

  for ( size_t i = 0; i < ri.dps_charts.size(); i++ )
  {
    print_html_raid_imagemap( os, sim, i, true );
  }

  for ( size_t i = 0; i < ri.hps_charts.size(); i++ )
  {
    print_html_raid_imagemap( os, sim, i, false );
  }

  os << "\t\t</script>\n";

}

// print_html_scale_factors =================================================

void print_html_scale_factors( report::sc_html_stream& os, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  os << "\t\t<div id=\"raid-scale-factors\" class=\"section grouped-first\">\n\n"
     << "\t\t\t<h2 class=\"toggle\">DPS Scale Factors (dps increase per unit stat)</h2>\n"
     << "\t\t\t<div class=\"toggle-content hide\">\n";

  os << "\t\t\t\t<table class=\"sc\">\n";

  player_e prev_type = PLAYER_NONE;

  for ( size_t i = 0, players = sim -> players_by_name.size(); i < players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if ( p -> type != prev_type )
    {
      prev_type = p -> type;

      os << "\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t<th class=\"left small\">Profile</th>\n";
      for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
      {
        if ( sim -> scaling -> stats.get_stat( j ) != 0 )
        {
          os << "\t\t\t\t\t\t<th class=\"small\">" << util::stat_type_abbrev( j ) << "</th>\n";
        }
      }
      os << "\t\t\t\t\t\t<th class=\"small\">wowhead</th>\n"
         << "\t\t\t\t\t\t<th class=\"small\">lootrank</th>\n"
         << "\t\t\t\t\t</tr>\n";
    }

    os << "\t\t\t\t\t<tr";
    if ( ( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
      "\t\t\t\t\t\t<td class=\"left small\">%s</td>\n",
      p -> name() );
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        if ( p -> scaling.get_stat( j ) == 0 )
        {
          os << "\t\t\t\t\t\t<td class=\"small\">-</td>\n";
        }
        else
        {
          os.printf(
            "\t\t\t\t\t\t<td class=\"small\">%.*f</td>\n",
            sim -> report_precision,
            p -> scaling.get_stat( j ) );
        }
      }
    }
    os.printf(
      "\t\t\t\t\t\t<td class=\"small\"><a href=\"%s\"> wowhead </a></td>\n"
      "\t\t\t\t\t\t<td class=\"small\"><a href=\"%s\"> lootrank</a></td>\n"
      "\t\t\t\t\t</tr>\n",
      p -> report_information.gear_weights_wowhead_link.c_str(),
      p -> report_information.gear_weights_lootrank_link.c_str() );
  }
  os << "\t\t\t\t</table>\n";

  if ( sim -> iterations < 10000 )
    os << "\t\t\t\t<div class=\"alert\">\n"
       << "\t\t\t\t\t<h3>Warning</h3>\n"
       << "\t\t\t\t\t<p>Scale Factors generated using less than 10,000 iterations will vary from run to run.</p>\n"
       << "\t\t\t\t</div>\n";

  os << "\t\t\t</div>\n"
     << "\t\t</div>\n\n";
}

// print_html_help_boxes ====================================================

void print_html_help_boxes( report::sc_html_stream& os, sim_t* sim )
{
  os << "\t\t<!-- Help Boxes -->\n";

  os << "\t\t<div id=\"help-apm\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>APM</h3>\n"
     << "\t\t\t\t<p>Average number of actions executed per minute.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-constant-buffs\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Constant Buffs</h3>\n"
     << "\t\t\t\t<p>Buffs received prior to combat and present the entire fight.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-count\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Count</h3>\n"
     << "\t\t\t\t<p>Average number of times an action is executed per iteration.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-direct-results\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Impacts</h3>\n"
     << "\t\t\t\t<p>Average number of impacts against a target (for attacks that hit multiple times per execute) per iteration.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-crit\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Crit</h3>\n"
     << "\t\t\t\t<p>Average crit damage.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-crit-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Crit%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in critical strikes.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dodge-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Dodge%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in dodges.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dpe\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>DPE</h3>\n"
     << "\t\t\t\t<p>Average damage per execution of an individual action.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dpet\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>DPET</h3>\n"
     << "\t\t\t\t<p>Average damage per execute time of an individual action; the amount of damage generated, divided by the time taken to execute the action, including time spent in the GCD.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dpr\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>DPR</h3>\n"
     << "\t\t\t\t<p>Average damage per resource point spent.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dps\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>DPS</h3>\n"
     << "\t\t\t\t<p>Average damage per active player duration.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dpse\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Effective DPS</h3>\n"
     << "\t\t\t\t<p>Average damage per fight duration.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dps-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>DPS%</h3>\n"
     << "\t\t\t\t<p>Percentage of total DPS contributed by a particular action.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dynamic-buffs\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Dynamic Buffs</h3>\n"
     << "\t\t\t\t<p>Temporary buffs received during combat, perhaps multiple times.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-error\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Error</h3>\n"
     << "\t\t\t\t<p>Estimator for the " << sim -> confidence * 100.0 << "confidence intervall.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-glance-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>G%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in glancing blows.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-block-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>B%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in blocking blows.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-id\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Id</h3>\n"
     << "\t\t\t\t<p>Associated spell-id for this ability.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ability\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Ability</h3>\n"
     << "\t\t\t\t<p>Name of the ability</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-total\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Total</h3>\n"
     << "\t\t\t\t<p>Total damage for this ability during the fight.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-hit\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Hit</h3>\n"
     << "\t\t\t\t<p>Average non-crit damage.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-interval\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Interval</h3>\n"
     << "\t\t\t\t<p>Average time between executions of a particular action.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-avg\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Avg</h3>\n"
     << "\t\t\t\t<p>Average direct damage per execution.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-miss-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>M%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in misses, dodges or parries.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-origin\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Origin</h3>\n"
     << "\t\t\t\t<p>The player profile from which the simulation script was generated. The profile must be copied into the same directory as this HTML file in order for the link to work.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-parry-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Parry%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in parries.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-range\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Range</h3>\n"
     << "\t\t\t\t<p>( dps.percentile( 0.95 ) - dps.percentile( 0.05 ) / 2</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-rps-in\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>RPS In</h3>\n"
     << "\t\t\t\t<p>Average primary resource points generated per second.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-rps-out\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>RPS Out</h3>\n"
     << "\t\t\t\t<p>Average primary resource points consumed per second.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-scale-factors\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Scale Factors</h3>\n"
     << "\t\t\t\t<p>Gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>Loss</b> per unit stat <b>decrease</b>.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Ticks</h3>\n"
     << "\t\t\t\t<p>Average number of periodic ticks per iteration. Spells that do not have a damage-over-time component will have zero ticks.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks-crit\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>T-Crit</h3>\n"
     << "\t\t\t\t<p>Average crit tick damage.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks-crit-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>T-Crit%</h3>\n"
     << "\t\t\t\t<p>Percentage of ticks that resulted in critical strikes.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks-hit\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>T-Hit</h3>\n"
     << "\t\t\t\t<p>Average non-crit tick damage.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks-miss-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>T-M%</h3>\n"
     << "\t\t\t\t<p>Percentage of ticks that resulted in misses, dodges or parries.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks-uptime\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>UpTime%</h3>\n"
     << "\t\t\t\t<p>Percentage of total time that DoT is ticking on target.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks-avg\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>T-Avg</h3>\n"
     << "\t\t\t\t<p>Average damage per tick.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-timeline-distribution\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Timeline Distribution</h3>\n"
     << "\t\t\t\t<p>The simulated encounter's duration can vary based on the health of the target and variation in the raid DPS. This chart shows how often the duration of the encounter varied by how much time.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-fight-length\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Fight Length</h3>\n"
     << "\t\t\t\t<p>Fight Length: " << sim -> max_time.total_seconds() << "<br />\n"
     << "\t\t\t\tVary Combat Length: " << sim -> vary_combat_length << "</p>\n"
     << "\t\t\t\t<p>Fight Length is the specified average fight duration. If vary_combat_length is set, the fight length will vary by +/- that portion of the value. See <a href=\"http://code.google.com/p/simulationcraft/wiki/Options#Combat_Length\" class=\"ext\">Combat Length</a> in the wiki for further details.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-waiting\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Waiting</h3>\n"
     << "\t\t\t\t<p>This is the percentage of time in which no action can be taken other than autoattacks. This can be caused by resource starvation, lockouts, and timers.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-sf-ranking\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Scale Factor Ranking</h3>\n"
     << "\t\t\t\t<p>This row ranks the scale factors from highest to lowest, checking whether one scale factor is higher/lower than another with statistical significance.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<!-- End Help Boxes -->\n";
}

// print_html_styles ====================================================

void print_html_styles( report::sc_html_stream& os, sim_t* sim )
{
  // Styles
  // If file is being hosted on simulationcraft.org, link to the local
  // stylesheet; otherwise, embed the styles.
  if ( sim -> hosted_html )
  {
    os << "\t\t<style type=\"text/css\" media=\"screen\">\n"
       << "\t\t\t@import url('http://www.simulationcraft.org/css/styles.css');\n"
       << "\t\t</style>\n"
       << "\t\t<style type=\"text/css\" media=\"print\">\n"
       << "\t\t	@import url('http://www.simulationcraft.org/css/styles-print.css');\n"
       << "\t\t</style>\n";
  }
  else if ( sim -> print_styles == 1 )
  {
    os << "\t\t<style type=\"text/css\" media=\"all\">\n"
       << "\t\t\t* {border: none;margin: 0;padding: 0; }\n"
       << "\t\t\tbody {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background-color: #f9f9f9;color: #333;text-align: center; }\n"
       << "\t\t\tp {margin: 1em 0 1em 0; }\n"
       << "\t\t\th1, h2, h3, h4, h5, h6 {width: auto;color: #777;margin-top: 1em;margin-bottom: 0.5em; }\n"
       << "\t\t\th1, h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
       << "\t\t\th1 {font-size: 24px; }\n"
       << "\t\t\th2 {font-size: 18px; }\n"
       << "\t\t\th3 {margin: 0 0 4px 0;font-size: 16px; }\n"
       << "\t\t\th4 {font-size: 12px; }\n"
       << "\t\t\th5 {font-size: 10px; }\n"
       << "\t\t\ta {color: #666688;text-decoration: none; }\n"
       << "\t\t\ta:hover, a:active {color: #333; }\n"
       << "\t\t\tul, ol {padding-left: 20px; }\n"
       << "\t\t\tul.float, ol.float {padding: 0;margin: 0; }\n"
       << "\t\t\tul.float li, ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #eee; }\n"
       << "\t\t\t.clear {clear: both; }\n"
       << "\t\t\t.hide, .charts span {display: none; }\n"
       << "\t\t\t.center {text-align: center; }\n"
       << "\t\t\t.float {float: left; }\n"
       << "\t\t\t.mt {margin-top: 20px; }\n"
       << "\t\t\t.mb {margin-bottom: 20px; }\n"
       << "\t\t\t.force-wrap {word-wrap: break-word; }\n"
       << "\t\t\t.mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
       << "\t\t\t.toggle {cursor: pointer; }\n"
       << "\t\t\th2.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAeCAIAAACT/LgdAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD1SURBVHja7JQ9CoQwFIT9LURQG3vBwyh4XsUjWFtb2IqNCmIhkp1dd9dsfIkeYKdKHl+G5CUTvaqqrutM09Tk2rYtiiIrjuOmaeZ5VqBBEADVGWPTNJVlOQwDyYVhmKap4zgGJp7nJUmCpQoOY2Mv+b6PkkDz3IGevQUOeu6VdxrHsSgK27azLOM5AoVwPqCu6wp1ApXJ0G7rjx5oXdd4YrfQtm3xFJdluUYRBFypghb32ve9jCaOJaPpDpC0tFmg8zzn46nq6/rSd2opAo38IHMXrmeOdgWHACKVFx3Y/c7cjys+JkSP9HuLfYR/Dg1icj0EGACcXZ/44V8+SgAAAABJRU5ErkJggg==) 0 -10px no-repeat; }\n"
       << "\t\t\th2.open {margin-bottom: 10px;background-position: 0 9px; }\n"
       << "\t\t\th3.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAaCAIAAAAMmCo2AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAEfSURBVHjazJPLjkRAGIXbdSM8ACISvWeDNRYeGuteuL2EdMSGWLrOmdExaCO9nLOq+vPV+S9VRTwej6IoGIYhCOK21zzPfd/f73da07TiRxRFbTkQ4zjKsqyqKoFN27ZhGD6fT5ZlV2IYBkVRXNflOI5ESBAEz/NEUYT5lnAcBwQi307L6aZpoiiqqgprSZJwbCF2EFTXdRAENE37vr8SR2jhAPE8vw0eoVORtw/0j6Fpmi7afEFlWeZ5jhu9grqui+M4SZIrCO8Eg86y7JT7LXx5TODSNL3qDhw6eOeOIyBJEuUj6ZY7mRNmAUvQa4Q+EEiHJizLMgzj3AkeMLBte0vsoCULPHRd//NaUK9pmu/EywDCv0M7+CTzmb4EGADS4Lwj+N6gZgAAAABJRU5ErkJggg==) 0 -11px no-repeat; }\n"
       << "\t\t\th3.open {background-position: 0 7px; }\n"
       << "\t\t\th4.toggle {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAVCAIAAADw0OikAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD8SURBVHjavJHLjkRAGIUbRaxd3oAQ8QouifDSFmysPICNIBZ2EhuJuM6ZMdFR3T3LOYtKqk79/3/qKybLsrZteZ5/3DXPs67rxLbtvu+bprluHMexrqumaZZlMdhM05SmaVVVhBBst20zDMN1XRR822erJEnKsmQYxjRNz/M4jsM5ORsKguD7/r7vqHAc5/Sg3+orDsuyGHGd3OxXsY8/9R92XdfjOH60i6IAODzsvQ0sgApw1I0nAZACVGAAPlEU6WigDaLoEcfxleNN8mEY8Id0c2hZFlmWgyDASlefXhiGqqrS0eApihJFkSRJt0nHj/I877rueNGXAAMAKcaTc/aCM/4AAAAASUVORK5CYII=) 0 -8px no-repeat; }\n"
       << "\t\t\th4.open {background-position: 0 6px; }\n"
       << "\t\t\ta.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAkAAAAXCAYAAADZTWX7AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAADiSURBVHjaYvz//z/DrFmzGBkYGLqBeG5aWtp1BjTACFIEAkCFZ4AUNxC7ARU+RlbEhMT+BMQaQLwOqEESlyIYMIEqlMenCAQsgLiakKILQNwF47AgSfyH0leA2B/o+EfYTOID4gdA7IusAK4IGk7ngNgPqOABut3I1uUDFfzA5kB4YOIDTAxEgOGtiAUY2vlA2hCIf2KRZwXie6AQPwzEFUAsgUURSGMQEzAqQHFmB8R30BS8BWJXoPw2sJuAjNug2Afi+1AFH4A4DCh+GMXhQIEboHQExKeAOAbI3weTAwgwAIZTQ9CyDvuYAAAAAElFTkSuQmCC) 0 4px no-repeat; }\n"
       << "\t\t\ta.open {background-position: 0 -11px; }\n"
       << "\t\t\ttd.small a.toggle-details {background-position: 0 2px; }\n"
       << "\t\t\ttd.small a.open {background-position: 0 -13px; }\n"
       << "\t\t\t#active-help, .help-box {display: none; }\n"
       << "\t\t\t#active-help {position: absolute;width: 350px;padding: 3px;background: #fff;z-index: 10; }\n"
       << "\t\t\t#active-help-dynamic {padding: 6px 6px 18px 6px;background: #eeeef5;outline: 1px solid #ddd;font-size: 13px; }\n"
       << "\t\t\t#active-help .close {position: absolute;right: 10px;bottom: 4px; }\n"
       << "\t\t\t.help-box h3 {margin: 0 0 5px 0;font-size: 16px; }\n"
       << "\t\t\t.help-box {border: 1px solid #ccc;background-color: #fff;padding: 10px; }\n"
       << "\t\t\ta.help {cursor: help; }\n"
       << "\t\t\t.section {position: relative;width: 1150px;padding: 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;border: 1px solid #ccc;background-color: #fff;-moz-box-shadow: 4px 4px 4px #bbb;-webkit-box-shadow: 4px 4px 4px #bbb;box-shadow: 4px 4px 4px #bbb;text-align: left; }\n"
       << "\t\t\t.section-open {margin-top: 25px;margin-bottom: 35px;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px; }\n"
       << "\t\t\t.grouped-first {-moz-border-radius-topright: 15px;-moz-border-radius-topleft: 15px;-khtml-border-top-right-radius: 15px;-khtml-border-top-left-radius: 15px;-webkit-border-top-right-radius: 15px;-webkit-border-top-left-radius: 15px;border-top-right-radius: 15px;border-top-left-radius: 15px; }\n"
       << "\t\t\t.grouped-last {-moz-border-radius-bottomright: 15px;-moz-border-radius-bottomleft: 15px;-khtml-border-bottom-right-radius: 15px;-khtml-border-bottom-left-radius: 15px;-webkit-border-bottom-right-radius: 15px;-webkit-border-bottom-left-radius: 15px;border-bottom-right-radius: 15px;border-bottom-left-radius: 15px; }\n"
       << "\t\t\t.section .toggle-content {padding: 0; }\n"
       << "\t\t\t.player-section .toggle-content {padding-left: 16px;margin-bottom: 20px; }\n"
       << "\t\t\t.subsection {background-color: #ccc;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
       << "\t\t\t.subsection-small {width: 500px; }\n"
       << "\t\t\t.subsection h4 {margin: 0 0 10px 0; }\n"
       << "\t\t\t.profile .subsection p {margin: 0; }\n"
       << "\t\t\t#raid-summary .toggle-content {padding-bottom: 0px; }\n"
       << "\t\t\tul.params {padding: 0;margin: 4px 0 0 6px; }\n"
       << "\t\t\tul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #eeeef5;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px; }\n"
       << "\t\t\t#masthead ul.params {margin-left: 4px; }\n"
       << "\t\t\t#masthead ul.params li {margin-left: 0px;margin-right: 10px; }\n"
       << "\t\t\t.player h2 {margin: 0; }\n"
       << "\t\t\t.player ul.params {position: relative;top: 2px; }\n"
       << "\t\t\t#masthead h2 {margin: 10px 0 5px 0; }\n"
       << "\t\t\t#notice {border: 1px solid #ddbbbb;background: #ffdddd;font-size: 12px; }\n"
       << "\t\t\t#notice h2 {margin-bottom: 10px; }\n"
       << "\t\t\t.alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #ddd;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
       << "\t\t\t.alert p {margin-bottom: 0px; }\n"
       << "\t\t\t.section .toggle-content {padding-left: 18px; }\n"
       << "\t\t\t.player > .toggle-content {padding-left: 0; }\n"
       << "\t\t\t.toc {float: left;padding: 0; }\n"
       << "\t\t\t.toc-wide {width: 560px; }\n"
       << "\t\t\t.toc-narrow {width: 375px; }\n"
       << "\t\t\t.toc li {margin-bottom: 10px;list-style-type: none; }\n"
       << "\t\t\t.toc li ul {padding-left: 10px; }\n"
       << "\t\t\t.toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
       << "\t\t\t.charts {float: left;width: 541px;margin-top: 10px; }\n"
       << "\t\t\t.charts-left {margin-right: 40px; }\n"
       << "\t\t\t.charts img {padding: 8px;margin: 0 auto;margin-bottom: 20px;border: 1px solid #ccc;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 1px 1px 4px #ccc;-webkit-box-shadow: inset 1px 1px 4px #ccc;box-shadow: inset 1px 1px 4px #ccc; }\n"
       << "\t\t\t.talents div.float {width: auto;margin-right: 50px; }\n"
       << "\t\t\ttable.sc {border: 0;background-color: #eee; }\n"
       << "\t\t\ttable.sc tr {background-color: #fff; }\n"
       << "\t\t\ttable.sc tr.head {background-color: #aaa;color: #fff; }\n"
       << "\t\t\ttable.sc tr.odd {background-color: #f3f3f3; }\n"
       << "\t\t\ttable.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #aaa;color: #fcfcfc; }\n"
       << "\t\t\ttable.sc th.small {padding: 2px 2px;font-size: 12px; }\n"
       << "\t\t\ttable.sc th a {color: #fff;text-decoration: underline; }\n"
       << "\t\t\ttable.sc th a:hover, table.sc th a:active {color: #f1f1ff; }\n"
       << "\t\t\ttable.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
       << "\t\t\ttable.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; }\n"
       << "\t\t\ttable.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; }\n"
       << "\t\t\ttable.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
       << "\t\t\ttable.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
       << "\t\t\ttable.sc tr.details td {padding: 0 0 15px 15px;text-align: left;background-color: #fff;font-size: 11px; }\n"
       << "\t\t\ttable.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
       << "\t\t\ttable.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #f3f3f3; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
       << "\t\t\ttable.sc tr.details td div.float {width: 350px; }\n"
       << "\t\t\ttable.sc tr.details td div.float h5 {margin-top: 4px; }\n"
       << "\t\t\ttable.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
       << "\t\t\ttable.sc td.filler {background-color: #ccc; }\n"
       << "\t\t\ttable.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
       << "\t\t</style>\n";
  }
  else if ( sim -> print_styles == 2 )
  {
    os << "\t\t<style type=\"text/css\" media=\"all\">\n"
       << "\t\t\t* {border: none;margin: 0;padding: 0; }\n"
       << "\t\t\tbody {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background: #261307;color: #FFF;text-align: center; }\n"
       << "\t\t\tp {margin: 1em 0 1em 0; }\n"
       << "\t\t\th1, h2, h3, h4, h5, h6 {width: auto;color: #FDD017;margin-top: 1em;margin-bottom: 0.5em; }\n"
       << "\t\t\th1, h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
       << "\t\t\th1 {font-size: 28px;text-shadow: 0 0 3px #FDD017; }\n"
       << "\t\t\th2 {font-size: 18px; }\n"
       << "\t\t\th3 {margin: 0 0 4px 0;font-size: 16px; }\n"
       << "\t\t\th4 {font-size: 12px; }\n"
       << "\t\t\th5 {font-size: 10px; }\n"
       << "\t\t\ta {color: #FDD017;text-decoration: none; }\n"
       << "\t\t\ta:hover, a:active {text-shadow: 0 0 1px #FDD017; }\n"
       << "\t\t\tul, ol {padding-left: 20px; }\n"
       << "\t\t\tul.float, ol.float {padding: 0;margin: 0; }\n"
       << "\t\t\tul.float li, ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #333; }\n"
       << "\t\t\t.clear {clear: both; }\n"
       << "\t\t\t.hide, .charts span {display: none; }\n"
       << "\t\t\t.center {text-align: center; }\n"
       << "\t\t\t.float {float: left; }\n"
       << "\t\t\t.mt {margin-top: 20px; }\n"
       << "\t\t\t.mb {margin-bottom: 20px; }\n"
       << "\t\t\t.force-wrap {word-wrap: break-word; }\n"
       << "\t\t\t.mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
       << "\t\t\t.toggle {cursor: pointer; }\n"
       << "\t\t\th2.toggle {padding-left: 18px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAeCAIAAACT/LgdAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAFaSURBVHjaYoz24a9N51aVZ2PADT5//VPS+5WRk51RVZ55STu/tjILVnV//jLEVn1cv/cHMzsb45OX/+48/muizSoiyISm7vvP/yn1n1bs+AE0kYGbkxEiaqDOcn+HyN8L4nD09aRYhCcHRBakDK4UCKwNWM+sEIao+34aoQ6LUiCwMWR9sEMETR12pUBgqs0a5MKOJohdKVYAVMbEQDQYVUq6UhlxZmACIBwNQNJCj/XVQVFjLVbCsfXrN4MwP9O6fn4jTVai3Ap0xtp+fhMcZqN7S06CeU0fPzBxERUCshLM6ycKmOmwEhVYkiJMa/oE0HyJM1zffvj38u0/wkq3H/kZU/nxycu/yIJY8v65678LOj8DszsBt+4+/iuo8COmOnSlh87+Ku///PjFXwIRe2qZkKggE56IZebnZfn56x8nO9P5m/+u3vkNLHBYWdARExMjNxczQIABACK8cxwggQ+oAAAAAElFTkSuQmCC) 0 -10px no-repeat; }\n"
       << "\t\t\th2.toggle:hover {text-shadow: 0 0 2px #FDD017; }\n"
       << "\t\t\th2.open {margin-bottom: 10px;background-position: 0 9px; }\n"
       << "\t\t\t#home-toc h2.open {margin-top: 20px; }\n"
       << "\t\t\th3.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAaCAYAAACD+r1hAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD/SURBVHjaYvx7QdyTgYGhE4iVgfg3A3bACsRvgDic8f///wz/Lkq4ADkrgVgIh4bvIMVM+i82M4F4QMYeIBUAxE+wKP4IxCEgxWC1MFGgwGEglQnEj5EUfwbiaKDcNpgA2EnIAOg8VyC1Cog5gDgMZjJODVBNID9xABVvQZdjweHJO9CQwQBYbcAHmBhIBMNBAwta+MtgSx7A+MBpgw6pTloKxBGkaOAB4vlAHEyshu/QRLcQlyZ0DYxQmhuIFwNxICnBygnEy4DYg5R4AOW2D8RqACXxMCA+QYyG20CcAcSHCGUgTmhxEgPEp4gJpetQZ5wiNh7KgXg/vlAACDAAkUxCv8kbXs4AAAAASUVORK5CYII=) 0 -11px no-repeat; }\n"
       << "\t\t\th3.toggle:hover {text-shadow: 0 0 2px #CDB007; }\n"
       << "\t\t\th3.open {background-position: 0 7px; }\n"
       << "\t\t\th4.toggle {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAVCAIAAADw0OikAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD8SURBVHjavJHLjkRAGIUbRaxd3oAQ8QouifDSFmysPICNIBZ2EhuJuM6ZMdFR3T3LOYtKqk79/3/qKybLsrZteZ5/3DXPs67rxLbtvu+bprluHMexrqumaZZlMdhM05SmaVVVhBBst20zDMN1XRR822erJEnKsmQYxjRNz/M4jsM5ORsKguD7/r7vqHAc5/Sg3+orDsuyGHGd3OxXsY8/9R92XdfjOH60i6IAODzsvQ0sgApw1I0nAZACVGAAPlEU6WigDaLoEcfxleNN8mEY8Id0c2hZFlmWgyDASlefXhiGqqrS0eApihJFkSRJt0nHj/I877rueNGXAAMAKcaTc/aCM/4AAAAASUVORK5CYII=) 0 -8px no-repeat; }\n"
       << "\t\t\th4.open {background-position: 0 6px; }\n"
       << "\t\t\ta.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAWCAYAAAD5Jg1dAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAADpSURBVHjaYvx7QdyLgYGhH4ilgfgPAypgAuIvQBzD+P//f4Z/FyXCgJzZQMyHpvAvEMcx6b9YBlYIAkDFAUBqKRBzQRX9AuJEkCIwD6QQhoHOCADiX0D8F4hjkeXgJsIA0OQYIMUGNGkesjgLAyY4AsTM6IIYJuICTAxEggFUyIIULIpA6jkQ/0AxSf8FhoneQKxJjNVxQLwFiGUJKfwOxFJAvBmakgh6Rh+INwCxBDG+NoEq1iEmeK4A8Rt8iQIEpgJxPjThYpjIhKSoFFkRukJQQK8D4gpoCDDgSo+Tgfg0NDNhAIAAAwD5YVPrQE/ZlwAAAABJRU5ErkJggg==) 0 -9px no-repeat; }\n"
       << "\t\t\ta.open {background-position: 0 6px; }\n"
       << "\t\t\ttd.small a.toggle-details {background-position: 0 -10px; }\n"
       << "\t\t\ttd.small a.open {background-position: 0 5px; }\n"
       << "\t\t\t#active-help, .help-box {display: none;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px; }\n"
       << "\t\t\t#active-help {position: absolute;width: auto;padding: 3px;background: transparent;z-index: 10; }\n"
       << "\t\t\t#active-help-dynamic {max-width: 400px;padding: 8px 8px 20px 8px;background: #333;font-size: 13px;text-align: left;border: 1px solid #222;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: 4px 4px 10px #000;-webkit-box-shadow: 4px 4px 10px #000;box-shadow: 4px 4px 10px #000; }\n"
       << "\t\t\t#active-help .close {display: block;height: 14px;width: 14px;position: absolute;right: 12px;bottom: 7px;background: #000 url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAYAAAAfSC3RAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAE8SURBVHjafNI/KEVhGMfxc4/j33BZjK4MbkmxnEFiQFcZlMEgZTAZDbIYLEaRUMpCuaU7yCCrJINsJFkUNolSBnKJ71O/V69zb576LOe8v/M+73ueVBzH38HfesQ5bhGiFR2o9xdFidAm1nCFop7VoAvTGHILQy9kCw+0W9F7/o4jHPs7uOAyZrCL0aC05rCgd/uu1Rus4g6VKKAa2wrNKziCPTyhx4InClkt4RNbardFoWG3E3WKCwteJ9pawSt28IEcDr33b7gPy9ysVRZf2rWpzPso0j/yax2T6EazzlynTgL9z2ykBe24xAYm0I8zqdJF2cUtog9tFsxgFs8YR68uwFVeLec1DDYEaXe+MZ1pIBFyZe3WarJKRq5CV59Wiy9IoQGDmPpvVq3/Tg34gz5mR2nUUPzWjwADAFypQitBus+8AAAAAElFTkSuQmCC) no-repeat; }\n"
       << "\t\t\t#active-help .close:hover {background-color: #1d1d1d; }\n"
       << "\t\t\t.help-box h3 {margin: 0 0 12px 0;font-size: 14px;color: #C68E17; }\n"
       << "\t\t\t.help-box p {margin: 0 0 10px 0; }\n"
       << "\t\t\t.help-box {background-color: #000;padding: 10px; }\n"
       << "\t\t\ta.help {color: #C68E17;cursor: help; }\n"
       << "\t\t\ta.help:hover {text-shadow: 0 0 1px #C68E17; }\n"
       << "\t\t\t.section {position: relative;width: 1150px;padding: 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;border: 0;-moz-box-shadow: 0px 0px 8px #FDD017;-webkit-box-shadow: 0px 0px 8px #FDD017;box-shadow: 0px 0px 8px #FDD017;color: #fff;background-color: #000;text-align: left; }\n"
       << "\t\t\t.section-open {margin-top: 25px;margin-bottom: 35px;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px; }\n"
       << "\t\t\t.grouped-first {-moz-border-radius-topright: 15px;-moz-border-radius-topleft: 15px;-khtml-border-top-right-radius: 15px;-khtml-border-top-left-radius: 15px;-webkit-border-top-right-radius: 15px;-webkit-border-top-left-radius: 15px;border-top-right-radius: 15px;border-top-left-radius: 15px; }\n"
       << "\t\t\t.grouped-last {-moz-border-radius-bottomright: 15px;-moz-border-radius-bottomleft: 15px;-khtml-border-bottom-right-radius: 15px;-khtml-border-bottom-left-radius: 15px;-webkit-border-bottom-right-radius: 15px;-webkit-border-bottom-left-radius: 15px;border-bottom-right-radius: 15px;border-bottom-left-radius: 15px; }\n"
       << "\t\t\t.section .toggle-content {padding: 0; }\n"
       << "\t\t\t.player-section .toggle-content {padding-left: 16px; }\n"
       << "\t\t\t#home-toc .toggle-content {margin-bottom: 20px; }\n"
       << "\t\t\t.subsection {background-color: #333;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
       << "\t\t\t.subsection-small {width: 500px; }\n"
       << "\t\t\t.subsection h4 {margin: 0 0 10px 0;color: #fff; }\n"
       << "\t\t\t.profile .subsection p {margin: 0; }\n"
       << "\t\t\t#raid-summary .toggle-content {padding-bottom: 0px; }\n"
       << "\t\t\tul.params {padding: 0;margin: 4px 0 0 6px; }\n"
       << "\t\t\tul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #2f2f2f;color: #ddd;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px; }\n"
       << "\t\t\tul.params li.linked:hover {background: #393939; }\n"
       << "\t\t\tul.params li a {color: #ddd; }\n"
       << "\t\t\tul.params li a:hover {text-shadow: none; }\n"
       << "\t\t\t.player h2 {margin: 0; }\n"
       << "\t\t\t.player ul.params {position: relative;top: 2px; }\n"
       << "\t\t\t#masthead {height: auto;padding-bottom: 30px;border: 0;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px;-moz-box-shadow: 0px 0px 8px #FDD017;-webkit-box-shadow: 0px 0px 8px #FDD017;box-shadow: 0px 0px 8px #FDD017;text-align: left;color: #FDD017;background: #000 url(data:image/jpeg;base64,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"
       << "gWK0KkUKOMPYcgtPBCvmS6hZoSBjNzyKj4gdHvBUqmWmOMsOUwoCMEkVQOj9OrVxrXrnqW6XZ7npSiOwiVUEM/rZUdpZRrW51hXMpSIUEqoclgt+crBXbvmf7yV84/8cCeyfGJeh1NzE7+0q9/+tz5tcZma++u6uJK7+d+8v6ZXc5P/+Ir02P26ddXG/3bcw9J8o/B52MHR559x8F/88cv/PYHLzz9yNxEPbd7dte9d+0+c379ox99lafgvU/Pfe1vzzz39ZthwputmDGlAYrOrUulIcADP6gUHQAgQtCxSLMdlEvu+ga6udaulrOSizsOz/QHya2tZM+sywNTJendd47eMV/904VVLNt5ZpjU1oKvt1IlANAKITJWzoo0OnM96vvBMErXmkE1yybKRneYtPtKAmBQFKWAUmkaRstTfqpyFsEQGAZEUBuWtdKOLYoylpHNma1hLBO9eya3vhXfe7gquMxYzLVQnAQaKK/Lt4dMJBAqDhAcBCLrEKm1H0rbhBZBrsm6QZw3kGVqEyOgdTlHillkYbNkcteFMuUVFwKI4gggoJilkQ0g1Uhr1ZcwBkEzzY44UZpiCYTXggIoAnrpCjWcgYp6naHldlzX1hoiYOVro6P1ySSG1y8vS+JkEldLEfHUtoaMOHZe3vfAiGUZf/7hjb/55LVTx6evLbYh1J1OsNXxHYMN/ShOhEEpo4gQ0u0OpRJf/tqNME63t7yHTw7e/vTB3/zd515+cfGdbzt8cP7BLzx3xTSMWiX/a7/zilTie9//YKf/ucbrK7dlqOPbXubpyXqexof3z3zlhSvXV7tz0+XuIFpYbL12ZukD/+S+n/zxh3/xlz/JDJSE/OXX1/bMFr7rnQeeeHjuziNj1Wp+ZbOfyeLL17ff+vBejPVI1YEAbLd8y8BXbjWGw6hecZMkfeTkHEA2QcgPwWpTDDx/tFA8f6npeb0Th0b7sdkfDrCRubHSsTOFVOgoJmudaKPDL6+lgwhWXVKwGIB6vZn0Q40I4EoqjQ0Txxx2Q42RMhgykGYYKQikZqutMGuTnINrpUy7H/X68eS4e/mmP1pmWqvtdiIBGsnTVOp6hgBEKzmTUlXL27ksZhSOVWgpS0wMc46xe9QuFe2MgaplE2syVrOkUDmHjBepgXgpx3KG3OijKOLFjMkVIEg7BQBtBSlCIfDXUqjMbI6KGGiIkjQFAkYtCbUiLhIqFL0w2kCpp8O+ivtSpjzudzsrmzxt1UZlrYwsA2uIhwMeR7JxfW18puC6rFBis1PZL31l+cpCo1Z2wpDXR9y90yWIdCFn3Vhumgau1/Jvf2z+Pe888NgD+/bNla7daHeHaSlvDvz47LnVf/9H398dhH/xly+3OmGxYB3YV/2NP3yhlGWPPzB/c7Fx4ODUlYXNHZ+/zRgfq4zY4MH7942MZD/zxYv5rNnpxmEsjx4c+dVfeve1K2s/9XMfDSK5udU5dffcQ/ePX7zZfeLxfc+/svLc6fXV1nBubuS+E/uK+XIQQtvMxAlYuNnUCvIUlHK5kUoRQAMDZtlGrVYlGBMChsMAAvP6+tDTYGqmhnEu4ohQ2mhHtpVVSpl2Ya0VrG33VxtB0UQVF3R8nmrUHsTfmDjxIwEBBhB1QxVyaGAllXYwwhgLBblGm/24XjILDslmzOVmgKTaNZu5ueTvnnaBBEKCUIA9Y07biyng2LTGS7lUci2RBgoiGHOZcxiGQGqAIDZtbGCCKRz4SdalOZfINHUYsKkCWisAuj6oV9lqIzYorpcJNoCd1RpDoslgNc3k3bgfRx6MwhgoYJgESwhSkMYKUI6ISLYgCmXc10ko45YK24IPIUxl0I46LT/s95Ds5XPRSBVZDCtgK2lk8iwOVSEni7nK2ko6HKY84Z12mskUck4eaOP4gd1veXCfUuLKje2Xz24xCu47OfeJz1/++X96yrCsj37i9V4//uKXLz7zloPPPnOs2Rourw210o1mP+OYc7tr1ZK5dGvbyeW2Gr0dn79tKBediosefvjA//QTT3zg+//wkQfmMYIXr6wXKiPvfubwL/zaJ//2s1cyFnvb4/MPPHT3p770arOb2I59bqHVDpIYww5XS23/4kpro9NvDrxb252tzlAzrJQSSkRJHAs56HmpjZlpTo2Oug4SQvoBbHTCIPaLNn3j2kYU2w/fW+8GxvrWMA0G88fu/dQXXlQ8rWZJHEeu62Jq+nEaxxxT3PUgRkBDrCAaJtAxdBonBYtgDEyTJAIBSNZawb7JLCVIajiI0jgQu6edMxd6e3flEZAAgEY/Hi2ZwyDsDsWu8Vyrl45UrG4vZQxgiKWScSIyNiYYtr3ENAgAyjIYAmB1OzoyY2MIXAMQCISEhkW2WwkzLS1jqViGJKUiMauQc001GrR5seRsroS5oqOFdIo2UKnpGBbqFMMAACAASURBVDIEjFFKYRorjaCNSdBRGAMggJIg8HS/J/2BlJGGiXJK1nAbrF/hbOSAhQfbG7FhmUpgqFTs6d17nPNXWssrPUiJwnqrM1heb240usvrzdfeWF5cbg36sc1gpx2ePrcxW8+ev9aIOL//5PGnH9396mu3PvSRc0Ew+M53Pfnhj728vdXK55wH7tn9ob96+cd+8PEkjm8tNhHBQy/e8fnbYaaqWnzyvl3UYH/2x9/z1Ft/vVYvPXJy9+efXzg4Xx8GwZkLvV4/uGN/9ZknD/7Wn75YLzLIskMO18PhwKArEg5dW9aKEdOJY/ISBuPMi0HASGLgKGvx2VLiWqGNedH2EIhy7I6ZXWWmW0PZ6PkLq23XYt1BuLg+fPSeyhs344W1frVgHL3zxFe/elrHnoHlrfXO/N7db3v7qVdOXwxjniRQIiNjEj8WcSo0wkUXcw6KGaq0poQmHHpcbXfCw7MZBSBjJEmFTMXumcz1W/78roLSnCe46yeMmDlLD2NdyeCuL8bK+cW1vu1ADImUSiodc51xCCGo1eeuxYSQOZdCoDa2/flpV0pgMe35UkNkUhxxZZsQCLx7FHlBWqpAw0QihGFPAAlX1nixYK0ue3GsfF9MzOf8ZkwIUqmGBLEMDruccuh3VL+vCKIaqDQBSsEoVL6v/L4OmyliwiHCH1K7Ml+bMIUAGEpM8HB94NjG81d7l7YCD4DAZKHSIcYRQsNY+hpGAHkSbnp8YxDJOO0N/IJbqhULw27ry1+/8Y4nDpQL5vnLw/WNAY/7hsEWllpPPrRvba2/tLL9wz/5+M1r6w6zhlEahMmOz29qpsbLD9w5dfbS5qc+/OzP/Mznzi80P/Cdxz/88bPvfsfRMBL9ftRsdh+8ZzqXs/70I2ff8847X7i4ebXZW03jMJvRtawz7mSmq8jguApZEVnjFskZRoVByIy6SwsGdSjikhVtkjGNEScRctjv7Rmtr2z1fd9b3ujNTdd6w7BWzmYdutbgGxsbRw7MNXs+Yei+Y1OY2dPTEw8+dM/2+vZzX700PVv9X//F+z71mVcNk0YKFjIMQ4AodU1EKQxTTQjuhTqOkkO78hogKeAwTHicjo46b1z198w4SgktdAq4VGxuFPUCAYQs5swghLvG7Ncu90oFUyOgFFRAJ6nI2JRg2BykuQwN+uloxeYaDHpJPktEqhglQawoBoQgi+oo4rOjFtXR2LgV+rESBAEURdAfKNdh65vJwWPFxJdZV68tBoWqlQwEYhQjncRaaK18RTEOfE0w8AKICYpjnaQphLg7FBLa++66Fzl49fS6VaxJQIFGW4sN0zS8rcDk4GIjXBYAMczyJivb1CQEIZIxCUVIK2JRoqHWsB+rhieX15obq1saoENztc997fpD980Mel6z3fcjMT6a/d5nj//Zf3zthz5w94c/dtGW0SMP7nrl7Podh2dbHe+/VNHv+PymY/d05X3vOHlhofXBX3v8X//h2dOvrz/7tsOvvb56/Mh4MWc+/8pNz0/HRnOOY758frM6UvjM6ZUhgrCecaZdd68NHS1NzUmi8hBlGMiZmkFJiIIYZSlyKbYYpBCVLWhR5BCAkU74JhGb3DtRy3gDuN4YFnImNUzLoFwRP0rCJB0brQoJHIsuNfkwRuMTY2k0fGNh+/iJ+Xc98+AXv3aT6UAh6+Rd+yC10zjNWAZl7NDRY2EQxwDKON497hiWEyScpxxqVSw4XU9M1o0wlAhinophhKfKeOCJRMisQ7kGOdsME7HpqTCMMzZBUCMIIi4yNqYYdPq8kmXb/XSqbnMu01R4gTIZNhlVSkWpwBCUCgZSaTWLXBdLkXa7AEplYDwMgB+DJJbZLL65GBSrBkTIsklzkwcRjAIBIA39GEDlUKPbFEJZCFOi0kBm7nz8VGtti5p2rZ5fW2pPzB6wchW3hvmADLf6pm0zJZtrXakQisVZBFYZwjULFww64qCcgfImcgnNWyhjQAShSZCJMSOMQsiMQazbrWGj5U2N5c+8sbVrunTmjdVizlzb6J44Nnlgb/njn7n2+IO7PvPczYNzhenZ0tVr3fvunFtcbX9b7+h62/pMmXFkfubCpcVf/pff8dxXb51+bem73n7kU1+89vj9M8eOFH/jD15mhGgAVjb6YQpDoZb81BhxzTGXlQx3nwtyTAgMLIIdAiRSUiOGVSwww9qhSGmcpdrAkECcM5CJEcVaKZw1WM0amLBvg115p92I8rZRyDmr231KSBSLlMeWaUOElle2opCP1goZxwq5ymTckdFqp+t3h7xaHz158o5+AMNek0AepvLt3/H4SLXyN58/+1P/9H3Xzp2b2r13Znb6yrUblTxBiGy1g0LOeOKJhzfWW8fvOfLymev7p8ucxxABJUTWMbjExaxxaWkQAac/TF1DmwxDBJNUWCbCGPV9XimYm81o15gTpwpItdqJRvImxdpiKAiVidRIkUyV8HAYOQZLUq0VaA2VEBhh6AmgeUopKeVtEcfbjcR0WKVmMYpNhkq7Jyv7dlMdeM2g39VHHn1o8dqKSERhpLLvvgf9rpek6tjxQ+XpmVQgC1u9Ac5Vja2VLhWwtT5obPS81rAN0deZOcybyCI4Z6AcwzmDOATlDGhj5BBadaBJoE2hAkooCIBVy6WR8BM56EelnNPqxv1BUCrYbsZ4/sWb3/fug/mM9fVXVx89OfuRT1y+/9RRt2Rcvrg6OlK/ubyx4/Objol6lcfife+/u1xiv/dvvjw3U/aD5KlH5o4edP/lb7zCuTpyoN7uB0f2Tba9ZCsW5qRrTWahQexpS1GcxgA6BEIAAAJQQ6h1qgDCSmoIAAAaaAAQhAxDDL9x8SBBtG6RHCUV2snqTSpMA/u+yiHq+3GayjCJAz/MZdzeMGx1h+ViVik9Xq/0e0PG8Mp6VygpNdq/f67R6COkJydGnFzh0KG9ruOmUjxw32GRikuXl7/nB5+9trCiwk4QKy9SFMq3PfN4dbS8vdmZmKwiI7NrPLe8uiUlzFgo4jDjsP4wWmtLyCypCeZBLmtiJD2PZ10bEzj0ec6hjXY0N5nvDKNc1lm41d03m+/2grGxUs41UOpVLJwr2zhNwxgAaISxjARsDDlEBGPilMpQy9H9+6N+38Eq8EGjkfixdPJZZ2SscuiuQSMSkdx78m6nUOs3h5lKyc5mc+W6kDBXzlA3L4EhuA6TZG2pVSo5/a438GR3EG+sNIcIfL1SWTQxwN9YHgkhQYhijQCyCDIQzhsoQ5FLEMPQotgkEGOAIRIK2maitN/3Du2ujY9m233/xOHJpbXuC6dXfuL7DlZLuRtLfS71+Qs3fvlXPvCRvz0zVnQuXF27DZZA32Y+wz3T1cdPTn3vB+769d/+os1IEKalvP3wA/X/63dfvLjQfvatB1e3hhnLWtrorXgpzJo4w7QGtEhIiXEOUYYhirXSEENIsE6V1hrZVAcphBBACAiESgMNVCggwThHgVTQRIAhRDEI1bAXN3TSBkkYp0YEdSKB1HGSaq2FVEmSuI7d7gxq1TxPUz+MKWM8TbiCE6OFRrsbRGmlXKIGEwoxZkBE8rlst9vbf3h/qeT0euHK0pofCUrIvaeOPfj4ydZ2u1pxzEzp6bedunRhIV+uJHFUzhlL23GzF2+1k5WuyjqmY7E4Slwb5rNGfWauWq2mkdftJrvmp7xeXMwBw3ZNyqbmZkV/myA8WnNn5sZsRlFvc+bJt2xevGTkSl1P1O++y9/uk3x55frq5N5dR9725OLrV3ffuY+WqxA5kdcHAGkJzGxu/MjhNI6zkxPMtrMjs1FrUBgbMTO5XC6nAYSINNrDjc0uY1YUxX0/6HSjOFRJHEbDqNUNe4XMhZnyDZsIKRAjpGzLXgIIAhhAjCGGEGqUN5BNIEPIJsiliCFoEZ1IzSU2qZYqTfXiSnNmtAAgbraHD983d/7S+qWr7Scfnbpwuee6xtWbTYfBh0+OLy0OtrvxwPN3fH4TUcg7u8dr9x4vUZz+1h+88o4n5m+tdL/7Ow5//DPXvvTCciFnLW8O+n1umuTycg/mbFa2AMK0apACUwhDmwACAQAQQACAVlpLhWysEokJBBICCABBOpGAKwChlgBiCBQASqtYAQSVz0GqCMUagB5UwyT2BVcIUqnThIuUh0G0utbQGmQyGYThoB+lXAyGQT7n5jLGtZvrGAA3Y/UGHoIwm820Ov1yNesP/NpIPooSP4orlVLM8dh47cgd85ZFGGNxog8f27e1usoh3jUzsb54/fpqcHU5XGvz7aFKuQrC2GGkF/K8BXfNjT/21GO3biwXy1k351RqtSefuuf6lYVsoeIH/Du+85TX3Aj9aGzEmb/nntHZvWuvn931yEPdZlA/duD1F24+/kPv1VLmxye3F27sv+9EfrTmdYbMYeN7D0MHl8aniWUliXRydrY+ooFKfQ9qTJDBY+75cbcTuMV8MPQHfrq50el1Q4SgHyTN7b4WoNOP00TELn1hvHRrKtvKUGVgICFkGGGsAo4xBAwBALTUAABkES0UEACZCLkUUoJsDIRSsdRRCqXWEALT2F5tzs9UNpvxRqPFBRgMkyAB9x0f7/RijPEnP3vpp3/8yHNfXZnfNXr20sqOz28WGMV3Hpia311562Pjf/7RKxnbLeXMXi967OHZn/nVL41VXcexR8olRsSlpW7EGMtZyCI4byACUZYBC+Mc01wBAJBBdKx0InGGaqERhshE+hu1UVIDpSFBACGgpAYQAgBiCSgGQmuhgNAAARUpKFWshEekT1TiwMiCQ0dzB5uQYAhlLBzHGnhhknKlQLvV2z1bv3BxkTLKDLZwYz2fs4M4bDUGY/Virzco5B0hVMrTONUKoJFaqZi3+50wl3cLpULgD65dW9+3b8Y0YczJZ7++1OwJYlpSYYqh1KDRHjx2ah5C/Z53PeJxMTFVHh2rNrYH09P1EycP5rOZlIu9+6Yskx2877juNaf27Bo9dBAQYWdKxKCAmhLnnJxlF/OZWh0oVSwWyrOT/sB3chbQBDOGEQnSqFAdlRgCyDKlPI/8oNP2u8OsW9hYb60vbvsB51y0tjtbm72hlyqhBVdBmEo/7Uq/7GY2LfqVsrVWMH2gVMSRgQHXUCrRTyBDgCGQKJ0qABG0qI6FFhoAAIRGFgKMYANroSBCgAPNFeASGFQg3F5vT07kK8Xy6mYn6xobzeET9+9+8czaaM1ttGUuQ7/72V1//cmlE0enLi5s7Pj8Px6E4P49E3snckcO5bIZ9MXnvfFR90N//eo7HpuHUH/muYX3P3vX5na/nDffuNboRoBkLZo1cN7ENoYWgo6BGAIW/kZmBlJrriEFOGdoqaHUAEAtlI41lApgiEwMGQYKIAaRTaBLlZcCgiBB0CRAAUwhyjOcYZBinTdSF0UuSgokqdFo2hzsMjnSuhFhAUSShEIqoMeqxaHPLcdOEjUYBBih2E+4FkAKyyBbm+1KNXP16jahtFzKRVGazbrtvl/IGsVC7sbNre3GcHayyAzTj9N2x19aa1FqWBbBEGkAvCD4ge97mDD2zDP33FzcqpYyfpjIVI5PjRfq5RyDn33uyom75kM/qs+OjO6dzo+VpUrTQd8ZmySM9rs9SB1MCEIIO47ikZHPAAyDds/IujKNTceO0ogqs7G+ZWctkag08kWYQKD8ThhFot/2u71w4KeDfqi4FFImqUIQUaRlxAc2fXX/+MDSL9XMIdRACpQqAKHmGkCtIwk0gBghl+lYQaihiVUoAQTIppAhaGIVSwABhAAVLJ1IiLGKBdAAKg0pTrTWw/DQfGUYpu966vBzL1zfP1cdeMn5K5ulfOXlM5s/+j3zN1f75841C5V8s9Xf8fmb5we+++TWev/JBw8ubnSk+CbfUcCI8cPvOb7Z9n/4/Qc/8ZkG5/a5SzeRlv/8J0791h++lETpXXdMnb+0STC4utTD1ZxZtCDDKENwlgEGIYM4b2qhIIYAAJ1IiBBkWA9TaCIAoBYScIBcAilCFoEEQqghI0BIyCh2TTFMMEOQIKAABFBDjRyiJUAMI4doCHCJQRfDLFUMKqADC/RnjXZeN7pDuOoZjrW03rRNZlhZLpJg6Pl+iinO5dxbS41QJmsb3TiSGoPri2vtbmjZFjMsP4z27p3pdzyukBfFpWLulTO3Nrd7AOJmLxj0B3GYOK6FCOJczYzn7jg4vb3V3VjvV4s2knJqz/RwGMztGnnh+ct/9B9eevDOqasLa/snC0LxNAgxT3mcmgVr89z1TjNc3fZ6jX65mAk73cbChmuZ/fYwbPbNjOlWakCppbMLc3ceXzx/RSTCcK3YD9MkxgTzWA+6oR+kAy+FlEqFVCJtDNNiURWynXzmcjVzdpR1ldiGWtkEKA0SCWyKNARCa6VR3oSRQhZVAw6VAgRqoKEBAYI6khoASBEAAEIAKdYAQoR0JHSqoFDKS4DWuJofdDzE+dxUqVpyzl3a1Egf3Tf6wisrCecz49Nnz6/+7E+deP7FzacemT1zYS1Ov8l10ZZtvf8dd/R68fRkqdHy/rH4XK2WkzS1TTY/M6qCztNPHXzua4v3HptY2/b5333P6mLB/r1fee/yavt7PnDs+ecXz18OCnmysLiyZ7qUc6wPf/LC4w/utxjB0FhYaW8PBLUNamFoYGRgRCCpWsgmkACAEYQIQQAUABAA9Z/6cgBAFUmIIERQSwUQAkojBKBBdaIwhbwVIRMDBIHSWiggNcAIaKi5AggApbBDtAZQAIgBxBgoDRnkBKQGSGfdaLkbvLRkZswUgpSnUcSb/Xi0ZpomQwiYjHQ6npQSETT0VLc7hIhgghFGWut+zysX3CgRIk1vLG/6fhonSS7nxLHsDyNCWZpKSjEh2B9Ec7OVJBVhlEAIry+2ZmYq/sDrdXovv3Kt2ewfmKv1uv2wP+ysdUzL7G0NECW3Xr1uW+YXnr9+/frWg6fmh8Owu91XWre6QTTwtRKUYgTh+rX1QT/EFm2tNrTUoR/FScyokYYRRDhOVZJiwlgU8mGYkDurH3eNcyV6tqyuMt6UCScQYKABBFoDBQCGQGvFJUQQAAiUBhioPteRgA5BWQOkEn2jf2RgiIGOBEAauUxHEtlEh1InSidSc6BTroMUAiAE9FqDx+4/cu3mxvEjE+cubo6P5AfDGID0gXsOfeq5yznL+K73Hfz933/t5/+XJ774wmKa8r97JxG945Gj5y6svvOJaRNGUWoMg8ixTYjgP9jreP/H+PzI3WMPnjoeDjwD8UwuN1UBzz6961PPrWMMvCD6u04aGIa18Mb1KOZHd5c+/DfXEKZSRRub7Z/8kQd/84Nfm50q5zP25eubj907+7EvLwDLcso2sgigFDGMckQjCAjCDgVQq1RpqSGGWikENHKITgCAAGoAMIRKQ4wghCpRMpEQa6iA8riWADEELao8AYBGNoMaAKUgRfD/Zu49vz29rvu+vU95yq+X28vcmTt9BhgMgAEIAgRIgkXsYhUlSpREaS3LhZGTvHDiKHkTJ44lx7a8HEWWHMdasagSM2IRIYoNICoxwGAG08udub397v3131NP23kx8ntLy6L8LzxrfZ69zz7n+9kSkXPwEC2xAgcLLjc8FADIBCcLFGl7pNws1Fqv3+3t7vc328Nb2x98avbv/p0Hot1+N+GE3BE1m1UpRBRnyGUcpfV6seBJbczqxn6xGO7ut4XkZCjLbeCLvb3Bq5dux2mGZIAb4yxat9vPTi9OxKktBjLJ9WCYr613RsPk9beWh4PsoZOT1phqJYwiPdYo9lsDJH739nZisTdQL19ceu+7HuoP8iw3Y/VCL7HDfu4JnivLEHc39pRyzsHa3d2xsVKW2qXl7ZInAx9L1dLty2tZO+22Oj964/Z+t1cuiXC8vDE7s58MpUQhOSIjbbkvsCDdfg6S0OcMkZcEkBOck4eQExLJokBiDMHlzilCBswXzFoMmcvv/4WZ3c9AOyAgbZExMARAZKwcL0WZeu/x+jd/uHRoobm62V08OHZssbG+PQxLXjEsX7y8+vip8e+/vPyd529qkknylwpIIyK8/6nTK/d2/vGvvbPX7l+7a5yOD8xM/+xnHt/c2u/2f0zbzn6s+WffY1PjJeP4j97qfHHa/tznTmWxBDTf+O6tpyL68i+f1FS6dL315uWtdns4GGZx8p/0FaIoLh2cWlxoou8nmTl0JJhpyM3tqkrzVJmTB8fqjfLd1c43n7+Rpro02RRFaQ1hbpyHpARqxwS6RLOKYAKACHzOqr7r55Q59AA5I8kp0ViWpKyNFC9KAkaJBQAoeRgZQAapRYEokYwlbZEzJjhY54xDRCBAT5DKWMBd5lAwO1AusQCEHms9VZnYmoal/QQhK8nzL9xZuba54rypGb9ZDj/17PQ/+d3lUoWPj9WiOL2zss0EcY55puvV8htv3W7WK7FIgzBgSETu+p2tTmvw1BOHtvy9+cmDV1+4k1lEYjdX9o4sNNMsH0a58Lgjm0VmrBFcX9klaDpX6g/SSjn8P//oIgVu7hD024Olq6lOyAsL3375xrseWdjaaP9wGNfGqscWmrv94VS9FEWZtZLIDFI7Pla+eqs1GkS+T8o668Tb3728vd0Ord/mxKvNNKNoZPBm/9lHFn6ny0VKgAY9Dpo5Sy5REDAZylLFW7ASkRDNCeQrlg95d26yPOMXhmm+lLsNkfcz5QwgMquJIUPJXE4MHa9wlyBzxENBQ8MK0rZjVgoo07Je+ep3rx09PJVEaqJRWlvvz0wU00SlcffhRxZ/61+/7RcK05OVYZLst9v/iRAXi0GjXhkfq557YOLkkZonp156bf3NK72nH5uvN05NNbM//tobccLmZxuCu5X1Afw1uwf/Gusz58L3JQEj5wBAcH7k0NSvfP6Bn/vJ42ma5aq8sbr8xLnKwaOLDxwf+8M/uf7KG1t3V9bf/zj/4idnn37y4UOHDiyvtooBjo9XlQLBsRD6Y82qFNw5kkJwjpyjFLJYkGP18kOnJ2any8+/cCdO09X13uJ8DRgjh3dW9zd2e4EXvn1rJwvKYYkz6SED4Aw4skCgRBZy1gwYEHJGwBAQ7++hRADBwZEzDhgiAzLIfE4MITeIDDhDwZyyYMENFAs4CYYMsCjAEimLHF1uwQJoB4I55cDjZBwZAo7I0RFYROpn4qmp2S0D3SR3ZidVYpiNev1f++/e83t/dO1Lnz3z21+9euPG5p2V7s5uP4m10bZe88Mg7HWHglMU6bFmqdWJQyn22sn5i6tB4O1sDhZOVJuPp4ceDYWRUSu9fH0HBesPsn5nOOgnq"
       << "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"
       << "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"
       << "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"
       << "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"
       << "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"
       << "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"
       << "Ll7xHzqmubJSEzhpn7ViDmyJnjAEgEAKU99PcGBIEbHa6PeinSaIYkG6/OBhUgaBbu8OZqWaam0GazUw2R2EGFLGyzjmcGIt6g+LI4tjFy7syUIIRKZlg1CMuzTXz0hAkvX7easQOXbMejui/nLM0091ensRKcjo7VRukxSA1WVFxCrc2+ujIW++d/bEP3/fst2/V27Wlhfa/+70Xgtmk6ZBLUVrXq8xyu/bIo0e/+tTFO45N3HvX9P/yq0/tdspCm7mpJKnFeV41asojFoXd3R8aR+JIzE7VitIQBEahNyyt99a4rDTHD42Vpbu12V+eb67vDgIhSu0AsF0LjHHGu6KwQlJKqPaEoC8Kc5CWjSSihHivhRBKcspoLRJRKLu9IqmFmztdY1BJnkQyUmyQFo161OsXeWWBMKVgYiwx2gLwlfWOsZ5TIRXf2OmeOzO3tZsBRQpEKhGHqixMFMZS4YnDzZdf7x1aTJ76xqV7Ti8Eim7s9KJAaWOUZPNztfMXts6emr2xun/uzsX+wO12siw31ul6TLXF5aUxIcR3XroRCSBIDLIjy2NAyLFDY/edmVKKWwNffOriF5+5eXxxcW1v7S2n5599eePD7z957s75Ye7Akzeubv/rP/wOlYEIwm63+xdVR/9y9OydI9ZKKnqD9PiRcQqkMPaH33fs29/dvXClH0lxY3vr0Mzcxevd5797fdir0MNLr22cuWPmqW9fCRifnmwbo0vr0yxbX++CCCdaUVH67b0DwTmhRDDOKSWIlFGgRDBGAabG2hS9LTLOKVDOGKMjBwZjjDPOKSHUO9RGT0233/euh87ddyfl8OIrb1y7vqaUBEDGqEdEjyeXW8z7hclwmJu8NBPtuuRsrBVNtaMsN+986OhuZ3B9vTs5FlMg462ozGxl3Ec+eO/Lr6/dc2rq+NJ4u6nCgG/uDIDTJ99+vCyMknxsLCaEzEzUdvf6+71sea65103PnpyRgiWhZECurnTPnpq+en1/faf32FsPcwFjrThLqxurB875586vTk42Th6ZSmribeeWkiBY2Tp45IHlrz97rVUPH7h3aWZK3VrZL3xwsLkzf3h2/shhXZWbN7e4CA/Nh8cPNeuRevbFW1979tpb713Y6qQP3b348FsXAskX58dfen3z8vXdhdnW1k7/sXPLc7ON8Xa0MNs4f2kbHSaxHKbV3SdnkiRgnN5xdOJbL6/cc3Lm1UubkeLLiy0lWRTJrDDtevjYA8tPffu6FHR5vjk/0zi53AZAxenJY9NrW4OxerA438gryxnsH2Rj9fCBOxf7/fzUkXHOmPNYS8JLN3bHW8ldJ2d7/eruO+YQrQqE85hl1YkjU0hcmpVPPnZ6ZjpenGtOjEX73SIJJaM8Cvn0lIoiwRm9cnPrys29u05NWU+AkIfOzdUiedepyQvXdgRlrVrw7fOraGwQB73hMC2KKytbYcCUUghkY7Pr0e3tFa16IFWw38nimIahyoclQ/qdl2793mdePX/14NjC/NX1deLIG1e6jz5w+L/+wInNnXw4rKx3v/6732xEtemFxVurq39xQvtL0jMSop2bnZne6xQ7nb63bjg0tWbwzkcXl2fqw7Qqy6oq9USz1Rva51+77hH7eXXq6PjrlzoP3Xu3c5bz8J7TR5bnGgShWRPAgompiTQttLYEEZ1zlAiglI5wYI4Beu/bzUaj0URd5JVz3nkkiN47j4gEiRR8YrLx4Lm7H33wvt6wePPSzQtv3CAAURxYY5RSuqoWZyY+8I4jly6v3FzfT5L4my9du+/0zPvfeWp+unnn6dkvf+NyoxZa55761rWFmfpDdy2FgaCEvHZlq1YPXnpl9dZ2/45jk4Hk3mGg+OVre0DIxau7q1v9xx44MtGM2vX4hQurl251Qs6yyvZSfe/JmbFWHCn+7MurvYOs1PrlS1uUQJ6bnf20PygvXd/b3BsMh2VpbMDFQb/odPWt9cEnP//dibGxY0v1emN8c7u4ub5LgV+7efDqpbW3v+XQwuGZyYm5sToXYF5+cyMApJRdXtnt9rFWj84cGb90PX36uSto5c5eeWt1f2O7t9NNJWNrO4NhVjnjb9zqXLrZ3dkbru8OvHYb24NmI1qYac5O1MrSvHxhfXt3EAp+a6NbrwVnT0zPTDaAkK88e5UytrXd7w/Lu09Oa+MCJQ6G5Qvn1+ux3NwbGGvuPjnbjOXsdOOgXz7z8spo5hDH6l2PHJ+fqZ+7a/5Pnrl++da+9zQvy/2DwQN3L1JGk0isbQyure4lYXDvmSP//qsvEkI9ujgUl27sVcZyytOyXJpptBpBVpooiN/x4Ok3rmwCIbvddHo87qdlZdyNtQP0tFkPX7m0xQDe9+jxiVbSqof9QXHm6HRh/CPnjj333Zs/9tceKA1XsubRzc7ORSFpNBQA+9I3Xn/21a1mrXnniWOvXb6YROLOEzM/8oOnPvDeI3mB/+E7N5NA/vrHn5sen5X1+oU3L/2FCu0v6bw92tNamJk5e3j51uY2Z+k7HjzSSFSzGbWbERKoJXJtq7u23jt/cff1G3t//UfOrVzfPTRXe/m8dmjyNFVh7cc+9MCVa7tl2bn/rsV/+8kXiVCHDi1fX1m/ubIWhgEAAHrwjjhHCCDxxAMwYpznQnIGaVk5QrQ2gGRxbnZxYWZqcryy+sb1tZW1dSRAKZNSEO8qraMgnJ0aiwVF7w56fSkFoA9CgQi31jvDQVEZwxidGq8f9PNWqxYoEQbBIM0Q/X5nwChdmJvodnuHFqZfu7JGgNSjcL+XHl2a4pxTQvKyGlFWtLaCAwNSlFoqVYvlKC4eia+FYlhoRJAcgkBcWdnp9obHl2YmWvVBXmxsd/My/6Wf+29fP3/j6995tdNL7zq55NFmefrRH733uxc3v/YfbjYb4VRbtdr1Gzd3d3d784emN27tTE42lxbGur1smJs3Lu/8jx99eH4q+bu/8PlH33L6oF9eW92px+GH/9rju529z37l24yrpZmxVj3pZcXz5y8dmplamB2zzuW5DhQVXAKQQiN6l9Tk/v4APcahKrVnjFFGgSKjoLhy3pZab3eGAaNc8DiSklNEApSurO5SRmem2t75ej02WveHWV5oIGSYllGojPUzU82JscQ5Z61lVBSVCcOwMhqdrdcjdBiEfHNnIBmxxhXGjzUjIWijrpwnaWaKyunKHZqrz880VCzTgd3v9cea6sXXN2bGxqwpiyL/6N9410d+5rdDxpYWF+8+PaN1ORhW33jxzQ++69wj56a/9q2VY0eOPv7Iob/3jz81ORY5i+Pt4I4TwalTC5/80uuX39henElOn5w4sjzebtWMsXlu07QcDqvtvf6Xvn55dvZIJMWbG+sbG5t/RfRMCAmVuvP40fFG/ZXLNwbp4H2PHrnvzoVWM5SSCUmbLZnEotFo9FLx1W9ePnsk+t0/fG1h6kRnMNDaZekuhfJvfOTJLKs+9dlnVzf7y0vjUtCxmfmd/eHW9jZjjHpfZXlZVUABPTrvnHNJEhMCIHgYxkzQZj1J4oQAlGW1urGzs7vHOW82m1JQyZnXRajY3NSEoJD1e9paxrjWRmtd6sp779EHSjEGjAIh6KwjQKXkWntE4qyLQhlHoXHOWcsYM8ZSCpxRbXwQBGVVeeeV4Npaxmktjr1Hrc0wzyTjBNBa76xLi0oJGkXcI6lKU2ntkSCSOOSlwThujjZEdZFtddIHz925s9f3Juv1e4SKXr9o1oOjhyeJCn7wvXf1Owf/6J8//ejdsx9839lzjzz22osv/Oa//WY/cz/94w+KJP7kp1+4+/jkpz77KggxN9nIhoN6vRknNef92vp6HMogiIFyh2RnZ2tpqr7dKzgDCsAZDwKOhBSlZxSUFKP+O6FgLMZJEKgA0Pf6Q8oYZ0wbJyUjhHpnKSXOe2c949waG0UBQVKaqigqIAyJowxsZbiglLER0bgyVmvDOKWEBVKEYQgjyCslQnJr0VtSFEOglFDw2gIDJmRRVt67QEpCSBhK7yDPi15adNMcCEnCuBbH73vniSs38fjh6NKVa2Wliso36u1Wi6f9jfMXN4I42d7P3nL30tsfubdWY7/4T39vfnEhUM1Wo761273vTmW9ecvD9x9ZqCcy390/KErb71VVjkj89s7gmRdX/+T5lbmJ9sP3nKnStfM39i6vdf6K6Jkx1q5F3A7DMBofn1YqXt06IDj4r95316HFtjNufDyamU8OHTnxR5974ZXXNqvSEN0eHxvbP9gf9LckS6WU2tP77j+9tp6CLgkOKKWdoY3q40IKBF6VFrw/GKbeOgRijMuybGlxnqCnQVCPoyxPdVWVWb6z3y1KHcdSCSE4G2/VQ8UFmCobgLcIxFsHQAjQkbGJMTaCUQGAvx0TNwqN/96KOBBE74136LnghBBnHFAARikl3t7OtQEAgt45PzI6jxD8aDxSQIdcckLQWU8QR607xqlzHiggIYxSZxxjUBmDhHDBJRcAqLWhjBIkFkFX1jpfVmZ6qj4sXFXa7iCfmB3f2e0cPTLRmGgf7HXWVnuBDOqS3ljff/T+5d3tPmPAGHWI4NFUmgASoCpQBJBScNaN3OEOgVLCGB/BH/B7Zo/bwXLWEu+9Q2tckASjhWj0BIBQBujQORwl8FHOnbbOOeCUICHea2OCIET0xHugzCMKJbzzozCzVyKLAAAgAElEQVQaYOC0u72/65EAjP5C77yQwmjjjEXvR7nWMlTOWDrK/iKeEKjykitJCNFlxYRAj0Iwh6OJhy+0l5LVovbO9lZcV+B9ZWBYinTYqUWsnoSEtayUpw5P/PvPfH2yrYrKcDG+uHi02WhduX516Vjz8sU3fuqnPvjQPZNvvPlmZ0f3emVR6utrnS8/fSUIWjMT9aWpoBWWeaU73fzZq/lup/NXQc/NejJXF5QY4xABwyCZnZrfGRhdZe0GacTy5NHJxx878pVvXv+t3//23Ozk3Nj8Xafu2NjebDeMNd0wiJJYAoNrK73pybFGQrxHQqDMcvTOc2UMEKAqSohQeam9J4xzRtAYr0KJOiuLIlSCoq8lKq5FxmKVDhXn2bBntSYUEAjxxHtPKKXs9nfWOc8k/96Qi3jrnPHOOuDABUOH3nvGuXfOW8eUgO+NvJ0xVLCRYL115HtsOu+8rSwwkIFERMpolZVMMG8R0XvvmWBAiDWOMco4Q0JG8H3BuXcOOCMIQIlUggIdsYSs8956bQwScM4hkso4xmhpHKes3Uoq76NaEMWMeBgcVLVQUgBtNHhCvDfOaW24YIBotBOSWecZo4xSHghbGaBgtR2t58Dtid/tGSAiUkqds1wKq/Uomp4LziTz1iMhQMA5J6VwzjHOCBCnLSHABAcAU2ku5bBzENZq1lToCVfSGTM6/zDBgVJvHVfSaj2K7Bv9DxGJDCXl3BkPlFhjbK6ZZIgEEUcsKmetUAKRcCmcsYxTQqmzzhnrrAMCXI4cL5BnuWBMSMk41cZUlfbIKCFAUAV8kKmbqytHlsb6aZWmpUevwomJiaUkrm92Np9+5jmw5c/8nXe9922Lv/eJ8zud/Pqt3X4qJsfbp480W5FNAg9CdjrD3d39zpB87ZW1/zhV9PtQz0GglqfaNZZRLqx1BCihRHLWqE8kjandbnFwsN8d7B5abK2sDovKnDpyeGF6KQx8pAYUKkJYFAf1JKrVYgKkrDTjAggyRp31WltPPPEeEFUghQoIUO9QBlIJ7hGiOKLelFnmEd0Icuudc75MC2AUnUPvCSKh1HvPhfDeO2dHuVrOea6EN6MfAO8cQRhBy4ABE9wZRynxDr33XHJvPReMCW4qM1puZ5zhaMjocARQsJUhhDAxuiKg1VYE0mo7GqWJQKDzRVaCR8qAKVHm2lnHGOUMQApjPSVEBVIqQSmgI4SSMjd5UeWFts4DJaOjhZQSgKpQBkoipXlRteqRYJx4B0A4B3TeIw56A49AvFexGg1Fq7wUSlAC6JFyOrJwEQquMqP3FRIkAEJwoNRqC4DOYp7mcT0CoExyxphzFj2iR64kIjI6ihEBnRdAqZCSUNBFNToH1Vr1/n5XBNJWRleV01YoyQOJiECBM+6dd8YIpYCBR7SlIYQIJZx3jLPsYCiV4koQROc85xwYtUWFBGUSmcoaXXEhkGCZVx7RVIZzpkLpnTfaUildpaXkSFAbV5WGCi4Yi2JFAIqsoIxVxubDoqh0mpcH/RxoPDl5VFu3sbO1trFRavPwuZlnnr+1MLccyvDMyelWzdRDy4Bo47U2aVoOs/zSrf43z699f9dnAFiem5iPXBhLY1EbQ4BQyqWkQDCMGkolTM7cWLt1a3PVO7z37Kk4Gju+VBtrFgRMmjmC2B5vtupxFAVAqdaWMsYZ1UUlo6DIc6MtY4x4J5VgjHEuhBBhFAZRBIwDEO+8LgprnbW6HKZIibPOlBUBQO+RIGWsGGZCccq4swYJUs69dVJJU2mPRAbSaQOUEgLOGMq5944xhrfhXlxrPVpOoIx69OiJKSo+ImJTQOcpp85YAmArbSoDBJigXAjKOPHeAzrtombkrHPalmmBxKMn/nagAHjnGaMyVMCZ98gICCUYF+gIl7TIdVFUaVaWhaaCae2d84TSejOJoiiOFQfmEBklnHhKKWMgFXfOWWN7ez2mBCEoJXfWVkUF9PZ8j3gUcVAVJQEyOngTQiijxKO1jguOiIxxXelikHEpwlqISIQSo4wRZ11VVEIIJvjIuzOysgJQYwwQwjhPu/2xhRldls459F7FgTeuTDMquFCySgtVi8p+FjYT7xwhUA1zFYfee1MZLvltf431Kgk4E1wwQggTfLDXs9qSEblG8BGVSoRB3s+MNVxyKZVQ0mpdFbqqyqhed3nuPGrr87yijMZJ1GzXy2FuEUttO9udUXwvMtbrDw+6Qwqy1jq61x1cX725vdeVgi3OLsxOTd9ztjYxpmyZ6qIEQsvKDPpp5fywn65sdl9ZyXa7g+/v+vzO+w5P1RwTQZoWeWU9IdZ6CiSKQykY51SopNU8+ubNjcs3rz7+wNuPLMbTY8MwklJFw2HuHbbGm0kSCiE45c57AkRKQZAYbZx1VVlQevuuK1TAuVBBILgQKkCHQS02lfaIBElV5sNup8wz7+zIdksoAEGPaArtvQWg1hj0yCTzxjMhiPdcCUqpc7cjMr21zjoquLcOGB3FCBtrZahcZQmjxPuRnQsYA+8dekopUGrKyjnHhUBEgiiUJADoPFdC56U1hgrhKm2McdZ+Ty1sdKRH72WgKGMECGOcwgjrS0FwQPTOGWPLUpdlSQCcJ9oi5SKuRbU4kELGSdIam2pPzXprsvSgs71aZRkwMKXOhpm1jivutPHO61ITQlSkAIgKg9E0kFBw2lBGrXEAxDtHR95pRoSUZVqgRy4ZE4LedoASAgQd0tsBmlgVhqCnlKpAefRZLwvCQEQKnS/yAihljFLO88EgadbSXp8JAUCtNmESl2kW1mtWa86Fcw4JEUJ474DSKitEqDjnXHGnPQFA48J6PDqEeee9cx69t07VImuMd67KKhmF3jrvvAwEIaDLKqjFFNBqO+xnNFROO0YhUiqox+kg7e31KsQiy7UxwGiRl9ZYbYxSgYOl9Z39r37zG48/9Mj8THthpmhEDI0bnSzKokqzoqxslhZlqdO8WNstn35ty1n9FyE0/pcg5jsOz85GutlqMUpDyfJSp4UutEPrtTacMWc9Z7kubxxdXHDO18Py0AwPo1BwRQQXvKaiKAzjqen5qcUjUtUo5xSJIQSctcVw7dabw96+9+iMZlyEcWNq6Vh9bIHSWAgqgjAfZpIJizY92DrYuW7KilJapEPKaNSInLHOe52mQaS849Y4CgB8lOLtkICKI/SeMooeuRLeeqY4Y44QgpJTCpSCJ0gZ51wILihllLHRyIxQ4IJa7aw16DwEAeOMUiBA0TngbGTMRueVklJJXVQqiYq0qCwSjyKQAIQ5QELCWsKloDDCDDPGOJdSCAlAmVRAsMzz2GmjNRDivPOOMhnEtcbJO+9rTs5laXnhldf/4Lf+5d7Gzff82I+/48kn0+764GC3t7vNpNRFicQTIUZHCe+J955xRggQikIFVhsEzwWjowst51QKRukIaxxNRfvrHYpcKjW6GDtjCWXovbeWjuhlAnmguBDoERiEcUwIcM6rvCAhAoFskCZjQVxPhvs9QoAFHICoesKFEFJxLnwYS8Gc9VYbHkhrta8MpZTREb7ThbUaADDOORe61EJwEcrOxl4YK8pIleWcCgAOAQmTmAthjWOUUU5tpa21jDMupJSKBcoWhjgvhKTAx8Yn2xPjg96gP0jLqkwHGU+o945QKIpS2rWx+vj89OzZY4dD91pDTgaCewLpIGWBCpQiiGFEZaAOOgcAdKZl75zmr2zY78XVfl/V57ecWV4Yi1tBPjbWZBSs9UholuthXuXaV6UOI6GkjJOwngRzs7MXr7mFOX/urumisEIJQmAUcB7F9UuXb33m01/WpN9IGmBp5QoA73T4ix/7Z1jt5+mACSGDgHjxx19/7ouf+kp7xuuBiBq13k6ntRQMd6qCwUd+5G8+/NbjnZ0tU5amqpJ2oxxmTHKrDQFSpBlllBDirB11UznnlDPvPBd8xFBgnCJBxpk1rsoKJEAYkVxQyqw1MggIEFsZdpvaRz16NM579ASrtBAhZ1yYUoP3Mg6rvEhaDW+st64oSiaY1cZ5Z4oSHcoktJUFQBkoxjhXigGjnKsgEEoyoWQQHTlxzxuvX+rs7x4+NLmztepNeRtVBOLomXsrp37u7/3j7k6XQy/PB4yiijE9IExMPvrkQ4rTdzzxwcPLE5df/Y6zlbeOMrDGeY+UUcaYsz6ohYikygv0zlQjm8Qo0ZlTzgEJE8JWWgSKMmoqzSS3pUYCjDGHnjgHjFEGKoycNlVRhLXYVs57356Zzvu5NbpMh94bEagqLwgQXZZUMELI7VM/YxSoShJvnIrCqqicNcBYmaa6LAlBGUiplCl9mEQyDBllQIEJASN3HSO6Kr3zZVEgesZZOSyF5FwoW5kwiQilHp3JKyoAOEPngHEgAJ6OfpXzDoHkw2E6HA4GKRLHKALisJemhY5i/vHPX3j8re8+fmiP8UCFga101h9aj96jMxYJMegZpQR8p5PtbG7nJV7dKl9bPciy7PtJz0fmpxaDfnO8GXDFGcSRbNaiWjP2BNJhNSj0MC1FFID39Thot+qzs/UoTghhSpJRi9JWpjbeZowHKly+466KTv2zX/j7q+vPC0FZwGxhms0zf/CHX7j4/OcJemut10bFteN3ve0rX3vmX/zaP/FFV0Q03fIgSGNy6WP/+/9xaLZx/Y0XTVWgc0xIgsAEJwTQ26osrTWmKJ01xhjvnZCyTPOk2QjiiHhKpQjC0FlrK2211rryXjPOCKHEIUhGDDLJnXWUMiQ4ancRiuicM44KqittCq1CJaOAUmZLo5IaZ5QAAKflsLSm1FVhdFUVhdOGcsYFV3E86tlSKoSQlDLOOBOy1hpvzZ/+yZ/474dZRVAcORL9o5/9ybXVawBAgM0tHXv6mVd/6Zf+5Q888fDk9OznPv05xcGYUoaIzkrhygIRyeoK+dGPfujv/+zfvHXlFaMrChRAhbWmCOIwSpgKAYmp8kFvO+3tmqoc1TEmgjCqMaFUoBjj6AkwUg56hCFBCsCstcRbJMgF09oQ9KYyQgVxoxnVW0KEIm5vbW8SAiGTQvJisJsf7FhbZlkGhFhjGOeUAhNSyCCq1RhI4EIISYi3ztkyr6qCCujvdQghQZwEURgEjbDWECrkPAKgBI0ucmM1AZf19vPBgIeCEKBUcCEZ8KheAyqct0CIrcpKF86W3hPBlFR1GdW4DIQKAAQ6PTzYGvS2e529wf4eUKCMOq+L0mjr8iGphU5FQW+vC86rOAiSqCy1J2TUpt3fPuiU/s2bva3NjcKzQeeAS77Rs0VZfd/oWTDwHqNQKsGOLc2dHDdTU21tPQOIA+k90YhZYTxluiwXF6YnJxtJrMI48MYSQnkgKSEyCgHpqDoFYVQfP/zxT37281/4w7Fp4Qnxqd3p4q/+2r86OR3qckgodVozLkQQHL3nHb/7W5/4nX/3C15rpwkNon/wc7/yvnc9dvXCM2WWScFTDVFcF6peWMIkpUCYrfJs2O1sMl8IAlpXTruk3WqOLWkW5MOi1opu3bjerLcHWdaIkyTEarhvvQ3CuNaYihtjxhpnDAIRQnARMMattVVRIDpvddrbzrMeYwyAcqkm5k6trG+NjU2UeXrj+pUTZ86uXL82OzmDpl+me/lwSAjqsgIgKoxkGMS18dbkLBCo8oGQNAgTFy3+xEd+1JTiG89/7dVv/4mkHs0wH/YJh5mZ+S9/89X62NS73/VOSSkSpy04R3v9njfu5srNz3/2qy99++kgzoXCN86TT331E4t1vbu+IsNgp5t/8vNPd7p9Rout9S7xcJDCZz7zO8X+1SpPrbdKqVu3Np579Rbh6vDyobAZrl3dWDy6/OS7Hh3sraRF1R3qOK4dPnrCWpse7JTDfeJtc3K6svDGpVtf+erTq9vXOlt7adbl0ntHdE5++K//9E/8+A/Z7GBr5YJ3dgSUASRKRRokC1pJe4oBQ0GdR9QFKXquGupyWBU5IguTaHJi6dLG9v7ecH177aXvvrTfXX/XOz7Unhpfubr2xBPvPr6YHGzd8t5a7z2ETCUiaQ56A6HqXAIFShleOf/s2eOHrdXAgs98+dnnn3tZ1P1gr5sOysLje9/znp/57370+sUXi8EAOegiL9O8zAvCeL1dK7IcnaeMmtI4Y8JaZK0rs8IYq+IYvd/f7+7s9PtZdZDajQOdRFGnc3BpZScr9feBnsMovHcxDjiZmRmXKqmRLpexkExKAQBRpATnKgx6w6LTHUolp6dbzVoYJSFjnHKKAJQQIRUX0iNKLoRSYRDF7cXf/sPPffGLn6yFBB2IGvjKi+TwZ/7vT6ZbF7M8HXWYBVVLpx544oMf3t88r2JmKyfqjf/tY791emFsZ/NGVWa1eu3lC7d+/4/+5OKFCzkMAQkSEtbJ0YW7P/zed5w7szA46ACBpNU2dPzb3734yx/75SInd99zSsjaeLt2a2Xj2Zcv/e5v/M9njzS8Myqa+INPf/najWvj4wvN8fFGLGUUb6zf2Nvp5Zk9fPRIaQsp/BMPv2VhMul19uJGW9PmV775/G/++m+kHXf23jMiVPOzUxdef2Nrc+Mf/A8fffdDR9fXV0eNNxkFnMqo1ii9eun1W/1h9eQTb58fDycXjv6tv/0PP/v5z/9fn/jEyVkqBEn7vb3tbVuVUS1aWd1S9fHTJ09cub6+ttk5cvToYJjFoRQqzks9OTE+Pz//R3/0xRee+9b5V74V1KqV/6e99w7Qq6r3vX+r7vKU6SWTDiEQegQDKIKgoCAo4MEjR1F5FUQECyqWgyJeLAjSRBEbSLGiNEFAinACSAmEloQkM0kmmZZpT9lttd+6fzyRc+89596/Dnrf953PXzPP7GfW3ms/3z1r1lq/73cze/gv9/B8R2N2qqO7t9S7/LMXXPzsX//c1Q0sBFXAe0654HMfP2X9c//mvHHGlsvtbX2Lf/6ru6+/7lZl8rY2SAv4wGmnf/Fz5258+amf33rXiy9vDhjUG1PtPQt+fu1FSb32q9sfrjWapbauIw5/48jocFJPypX2rWObnn38MU2mJ7aDtXDepz9//FtXCZxJmzXv0HuI4/LOVL7/zLOZA8LAczAWuBEXXfiVk487eHT7kEdriqJ3YME9jz5T7Vzw+LNrt24f7G2v+lSteW7d+ORMey80GvCXB//cxuuN6TFCxZYds9fe8IeNr77YbCoegXUQeHHQqiM/eeaJu89vU0UehvHSvVbd9Jv7r772AhFoLgjx3tj5993xx60vPlKbHM/SJM8KUxTAadFMwfuoWsoaCefMex9VSgBgtKGS60x7jzpXQSnMlVWqmJmY1Vo3m0mWqhe2qxe3zRhj/m+fD7PWOhqLkMRURQyARBad1957qLaVglByyqJyQDiNS6FzWK3GnDMuJQEPQJwyRa5EaKJSiUsB3hPCmORCSi9FW3e/wLBpd1jjvCeN6aEv/+vXP/D+k1YsWVCbGY/jqog7Vz/1srENUuXGAUacebHbwt0KNeoBCWMz07N779Z/64+/89Lg7NlfOE2Exhm3116H/fK6X2989p56bVoEATrf3rXwXR/4cpJu6p0P1c7Ft/7i9q44VclUbuVHP/XfOG1FlZOe+Uuuvf53y5d3rdhzVb3R/POD94xNTFAPixct2Wefldu2DW7cOlirjZ7yzuOCkFXbOxITXHTFFS+9/FxcgaW7Lb/t9t9X6XSzNpnY4LKrf9bao+EdEqCMMYLAJS93dEnaQYLG9y/92uNPrz7/M1/J1z/x5DPPdPe2L95t98mxdRa1zZK4FCY6b8zWFw70hVGcNaaWL1+0fmv9lH85Mw48UPAW8hze8+53nXfGu/7p2L1Oee+"
       << "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) 7px 13px no-repeat; }\n"
       << "\t\t\t#masthead h1 {margin: 57px 0 0 355px; }\n"
       << "\t\t\t#home #masthead h1 {margin-top: 98px; }\n"
       << "\t\t\t#masthead h2 {margin-left: 355px; }\n"
       << "\t\t\t#masthead ul.params {margin: 20px 0 0 345px; }\n"
       << "\t\t\t#masthead p {color: #fff;margin: 20px 20px 0 20px; }\n"
       << "\t\t\t#notice {font-size: 12px;-moz-box-shadow: 0px 0px 8px #E41B17;-webkit-box-shadow: 0px 0px 8px #E41B17;box-shadow: 0px 0px 8px #E41B17; }\n"
       << "\t\t\t#notice h2 {margin-bottom: 10px; }\n"
       << "\t\t\t.alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #333;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 0px 0px 6px #C11B17;-webkit-box-shadow: inset 0px 0px 6px #C11B17;box-shadow: inset 0px 0px 6px #C11B17; }\n"
       << "\t\t\t.alert p {margin-bottom: 0px; }\n"
       << "\t\t\t.section .toggle-content {padding-left: 18px; }\n"
       << "\t\t\t.player > .toggle-content {padding-left: 0; }\n"
       << "\t\t\t.toc {float: left;padding: 0; }\n"
       << "\t\t\t.toc-wide {width: 560px; }\n"
       << "\t\t\t.toc-narrow {width: 375px; }\n"
       << "\t\t\t.toc li {margin-bottom: 10px;list-style-type: none; }\n"
       << "\t\t\t.toc li ul {padding-left: 10px; }\n"
       << "\t\t\t.toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
       << "\t\t\t.charts {float: left;width: 541px;margin-top: 10px; }\n"
       << "\t\t\t.charts-left {margin-right: 40px; }\n"
       << "\t\t\t.charts img {background-color: #333;padding: 5px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
       << "\t\t\t.talents div.float {width: auto;margin-right: 50px; }\n"
       << "\t\t\ttable.sc {background-color: #333;padding: 4px 2px 2px 2px;margin: 10px 0 20px 0;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
       << "\t\t\ttable.sc tr {color: #fff;background-color: #1a1a1a; }\n"
       << "\t\t\ttable.sc tr.head {background-color: #aaa;color: #fff; }\n"
       << "\t\t\ttable.sc tr.odd {background-color: #222; }\n"
       << "\t\t\ttable.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #333;color: #fff; }\n"
       << "\t\t\ttable.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
       << "\t\t\ttable.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; }\n"
       << "\t\t\ttable.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; }\n"
       << "\t\t\ttable.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
       << "\t\t\ttable.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
       << "\t\t\ttable.sc tr.details td {padding: 0 0 15px 15px;text-align: left;background-color: #333;font-size: 11px; }\n"
       << "\t\t\ttable.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
       << "\t\t\ttable.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #222; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
       << "\t\t\ttable.sc tr.details td div.float {width: 350px; }\n"
       << "\t\t\ttable.sc tr.details td div.float h5 {margin-top: 4px; }\n"
       << "\t\t\ttable.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
       << "\t\t\ttable.sc td.filler {background-color: #333; }\n"
       << "\t\t\ttable.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
       << "\t\t\ttr.details td table.details {padding: 0px;margin: 5px 0 10px 0; }\n"
       << "\t\t\ttr.details td table.details tr th {background-color: #222; }\n"
       << "\t\t\ttr.details td table.details tr td {background-color: #2d2d2d; }\n"
       << "\t\t\ttr.details td table.details tr.odd td {background-color: #292929; }\n"
       << "\t\t\ttr.details td table.details tr td {padding: 1px 3px 1px 3px; }\n"
       << "\t\t\ttr.details td table.details tr td.right {text-align: right; }\n"
       << "\t\t\t.player-thumbnail {float: right;margin: 8px;border-radius: 12px;-moz-border-radius: 12px;-webkit-border-radius: 12px;-khtml-border-radius: 12px; }\n"
       << "\t\t</style>\n";
  }
  else
  {
    os << "\t\t<style type=\"text/css\" media=\"all\">\n"
       << "\t\t\t* {border: none;margin: 0;padding: 0; }\n"
       << "\t\t\tbody {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background: url(data:image/jpeg;base64,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);color: #E2C7A3;text-align: center; }\n"
       << "\t\t\ta {color: #c1a144;text-decoration: none; }\n"
       << "\t\t\ta:hover,a:active {color: #e1b164; }\n"
       << "\t\t\tp {margin: 1em 0 1em 0; }\n"
       << "\t\t\th1,h2,h3,h4,h5,h6 {width: auto;color: #27a052;margin-top: 1em;margin-bottom: 0.5em; }\n"
       << "\t\t\th1,h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
       << "\t\t\th1 {margin: 57px 0 0 355px;font-size: 28px;color: #008467; }\n"
       << "\t\t\th1 a {color: #c1a144; }\n"
       << "\t\t\th2 {font-size: 18px; }\n"
       << "\t\t\th3 {margin: 0 0 4px 0;font-size: 16px; }\n"
       << "\t\t\th4 {font-size: 12px; }\n"
       << "\t\t\th5 {font-size: 10px; }\n"
       << "\t\t\tul,ol {padding-left: 20px; }\n"
       << "\t\t\tul.float,ol.float {padding: 0;margin: 0; }\n"
       << "\t\t\tul.float li,ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #333; }\n"
       << "\t\t\t.clear {clear: both; }\n"
       << "\t\t\t.hide, .charts span {display: none; }\n"
       << "\t\t\t.center {text-align: center; }\n"
       << "\t\t\t.float {float: left; }\n"
       << "\t\t\t.mt {margin-top: 20px; }\n"
       << "\t\t\t.mb {margin-bottom: 20px; }\n"
       << "\t\t\t.force-wrap {word-wrap: break-word; }\n"
       << "\t\t\t.mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
       << "\t\t\t.toggle,.toggle-details {cursor: pointer;background-image: url(data:image/png;base64,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);background-repeat: no-repeat; }\n"
       << "\t\t\th2.toggle {padding-left: 18px;background-size: 16px auto;background-position: 0 4px; }\n"
       << "\t\t\th2.toggle:hover {color: #47c072; }\n"
       << "\t\t\th2.open {margin-bottom: 10px;background-position: 0 -18px; }\n"
       << "\t\t\t#home-toc h2.open {margin-top: 20px; }\n"
       << "\t\t\th3.toggle {padding-left: 16px;background-size: 14px auto;background-position: 0 2px; }\n"
       << "\t\t\th3.toggle:hover {text-shadow: 0 0 2px #47c072; }\n"
       << "\t\t\th3.open {background-position: 0 -17px; }\n"
       << "\t\t\th4.toggle {margin: 0 0 8px 0;padding-left: 12px; }\n"
       << "\t\t\th4.open {background-position: 0 6px; }\n"
       << "\t\t\ta.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background-size: 11px auto;background-position: 0 3px; }\n"
       << "\t\t\ta.open {background-position: 0 -13px; }\n"
       << "\t\t\ttd.small a.toggle-details {background-size: 10px auto;background-position: 0 2px; }\n"
       << "\t\t\ttd.small a.open {background-position: 0 -12px; }\n"
       << "\t\t\t#active-help, .help-box {display: none;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px; }\n"
       << "\t\t\t#active-help {position: absolute;width: auto;padding: 3px;background: transparent;z-index: 10; }\n"
       << "\t\t\t#active-help-dynamic {max-width: 400px;padding: 8px 8px 20px 8px;background: #333;font-size: 13px;text-align: left;border: 1px solid #222;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: 4px 4px 10px #000;-webkit-box-shadow: 4px 4px 10px #000;box-shadow: 4px 4px 10px #000; }\n"
       << "\t\t\t#active-help .close {display: block;height: 14px;width: 14px;position: absolute;right: 12px;bottom: 7px;background: #000 url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAYAAAAfSC3RAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAE8SURBVHjafNI/KEVhGMfxc4/j33BZjK4MbkmxnEFiQFcZlMEgZTAZDbIYLEaRUMpCuaU7yCCrJINsJFkUNolSBnKJ71O/V69zb576LOe8v/M+73ueVBzH38HfesQ5bhGiFR2o9xdFidAm1nCFop7VoAvTGHILQy9kCw+0W9F7/o4jHPs7uOAyZrCL0aC05rCgd/uu1Rus4g6VKKAa2wrNKziCPTyhx4InClkt4RNbardFoWG3E3WKCwteJ9pawSt28IEcDr33b7gPy9ysVRZf2rWpzPso0j/yax2T6EazzlynTgL9z2ykBe24xAYm0I8zqdJF2cUtog9tFsxgFs8YR68uwFVeLec1DDYEaXe+MZ1pIBFyZe3WarJKRq5CV59Wiy9IoQGDmPpvVq3/Tg34gz5mR2nUUPzWjwADAFypQitBus+8AAAAAElFTkSuQmCC) no-repeat; }\n"
       << "\t\t\t#active-help .close:hover {background-color: #1d1d1d; }\n"
       << "\t\t\t.help-box h3 {margin: 0 0 12px 0;font-size: 14px;color: #C68E17; }\n"
       << "\t\t\t.help-box p {margin: 0 0 10px 0; }\n"
       << "\t\t\t.help-box {background-color: #000;padding: 10px; }\n"
       << "\t\t\ta.help {color: #C68E17;cursor: help; }\n"
       << "\t\t\ta.help:hover {text-shadow: 0 0 1px #C68E17; }\n"
       << "\t\t\t.section {position: relative;width: 1200px;padding: 4px 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;background: #160f0b;text-align: left;-moz-box-shadow: 0px 0px 8px #160f0b;-webkit-box-shadow: 0px 0px 8px #160f0b;box-shadow: 0px 0px 8px #160f0b; }\n"
       << "\t\t\t.section-open {margin-top: 25px;margin-bottom: 35px;padding: 8px 8px 10px 8px;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px; }\n"
       << "\t\t\t.grouped-first {-moz-border-radius-topright: 8px;-moz-border-radius-topleft: 8px;-khtml-border-top-right-radius: 8px;-khtml-border-top-left-radius: 8px;-webkit-border-top-right-radius: 8px;-webkit-border-top-left-radius: 8px;border-top-right-radius: 8px;border-top-left-radius: 8px;padding-top: 8px; }\n"
       << "\t\t\t.grouped-last {-moz-border-radius-bottomright: 8px;-moz-border-radius-bottomleft: 8px;-khtml-border-bottom-right-radius: 8px;-khtml-border-bottom-left-radius: 8px;-webkit-border-bottom-right-radius: 8px;-webkit-border-bottom-left-radius: 8px;border-bottom-right-radius: 8px;border-bottom-left-radius: 8px;padding-bottom: 8px; }\n"
       << "\t\t\t.section .toggle-content {padding: 0; }\n"
       << "\t\t\t.player-section .toggle-content {padding-left: 16px; }\n"
       << "\t\t\t#home-toc .toggle-content {margin-bottom: 20px; }\n"
       << "\t\t\t.subsection {background-color: #333;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
       << "\t\t\t.subsection-small {width: 500px; }\n"
       << "\t\t\t.subsection h4 {margin: 0 0 10px 0;color: #fff; }\n"
       << "\t\t\t.profile .subsection p {margin: 0; }\n"
       << "\t\t\tul.params {padding: 0;margin: 4px 0 0 6px; }\n"
       << "\t\t\tul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #2f2f2f;color: #ddd;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px; }\n"
       << "\t\t\tul.params li.linked:hover {background: #393939; }\n"
       << "\t\t\tul.params li a {color: #ddd; }\n"
       << "\t\t\tul.params li a:hover {text-shadow: none; }\n"
       << "\t\t\t.player h2 {margin: 0; }\n"
       << "\t\t\t.player ul.params {position: relative;top: 2px; }\n"
       << "\t\t\t#masthead {height: auto;padding-bottom: 30px;border: 0;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px;text-align: left;color: #FDD017;background: #331d0f url(data:image/jpeg;base64,";
    os << "/9j/4AAQSkZJRgABAQEASABIAAD/4gy4SUNDX1BST0ZJTEUAAQEAAAyoYXBwbAIQAABtbnRyUkdCIFh";
    os << "ZWiAH3AAHABIAEwAlACRhY3NwQVBQTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA9tYAAQAAAADTLW";
    os << "FwcGwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABFkZXNjAAABU";
    os << "AAAAGJkc2NtAAABtAAAAY5jcHJ0AAADRAAAACR3dHB0AAADaAAAABRyWFlaAAADfAAAABRnWFlaAAAD";
    os << "kAAAABRiWFlaAAADpAAAABRyVFJDAAADuAAACAxhYXJnAAALxAAAACB2Y2d0AAAL5AAAADBuZGluAAA";
    os << "MFAAAAD5jaGFkAAAMVAAAACxtbW9kAAAMgAAAAChiVFJDAAADuAAACAxnVFJDAAADuAAACAxhYWJnAA";
    os << "ALxAAAACBhYWdnAAALxAAAACBkZXNjAAAAAAAAAAhEaXNwbGF5AAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    os << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    os << "bWx1YwAAAAAAAAAeAAAADHNrU0sAAAAWAAABeGNhRVMAAAAWAAABeGhlSUwAAAAWAAABeHB0QlIAAAA";
    os << "WAAABeGl0SVQAAAAWAAABeGh1SFUAAAAWAAABeHVrVUEAAAAWAAABeGtvS1IAAAAWAAABeG5iTk8AAA";
    os << "AWAAABeGNzQ1oAAAAWAAABeHpoVFcAAAAWAAABeGRlREUAAAAWAAABeHJvUk8AAAAWAAABeHN2U0UAA";
    os << "AAWAAABeHpoQ04AAAAWAAABeGphSlAAAAAWAAABeGFyAAAAAAAWAAABeGVsR1IAAAAWAAABeHB0UFQA";
    os << "AAAWAAABeG5sTkwAAAAWAAABeGZyRlIAAAAWAAABeGVzRVMAAAAWAAABeHRoVEgAAAAWAAABeHRyVFI";
    os << "AAAAWAAABeGZpRkkAAAAWAAABeGhySFIAAAAWAAABeHBsUEwAAAAWAAABeHJ1UlUAAAAWAAABeGVuVV";
    os << "MAAAAWAAABeGRhREsAAAAWAAABeABBAGMAZQByACAASAAyADcANABIAEwAAHRleHQAAAAAQ29weXJpZ";
    os << "2h0IEFwcGxlLCBJbmMuLCAyMDEyAFhZWiAAAAAAAADz2AABAAAAARYIWFlaIAAAAAAAAGwPAAA4qQAA";
    os << "ApdYWVogAAAAAAAAYjYAALdyAAAR/1hZWiAAAAAAAAAokQAAD+UAAL6XY3VydgAAAAAAAAQAAAAABQA";
    os << "KAA8AFAAZAB4AIwAoAC0AMgA2ADsAQABFAEoATwBUAFkAXgBjAGgAbQByAHcAfACBAIYAiwCQAJUAmg";
    os << "CfAKMAqACtALIAtwC8AMEAxgDLANAA1QDbAOAA5QDrAPAA9gD7AQEBBwENARMBGQEfASUBKwEyATgBP";
    os << "gFFAUwBUgFZAWABZwFuAXUBfAGDAYsBkgGaAaEBqQGxAbkBwQHJAdEB2QHhAekB8gH6AgMCDAIUAh0C";
    os << "JgIvAjgCQQJLAlQCXQJnAnECegKEAo4CmAKiAqwCtgLBAssC1QLgAusC9QMAAwsDFgMhAy0DOANDA08";
    os << "DWgNmA3IDfgOKA5YDogOuA7oDxwPTA+AD7AP5BAYEEwQgBC0EOwRIBFUEYwRxBH4EjASaBKgEtgTEBN";
    os << "ME4QTwBP4FDQUcBSsFOgVJBVgFZwV3BYYFlgWmBbUFxQXVBeUF9gYGBhYGJwY3BkgGWQZqBnsGjAadB";
    os << "q8GwAbRBuMG9QcHBxkHKwc9B08HYQd0B4YHmQesB78H0gflB/gICwgfCDIIRghaCG4IggiWCKoIvgjS";
    os << "COcI+wkQCSUJOglPCWQJeQmPCaQJugnPCeUJ+woRCicKPQpUCmoKgQqYCq4KxQrcCvMLCwsiCzkLUQt";
    os << "pC4ALmAuwC8gL4Qv5DBIMKgxDDFwMdQyODKcMwAzZDPMNDQ0mDUANWg10DY4NqQ3DDd4N+A4TDi4OSQ";
    os << "5kDn8Omw62DtIO7g8JDyUPQQ9eD3oPlg+zD88P7BAJECYQQxBhEH4QmxC5ENcQ9RETETERTxFtEYwRq";
    os << "hHJEegSBxImEkUSZBKEEqMSwxLjEwMTIxNDE2MTgxOkE8UT5RQGFCcUSRRqFIsUrRTOFPAVEhU0FVYV";
    os << "eBWbFb0V4BYDFiYWSRZsFo8WshbWFvoXHRdBF2UXiReuF9IX9xgbGEAYZRiKGK8Y1Rj6GSAZRRlrGZE";
    os << "ZtxndGgQaKhpRGncanhrFGuwbFBs7G2MbihuyG9ocAhwqHFIcexyjHMwc9R0eHUcdcB2ZHcMd7B4WHk";
    os << "Aeah6UHr4e6R8THz4faR+UH78f6iAVIEEgbCCYIMQg8CEcIUghdSGhIc4h+yInIlUigiKvIt0jCiM4I";
    os << "2YjlCPCI/AkHyRNJHwkqyTaJQklOCVoJZclxyX3JicmVyaHJrcm6CcYJ0kneierJ9woDSg/KHEooijU";
    os << "KQYpOClrKZ0p0CoCKjUqaCqbKs8rAis2K2krnSvRLAUsOSxuLKIs1y0MLUEtdi2rLeEuFi5MLoIuty7";
    os << "uLyQvWi+RL8cv/jA1MGwwpDDbMRIxSjGCMbox8jIqMmMymzLUMw0zRjN/M7gz8TQrNGU0njTYNRM1TT";
    os << "WHNcI1/TY3NnI2rjbpNyQ3YDecN9c4FDhQOIw4yDkFOUI5fzm8Ofk6Njp0OrI67zstO2s7qjvoPCc8Z";
    os << "TykPOM9Ij1hPaE94D4gPmA+oD7gPyE/YT+iP+JAI0BkQKZA50EpQWpBrEHuQjBCckK1QvdDOkN9Q8BE";
    os << "A0RHRIpEzkUSRVVFmkXeRiJGZ0arRvBHNUd7R8BIBUhLSJFI10kdSWNJqUnwSjdKfUrESwxLU0uaS+J";
    os << "MKkxyTLpNAk1KTZNN3E4lTm5Ot08AT0lPk0/dUCdQcVC7UQZRUFGbUeZSMVJ8UsdTE1NfU6pT9lRCVI";
    os << "9U21UoVXVVwlYPVlxWqVb3V0RXklfgWC9YfVjLWRpZaVm4WgdaVlqmWvVbRVuVW+VcNVyGXNZdJ114X";
    os << "cleGl5sXr1fD19hX7NgBWBXYKpg/GFPYaJh9WJJYpxi8GNDY5dj62RAZJRk6WU9ZZJl52Y9ZpJm6Gc9";
    os << "Z5Nn6Wg/aJZo7GlDaZpp8WpIap9q92tPa6dr/2xXbK9tCG1gbbluEm5rbsRvHm94b9FwK3CGcOBxOnG";
    os << "VcfByS3KmcwFzXXO4dBR0cHTMdSh1hXXhdj52m3b4d1Z3s3gReG54zHkqeYl553pGeqV7BHtje8J8IX";
    os << "yBfOF9QX2hfgF+Yn7CfyN/hH/lgEeAqIEKgWuBzYIwgpKC9INXg7qEHYSAhOOFR4Wrhg6GcobXhzuHn";
    os << "4gEiGmIzokziZmJ/opkisqLMIuWi/yMY4zKjTGNmI3/jmaOzo82j56QBpBukNaRP5GokhGSepLjk02T";
    os << "tpQglIqU9JVflcmWNJaflwqXdZfgmEyYuJkkmZCZ/JpomtWbQpuvnByciZz3nWSd0p5Anq6fHZ+Ln/q";
    os << "gaaDYoUehtqImopajBqN2o+akVqTHpTilqaYapoum/adup+CoUqjEqTepqaocqo+rAqt1q+msXKzQrU";
    os << "StuK4trqGvFq+LsACwdbDqsWCx1rJLssKzOLOutCW0nLUTtYq2AbZ5tvC3aLfguFm40blKucK6O7q1u";
    os << "y67p7whvJu9Fb2Pvgq+hL7/v3q/9cBwwOzBZ8Hjwl/C28NYw9TEUcTOxUvFyMZGxsPHQce/yD3IvMk6";
    os << "ybnKOMq3yzbLtsw1zLXNNc21zjbOts83z7jQOdC60TzRvtI/0sHTRNPG1EnUy9VO1dHWVdbY11zX4Nh";
    os << "k2OjZbNnx2nba+9uA3AXcit0Q3ZbeHN6i3ynfr+A24L3hROHM4lPi2+Nj4+vkc+T85YTmDeaW5x/nqe";
    os << "gy6LzpRunQ6lvq5etw6/vshu0R7ZzuKO6070DvzPBY8OXxcvH/8ozzGfOn9DT0wvVQ9d72bfb794r4G";
    os << "fio+Tj5x/pX+uf7d/wH/Jj9Kf26/kv+3P9t//9wYXJhAAAAAAADAAAAAmZmAADypwAADVkAABPQAAAK";
    os << "DnZjZ3QAAAAAAAAAAQABAAAAAAAAAAEAAAABAAAAAAAAAAEAAAABAAAAAAAAAAEAAG5kaW4AAAAAAAA";
    os << "ANgAAo4AAAFbAAABPAAAAnoAAACgAAAAPAAAAUEAAAFRAAAIzMwACMzMAAjMzAAAAAAAAAABzZjMyAA";
    os << "AAAAABC7cAAAWW///zVwAABykAAP3X///7t////aYAAAPaAADA9m1tb2QAAAAAAAAEcgAAAmQUAFxfy";
    os << "qwIgAAAAAAAAAAAAAAAAAAAAAD/4QDKRXhpZgAATU0AKgAAAAgABwESAAMAAAABAAEAAAEaAAUAAAAB";
    os << "AAAAYgEbAAUAAAABAAAAagEoAAMAAAABAAIAAAExAAIAAAARAAAAcgEyAAIAAAAUAAAAhIdpAAQAAAA";
    os << "BAAAAmAAAAAAAAABIAAAAAQAAAEgAAAABUGl4ZWxtYXRvciAyLjAuNQAAMjAxMjowNzoyMiAxOTowNz";
    os << "o5NAAAA6ABAAMAAAABAAEAAKACAAQAAAABAAAEsKADAAQAAAABAAABKQAAAAD/4QF4aHR0cDovL25zL";
    os << "mFkb2JlLmNvbS94YXAvMS4wLwA8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4Onht";
    os << "cHRrPSJYTVAgQ29yZSA0LjQuMCI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5";
    os << "vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOm";
    os << "Fib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvM";
    os << "S4xLyI+CiAgICAgICAgIDxkYzpzdWJqZWN0PgogICAgICAgICAgICA8cmRmOkJhZy8+CiAgICAgICAg";
    os << "IDwvZGM6c3ViamVjdD4KICAgICAgPC9yZGY6RGVzY3JpcHRpb24+CiAgIDwvcmRmOlJERj4KPC94Onh";
    os << "tcG1ldGE+CgD/2wBDAAYEBQYFBAYGBQYHBwYIChAKCgkJChQODwwQFxQYGBcUFhYaHSUfGhsjHBYWIC";
    os << "wgIyYnKSopGR8tMC0oMCUoKSj/2wBDAQcHBwoIChMKChMoGhYaKCgoKCgoKCgoKCgoKCgoKCgoKCgoK";
    os << "CgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCj/wAARCAEpBLADASIAAhEBAxEB/8QAHwAAAQUBAQEB";
    os << "AQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJ";
    os << "xFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZW";
    os << "ZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1";
    os << "NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL";
    os << "/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChY";
    os << "kNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiI";
    os << "mKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09";
    os << "fb3+Pn6/9oADAMBAAIRAxEAPwD52wBxTW4BPtTw25CTj8KikycVzI62BPI46HNGetCk7cUuMg0CHAYX";
    os << "pSHtxUmcgYph7UIGNbihhkfSlakNMkchxj6VJgbeaiWpFfAx2oZSG4zmkI+YU896YeCKBMa33j9aUcm";
    os << "hvvmlXrxQIRe9Ieppyd6b60wGnp9aaODSn0pBTJHkcn0peMgd6a3ByKUnmkMP60q9Rk03I4pV46mgB3";
    os << "IoPTPpSHoc80hOFNAxxGepprfdBFBPH1prcAUCAdcdqdnqB1qEuSfQU5XOOelOwidRlaixhDUseSKac";
    os << "7CD0pFEBqSM/u85qN+KcP8AVj61RKHE5JoTBpozTkJFIYZweKU880AZ5obgYFACDtStwM96MdKa55xT";
    os << "EOQ5FKetMBIPSlQ80AKvSk7kUL0Ipe5oATODUnHJ9abxxRuJJxQCE5JpNuQKUn0oJwopMaIyvIpw9qB";
    os << "1FIMknFMQ/HOaaeR70p4puefamAN1PvUbjPenk5qN+nFCEJj3pQeTSfw0meaYhR1zTScULyaGoAUc5p";
    os << "6HpUS9alHHahgiRv4aRz81Ix6U1up+lIY5etBHzGmp94fWnZ5NAg6EZ6VJtG0YqMHkU/dkCgYw85pg4";
    os << "apG/pTMfNQA4H0o9aavFOU5HvQIWkFBpKBscaKOnFJntQA7+EjNJ2xSbsLml7UwEUfOKGHzmhetK/Wg";
    os << "A7Yp38OM0g6Um7K5pABoHFJnNL14pgIe9Kab0pw5oAPSkb3oY4FNbmgQHlqeOMUzHzU8f0oGP2jac0z";
    os << "GScdKfuwKjJ5NIBQORSN1ozyKa/3jQIenLUqn71MX7w+lKp6n1oGhH71G3GKeeaibrTEOBzSnrmkWhu";
    os << "DQA8nkU3HvRnml/hoAVBjvUi9R7VGnTmpAcUhijpTsc5pmeR6U4ZNAwPNNC8mlOQRmlPU0gALwaXkUo";
    os << "OVOaQH1oQ2PwOvNMzkmnbiCM0EYzigTG9xQ3SjuKG6AUCFHWh+AaRzzSEknpQMVeRmkPehDzinetACD";
    os << "gZpM5PNOHIwaCMc0gQj4oBwaRzmkoGLIf3ec0wUrf6s/WmpzTESYyoqZhheKYAdgA6U6TgUhjM+vWmn";
    os << "rjtTWc446Ugcg+oosImX7pzSgY6GmLyDTgfl+lIYo6A+tKcnvTeqilHQY4oGI3U80f0pG56GjI5oEO7";
    os << "kd6bjkelAPJoXk80ANNKOlIaUdqYhw6inN/DSdxSv/AA0igPBNIv3h9aVvvc+lC/fFACgfMaXAGKQck";
    os << "08dqQ0LgAcVE5znHpUjPkY7VGw4NCGxqjH40o5PSkHFKnNBKAHr9acy5WkHen5wDmkykRZ6UmeSfWnY";
    os << "wB3prE7cUAOXkA+1PwDwaijyM1KW2oCMfjQxoaF+Xik2/dJ59qcrfLRzkHHagAC8DpTjjFNc4xSg5Ga";
    os << "BCA9aQjgUp64obpQIa/GaXjNNl4P4UIcNz0piHYx+VJ2qQrkcdcUxxhM96B2HKcqc1EzelPH3SO5qPG";
    os << "DQA1v9ZUikg/SkI/eU4DmmJCRNkvmlHWiJQN9Ljk0gIm+8aQU5+tNFMQsnQ0fxA0jHINNB5piuP6mlH";
    os << "Wmg80oNIY8n1prEbaGPFNJ4oAVjyoobp1pq9TQTxTAiyCfc1Ioynqaj24PSp4BlCO+aGSiWMggDOOKA";
    os << "BsOeQKWPCqwIPTsKZlRAeep6UiyCUYXn8KX+FabKc4ApZDtiUiqJHL96nqM1DG4ONwx9KsRlWYAEUmC";
    os << "FH6UhFSkdcdKiYUhsYzYANGM4PrSNzxT9oGOKYhtNUkZNTY7e1NCjHSgBqng04D9aaw24p69OKAEPBF";
    os << "PXC5460jZHJoLKPvECgERvgHjvRn5RTJJAMlRmljbdHnvmnYLiqMuMetKBzxwM4zSRsA+DSJjB570CH";
    os << "khSRnNMxj2NKTnAHakmwAPc0AIOhqNqeajPUU0AZxTQfmNDGmE8mmIkTHFI1IpxinYoAROoqTvUS9ak";
    os << "7igEOY/dobrTX6rStzSGIv3hS560g4ajvTAd6UtIThRTkHy0AITTf4qcw4pBwaAEHTmkXrThQo4JoEK";
    os << "aBQe1FIBG6Ug4oPNHrimAo56048ikUZ6048CgY1eCKVupoHUUHqaAAHFIeOlPHI5pjDHSgBp5pymm+l";
    os << "A45oEOIoWlFIOpoAa1BpzjgGg8UAN/ipynrQ3JpVGBQMSk9ac4+Wmg5U0AGelI33qO9Dct7UCFXg0Kf";
    os << "vUgoTq1Axe9Rv1NPzyajbrQJiilfvSU1uQaAHZ+YU7OaiB+YU9aBDlqTsKiHU1KPWkMXGfc0/IYgZxT";
    os << "IcEH604HGQe9IYEc88jOKGGHOfWkkxgc96JGBfAoAdn5TQmCaZI22PPfNJHIDjcPyoHcsNhscdKaOTS";
    os << "hlP3TQuT0pAIRTGPFPYcc01RnNADWJODTqcVGOlOx29qLgRY6n0pVbIJp20HPFMXjigB4px/SmqKlA6";
    os << "Z6UikQMMU09akkKqxBIqCRwM7Rn600SxxPytSRDKnH40RndGxJpsRxwaYFsj5BjgetEhABGc8Uz5TAO";
    os << "eh6U+XDBQAenWpKK7DCe9R5wfep5xhAO+agxmmiGTr0zmkU8sDTQePrQ3UUFD1I204e1Rg8U9TxSAO9";
    os << "NPWlJ5ppPNMBT94mlTpTCeTSqcAUCQppV6ikPNKnWgZIetJM2CmKdjkUkq52CkMaxJPWmL/rKkK801R";
    os << "+8FAMVW9alY4AxUGOakPKgdxQMM8UYzx7UqD93nvTguBz1pANwM0idqa5y3tSxcmmIco4NKT0oXpQOu";
    os << "KkYo6UhXg9KU8DNNQ7s5oGJt+8Rx7UFRt5p3OScdqRm+WgYMNoz2pVbdilk5GKijO18dqNxPcdICW4F";
    os << "OQYXBp5HPHpTQMGi4W1Gt96l96RvvUZ4oENl5GaTFPIyPamEYOKaEPjPapZMBDkelQxg5p8pOCKCuhA";
    os << "D8xFLjmmsMMKf3pkgfv1KAKhPLinZIpDQ9B8zUnrSqQSfWjoSPWgCOQcimfxVK/rTCKZLI+uaYKd3NI";
    os << "vWmSKOtOpop34UDQpxTDTj0qNiaSGwDcn1qSNc5zTY1x161IrbUJxmmxIWTaNqnqe/pShdhyvWoH5NS";
    os << "xzDo/OO9Kw+pL5mRgjbx1ppVBHxinoYynLLiqzbB0JI9KENjXG85A4FEnzbcdBTj93A4FIeQABTJI9p";
    os << "A4HFKoIYEg1My4WhGQn7wp3FYj8xlHBNOE55yAak2Kw7GomQKT/AHfX0o0G7i+aMDK8/Wn+cmRw1RmE";
    os << "DGDxSmI9jRoIkM6Z7/lQJkx0aovKOfvUohIHWjQLitKCCAp/OkErY6CnCHI60zYD06D9aNAGSSswOSa";
    os << "FyT0NP2j2FPVkx94UwRXdSDz0p8Z2Hn7pNSOuc1H0wDQA9MLMCeh705FUk5I61GjYbB5HpSxhCSCSBS";
    os << "BDy/zEKNwHeoyC5yevapSUA4YCoJJccL+dCBkisGyvcVE/FRgkcinbt681VhXGtTDTyOKaaBC8cU8c1";
    os << "GOSKlHWgBqj5jTu4pBwaXPSgaBuStLSHqKWgAH3qO9IPvU5euaADrx71Kn3RUK8salXoKBoQ0lB9qBy";
    os << "aQABS9qUdKQnFACUjnApRTWOTTEJThwKTHrSmgB605ulMTpT36UhoYOooPU0DrQ3WmA9fu01qcnSmP0";
    os << "pAIeabSr1ox6UxCpyKU01Tg0rdKAHU1h+VKDSmkMaaUUh4NKPemIc33TUXQ4qR+hqJuCKBsXvQ33qD1";
    os << "pD96gQtNXq1LSDgmgBe5prDmlz1pDyaAFNM45p561GepoEIKcKaOtOHSgCVOalZgu1e5qDfsXjmmkk8";
    os << "mlYZOAUOV6nrUgf5gCNoPeoI5c8N+dWMoQMsDSY0I6qDximOd0xI6CiQICACSKR2y2BwPSgGNkbf0+6";
    os << "KaikngU4c5AqVFxTAhbI7URysuME1MzLj7wpmwexoBimViOgpyygAAqfzpmwDGfun9KeYcDrS0AcZkx";
    os << "0agTpn+L8qYYSR1pvlHP3qLILknnJk8NTfNGDhefrSCI9zSCEHOScUaDHGduMAD8Kb5jMOSaFTJGPu+";
    os << "vrUuxVBzgUaArldsliQKNpxz0qZ2QH7wpVXK0XFYij+Xdn7p/ShBsOSODThwCD1oH3cHkelIZMFQx84";
    os << "p/m4GAAwA61XXYepIx2qy5jCcMoGKTKRHt8xiW60ke0llHUd/WkkmHITv3qFOD70WF1JJVxjFRluR61";
    os << "MzbowcYqORc9OtNCYA04VGpqQUmNAaaaU0hpiY096d0ApG60vcUCH96dGOaQCnp60i0O9KHHzLS5yRi";
    os << "kdgCPWkMVhUQ/1lOOW5NNHD0wYmOaQn5gKf3qMDLGgRbjwUGBzzUcrHkUsJwAKbIDk0iugzFOi4BNNA";
    os << "ycU9Rge1Nki0i/eNGeKF+9UjHOMrgdabGCG5FPNOA5GfTilcdhjNtJpoG7mmSHc+O1Sx8LinsC3Fzmo";
    os << "2XkmpBTQOlADk6EUYpF/iFGeaQDT945pvc0ueTTT1qiWO7UpAJGaTOBSk8igY8YApjmnZ4FMc9KBsjf";
    os << "iQVJ3zUcv3wakY7V5pkjT98GlNIjgkD2pWyOlIENJwSRT81GVOTxxT6YDuq0zpmnr92mmkBAepoFKw5";
    os << "/GjtVEiClB5oFL1oBCN0zSIOppSMnHalHXigBTxntUanOTSyHJwOnem9sUAKOW9qCuCcUo4FBGQeM0A";
    os << "MywGBSLn0qTbhTjnHWkV1z3pgO2kpz0p6qAvt60Fx5RwPzqIsS3r7Ug2FuH3LgdPX1p0CgLvxk1Gw4y";
    os << "acjlOvSmCepZwNmePrUT42vTt6fwg5+lM5bORgelShsVecBvSnc+nNRs+0jHWmPIxPJ/KnYVywF9qVu";
    os << "mKqK59Wp6yHHB/OnYSJuitj0pAoEaemKSOTdnPBoYlF46H9KQxRgLniomwU3YwaXevUg5+lRs5bOOBT";
    os << "SAI3xwen8qkdQVU9c1Fj0pd2MdqZIzBB47Ug3ZqQMM8j8qRnUHvQIZkkn60hBzUmATk9D2pCMdBincC";
    os << "M8UgODT2HFR0AP7U09DQDg0vY0AIOoqQnOKj71IKAEJ4pOhobpQO1ADgeafTFp460DGt96nJSHnP0oH";
    os << "agAT7xqRaZH9409T1oBCGhetDUi0hjqQ9aWm9OaBBSYoHNKaAF25x9KMc07OMUnWmMUdaVzxTaGPNIA";
    os << "XqKVuppB1pW6mmAqHikNCmigBMc8UFcZpc4peuaAI8UA/lQKQ8UCHDrTqbnJzS0AI3WlHSmt1py9BQA";
    os << "rVG/3hT3PSmyfeFAMRqReopTSCgB1MPWnGmGgBO9KDxSHvQtAhQcUw9TTzTO9AMB0FLR2ppOTigAPJp";
    os << "w5NMqRRigAC80ZIP404DPUZpcAHI7dqAGndnpS4JPPelV1J707cMjA/OkBJGoCt2xUcj54A4/nRuJz3";
    os << "puMdaBki4CbsZNTHBXPFVlcrjPIqTevUA5+lJjuSEAxt6Yp38K59KYpLjnp6etLJJtxjk4pDHryMUhX";
    os << "vioWkJHJ/Ko2c+pp2EWufxprDGdvpUKSMDwfzqRX3HnrSsNDkxsSpcDZnioeVxjkelO3rzuBz9KBojn";
    os << "UFdwGDTbd9owenr6UO5fp0pq9Min0F1JmVWHqPWmYIXjpSBiG649qlDjyuRSArtn0oyxGDT2dc96Xbl";
    os << "eePSmIaFyRSnhvagDGOMd6UnINIYjHABFSDnFQ9sU+M4OD+FAA46GlXpmlPXmkAwcdqAAnmkNO6U00g";
    os << "YGgdRQOlKBz+NMCXGcU/otMFPblKRQ3OOlMByQTTs00KfTigB4/KkH3zSqCetI7AEg+lAMXvmo05c1K";
    os << "p3LxUcX3zQBLGc+1POGWooz1p+eDSKEUAZpOxoB60Z4oEN9KePvDFMHWnZ6UCQ/HNDngCgnmkb+EUih";
    os << "qryDT84pCOtKaAIwTnmhTyKQHqO1JTsTckHU0metIDSUDuLSGkJpKYrjz0pCcUD7opp5YDtSAee1Nfn";
    os << "HtSk0hoGI3LD2pHbI5p5GMVG4xTEwAwwx6VKDkVGByMU/GB70AgNJSk8UmeKBC54ozSZpM9aBjDyaT+";
    os << "Gj1o/hpkjQwPFLnjg1GOG5paYiVRxzQx249aYGI5pNxZskYFIAJxQnqabnLe1OHSmBIuCMd6TBBIpg7";
    os << "/Wl3nuaQx+MA5qHad4AOKkLZz1NJnbksaEA4INmSaMKOnpUbHIyaUAqo9TzTEKRubApzgEgDoKD8i47";
    os << "mnxDJB9KQ0SBdoHr3qCRhkhT9aklfHHf1qD3PFCBiHnrT9qry3Wmryd1PHDZIyT0piFBH/PM49aCiso";
    os << "K/kacN394Z9KGycnGCBzSAixgHFKjZADH6UE/xUwjIyORTAkK7h70xRjOe9SRNxjGW9aSQcZ9aAGAbW";
    os << "waVtvGaPvAg9aY3zJjuKYhCoycGmhfnxSLw2al+9gjtTEB5Ge44pCCcc0hPJ7UhcnvSGK2AMd6hanHP";
    os << "emmmITrThyD60zoacKBC0/PFNPQGlFMYHgY70q9KRuvHpSjpSAdnBozSGgHigY7uaTtQDzQBmgQsfU0";
    os << "5eCaanU04UDBulNBpW6U0UASDkVGx5xTlPyUw9aAHg04cUxeTT6AQ4npmmEkHinGmMeaAHA5NK3WmA8";
    os << "4pT1NILjvSl9aaKDQMVetBOKQHkU0nmmIUcmpAetRA808d6BiHmmk08VGetAgU84p54FRjrT2Py0AIT";
    os << "Tl6Uw05elAA3JFJJyRSmmt1FAMB0o7ig0d6BBmk6mg9KQUDEbpQOnvSnpSL1oEHam96dSdAaAEPA96Z";
    os << "Smm9TTEOWp1xjHeoVpwzSGSgEd6XoCe54pgcjvSg80hjCvz4FOCjIyaeflyT3qJuWzTETrjJxSEbmwK";
    os << "avypjuad90ADrSGIw6YqQLtHvSRjjPpSyscYxg0hjWbghT9aTGQM00DAyelPB/ioAeEVVy35CkJGP9W";
    os << "cetOXIwcZJFB3f3hn0pAM2q3K0wcU88tkDBHWmNwd1MB8bDIDc+lTld2R3qr7irET54PWkxojQAEg9D";
    os << "TANrYPSppRgk+tMA3rg9RQAYXv6UhQbODTSCyn1FNU4GRTATad5Gc1PjIGOaZnccqaUPjHUUhC4JIBp";
    os << "WwBjv/KmbyTgGkPagYj+ooHNKeRTejcUxEqnP1pWHHvUW4q2QMinFi3NIBc8cnmkLDpTTSdW4oAl/ho";
    os << "HBo/ho64oGS5pc8UzPSlz1pFCmgUmeKUHigBS2BURGWOfSpMZHvTCOTmgAVsDinLwx96YozipcZzQCG";
    os << "ocZpwPWmilBoGKDmgdOaaOGIp2floEIKXNN+tAoAfnpTj1BqOnE0hgx5NBJzxTaCeg7UWFcaKXvTc4N";
    os << "OpiFBopF7UtAxDTTTqRqBCqeKVRk0idDT1/pSGhmPmIoHoacB85xQvU4oAG+8Ka46U5vvUN92gBq9ac";
    os << "1AGGyMUHgUANPQ0lKehpvWmJijoKRuppyjimuORQAw0dqD1pD93NMQ0gHvSomT14pmSTSk7RgdTTEOb";
    os << "HbpTGPOKYTjvTo/vZPJpANANPXOD608jk46U4L+NAWIgSCM08+9DfdpM5GO4oAbvOSAKaOT83Wgj5iK";
    os << "WNSxpgSRLkknoKeBk5PanYyAq9KSU4wopDI2yzZqZTtXPb+dRj7v1pHbkAdBQAhJZj3PWnbFHU5NKo2";
    os << "Jn+JqWNRzxlvU0ARkcgAECnA/eP4Ur8t1JpiHcSOtAiUYx935fWgtnafTikOcdvpTTgEDGDQMMHBGDi";
    os << "lCqRx1pyYxwxzTW9ejetAiMkoeeDTgdyj3/SmyksnuKbG2CB2NUAAlWp54II70EcUiHIKmgQx12vkdD";
    os << "Tfcdal6MVPTFRspBx2piGlzmjvSd6XocYoACSTTGB71KOnFNfoKAIjmlXpTu1IV4yKBCqeaeBUQOaep";
    os << "zx3FABwTzSjpSMDQDQMdSjpSdqcBQAtIOhp3YU3tQMVTyad2pq9aXtQAh9KO1OpG6UAN6CikNKvWgQ5";
    os << "RTqSloGIaaetOPeikAzNO+tGcmk9aYh3alpO1A6UhifSkpaM4NMQDrTl60nQ0DtQMWmsKd2pKAGUvUU";
    os << "N1pBzQIdjiihelL3oGHamt1FO7U1utAAelFJ2p3Y0ANPSk7UpHFN7UCA9KTjPFBNABoAUjimsefalc4";
    os << "47mmZoARulIBTwvGTS9qYhFBxxTwTnBoXpTjSGNPWgOc0nU9KTvQA/r160qLubnoKRVJPtUnVwo6UAK";
    os << "OSSabyzU5zgBRQBx70hjidqn2/WmglzxzmmyNkn0FOiJVPc0DJCqjryaTBwBg4pVHfq3rSvjH3jmkAB";
    os << "sbj68UHGPu/L600YJIxk9qUZx2+lIBCfun8KaBgkYJFDnBA6U9OD1xQAmxT0ODTQSrDsetSSKOOMN6i";
    os << "kYb0z/EvWgY5juXPb+VRL8rUqNyQehoI+UH0oAceDkd6ZIuDkdDT4jnKmlxgFWoArHg8dadvJOKSRSp";
    os << "pB94CmIlHtTCSScU7OBjvSr92kAxgcCmEGpyvpgUgHPPSgLDFPOKeuOh6VHIfmyODTc5pgTOmD14poA";
    os << "HekB3DB6ikyQaAJe1KKQdM0o60hj15NKelIo5NOYUhjacOgpnSnDoKAQ9aa33qVeRQRlsnFIY1B1py/";
    os << "eNCn5aVfvGgBh9KXHzAUrdRS4+cUwGsO9IelSN3+lRv296AYlKKQdaXvQIdSE0Chu9Awpp5NLSE5OKB";
    os << "CLgn5utO70qgd+tHegYAc0nanL1/Ck7UAJ3oPWiigQoGAaUEUAfJmmnrSGPHXijHyn60qj5aXHFAxjf";
    os << "eoPK0OPm4oP3cUCFximmn01hxQMYe9JinHpR2pk2Cmuead6UhGTQMbTH+7xTjxmmE8U0Sxo9KTa2eRz";
    os << "TkHNTL7jii4iqVPXqKVTlhVl0XaWXpVfG1ge2eaLgKp+cn0PSrGQVDZGe1VAdr1MkiY5z+VDQ0EowpO";
    os << "eKhVvmJqZz5ijjCj9aYBzjGM0IGLtGSxOacD6Co0Bb6ZqcAKMmgEOA2LnvUYG5snvTiS3Pagnb16d6B";
    os << "iSHaOPwpgGRmhzubinbgCFoESYwy/TikU/K315pfvIAPvLUeSWJU4buDSAeOVOOtNGAwxyOg9jR/Fkg";
    os << "g+1M3Abuo560wZKQPxpVAySenekXfjgjHrQSCo6nnOaAEIwnPXPFI33fxpy8lsAk+9NJ24JOW7AUCEI";
    os << "Bc59OaiIwoqUDAOfvGmEgnb2qkA5ORzTWGDxQDtf2pzcgHt2oENPzZPtSZ6A0gyHPpUmA1AEJUZz0ph";
    os << "OGzUjKQfahhyeM0xDlwVBpsgGPejOzn+E013Ujv+VIYwnGPegnAoJyRQBkE0xDQOaVN27IFOC9CelSL";
    os << "7DigSEPOcikwMc0489aQmgYgHanDikooAfTe1KDQvQ0DEXqadSJ1pwoBCZpCcilbpTRQAh6UL1pcfLS";
    os << "L1oESClNIKWgYh5paU9aTvQAhAzxSDqadgZ4pMYJpALRRS9KAG96UAZ5pAMkU7Azz1pgJQBzR3pQOTQ";
    os << "AgpKUUhoAY1A6UHrSj7tAhQeKKaacvIoGLTW604jimt1FAMD0paQ9BQTQAh5pD6UtIeaBCY44pynpgU";
    os << "DpQOOlAET53cim1Oec5HFMZepHSgBAcigc59qCMAGkB2k0ASpjGe9ObAUmmK6gd/wAqXO/2UUhjAcsa";
    os << "cFG7PWnKOQMYpFUk+1MQ7PJApR8uCfSnYCjJqNslx6UhjlG4805+BQvAPp3ppO5vagBAMqakAAcfTim";
    os << "ggHb2p55UYPzCkMVfu/jSgZTjrk5pud2SOG7g05sArwQfakA5gNwx07UgA/GkBAU9RznNDb8dR65oGN";
    os << "OC3PTofrTjwoz1qPcDt6nnrTs5bgEn3oBDmPyr9eKdjLt9OaiyQwLct2A7VKPlQg/eNAIhIwM06M7hz";
    os << "+NJuBJWmodrc0API2tnuKkI3rnvTQd2cdO1AJXkdKBkZJxyKbtGQwOKlYBlyKhcFfpnFAmNZvmyamiG";
    os << "VBpmOemach8tTxlTQxIlyApbIz3qBj849z0pzyIRxn8qiJJehIbYrHDGkCnr0FOxls9s1OqLtDNRcRX";
    os << "2nPyilqw3sOKhcZNCYWFT7vNPqMHing5xQND16071poGDTvWkUNPWlFHUUo6UCFA4pcDikUcU6kUNHC";
    os << "0q/epB0xSoOaBC4+UfWg9adjIpGHymgY0kGmkcCkHWnHlKYhBSd+KWgUAB6UEc0U5uo+lADO9I+Aflp";
    os << "x60MB260AIAOp60o60h5pV60APHf6Uyn9z9KYKQwNNFOb9aTHrTJHfw0hFKegpR1FIY5BhaC1OA4Aps";
    os << "g+YYpFdBknBFIMkU6TqB7UAfKaZPUO4oNKPvD6UpGBQMjbpR2pTSe1AhB0ox1pyjik6UAV275NNzxQ/";
    os << "LGkPOB71ZAoOAPU1KmAe9RhRuA6UpG0470hofkqAScjNIQMn0NEo+QEH8Kb0ApDZGQc59KRFJ5qQNl2";
    os << "xxUiLxxTFYbG3BVqCDnHanFc9qYwweM0AKMLSn5sZ4FIqHPSpFA6mgAA9evpTZjjCjr3qSQhV3d6rn5";
    os << "nyaENiDipFXoQM0bMHJ/KntJyFQc0CQFGxkcVESMneMH1FWTu/vDPpULANu3dR196ENjOQRhsrS4HOf";
    os << "WkHK47il3dwPYimSPCe520cAAe9NXy8Z3fhTt38RGOMAUgEGW3ZYhRSAgD5Bk+tL92Mj1pwwkYwMk0D";
    os << "GIjHlsc0roMdMc9acmTxvANKX6qw5pgVyM+5pynIC/lSleTt/KmHgqRTJHEfMaQfL9Kcp3Env3pcZPF";
    os << "AWI87gaQkg9aV15PHeo2HODmgAdgRgUzHHNPwBSkUxEeKeAMfSkY/MKUfcNAARu6dKkIHGKjjHU/pTw";
    os << "M9uaQ0IeQfamjpSt3ApB0piF7UvakHWnEUDE6E0g6U7ufpSAcUAKnJNOFJH1NKnU0AgbpTAKkboabQD";
    os << "G/w0ijJp4HHNA60AKKWgUUgFI6UnenZ5FIetAw4696RqWkbrQIUCgigdqD3oGIvWlOOvehetLQIb3pw";
    os << "HJpB1p2etMBlIadSGgCMjml/hp3egjigBpFOUcUlPXpQAhpj8EU9+opsnUUANPIpepFGOKB1H0oATtR";
    os << "2p2KYaAA9KUcAe9IelKvbNAhwA5zTANvWnkY+tMkHQ/pSGwIGKZjj6U8/dFIp+Y0xDMelSRtxg0Y4oI";
    os << "BoAcpJNOzgColHPGalReRx3pDBvm+lOVfmFOwAeaRjtIPftQAjHAK/nTANvsaUcliaeF5G78qAFRRjp";
    os << "nnrQ6MOVxUgfoqjmmvkcFhmkUMOCPnGD6045XbhiVNO4eNsjBFNzujAPakAvBBHvQye52/pSbv4gM9i";
    os << "Ka2zGdx+lAgwOCPWk5LHLfLRu7kewpDwuPWmAoI3DZyfWpAjYyeaao2hdvU9KmG7+8M+lJjRCV6kjFR";
    os << "HmrCyckOOaj2Z5FAMWE5yp69qeRx7+lQg7XyOKnjIYBu/ehjREMr05FIcNUjAHkcVGyHPSgQAHOO1Ej";
    os << "cBVpFGTzmpNoHQUAV3UjBpQDn69KmdeOaYWw696LhYeAMj0FLksDg4GaZ1BpYh8hJPfpSBCvgnv+NRE";
    os << "5B9RTwNx96RgNxA5poGNzxT17YNMHpSpwwpiLBGMUp6UvXmlYfLUlje1KvSkx2pQOKAFHFL3P0petIf";
    os << "vH6UhjTkCljPJpSPlFJH1INAhyt1pXGVojHzHNOYcEUFdCEClP3aO5pAODQSNNLSYpw4+tMBKeR0phq";
    os << "TuPpSAjPWg4PI60N1pOlMBR6UDrR05pqnJoBkvc/Sm04Hg59KZSGH1pO9LSgUCFboKD2NDZwKO4oGSD";
    os << "tQ/bNAobGKRRHJzIKXPBpG+8DQxpkiKfnAp7daYv3qeaAQw0nenAZJpQOTQFgAwlMY8VIfu1E3ShAyu";
    os << "eSaAOM0DqaeBwAKogTd6jNMDY7U/GDj0oA5oACSwxjims2NoFThQAc1UdssTQgYZO8kVNE2e/wCBquD";
    os << "ySRUsRXHIpsSLWwge9RnIyTQGXPUikZhzhjUotjiTjNJuA6c00bSMkk/jTlZVHAosIRgzDJpQAp4od8";
    os << "pxTAM9elMTElYtwOacg2sMnkilbhQFx9aVVJUBsZHQigB4wqgEHOaYcqWJPWnP5gxyDTNhJO48egoGy";
    os << "JcqCwHHpTvvHcnX0oYPu4HA7UFcnpg+1MkcCcj92M0Nx80h57Ck+YcbzikK4HQk0AByQXPSpT8yqR2q";
    os << "NN+TwMY9aUqU+6fwNAC46jByaJBubAPIFMy7HGQKft2xkL1PUmgZGjFWwRUjYYLTcdQ2PrTSMbcGmIT";
    os << "BDEinIwJGeDSBsHmlVgTyKBDj1JqNqU7QeCRTGIx940IGAUmkPAxnNGRjnNBK44pgMP3qcDwRSOcngU";
    os << "3PNAh6naemRTi2e2KMc8U8rmgaIyeOBTQMipigIpm3BwaAEA7VIPemkYIp3figBrdTQKCMsacBQA2P7";
    os << "xp601QNxqRR3oBCEelIB1px6UgpDE7U3vTh0prDvTEOFBoFFAx3pSUp6D6U3NAC96VxzSDnmnuOKQDB";
    os << "1FK3U0g6ilPU0ACDminJ0zTTxzQAlO9fpTM08dDTAaKQ0tIe9AhvendqQDvTj0oARhmlUetB604UDGv";
    os << "TH+8Klcd6iYDcKBMDSDqKeRTQORQAtMIzT+/NIBkmgCM8ClB45FOK5OBTggAoAYGx2zTWO4+gFShcUz";
    os << "HJzQA0ngCmj71GeaVDg8igQ4cjrj604gikBFJkY60hj1qTuPrUKkY+8aeNpPJJoBDnYA8cmmYJYE09m";
    os << "UHims2TxQBIuFDVG7FmwKUDO7Jp2OgXFADo12nB6kUmOg7ilC7owG6joRTMupxkGkUSj5VYnvUIyAHH";
    os << "SnYL/eP4Uj78jgYxQIVefmQ89xS5OT+7GaaFyOmDS/OeN5xQAnQ7n6+lNbLDcR+FKFweBk+9Kofccjg";
    os << "9qAJBlipB6U44KkAHOaj2EEbTx6U9PMOeQKRSGONzHB5ApsTFeDxUjLhSF6nqTTV5UhsfWgQpAY80KG";
    os << "UZHNMIxjB4p6PheaAQbgevBpcnGaGKsORTSFHIJH40hjhk4NP2Eg+tRKw4yxpxZc9SaBpkcrY759hUO";
    os << "TvBNSSlcDA61ETyDg1SIZKp+8DTlJUYxxUSNhgSKtlQQKTGiuW9qdu9BSkc0mMtg96AGleM0o608jgg";
    os << "0w9RQBYQ5FSEZSo06VKPu0mWiMjnFAp5GSKQ8MKQWFXrzTWPzkU4U1vvUAxewojOHNCmhfvGgCRO+KQ";
    os << "9DSrjHFIe9IoYO5oXoaO5oHSmSM6Gl9KCO9FMQU/uPpUdPY8DHpSAYepoPpSE4NL15pgI/wB2kSnHkY";
    os << "pqjBoBjzSd6cBkH6U2kAGlHvSUoPagBT0pe4pD0o7ikMetK/Q0gpW6GgojP3hmg9aGHzAGhqZIf8tKU";
    os << "01fvCnNQCBOrUozk00cE04Hk0DGP1pr/dNStjbmom6UCZCvU0q53UwfeNSA9DVEDP4uelSjHGKj4J46";
    os << "GgHmgELOflwSQPaq3HPJq9wwOeneqTrhiKEDEXHfNTpjHHSoAD0zUsS5BBamxIlOcDgYoI+XpS7ACMk";
    os << "01gvPJpFCj7pwO1NUDHNKu3GN2KcEBXjBoAjO3b1/WjjHWnuhC8U0H1oENbHHNOQjPU/nSuOAyjNPRh";
    os << "tDMAPSgERueB8x/Omg8H5j1qZyxC4SmhxkhhigGR56cmm5Oepp7MwbAAoLYPzEZpiIyT6t+VLk8/M1P";
    os << "3g84NLuyPlI/GgBmeepoY/L1NOQsWIwKczbuFGaBWIFPI5NS5GOp/Ok5Xll4qQkFNy84pjRE2N3X9aD";
    os << "jC808c5JGKaT0wM0CGnGev60igZpwUk80qIO+KAAgYpmBjinMF6Z70xgo6E0AxR34o9KaAMfepduO/N";
    os << "MAeo2xnqac+QeuaaRk0CHr0GCalFM6EY6U8kCkNCikPUUFgBTd2SKAHnqKKaTkil70DD+Kl700nDGnA";
    os << "0xCdzUidBUQPzGpF9KQ0I1JTm6U0UAFBoHSmk9qBCrSmgUUwFprdaeegx6UwikNjo+lPb7tMXinv0oA";
    os << "YOooPU0DqKD1NMB6fdpsnSnJ0pjc0gGr1p+aYBTx0OfSmCEBprdKWg0CAdKWmqe1KaQC0q0hpV6UDHP";
    os << "0NRHqKe3pUZPzCgGPzzSfxUE00HLCmIfSDqaTqfakBwTSGKOppSaZuwaeGBFAhGqNuh5NSAg0zrnPSm";
    os << "DI1xmnpj3qMDBpyZPegRJ60h+lJtz35oIGOtAx3GOacMVGoUjkmnqFzjNIEI2M0DGev6050HakKkHig";
    os << "A4w3NKpG7r+tAPXIpx4wQM/jQAmeOp/OomPJ5NWAQE3NxmozluVXigbEU/L1NBPPBNSK23hhimuWDAY";
    os << "FIQzceOWoyfVvyqTdgfMR+FJuA7GgYzcc9TTs+5pQ24/KRmhWYtggUABPA+Y9aVD1+Y04uMgKM05CwB";
    os << "ylIaI3Iz1P501cc81MzDaWUAjvTEHBZhigBOMdaBt29f1oJ9KciErzQAjAY4px+6MigoAvOBTW24xuz";
    os << "QMcANvSkGeeOKRQpxyadtBzhjQAx8Y56VAxHYmpJVwAA1REHpmmiWLxx1qzAfl4JI96rouWAq5woGOl";
    os << "JjiNOOc1Geox0oJ55o4B56CgGKx+akfqKcT1NRnqKALC9BT0pi9KlXG2kWgJORih+q0E4IpDyRSAWm/";
    os << "wDLSlFNb71AMcKQfeOKFoX7xFAEqH5RSMaF6UGkUM70DpSdzQOlMkD7U0UpPakoEB604U2nHgDHpQAx";
    os << "6VD8vFNYZNOHAxTBBQKTpSr1oAf3P0ptO7n6UwUhhR3oNJ1oEPY8Cj0pD0FL3FAx46ChqVT0NNkPzDB";
    os << "pFdBsnDig8g0SfeB9qAeDTJ6iL94HtTm4NIPvD6UpOeaAQ00A4NBpPegQ8HKVG3Q05T8uKTrQMrHgmg";
    os << "HjFI/DEUh4x9aozHhTjkgU0Bj3pwI3DNKeTkce1AwO5RzjHrTHXkEU+UjYB3pOoGKQMhx85xzUsS+vN";
    os << "IFxIe9Sowx602CJd3HAAphJOR0pN3vUbN83BpWKbJCDjkZoC4+6cGmhzmnrg8dDQIaznGGowG6U+Vdy";
    os << "8dRUGSrelNaiYsgKcr0pysGZeOgoLbuDTmjKsGT8qAH5LKDu+lMPzbhjp1qQ+yHNRE7d5JwTSGyIEkb";
    os << "R19adgKcAbmoUbVJpcEdOp61RI4eZxyPpmmsA3DDa1ACdNx3euaeQTwfvDoaQEeSAUPX1qXARVx1NNI";
    os << "DJu7inEbo1IPI5oGNz1OelDkIc9iOlKmMZKk5pShJLN+VAEChnbnpUrAKFNNLYPy9aYxyV71RIbjuIW";
    os << "nKvILcmlVdpPrS8A0hidMjpTGzSu3J+tRM3PJpoTY7PByM008jjijOe9LkYpgRt97BpwGcmkYfMKcp+";
    os << "Q0CEUEtx+dOYEck0kZHIPWnjjrg+1A0MYHGcg0g4+lObgnFNHSgBwPQ08VGKkJoYDW6mlFJ3/CgGgBU";
    os << "HzmnrTY+ppydTQCFJpAc0N3puaQx3am96A3HNA60CHCigUGmMd6fSkpccimnrSAXvmlc0lI3WgBR1FB";
    os << "6mgdqD3oAVDzSd80i9aWgApfX6Ug60uOTQA2g0UGmA3vTu1NPWgnjigQ4nGKUGmZpy9qABqY4+YU9+o";
    os << "psnUUAxDQvUUGk7j6UAONNJ6mnA0w9aAEJzSgHGcgUh6Uq8kZoAACehprAg8/nUh56YHtTJCOAOtAAR";
    os << "jBpq/e4pxPyCkUfMaBDhgDnmlJ44GKTNGcd6Bj1zT+uBUCtzwalRuR9aTBMVl5JXg03cdwDVJwTSMu4";
    os << "j1oGOUBgxqJgyNx0oU4J7VIGyfm60CFQhz9B0oz3z1pwQghl/KkfHUKRikULjerZ6ioskgKOvrUo+WN";
    os << "iTyeaaAFTd3NADVAXhRuanHzOeR9M0oUjgfePJNNITpuO71zSENwGOCNrU0kgbT1p+CevUdKRhuUGmB";
    os << "IPl2jHXpT8lVJ3fWowd2wg5I7VKPdDmpKRCxCs3HUU2MF+TUixlmLPTQ23gUxC4C9aVXOMLUWSzetWI";
    os << "l2rz1NDGhhXP3jk0AHHAxStgcdTTGc5pAPDEYHWnluOQDVdW+bk1Ju96Ghpkcq+nFQ/xgVZdhj0qIrm";
    os << "QdqaJYIvJJp/zMOOnrSdAadERsI70AiMgjvTypxwQaBwcnB9qQkbjigBCeMUDkim9c/WlTlgKYi0g4F";
    os << "PJwlMzilY/LioNAJyaBSZ70ooEPXrTG++T2p445pp+8fpQNgOAKI+ZDSk/KKSP7xPtQHUkTvSHoaIz8";
    os << "xyaVj1NIroRjvQp+U0dzSA8GmSJ3opOlKv60CFp3cfSmGn9x9KBjDQfWhutJ1piAYP1pR1oXB5o70gH";
    os << "jv8ASmU5etN7UABpopaOnSmIdjK5NITSj7vvTT1pDJUOVpGHShThaUnigYyT7wpASBSv97ig9KBCnqP";
    os << "pSGlApGPFAxD0o7UHkUnamIUdKTPWikJwaAIW5zxTccVKRnNMI45pkDAOBjtUqAZ60xME1MmfYUMaG/";
    os << "eAXHGaTgHHYU9ioQqDVctlto9aSATcQT70I5HFIAWfipEhBGSxFMQsa7ss3SlJwenFDAxqATkHoabk5";
    os << "5xgUDY7AY8fmKUnGM/gajjYr2yM1Nwy4oAUHOc9aZMOjDvTsEcGgjd7A9aBkPXmpVY8YOajYbW46U7b";
    os << "lgR0oEiUysB05qFhuJ3nA9Km+6obuegqI8MeNzGhAxMgkAZxS5689TinHORkgewqMrnOSevpQJkoZc4";
    os << "wMfSg84PocUKTt+78vSgqAo54z6UDEUhdynIBpAMD5DkelPXPzYYH2NMxnoNrUCFjkPcYpXf5TkjrTe";
    os << "GBOOR1phXDZ7UxiZxTlGMNTcbn9qe3AA7DpmmSIx+Y+tIDuPFNGS5qQYUCgEMwBnNNYZI44od8nHbNK";
    os << "xOTQBG64GR3pucjmpSN3A6DvTGjAHU0CG5zj2qQEYPvUR4IpQcKR+VMB+dnGO9SEDjGDUIIOAakHYcU";
    os << "AB4Bpo6U48daTt0oAOlLnikooAXqTSdqcBSL0NACx8E04U1OppwoGgbpTQaVjxTBQDF/hoVuaM/LSL1";
    os << "oESClpBS0hik9KTvSnrSd6AF4x70jdaOM8UhOTQA4GgmkooGC9aXjHvTQcEUvGeaBB3pQetJ3pR1oAS";
    os << "kNLSGmAxjzxS/w009ad/DQICacvSozT1PFA0KabJ1FONNfqKAY09Kd3FI3QUpFAhM8UnUUUUAIelOHI";
    os << "FJ+FKBnpQA4Ac5IFR53cU4+nFMJAyBQDHHGPpTM4z70E5UD86QZJ4oAM8cU5F3DJ7ULGCOpp4G3g9D3";
    os << "oAFGD04p2M4pFJyM0iNg47ZpDHk4PNOB+YetBwwNRnIcUAx7jOWpuc08cgjsetMxtf2oAmR/lGCOtJJ";
    os << "IewzUYXJz2p/CgHHJ6UigIyPnOPalYhtoGcCkxjqNzfyp7Z+XLAewpCEHGT74oZlzjHH0oCgqeeM+lD";
    os << "E7fu/L0oAaT056HFNyASDnFAXGME9fSnjOeCD7GmCGKNpGw59qmErY6c1EBlhgbWFS/eUt3HUUmNDGY";
    os << "85OKi6c0/bhiT0pqjc3PSgGPhHVj2p5OMY60gG0eoHSjBPFAxoOc4/E0mAp/rT+FWoXYntxmgQ8HJ6c";
    os << "UjrtwV6Gkyc8YwRTlBkUgHgdTQCIncnil3Eke1PeEAZDE1Ech8GmJk/BOOxpfugrjjNRBsNtPbpU6lS";
    os << "gUmpY0xjhQetRkcHPep3B6YBqF8CmgYmOKcvGOKAOKeBjFMCQnpSnpTQcmlqShe1C9KTtSjgUAOHNHc";
    os << "/SkU8U4j1pDGEkgUsecmgdM0qH5jmgQqjrSucLRnikY5U0DGg5oxhcimjrTj9ygSGGnA0nXrRTEKaee";
    os << "30pnanN1/CkMaetBwPrR3obA5pgMQfN8x5p3em4yadQIUHnNHakXtS0AJ3oPWikagBw6cUoFNXoaeKT";
    os << "GgHWjt+NJnDGhTgkUDBvvUH7vNIT81K33aBC5pppR1wBQfagBhoBoPQ0n1piHU1xzTlPFNfqKBiUx/u";
    os << "0pPNIfu4pksYPUHFJvYnmk6GlZcjIoEIX7ChRhqYeafHknmmA5B87AdSanCgLjvURODgU8HOKQ0NmIK";
    os << "Y61AoOcZqZvun060gGPm9aBMMqTjuKUAjpUROXJp0T4PPSnYLlnO9femKcHntwadnbgr0pso5DCpRQk";
    os << "oyMjtSKeMUufl+lI64II6GmIlzll9hTV+6x96QHcgI+8tOjOQSDk9waQCAbVJ/ipABv4+v1obAbuKYg";
    os << "wfpTBk3uDz6UAAkg8g8/SmHp05oYcg569QKAFY5TcetI33QR2pV247k0j9OfwFAhCQHOT1HNRscqOaW";
    os << "QFUJPU01FyRnoOtUgHx9PrSO2en4UpPFIg4JNAg+7n1xSYJOaU8uSegFRu+T7UCAkA470w5Jxmk7048";
    os << "nIpgSIMKAKSTGPekHTrTX6CkA1skj2pCOKXtSFsLjvTAaDzinRs27ApAMU9BjJoEPOe/akyMU1iaTGa";
    os << "BjwaXrzTe1PWgB1N7Uvak7UAC9TTqavWndqQIDSEYFO70jdKBjD0oTk0HtSr1piHilPSkpaBiE4paQ9";
    os << "6KQAcBuKQdTSY+aj1oEPopvalHSgYh6ilGCeab6UuOaBDqAc0lA7UDFpDS0lMCNutA6UrdaQUCHAZFK";
    os << "KF+7S96BiU1uopx6UjdaAE7U6m9qd2NADelNNONN7UCDPHWlGe3emYxSqxoAa7NuwaaTzinuM80wjNA";
    os << "hwHFC5BPvSBsrjuKXqKBkseMe9K4ypBpidDTj060ARjIbGaeCCcd6bznJpO9AE2CDml+9j1xTFbB9jT";
    os << "wNrgjoRSAVGwefxok5H0pHHAYUoPFAxqn5SM1ICC4weg4qJ1wTjoelOjBZQR1FDAlX7pJ70KcJnvQvT";
    os << "j8RQ23HcGpGOIAIA7c/WjPcnn0pijknPToKUdOnNAxCBv5+v0pxG5Qf4qicZP19KeuCe5xQCFb7qn3p";
    os << "2cM3uOaSQ4AJ4PYCmk7UJP3mpANY8Yp0QwMnvTUXJOegpSfl+tMBWOTx34FPzsT3pkQ5LGnZ3ZJ6UDI";
    os << "yCetNyoIHc02R8nimj74NOxNwYHOKnhIC46UwjPzelKv3R6UAtCUqCuO9QOPnAPXNSk4zTAcnBFJDZG";
    os << "wy1AfsRSyZB4pg4piJN7A8UH1JzSAYGTR1NAEifdp/GaYPu4pQeaQ0PQc06mp1NOY8UihCaBSH2pR0F";
    os << "Ahwp2RSDpzQeuCKQwH3aF+9Qv3aFPzUALn5aD1pGPIFKTlxQMQikPTmnN/SmP0FAmA60d6RaWmIXtQT";
    os << "zRSN3oAO9Ncc/KeadTcYNACrS96ADz6UUAKKKUUlIYhpppxpKYhV6Uqn1oH3ab0f60hjs/MaQE0EUE0";
    os << "wHN96muelKTnFRueaSBj16041GDgin5yM0AI3Q0lKRxQRxTExB0FI3WnYppGc0AMNJj5aUjrR0WmIYc";
    os << "DrTkYA9Kj6tzSj0oEOIGeOlMYHOR6U7G4YoAYNhqAIgxp6k4JowA3NOHTigBvLYB4FPNIO9GCeeaAGF";
    os << "GyTSL15qbBGeaTaG4xg07gLE3JU9KcDzg1EeB707JKgjqOKQwOVbFSgblx+RpjfMMipIjg49aGCIeVY";
    os << "np2p4KnkcNTpU/i/OoMZJBo3AezHdycilxncDTF/u08ZLcYBH60xEmDjqNtN2Y24780DH9w5pWJHXqR";
    os << "x7UhjQSAcHij5R82cmmnONtMzjgfnTsIV8v1pyjao/WliBxuH4CiQ8YoAi5Y049lFA+UEnrTWOFz3NM";
    os << "QkjZbA6Ypp/X0pFyW96lAC44yTTEQlTnmlHWnkc9aaVI55oATkGmO1OJJprUAMJNKvTNIeTThTEOXk+";
    os << "1P7U3oBSikMTjPNA5FKwAPFKBxQAdqUdKQ0oHFAIdSDoaB1oB60AC9TT+1NT7xpwpDQ00E8UrdKb1pg";
    os << "FApyj5DTD1oEPU5p1MXrT6BoQ009aeRzTGHNIBKWlA556UpHNABR2oFKaAG0lOA5pCOeOlMBB1pwpqj";
    os << "mngc0AFNY4p1MbrQAhopB1p7D5KBCdBS02nL0oGL2pjdRTzTX+8KAEPQUtIxoPWgQh6UlKRxSUAxDxS";
    os << "cZ4pxHFIoGaAFNMYYNONJ1BoAjb1oBNONMHBoESK1O5J5pgp4JoAO9IFOeKcFJ7GnAc8GgY0frTo2w2";
    os << "D0xTiA2eMEVE3DAd6AJx3U0zlTSg5Xd3FOPzAEdaQxWG5T+lNTKdKeh4xSyg43H8RSGB2n5s4NBJwMn";
    os << "iogc8H86eAcbaLAP2Z3Z7c04g46jZSKSen3gOfekOP7hz+lIY3GNoFIrHdwcClOQ3OCT+lMb+7TESHa";
    os << "OTy1N5Zgevam4AIAqaFD978qWw0BG1cfmaiGWbFTTHJx6VGvyjJoQCk84FNlbkKOlJkhST1PFNHI96E";
    os << "gGN14pQpyDUmAvGMmlwTjmncVhBTORnuKfgjnnFIe31pANYnANNLGpD05puAW4pgCg5yaeoBPPSkIYt";
    os << "haXBUYpAK7AnpTRg9KQ+lHRuKYEmOKBR1WgDpSGPXrSnoaQDGKdikMSlHQUAcUAcUAhwprdaXOBmmE5";
    os << "JoGPQ9aVfvGokPNSg4zSYIaSaUn5hSA0AUwFY+lI3Sk6v9Kcfu0gGCnCkpRTELQRzRSkUhje9IaWgg8";
    os << "elMQ4DtQBzT6Qdqm5VgA5NNxTl6saMc0XCwyg/pS96aetMQ49KQjNL2pSORQAhHSmtxipMcCmyDpQMY";
    os << "Thh701lwOaWUncKkcblFMRED8wx6VIBgU1EAIPtTmPpQCA9KO1N3HNLj1oEL2oxTl+7TTSGQnij+HFD";
    os << "Hk/WjPFUSNVMHOaXAxkUo5NLQFgUjHvSOM49aDwfY0vQ0ARkZpE6Yp7jac9qbjuKAJFAxk/hSE5JpBy";
    os << "PajOAcY6YoGP3bgc/nUG/5x1qTcCp29+vtTVjGfvGhCHhlKcjk+tGFH3fSnGP90cYNRYIbHQ+lAC52t";
    os << "7U9jtIweDUbHK88GlVS/0piLO7cAfzqvIgUnH/AOqptmOjHNM5BO4DB70kUyEnByfzqQlW6nDUOmT8t";
    os << "RupB5BFPclkwDf3xiglVHBy1QAE/wARp6oSOMmiwBnqR19aEUEc/wD66kjjxnd19KHywwo4Hei4CZ28";
    os << "96jB3ZzTvLz1Y5qMqVznpQgHZDt7UrAHGaZnHTrSkE470xDSVVjgc00P8+e1SBOewprIM9aBDuAPc80";
    os << "FvrSZAIDdR3pGP0oGIwGMioWqVjxUdMQ0DFPAwD60mM5pe1Agp3am96eORQMaeRmlXpSN0paAHY5paR";
    os << "SaeKBoTuaTtSnqfpQOwoEEfU05eSaSP7xpyDk0DQjDimgU9qRRzSAUcCo2HOalxTSM8UwGgcU8c00Ut";
    os << "IEPI6VGetSHJxSdKBiAc0rdaWkbrQAelHrQOtB60AC9aQjmlUc0tAEY61IB1pOtKMjNADTxTCOKd9aQ";
    os << "80CGqOc1IeRSAY4p2KYEZFOXpQw5pVoARuCKbJ1FPccimyfeFAMb2pe4oPegdvpQAlFONMbNACN0pF4";
    os << "GaD3oXpQIXtTKkNR96YARkAjrTSM0/sKaRg0hAtTKBjJqGpFPFA0SBvrRwR7imqaXIyQvU96Qxhf589";
    os << "qcCrMMjmhUGetOKc8YNMQ5QBnFJkI3tSAEZxxSE569aQxxO3GKkzu571CFLYxwKk8vHRjmgYjqAOP/A";
    os << "NVGehPX1qRMqMMODRJHnG3r6UgAFWBydrUEP/fGKjZCByCKYwI7miwEoKr0OWqPPOR+dCKSeATUiJg/";
    os << "NQCEjQMRn/8AXVjdtBP5VFySNoGB3p+zPVjmkykRqQxOegpmdze1DKU+lIpwOOaZI/Cn73pQWUIMDke";
    os << "lMwS2Op9KlEf7oZwKQ9yDf85OKn3bQMfnUbRjPWnbgFG7t096GAoOCOvNDAYyPxpucgZx0xQeB7UDGv";
    os << "0xQBijHc05BuOe1MQqDGc9aViMe9L3po5PsKQCYGMmkKZOc0+kNAB/DQOaO1CnkfWgCbFHagU5vu1JQ";
    os << "3tQOlJj0pAxzTAcVyOKjJ+Y59KlU+vSmugLE+1AMYq5HFOHLH2p6DYpqOI/MaAHLzmnAdaIx196djg0";
    os << "hjAMUo6c0oHJpMcUCE/lS00dadQAYpxHIpSOaG6qaVx2GMOTQQelOPenUXCwmOKYTgmnyHAzUSfM/tQ";
    os << "hslToTSk0vA6dMU0cmkMafvGm96c33qTFUQwpSemaQ9MU0nJzQMm4Ipj9eKSNuadJ0J7UD6EL8yCpPa";
    os << "omPzLipO4pkCH74FLTT9+ndaQ0MIznFPxTkXBPrSkZJNA7CDhTTOtOfpimZxTERH7xo7UdzSLyaZIo4";
    os << "pRyaQU7nNIaEbG3FIh6048io2yORQhMlYZBzUajjBpyMDTwu5DjrT2AiBwcUMQSe9DcVJHCW5PAoAhy";
    os << "2BgUKT7VcSKLbyO3XNV2CcBWoTCwbyE56U8bSvqKYcheenrSEbQCMY9KAEuEKD2qSA7k2g/MKHIZeeQ";
    os << "aSNEB7/nRfQOpPgbOmKif7r07IWoXcMSP4e9JDbJVPQnincn6+lQmVTilMo9KdhEoPbilPTiq/mjP3a";
    os << "eJgR0osK48nhiPSkBHlrjpikWVQOaj3AcZ47UDJdoZemT61EwwmCfmoBH1+lOWNSMnOfrTDchRCT7dz";
    os << "UzEKqgcUMQvTgVGfmwT0piGFiTxTQzZ7VKi7n46U6MJuO5u9Akivzkk+tGeeOKtmOPHQ/nVeSIjleRR";
    os << "cLEZ5po68U8dMClCbQd1MQ3HFJ2NOOMUw0MBR1FPIximDqKlHJoBDTnFIKXqaXGMUDAdafTTwRS0AI3";
    os << "WlSk6tzSr1xQAJ941ItRrwxqROgoBCGhRzQ1A60hi0nenCkIzQA00macOlNPBoEPDYxSZ5pp569qU0A";
    os << "PHWlccU1Oae/IoGMXqKG6mhetDdaYDkHFIetOTgUx+KQCZ5pS2c00Ug46d6Yhc0Ug5NOPSkAd6WgDFK";
    os << "aBjWHNAoPWhaAFamP8AeFPfoajblhTEwekXrTm64pvRuKAHUw9adSDkmgBhpR0pcZzR0NAABnNRnqal";
    os << "6Goj1NAmKOgpe1NFOHSgBh6804cU8puHy009MGgA3ZPNLzkY9afHETy3AqcRx46H86Vx2KpZs9qcGIP";
    os << "NSyBNw2tSOu1+aLhYepDKwPNROhB9u1KPlyR0qRSG69DQMaoymAfmqXaFXpj3pjRqBkZz9aaSPp9aQy";
    os << "Un92wPTFOz8qk+lQ7geM8d6e0qkDH0pCJF6c0hPbiozMAOlN80Z+7RYLk3I/wprHqRzTBKPSkEqjNFh";
    os << "j0+6lS4Gzpn1+tVkcKQP4e1TEhqGNDJzhME/MajgQsPanyIhPfP1pVIVeOlHQXUU7QvoKZvJTjpSAbg";
    os << "ScY9KUcrx09aAI2J9qMnHIqRQnRmqy8UW3gduuaGwSKikAjtQTk4qSSEryORUa80ADdMCpAMAYpWXag";
    os << "z1pjsBRuAj9qVR8tMXJ5NSDpQwQh60hp3OaaaBsO1A+8KG60dxQST9MU48qKjzmnoeMUi0JimgYxmpc";
    os << "YINI65IpDsNpB98il6U0ffoAf7VGnEhp/c1Gp+Zs0xEye9P4ApsXIHpSSNzSKFGDnFN6g00HBzTweMU";
    os << "CG96cPvCjHFC/eoBDwaRxwKDxTuD16YqSiNTk+9PxmoX+V/apYzkZpsSGsdwwKVF201VO2nZOQPagBH";
    os << "YhuPSlQ5Gaa4zg05RgYo6BrcRvvUdjQfvZoPSgQ2TgUlEvX8KRRlvaqESRjPNSyDKH8KYTgcUxz8nvS";
    os << "K6EYHzZpc804DgmmZyaZIp+/Uwx1FQk/vKcG54pDQ9T8zUmetJGwO40Z5NADZDyKZ/FSv1pBTJGjjNM";
    os << "FSMMA01etMkB1p1IBzTsUikB65phqRhxTCOKAYwDBJqWJsA5pq9TSEcUxEsiqSrHAA6+9KX3EBeTVbd";
    os << "nvU9vwpI65pWGmSeW2Mv932prKnl8AU+PLKTuwMVHtXyT2INIZCxKZXPB6UP8m0dj0psowc9frTpRmJ";
    os << "cVZIzecYGKVWJIGaIk/vHAqxGiK4OM/WjQEiAgsOMmlELnPFW+BwOlRsaVwsQ+ScAkgU7yOR8/6UMcd";
    os << "6k3fd5FF2AwwD++fyoEAx9/8ASpc00Nx1ouwsRNEQCdwNNEbEfdqRjnp0p69KLhYqOpXOQQacrMO9Wn";
    os << "y3BpDGp68H2p3FYqOxJwaco3kDsOtOki67TmliG2PB65pgKo3SBc4ApyBOhApsa5kz0oQKQc5JzSAXY";
    os << "QTs+7703dtJDdaccjByTTZsHb6g0ACIF3Occ9BUcnNPJ4xmoj94U0JjG6001I4phHJpiF9KeOKYOcU7";
    os << "PFAAp+Y0vcU1etSdxQMa33lp1D9VpSKQxBy1Hegff46Ud6Yhehz71Kv3RUZGVFOQjbSGgNJihjgZpBy";
    os << "aAFU9qWmilU5BFAgFI4yKU9RQaAGU7qKSjkZxQBIlObpUan1px6UxiDqKD1NA6ig9TQA9elNegdKax9";
    os << "KAE6Cm0vJxmigQqDApTQKB1NAC0jHtQxwAKQ0gFpRTTwaVTkZoGPboai6nPvT3Py+9NAwppgxO9IfvU";
    os << "d6D97mgQUi9WpwFInVqBidzSE/MKd3NRt1oExx5NMx1p9MPegQ0U5aQDkU9RQA+PipHQNhxjjqKiHU1";
    os << "KDxjNJjQbtxAHWn7CSN/3famw4Ab1JpwycnOKQwcJ0AGKaw2yFc5BocLgYyDmiRcSZoAY42EjselMRi";
    os << "DgVLKN0eB1zSRxdNxxTAazMe9NRS2MAmrYjUdOT70qfLwKVwsVzGwH3aesRIBLAVI3SmqcDnpRcBDAM";
    os << "ff/SgW4z98/lTy3HWnZ5pXY7EPkcn5/wBKb5JwTkGpt33uRUanPei7AYYX44pApA5yKsKal4PB6UXBI";
    os << "osxBxmjccYNWJERnJAx9KglTrtORTuDQqfPkdh1oU78L2HWiIYibNJEMnPT6UAWFVPL5Ap3ltjKfd96";
    os << "ZtXyR3JNSSZUKd2RipKQ0PtJDcH0psaqCzDBB6e1JccqCeuag3Ed6LCuTSnIGKiIyQacBxSt1FMQgp/";
    os << "vTQOKeo4pDQ0009afimkc0wGGnnnFIw5pyjIFBIv8VPjPJphpU60iiX0pWPzLTc8iiQgbDSKHtjvUQ/";
    os << "1lKW5poP7ygGGeaQj5s0ZwaeR8oNMRPGMIPxqKQYyaSM/J70/ORz1pFdCKli5GKa4w3tSxdfwpkj+1C";
    os << "/epR0yaQfezUjFc4GaRGLNSsMjFIgxk0dB63Fdd1Ip2jBpcnJHtTWU7aAF3cdc0m77vY0oXahBFRScE";
    os << "UDZMDwKU4FQLnbntT92Ac0gHAZzSE9KdjA4ppzxTQmI/elGM0jUhpkjs5pO1C09VyKBoFHynNQsuOnS";
    os << "pz3ph6igGQsf3lSLyfrSkDeeKVTg8UxDYhy+aXvQvek70gGP1pB7Up9aQUxCydDSfxAU5uTigjnNACd";
    os << "+aUdfajHTmlXmgBT05prD5aceQaQjK80AMIwQRSt0pxHA9qRjnFAFfGDmpVOE4NMKkE5H405VJHt61R";
    os << "KLEQAUcnkfnQMGMjotEZwtNJyh9Kksgl+ZR7UvO1aR+acPuD61RILjOKepx1plOQE0holFNbFIGwMUj";
    os << "dMikAjDIAo6YFLn7uaawOaYh3frTBk5ApQCaVBzTARRxTxTV6Gl7mkArHningbqYMcUAEMeOtMBrjni";
    os << "j+FaUj0oOdoxQwGqcOPc0D07ZzQByKQcE4FADiATxnHamnnrTifTimY546UCEHQ0xhUhHFRvwKaAQjN";
    os << "NxljTv4aTHNMQIOlIaE4NDUACYyKk71EpwalHNAIHH3aVuvNKw6U1up+lIYi/eFLikT7w+tOxyaAD0p";
    os << "elAHIFO24HIoAawzTejU9v6Uz+KgAB9aRaUc0qgAe9AhTRQaQUADetIOaceaTbzxQAo96XoKQqSvvS9";
    os << "qYxF6ihuCaF60N1oAXqKQ+1L2pApC+9ADTSr60beeaUcUCCgUhpRSAY3WlJpWAI96Q8UwDq1OUYpp+9";
    os << "T1/pQMTrSetSbcjgUwjkikA3FI33qdjkU1/vGmA5etIg+9QvUfSnKOtIBveo2xk1IeKiY5NNCYo6Urj";
    os << "rSLQ/JoACPmFOxikxzS/w0AKtSHpUacipMcUhijjpTgADzk+tMxzzTwfXmkAH07ZzQxy59jTW5IyKUj";
    os << "k0DF/hNCDnmlGdpzQB60ICQjbTFJBOaCCWHHSg45oADTWFL3FI3QUANORgGn/jSOOaQgg0AKecihQQC";
    os << "KRRzTs/eoAcKcajXpk0pbIxSGhGOelMbGcU5wRTTQDE52tRF8qn3pT9w/WmpxTEWTgRgdVolAKnk8Di";
    os << "mg4UccU6U5WpKIGOU5NRYyakZSB7etNCkkYH41RBKvSkAySTTlOAaUDg+9SUIo+WnDpxSAYXilHAFAD";
    os << "T19qTvxTm4pMdeaAG/xEUsfQUoHOaF4OKAGn3pU60jUo9aYiQ9aSYZKYo705u1Ioa3B+lRqf3lTMcnm";
    os << "kUDeOKAGKuevSpnHyjFNHU08dqQ0R9qXOPypzLgUxu9AMU4J5pE7ZpBSrQJCg8GlIxikHenYz1pMpCj";
    os << "FITwaZnIGKY2cZ7UASbvvdzS7vl64qOPnNSFdyAAc0DQpweQTg0x+Rj2oX7tIeppAxOen4UEdetDdV+";
    os << "tA70CJA2V60054pE+5Snt9aaExGGTxQxoPemv/DVCY9Bn8qmB+X0FRx9B9KVvu0ikGcZpufmFJQe1BL";
    os << "EP3j9aUcNmhvvGjvTAF/ipD1NKvU0h70g6DT0po5NL2agfe/CmSOPDGl7g00dfxpf4qBh2FC9RQPvGn";
    os << "CgEHWjjBHrSjtSHt9aQwOB1prYCjFK3b60H+GmIjH6U/nBI/KmjofrT160APXheahJyhFO7H6Uz+CmA";
    os << "w09BmOmHpU0f3R9KGJDSME+lCcd6c3Q01eooGB65FBGBigdfxNK33vwoENB6U5uV96RfvClbrTAEGBS";
    os << "nrQtI33j9KAEXpR3J9qUdKT1oAOpHNScdM0wU7u1AIaOppM8CnNUb9FoY0BPIpw71D3FSR0hIfxnFN7";
    os << "E0dz9aH6iqARutRucdakP8VRt0oENyKUDk0g608daBEY9KaeTT/wCOmimAKMZqRB0qPvUi9qGIe38Oa";
    os << "Rx81OftSN96pGNXqKUn5jQP60nemAvUjtUmRtHOai9KcPu0DEPemDlqc3f6UxfvUASKMUetC9aU9TQI";
    os << "KbS0npQMcaKRulKtAC8Bc0nbNO/5Z03+EUAxFPz0MfnpB94UH7xpgO7ZpeCuaT+E07/lnSAbRQ1C0AJ";
    os << "S031paBB6UMM0vcUN1oGRnhqeO1Mb71OXt9KAJcjaecVH0J70p+7SetAAD8wobqaB1oPX8aBAg+alX+";
    os << "LFC/epU70ARuOtRsM4qRu9R96aAAMU40007+OgBccikyKeetMPWkA5DmpF61GvQ1IP4aBi9gadxnFNX";
    os << "qaO4+tA0KaaDyRRJUfc0gJs8GlPUUxejVItCGx3HTNR9Ceaf3Wm/wD16BMO4+lDdKT0pT0oAUdaRxkU";
    os << "L94fSlagBF+770hPWlXrSN940AAGRigdcmlX734Uh60gB+e9AGT7UN1NOXoKBjX4j/GoxU0n3T9KhXp";
    os << "TQmSA4QCpm5XioP4Kk7D6Uhic4BP5U09fant1ph6D60AOXBU5604YPSkH8VCdT9aQC8YA9KOlA7/WlP";
    os << "egYxupo7GnGmn7y0ALjkmk6sKP4qQ9fxoENPFOHSkP3vwoHRaYh46ilb+GkHalbqKRSA8tmkH3h9aU9";
    os << "TQv3hQAvVjTs5xTB1NFIETE/L1zULjH5VIPu0kn3T9KCiNTQvB5pE70o7UEijPPNOJwvWm+v1pH+5SG";
    os << "hMdKOen1FKe1IvVvrQMcnAx7U8YHJPFMHUfShvu0DR//2Q==";
    os << ") -440px -10px; }\n"
       << "\t\t\t#logo {display: block;position: absolute;top: 7px;left: 15px;width: 375px;height: 180px;background: url(data:image/png;base64,";
    os << "iVBORw0KGgoAAAANSUhEUgAAAUUAAACyCAYAAADVlN1IAAAKMWlDQ1BJQ0MgUHJvZmlsZQAASImdlnd";
    os << "UU9kWh8+9N71QkhCKlNBraFICSA29SJEuKjEJEErAkAAiNkRUcERRkaYIMijggKNDkbEiioUBUbHrBB";
    os << "lE1HFwFBuWSWStGd+8ee/Nm98f935rn73P3Wfvfda6AJD8gwXCTFgJgAyhWBTh58WIjYtnYAcBDPAAA";
    os << "2wA4HCzs0IW+EYCmQJ82IxsmRP4F726DiD5+yrTP4zBAP+flLlZIjEAUJiM5/L42VwZF8k4PVecJbdP";
    os << "yZi2NE3OMErOIlmCMlaTc/IsW3z2mWUPOfMyhDwZy3PO4mXw5Nwn4405Er6MkWAZF+cI+LkyviZjg3R";
    os << "JhkDGb+SxGXxONgAoktwu5nNTZGwtY5IoMoIt43kA4EjJX/DSL1jMzxPLD8XOzFouEiSniBkmXFOGjZ";
    os << "MTi+HPz03ni8XMMA43jSPiMdiZGVkc4XIAZs/8WRR5bRmyIjvYODk4MG0tbb4o1H9d/JuS93aWXoR/7";
    os << "hlEH/jD9ld+mQ0AsKZltdn6h21pFQBd6wFQu/2HzWAvAIqyvnUOfXEeunxeUsTiLGcrq9zcXEsBn2sp";
    os << "L+jv+p8Of0NffM9Svt3v5WF485M4knQxQ143bmZ6pkTEyM7icPkM5p+H+B8H/nUeFhH8JL6IL5RFRMu";
    os << "mTCBMlrVbyBOIBZlChkD4n5r4D8P+pNm5lona+BHQllgCpSEaQH4eACgqESAJe2Qr0O99C8ZHA/nNi9";
    os << "GZmJ37z4L+fVe4TP7IFiR/jmNHRDK4ElHO7Jr8WgI0IABFQAPqQBvoAxPABLbAEbgAD+ADAkEoiARxY";
    os << "DHgghSQAUQgFxSAtaAYlIKtYCeoBnWgETSDNnAYdIFj4DQ4By6By2AE3AFSMA6egCnwCsxAEISFyBAV";
    os << "Uod0IEPIHLKFWJAb5AMFQxFQHJQIJUNCSAIVQOugUqgcqobqoWboW+godBq6AA1Dt6BRaBL6FXoHIzA";
    os << "JpsFasBFsBbNgTzgIjoQXwcnwMjgfLoK3wJVwA3wQ7oRPw5fgEVgKP4GnEYAQETqiizARFsJGQpF4JA";
    os << "kRIauQEqQCaUDakB6kH7mKSJGnyFsUBkVFMVBMlAvKHxWF4qKWoVahNqOqUQdQnag+1FXUKGoK9RFNR";
    os << "muizdHO6AB0LDoZnYsuRlegm9Ad6LPoEfQ4+hUGg6FjjDGOGH9MHCYVswKzGbMb0445hRnGjGGmsVis";
    os << "OtYc64oNxXKwYmwxtgp7EHsSewU7jn2DI+J0cLY4X1w8TogrxFXgWnAncFdwE7gZvBLeEO+MD8Xz8Mv";
    os << "xZfhGfA9+CD+OnyEoE4wJroRIQiphLaGS0EY4S7hLeEEkEvWITsRwooC4hlhJPEQ8TxwlviVRSGYkNi";
    os << "mBJCFtIe0nnSLdIr0gk8lGZA9yPFlM3kJuJp8h3ye/UaAqWCoEKPAUVivUKHQqXFF4pohXNFT0VFysm";
    os << "K9YoXhEcUjxqRJeyUiJrcRRWqVUo3RU6YbStDJV2UY5VDlDebNyi/IF5UcULMWI4kPhUYoo+yhnKGNU";
    os << "hKpPZVO51HXURupZ6jgNQzOmBdBSaaW0b2iDtCkVioqdSrRKnkqNynEVKR2hG9ED6On0Mvph+nX6O1U";
    os << "tVU9Vvuom1TbVK6qv1eaoeajx1UrU2tVG1N6pM9R91NPUt6l3qd/TQGmYaYRr5Grs0Tir8XQObY7LHO";
    os << "6ckjmH59zWhDXNNCM0V2ju0xzQnNbS1vLTytKq0jqj9VSbru2hnaq9Q/uE9qQOVcdNR6CzQ+ekzmOGC";
    os << "sOTkc6oZPQxpnQ1df11Jbr1uoO6M3rGelF6hXrtevf0Cfos/ST9Hfq9+lMGOgYhBgUGrQa3DfGGLMMU";
    os << "w12G/YavjYyNYow2GHUZPTJWMw4wzjduNb5rQjZxN1lm0mByzRRjyjJNM91tetkMNrM3SzGrMRsyh80";
    os << "dzAXmu82HLdAWThZCiwaLG0wS05OZw2xljlrSLYMtCy27LJ9ZGVjFW22z6rf6aG1vnW7daH3HhmITaF";
    os << "No02Pzq62ZLde2xvbaXPJc37mr53bPfW5nbse322N3055qH2K/wb7X/oODo4PIoc1h0tHAMdGx1vEGi";
    os << "8YKY21mnXdCO3k5rXY65vTW2cFZ7HzY+RcXpkuaS4vLo3nG8/jzGueNueq5clzrXaVuDLdEt71uUndd";
    os << "d457g/sDD30PnkeTx4SnqWeq50HPZ17WXiKvDq/XbGf2SvYpb8Tbz7vEe9CH4hPlU+1z31fPN9m31Xf";
    os << "Kz95vhd8pf7R/kP82/xsBWgHcgOaAqUDHwJWBfUGkoAVB1UEPgs2CRcE9IXBIYMj2kLvzDecL53eFgt";
    os << "CA0O2h98KMw5aFfR+OCQ8Lrwl/GGETURDRv4C6YMmClgWvIr0iyyLvRJlESaJ6oxWjE6Kbo1/HeMeUx";
    os << "0hjrWJXxl6K04gTxHXHY+Oj45vipxf6LNy5cDzBPqE44foi40V5iy4s1licvvj4EsUlnCVHEtGJMYkt";
    os << "ie85oZwGzvTSgKW1S6e4bO4u7hOeB28Hb5Lvyi/nTyS5JpUnPUp2Td6ePJninlKR8lTAFlQLnqf6p9a";
    os << "lvk4LTduf9ik9Jr09A5eRmHFUSBGmCfsytTPzMoezzLOKs6TLnJftXDYlChI1ZUPZi7K7xTTZz9SAxE";
    os << "SyXjKa45ZTk/MmNzr3SJ5ynjBvYLnZ8k3LJ/J9879egVrBXdFboFuwtmB0pefK+lXQqqWrelfrry5aP";
    os << "b7Gb82BtYS1aWt/KLQuLC98uS5mXU+RVtGaorH1futbixWKRcU3NrhsqNuI2ijYOLhp7qaqTR9LeCUX";
    os << "S61LK0rfb+ZuvviVzVeVX33akrRlsMyhbM9WzFbh1uvb3LcdKFcuzy8f2x6yvXMHY0fJjpc7l+y8UGF";
    os << "XUbeLsEuyS1oZXNldZVC1tep9dUr1SI1XTXutZu2m2te7ebuv7PHY01anVVda926vYO/Ner/6zgajho";
    os << "p9mH05+x42Rjf2f836urlJo6m06cN+4X7pgYgDfc2Ozc0tmi1lrXCrpHXyYMLBy994f9Pdxmyrb6e3l";
    os << "x4ChySHHn+b+O31w0GHe4+wjrR9Z/hdbQe1o6QT6lzeOdWV0iXtjusePhp4tLfHpafje8vv9x/TPVZz";
    os << "XOV42QnCiaITn07mn5w+lXXq6enk02O9S3rvnIk9c60vvG/wbNDZ8+d8z53p9+w/ed71/LELzheOXmR";
    os << "d7LrkcKlzwH6g4wf7HzoGHQY7hxyHui87Xe4Znjd84or7ldNXva+euxZw7dLI/JHh61HXb95IuCG9yb";
    os << "v56Fb6ree3c27P3FlzF3235J7SvYr7mvcbfjT9sV3qID0+6j068GDBgztj3LEnP2X/9H686CH5YcWEz";
    os << "kTzI9tHxyZ9Jy8/Xvh4/EnWk5mnxT8r/1z7zOTZd794/DIwFTs1/lz0/NOvm1+ov9j/0u5l73TY9P1X";
    os << "Ga9mXpe8UX9z4C3rbf+7mHcTM7nvse8rP5h+6PkY9PHup4xPn34D94Tz++xtAWsAAAAJcEhZcwAACxM";
    os << "AAAsTAQCanBgAAAFuaVRYdFhNTDpjb20uYWRvYmUueG1wAAAAAAA8eDp4bXBtZXRhIHhtbG5zOng9Im";
    os << "Fkb2JlOm5zOm1ldGEvIiB4OnhtcHRrPSJYTVAgQ29yZSA0LjQuMCI+CiAgIDxyZGY6UkRGIHhtbG5zO";
    os << "nJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxy";
    os << "ZGY6RGVzY3JpcHRpb24gcmRmOmFib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOmRjPSJodHRwOi8vcHV";
    os << "ybC5vcmcvZGMvZWxlbWVudHMvMS4xLyI+CiAgICAgICAgIDxkYzpzdWJqZWN0PgogICAgICAgICAgIC";
    os << "A8cmRmOkJhZy8+CiAgICAgICAgIDwvZGM6c3ViamVjdD4KICAgICAgPC9yZGY6RGVzY3JpcHRpb24+C";
    os << "iAgIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CuU/DUEAACAASURBVHic7J13mF5lmf8/p769v9NLZlIn";
    os << "vRISQui9qQiIgrAoqyuIuir4c7GAylJcFXVdEVFRsIJKh4CUECCkk94mk5lMn7f39/TfH8nAJAS3gas";
    os << "yn+t6rnPmfc+cmeuU7/k+930/zxEcx2GcccYZZ5yDiP/X/8A444wzzl8T46I4zjjjjDOGcVEcZ5xxxh";
    os << "nDuCiOM84444xBfqd2LAiC8J9t44xnecYZZ5y/Mt52URwjhsIRbRRntI1uOi6O44wzzl8L75RTFI87Z";
    os << "ubs9Zs7b/3GP5+z77ipk3PUhzd/6BN3PHPgQE4H7EPN4qA42kfbybhYjjPOOH9p3lZRPOQSRUCSJNPn";
    os << "VoSOdL5iaIaW04p2OqAEYpArACYHBdHkDYF0jmxjxXJcIMcZZ5y/BMLbpTWCIIwmbeTTT552TCwQ/EN";
    os << "ffza6ac+g0xx17GBVsJRJ04vHzJn5pe/ede9zIbfb+elnr7hsy6aNbqd15nNWMpF7eNX63m0jIxXecJ";
    os << "Gvu8lD68C4QI4zzjjvHG+XKI6GEqX29trYmcdMWKlbwrTOvgKJZIqBnMH8SfX0Do+QK2rV9xx3/IqhR";
    os << "NK/pmvgBK9LEeN+jxZydMdJZ3sbFy754K+eeGKYgy5ytFkcLo5jHeW4SI4zzjhvG2+LKN58883iTTfd";
    os << "JADyMXObvxVyua8J+LxC0TbJ5i3qY2G8PplyIc2OPQl6hjKO2+0VGuvrsWwoFXNkqzp6IceJC2enL7/";
    os << "owseeXLvtwDF+R6lraFx17b//dEOpVDJ4Qxxt3tztZsySMT+P/X5cQMcZZ5w/y/9WFEXAeeCBB8RLLr";
    os << "lE8EN04TFtq9NZZ6KjupzJLUHBsXVqwgHyJZ1sOs9Aouz0JXWhMR5CFGxM20RGJlepIEkS8Xgt4WDIc";
    os << "byqE/QHkSVR94l6wm2L1cWzZq769m/u/9fBwf6qKPotURTtkCTZ+EEUBadclmwAURSd7u7u0XjlqLsc";
    os << "uz4ukuOMM85R+d8mWmyAHTt2OIAUamqyhvNqqinKxMaWFrr7BqkUi1RLOoPJArlMGdxuQXXJWI6JZtn";
    os << "IKFQdDU03aamNMKl9IpZoC1VdExRs/IriLpalll3ZEoltG6ZcdMExl7vlUyrhaLjLKxpONms2id6A3F";
    os << "Anm5FQqL8qiIFKVdb3b3r2+if/tGVfoiqWR0aK2tDQkM6bnaYjCMKb3CSMi+U447xbeVu6z4eSLArgn";
    os << "Tyx5X2qkP9ha32zKsoKmqEhmAbb9idwRAXTdiMrIgGXjCxJGIaBZmiIsocJdbXU1tcguT24XC4Ey0QU";
    os << "oKpbSG4RSZXYsq2X/d27MAppdN0EQUYQRBQZ4qKJ3++QqIpcfXojWny2kUxLL51zzuTtw+nM6pE+uta";
    os << "t396/bdOuykChMNodP9JBHq29zrhYjjPO3zdvpyiqgK+xMR4v5gq/snVzgayoRMI+asMy/RkdWw5QKm";
    os << "mEfW5CYT+WaZMvFjFtiMTiNIV9eHx+/OEIbkXG0Q2qho4pyRiORbmQA6tMVdPYvL0Lrayz6NgOomEfr";
    os << "67tJTGSRHHJxOIhClUTq1pCKxY4dmINS2c3EZ3QWJmxYM5LA6nyFheVLfc+vHZNw3C2IzJvXsf+vbsf";
    os << "ONCfyu8ZGDA4PGb5ViVD4wL5v+C/MuLpSMaP9zh/Cd5uUfQC4bbm5qkTOmZ8e/fune2ZkYRbxMTGobZ";
    os << "uAiAQrYkiIGLrFUxLxxZUosEwjQ01IIioqgvdMJBME0EWsEQJl0smmUrRP5BkaKgfS69SXxdAUTxIio";
    os << "Jd1chVSmi6hoqNrLqojdXQnymTNSyagl6cQoL+oRS2oLDo2CmmpHj37li9o93yqO7GYGzfVUunPx1K9";
    os << "7xQmXeyP15T70v/4ecv332gPLihp6fM4SVCR0vyjN+0/wWOIoZjRzwdOfLpaMvx4zzOO8rbJYoCoIZC";
    os << "Ia+macFqtRoNBALNM2bNmd/RMe2stWs2thl6MTIwMOKOxWuoqa3Btm1M3TiYbDFNBFEkEAzT3t6K1+u";
    os << "lUChiaQaOY4Ik4fUqeNwKPb0JhkaG8LsFSlUNwZFxcNB0A8HWyeZK+ANe3LKIZVs4NqSLGrW1Eaq6RT";
    os << "qVp7U5RDKVY6B7kIDiJixBX1VDEkXafC49JSlC1Eb6ilrJSe89/0V70dRf/PAnv3xt044DpVKJo8Umx";
    os << "yZvDkvkwPhNPMqfGQJ65FDQ0fWjhTLG61XHeUd5O0VRBtyAHwgDcaAWqBFFsXby5Ckt/kBwQTaba1YV";
    os << "V9zldYmCIGGaJka17CBQratrcOfzaaG+rpZQIEqmWMTjUnDLLhwXqJJIb98wpl5ClkVsG9yKQqlaIZ3";
    os << "JoQgOmmFR1Q1CQQ+ObWFbDvl8hXBYpVDScCyBxgm1lPMlCj1ZQi6bqiNQ1DSquk3VMrEMDUcUsQWJqd";
    os << "Nj3PmZ87X+oYGXVqwZ7m6r8W2eMbW+s1I1hdcSwaEf/OBn3RzuIMdmu8drKg9xhCCKY5YiIESjUdlxH";
    os << "CGTyYxuA28I4NgH0Og6juMcdXjoOOP8b3g7RVHiYBfaw0FhjABRIMZBkQwBAZfLFWptmTCpUCyGJVV2";
    os << "BYIhj9/tTg8PD2Wi0WhMVdVQXbxmii1I7kJikESpii9ahw+NmqYmdK3qmFVDKFXLFAoFtEoZSZIwTAc";
    os << "BA0WSGU5lEASTkM+Hblhk8gVqo348HheFkoHuWDgFjYAoYVo6A6UqLR4XXllGckSKuo4mCiSDHgTThH";
    os << "yBBbPayRRNmuM2DcFAOZHTnCzhnKPnHzlnilxUvbXeFWllKKKn7vvNQ68WeMNNvinjzbtMIMcI4qgYS";
    os << "ofWpaO1/7jl9JMn1wVPs+qWPHz2+dev4/BCfmPMusPBw/iuOI7j/GV4O0VR4GAGeqwwBse00KHPvIe+";
    os << "dx/aduwNIQBifX19UHS5asR8XjzulJMVw3H8kiAar6xZU1UV2RAcgapWleobausi0Xh7d1efV7M0sVQ";
    os << "sYpkGoiCQzxdQXQKqqqJVLAxDx+/3UVMTxu11oWc1JNNkpFxBskxEBKIeFd1wcCwDQ5IJN4fwqBKdB7";
    os << "JMbI5hGzo7e4ZRFJW6WJCAyyIecjF/ehMPPdvFtFqR84+fuL2+vWmoeclJL95+670P/PaPr6Z48+icv";
    os << "9uaybdIoLx+bg+tSzfccEPIo6rfuuXWW05uaKhP9PUP1d/5rVs7k90r0rs6Mye3TD/Wldi2sfzei2dv";
    os << "e2jl4Bfvu+/JPqAKaIfam8Rx9I/9PRzHd4L/SXIL3n3H8+0c+zzqAMY6Rg/gO6J5x3ynclBIZd7sIuR";
    os << "D6/KYz0a3AUAURckX8HklxEhDY6Q2Hm9osBwaB3r6hEgsZiBIpqZXFMs0lVKxJA6NjEiWZSGJAl5fgP";
    os << "qAH90GwbbIVUr4AM0C1baR/G7cEQ/lkk5jbYRcPkv/cJa6aIRYxIPf40aQwLFsNnYmkVSZT57cwLr9Z";
    os << "fbtHuQ9c4NmtDmeWnzmSTuHHflHKx56dstwidITT2wq8sbNfNhsQfyNusg/kzwRAHHVqseabRvXjTfe";
    os << "NvTSSy8BKNdcc03M7VZ+/OO7f7TstFOOy5+5NB687d9ftlEQn33xZfZsXsEnP/JVdMvmzBPbih/4xw+";
    os << "8/L3v//7bjz75Si8HxbEM6IfakW78df6WjuM7wX8jsTXK2MTWu/JB83aLIrwhjArg4g2BdI9po4I42s";
    os << "aKnjjm9+Uxy1G3Mfakjm4ri6KoSJLkURTJbRiWfCgjLgKi4zhiKBRSOzqmTqiWKg26Xq3bs69LNXQdW";
    os << "VTxeNzEQxFE0UHXNUKKB1ORUAMSsiSRzZbYPzDM7Ml1pHJlUukikxsCZKs2qixR0TQiIR+apDJrQhwb";
    os << "eHnjfgqZApIETT6l+tVrjtcXLpuVNaIn/vKlF/608umXV+/fs2ckv317d5U/P3zxr1Ykj7jhBEBYuHC";
    os << "huGHDBqGtrU2sVCrS8PAwK1Y8PsvlInjRRVduSyaTCqDefPM10xct6Lil0Lt19o/+7Xe8NizS3FrP7/";
    os << "54J56An1uvvwlLlRnuS7Fx037yWpGLzu0oXn3tdU9+6as//tHzz68eBCqHmsFBcRzrxP/uXPh/h6Odm";
    os << "zFNHPP5WN6qTvev9hp8J3jbRBHeJIxj3Z76Fk3mzU7wdaE7oo11ikcTYJWDIqyMaYftVxRFSRAEpb62";
    os << "Jjh/dsfU13Z0T2usDarJTM7V09MnT6pvQlBF0rk8jmmg+AK4fS6qWpmGmihuRWFvVz/FcpFYOEBN0E1";
    os << "Rq+AIIpgC/Ykcx81upj9dZiBVZP70ZnTbIjmSo7c3xZJmlYiqWOn4BG3GhMjw12+5eLBAeOWmTdUXP/";
    os << "axT21JJssab45DvlVhOfCXv1DfUgirGwS2IzQ2Nkper1c2TVPs7u6WAMnj8bgqlYq8aNHc8Ne+9tGzJ";
    os << "7bWnvLMQ79f9u0fPOFKpR2ho30C4YYIn/nU6cxfehL/euN3KKULeKJ+0sUsQUVh47Y+tu/sRXUL9vWf";
    os << "em/xtHPPe/pfb/3xzx5++PkDHHSOFd5wjmOd+KgLB/5+b+z/xK0fltTi8PtI4M3Z/SMfzqOJrb/LY3c";
    os << "kb6sowlGzjKNtrMCNdX9Hdo2PJoxj9zO677ExqlFRHNuUI373MAEVRVF1uVx+j8vllWTZP6Gt7ZjB/f";
    os << "sm5QpValrqrMbGptLW1zb6C8WqCCIer4dwJEY0HMQwi5RKZSI+D+lMHo9LJBz2sqtrhPqQj0jEzUiix";
    os << "NzpdZRsiYBLYc2WHmRvhOn1LrZ3J/CLFqdO83Pc0in2MfPaKltT4t7l7znTNm3XK2vWpB//2Meu3ZJM";
    os << "lt/UNWxubnZkWR4d2/0Xe5K/lRiqqiqvXr0aQJw9e7ba3d0tFgoFGZAXzZkU+cBFx8xpa5tw6vS5p5w";
    os << "XifSFV9z/oPKl7zwvpDIWUya3MWNqC5Pba3D7ZFa/tJ8PfPhUtm/dTbVq4vFJ9PUm0So6XsnGFGDd1j";
    os << "66e/qoiXmd79zyvsrMJf/4/BUfuurOTVu7khzsUo/tVo/GHcfe5LzTx+qd5i1ig0frRR15D0qA5Pf7p";
    os << "S9+8uwrLcHvv/u+R+7u60uNHbAw9mFyWFXF3/Ix++/wtovi6zs+/MSNPUHSUZZjLf3Y2OKRDvLIeMhY";
    os << "wR11h2/lQKUx27kOtdGYp0dRlMDktsjFDXFPneX4tR2d/es13Uq3tTV5Tlk+uRXBbv/Jz1+QTNMWHMc";
    os << "RHBTCERWX6EYWTfx+F4l0iWLFIOqTyBQq+P1u5ne0MpLVEESFWMSLYWpo5RItjTGee3WQajHNx86fyd";
    os << "NruhjKV/jiBRPthYub9eaG9pQ6p2O1LraMeNR8QCsq5s23P/DtRx99OjEyctiMQW+VuHm7T6ywcOFCc";
    os << "eLEiUKxWBRGRkakvr4+YXh4+PUHXjTqcbe3Twh/7B/ec/nEds8V0+dOrAkFZ0qO1cs3b/om9/9uJ3pV";
    os << "JRiWWTC/nZpQiFxBJ94UJJ/V+NTn/pHNWzby4P3PUNsSprExykBvjlQqiyhZGFUDryqQSpXo2j/MSCr";
    os << "DxLjfuednl1WL7nMe/MgVV93XP1xMl8vlIgedY5XDu9VHzs35t3STH80Jjl0/mit8/dr3+ZAVJex57x";
    os << "nHnDV5YvALk2orrS+t2pN68tXEOV2DuQRvTPp8tCn7xpZB/S0ds/8R75govukPvXWd2uhn/9lT7mgFv";
    os << "mOF9sju9tFEd2ys08XBpI+PQ1nxpvr40hMW+M/PlAQhmZXI5ypWtlRely+UeizLEmzb4dprP9Q6d3Jg";
    os << "9nVfuE+RXUjlclUMetxCbcRHpmJSKJkobgG/qoAjUV8boTEWwe1RsJ0K+WIFU9eQRdi9f5iBZJklCyY";
    os << "zvTHAU6/uJZs3mDmtkezIMDUeiX+5eimbdifpGkjwoffP1oJTJzz83MvWfdu27O5f+1pn5sCBEb1cLh";
    os << "9tWrUju9pvx4k+7Ebz+/3qnDlTPG1tjfG6uFkzc2r9DTPmTTtp8bFTBUmOoFd1fn77ndzwrRcQpCBLF";
    os << "k4m6FPx+SQkUSKfqDB13kTS6RyRaJSLLz+NbZt2kUpk2bmnB7es4PWr7O8awuWSsXSNSrkCtg6mhVbV";
    os << "yWWL7D2Q5APLAs6//OAq9vad+sCXb/rqr197bd9AKpUq8IY4mhwUyDe5xrfp2PylOLIAfnT9qM4w6vG";
    os << "oy06bFZw/veOCl5/f8uX5C5s8zTUOYU/afmyt+zMPPrTyOd64dsY67LHZ/dHvx0XxHfujb23/R5dHnu";
    os << "wjvx+7PNJZvqnEh8NFcTTGOSqKo+VDfo/LE3nfqW3/HI9VW/Z0e3ArIu0za1HkoL1ixfr1yXQhmclVK";
    os << "5puaI5jCyeesLzmtS076tomtwVdVGOqbUm5nKYIiiRGYlECfo+wpyshlCslJEdD10wUESY0R3C5bPb1";
    os << "ZKlULYIhDzVxP8PDaSq6xcT6AIKskq7C3j2DVKpVVJcfVbJxCyaKbPKtry835h930WOJ/sFN3b1b9v7";
    os << "hycLe9Zv35lOplFapVI5W+nPkSJsjT/x/Vq7xemVALBZTP/e5a9rOWL7orMY28fxg2D1LdeclSXAh2i";
    os << "UcQSA3kODS8/6VF/fmmDW1jkWzW7EkL1VDw63KDPVlae1opLEhwtoXt3PzHdeSTKR5/I8vEowECIe95";
    os << "DJFTNMkkcmhlTRM3URVBAytSqVQQBAsbNPBF/JgFEV6t+7k7KUOl37xapLW0keeWLHl8bvv/sX2ZDJZ";
    os << "0HW9ykFxHPsAeafd9dvFW7nEI3tYoteLFA/UuRvq5NpjZk+bN6E90PaBK8/5dM/eHe7Vz3eRLRics8z";
    os << "mla16z233bPlYOp3OcbggjpY9HRmftXiXdKH/T0Txv8J/oaZqrCiOLv9cMHmseI66xdFMuB/wSZIUDH";
    os << "hiV//g1vmnigrs3VXm1c1pCpbDxRctQDIcurozeveBdHc6nUnu2zdSGEkW05ZjIwii4HIpantLNGRWB";
    os << "UX0ev3VctU/Z2qjN5Ep1L22o0fStKpoWRaKeNDAOaKMz+vC63ExoylGoVJmZ08Gr0umLuTFcnSGcxZN";
    os << "zWHcHpXWgJeJk+vp7M2x5eXtTIlozJwRIdjc5iQ9MwamT4mv9NnF/p6CZ93vfvnA/r3dmUqxWDzSRR7";
    os << "NPb7VELtRxEmTmtVp0yKeZctOqH/P+R98z70/+slHb711klsOTMQxylhMZ/vKRyinddIVlT888iSPvJ";
    os << "Ji+YIGgj4XMgEcVaapOcruPT1UylUu/fCpvPjcFmrDYS65+gxWrdyMo1cpZKuYBrh9EpIq0runj4ppU";
    os << "8yVsAWoq/VRGClQyOeRZAtFUInGXIQ8KsNdCcp797D8FJWFp5xCte3CFSMpcafLNVK1bXfpwQcfXf2T";
    os << "nzwzwBtO6MhSnr/OG+IgR54nMRwOyxObQ56gJ+I6ZVnDgvYJNa1ts6YvdKnlE2dOqQmk00U+f8eLNLs";
    os << "1zjy9hZmtIlo5y5U3jjz0ypo191iWNeoMNQ666jIHhXE09DBWGMdF8W+BP1N68FafjXWLo5NYjNZQ+g";
    os << "O+wPzTTuy49VP/MFPs702ydXeW+3+/BUs0idUFuOTCJUxobyM/PIItifbePSO5dKJYLZTKwwf6M5lC0";
    os << "bCqlaqlGUa5UtV1B4cp7U3RyR1zozPaG2Kr1m9ySaIa3bG7Uy7kslKlUsFxIODz0NEcxhGhP1EmXdAI";
    os << "eCUWzGqnagoMD4xQ6xeJNMUxNY18RWFPf4FMOoPoaGhVnQa/wPuWTKBpTlP5wguW7du0xVy3cb/+yjN";
    os << "PP9WXzpfKU6c2uROD+WqmkK4oigdQsKyqfccdly9evU/xr3voj2scU1DOOHd56+ZtPQcGB7u0WExWFy";
    os << "xYXnfF5TPf2//Cv8131X5hwpL3XuCqr53O9753OcfFvTz13HYWnX0quzbu4MY7niZfLdMxqYZgPETIF";
    os << "yAU8ZFJlJgwqZ59O/fT1FzDJVecyAsrdrBx7V5uuOlKtGqJ9Wt3UixWwDKoqY2TLxaprw+TGM6yf98Q";
    os << "LkXkQO8IdY0BGiN+BnpTKLJNwK2QyVSQhSqt9R4CHi/pzgNEfSlEo8SSj59G4zGLSA+m8Ir2yENP+b7";
    os << "9lZvuXSOKtu5yqYYkSYYgCLbb7bIPza+JYYy+NE0/4opTj/LZkT8fuf2R24zu42j7UlFVRfB4FDEUsk";
    os << "XDUB0Aj2SLjiHKbkWqD4eUSNu0+YGmGkIdk0MTHJRFkWiktrUj0kS1KOGLsf6VLXz1jj9wYM8wC6b4+";
    os << "fAlZxKKOcydVcul//SsVTb9d6xZt2oTB8VurCCOxmNHww4a46L498dRhHM0Bjm2G+0H/BJSrDYauf2x";
    os << "J66fWRxI8+kv/hyfR8Lv89M3nKOrN4kii4QjQebOaqOtxc/EZi+24CJXwRFt0xFFx86mK7ZhCHkHw9A";
    os << "FUTBt0XYcQ6xUsC1bMSORmBoNhrxbNu9wZwqmtHv3LimdyRFwC0TCAbxulfpYgJJmM5QpEnaLhLwqW3";
    os << "vTSAY01AVpqg9RH/GQLhgsmNtEZ1+Rlc+spbnBz+6hCpec0sLuLsO6+P2nZE8+f1nywL7upmROqLRMU";
    os << "HaGw75QNmcFJbnSl+0rLAnUtUt6NptWZZ+06NSlwYpeLQ/tfiEvSwMhJ9/tarBeVvIvlDnhBx5ue/AJ";
    os << "Tp45wOIlX4CKRm8mTUvYzQUXnc7wSJK+AwkMB05aNgVFVdnTOUKkJkQ05GPDxm4++KFF1DTVsuLBDSg";
    os << "+F1ddcwZb1nRRLJboH8giSQKi6BAMeKhpCGOaGq88v4tAQMarQDpbZUJrDMfS2fxqF031ERpqXeSLWZ";
    os << "xinoaoRbwhTuvCdoY37yLXu5noJAUnLIHLZMmJ91sbNm/dWSrnRwKBel1RsT0e1RIJoUpFW/GJIDQ75";
    os << "WKVQEBHViJgl7CdBCJTyeYzlupJ4lYURDGMbQ0BCpJkYBk5JHV0bEIFqAH8WNZuLEeFbBYpMg2cYRx8";
    os << "CIKNJOoIoigUsoZtCT7Z71Mlv08Nu9Si38FnGEZRsAsF1So5bt3QW9weVdKxxYLhSEK5IhqVAq9u6WP";
    os << "t5jxbt3XSuW8A3bBoqY8waUIMW3To6Uzw+59fj964kNNPuGldIb/xXk3TRsuYKkCJg4JYOLRe5s2jh8";
    os << "ZF8e+VMYXdY7vRXiAABFRJbX3/+5f+8DMfmx38+e86GUon6Ny8i9aWRoJhL3v2pdm6cwBNt1AlEVEUE";
    os << "CWBlsYAoYBIX6KKW3HRVOcj6HcjWTYFzWEkbVAs5imVq0iiRG3MRyToQnYHiNXUkcnZ5AtpegeGyQwl";
    os << "CIZ8GA6E/W6aawLs7k1SKWlEXRKT2moI+l3s3pvk2MVzqI25uOfhtdx864eZHQ9wz6Or+Lebz+Z9p3+";
    os << "L8y9cwJLJrXQOZjnj1A4KpSrr1idJp8EXdTNSsLniwumYegE8OmZyK9U9r+JSdEKqQ99IhVde8TJnTh";
    os << "P3PdHJ77fLfO7T72FgMIka8tDXNcLTz23FdmTmzGxi2eIO2tvqMap5nnlpPxObozQ2R/ntA6v43vev5";
    os << "Wtfe4Cw12Deotk0TainoSHMxo37qK8Pk8/mKOk2T/3xFaZPbWbRCVMY7k2Ry1XI5gqcuXwKiUSZnt4E";
    os << "tl5l4eIJpBNZxIqJaFWppIbBqRAQNRpa/YSaaskPJBDFItFF0/C2HYPjTKTzlR/hDbvYsNbP009tJl0";
    os << "okMw7LJsRJRpR2NyTpqXGQyJVRXJ5EC2bfKpKS6tCIBRkKGdTLOpEQ16Gh8vIsswx0+Nks0WWLGghkS";
    os << "izducItmyiFYvEog1ogpu4W8QXMFizdh8ejwtLdJjU6Gf6lCZ2dmbYuDuBbmi01MdAibFgmgtFEChbD";
    os << "oPpJNv3FDGqBtMnqPQldHZ05bAMC8MwMWwAgbOXt9PUGGF3V4b00DAzl84h6IvzqWtm8y83rS0//cxL";
    os << "t+t6Nc8bMcRRQcxzUBSLHBTFCuNO8d3BmLHaY92im4Oi6AeCdTV1M2WrdPuyY+s9X7rpItRwGx++9Bb";
    os << "6e3PMm93ACYtq6T1g8sjK3VTMCo0xP109WQRB4MTFLVQ0h3yhwsBwmUS2SEttkLb2EIW8hmPb7O8tki";
    os << "5UkEWRoE+lrs6DZYgEg24ODBSYOKWDgWQf3Xv7iIdcTGyIMpgxGEhkOO2ckzELJZ5duZo77/wMdQH4/";
    os << "Bd/wac+eQ5XX7GEPRu7aZoQwC27QbV56um93PdoH+01AXKGQmscZs5w0z5RxWtXCVhlqhxAyg7jKhQJ";
    os << "1bjQi9Cfkli702Y4aSMgszOtY6LRuz9HouDg8biYsngOE9sbaKvz4/WJPLeym3AwQDKn8+rabTTVxog";
    os << "3hDn/1Km8tHmY45dOoXvPfnbs6Of4k45j6fIOeruHONA3xMxZTfzsF8+T6E9z3cfP4a6fPo1ThXnzWw";
    os << "jFvRSSOWJBNzU1QXZs66FazNLUGqe2PkZQNOncuJ2muAefCtViBn9Apm5KE8FJzYj1fqQalfJAjt6Vj";
    os << "2MoaWrb66gLfZahtMOO7Y9z76+7GezLURNzkcxU0XSbhno3DTUBNNMiFJJ4elUvdV4fFQsWz4+SKVqE";
    os << "fTIFTWPlmkHibj9l2yDi9fHeM1owRJO6qMpd9++lWKkSVr2UHIv3nNJMumjSFHfz3OoB+kcKmCa0xP2";
    os << "ctKwRTRBojnv55W93kCxrxKJ+Fs+MIMoeGutc/PR327EMg9mz6tjfmyMaCPGBC+YQ9hvs2J/hoae7eN";
    os << "+5dXzl5suppCxu+95TPPzwOr2sCfdWDWuANxziaJc5D+QOLceK4mjM8V2TaPnfvqPlb53R7ONosP31o";
    os << "PJwYrh78oSWb6RGuPHEM77v/eGdn2PFM3eyfMmneeL53fSP5Dl+WQNPPXUlv797F6vX7SQ4NcBrewbY";
    os << "3Z1mUn2Uyc01bNmzG9200HEoZDXqQn5cHontPQVsx8ZxBGwESpWD/4pbN0hmCqTWbSAY9jF/zmw2b9t";
    os << "GUcugaQZzjp/D7+/7Mi8/dCcfu/bTnDJnPl+//ec8cPe12LbIE7/bhCn6+NP6EQQ0uvszdA4X7Xr61w";
    os << "AAIABJREFU8Pm8dCz2M3s+tFk6kq/Evq1bMHJVwjGNiCYSDPnpKbj51ZMVCjmTQtpAdYGDztahPKmsS";
    os << "cin0tYSptVy2NpfYturG1mzUqB5UguL501m6fJ5eFUDQxO57PJj2bltHz3daXZtG+D8Mxejy26effpB";
    os << "vMEQwZDDvq4+7v/hU7ROimFUdZyyiSpIPL9yG8OZMg0hD8NDCbz+Oton1bN7cyeJgRFioQCuSA1awWD";
    os << "awhCCnsdsjVPfFMMbcuPINpJlYvsgp3dRWDVAascQqq7jnQCh+VF0830cSO/DMR5l0RyFhbMWsHFnnI";
    os << "ee2kRdbYln13TS5o6zeNYEBlJFbLeFKA7Smy0iyxJnnHo6m15bj1uV6UnmgUFMWUUvVxjKZxlMxPD73";
    os << "OzNm5R0A9OCjKZjCxYOLrJ5nXKlSr4sYZo2hmORKFk0xkIkclU6D+RJVg2qpslQMkvQ10I+b7FqUz+I";
    os << "AotmTKYuAFd+fhlnXdTKb3+ynl8/tpf12we5+pKZ/OhH13LvXXu5/ua7mBg1jbIm/b5qGHkOGgGbNyo";
    os << "0/ty8lW8aRfX3zrvZKcLhscWxmejQaDv3zFNPX/Gn564QkOQd626jrt1LfdOnOXZyA7ZHRA443PXNj1";
    os << "DOFPjGTfdy3LKldA4bZCtZZrV6efr5Xs45rYNbv/8sLRNiNNUGKGUrKG6b95+zmEI2z/NrO0nmqoiOR";
    os << "NDrIuAXGEhVqQl7GU4X0EyB8993EaeflKSueS6pjX0sOc3F/j4/j317NZOXz2a/6eWl5zbRX6gwrT4E";
    os << "ThlJLjE5JnHm4gAt7QEOCEOMvHiAwd0VDE2kfZKbBTM8lLICq/cU+O2qLDv3a8xqlWgKCug2HMga9KR";
    os << "MJEekOepCdcFQ3iZRtmgJy4TdFrIocCAJG7tyBDwuXAE/oUiY405ezvnnzEZyckyevBSXLfP8sytonh";
    os << "bH65Lo78lgmALpkQR9fWmGEjkcSyQ1nGPeadM444QF/PrHj5AtlZgxqYlwTRCtXCJS46Xe70Evl4lGX";
    os << "MRCNiMZA6cicaB/hNTgAEErR0Atgp3DEUx8PhXRFPDHbJRjPBjCTGxBQHBeA62KbQfR9Xb8Ygsx11p8";
    os << "oQtwlFn4mgxso5Fnn91AR0eQaDRAKllElFSq5Re4+2cZPnLluSQyOX59/2MsWzqDedPqeXblVlZvGSY";
    os << "oO1R0h0Wzwrg9EQZH0jTF3YykLbr70wiSTcDv4uIz5hCJevmPX75MKmMRjwXoGUhx9rHNTJ7UwEtbEs";
    os << "ybHuLpF7toamlh+ZwoD/zqcW664wq8jfVc9MHvMTEQ5ECywKYtQ4wkbqV/e46Zx3+TiQ1uq6LzfG8y2";
    os << "8nBB7/G4THE3KGW5fDu82gJkwlY7waXCO9SUYTXhXHsaJixI1xGpzoLK4oSCcrCdTldn3389Hae3/xF";
    os << "zjjldhJDJc5YPoE77llDXW2Qla/8ioZIgOHd9+F1T8VV38yXPvdNrrhgDoYMTz6yi1Mvns2OnUmcQga";
    os << "fKuN4GygW8+RSI1Q1g6DfTT5rkKkY+DwCpZKBgECuWOHc84/j/Pe0cdvNuynWWHz5qmbmHvs0Aa/E9d";
    os << "ct4td/2kdc1qgJOtQFHaZEbdyBGLMmBwh5k/z0gV72vpZk5tQAy46NMb3NQ6lkcs/jQzyyNkfJFJjRK";
    os << "BH1gWyJDJYs+nMOjmkS9cqEAzIFw6Q3a2PZIo1BULEJh1QMU2Vvf4WOFolJNS62D1TY1JUDQyRnKIgi";
    os << "qP4g8YYO6us1zlh+LBmjzOT6EKFIlHDYT6GYppgvkEqUKWbzzF84CcuS8XpkunqHySSqqHIZW/YgI6E";
    os << "nB+k9kCA5nGJgoJeRrEmzA8d0gOgVaI1Y1NaJeD1eMGVcCniaJYw6k4oqI0gigp2gsslGDKlQI4FrDi";
    os << "YpJGuQoHw6imkT8z1DcFIcX+R4XN4mTEshldpLPjWHkaFVqKpAQ/Mx6EUdS0ihurIoQpCRpEy5bLB5z";
    os << "V48iorHrZAp6zTWBtjZmaatMYiNTSpdJBCQCMZasW03kn4AQ1Rw1CB1Lp2qZtEzoHPOxaewb9N65i2f";
    os << "ieTE6dq6mSlLTkET3Jx+2nW8tmk/X/jIMTz98iDnnBbjq7d/iClTfkYys8+Z2la/b/Oe/lds2x4tvRn";
    os << "tMo8KYvZQy/FGomVs0bsJ2OOi+HfOmLji2LHTYxMuIQ5Ojhu+4LwFJ378Qw1XfvqG3dInrpvNxSe1M+";
    os << "PE/6Cpxc/jj9/G92/9OXf9ej279jxGwFjP9Vffw/reEvv601z63vn87tHNxAN+8tUKpm7RXh9HMy16U";
    os << "xnckoyAgKoq5EtlXG6VBdPrWLd5GBOIR9w0NQb40f+bwWX/PMJ5F9bzhetm8siDOe5/fBv3P3gxW17Y";
    os << "wuc/8XMuPCdOc309uUqJWDRId1+ZdLpKybZpDYjUR2BBk8KgZvGL57I8/1qeaECko9VFc0CgULDoy5m";
    os << "ULbBtEUWwiPoVBFQO5DSKuk7Q7aLRJ2AKOjGvl1RJZlNPlkk1CnOmhrBFF739eaa0qGQLFrmKxXkzBL";
    os << "7yhySqRybkgsFBHcURSFkitiAgIiALYCOgyuD1uMiWHGQZJEklYJRwHBvDtql1gWk66I5Day2YsozkK";
    os << "Jw6CQzBZuGUgzG3kZyJVjYRHQfJJeKVBIQOATOkYxUNrIRJeS+IMRco4G4GMe4GclQrYIuLkdVGZFbg";
    os << "EXJ4/CIBr0I83ookzcJd24JutLF/1/P07i2TMwssmDuLUjGFbmnU1TRTKAsMDw+xdbvGtMmLuO5z92K";
    os << "UNRbNrmXVxn5sxyQeDlMp6YiyTb6iEfYEqAvK7BvJIIhgmCa6YaKqCo5tMaGxjgXNEp//8uWIE85hyd";
    os << "xTuOGfTuKqT5zFscd9hZFEiaGeG/n+nS/ziwf28bvvnmJf943nXl23qaeHN+KIYwUxw5tFcWw88V1Vu";
    os << "A3jMcXRWMlocfPY2Z1fH+7UVufMlaphqaFe5ItfepQzH/4oJ8yfzKeuP4Hf/ur3BOMt3H/PRzh23oU8";
    os << "/thnECY2sPX5F5ka8/PHRzdw7WULaa0PY1o2yUSBb9y9Gq/XzR03nciBvUXqggoBH9z20y184sLpbN6";
    os << "TQfVpXHhcB4OpKpecO5P+pMKy5TFOXFjPmpcMeoaS/OK+M/AJdXz+c7+hpUlEcxRe3p2mNiizclMnHZ";
    os << "PrUQMKtaqbGq9Aolri//2hyKYekxa/xgkdHibVqaTyOlt6NAwbFFnEqzjIgoCiKIyUIFvWQDZpCam4R";
    os << "ajoNuFwgP0Zk75UkbntLqa1RCgYIns709REVLqSJpmcwYXH1rB6f5qOCVHOnBEkWdLJtZWZXCfyYhdo";
    os << "BZsJEYmcprN3xKApLCErNoMFi1Qe5tUIxKIBepIVREHkhKk+cNu8skPntIVecByqJVjWCjoma7vKlDW";
    os << "LcNBFOi/gVSwibhs75qAEvYgjNlZSQBHDRBpN5KiEJQtULR27UqG8VcIzzUYMdVPRypgGVPtjlIMi5Z";
    os << "hOMjGEp9qHAMQmT2f6nIXUN3YzPBKhUlXJF4MUjAyZ1ACSZWDJAmecOJX6+Coe/u3pnHvhH6gLS3z8P";
    os << "VP4w4vd3Hb9XPIZi7U7UyyYFeGG219lYH+JL/7jUpYsaiVTKPLc6l4eXbGDsmGyc18fF519As1zpvAP";
    os << "V1zJ4/f9P+753SbuvucxbvzsGfTsHiK/d4Tv3LUKnyKxrzMnXn7R/I69+1ND2WzRGHPdW7wxnG/s9X7";
    os << "kXJ/vqngivIudIhzWhR6NLY4dDx10yXI0HvZ8RnLJZz3y4D8LV//TfWzY0knXuo+y4MyHUASdqy46hS";
    os << "/ccj1b123g/AtvoK6hhqvOm8KdP3+Fc887iX+97aP0b3mJRx/eSjSmsHJnhlMXN7F6wwjFgk57bQjDq";
    os << "vLsy7vJ5GwCEZl8RiMYkPG43aQKJuef0k68bipTGmUcBF7blycWUTlxViM3/XA3555e5eJLLuDXP3uF";
    os << "rdteOzgFuqog2hq18VoMVWTdhr3sHyxTH3AxpU4h4peQJInuwSy9mQoBVT6Y+LEdFEVFQCRRtbAtC0V";
    os << "yiHlkROGgtfZ6XezsryJLDh31CpGYj6wm03sgT11UQFEl0nmTjhYfpVKFbd0V5k0J4pZsdvVXmN/ipS";
    os << "L4AIuRbJmIz4XuWOQrOiGPhCo7NMZdbO2u0hJVicWD7OvOIlgmjbV+VMUmkSwQViUm18ukcyUUUaExL";
    os << "NCTNOlJaZw8M4CqQiWv0TTdpuoCQXRhJEzKKR1HdOEJiFSyGrrgINbayJJIachEUGwCi70gqZQ2FSEr";
    os << "YUsCjiKiWBL+WhHVZWJXKhiyQNP8MI3tERxfGLQglZzEgSGB4T05Js2vxxuJUkxamPp2vvsfGk8+vYf";
    os << "6uIfhpEbAL3DpefMoaxJDwwViYTfHzgjx1JoBZkz0IaoO771wPjUTj+Paa3/EznXr+PL1Z/LpbzxDVT";
    os << "fYuO423O4QN9/4Y/748EY++9HjWby4lXP/4RdEAm7WvfoVzv3gT7j8gqWpr3/z10/l85U8BzPMWQ66x";
    os << "DRvuMU8h9covj7i593iEmFcFMcOoD/sVQpBrxI+b2nLFzb0OO9/6oXvcs1VX6YtaLFuV4pZEyM8tqqX";
    os << "bTufxzEGWLz4MhKpEp+4Yhn5ip+JMydy+tIgV113D0ODJm2tNRSw8QYU5tb42TesY5sWw8kC9X4XeU2";
    os << "nnK8gOCYYArGYF0V0kc5rREJuWtqjTJ8zk8Yg+IMKQyNlApLMXU/38MV/amdXd5Wf/CzHD26ahqhn2d";
    os << "hZ4KlHXuTiy86iYup85zuPMKU5AFYe0XRobAyRr1jkyjaCpCOaAhXNQhRFJEFAcwRKlo1HdJAcG78qI";
    os << "qsCgiSh4GbzgRx1YYXJTT5ESSBRFUgMFWiJuxFUiWpZp63Ji2pbvNZVJlLjZVpcZENngfqwRCDgRzck";
    os << "wh7IVQwsCzRdp1jRDyZqFPD54cCIhSQ4zJgQpme4RF9aY0Gbl+aQTKVapVDUmDlBJZ83GMmCy+uiKQK";
    os << "d/WViLpOTFgSo+MC2q4iCSLkARknAtsAfk8gMVTAtCDWJiApYpgy2Q7mkUVEMQtN9WAMy2Z0lVLeIqE";
    os << "pUCg5aWcLjkQlHJRTRQojJlN0iUk+amulRWs5ciJBLsH2TimjFaOwIoKpu0n1J3LU6H7lyE4lUBQWbq";
    os << "iUS8XiIhVyM5DSCPjcBn4sZ7SIvb0qg2TY+2U1nKkdLSOaeX36Vh1f0s+XVlUTDNvc9uI7G2gCvrHkU";
    os << "zS7zofd9kh27+jhzeTuCaTMkhrj969fy8Y/f6hw/N7Tl7l+vf8UwrNHucvpQG/15NMkythTH5l0miuJ";
    os << "/vsnfJ0cZ5fI6k1ujyvUfPv5zuzrT73/0+Qd46YWtaMODnHnmDPoHqjzzSg+N9XH2rH+JmbM/iD8U4N";
    os << "d3nImEw8QWD9++435OveAuzp03geNPn0gqpEBDgBFfiN/0ldihGXT7HYwOlQ19OfZmdQpRNzmfh8LMO";
    os << "Hv8Mlu1AgdcGuuKeXYKBhOawhRKOrmSRSjk4perulne4WH7y8P88O4N3PDROF5V5YmdMus37eC6T3yA";
    os << "1/q93HLrLzl2RgCqWTqH4XNfu4Fjj1vG1l3DJAcyuGQ/kssHSBQrBiUkbEEk5lFQMHCJNi6PjCK7KFY";
    os << "UNndnaYlKdEzwYqMwkjNJD+eY3OpFdknoJYvWBi+CbbOlT0Nyu5gUk9if0BFtaAz7yBYcmsIyiUIZWX";
    os << "RQReFgHMcSsQDV5SCYDqIoUXUEBEFEFmQE3cTt2Ph8Iv7AwZeMDSRB8ngxLJtq1QTLYVKtC1EDwW/hj";
    os << "4FWlNFHTOyySaUE1YpIf2eVkSEDJeAmEvciGjKS5iCYCoGYl3CNj+IBHcExcSQX2RJgg9srobgNRlIl";
    os << "unqq7Ou26Fur0eiSCU4N0LM2xc6n6rBDy4jVuPE3SBg2WLqB5EhYBRff/O5yUrrOCCJln8Teco5VPcP";
    os << "sKxZZ3zPAs1u7uPPBnWweKlEwHPqKFZZMDHH20gZOOuV6Vj72G847ewleX5D7v3MmLr+XjplnkxkewB";
    os << "YUgl6RFS/u57JL57J38y6efmgrd3/vZuEPf9o/+x8vOXGW260e7b7/c5O0/I/f7/K3iHTTTTf9X/8P/";
    os << "yfcfPPNYyeUeD0LPWdane/ub1x28/cfX3vpD395A+XcEFd+8Ct86YbT+MUDu5jWGmXJonoGhzL85L5V";
    os << "LFo4kes/PIPOwRL/cf8O4mKFZQva6Ufiyc15+mM+7CYfSlsjwSl+gk0iao2Kd0YY9zwP/lovgsuD4JG";
    os << "hIYA0J4aky4geBXd7ALffQ8HQUBGY2lxDIlVh884k27sGOHXRFJ7eOoSqOlx2VgMvbiry1AuvceOnzi";
    os << "dnhvn3f/8Rd331bDwuhd+/2M8LT3yfjlkNnHfZrfjDfm665VN07ennQP8Ig6kydY0BFFEk4BJRHIVct";
    os << "kworGLqDn1Zi6FMkem1ApPbIxSrEpm8TjpXYmqzH90RyeeqtNa7ABjMGBQMWNqmoBsO+4eqzJnopTdr";
    os << "0x73IksWuwfLxPwqmgmy5FDSDGRVIuwTqWgWFhLVgsGkFjeWZYGp4fZB0ONC120qlkMyq1EbcmHgoBs";
    os << "GQdXNpIiBJy5il6t4XCqmKZFNODhVm3zRQRMVQiEV1bIojlTRAcnlYOkHBViwbeSQTDZn4qRtohGZ/s";
    os << "4ylg6CR6aYrVJKVZBdEiYCO/YVSOx0sfzis3BH9jK0aQemM5/GBQ0Y5RLprhTeWAijrFHaLdCZT/HEK";
    os << "wksSUTwuVEDPlxBLzISks+F6lVxe9zIfplU3qRQ1dmTqLBq4zCzJsQ5a/kMuje/zIuvDhKbFOeaC9vZ";
    os << "vqfKt7/1EKKW5TMfPZam5ghbtya55kOzuPU7v+Hqq87k2DOWCbd8497WGz/7fmfda/u6Nc0YHbEyWqZ";
    os << "ztEkyXo8pvlu04l3rFA9x2Bx0Z5xxgveBO8/56qX//PsPnnXSbIRSlpOO/wzXX3cMD/9pB32DJX5y74";
    os << "d5clUvfQNlImGNf/vqdH73zH5u/OYrnDW7id9uzvLNR7YzIBo0friVmkV+atplfLUGhq8EbSqeuX7kF";
    os << "hWHAGJjAP+8ML5j6vBNC+IqargD4JkcxB334ZkeQDJgxZZtbOrvIRg0GEmVsUsaA/ks7c1xls1ppDcZ";
    os << "YMPOHlTRJFOEtZt28vUvfZiC2MK6/Qpf+/LlVLO9nHrqZ2hvCLJ+3T2cfs7pbHyti67hCh+5bDkLj12";
    os << "MWYVwIMymrhRzjlnOiScvY8uBCoYErS6N2oYAwyWFbEEjlysypcGLLSrs2ZFEdsqks3msqkk6azC3Vs";
    os << "VGZvuATtjjIMoSparAkolufvz4IE7FwcJEUiQURQDBQRVlXIpE2XQIKWCaNj6Xg8cl4YgqnYM25bKNa";
    os << "QuoioQjSuSLBs01XpAcQl4Lj0+isVamZ1Agta9MXBUpGQr9GRG3R0HPVxhO69ROidA00UtlRGP4gEHv";
    os << "Xp3BIZNSzqbzmWGMnEmszUt+WKeas4nURyn1lzHUKZz98TN46cUKgaBMY4vKQxuSlLKttC/7BsGmGnJ";
    os << "b1nDgpSSSFKa4J09qfw/lkkM1n6VJBtcEL762g807J4x3agDv5CCBthD+iSHUeg+qLhIOuwnX+vAGfX";
    os << "jro2zoz/DVH67ggRdTnLW8ge9+Zw0PrEjy6Y9Nx+eu0pcy+N79W/n67RexcV+aVzcP8qFL53H86f/E3";
    os << "HYv13/tSuHbP3xy+S9+/Inzg0Hf6Bs1RydoPtrkzK9PT/ZucYvvSqd4xHuIJUBubGx0/fSeW64995Kv";
    os << "f6q5ZS4XX/kPXH3pjfzLR4/luZUDbNmXZuXzn+S6z93Hq+sGOGlRIxt2p+jqMnl5S5ramiCrOzPI9W7";
    os << "ikxqZeFUtnnYVbcRCV0UMxcQWBBxHxVFVzKqFZYvYigqSiBR14UhgmzbSlAD4XSAImBkdRxGgPsD2gS";
    os << "xhReDcJRP541M96IUipyyfxPbuPJKoYDsClm0hSz7mTKtl05b9PLayh3AszMwprTzwyBqidXX87K5Pk";
    os << "e5L86Wbf4LXJXP9Zy/6/+S9d5RdZb3//3p2Pb1N70kmk94TkkASDIQSKRcEKSKiCIgiiIoVsKCiqIgK";
    os << "KqKooFxUpAsCgRAgpPdJL1OS6WfmnDm97Pr7YwwGxPu9xXvvd/2+e6291qwp+5nz7PV5P+9PfbNi5Vn";
    os << "8/EeP4SHLpo4EV169klu/diM3fuanXPepK7BTowR8VUxesIxd27aCsGio8mEi8dq2OFdevIJodRP5cp";
    os << "ntu/uIhlSmt0YYTJkkEhmWTI1yOGGzeHyU9t4srx808WkuAcUmEvCA5GA6gnKxRCSiUCjZxEI+EgWL8";
    os << "TU6JRP64iXyZZeWirE9KxXKFDIlPCGdhqhKre4QCth4vDrFTAnXkTiUsBnnlfFoMp0pB6dYorbKR7o/";
    os << "TWK0jOv3EAyOJZYUTcFOpfCMm0TNlPE4g0cxsoL9r+SY8YGLsVUf6x49zJTFkznjxntIH3iVF18c5q4";
    os << "nbqBjcydmOc2EhkpkfxjLo5AfzBO0BTgGyaEEuzYncKSjbB6uZVcxh1TpR670IFfoKHVB5CY/blhHDq";
    os << "moYR0R8kJQB4+KRwbZsPGF/fgm1jE0lGXD9l4Wt1ZwoCvP3s4s3f1p3ruokZ0Hhlm/rZtf/vRKPv+1l";
    os << "6iOePjQhdO5447fccedt5JOJ3ngp083fPn2a+01a3Z0WJb1bgNl321Q8f8TbPH/Zab4NpmCSp/P8+lb";
    os << "7lmSt708/vQPufe7d9Nco6L5QqTysOrVz3Dvwxv57R92c+uV8xkulLnivVNZOC5AfDBFIqZSd3ULFTP";
    os << "rCC3zU3YlRvdbY3V0ioyi6EiWjCjbODkLJ23hmA6SLhCSA7aF4lVQqrxIuoLslRC4SH4Vz5wK9PF+PN";
    os << "P8vBzN8afhDs44v4FQTRUqEkPJIns7BsCxKRUKKLJN93Ca3z+zlYaYw/QJIRRZQfH5aWmu4tln9/CDn";
    os << "64iEI3wwY9eQjBSz7N/3sTF50xjwSnzWbRgAl+7/WbWvPQipy2bzamz69lxcJiv/+ybVI+rJeQ1aKn1";
    os << "IOsaqzfHOX35HK79wvXo/gDzFy/ACDVxyQVL2LbrKB3dA4yv0EhZCjVhL2HN5vXdaZpb6kg6QYaTJpZ";
    os << "rEw0qpJMpegfy6D4v6WyekdE0QckCScYVMna5hFPO4A2oJNJl2mY00lxRweEjGaqiCuMjJYRRJlNwKZ";
    os << "VUyiNpSkWXR7dmcDRB2GcS35fDkr1MPe8kxEGDQk+aRNwhWxjzHeubdEJ+mwUfvZuKyctp/9csi69cw";
    os << "NIrlnB0V5KRsgdhJ+k9sJEPfu0cZs9qxuOZxMc/dyFNUyawbe0hjuxKEa4KgHDp6hilZ8jk9deTHNh+";
    os << "kD3DXn6z4yCuJCFsF1eTkMM6ckBBkiXUBj9KnR+lLoS+rBJtZhitwY/cEESEPeBVkVWXiqYK5EiMtX1";
    os << "ZBuJxzpxVx+ypLWRsl3tvPZnVq/ezZlU7mzdez/rtWUKxCPgk3nfOTXz19pvImQG+eceDJ/t8nhB/kx";
    os << "w+UY/9RB2lt0kL///9+r8WFMV/4vr3Ppd3kTIIRv3hoJyc/4sHv09f7362btlBfVUV9zy0gcdWXc66T";
    os << "Xv45m1P89NPncahoRyaK7FsQpj7nj6ApyqIpzmAlHSQAy6iQiJ7zAahoFTouJLAMV1kr4xrWdgFA9nv";
    os << "QUbg5Eogg2u5OLhIfgVntAymjdLgRYppCJ+MHFZQIhq4Mlv7Rlkz0sfOZBePr93H+EodF5d4MoVpOoy";
    os << "OZujrTVBX7aM6GuJARwpddZg+oZLESJpsqYDqEdi2i4bgWFcvmXye1gUnM376LD7ygQv5y59eAlPmuo";
    os << "+cw5bNh7nmyhVUxkr47CF2d1h0HM3x5uY+2ip9/OI397NnxzaMTCeVMR+vr/oxmWSJorcBubKFmW0RX";
    os << "jtYYFq1y3ce70YIwbgaH01VITKmRNDvMNI/wIjVwJfueQBzMMNoWjBt4Ux6enKUChbFYokFJ08nmfWy";
    os << "bvcgMY9FYdTmgs9fTuWQjTlYoOayS7BLGgOHBnAqxqHOW4o8YhOtruCZVxIU4jGueuROhnaUGbdoMad";
    os << "+70bsLpfiwCipgRw9B1PsOVBP27JTSXavpuGia1j8pWlMOvcsBjYeZtHZrXzy7jNpmN5G8sgBDmy3OP";
    os << "lfpvPMIy/R1VeiuSHKqFPgzV1DpI7mwCmQcW027h5geHAYs9rPb/YVsFwXoQqkgIpwXOyihW07iKCKI";
    os << "jnIHoHS6ket9KI2+9Emh9AmhvDMrUAeF8AeKSOZDoHaAGrEz4jj8tsntnPq9Bi9cYPRvMO9X1rOp29/";
    os << "klxyhDc2XcHt31rDuMo6Dh0+Qk/HEX70s0/jyrKvoa4mylgZ2vH7ODDq/D0o/ofs7L/Dfv8nrv9rQPF";
    os << "dNuidIjzvlBo4Mebxtmf8W2sc//KEZ6qA1lA9+aby0EBo0XyN9138CeZOqaJtYg13f/1sjKEC1334j1";
    os << "x7wVSOpAts2T/KGQsb+fi9mxmWFSJzK3HiJsWEg4hKmAkbIYMSlRGui2uCi4PQZFzTRdJlXMnFLRlQB";
    os << "iGrCJ8CpoOdMXHKLq4DsqYgeSQo2WA7uMjYORvHBinmoRwVbEv1s7c3Do6Da4Isw6GufkZHUni9Cpv2";
    os << "djA0nKSrL4MrFEzbxXFMsvky1dEQqsflSE8vplnGth3640nSoyUUTUcL+ImPpgmHApxyyjQO7z1K0fF";
    os << "zyYUr6YlbnHLSZO74wfUY5U6kUpaKWA1t06ZSTB8j2tLMaeefz8KaLKpdpKezyLf+OMBAzuLgQImhvh";
    os << "4iHpf+lMuaNwfJ5D08/MeHeO6pNyiJMCvOnIUI13PDF69jzaoeNMXFlJq564dfwR2wGBhIkdh3gIbWR";
    os << "q65+1qOrBpEr5iJE5xA44JJdOw4yhnX38CKC+cze/l5rN1RonZiE82zliHGBdl031NMWHIq877xKaaf";
    os << "twgrr0JJIhzJoNZMwkilsIZ20nDySrJTA6AnAAAgAElEQVTxHLg2ZSQcb4hYdSNmKU16tEA8Y7P6+e3";
    os << "kc3m6jg3S35tEU1xeeGOQnr4CPdv2YNoKIxOm8HxWx/IqCElC6ApyjQ9cCbfsIDQBuoTrSjiui+SXEZ";
    os << "KMsAVK1IPUGkBpCaBPiaA1BXFlkBwDryoRqohwMG/w0BO7uPfa6fxpVR8trTqzptSxeMXDOAb8/O6VV";
    os << "OheTl3Qyoc//gVmTSzQoFd4ptSGTtV15fh0qHeC4nHGeKK8x78ZW3wXG/w/2fBbA6D/bwHI/3VQ/Aeb";
    os << "eKJu9L91v9sGv+tL+weAqADKBae1NQ12brj6kk+dTyazB59hcu3Fs3n4uQ7qJvq5+fbnUCyXRMrkz68";
    os << "dYv7kKPf8bgdFWyJUFUZCgFdBa1QRsgSqglSh45Qt3JyBHNMRQqY8VAKvDK6LUzYQERVUCdd1cB0Hp2";
    os << "QjLAdZl6FkYSUNZEfCHjVxii7CdBCmQHZBliUkVUX2K3TYo7zRcZAtg0fxRUOUbZPRbJ6Az8vgQIqOI";
    os << "0fZd7AXSVYJeP3sPTLMniMDxIIyrlNmR3sXA90DYJVZ9cZu/AGJYIWXNa/tJhJQ8fg08pkCfX3DaLLN";
    os << "0rMnc/XHzifU2IIuFLav20Q6X+bkM99DfWOE1S9sYcKMCVxyZiPIfm774yCGWyZTltFUD6qscqgnxbb";
    os << "9fSw8aSajRYXLb/gMVcHDbNz4Khd/7oNQSNG9fhcXf/gjtM1sYMl7F+NVBpky2cvdf/4ZL283GDchhj";
    os << "cUpfX8K6ifHqN8aBtaZQyleTmlQhiP10/05JOoqg+xojXC/H+ZzbE9a5m0pIWKedMYOXCIQARqlp3BB";
    os << "T++kVkXLiLcXEfmSCeecAV9a15ieMsO/CE/Zddk+6r9JPoKqH6XeG+crZsO0XNslIqaKMmRDAcPpknG";
    os << "s4hSGlW2Gc5LvBE3eXwoxfr+QRwBSlhHCutImoybNpD0MS9AAG5u7GASfh0za2Oly2C6uIaLossoVT7";
    os << "UZj/K5AByrQe36ILrIAuXcFMV/YbNZ+7bzowJQW770R7iiRKSo/KlTz3HpGkxtnT0c+biRga7imQLWe";
    os << "789bm8uW/7KReetWgqb59A7+NvwPhOqeD/qI0dv9/Ndt9px297/v8WOP6vguL/YRNPlCw9PvPw+H2it";
    os << "vOJJ9lb2tAnbuq7rPMWKF58/smxgHBePmYI7dzzp3PGivtYsXQ2mw7GmTXJR31AYufOIVae0Uqq4OEL";
    os << "V5/C8ECC4XQJ37hqfEGBZSmoEQ9OwcUuChxVQfap4AgQEghwShaubaNWepG8EpLjIBQJVwK35OKkLYT";
    os << "rosR0pLCKUASuInB8EnJAQThijEEEFdRqH3LYgxLWUaMaSpUHM6ZS8LhsGupkZ6KfzcNd7MwM0LZgHH";
    os << "qtxr7OTnYe6CGbzxEJqtTWRPj9s1sYPJqktbEaf6Wfw0eGKaZMduw/hjATdB8bpPNQHyGvID2ao3VcF";
    os << "RGfj21bepAkD5VenXLZZNPmHgKhADOmN/LMk2t4Y2svdSGFCW0NfOBTN+OLhMkUigRCYSoqI/g8OgKF";
    os << "UFTl+msWcNl1FzB5XhXbt+zl97+9Bp8Gh7sdLrr8TIbim/nhk/egqNWcNKUeMx8n1trKL797Lss/dhW";
    os << "JZJyBjqeZdO1V1CxYQl4IwlVe3nfHuRxa/yblUCOBOpUPf/sDlDx+hvp68FXUE64IkRzuIhANs/+pdR";
    os << "gjJWZceBaFnJ/O3ftQzFFKRchlc+STCYY6BynaFu27Onn84XXEB5OUjDLpgSSGaTNwbIiezsNjcdGSg";
    os << "RLWeG2kxPqci6GApCoIj4ZQJJSghmQ4uGUb4VVRKnxgg1N0kbzqGHvMWEiqBCEZoYFVdHAESC4oDT60";
    os << "yVH0aVUIRUMIB9W1ibTV0z6SYfPWLj73gYVURGJ85iNzeOGNDjyhKCXLz+7OIa543xzOXnk/k1rHoem";
    os << "1imP6LwoGPSH+BojvBoonAuPb7Pcd3t27SQr/e+z4xDXexkr/p8HxfwUU3/FB37mRxzdQB/S5c5uC9b";
    os << "HgJXfdetFS3h73ODEofPzl/V1ZgfjblO13e1na7bec98Dz2+LB11Z/mfVP7cMuVtOTcbn/X3fy6S8s4";
    os << "7nNQxTSRb5+41LOO72e+/51K+vb48jRGL6ggmWLt/4bKahhqzKuCnbWxNUkhEfGThi4roMa9eAWDGQJ";
    os << "hF/BzlhjrrVrI6sC4ZdxFRc5qEJQBRzkgIpnaiVOwQQEcoUXpSmAFNZRa33oM6Ko48PoDQH0CSGo9uB";
    os << "U61DjxanQ2JnuJ+4x6ZDT7B3sY6BQYF97B2G/QvW0RrZ39lJbW0M4WE2maHPWsvEc2t/H8891MH1KPd";
    os << "sPHqX9UC8vv7aLQibD1v1xprWGefLF3dQ3VlGyYDjvUFFfSSaZomx4iVZECAclnnhsCy+v2sq1Fy5E0";
    os << "1RSmRT5fAq/X8Pr1YgPZ+nqGuSCFZN57aUtdOzrJ6pL9O07yle+8WGSBQ2hyXQd2MM3v/MaDfOn8fQj";
    os << "rzF6aDWn3nAdWn2U8sgA5SN9VC6cilHOIoommk/itaf2UtPoo5xKM/DGbtyYD3RBfGsHVtkgOmk8pVK";
    os << "B5FAf45fOoXPjAYb2HCY2IULP1qMc3LibyIQmsGXa17TTfTiO8Kh4AzIl18P+g4NImWGKhQLgong1XG";
    os << "+A7a7OM30FfrKlm81DcaSQB0mRkYIqSoUfWVWQPQoi5kMJeVB9YzLljuki62IsrFI0ERK47tihKoXUs";
    os << "frJrAHKGGPUqgLI1T5EjQ855oeigVOyqJ3Xyp54gXWbD3LGslrOXjoe2bXZu7OLZx5eyWMvHKb9SB7V";
    os << "rWXnun28uuYTFI3B2K0fmX61risB3j2++E4t9bcIiBDiRCnhvwPCa9+3cPH4xrr3VVb6grw9oXP8PhE";
    os << "gj//t34XH/ifB8b8dFP9BQPWdLqwMqJdc8t5QY2Po+AnlOWVuy+T3Lp21YfKU6gd+9dT+Pz312w9/iL";
    os << "dT/BNf3IlxkBNZ5HGgPPG00ubOrQt1H7r/nutvvn/pww9fjlc9yidufYFvf/EiCrmjRAI6Ts7mV7/ax";
    os << "vvPnsTzr/TSvjvHUDJH1jRQgn4U1cFyQbguWCAHx9xZyQa7ZCNUGf6aZRSKhGSBNVzGMsB1Fdyig2Q7";
    os << "SD4VRxO4QQ+uA6JcRo16QJYQksAYLo4xCl0BWcLO2wjHRfgUhKwiqQpSSBtjjpVetGofStiLWuFDqfQ";
    os << "iBVVcj0wHabakemjX8ry5egfp13bQcTROVjJRYyF8TQEOHB3lxdcPs3XAIhwJ4JgOB7uGGM0VeOqF/d";
    os << "iOyaNPbWN0ZJSSIjE4VAIXdmw9jLAcZsxsZsL4Cp546QiPPruLDds7qG6p59przsM2ypSLDoW8QTjip";
    os << "1h0WL1qN3URm/6+JAOJMl2dgwwP5glVeggFTGpDfr7+hcdI54Z5+bltVMa8DGzbw5M//j3Hdg8wtHcA";
    os << "uSbE4MF23vzOHwj6dC674ndURDVGOuPsfnodR9pHGD54lH2v7mG0N42dyRNpqSXTW+CVu1cRa61mqGe";
    os << "I/n29yDLkJYl8roQ5UkZyZSQFZJ/KxEmNaIEgExpV3ECYNzIqWwnw3FCWx49m+e3hEfYXSrh+DVMWyC";
    os << "EdyaejBHWUmAdJV1CD2hiwVXiQwxpO2cbuzuJKQNgDlotcpSD8MnbJQlgOVtZAqDZyVMd1bVAEVCgID";
    os << "VS/ApKCUXQRg6NQtlAqI/zhxSP09dr87rmDfOKSuVz3+ecwrRIBr8Zwspfv3LKSm25+Fr+m0TxjMms2";
    os << "J9v+/MSXLo2EvBH+nny8M874Tnb3bmzQc9kZ887Y0D78/Enz9V/dc+uKV7xeddxx+/3KVz498R/Y8Dv";
    os << "rJt/mWv8DPPmnXv9tdYpCCHFC18g7qfW7CdhrIyMDyrZNP7mrtW3JiGlqiw+173xkW/tg1cO/nC8Vyo";
    os << "Z6z/fXn9JQ1dg+kh5N8fbT653xxXc7vU58efoff3/3xU88+fJNzz63U/3xvR/i1DnfJVrXSMhn8cyq7";
    os << "dz7vYsYGc3w9FP7ue7y6Ww7mGRKU5hjg0mG0hYeTUOLeUFVkVQZOSgjPDLCL5A8KrIqsAs2uDauAq5l";
    os << "j9Ug+nRs28YtjjE/SZbAdcEeixPahotdsJD0sT5cN1vGThrIEQ3JL+PkDNyCiRLSEaoyloRRZCRZQlI";
    os << "l5LCKQCBUCaFJCCFQAjqST0HyyAiPiiMJ7JiHaNIh1TlIz7FBrOFRtj/zJodTgk1bb+eSeRavbYgje8";
    os << "IkRstMaAwR9HvIFC2ODRYZ3xDCo2lYjo2muOzcP0RLUxUlo0C2mGdksIBpWTQ2hpg/s4Y/Pr2ZgcEU2";
    os << "DKWZSAUME2H/pEkVZEqdK9KZUwjMVxk26EkfT0jtDUHeemFbSTSRXy6S1CXaZtYzdb2ASZVy4QVgd9R";
    os << "yfWB3Btn+oUn8dk7X2TO+BinnjmFXdv6OWlmGEv3svtwGqVoIAdU/CEPg7s76OuI43o89B3sQSgKYZ/";
    os << "OxvVHCEb9NNcEcSUXEZQplFy8QuNA+yFeefEQazZ2ED1pAa/s7WSwUCQruaRcC0cSSKqEEtJRIjogIX";
    os << "sk1KgHWVdRwxpSQEJWVJSQgmML3JwDrj2WTPHIuHkT2xVIApSQjiTANeyx0IlXQZgCK1HGzpgIe+xnK";
    os << "ALXcHBsB1E2kSsCGKkSP7p6Fs9t6mHezCpeWtvD1OYwpyxo4M2NR8kYNkZJYe/2nXz9y6fxvZ9uoH/w";
    os << "SEMiITamM7kyb5/A/Ta5W/4W738nM3wLGCdUNy7beST+6Oc/2ez5yEVN8hU3vl6VyZWunDV97ta77vr";
    os << "GNG//z854fkN2N39f+nOiXvuJiZh3ShYfxxXuuOMOcccdd4h/Fpb9t4wOewd6v9sH+7sgbH19VM/nHW";
    os << "XhSZ979c93xZ58UfYG5k5vE9EqPzfctJG7vzKbBdPm+e/75chdpzbNvvHgke5UIpEvWpZ14sijfzT6i";
    os << "BPWVQDtsvfftiGdyQ4tm1rd3Ld+B4fjJk9+dxEec4THXw5RGsnzq8f3sHxRIw8928O4Wh+JwjB7OkYI";
    os << "TmnBo8lYZRcXE8cDjqMiu4xlk1Ub4ZeRCjaSA25ARQR0nFQJR9hjv+fVxjLTRRvJr+C6NlaqiORRcT0";
    os << "qdrqE+9dB8YpHHos9KgqY5pirpgrsjImiSiheDbtsIoomjiQQEQVNl7DTJaySg6QIXIcxd90pIwuB65";
    os << "fpmx3BeWWEyFCJIzmDVNrglukqgWPtfPCOTZjFIp+/ZjoHK3RSBRUhZBZMiTF1YoSf/3o9jc0xQgGdf";
    os << "N4gFvDw2J830xD1oXtVNEXg8emcPLeRn//6DTZu2M9ZZzTiawzw4pPHKKQsyuUCsqxTKJVorg+STJXp";
    os << "HjaoqFRx7By/fmQXss8mb5SQVZ2rL5tKLmUzf/7J/PB7a0klBpg4R+NfN+1gXCiK+cAWtnWNsvLc2dT";
    os << "Xhdm+uZ9HnjyEFdI5Z9k4PGaJsm2RSpXJZU3UoBdVMfBKEtHaAJu3HKU/kWHxtEoaFzZyYH0vv77tFS";
    os << "YGZYohhRdGTOaNi5He72IO9BCuCJIqlpE0BaEI5Ig2xuC9GmgyFG2UCg015kE1QHgkhKLgKRkYBQdRt";
    os << "vFHFFQh46RLlMs6tmXjZsqYmoIak3ANA82vYptjhfxCFgjbHTtMIxIiM9azrdcGMQwb23Lw6Aq5mjDP";
    os << "v7KPpoYoL21IsHBqJb94dA+3XLsY29L40PnV9Li1fPXW5/nGpxMsmRLjyT8fsJEVmbFM9PFRevYJdiz";
    os << "ztxmLf2dbkUhEra2NaR/7yOx5q1859OvvXjRJD0kuV93UzhmnzCCeGPR88PKmR46+8UX77sfMDwQCnp";
    os << "jXqzqpVLZkmm+1G75zhJnN2wvKj697YnG5cxx3/hmDK/7poPguscJ3Y4hvY3CzprZUf/+2yR/z1i7Mf";
    os << "OKzj99w+Vd6/Y/+bD6NExaRzwteXruby67fQFOtwiduqKpraxr3B0M/Z2084Tl8tDff8eP7HtyZzZoF";
    os << "VVWdbDZbMk2zrGkajuOPqmrJLBaLiRPWVgFlJJ3WBVJp7sIW6ue24lUVvnzPm5wxM8K0CRUgu5SLFqP";
    os << "pMnVVPiQhePAPHZiWhGTZOJoKsguSjJO3oeTgFkykgIyrijHpyqiGnTPGXKKwhpMu4dgOikcgpL8ySc";
    os << "lF0iWslIVAIHllnLKDY7kokkBEPVipMqLs4iRKSD4QPgnHslGq9LG1sybCK+EKGcexQVJwNRnZ60BMx";
    os << "s0YCFsgwmOtdEJTwLAxLZvgyqmIVzuJuhZyU5ivbT3Goyt/idpUz1k3TGHdzjjnnD+eL3/jBTZtiDN7";
    os << "9nS6ulMU8iMUy1lu+eRKMvkiAUUmlS3RMZhg8axxFE2bc5eOw3ZsXt94GEWR2bshzeVfq+GLX5/Lsfs";
    os << "MXlq7h9wBi58+/DrvP38O6RwkR0ZwJJWKqiijxTQNis6xzgST2gKc9P5HaajwEA56yZSyVM/TiU3z88";
    os << "X6qVz/1d34fRALefjq91fznR+9xpwKnc6SzazJNTyfzPGb1Ye59v1zWTolgs+nULRdmltj7N4Z51sP7";
    os << "mN8o48pDVHWJUw6793Mjmf2Mq1NRguFKfmDtOk2hjCZ0NbE1tXtrLxhJY+9uQ8kCdmnIFQJyauBLCM5";
    os << "Lnqtl9bGKF47zR2Ll9OdT5Ie7CU2eIyaGdX8tsvhk/UVZJw4ua40A/o01td5eL3rGGXh4KSLIEmUbQu";
    os << "hykjCBZ+GFFBw4wZu2ULygtlrgiYgb+LILrZtowP3rxvg0hVhYkGXlCkznLYYGEwTDKsci5f52UN7kI";
    os << "HqGdMY17Yd+1Uta1uGjzFgeguEFAVhWXJMUYQpSdaQYWACTJxYK6dSltVcW6F/6bPvWVLVcOYETeqfU";
    os << "iGvO0dVkG7/3mYk4eGKy+bzsctORqvR2fbqs9J3X3elKTMn/v6WG5f/ppTYNu6bPzjyQGfvwDB/68E+";
    os << "ERxPnHP6bgDp/BVjHMD9ZwDjP2102D9ghzIgVVaiXHbO7LqGCRNYdMp541a/tnVw1aoXy1u3dlnzZ7V";
    os << "Vvu+Sq6578/Hff+iUGZ3aVV84j5//PMFd969h1kSZBbNqueii5ehC47Fneth1LAnpJMd6u5Fk+NLHJ9";
    os << "I0ce6bHQMtAwFvue6Pf9kR93qM9iqvFFtyxulX7tq9J+MRo79MjEryjoO99RG/oUUjtaPnLmluznhKF";
    os << "1RV1GrL5zdx6bkPsPj0RnJZh47eDB97/xS+/N1NtLb4QPdx5ekt/OhXG9ibgNikGHi9OIaDpIMU9kJQ";
    os << "RpUl1AYv8jgfruUgcHFdBQkXu1wCRUHoClgOTslCqGMMwy3auICkSbi2i9BcMFxgzNjsdBkz9dfs9AQ";
    os << "/kiLhZAzQZTBshEfBdWycUQMcCUkT2GUHoQFC4Fjg2mPJHDtnYCVMVNNFKhqomsaXzr6ce67+EuFYjE";
    os << "HXoiue4qYzW5i3fAqf/dFmXvjN+7nxtrVsau8kGA7j9Yy5mr3xAjPaKpk+sZKXXu+gocLD5GmNzJvfy";
    os << "OatnRTSFlvaj5LOlwkHVYbiSWoCXi6/V6cwRyf8Qj07/nSMV3YMAoLKyiiVPg1ZkQhEvIR1mcGhNGGv";
    os << "h1ShxJG+BJZhsfzUKB/7YTPT6xzWPGJy61d6CPhNKkI6sqpRLFmU8ybvXToOU1bY296NVTKRVNh/rEh";
    os << "tWGF8ow/NG+Jwxyh5XE6aVIHf65I0XMb7VOz9R6mbIBH1+DkkKknYKla+TDKdI1QZZtXao2zbehWLb1";
    os << "qNHPPgj3hBFYigjrAFk8I6VzQWiFkyyU0HOPfcCGq4msTBUbbZIWYaBv0Flzv/2MWO3gKGIWHaFpoMV";
    os << "9x5NS/3HcQMyigxL066jPAoY3FpVR6zMAccw8HsyI4dllUB7GyZ9FPtKH4dJxZAjqe5buVMXu4YZVGr";
    os << "n6fXdPKbu07l9S0J8pkiBaPMkUNpHvnD53jxzc3Ee7oKW7dYe/1hkdy3p1uVvf78nFk1fVbeNk0neOF";
    os << "5753le/WlVQ/v6lIz4Up58m03zGvacyjUUaEMtXZ3v7r0hw/2IxyYMrGNJNWcObuCpaf5URWFZ596la";
    os << "de7mMkAz+5dRmnnB/l3k+9SNKpsae/57yXH3zo0YeGk+nUxHF1zJk7V124cGpwwxvP9HcdyxX2HxkdL";
    os << "ZfLb4nK8TeAPM5Y38kigbFR4f8pLPtngOK7AOLb4obV1X7v9eeN21BXVQ5PnTZONZTW4YFRpz8RT69t";
    os << "39kV0XRxbrwQib3x8urAkml+LniPzkOPj9KecFkx20Pz5DrU6GQqVI2zlsdwFJcf/Pgwsl/G77TT35H";
    os << "k9V0wf0aMJ169i4cfWMfEWB8LVp5N52Cazs3rmT23nqwUo5wpEy8UGNo9wgevP4MtL+/hU7f8ibwquP";
    os << "jMiby8rp8vfXAa/QWb2+95g9tvWM7J02v4+FdfpCddIjy+Bj3qxf1rvkhWBUJVIaYhx1TUqIqIecBxE";
    os << "IaJ8Ou4rgMCZFUCj4xbcrBTJkqtH9uwsAcKaA0eXEXGSpbRqjWcrIVrgJAEVt5EAlybsbilR8bKWMjC";
    os << "QfIpCJ+GWyojdAkzY41lLF0XoTo4JYHQJZAchCMQeYuF1TEundJMRcRHqMvm+e5OOjZ0klrVx0aRw1Z";
    os << "AzxjUSBJDdSFu/sw57FjdwR9e3kFTSwWOAbYjUSwaBDQPimJi2iUmNFSTLrvsOdCNadi4roLHo1Fd7Q";
    os << "dHwbTKxIfiyI5GyBukbBcw5RzRUJRTF08km8wxOFwETRDSBcKRUXSwXQevIhNQFLoH++lNGNg5Qca2s";
    os << "MsWtdUadQ0h0ukSidEcNZEgMY/EsOEyd0IddRGdMgbZwThdvRmG0xaWaVEyIFYZoKZKJeLzUJQVKhWJ";
    os << "aHKQomQxKRomoWpsLzr4TAVFVejpz9PUGOa5zYN877OT2ai0UFvjY8W0cUhikOFhF1Op5/yJo4ysf4n";
    os << "Nb6q0zXDo7htGiHrmndzEPY8MsutwltY6g0TSZV0XBMKCG86vZO32Qba3D3DlN89mbdai1XWI1XrZOF";
    os << "oiKUk4lotr2WOVDIA9UB7zJNImVt7CHipiDeVwFUiN5pilwlNPXsqqV+Ncf9sz/PqH72M0PsJXfriZW";
    os << "66awQPPHEIqmrz422upmRFi+6YuBgyYWB1FD3koZ/s5dijOvNOWUBHWefw3r9IyYwGnnj6HG6+4nqdf";
    os << "ynDSVGgaX4m3ag6J/lG+9Y33UMhmeeGlflK2hVLq4pXXe9jTaTK3Ai5aGWPN5hK7Dufdf/nwxamg4qR";
    os << "2tbdvWbRoRmHK1Ia5upOtDFiHI8MZ097fzRv3P7rjm4mMmeeEafi83cV+52Sf40qE/ylg/C+D4j9wl0";
    os << "9MbmiAevo070VFk7N6+4vn5U0I6IIpjUFqItLRRN5onzkluHT8lAnRNRtyzJvk5/QVMYLhyVhylKGj2";
    os << "1i7J03tpMkMHzOpMl5lwekL6C218MQjm3j9zV1MDOqsPTTCRSuqeOChK9m5c5ALLn+KSROjFEc10vkk";
    os << "i2Y2kjZL6F5BT9xGHxFMavOzencvTQ2VtE6dxNHuHv7w0IX84Puv88Cjm/jWzUsxSw63/2QtcnWUcEM";
    os << "MV5WhbKCEdHBkXB1kr4JSreP6FJSYjhxWcR3AdhDK2Na4LkiKO/Y9ScKxXZyCjdAk1JAMssAqOWDbuA";
    os << "ZgubiOg5Al5IiKm7awCgZSSMMpuuiqzPjqIIf60rjCBY/AKToIXSAsB7fsIIV0JBccyyKqaXx2yXzeP";
    os << "6sNyzQol3w8/d1f8IvfbabqjIkktyVoSdt0OAZDhoVkWcw2ID6+Fq2thm1vHkIoAl2WSGUL4BjMnlCH";
    os << "HAihKWNxiXTRpnswTTjkwTZMyqaF40jouoJtl8YM2XUY7EsADmGfj4vOmEskojNql6nx+WmojVI0bDL";
    os << "JPOEKD76Qh9FEiqMdcQzTYPPuPg4cS1EZ9NLaGkJxFXLZEolcFuHaVAY8lA0oyzLX/8tJ2NkyhwcHGU";
    os << "nm6R9KokoOkbCHfNFE2DJ+v4eibaAh0WCWkSWThqAHOaizX1YoJctYjgyyTtFwsByLlKVQ6k/SProb4";
    os << "T6FsC2EU425+zC71+3l4KhCJrKA5Wc2MyFk8auf/Jnv/uZVhtMlZoyrp7YWsvkybWF47/QQekWIUFhh";
    os << "NG3S1ZFn455jSEEFSQ+STJhMmVZJfkqE11UfRdNEkiSEKjCHS5h7c8g+gZ2zwJIwRnIIRaaUKWF3xvn";
    os << "mZ05H95e45dvrOGd5G9//7CKmX/A7jLJNS20QIQnmtjQynCiRIk8s5sHv1wlr8MrGXiJyFE+VSW9fin";
    os << "998IOctjzEhy7+LY+vTfCR88bx/LoBrrvidK766DSc5G7WvXmYIfdUwg0q+d6jnDTOS8v0RaQyQ5Dq4";
    os << "pWNw+w5bLF0lsnOXYPJrr7Sjmq/Ob+7z40c6StSdiGiY9fUetcWbJ7Y1VXcw9tHnJ3oXr9bDPJtCaL/";
    os << "KDD+l0Dx3wBEFVC+/vlzWhpaq6v6drdHn13TNRCJRKbs3Wvef9astHzykvF4fH5apzXS1FLnepWACNW";
    os << "FQSkyeCyNHo4Qi1Sy6rk1JIczNFWCHqokmYvxqS/+lq99bwLxXSU2bM3zrVsnUDc9zGO/6ObLd7Tz0M";
    os << "+XsO5okCd+uovGWpuzLp7G/Q8dphAf5JPXzKS7x2Vaa4gX3zzMhl3DzJxSQyTcwIS28bS1jWPG5DoG+";
    os << "zrQon5G+kbYuWM/R4dhxLSIF5Ogy0gyCEkGj4QS9oxtgk9CBDWUCg9KREX4ZFzXxS1auCUb4ZHBAkkC";
    os << "EVAwE0WwXZSwBzdn4iogR3WsUXMsARNUEVgIj4rwyoiihVmwcG0Xv9fHD866Hq3nFxwOjmNrX5k9vcO";
    os << "MZtLkcZEUcEoOkgyaI3HJnBaWBdvoPNiNebCD7XtH6ejtJdZSwcSYn9+9vB+5LsQlkQipfUPscCxGsZ";
    os << "kNaKisCkfRClkwypiuS7lkoHtVAh6NKRPHEQr5kYUgnc1zoCuOLFwKpTya5keWFDL5BHu3K8gAACAAS";
    os << "URBVLgQ8gfJFXIUCyVOXzyF6pifpdOb6BrIMFq2mDu9noCukM5buJaJIxxcIYNhYjsWBzpG8IgS7Ufj";
    os << "bNo9SFXIT8Cvk0zlEBZUVigoskNyxKJYFtx41WK8suBgZx+GYTGcyNHbHycU8mMhk02liEQC+DWVULl";
    os << "I2bZpCSlU+fzslEx8rku5JJF3ZQxTYNouI9kSE8dV88aWXjq7PwUHXTKJY9y3psAFFy9iVrOHY1uO8s";
    os << "JrXSTyZfxVE1mxYjaV4+u44dN3smPddq45tZUsgpoIXDXHz4Giwj0v5iiMFmhsVOjpTVMqlDGRyGZsE";
    os << "oUCkqpy/Z0f5YncfhxXINkObsHAOlIGw8Tqy2GVTZTWKpyihXk0SbpnmKCQ+Ng1J4Gs0HloiId+czOr";
    os << "N+7lR3e9RG/3KDX1XtbtOMrp8+pZMLeZoYEiVTH4+eP7aZzUwBc/2MYf/3iA/R0ON942H6W4h29/vYO";
    os << "bvjCFm26ex951nTz2TB/LZ1SiNhhcc3M3v/vp1YQCSUaHE/QOGEya2sqS0+cRT/RiFiwqqxVcqhnt6y";
    os << "WbibuJgYI4dKCffCrD+m19rN0j8t6A+Y1CqTS6ctm4atfXFO/o6htdu/7gAGMqg+V33Mfd67emhf9ng";
    os << "fGfBYpvc5d/e9+5753dVnNNtr9rxpFjZc+fnt3jmT61ylVdR2puUH2nrZxD/Yw5WIZLMVPGxqFcLOGW";
    os << "C2SSCfKpQYS/DTd3gKzcwsCxXkq2h6qaKM2+IX77x328vCNPS9jh59+dRTJpsXZLP9d+bgHX33KUN1/";
    os << "r58PnN7Grt8iLq/bz0u9OYc6ZszjlvF2U4h0smFwBlk57Rz/DiQLFUpmlCxs5beXJ/OT+9cybt5jBeJ";
    os << "ZPXjmLBQvbaNKPIpc1RvUq1vXqfO++nzNkZXCFBLKL5NWQNIFSoSFXe0EWSPpYeY5j2AhXQpLBEi6Kp";
    os << "IwlWNS/ps5KFjLSWNOyCg7OWAbbcUCREMpYMTiAMB1cwGML7r7gM7x034M89Ls/s2LFBE5eMpGzF0zB";
    os << "aPGzu7uM1+vSaxvEjyQ5oy3Ewubx3Pm1Z3j2hX10D2UouSVAEI5W0NYcJpspYpgw7Pcwr2RRn8+w0zC";
    os << "wAEnSGRASGCUkScKRFLyqhqr78fhgzqQm/F4vqlent6efNzcfRNdlJMVFlmUkIWMaZVwZMskCiiJz2y";
    os << "dXUhwuM2VmDZd8/P3k+9oZ7R6lM25ypCtJzjRQ5LFOINt1UGUZx3QZzeQYzWUIegU7Dwzy2qYuAj6dy";
    os << "soIQVVCVkzS2TKFAjimzb8sm8jE1lr6eocZTKQxLJP+/hFyRROPPwyuiWUXaVZULNMERaIppNOvqkjC";
    os << "ICQgZUgUbQmETLZskc661FSoHB4W3HKaSjjcgkeTkKOC3R0Fthwpc8edH6VJy/H4b1aTFR40xaBi8iT";
    os << "Ov/hUfnjvKn5570NcvKARb0MFL+7KMslbpqfoYFljda2hkIpXkzDyRVRFIVMu0DmQxVYCNF09h4RUQt";
    os << "YUhDWWgCNnYvaPNQkQ8OKMFkjFUyyqquQ9s+fw8PMvc9dtc3j0T304boSPfqSN2ScF+MznV/HiS13Ew";
    os << "jq6pLB0wQSGM2kOdo4ybtZ41v5lMS8/vJ6V1+7kzHMmsXhamB/+7ADXfXgmn/9EkKcf3sOCBfUgO3zh";
    os << "C3voNHXeu9DDlZfMYigTYyCRxqdBU1MUnRRCiWCne4g0tOEPyWjBJhRhgSQIRHSE7NC5dTev/WW7m8g";
    os << "5xXgy78QHLeWc904uNI8PlA0R7vjFr1Y/+uza0V2MySYcl2I9Do7HXevjoGj/j4HiPwLEj549+8oPfy";
    os << "j0/Vf+vDWwfkuZIxmHhTNqWTijmpOWTWbi1CqGB0u4ZgHbskkOZciO9iIpAdKpLB6PoGgo5LMF+kbK1";
    os << "MaizJwcIFYfRtYVRuP9PPrEMQ4eLLO7t8ydN9bQ2WGwYbPDyrMk5iyqo7oqSP8xha2HRhg/roLd23ox";
    os << "Slku/OgyrvzIJm67upF585vJlgXCzLP7YI7lS2voHRbc9JWXaRo/F2/Qz+QamVImTb5ocObJjZxWnaB";
    os << "h1kTybWfzxIYdfOOBJ7BMCymkIvkVVJ+GqNCR9LEuBNdxcPMWjiyhBBVEycL2qLj5Mk7RQol6x4qxCz";
    os << "aSAMcFSbjIioRVtBCKQPIIXEvgFBxcXGTJ5mOnnIm1bh3f/u5rRCqbkVExjTTeco6mCRCtauRjpzVzy";
    os << "odOxlsxF1XxQMcWdu3rgGofkmHi5m1+9OBufv/8bkwhEw0paK5MzrTJSy5LZGiSLHYWTQ6ZMrqwcbGQ";
    os << "Vc9b5291bQ3VFUFaaiqQFRlXOORzebbs7gbhEg4HyORTFDMFLEsiW8jRUlfBkpMnUxzO0VYr+NoDN5G";
    os << "3Qsglk0hdK4X0Mbb9ZS1OqYzwqRglA02V8XpURhIlVr3RyXCuQCzg0hD20J8q8IvHtiIrgimNEWzHJV";
    os << "0o4BgSui64YPk0amMRErkMPccSZJI50qUiiWweoXjRVRm/VEQp2viEwBfUyXsUyrZFtT52cGVKUDBcJ";
    os << "MVD0XHIFyyKhk1VNEqVX+GiU6tYd7hAS0imsjpMfCTPw6u7uPSTH+XzF01jxwsvsKM7j4FMbSTA5MkT";
    os << "6Y608bFLb6CGMpMmt1CwCsR0h4Kjkki7qKpLNOxBwiWdLo0dlo5N++FhaiY1Ujq9ARFWcIsm5oiF5Ix";
    os << "JxbquS/HgKGrER+pYnNasy4MP3kzv4AFaG4ssv/B50oUCtZEKfvmT82meaPPik8eY01aBgUwoIPPSqk";
    os << "4efOYIG95cxj3f2kY6LjN9aQsep0x1VYi6qS6lw8O89lKc7riLIxmceVoln7nzCFNaApy62Mcl5zdRW";
    os << "dlAKl0iM5Jn084kmUyKlvoQ/qBMUFNJF0tUVfrIpw1CDc3UVvmQVBVF8ROr0ti9q5ejh3p4dlUf2w4N";
    os << "MX+8RGuDS3XAk6s+9Ts/ue6Tn36BMZGt45Ksx8HxRGnWt2KM/15g/K+C4t+p4Z092TO710Ic7FOs+rr";
    os << "xd69o6z7pzOWzGN9WSSBagVEYZqB7mMRgaUyfOCxjoJMrlnEsgT8QoLoqQCpT5uCRLFv3D9LZVyIc0P";
    os << "GoGkuWtrFqcx8tvgzJQZk7vxmhw/Hy+WsO8PFzgnQfLfOeZQrj5k3lub8kOeO0JnLZMj/62Va+dtsKN";
    os << "u6L88P7dzG+3k/Rlgh7VVRV4VBXCl3RuPxD/0JfX4E9B44wobWGYqbAaN7icGeOQEzj9Locl04vcsZX";
    os << "V5LhYh55ajU/eOUFhP7XIumYOlZLFlGRJRc75SAFJVxVHmv6V2xkRcfKWOBaaLVBrEwZe7SA5FMR+lj";
    os << "PtJMsoUQUHF3CHTaxcZD8Mq3+Kt4vZ7ntS+tprm/kcPcRKiorCfnDRGMaFZUBDndmGekfwXWTuJLEKS";
    os << "c3c/NVp3BkV4GfPLmRci7PvNmNXLl8PMMjGX7/Rj97Do8wlCkhsPGqgozpsExX8GCxqljG50hj7r0s4";
    os << "zouZcMiGAwxbcp4GitCGEYJVR9r3zvUPUAiUUTTFHrjwySHE0gC5k2t49SlkzmwZ4BLz57OZbd8m9Hk";
    os << "UazcKI1ts/narZ/jgYfWYBs2KxZPoaIiwNBwnl37BkjnU3zr82dz8pyp3P3LzWDl0WWb1vEx2g/F+dM";
    os << "LO5Bch7BPJRj2UShZ1FR4uemSUxgYypLI5MlkC4wksxjlMsNZA8N0kFUIqzJG2UC3DbRwEEcReGyTsF";
    os << "/BcU0yZSgYUCzbIGnYSIyki0SCfjSvzoLWCLIsU8wZJPIm9XVBXEfhiVf3cenVF/P1W87kLw88RU7yE";
    os << "PVq9Hb30jRrOYsvuIyFixdSHEizbFYzgYiMZFoM5CWQBLINjmNjYoDtUCqbdAykaZ4xn+RMGyISlFxc";
    os << "e4wxuskS1rCJg4OTLJHqTVEnWdz71TPpzss8/OsdnDLPR0kt8+ifegj7fBjYzJpQQcmySeULyJLLUKL";
    os << "M1ZdOZemiam67bRV3fvl0XN1l7/4BFs3xsP6Vg+zfJ/joB3xcescIh9qX8uiP9/HwMxYzp8HOvghnLK";
    os << "v7/1h77z87yzr//3lddzv9TJ/MJDPJJKRBQggdQhcREBEVK6664uKKDdZV+egX69rWvioqKuqKKB0Fp";
    os << "fdQA0lIz6RMZjK9nX7O3a7r+v5wcJd13cfH9fH54f4HTnnf7/J6PV88++Qhyo0qvu+zuj/LcUe1s2Sg";
    os << "hfaMw/BoERWHKB3Tkk/goigUQvJtadq6sixc2Il0s1TKDcYOT3HX7S/ywPbkZK1W+O7AkkWJ4alCw69";
    os << "XB5VSBf4zgbD2isL4p47xP4C5f21R/JscLX/WJf4XL/GBubhaqsWxUc45/+cdXW9Zv2aB09mTRdoOs2";
    os << "P7GZ8KGB0pIC1Nvi2Hb5LksjkOHXZY1NnB4EFFEKeolBP45Zi+vgxnndjHSUe2kEvDmWcvYqZQZ/Rwm";
    os << "c9/eAm/vXOeY9YtYXBC8fSLmkvPTJFqb+exx/az51Cd+fk6O/dNsagnyXzV5fYHJuhMg1YOyxe20p53";
    os << "mZ6tct7pi7nsTRs4/7Wn8eVv3kU26bCoO00uk6a/M8fSbkNca7BpRvOrB6b51Y+e4u3LS5z/9it441l";
    os << "v5OHNGymLACMNMikwkQLXwupIIHyDarzsQqkrhCMQmQREEdQjdDUCz2q6UAKFqoRYGRsdG9AC02j+Kd";
    os << "pSCd7buZSPfegu1q1by449O/F9hWMlKJXKDA+PMDNdYKC/iyPXdrNy5WIaocMzzwxz8+93MTw1xcffv";
    os << "paOXIanNw9z3Z1b2DtcZsOaTjpbPfq7PGqBou4bXGlxKITDUYSLxkgLISWWMWgpcG2HWqNOJuOQS3lU";
    os << "q1VacnmMgQWtSaaLVQ4dnkEoOPn4fo5ft5gLzljDnt1zvOr0ft736a9SKMxRnh5lYMUKvvGNr/Hlf72";
    os << "T1SuW0dnVzqYtB9j44j6GxucIpcJ2bf748G6WLOzkvA1L6WrPMjZTYWK6wsnHLWJFT5q4YTE8X0HFhp";
    os << "ZsgsWdnQwsyDFbD6jVA+q1OrV6xGyhQoQBaaNDhSsNbsKiFIc4doJsQtIIgqbEynVoRIowAj/UIGw0k";
    os << "iCKUEaRybgsbG8jiDWFWoDnZQljTRRLjljYwh13PMamgzEf/fibmNx3iJKyWLqkh83PPEp19BDf+MEP";
    os << "+cmvf00wUqC9u4NIKNo9qEdQKgdEQdwU5ikQRmAQTB2eoHPpQuqqgdXhoUZCTCVC9iTQlRhV18hGRDV";
    os << "nsQSHVT0ZfvmLjRwYn2Hr0DxOIsGxKxfwhvOW0dWZYWRijmOWtSJFglolYEGbzVjBYUlGoGQFzykyeK";
    os << "DG88+PUpmucOzqbtYdkeIjP5gl0ZLgtRcs48ffPsAXPn4k379tguNW5HndJQOIWo1zju/g2DW9tGVS1";
    os << "OYliWQX+4YCalWXjlyOiaLL8v48xZoml04Q1BvMz1UpV31UUCGRsBBa0NefIe/47uYD8dDEzPRkFAZl";
    os << "R0qUMX+qRa+khP85NdwA5q+tdX9TUXzZvvfnXaJzxklLW3/2089e9NwTm360Ml29+Nj1ntvdvxDH0hw";
    os << "YHGFirob2YwaWtDFb1YR+kvn5LE89F4FKMlVK8dO7t/HIsyNs3j7LnvEa+YTNyOQEtz82zqbdPvdsnG";
    os << "TVwgRHLU6wZfcUj26t8czvD/Gv165hyYldfPH6CSZqORYv76B30WmMjm1loM+mVPNQjs9jG8dxpUQ5D";
    os << "rlsis17ppiar/Pet6+lb8lSntkZ8vD9D3L6iauQ2sJgcJJpYq3IuiEL0ho36XJgJub6320hs+VmjumP";
    os << "efPbPo5tO+wc2UfkSAibY7DMJImKAZYNJhSYEDSqKcRtaOJi0BT/tiYQCOLpAJlykEkHVVFI10K4FrZ";
    os << "ncemSHr5wxa/p6V7J7OwQvh/RtaAVIww6jqk3YoqlKsPDY+zeM8L+A2MkPIuFC7pYu2oJM8WQX/1xN7";
    os << "mMx2s3LCWTTrJ9/wTP7BinUm/QmXJY3JmlYQRhFGJJQT2MicMIY14Wk1s2SIGNIIoi8pkMEqjWGqTTD";
    os << "nEQMVcs4ziCto48Z5+0glefciTnbVjDnuE5okgx0N3F2hPbmRobJZHO0NJxJJ++5lqGRsu05LJsOONI";
    os << "vnDt2ynPBVz4xhOpzFUxPkzMFihWa5y0djETMzW8RIJdg6O4rssJ64/gjFNWs2LpQp7deZBuJ8GygU4";
    os << "s1yLymzEPM8UaE3MFAiXwI0AaXMeGSOG4kjgGbWISUmIsMBiMEQShJggVsZEoLYiNQmGwLZtGPcZ2bI";
    os << "5Ztoh6HPHivikafkhL1qauHI4e6OSRh5/m5ocGueaqi9mz8wDK9lh91HIefehxzHSRz37v83ztlzfhV";
    os << "sHOJjA6IGHZRNLBcW0s42JLiZKKjlyOer1KYVLTuqybRhyDUoisA/kkaszHXpjCO6ObyqOH6cgJ3vyO";
    os << "M3hy0yANo0l7rZx81ELe+46V2I7F+iPb+dWdu9g7XmZxbysjU2XiIGZweIZ0b4pqnMSSdR7aAicc8yr";
    os << "S/TVufURyw/1FrvjQMr787h5e+5Yn6VnmMTpVY2BRHmXbfPfng2zfXeSF3TPYOmZqRnDf07O8uGOGB5";
    os << "87wMSMIGkvIAocXtqlSbtpHBfqKPq7M8zN1pmYKBI0qrTmU2jbw44a1tihufXrjz5m1VsvO9U89+Lgb";
    os << "BTpPy9+mv9+aDGA+WutgH/T+Pxyp/hfxuZrrnj1CWcdFd742a+90N7da+TatYtY2NfBQK+gMFel2lDo";
    os << "2CHpxRgZs+egx8YnDAeKJTKZiFKtwmyxzmkn9XHRhqV87vtPksm6zM42sG34l48exWvPX8f8nKJcGGH";
    os << "XjsN8/aYxBpb0sLIjwC5UuejsPla9/hSOP/e3aA2nnHYcJ5+ymBfuvJPFC7tYf3wrvtbcfk+drqRHVU";
    os << "X096bYfaBATUuwbf75qjfxhX/9A2tX9ZNIWMwXC4CFLZtX3yiOIAwI45AXdkwTByXeu1rz4UuP5oiPP";
    os << "cDObQ/zrrt/TCWOmkCH1gSWtDBRiFEWVsvLnMUoQKY8CA1aGOychymHxPUI0eJgGhrbSBASTcya/g7C";
    os << "nz7NE894LF1qGBmbZEFnK5GGYrFOsVTCEgKBBDSx0mitEUIghEMq43HsUQNI22VkqsL01DxHLsmzemk";
    os << "723YN89L+KbATeJ5DeyaB5VoYrQgDw1yhRhQ1yGQcbONgux6xitCRIVKalUd0sbizjZoKQULkxwz09d";
    os << "HSkcF1EizpzDE7MwtJyQmre2jrzdKxpA8rzuImsrR1d1EL6nzn+zfwra/djOt5fODKN3HkEYt44unt/";
    os << "OyGe0gmbO6847PIUplv/9v9tHWmOaKvg937Rxg6NEvv4m7OOHE5ScdhcPcQz780zsnHLKAeKmoVH88V";
    os << "TEwW2Ts2hZAWUjjoKCKTlkRhhDQCSxikVmSzKSwbXKmwLZt6rKn7MX4o8MMQO+ERBJIISCdcpK3o6+m";
    os << "iNeMglMXWwVmkqdLb001sJVmQgZsf2Mo73vd2rrn4WO5/bjuLF3YyX6uw64VdXHrBG9jYPs3HXv8Vzu";
    os << "5bQnphiqwLlXrA8GyNRiNGxzGWkKQyLrYl2DY4zUlvOJdd/jBuu4fbmSI+5BMOl3H6s8ikx+g92/n7c";
    os << "9by3Rs+z5Fr3sOpRyxiaW87/3bbI6QtD58kbS0uq7scLC+BlU5QL9fRLrxuQ5pp32ZyfIaDY9CxZD3D";
    os << "+/eybfsg6VSWlx4+nWhojA9dtQt3WZ7xyZD94yW++P5FHLF6CVJ00dPrcPM9O/nqj7bhWB6ZtKRYCvj";
    os << "Op87mKz/bxORMnbTnkMmnEX6eI9oSnL7BsHiZwY5twlgSRD5t7UnSboKJkmBk/xiPPTFG0Xf01z+9Ye";
    os << "7nD/g/v+2eJ7cBpZefMs3s6jr/OUr/x37xrxmh/9ed4l8anRe1t2eUNtd879dbacnJaMlAZ+vyVR0sa";
    os << "jH4tQpDYz5RQ3HUgMeT2+vce2+VqdmYiUaBRFZz2toUkzNV5so+H/rwKXzwyov5yY8e5silrRwcL5FM";
    os << "uoSh5N9v28YvbtvJ4SGL53bVectr+viny4/m2c0z3PjkHLc+Ncuq1jrf/GA/f9yluf4nv+G0lRm+cds";
    os << "BelsdurxxfvNglfmq4qXxGYZmqgyPVhmZKzNdKBPWfWamfFpbDK6dRMchdT/AQWBijR9H2KJJxQ4igW";
    os << "sHhMpmv5PhlgcPsPvGH/Kmj/49bzl6PS9u2cRkIwRLgG2j6jF2SmCUgSBGYDWBDo7ENGLUdIgOFdaCJ";
    os << "EIZLBvIOsT1iHwqwbs7lvGD72ykuzvJfGGOltY8tZrP5EQBKcDzEs3DjgRh2Ti2g0DiumkcW1KrlRke";
    os << "nWR2vsjSxd2sXtrL5GyZobF5lvd30ZZ2KVViMIpGoFGq2SnZtoWbSGC0II41jtMk92ilkRIaDZ+6H7F";
    os << "sYQt+rNFxTDrt0draxvxskc58hvYFHVz7zZu46Ix1LO5r56jTVlEqZkjaNpblUqxMUhrczqvPPJ73XX";
    os << "EOTz6yjVvveJydO4Z49NEt5FtT7N31W6b3DXHLLVtIpS0y2QTSsvBsgU3MY5sOUK5AV3uSYqGKlxQgB";
    os << "HVfEQUhKo4ZLVcoFxtYtgvaIGwHrcCxFGFosKSFxuB4ElvaL79UDJrmHzQMIxRN/7JBoNEI2yZoaFqz";
    os << "GebKDQq1Kj1d3dQaIeVqGS+RoxEZVvS08/v7NnL8q89mWXeWrbtHKFRi1qxayNDoQV7trOVAa4Hdmw6";
    os << "QNykOFyuYKEIYicbCcZuoQa0Fnu1SKxfxWrpopELIe1A3xJN1vNO60IcrhNMN1GiZH159PBt37OdXv3";
    os << "4GWWtw9879BH6IKwzG+ExXK0z4MF2ps3P/OOOFGlLFvLR/BjE3wUS1lYGVR/ONT6/nrHPP5fGNL3LPV";
    os << "5bz8CMjvO7qHewvRpy1voVPffhEKrM17nrSZ3hI8fM7N3PHffuYng2Zng/IJWwGetMUSxE/ufG93Pmb";
    os << "JxmdqtOVd3j18Z0Mz9WYLhVQkWT7lirZVpujBjwOjZapVmMStqazzaYe24ShNJNlxn/6uz2zRy5b2rp";
    os << "9cGiQZlf458Fb/y2q9a/pFv9WdNgrXSv26Nycuf/pndc2DB9Z2OOO5/KSFBWECNlzQBEHmv6+NN+4YY";
    os << "TJSYvhyOGhbUOcfmqO391wLhtfqrN/qkLa8+hqeMwc3k0tVmzZVyKT8dj8+AdZf9xqXhosMzHn47t1v";
    os << "vWBRSga/Oz6F9i5vYxO5rGTC/jSb8b4+nU7eeBX5xHNPsnHr76Gu7+5jnWvPoVP3FBly5DP8HyA1d+G";
    os << "SWaYCxxCyyNzdC9BS4JnhgY5PDbDyOFhgsDH1ooorGNLgQpiSpWmALnmBxjbI5d2qM0EHKqlufFQlVe";
    os << "tPY/J4a3c9tlf8PcnrkPUFfFUCeE2CwkYZIsNtm7ScYSGuEnVthwL6TmgBLy8h3STDlcdfRQ3f/52tN";
    os << "NC3Z/H8VxqlQbT0wUQAmM0tmWRTKdxHBuJRmuFkAbLVlhSInAASbVa55lnt/Dks9s47aSVDCxdyO7Ds";
    os << "7iJBIs6XTpa07S3eEgREUaGMI5xJaTTKZQyKB2hiTAvW05bW7I06nXu2bgDqaAz307Cy1KrVdFRQBT5";
    os << "bNlxENd1+OIPb6deqDM/HeNYMb7fBDQYLdEJxX23P87vb9rEtR88nzNPWcWuwWF6F2TZt2cjLzywkR9";
    os << "edx+nbVjAUat7caTFfLHCyHSZgSU9LF3cyqaXBjFRwHytQb3eQEUGHYa4UlKpBkxMlJq8g8gQBQqlFZ";
    os << "ZlUMrCsh20BqUhCBTxyy+XKNZEUYzWCiMktucSq5ggUkSRQgpJrBS1SkR3WwvFUoOhiQlyHR1o41GtF";
    os << "KmHimKkOWrpAt7//mtZftwpVCsNasUSsXERySS/2/k8H/m7SyjmNcWgQjaZItQ2mYTEEwalDKEKCP2Y";
    os << "oOGTTyZoTSWbcNp6BI7BO6UDMxVh1Q0FItYv6qBlWR+XX/lLjGsY6kmRWNdPtrOTBgLT20Z+5SKULak";
    os << "YQNoox2HfXJ3tg2Xu32746lcu46Mf6uX/fOSnJOoz3Pnd4/iX63fwxV+MI9wMsZ3gqWer3PDL3fStkN";
    os << "z4T8s5VAgZPFxl54ESJx2zip1bPkbswEv7CkRK4c+OsbSjkzOO7mNkps5z+xo8ee+FXHJRJ/e+MMShm";
    os << "s3mHRG/umuS1UvbCIKY8amISrVGZyYmkQ7EKev7pdLWkzfd/cgT/HeU4CvhuK+MUvirMGP/q07xfziw";
    os << "ePl8InXi6pZVq3pTP+tqc4/Opg3IFBe+8XJWH9FOZXY/W14q8viwp1YtT8trLutg50jIqesGuP0PB3n";
    os << "0+QO0ZD1+/IU3cOyaFu55eJBayXDJ6Ys4OFvl6iuO5wvfepjxiQq5VJ79RfjNvWM8uaXGfgSh7XLa0l";
    os << "Y+/dEePnPNat796V388Pot+DseYsnqBNf8ZJDf370Hp7uFXHea1IIclm2T6kiRaYdEIolsTeKkEthCU";
    os << "qjUmBufoi3fghTghyFaaxxbEscxpVKDUCkalQa+jjAKurIJIukwFcIdNz2C3PMIV370e0xMH2L36AhO";
    os << "uweqmdmCK1++j2kINcISyIyFqsfoQCGRiIZC+wqpJW/qbOGL37yX2FQJGjEqFnTnHAZ6O4iFwagYoUL";
    os << "iOMSxLTzXw/YcpJR4rk3KsxC2TRypl9+ChiD0eWnXQVpbsuTzrfT39hIYwb7hCWzLJuMlMCaClzcl0p";
    os << "JooZouHSRCaPTLrEHHglo9YqZaZ2BBHmU05WqNhGsRYKPjkCvfdhr3PrGX445ezOpjeohQ+JUIYSmQR";
    os << "RqTFcIwZsvew4xOVLjkvNU8/txB7r/lMzx8733MTM+xdt0iCsWQmdkKQkCtWsP3I4LQMFGsYaNwHBdQ";
    os << "VEt10rkM2jRTEw9MVSiWaniegxEWYaSRKGxbICyLWCukEGitsR2BI20sqwlO18Ju2iU0xCpGChctJGG";
    os << "saMu1Uvd9JksV2lvytGezzBTKJwISzwAAIABJREFUBGFz12gsCx1DGMUkPIc4jLnlvse5+vJLuO6n9+";
    os << "Blk6TTKSbmCyTnXJac3MODD7xEh0xR1hFVPyKKGvh+TBgohBAECkQywej+fcyPF0ktaW+yFjVQV0QLU";
    os << "pi9c3zjredzxbceYHKqQNeGFXiJFFQCdM3H6+9AhDHqcAHHkSRcF8sWyFiTSHhILQiUzW9v38h9jx9i";
    os << "9eKYR26/j/d9eRcjRZstj5/Da9el2XdIMYzD6GiZux6a5OcPTTLtSBLCJfJ9PMtwzvkL+fWvd/CGM5f";
    os << "TKIWsXdZLpS746rfP5fvXP8/MXAnhC5YvWcjYxByf+vtlDE5IHnhhWi/O18Wi3iRvfMelZFqPZPfeUc";
    os << "J6iWqlmhWRWdYw8qDvx38akf/8+Ut0nf/rweVv6RRfeWSxV61akrryTSf/U+lw4R5pwoG2zhamp0tsf";
    os << "W6E/JI1tC7N8/Tz09y+JVGrlhpPPvj4dvOl6wZ5/fkL2TM6zX1PDyElnLRmEVHY4P6to8SRx/FHd7F5";
    os << "X4Er3ngM3/vqNl7YPE5KSIwLSW0gmyHX30JhChra4j2XJ/H3lTjmVU/TsjzPDz5+FPnFfQzvt7j10xd";
    os << "z9Lp+ZBDhLMphdyWx7KYnWPS2YC9MIXyD3eogOhMkBtrAdjhw4BBBHKINVP3mmOjXQmZL80zMzDJeKO";
    os << "LXfLQJ0ZahLZvCszymwwzX3rKdT5x/Mp+79CqWtrUST/loHSKMQc0H6FhjhGky+1IWMmsjXLvJz3Obh";
    os << "VMKQyY2dJBD4+AYeNsFy7np8+fynguPolStMDExg0TgJmw8S2ILSSLpsrCri/7eHlpaciSzGXq6Wuns";
    os << "bMPzPCzbASFRseKFF3ex9aVdTFUqLDtigLNPPoG5QomZQhFbWsSRIggDTByTTmTACKTQKCUwKkIKgZQ";
    os << "OKS9NrRJw9+PbmJ0q0J5NUq7WMX4DP4qoKUN3ewvVqkLNVgnKBULK1Ovj+FNzDA0XGJ4p4AqbodFpHn";
    os << "/uMP/4jnO57ro/Ml2YxU15lIoNYj/CS0py6RSW8IiimL0jMxyxsI2zT1jK9FSRONQY0yx0Fi4jc1XGZ";
    os << "+dxXJcg1hgXLFditERgI7HBCCIdI6RFFBuUaHaNWguaX4qNUqBNM0JCCHCkQyOOkdLGdS1GZwoUShVa";
    os << "kikaUUxkDAlp49gulUZIEMe0t2YZ2jvMVMXiQ1e+le07DpPzMqhYs/3QEMe2rEK0eczOl8nbzbxrhUs";
    os << "u7WC7NrFoEpiCIMT1ErilgPZpj/BwBVPxsRYniYsxZ711GXcPH2L3/sN0rl8MlYjGwSnCyRLyZT2sqQ";
    os << "eoMCZuKBqTBVQjRuZcjAKvNYMOG7zqjBXcd+ObcO1Weo9dyQ+uWUfsB6w96SFGxupc9d4WAl9xGI+W1";
    os << "hak5ZEshsg4xBKwcfMId33nAB9483FYTsREOWLhSpeHXtzHC48Nc8Yxi5HS4oZb9/HQxv188LJl/OA3";
    os << "+9h74FAoJDt+fG+5sG/3JMmOPP1r1vHwXVupBBaL+trRKm47vj/7j+duWLNaSvmnCIVXwnBfGXHwXwK";
    os << "y/p8Uxb8k1s5kMvY/v/+N59z++xfeNxsLtWJVK7V6ncGhKtnuRVh6mg9deSM/ur9kJifGHgvqVR2KxZ";
    os << "x6ZpbCVEhrSnDvDafSkk/SmvXYsmuacs1HWTE/vH0bK1Z1ceLaHJ/96f3kkhZp22rWemFwMwm0scm3J";
    os << "Ei6Lm/5yDz/8PMy2f4ML376WM4/9Sh+tllzyO3i+RcPQXdEx0ndkGna72RvFrsjiZ1ykR0Jkid3gi2x";
    os << "25uOlOSKHmJLsmPXIKVSiVDFTBeKDE9OUK01iKKQbDbDquVLWdDVjYoiEIr2Npeko/Da27l31wzXXX0";
    os << "pP/v7y6EYocoRcaAw5ZcDqpIWdtJpjsyOhbMgAcogpMAayKED+NllJ3Pnb1/EcWNuuPGf+Mz3vsXN9+";
    os << "/mS796hrHZOuecuJyejhzCcrCTHtKGMAyJlcJYuulwQBLG4HgO+dY8+XyeTCaF7SUQSGq1Oo888gzbX";
    os << "trFitV9vOuyN6CUpFSv4SUspJQoHWNQWI5NGIQkUgmMUQgdYUSz2/IclyiK2Tc2S8aVOJbDxNQs8zNF";
    os << "RkaKRErzy7sfI5vvojpSIhX56Ik60+N1QstQmfeJUXR1tpDNJEklXLp7WqgUY2bmK9T8kOlijVLBp+7";
    os << "7OLYkk/DwVUSjFuCHgnTapRH6OI6DGyu2Dh3mwNgkjoRkwsOEmrihmvInS2Jkc1eqdZNFKEQzBiBUCm";
    os << "Wa3bCKm8QiIUXTnaPAshykkGhtcOzEyz9LTcVX+Mrg2g46lvixxkpZCBvKlRDH9shmsnzy89/iuLVLu";
    os << "OKdryOXTVEuNijMVJidLnLuB09lPKpSKzaQtqIaxPihImU7RGh8mnttY2xiBN2ZTlKJFFgQFkIWXNrJ";
    os << "YAw/f2IXS649Hbc3S7BnDm9VJ4k13STW9zSBxIFFaqCH5NEL8Ba247QkIZQkVnZgeluQC7solsu0egU";
    os << "K8w7X/67GW65az6+/cyJYind9Zi9vvnoUS0pSlkSkHZJpFwATGpKi2Wp/5pePsHp1guOPXkKxXiPXme";
    os << "VVp/bz1PZpTt3QSz6b5pZvb6ArA7v31jjp2CT1WiTrNb9UqLLrX+8I4i9c9WVsUYZWyfBIGb9R57yzu";
    os << "9lzcBpH1S89ce2iZfzXeIO/lN/0V43Qf/X4/AoZjuBlCU42m0329SxYPjI+/1h/f8+4J+rHBUGd+bLg";
    os << "oovW8vzv7+KbvxnCtltNLh2PnX/mwGnnrcnbHRmFDjRnHpmj1vB4Zk+d15y0kKlaxAWnLuJ3D+5i664";
    os << "prnjH8VzztY285vgu0u0J9k0LvNYUdquHECA8B681QeQIEgtSyOOSfOesHONjHbz1J7sIV+SZ7EhwYL";
    os << "4MLaCWtiBcgchaOH1J5OI0Ujax7labi1IGaQTCBoXGzbRg+zH1ep0wCCmVKyilsZF0deVYs3QxqXQOr";
    os << "Q1BGNEIfBzpINGUyw2CTBubtx5kbescM26a0fkqQgiQLyPCBChL0uYmsOsRoSPQkUYEGh0J5EzEZ193";
    os << "Op/5+M0sOm4Jn//oOznrVdeCmuUDFx3JyUf3s6QnT9X3sR2rGZUqLGzbpl4PqdbrqCjEsR1iZYjCEK0";
    os << "NyZSNl0iS9FLYjoVBEEcRk9NzzBWKHL9+OcuXL2fL1l1EUYSXSGC7skmDlha1ekDCtXG8JNVqrXl80S";
    os << "CkIuHZVOoNxsYrrF3ew8j0PLoRkMrYrF+7jHse2UJ70uHk41YSNyICpUjkEviVCK0kLdkUixa18/yOg";
    os << "9SqPuvXLmFBPo2WECmoViL8ICCRsmlrS/L4iwcIGhE9HXnqfoSKYnw/xs3a7B+bZ/fBKVzXwksmQEpC";
    os << "FYMQONLCcSRaacJIISwBElyruVu0HYHrOMSxfjkaFgIVoozEcl0sCX4UI6WFZbv4jRBheziujVEKjEF";
    os << "J2dz2C9BKEgUxtiVJpTyGRibobk+zeukytg2NYicMpaqPYyzaBlrYuH0n/riPl3KxLIsoFqg4wBhDEG";
    os << "gkAiUV1ZpP/9IlFNINlCNxshKhLKo+9LxuACtrEW6ax/Y87OPymKmIeKqBvTqH7MjgDOSw2l2czgwi6";
    os << "+I4NlZfBiflErRmUHHMtlsHuWVbSK1e45ZbD/Oxj57OP77d5vpbqsi2JJaOcIwiqsegVROIHEEQK44/";
    os << "tpMNa/r59R/2k8km2Dk4z5ZnZvnwBzbwm3s2897LlnDbHw9y9dWrceuGg/uqrF6cZGFHWvb1L0sNT5a";
    os << "rjt3a8dzOebsvMY7xuhg/PEk+CzNTqOmg9YnBg4dHi3NFXY+Z4D8hEX8OivirR+i/BTL7H51ivV63vn";
    os << "HdTU8DuYt7jzkj8Mso7dDZlmbn5iFu3zjFB9+3ARFp2Z+PzhqdmJJ5p8K1P56mpzOLrvu09FSYn6tyz";
    os << "8NTVKI6/3zFGuaLCmkEV33+AT73zuNYuRTe+vW9WO1pbE8gDFgpCZYGHeFqgwljxKDgZiVwkoc5/YQc";
    os << "c47hJS1pHJEjmqoiMNgDKVQ9bkpVUjamXeOQQNc0dpuLCTSEEidvExYDsouW428aY/LwCD3dncTa4Lo";
    os << "WHekWyqUq5cYckVE4no2uQTls4Hou7XnJfLFC0c7zte88zqc/dSE7ZgVRHCOyFtKTzbA/o2jP5DmhIt";
    os << "i0bYJDi3NEKiQ+NE8uZTFRaLB/Ypx/+eJbmB7cyalyiPNedyq7hma4+YFBpss1WtMebR0ppLTJpNpxP";
    os << "EkYBVTKdSIVMV+ukXCT5LJZtDHEUYTnSownSSYdWlqzzE4VqNYq7Ns3wi233csbX38er7/oPO78/f1U";
    os << "yhV6FnSilKLWCGhpyVOrVsnmLZxkGr9ex0skEUYgRZOiM1WqsH3fBOU4QjmSoFon39vL8Uct5hNfu5n";
    os << "7ntpNWy5Nb3cbc0WfZ7ftZ/VAN6NTc+w9OE6t1gCteWlwgmX9HZx45CoWtGdZO9DGbDXmmS37uPH256";
    os << "nHEeesW0JAUzztGoFjW0zMVHnp4BRtSY+Ek8RICDVoy0FI03SzSIklBApDFAtcS2C5NjqSCGFhhGw6i";
    os << "WTz0qxMM77WkoIwUhgpyeVyBGFEoBW2AstXSAyxiolt8CyXwG+gAo0RUIkCWpIZhLAYnqpwZK3KvgMT";
    os << "LOrJIS2olgPadzsk+tLE4w1EQ4JnEcY+oVEkbYkjBUZLMDECCMKQuNhA2x4aj2CkRnZ1O7rcIC7FuKd";
    os << "0Ilod9LiPXC4QCQvbdrEvaEXtKqOLASLn4GQc5DKLaKyG3ebRVlIo4XLTTIzT6fG9V3djm5jKSIGnN8";
    os << "5ikGSyEqFd4iBG2hEyFhjLAjcmjgXb9hb57i9fxdTBGn/3uQeI/IgnN++nc8V7eP7ZMX58XRYXiztve";
    os << "on6lOLW58v89omYKy/Ksaq32Nl/cX/74oE2sX/meL748yf5xwsW01A2vpZoPGlhsoWG2kRTiuP+hed/";
    os << "3Sn+b4vifxFtNxoNCdjHL+95z8GDe99w/NoMMzMxjRAG+nJ88t29JFs9ZkenGZ0oSVPzue7hAicttVn";
    os << "R5/DlW4fxXMGrz1rMs89N87Z3HMWECdi+W5FPepy4rI0T12e44BOPUS9p2he3NwELKQdhW6AEstWGhI";
    os << "OlFYlcmmf6XHqCMv5okVwY07ZhBY2GQiSTZI3FJ447DVOoE3ek8LweHnvuEZYsG8AEitrcKO05l0OzA";
    os << "Y/sO0zJEtTiGmrARo8lEdqQT7tIx6PYqKMiTRiH2I5NHGuUMsSArRUCQSppqEZQSHWx9Zlnufo1F/OV";
    os << "F15C2gEyn8EIG6k0h2anOGfl2ey97sfUzCCnfPIctgyN4pyYoTWa5GMfOY1Lz8zSNjfPG992Olf9eBu";
    os << "NoM4xR6TpqltsP1BmulQnUgrXSTSPK5kUnW1t2K6NUopYKfwgJJVMIyVEQYA2McpYhIGmoyNPImkzO1";
    os << "tiaHiaP/zxCc44/QTOPONEHn3sWebnyixa1I0xkjhWpLJZwsAnmfCoBrKpo7MlkVJIyyGdEgxNzmDbN";
    os << "kUt6cr75ObLXHDm0ew9NMkDT2xBSKc5AmdSDHQk+MPjL5BJ2HS0JLGIaW/L8tzmXRwY9Lj3ka0gJP0L";
    os << "O8mnHZ7fto8wVJx38moy+RSNQhW/GqJTEl1T7DswRcZxSaaSzSzlWKI0GG3Q0hChEUFMOuE0JTZRBEZ";
    os << "guS7CMURGIeLmC1BrQaQFRjjYxkJjoRE0GxBBIwwRlkRLTTnWJCyJZQl0rKgGDRzPbqLiiIhDqFsByY";
    os << "TDjj3jvOrUiFzeQ4cOLdkW5oo1eu12+ux29nGI2JVEKnxZZGfTqIW4TrPlqQcKLQSm3SEshkhfEhcin";
    os << "IxDOF3HSlgvW05tEAJrQRJLJnBW5ZGhIS7HyBYHp8MDG4xnYyoxphY3UyPTIY3DCm9RnhVtDiq22VEK";
    os << "+NF3nmXbiwH5FS1YSmEaASbS2Pk0phaCNtgtSfL5JMWxec5772288PvLuOCkfh7fOst8uYyO53jrG1a";
    os << "zd2eRK966kk9/+3kc1+ayczt5YVuDPzw1w4UnGApxWba2uRx/ZDs5tYwFHRK/oShMx/T3h2L22fFj3/";
    os << "bGC2u/veOPL/6Fgvgnp91/i2f9f10U/yNSwPNwXn/a0Wf/4cmd71u50LLKhZBnBhss6/KoFwvUVJnn7";
    os << "hrmqQMhWghOWpZiaC5gqiooRy4LO3Mcni5xwlHtjB4O+LvXL+e2ewbpSKUQ+VZyGZsLvruZamjRlrMh";
    os << "iJtB8ykHcg52axrZk8LpdDBpkEhSaRt/PkmUbaOmNNQVyZpgw9IVfOmyt5CzNHp+J3sfeoxt2yt8/R3";
    os << "Hk+o2TE+kSB73OpLlwxwoFvnMmy/mK3fczU33voTJWLSsbGVicIKl6QVYAmI0rudgJFQrFWJliIUB1e";
    os << "QlWq5D0kogGxGTU4bRYpKD//4HFq5bzkSljnBDrJwgDpqh578MHmXF2Ru4/+e/YuPnHuCID24g7JYkS";
    os << "5pPfe2DMPkcpCSvOmMhT67xGJnRFGfq3PncYfwYygHkW1uplgOKtRrVqk8YTpJKJlm0sJP2tgXMFSrM";
    os << "zBbJZRLIZAIpaAZPOQodxbiORxjFlEsV9h4YpbWjnY72Fto725manCZXybCgq5P9Bw8Bkvb2duqNOsK";
    os << "yMEajlYXWsnnEEhIhJBaS2UqDbQcn6e5rxxKKj7zzAr7yo7toBBGOMDx743uwW1N86uq7+No3zqNU1h";
    os << "QKZWYnyqw+cREj+0u8+5o7efLxj/HBt32PBzdNA3DK2qUs6WllttzA0y6zpSpuA8q1ECMEac+joUOkZ";
    os << "eNhoUzYLJCWwBKKWAvCWKOFwLI0yjhESCwLVCMm1hLHkkjLxrIVtpDYloWUFrGJMaGhVKvhRxFgYeKm";
    os << "ejHW4EeahOtgHEkYxKQ8Gy0kRilio3Ach+GDQ9jGpaO9g6nxZqhXxa8RuII1C5eyS+3DDyNiVFPfqhS";
    os << "uEtgabCdGqRCnu5ehyYPopIRqhMzFCOmiKxESAZ6NUDT1sikLXY2aGT2eaH4OaRsUCNs0d9spGytpoS";
    os << "0LE7kkHIvuQFEox3xiV5G4oACHzJoUiQREpQhSNm7CaiYTOhYiUhijkUmH1r42CofnePVld7Gi02FgU";
    os << "ZbEeCuf/Od7+OTnL+ZN5/0bb+5YjLAknuWydV/A3sk6sRF4OwrsHAvRfxzj5GVbOeukPrZPxRgsHtpW";
    os << "4UIZM9Aj5AMPPn7a6uWLx3fvG669oiD+eSLgK6NRtBDifwRE/G+K4it3ihKwli/py09XzFV+rK3940o";
    os << "PTiJrjYhEj81jmyZ4dK+PMNDW1sbiBe08sW0f2hgymSyXnH8UX/3Jc3z0QxdSqnsUq6O4bUluvXWE/r";
    os << "6FzM1UGS3UqI5E5JNJ7B4XZ3ELOgAtBN7RWdRUhJ5uIBZ5mEBgRYKjsgto74o5pIpkY8UXX38irjEEK";
    os << "o3efi/X/+JRHh1L8+HL1zORGOK0f3iAHrvKRy5pY2Cgh3/97UHmpqe54NS1vPPMRRz//tdwzS8eIFqR";
    os << "JdwzTnGuTDqrkJZFOSgRhCF+PW5awqRBa0ksJbbROLaLm7bwowaP7HZYn6vzf85Zz4cffJa4HEKsEGk";
    os << "PYdtcftz59Dov8IiUzE2X+IfJOles+yROZieIefS+SR7f3eA7t+1i595ZoiikVLexUjYdaQuJxG+EZL";
    os << "IpsqkELfkcpaDO7GyJA/sOM50ts+bo1XjJJJOjk3gJB9t1yCSTzM8XsD0PR8QssDswSlKpltj0wg7e8";
    os << "NrTefUZx3LzHQ8zNjZBR0ueI49awbbte9Da4LkJUilDqVTEswRSWmA0RtNc5ktDwrGYKVSYHpvFO6Kb";
    os << "dT05Ljl7Hb+573n80HDjLS9w8YlrOKEny9997E727J/BzbZTL8xw9mmriFXEO1b20ZZcQCNu5ih1tLR";
    os << "x7JE9VANDo64ZmppjolQh5QhsKUh4DkpqiAVSakInxgiBhSCMDK7TvO5HpvmLVlpgCY0fRiQ8gevYGA";
    os << "NhDLZsBtFrKVHSoIQgUBplRBOaqxS2bZAYoqjZMQq72ZHqmGYchGvQSEI0JoiwpKQWNjg8W8BEGi9po";
    os << "aVNOKZQjZDWjrZmvIGGRCpJtVLDMoaUa+Ebgx/FJC0bf36O5HaL9eefyGFTY8oo4oqPnfdQDYWVdlDG";
    os << "IEoRxm9gtTuIpI3xFVbeRgeqKWR3LOJqgHAFdo9HXI0xsYVRHtF0He1ArjWByJgmwBiae/G0i5SSaLQ";
    os << "E0uD1ZAgPl5vneW2wlcLNZRkPoa1cRxvDSeuP5qGnNnNtuYy2HMYmHb5y7YV89isPct5py6mEB9h7aJ";
    os << "bn99U444QVjExV2HhgkmeGD3HmsiS2UVRqEfdsFthKUwpr1gnHLDl/tpD55cxstfoXCuJfGqH/R2fLX";
    os << "9VO/gUijuV5njc8XuwZm6t/4S2XXnT9hy8/TX70XRswGJ7d1+ChXXWCUPPZf34tW5+8kpOObAUp6Mhl";
    os << "+PbHzyWbTBJHEZecfzQPPTPOTTe+iUKpwPw8JN0kCV2hIjRea4Zkh0dc0zSG65B0sTIOoqaxsw7u6hy";
    os << "yN01S2vz6ss+z53v3c+en7+ZVvsf17zqGz175bU445+ucfM7nWHb+9Xzq9jKbdg3xuvfexEv7a1SikI";
    os << "2HXd7x9YPc8IcdvOacRTw56HH1Dx7j4nf/lof/5Wfcfs1luDF0bhhgslxjaGyaQrlMtd4g8AOEbdPS1";
    os << "caqI5axYkkflutQ9gOiOMZ1XLraMsyWFbkFLTz05V9wYaYVE8fEDY2JFJ9+zetZ/dRzXPGp3yCdJO2p";
    os << "HKvmNjOQXoGwfCa3P80bv7CbC666m6e2zmKki+t69HW5LO7KYicyJNM5RKwJGlXqjYCDw4epzFdYlM+";
    os << "zetVSlIh5+vkXsCX0L+3FTXjMTM3g+yHdPV3U6w20MrhWgo6OVtxEkjiOeH7bPixbsvaYlSgNW3fuoX";
    os << "/RAk467mhmpmcwxtCSyyAdC01zT2cQYDRRHBNqTS7j4bqSiYkyY/smma1WufLyM7ng3ONIpRN87dfPc";
    os << "MbHbuD/u3sTW3ZOsWzJUhqlMkKkeeCx3dz/8F5+vGk//as+xNO75kl4LuduWIZ2E2Rcj+0HxthzcISM";
    os << "55BKehhLImyBimOEkBghiWJNbFSzMEWqmSqrJfVIoDQobWHkn9wqNpEwaAukZRGLZig9olnYJIIwUFh";
    os << "Wk3wUKo3t2hgkkWgOC1JYICFQEVJ4RJEkEgoshUAiLYdGXbBp+24CHWDbklVLF2N5HpOzFRKWg0xKYh";
    os << "0hECjLYAmJbQQ512VZOktPOsOSXJpyQ3H/nU8zct8uPti6jvXtLXRJCxHFiDhGRCG6FqIxaGGDFhDoZ";
    os << "qZPQ2O3elgO2K5AK9PMJZcCVfAxoUKmPCzXIt2Xwu50EQLsvIO1MIO1IItI2Dir2rAX5YgmKpg4BmMQ";
    os << "lkAJgYsh1haRZaNjH18pqlVBKW7h+z/7DHc+uJNTTx+g5gd0tCf51PtOJeG5uK7Fkt4UW1/8Mm+75GQ";
    os << "avuKRwTpbR3081+ZrH7+QKy8/gZNO3DC4eefE/dVK8MqM9z/Pff/zwvg/SnP+r53in9G1/6MwBkFAEA";
    os << "Rj9Xq95VMfWHvp2y97E+987/UAREqQT+eo+zUGdw4zsmkBNz86yLe/fB6Hd8esXJzh6m88yamnHcVFb";
    os << "/k+//CudSxcuJ6TN3yBNSsX0Lewkxe2NpCOh4tBSYG1OI+IJNLX0O5gInD7EtCdxK7Ajy+5nAtOfT1z";
    os << "8/Ocsr6Pk/pqnHTKtzk4q8h3L6ArnUBrgREx3a0tdLQqnn5+iKQIWdEukV4XP7pnivxTu1m9NMuugwn";
    os << "G63DzMzXKF3+GG2/9Jpd96ce0HruI+o7xpvdXOwhb0tuRIZVrxfVcpNG0tCQRFYPvBygVk0+nyNqSrW";
    os << "OKNW02x42O0plMMyMVpw2s5Yj927ngi7+ls3chJ6/IMjge8f47hph0juOK725kyYnrcJ0Mp6xdjKpHV";
    os << "E2MlciS8fLYiTRaN/NfoiBqWstSKSwBtWqD+ZqPFYW0ZFMkUwkOHRommUjRu6iHZMpj/75hXK+Xvr5e";
    os << "xsYnsSxIpxy6FrQzNT7D8KHDlI5axoZjVrBr2yB+4PPUs9s45sgV5PN5SqUy2XyOfKaV+blZhIiQtg1";
    os << "o4qgZ3mUlLZIJj/1z8/hRjLVxP07C45c/vIJEup2vfOkX/Psdm3jPW87iqisvJLdgKaE/j4oC9jy/jz";
    os << "88upXJYp3f3vEYrmt4zYb/n7E3j9L0Lst1r9/wTt9Yc/VQPaU73enMA0lIgJAEwRAxyCgCggMog8L26";
    os << "FYczgZxC25Flkfcmw0oikSQeStmYAgkJIGQxMxTj+m5q7pr+qZ3/A3nj7eCEeEce61a3at7repVVd/3";
    os << "vM/z3Pdz3eexbnqSsiw5emKRE4vLRIFASYG1AoUkLwzSK6K2xBmBK3JkUKvBzgmqyqHjAJFbUEEtpCg";
    os << "Ba5YbKTVBpDAGrDNrBCNJEDQpiorKlrSa4+SVwUqJRYP1BIHG6IoyE/UiS1m88pQlSOdqoEYApXMYV7";
    os << "E4v8jOHXOcOLVItvcoTirm+ylnTXZQ7ToHnMrUp34uJwgj5qIIBxTWoIRiNk5w0hJYy/v+/O85Jxa84";
    os << "opdxDtn2Z+F3BUOSGOBQ+NOZZgxDZXDVx7ZDjAjC5lBdQJ06aj6JTKUBE2FK0A2JDZYs51EGhuFqMhj";
    os << "BwYZgJgNkRMJ+bdO4E6N8M0I4Sw+khAolKqIheLgUsZmLdFKctnus3jtGz/Nvsc/zS+/bI5rX/a3nHv";
    os << "OFj7w1w9z68dezm+/5QVsuSTm7z7+JPd89k68KQjDgJ2bpnjswEmkNOx/eoH3/+HLuPiWA5t+6T2PP5";
    os << "IVVcWzsqf58WN0DQj4Mel//7+WnB8i4jxThaMwDBvXXnvttSeOHvrLX/mJLe3//pFv84Vb9rJjbh3/5";
    os << "ZdfwBlz4/zrY0e4+PwZXnrddj7yD4+xfWKGV758K7d+dw8mSnjbL17GypFlbrhPDheaAAAgAElEQVR2";
    os << "hp9+48dYXh5wx9dupQP8/U234rsdgtLUUIVGgmoq5FiAjDVyPEZOaHzpeM2mC/l/3vZHHDxymvXrppH";
    os << "C8L++uJ/VPOTMM6aZbggC7xAiRwlHr1eSFyXthkZGDfqDAqUV62cnyK3BS0E30nhhSSbGKFZzlu79Fn";
    os << "/6Vx/ni3ffxmuuv5Dv37mf2clJ1k13cF6SjXKqqiQvS6SAVpwgnGJ5MERITTMJOXpywK7zxjkjtKR7V";
    os << "zi6vsnGRsKT/+c7PHwMrrtiC1E0QVn2WRmm3PJAfc2zZ/nFHHn0y1x50SZOpJJOe4qx8QmiuIkS9bWJ";
    os << "kA6PwHuFUB7nPGEcM7VuHZ1mh1OnlimKnHanSa835NDBo2zespHxiS6HDx+jmSREcURVVZRVRSAkoQ4";
    os << "YDIbML6zw3EvOwjjLseOLZGlGc6zJ9jO2cuzoCcqqQOuI4SgHHForhJd4/Joi7ZBSUlaexdU+Jrdcfd";
    os << "XZxNaT9Ve56vId/NovvJAXv+S53HHnE3zmH29FS8mdd+7h8EKPi87ZhPSWC3ZvZ7wT0RlvUeQVCwsrf";
    os << "PfJo5jK0IxDKuFR1AUvrTx4QaI1hakwZYXWEitAC4EB4iSuC6izyFAikWitUYFcE0Yk1nl0KDGVp0Ij";
    os << "ZcDyaAAI4qjBYDSq93VS195BDZUHreoMaCVAiADrDM47kBJRgROe4XDIWdvm2DC9jmOHFximA4qqqoE";
    os << "UcZN/vedhmnFAIGOKUUYo6wuZJorSWbyw5MZydhJxXiticxRToDlWem7ae4Lb73+apx46zBsmt/DUrM";
    os << "VrkM7WbE8hkUrgtYDc1HvhvLYYoUEnGpoB7hmbTVg3IlJL9FiEyOvrK93USCWxKzl6NkYQIFVAeNY4b";
    os << "lji0hKoHQlpZji9OuIjv/9e3vD6V/Kh//U/ue2Wr/Ket11MlQW8730v5tChJR7ae4Kffcul/OGf3M7T";
    os << "R0a8/bXbObaQcs/DJ3jliy/h2iu3cuhoxm337GXldM4Fm6vg/9w+v3PT5q3LKyvLS977Zyw5zw66+uE";
    os << "Ll2foOfxwDfz/7BR/RAaLAtQZG6eaUzMb33X/g/f/8kffd3Vgxhp8+qt7+ImLL+T663dy+e6Qex9VSA";
    os << "mb16/n+LECk6ZcfPlZxHOT/LcP/w3t9gTjseVV123mTz+6yOJiBng+f+Pf88RT+7HOEWmwxmGWSmRSg";
    os << "REwEyFDhR7T+E5MdKqivO8RHn7qNKDoDwesLFo6Yy1mprrYwtLPDbWzSxBoTbOlqcqKNDfIWsRm2B+Q";
    os << "5zkT7Q5CChbTkkgI0qxkdXyaf3rsFAtvej23/+O7eOcHb+MNr7uUz33pCbZuGufofH39YY2nsJZASlQ";
    os << "okBq083jhqJyj0wy4694++9sBl05kvHX9FVx6+Tm86Y+/wtapMY4v5sxMRGyYmmB+cYXlXsGvvvF3+N";
    os << "iXP8OGL3a4/9ETbDtrF1KotdFNIFWAQ2CwFLnBWccwTRFCEiAY9lO8d0zOTFCUJQsn59k0Nwezggfuf";
    os << "4gzd+5gamaapaVlNs5tJC8Mo2GOlgIdB3S6HVZ7PR58fC87z9zEfQ/sIQoUw0HG3KxmcmqS5eVllHIE";
    os << "WmGMI/IGhK5fNsKDV5RVRSMK8VZyZPE0jSgCpbjvoeNcfv4s9zx6mmPzD/HXn7mJw8dO86d/9SVaYYP";
    os << "cltzw4ucyt2kSWRjarTGOnzzFhomEA0dOs9ob0GzG9QPBOIx0eAORUHjlKMsc78F4CQ6EqK0xQgYYF0";
    os << "Bg8GWFd7oGdEgwtm4cAu1RujZIW1mLI73RgNyWjDW7ZKbEBXUBtGX9RrHC4FOFjzwFjsArAi2xKsChi";
    os << "SgxXiJTh7CSXllgseTC0g6ahLZeQQxGy7jCEcQSIw2xApwjkppTzjIeCEoLoff0BTTRPD4acrrIuX6i";
    os << "y9dOW0aTCWdv6vLnX72d8FsJr3rFRVyyucM3XMLjchXvJYwsIgIvHJQWGwhQEleu5ZC3NS73iKFDSIc";
    os << "zDqHBa0nQCfGJgqFFxh45GRGqWmhxWuJWDaK02MzgcLQ6DZYHA77+/e+T3/4trPd87+ElXvOre3jnz2";
    os << "/gEx+/nXu+e5jVdMif/OXb+I13vYw3v/ETZLTZurFDGEWcv3uCbRtCZrqz3PztfXzsM9/nyvN+gne+f";
    os << "Jv+k8/uv37dRNRZHdrb0rwc8m9G7mc6xh8nuvy7bvE/e9Hyg27xyisvar75hgs+tbJ06G2bx5vBzjNn";
    os << "eOvv387OdZtYGPX53gNPcXx+lScPHsYYx6vfuptv3LWH9ZtmedHLNnP9dR8iCjSve8EMLzx3C1m1gzR";
    os << "1aC2RQvD7f/4hvnLbvxB5kGGInm6gIokQHrRCBgq1LoEkwJ3MuW7DLDf+/V20ZruEiaIRBUxOtAi0Yj";
    os << "jMWFrtMSpKOp0GmzdMMz7RJg4DgkAicWSjAdlwSGEsw0HKseMnOb14mtJZhAhoYun3h0TTM9z7ZMENz";
    os << "/8z/uKG8/kf73851mQcOb5CJ4lIohBnPcoDzrC8tMrCch/CAK00eZbTTBIWV3L2LFZMb5lAHdnHNz//";
    os << "HUa5wEvIs5RDxxdZSQ2dRovp8SY337fI6KFP8Hdf/D32Hk/ZON4lSWKGaUnlPKvDlMEgIxtU9AZDKus";
    os << "Qul6uB1oihKEsCry1RGHI2Ng4e/cdJC9yNm3eyNNPH0UhyIqCpdUVJsc6CAlpnuIsdNodlFTc+d29FI";
    os << "Vlw4YJrBccfrpeITSSCOs8UkpajRglBMZ4jPN1B6ugqBzeCaqipNVsU1aGv73x6yyuFszMjdFqtfnsl";
    os << "+/mvX/294yGGXMzE3SaDdZv6OCk4Es33c0t3/g+RxaO46RHAMPlEftOLBNGAVKC8Q6LJ6t8zcD0ljaC";
    os << "1HqcU4Q6xFUCj0aKoDZdh5IgDLEuoPISGUYYH+BRIDRCh1jpqZwDpamsp7SWMAiRSmMcSB3j0KhA48O";
    os << "AykqCoO4wnZSgJZWt8BKchNIqjBSUkUYqwfJCD+fr3BsrBK6syIoSEUUILyiKOv2wstRNgvMsVzlSwH";
    os << "JpOG3AOce+UY6QkitnOhzoDQnHYl509gZWlzKm10/yvPO3sHxglQ/8xe3c8Wdf49xRC7k8QFuBEwJWS";
    os << "7wHV3qoRJ0r7S1CgYpBRKLOnHYCm1bITh294QE5ExNua4HzBLMxJBqRVQSbG+gNE6ipBqIRELRiZBzy";
    os << "yRs/yV986lP1OK4VNooYlJu4+MxtvPiaOQILz3vhB/nJq8/m7LPn+N4DT3Ldi3Ziqzoid++BJe56bA9";
    os << "Lo5SLd27nv374Pq76mXO49oUb5HBkrnrvr7/wdRPdZuvHFMUfLoz/4dd/pij+u13iL/7im3f+xY33Xv";
    os << "qed7yAj/+3q3jLH9zG8mqFjCqe3LeP654zycc/9xQf//zDPP/sjbRI+Po9Q77xpVew98knWFhKeeMrz";
    os << "+WKF2zj3qcC3vvRb3Nk6QB//ju/Qbc7xs7Ns1y8azui26RyDlfZ2jMVa0RXIybrXJNqMccOC3oPnaA/";
    os << "HbHh9RfyG297A4vLg9oEm2Ys91dpdZqcv3MLW9ZPETZCXOWwmSWQYe3XqwxearrNkGarSaRhMEjJigI";
    os << "VBnil0bbAlRXBxBQrecLr3vEpPvXBf+Tis9dxarlHVuYs9/qUeUqZ5/SGIwZZilCCRhjhjUEIT1laxj";
    os << "sRE42QD375BKbIuCAZYZxGqgAtA5wxFEVFa6xNEmqsjLnmzf/Cdc9RvPv/eglfuPUYTR3QaERESUgcR";
    os << "xSlw/jakOudIwrqiIVRbnEWWq0IHdSRq1OTE5x99pkcPHiQ4TBlw4YZVnurTExOcvLkKebnF6h1ktoE";
    os << "rTU0kpiiHBFEiuecdxZlUREEnoOHDjE9NY51kI5ydBigFEghEGsHBM6JtXO5ugOzztY8vj0LfPnWe/i";
    os << "9//ZZ3vX7n0N4R7uZILUiiDWNZkyaGTqNBmEScvDIIg8/doJWoEmimG88fLAufkrhraup1K4egT2C0l";
    os << "hKZ3BSUQoFUhIISBQEKsDLeuyTur5pllKDFlS+3mEbJaisxHmNI8AIReYcRkrCMCErDUZ4rJM4KSllX";
    os << "ZwCUXeWmRc4BFbUQo13FThHJQXeWbwIcYSsjFKyNCdUkiDQpJWjqizhGthlVDiEDfDKIbxkxVkK6xlk";
    os << "BQ0laQWKvvf0rWFCCcYrwZMYJqYb3P3gIY4vrHLdhZtIQsWTxxZodlqESnD0Kw+y43sHufzIBDuTaQg";
    os << "lvjCQuVqgcTW9yXtX33tTT1QiEKipiGB9hAoEwXSC7AQIBHo8RE420Rub6LkmaqaB2thEz7bQ0zFW1k";
    os << "CLHVu2M5U0WD87xQd//ddJsxP89Rfv5a77Ky7atYldu9dz5OAJZKD4zfe8jM997RSLC33O3jHF//1Xd";
    os << "3LHvYe55pwp9j39NKvDjKyv+bXf/ife/5bL+a1fPoev3HZ4W7s1tf5ZBfFHdYs/lpzznymKzw6mUm99";
    os << "63/ZK9SW/CVXTPH1+w7y+FPLPP+Cs3jq8DF2bZ3hsufs4uJzJ+i0QnafPYlZyilGy/SPH+fzH9nL1u1";
    os << "tXvfyOR5/ssWNX/46i4t72TA7w5aZLUy0Wux9ep57DxwnmOjgM4tvhggnCLYlRGe0a9uscohEILTke4";
    os << "OM7qWbMSPLu37zDQgh6OcWKT3jzQ5jSYOyrBimOSuLg5pyEjmEqOiPCpCCiU6HRqNNqBXNRov10xN0G";
    os << "zGr/QGr/RGldShfIZwhdYIg6fAHf/c4Dz4xj9CCxeU+6WjAatpnZTBgMDAIr4mDEOOhtB6LphQgLFRF";
    os << "ivWOm+9fYUMnoJUELA+KOpbUOZZ6fbSEIIzYMNng8OmCV1/7Xn7j7Rex8Tnb+M6D8yjpWF1OUUIRRQH";
    os << "DLKUqHQ6oqhylBEIKKmsp85IirwDIipzKWC688ALm509x6tQpAi0o8owtc+spKku3XXejKgjQQUir3c";
    os << "Y7z513PkKr06TbTTBWMX9ymSBKmB0fZzTq47EIoXHe1V+L8VSFq8UC4RFCU6Q1yisrcj7z5e+x9/gi3";
    os << "/nXPTy+7zjtdoIVkGUWU5o63xhLGCpQkpMrKaNej8f3HqHfH5KECiUlxkicr0lsxpraQ6oF3gta3ten";
    os << "cQBC0HCOrjZMhNS0HBQ+8ESNEHwtMhsJTkhQAi/Depxc+zmiE8Kojkc1BAip8dQEHafqt1PlBB6F8Gp";
    os << "taSPxSqOErNV5oXDe4kRFFEfESYiwloXTSzhrSeKIhfn6z2Go8cpTOOhlFZGHppKsVI5hZam85Uiacb";
    os << "xIyaqKfzq5zLoNbfK0YqmfMz3W5PDiKoePLZENKiYmYi6/YCNbN47hGhu5/etfI/vfN7HuRE37NsMUE";
    os << "0KVearTFp8BpUF6jwglcipEakW1VHuPfeAQ3uDaEhkrsBViTCPXNQgmQ4JNIXpjC9FoILyie8Y0jy+d";
    os << "ovKgvOSyCy5iYnyGJw88xBe+/m2ePhLyxldtZayhuevr+3Gjkwz6Q04cGfHSq3eQJJprrjiDq55/Lq2";
    os << "GZP/xo2zdPMXevRUPH5jnoivOYM8huef4wskhNRzi2ffQzxTGZwsu//mi+ENy9Q9QYWEYNtZPZ8nYlv";
    os << "V85MaHOWvb2Vx/9SVcfv4WJqbbTOyYZmZjh7K0fOyLD3Pv/Qu89Y3nYm2Dm797iIt3RRw7MMHUzKWcu";
    os << "3sbACcPHeBn3v3rHF5cQHbbMNki8KCbmnjnJGo8hk6CiGUt90cOORYipWA05glmEgpTcOf+o2yaaNPv";
    os << "DdGRYG5inDCULA97pFmGN1WNihKCIwvLGAszM1OMt0OyoiDUmjgJKWxJr5dSVRXt8SbNdoPeaEiWZjQ";
    os << "Cw56FAYEKAENZlgxzR2U8RSkojMI5h61quKlWDkdNsQ60RicR3ismJ7o8sL/Hl+5d5trLZsFZRqMSoT";
    os << "XWW0bDnJmpDkVZsmndJHc8UfFHv/kJ3vuzDU6tCI4vFFSV4/TKgMIYAg3GOBACIUO89TibI6RH6pAyN";
    os << "6RpgVKaUT9leWWFiy85n5WVPstLPaw1DEcpWkmsq9BBgLWOJAmI4ggdhhw5ehIh4PzzziJNB0ilWe31";
    os << "0ZFESYGSkiip93s4KJ0njiO0VnjniANJhcN5W68wpKIRSwy1COGsxRtbBy/hKIuqLk7UKK8g0nzn0UP";
    os << "MLy8jZYAUGulBURe+QNZuE2FN7WXVAYnXhBoyacmROKfqLi/NmfYej8AGETpQGBmgdBNJhNYRUgdUHo";
    os << "wQdRF0CuUDhnlJKQxIhVNQGQusdZ3IGo4RxUit8QJcUGN1nJdgPV5CVdaXH+tbY/VIrzVVWaAjRdKMO";
    os << "Xr0OOBRkcI4Q6AihhhGZYH2nqG39I1htbIMS4uwgicHOava1nar1ZTdZ0yxfeMEh08OWBkUTE00MaVD";
    os << "SoUVkg3diDe98mIema944JP30P/CXuaCGZr7DbsODTkzqRDLhvywweYWoUFKiRmW2KUC2VK1HRHQYwF";
    os << "OeeR4gE4UUjrkVEC0rUmwtYne3kTPNAhbTYLnrcdfNM2x3gov/IU38fijDyCAM7fMcd65VxOxmV1ntP";
    os << "nLD9/GtNP8zYefz+H5Hu//yB21V3MiYfbS81GNcc7aNM5rXnY52+Y28dEv/ytXXbqDi3cUu40pJ/g3a";
    os << "s6zyTnPBkX8SHvOj1Wf11TnZ4rhM58o2rnlkle/+ZWdF+3ZNxBfuTll66aNzG1I+YN3Po9H957ik196";
    os << "gFtuO0ivl+LxnFjK+OP/fgOPPTDiwX2PIatzePev/yo3fesWbr79Ds55zhiTUw36C6A7LRpjCcqBakc";
    os << "QKETpEJEC51Hjda6yTHS9Zwzqr8lbAyF8+1/v5/zdm9j7+FGaKsJKTxJrYq2xVmB8RVrkzC8OUUjWT0";
    os << "+AlBR5RWkqdE084NjxRYRUXHHJOYy1xxBKk6U5WVbisCSBQglJoARRGJLoEOcFWeHAW5JIMrd1jjO2b";
    os << "SSSGrwjijR5OsJYQaPZIIkjerkBBVunGjx2qI/UitJaTGEJtabRCJk/PSDUCt1IuOO+ea7o5txw1TZu";
    os << "fqJgvAUrgwFlXqClJohDQikQQiGVA+HxQhAFUQ2jFVBmtbqZlzlVUTEzO8uRw0cJggAd6Bpo4B1ZmuG";
    os << "dRyuJihSj0YgiL5hoTxE1Ap4+fJxQhThbEUUBo6xCCEWSBDWlx1RIKWi22iilKIqSpBVR5OVaVGuACk";
    os << "EJgfYCGdRdrV/zAQrrqLzDe4nzFuVqBdh4g8BTVb7uokS9+7K2Bu0iQa35JGMFPeMJhaQVClKhyaWiI";
    os << "QSpM0QCZiX0whAlIxA17MGv5XBbVxeyCkFRebyCMFDk1mG1X/MPSpSvr16kBryqT/7wOFf7/aQUCO8x";
    os << "TiKFxbr6Bt7lOZecezZOS04uLbF5ZgatA9LKcOjQAUaDnEbUqB+wzjG5bpp4tsXCyUWoHEppvPAUxtD";
    os << "UmmFhSMZiXOVZTVN2bRgHJPMrI4z3bJxuAtDr57QbmtWlIV/85pN0xztcftEWRtmIzUcW2GwafPtfHm";
    os << "DD3pRXa8vMVknST6gQpNpC5ZBCrAGOwfUrREkNIxmPUGWFCAReKnSiCNqeYFyj18X4VYtOdK38e4XOB";
    os << "Wfu7LJ1NuapQ/MEccxbXvcKbv3eER567FHedP1FPP/6G/jpN/8lo6zEWPjmXQe443tH+MmL1/F7b38u";
    os << "J04t8+gexd6nJNecbWhMCPXIPvvoaDQcUqvOz1agn/n9GTL3MwLLDyARP7Io/jiYLBCL1sQn3vHq2fY";
    os << "ffegku8+7BGt7fOnW28kGjm/ec4JHHzuItjA5EZOEAYePL3LJpmnOOavLB/733UTBGHnh+LuvfI5rrl";
    os << "yP2DDOo/ePoBETeItbSQkmEvRkE59oVBQg2iEiAtkKUFFYB8eX9fUpDlxmcVJi0oqj6ZBwtoU9XbB8u";
    os << "s9yf1hz8JxjeTTk2PwKo2HG9k3TIB3DQUZlHXEUY5xhcWlAZQw7t20CIekNB2A8zbhFVhQUWYZS9S2p";
    os << "1gGNOKlJM0KgJISBYP30FHMbZ0jCiMpYlIJEBuRlTtwco9lIGGUpoRYsrQwYpR6hNCpQ9HoDkjBmrNs";
    os << "gzXNGqaGyFe0oYFAIDiys8LwLZ7j3UE4rkHRbMaHU5GWFp0Kh0IFGhzXjUCBRUiGVQOCxzpHmGZNTUx";
    os << "RpRpblNFptTp48SbMVo6VCq5CiGKGkoqw8Qgi0kPQHQwrhAFhZ7iGQOCFotRo4W5GlGToICANFVZRII";
    os << "I4jSqdwrqTVjClTQ4UD61FrxCB8Dco11IXGG1gTi/He4b3FehDCk8QKUwmKyhJFIPAUxqJEPQ53lKqL";
    os << "o3NYB3EYkuOIlGY6DCmVZKQVOlDMpwWB94wlCT2hCaXAC4WIJAaJ8WtXKNZQ4hAqwEpwCJyMkFJCGCD";
    os << "RCC2xQmANeFFfulQOvPcoqKk5QtZfD4oqL1Am5/lXX8EwTTm1vIqpakr68uqQQ0fnKa2rL3OsoSgy4q";
    os << "kJVtspen2CX9VYUwDgpCaQAmcqVKQpvcc5R6I1xxaHtYjpPL3UMLeuSyPQeG+567FjeO941WVn0N02y";
    os << "5kRPPLt/dy9/xgdFfPYSsrC0R6XHuhz52PHeb5pMpVr3JRi4B0urX2SdlDifS3wicJgS4dMNFLUP8ug";
    os << "qdGhRK0WBJHEjQzV4RF4B42Ahb5jbrbNuVs63H3fo2S9iDvuf5LCLPNnH/lVPvfRm7jxpgc5Y9MklbH";
    os << "40nHg4FFOna4orefP/+YupsfnmJzexupwhW0beji9M3xq38ED3vtnF8Vnx57+B3vOM1EF/6Eo/ggbjq";
    os << "ZuO+PnXPCcF73sReFrd22YVZ++6UHe+7aEd7x2M5PbppicmWJDVNEKBfvme/TTgueeNcdiJrnq+bsYi";
    os << "yx/eeP9zC/Ps+/gk7zi5btYTEd85+ZFnPEkkwESsMOyXg6NJ/jcoGcTCCXKeWQrqlH+1uMGDlvVHDyl";
    os << "JXjqRbFWqEAhxjSiE2IlDEYZp/sD8kQQbGhhVjOWT/eQyiOkAgF5VVGVOVVhaHc7aK1YWuxRlCl27UZ";
    os << "Yy3ova5wlSiKQCqFDhkWBs451Ux02zs4gAk1RZAx6faSgxlIhiKMIFYUEccio38cZwyitGGYV6hk7i6";
    os << "glirSqKCpHf7WHE44oDkgCwaGFEaGu6DYS9hwZsn6yjQrDGjoAFGVV21OwNQexAuscjWaCXfu3UZqxu";
    os << "LDIunWz5FVO5SqoPKNhTpLEmMoipKKscoRQGOORslbz4yRg4/Q0S4srGGtrkjUOrWvhKk0L4jjG21pN";
    os << "91rivCOQgkCJ+lXo7NqGWyCkxDiPqwx1WqXEUl9ESKHq12ud9EoUaDCOIrcgQQdrqworaTUTrLUoIWg";
    os << "ohfOOEmhEIaFWDAqLFoL1kSYOQ0TcoCMkVkrmtCYPNFWg65AFXavHKIUVte8QVRu080rUooyoL2BqYU";
    os << "dSOYel3kMKVe/whfJIEdQikPAYb9c6X0c+GHDWmVvZNLeRpw4cJgwUaVGCUERxxJ69B5HCI7VEIClHI";
    os << "7K0Pt8TQqFm2rAyxKcFWIsKNWllUFrjlMdUkDRqYG5vlNNONJ0kYmFphJSKNCuZXxxxxa4Z0kRz6nSP";
    os << "Y7cfYsPYGDumGiw7wVljDc4ZH+NvT2ccXx6w59ApDj+1xGUjz6/ZjMfWT9G3GcFYhLAeMoNVot6tZhZ";
    os << "na56nlAphQQtPMC5QqYFhDeBAOqrFnMMnU6YaES+5eI4v3H0PJ06eoCgdP3n5NLc/1WNpBSYaJU8f7+";
    os << "OF5IW7Z7j6sk1sOnsTL7x6ht/9+UnGxw/xsc8e4O0/ewGf/8b9U/jGnsFwNPqhDvHHFsVnJuT/UBTXx";
    os << "mb4N4UmAKIrr7xy9qFHHvzMr7x8V/PoAtz90NPcddd++scPM7Nuil962zW88rUX89qfv57v3vwIx1dG";
    os << "/OlvX8Yv/dxuLrlsGzM7Z5DBGPfc9STb1iU8PZ9xz32rNKYaNDsBFBbZiqAb461HNQOElEip0FMJKha";
    os << "IyQDVDWsMEw4R1qqXCAWyoxHegQXV0chuhJQC2YnQ61pEsy1UO0JFiub6DuMbpji+9wS9QUaWZ1Smwl";
    os << "SedrNBq5HUxG1b1V4uLylNiQwVrqzPOqUQVKUhyyuaiWZ8rE0UxHg8eVkSKFVfWHiQ1mFxeBFQ5gWjL";
    os << "CPNcnD1Hi6OFFVl6KUZWmsqa8gzs6bo1m9q6wxhENDvlyz3Sq69bBenUs/i0iqLy8sgZE3AUQJrbR0f";
    os << "4BRSeqyvP18jTijLokaApSmrgyFj3RajYU4QRQx6A8IkIk4CysowGuRILQlDxWhUd5XCOjZumKQ3yGv";
    os << "6cxhQ5AWdbod2u0Wel1TGEASKUZ6RZwVRGBDHDRz1fXCRVyAV3gmkAoTHeuqoVufWOi2BDhXOeqx9Bv";
    os << "AqKMo6HyXQtee0KCwqUEyOdxBVxUpe0glDDB7vNQ0JqpbE8c7hLGjlCZwgCgMiJcjxDIMEj0AqSYFYo";
    os << "3JDgV8DRoRYURu9lVRoJWvjs5NYPEorlHc/6BK9F0it60wYfA3p9Q4dhpRlhSssF1+4i35/wEpvwLrp";
    os << "SazzNNsJi6urnDx8jDgOcM7higIHdM7bjPGmfkI0BKrdJJhdTzIxTrm4QmkMrShkMExJ84p2kiACKHJ";
    os << "LZRWdpqbMC44v9VlaHdJtJExPtimkJ9+3RK8wTDUjlivPTByyPYoYGMPQen5yusv28SYrVcH398/zxP";
    os << "4VJpdh994FnjPX4QmlsMOyblqEwOcOl9ejMjqgGJT4JEJohR8aVFthhwbZCGhtDrFLFQdPFyy7kigrG";
    os << "aae977/dbzkmnU8/7JNXHmO4MIds3zltoO88MKd/MvdH+KnXrGdM8/osveeJ/n6LffwD7ccYn5pxLt+";
    os << "7gUMs6G468He7rIY3ftDXeIPxxU8m7NYF75nF8UfQdd+pkuMXv/6d7wn4KHnbpo7U7z61WfxP//u+wz";
    os << "6nkYzZs9jB/nCZ+5ieXWVcy/azLXXXMh7/+hNfOLzj/J7H7iFb3/lu9z01bu55IyIt75kCkzJLd9bwj";
    os << "fbtNshvjJ4Z6CwCB2gJ0gOUL8AACAASURBVBoI75FBHQ+gEo2QAtGN8Urh0wrpQYfgtELECllZaIRrG";
    os << "SKgWgm+qHlzql3/vdaKMNG0GjG5MbS3TjI2M87wVJ9sVNBstWlFdYqbA2Qg6oQ3WyGAPK/I85wgql9c";
    os << "lvqEa2psDKU11joaDU1eFDWfLwzx0mGdwhiD1IKqqFhdWiEvK6I4ACUZ5RZvLUpISle/6a0pEdITBQF";
    os << "xI0aqiNWVIR7H6667gCVXcfhIj9mZSaKwiTOGvMyJghitA/AB1lVY5+vrjKoiLUrCOEYKi5KwstwnKw";
    os << "o6nRoDZq1lOBox1u2A8wzSDC0lSnu0gryosNaybv0EeWEZDkZ1dyfrtLkan2YAmJhoMz4+zurqAFOWI";
    os << "CVxEuG9Jc0qkjDEYzHO1PupNZ3WuboICVk/eIzxtQdcCMrK4Fgb5xU18QbJupmxOl0vzXG2hjdMBCEr";
    os << "JmMsahAhkE3HKABfSqS1ZFSkpi5qvSCgVCHSK0ppEUqhg5DKOUpEzVVUCic8SRCQxAG2jgdDqFpRRov";
    os << "6mkPW1yJID87j8JTO4Z1DSY0UkqyqaCQxOzetZ2mQMTlWw1xL69Dec8/d9+K9odlqUWYlxjmi3XMYma";
    os << "MijWprZKQhVrjIYBMPMw1iF7HYG7D1svOww4zVlQHOWLSARlORNGI6zZg40JxaHTHWULTGEpzxLB1ZZ";
    os << "jpOKLxHCkE3EJyuLPvTjKvGWuTWcf8gZWfS4MyxNgcqYHmJ4chy+/dP8rZTOcmVZ3Ns2EdUHm8cflQb";
    os << "1H0o6oklhyqtu1hvJLIF2lWUC5aoEVJKyerRFX72spjfevNWTH+FP/zAN/nmF7/Dhz79FKlfx213/C4";
    os << "/ed1u4tjxofd9ivf/7hc5fvw4x08rHjyQs2v3LG//lSs5e+4M/uHL3w22bj1rx+nFhQd/zAj97MS/H+";
    os << "wW/11R/BEh98Hc3GTr/e+54c8e+fZfv/65F2xTk9Mt7r3/NOsbnsl4yOFFiw4TEJ6vfu1pPvzhbyHTP";
    os << "fQPHOXRBw+S9XqMRpZmojj/qvN5wzveiFk8xOe/M4/rThL0R4huA9lq4IcFohuBBx2FyE6A7VUoCXp7";
    os << "B1vU0Z+qWUdMersGmq0E0nt85VGRxDuw/aoeb0LJunabX37p1bx2+zR/+FMX89Y3vZjuqWXOvOhyHnl";
    os << "6P41144xNTjP/9LGaPCwFxhisrUcAi6PMS/JshJASpSWFcwgVMpk01rqlHOcNkQ4pS4dQiiQO607COh";
    os << "Qax5rvq/J44ZGq9seVeUYUhcxOT9BtRjRjTVY4nJFUJqMqDePjXUZr/8cn/8fP8NyLf4J79yzwxEMP0";
    os << "WiOE0UxaZYyGOY4W8cGOO/qpDwDxhmqKiXPi1oF15JGErG82kd6SxQHOC/o9wcIIQjCaK3oeZRQKKlI";
    os << "sxzvBWOdFqM0J03rUazTSmi1GqSjnDwv6sCsIKTZjkmaCf1+SpalCOcJwoiqrBCyvte2psat1V1j/bo";
    os << "0Zm2aERKztpdzWPAW7xSzM+NUxpBnBTMzLbqdLl4LTF4QGE8uPEmg8XhOlRU7WjFbooh9ztJTAh2EhE";
    os << "LRTAQ21KzIoL54Ceo9sVOK0jmsqh+6RkCgFZ2wgZKa1BlKQAUapTTGijrHBbX2LvN41r7/vrZGCS1Qo";
    os << "n5wGmO54qwzKZzFupI4iDjdG5BozfLyKkumJO6Okw0GqE4DvaMDsu6qZDdEao2MAoJuDUch0vhQ4MYC";
    os << "wo0dUlEQznaJfEU2rI3fgzQHV4tPCEF/VCKlZryTcPJYj9VRhg7qrtoDZeUovWV9FHO6cNyx0uPnJsc";
    os << "IheC+UUpXS8bCGCsDOmHEPx1Z4fB39/FfWzM81pQoBFI5XCgQ2kPpqBYL8A4508A6T9W39Slh3+C1JC";
    os << "sFYb/Pe3/nCl78i7/C9+55gocfOMTekyU718WMJxWDUwe59cabefM7v8Sjjx5ndiZilIcMMsMlZ0Rcf";
    os << "t5Wji0ZTvVXuXCXQlVLrVe/7hV89/uPH7HWlc/qEH/s2d8Pzvx+zEmfPn/3rulP/s1DV+8/MtI3f+9h";
    os << "jHiEUMH1l7TZumOck8unSZot4jjgF6/cgagG7H1qiF1+kr/43d2k/gKWZMyO7WcQdhMOP347t93yAIR";
    os << "NlBCEG9u4Roh0ArVtCh8IFArZCXBFSfHEKYLtW/F4JA5kgFQaOqI+lF+q8IHDtiMYlIhQop2AoaHTjX";
    os << "n7C57HpUGHL376dkxxkDv/ZYav3LmHX3rjc1g8eR/tA6d57bvfyvRYxT9/colvfHeRmbEuYaRJs3qBL";
    os << "JUgzQu01kyOjxMEErxmlOV4a8jzjNI6us2EvKgY5SVxZEhTgyAk0gE5hryo6jCqbkK2lGHX9j7tTptO";
    os << "K0QKQSAbxJFjbuNGjPPc/+CTRIGgqgoqU5EkkscPjVjsP827f+kX+GxS8I3vPMC2LbsYDFOaSUyaj1h";
    os << "cXWHTxg0UpqQoLc5URGGE9bbuuKwhLws63YSlhR7diQ5KSYSoTwObraTOe6kq4rgWjIwBicc5Q6A1QR";
    os << "BhnaXf6zE+MUG722KQjqjSEt3pkuUGV8GG9eso8ox0NGLYH9FoRVSm7vKUcFS2qu9wTS2mSCXx1MUDw";
    os << "FlXP3gKQxA7wkhiVgxJGDLZncRbQxyH+G6L3uA0rShiZAzrkxaj/oh7exmX2jbPb7a5V0mWnEF6mJCS";
    os << "duIRucNLiZMCIyVW1uZ1iUIqhbcS7yWVh8J5kAmx8lgJRVEiVIjD4kQdYiVETeqWKkRYj3AVSqo1Go+";
    os << "k1W4x2YrZd/IU462Ek0s9KmdoxxEH9QJik8KOLKrVgoYCLZFaobshRBIpJDIJEUqu2WQ8blXiQgNa4A";
    os << "tX74V3TdNdV+/3isURq6dOk5cFUsAoqxhrJ0SRwlqD85Je4RhWOU7Apjgh8LUd6Mm04Kcmu5TCc9vyC";
    os << "ldPjNEOQ46XlhGOdRLU7Dj7s5Kbb32EYGqCHWdt4qXrAr61NeChUYUZVai49puKzCIijerGVKczjNTo";
    os << "UKBDgZpKuO0Ld7P7Obt55+9cx1vefQ2P3refuTBHFKt89K8fZKU3xu+8dZzVVPPEEycIwghDj7PPnmL";
    os << "PUwf52BcfobSeSAqE8vKCU49droS8CxhS52g+8/HMffSzR2kvnjn5+yE82A/EFaD5mhuuefW9Dzz+gf";
    os << "VJT2+YarH7zAa9VDA2Nsb2bS0a4y2mJjRbJ5pMb59Ct2OGPcPBvfM0OiGTSYtmI+b2m+/g+19/im/tg";
    os << "3vFHK0qJ+kqrBV1aE6sMVlRv1kigbAeO8wJz58i2NRCxLp+0zR0/QTGIWONKw3e1vECZilHhooLxsf4";
    os << "0AWz/N5v3cQ/338UnCSZnGVlMaMdF6RZVo+0MgYKzt7cYP22rXzjjqfQUtKI6g2CEJ6iqNA6YuOmGQK";
    os << "p0RKUrHeKS/0BeZETCM1Yu0MYhwzSFOcMURRgKkscxlhnqVxNak6zlF4/p9NtIvAIEdJtx1RFAc5TVI";
    os << "a4ETI9PsaefUcQElqthKMnTvGqq+Z40ctew3ceO85F2zfyile8mde++ReIVIlxMYeOHOOMLZs4Mb9AY";
    os << "Qw7t2+ruwTr6zVAoNFaUVU5xaggTQuysiBPM8Ymxjl96jQgmV03xepqj6qyjHUaeDzLKykIz/nnbmZ1";
    os << "JWNxaYD3hjwv2bxpjihWLMwvoYRk247NlGVJvz8i0IqqsoxGBUWZI4XBe6iMxGPJ07y2rch6jyeUwLo";
    os << "6nN5bUz+/taLISnbtmKM/GDFMczZNdMjzEdvPOY/5pZLlEwfprazQihOc8Ex22pzspZRFTtlI2B7FzD";
    os << "U7HNGOfRhkbtnW1ZyyISUxSgd46WqhQGq0WBNaZMhEHBBJD0JhgVFpSascrEGpAFfVawBPTbURrs71d";
    os << "qYiRmJ8RaAEo2HBC3ZtoZdn+MqRBJrF0bBWwN2AwycWEJFAd+piiJZIWRujVSdGNTVqIoTKI1AgHD63";
    os << "iEjhnMcOK7zx+LTElXUDZLIKkVvaw4j+qQWiZkzLNThx4gSNsO4cbQrjYU0IMt7SChVIRSwVofCcEUd";
    os << "8pz/k0nab7Y0YLwRPW8NsqHliecSDyyvcsGU9Nm5w2+HjZGmKxHNDFPDSt76aDzaPUsUKN3J4DSqOsQ";
    os << "sDqj0DrARpDfnJIb1BxfXdHhds8Lz056/k3PPOp2pYjh5dxFvB3NYpWmMal6ac2LvMocWUfm/E0qmcA";
    os << "/uHuGqVUMNTe1PmewWnet5v3Lr99rvufeRO51gBVoGVtY8+MABSIHumQKr3ve99z+4SnxFXNBCNxXHr";
    os << "mkumXu3z5T9okjU2TAectX2KTqIZ77apiozpmSahyFg+ukq/t8z86ZTV08vsuf8Ii0sj0mPHOf70Ee6";
    os << "56wm++s+HqfqCu90UVWbpTih8EIEAM0gxCyPUdBPVDEAKxHSjXrDnoLZ1wdZB8kE3gKzEWYsMVK3oVQ";
    os << "4xtLjCsC4J+OPxgJ/4uRu5Z1/Klm07WbdhGmc8s21Jkig6Yy06SZ1122q3OHq65OnDi0yNtTC2pCwqh";
    os << "JNUxmCsZfOmdYx3uozSlCKvkGsKYxgmBNLX42GgaURhjaL3IL0EFZJVJcLXooopKvppTtiI6bSamLLC";
    os << "GUsjCgmCmlqtI80ozchGOVIrnLDYSpDmOW9440W86qdfQX/JcnLhKIPekJ/72TfxkY99kqnJCeIk5tD";
    os << "ho3S7HVZWV6lKQ7fbZJRmGOuwpsJUBm88UiuyPK8hq7amJSulKcuKKAwpiqL+HkuNc4I8y3HeMjXRYZ";
    os << "AVlGWOQBKGuu6onCPNMprjLbqtJnhPGAVrXald40sG9XjsHV5YlApRUlFVJeBrcUxS8/gA7y1KSYqqo";
    os << "hFoZqcm6A1yhLNcfvl2Hn86YPvmDi973ix3PNkn7fcJtSe04HAwMUYYKJrWMu89q8YyKQKmZMTGac2K";
    os << "C1mqAhAGZL3OcEgCHSB0SOodKoR1jQSDIpO1Vcdg1+7eFOh67+l8vUdUQiFEzZfU8v8l7L3jLT/v+s7";
    os << "3U37t1Nvv9BlpRs2SRs2WkAtuNNtgBBiwnRiDcagJCQTYrFnACQHCmmwoJqwJdkhEYgMORe6ybMlNkt";
    os << "XLjKTp/d659dxTf+0p+8dzJRzW2f29XuefmTNzZ055ft/vpyqsAqk8roJYxByYb7M2LGmkEcN8QjNKa";
    os << "XZSji9dgNSjMoFMNNFCA5UooixGTafIbhKCQJRCNqNAQGoVytC0QGbRdvdzSCdCS1QnRjV0ENM3HWJH";
    os << "E9PWlLFj5949dNrzXF5ao6pKUh3jFBTbhJSQkpaWZEpyMTdUzrO/mXC2KPlcf8jNzQZFZbl/a8DL5+a";
    os << "4qplysSy4carNwfl52tNdHhhP+PTXnuKOO27gcrMO76vwEIvtIQcYVXgHyjjKoWPVa3bWhq89epFJuU";
    os << "a+vMLlcytsbpVsXVxhfbnPxQs9zp48y2BtjPQlwmjy8YiFhRbNWJMkOgxNvhBTyWBH3JjeWNkYb/LNe";
    os << "6H/pzVavf/97/9fYYmNO2659hePHTnzq71+P9WxptFIGfULCu+QdY2xNSsXe6wuDdkcFowGlpXlTZ55";
    os << "Ypml81tM1nOeONrn+JkJ939xg50zEffqRdZHEVOqAh3ubgiPrwDp0a0osMpRiHByxsHODKnAj0xwxac";
    os << "ivIg6wvRMaE/TEq88USPirtpx9+99juODFrt2zNJuSOpJTqIdIFjrbeGtI4o01tcopYi1JE2jIOvwjl";
    os << "a7BUJQVZb9+3YwNzcdbFg4lARnBakKB4LQiskkxzuB2hbuShW6XPAEwqN2TIqCwtR4qWi3UrQJUU1GC";
    os << "KqipBlHqDhCS8VMu02Uxmz1++Gt8YpJUXL9lTu4YvceZhZnefzoee6559Pc/vIbuO7wy/jox/6a3Tt3";
    os << "kmQxS8srzM/OsdUPFkS5jTGasqKqKuq6RiCxxmCMoXaWqirJspTaWHQksdYETFEprHEUZU6zGdPtdJk";
    os << "MqwATxBFKCxqNJpWxjIYjpqamaKQJk0lObRx1FRJmlCLcPJSitgZjg15RRhGIQFqw7ZFWSmKtBScRUU";
    os << "QsBVkc02hmzM606HRbPPaU4oP/XHPz3OM89JVzRM2IcaExeY3OAtG2Z8c8k06XstcnFgIXRaxvNynuj";
    os << "hJKIrZEyP3TIuCHQqekaUq9XTx/S7dJqjVL1iIJaoSgowzVBKHeQKNVmOy8AIcn1gGf9s4SKc14UvKa";
    os << "a/awVVREQlI7G5ws+ZiTmxcwDYdKNaqZoJoxKo2RjRjdiUNcXhRCJCQSHScILQOBMxUjnMJbj4xCwC5";
    os << "aotoxlCAThYiCLVYohdhOGx9TMmBCMtuk6VO2RuOX2meEEFQext6SSEWOI40UWgrOTGqakSYTkqdGE+";
    os << "aTiBunWwE6UJIo0VhnuTis2HvdbnYdaJFVNT1fIxsBRkBJ9EyCm1j8Wh66YZQkSSSbQ8GonXK1r3jk2";
    os << "SHHL4yxBjYu9Dl3YZNjJ5ZZvbDJcGw5c6LP6nrBpYs9otgx3Co4cz5nbSunMI7RWLC8WkT9/uS6m6/f";
    os << "U1xaGy45578ZC/0SE62+2YEIxP/2V376h//07k/++lTTialuMzBy3tHPDbJwbOY1Ve0ZTSyrG0FisTV";
    os << "wbG15VjYd/Qn0+paNPqz1LHNzDe4tu6yuGGZaoKZbmLLGlwaZJsStOHgnldpmIz0UNT6vkQrMVoUwHh";
    os << "Wr8GGIRFgJa4tKg+VOesXMiYof72xxz9Gc85crmonHlDmToqKqTEiDNhVVWYYkY6XxFsqqpKw83huUd";
    os << "8SNjLqqkdKzY2EOvKDfH6AROO9xJjDvSnrKylI7T1HXNJOESEfUzlLkY1xVUkxKShfsa8aIgH8hGIwn";
    os << "CKlJGw2qsmCS14htB4z3jvWNQTgYvEDFEaO84qYb7uC2mw9x9IVjRLrB5mDMkacf57u/49U8/vQZnnv";
    os << "hGPv37cJZz9r6JkmSMBpPiJRGeIGxhiiWGFtTGwNIyrLCGYOxhEnOglYq2O58eI1qWwS5jJPoKKKuay";
    os << "Z5wcxUh9o44ijG1IayLFlcmKMyhrwoyeIEqSVVXSOlojY11jkiFSMJGkUloZE1yY2kMAbpHJGQCOkxL";
    os << "lj8sjSiNxixsGOG66/ZwwvnNbdcu8Zbv3XID//aJg+fKuk2LTOxYr1v6TQjYikxozEToXDtFrELSdzt";
    os << "LGUsHWcKw6yExShhoiNG1uPjmESH2tKhD86Nq6c7KK3JbRBiS6WCmFwLkB4nFcKJQOyJ8EVKdIQSOgj";
    os << "AraMYTrj9il0kWpOXNdY7rJS0YsXp0XlM4tHdFJkEZlk1E4QQyFggkzgcVJnaPiAjvFZI5xGxwprwb5";
    os << "FaEs00kDLIgpQMtkKch0jCthdbJgrV2j40vYBYInc2mW11GOeO8XCEc2778AwazNI5unFEJBTrtWV/I";
    os << "2WrLEBCqmMiB0fGBZ0kZTrLWJ6UXLCGdqLIkKye71Me2eJw1GIlHyPTGNevMBfHiBmFjzVuWCKqmlR6";
    os << "lgvNRpxybSYYlBH9LcfWqGZtyzPJBeOJY6tvmRjP5dUch2djo2ZzYOiPK7b6Nc4KChc6uFcGuVxe56q";
    os << "3vOHm4XMnLl76BxPi/1SB+o3G6BfX5nh6erp5/OTKzyxdXtu7o62iRiMRxniqWpDFCYUR4W6JxtsIrV";
    os << "JGuWcy8cQyBq9oxgkTl7F/JmNDaj5xyuMqSzfz2DKsjGqqCUmE8CGnzRU1IpGoRhzi+jcL7MYEMYD0O";
    os << "3ejUoWvLbKrUYlCmu2URBV6da+O5zj14ft55U3TfOQzYxIF3WYgRJpphlaK0tYMBhOaaczcdIfNYUVd";
    os << "VCAFcZSA9xgbQlHrqiSONGmWYIwNU62HLGvgvScvchyKLImoTEVvOMLZAHzXxjAeVWzlOdYatNZUxoO";
    os << "UJHFCXVUoFYXDX0KSZRhnqfIJ1hiK2uK8J200KKuaWCq0dDx7/Czvfs8/48knjrC2cpErDh7kuWNnmO";
    os << "p0ueHmW3ngS1+i1xswNTUFCIbDPt4L6tqSpEESYwzUlcPasMI64xBSkOc1SofJMMT1G8AjJFSVARd6Z";
    os << "7SWIUwXaHdaTEY5URJhTIVHsXvnPONRjtKaJNYUZYETHrBYKyiKCoRD6QglJMY5+mNBHGuaqaQoaox1";
    os << "gfiRYIwlzWJq41icyZiZ3sHxFy7xf/92m9/+ywlnzkbsuX6WF4Z9FmcUZc8zLGt2t1K0hazdxbYSbJ7";
    os << "jDMSxYjFOGArFqoVICha1wumMiQKRaCwKJeC2uTZnR0HYPd/MGDrPxHmSSBIRIUXIDZUyOJrCVKGIhK";
    os << "CwhklZYSclr967wNxUl41xiNIf1hWRc5wdnqVKQaZ6e0qMieYaiEgglSDe0ULEKmxRO5pEO1th++iXq";
    os << "PkUFWsYGcRMSPT2pcfXjmguaGZtXaMWGug4DpDJbBRW6lijpmJkM+Reeu0xu1MW5hdoJhml0JSTHGcM";
    os << "0ju8VxhkgDiUI5KK3Ams99QiCN2H1iEkaBFxsrREjZhqUnLp8oAKRbPZ5PSZdcbPrbLHNJhupoxbnno";
    os << "9QGEMauywRsWSqYZi5WzBkTXBrTfP0ESTyQwpoYEGkbA5MNSVQIiIqtRYpxnnntIInNOMy8C8x8qRV4";
    os << "KVrdo2dKWW1kfP8f9moV88FN03lrq8mKqdWlssDkflc1r4v5tqyMNSuYXaKbIooxOn6CghSzJSleBkx";
    os << "MQqTl8ccXGjZmwVL5zv89zZTZ5fKzkn4PETOakWtFoCEyWB2JEyWOO0wm2OML0xKkuRrQQvw8HIVg4W";
    os << "kjfvQYng85QtjZwK/cxeSCgctlfxfdfeRv3V86weP8ULK57TF4cszmjywtLpTLE43yGKJYWpcKVhfn6";
    os << "W2ekOZVEwzHMWZmbpNlPiKGJSGZypwTpkktJptqiMBRU+7EKFrmB8YFCzrEldVWxshuDO2sIgzylLS6";
    os << "RjGolkPKqwzjE/1aYykKWew1fspJE2WB/loYNYC8rCkpcltXVkjYzKuaCIsxWtRsbaRo9xUfKaO17Bv";
    os << "V95kL27FhgVnnvuuZd/9Lbv4rP3fZksSen3+zSaKWZ7GrQuJGJXlSXPC9I0TOT5pESIIBuq6xIldZgS";
    os << "vcM6gxTBIon31LUlTRSzc20Gk5JYRUFsXpU0W022+n1mZ2cQSIbDIa1G46XIMDyhyEgFiMIZi3ceryR";
    os << "bPUsnqfjQj0uaFp69qPFKEkVBJWEdeDxJorhwaY3jpw0//lbPdc1N/s2f9Lj3y1/joJvm06c/T+twSn";
    os << "tVs7lpqJVnpt3mlHGsrK8jtaAVJYyNoBXFNLOEdanQSlBHmgUdsVdq1uKYiTV0lix/9a4dHNzhuefBT";
    os << "RbnutRJwoZzlN4jrAMliGTwOyMFwscY6xmamqI2xKbkzQf3MDM/x9nhAOMsG8MBiVac3TxDrR2yGaEa";
    os << "ETJTRNMZuh0jhER3YqIdKSrV6EYSpj0r8MV2UngaLJJCShDByimtQ7SC8cEbh9AK3YkCBplopFaIWCG";
    os << "bCdI5dDOENiPDej3Kx5QtmN2/yMGrrmb50jK1hbqsmIojuo0Gw9rgvKf2jhxJQwi2AIRif6NFJWEjFs";
    os << "x3umQ6YmouoxHHZHHEwUN7iLImZ44ts3J0nbnCcsVUm2GvpC62JTpWYEYVjczhjODIxTHniopnlnpcP";
    os << "N3j4sgyMRGnL0/Y6Ne00pROIwMZ4X2M8MHDX1Zg64reqGR6+sDl3qh/7/LG8BnnfPVNsMQXHy+tyy85";
    os << "V4DUOWRdlzhoqHTq2jNL/UOXNkvW+iWnVkacWR1wYmnAkYtbPH92nRfObjApDWvDirOXe7z9PW/hvT/";
    os << "/w5w7eoSnHl8l6WS05hLKSqLcdpKLkNitMd46ZCMJWq4Y2G4I87XDbE1CHluSUD28gqdGCIlbKWFKIp";
    os << "TGrJZ85+IM5//6AT7+8UcwNmZlrFnoRvS2hsStBgf27KSqDZUpGWyNUToOfSoI+nlOXtTMdJtIqXBSB";
    os << "KbYGGId4b2l3e7gvaUui+BkKUuUUiRxQiQEeVlzeX19uxogYVxUKCVoZpokiqjqEIB66IqdHNi1iNKC";
    os << "LGtQWI+Wgk6cUljLJK9ASJJmcH+4OkhNkBrnDKaytNKErz/+JO96x/exttHn6aPHuebqK7hwcYndu7r";
    os << "8k3f/GB+++2NMT03R29qk2WiitMRYj0ASRQrvLUIKrA3+YIGlNiHrUEYRUgpwFms8Uka4gLpjjKXZio";
    os << "iTJv3emFYjxUPQPkYRdW3odFqMhmOkFmRZhLdQVTXeBZ+yqWviON5ejT2b6wW33qj52P/W4HNfGfOJ5";
    os << "yxXLCZ8y9WKtUHF5laJs2Hlds5xxy3Xc/sbvpM75hwfu/coT55yTKcpf/7UJ8lsD7UnYm7i6Y8yxpHg";
    os << "nBeUeQHWUesEpzWxd2wBO5KYOtIUQhJJzQhDN4vpRDHj2rJjuuK/nHyG3VHF9+9b5O6/+yyd2vKmg1f";
    os << "SzJqMXM0Yy9BAYS3jGnLrKZ1FeseBOOb1+3ahWi2e3ewzHvXxzjCe5KyOL8C0Cp99B3o6Rc81UZECA6";
    os << "rTQC1kIbh1KkV1Ukx/gigtIlKoZozrm+Dc6qaYS0FDq6dTbOHwwyo4wuYaiNqGg3WhiRtUwa2T6vB3Z";
    os << "duJ40bgx2WAorSiYMJ6vkW6e4obX3s9l89ssNkf0kwSGo2McV0zLksMgnaaIqSgmWRUStJ3jtmZNjtm";
    os << "2nSnGuxZnKbVSOhOd8hSzc65Bm+482Vs9QtOn94gLgw37JimM9/FCUe5NqGaVHgEWVNQDyv6qxO+602";
    os << "7+Imf/DG2VJvPff5h8qJmq6g5eWmDZ8/3OXphi1MrA04tDTizOmRpc8K5jYLeyPlOI764PhxesNbX3v";
    os << "sXD8BvJuD26vDhw+krb2q1mnEn2bc4P3vr4SvfmOnm66/evfCjCzONH13pbV0z3WlEh69dpDIVs92U6";
    os << "Zk20kEURdxw9SLf9z1X8Ya3/CDvedNBjj11nFfcdhXf+z3fxqH5jE/d+xRMZwgVIYVHpcG/6kqDTjVo";
    os << "kEqgug1saXDrY0RTB89wpAn5Aw4SgV+pEIsRlOELrnckzGwZxp96ns/cbrqqqwAAIABJREFUfwatY6x";
    os << "UzHRaVHWB8bB3YR7rYJTnlMZQl5YoVqRJxNrmFv3RhHaaIITHWbAGKlOipKCVNZFeUZQFSRwhCRY654";
    os << "KrwnuIEwUKNjb71N4TRYo41rSzCOEhrw1xmnHNod2kKmFzOA64jo6wMqIoJigJrWaD2gdfssLR6TRBS";
    os << "CKdhsN5u00uihS9wQglDa+87TYee+4YV12xyLVX7ucjH/0k3/uWN/K1rz7GyuYmzUZGVdmAUxLW5DgJ";
    os << "lZRlZTAmaDCdDZ0u3nqyNEX44B6RSqKlw3qPdwprS6Y6bawR5JOcViulLEqSNAkNbyp4kcu8otNuoqQ";
    os << "M7hzrEQTXTqRDA5IxlpXVmuv2OP7o51t84gHD736moi8q+r0B6daEk+s1cRazY2GWQ/v38J4ffhM33f";
    os << "wyTp9a5uSZr3PnwZgvHS/58iNfRbcvs+8P5jm/ZLjmEcExUhqNjFRI6rqm1WriEFRSIKMYvGfo4VCSc";
    os << "dEpSiVoRBE9KXEompFm964Op58/w8NPrvHAmQvoToSPCk48fpREReztdjmYpCRRKLufizTNSHJNq8Er";
    os << "pzssTHXolY6vnT9HNRow32pRVjnr48t47VGdBNWKUYlCT2WoLAqRGFmEXszQWYTLHTIJ/S9KSLyxwcl";
    os << "iwQ/CzQ0VJnGx7TP3A4PUEh9JdFfhx26bKRcIZwIMVVpEQ6OmG/jSI0qL7MTIRoSvDKqbIiKwhWFNjc";
    os << "haDRayRY6fO0ed58y2OxAHITkCtNR0soyJsxRSs38+aH0rB8XE0S9ga1Qw2BqihMZ5uGKuw6AGLyM6S";
    os << "Yrpj0jzmkN75xlXDiMFToITCllXfPh97+bQy3dx5IkXWLuwxB//wY9xxbWv4c4bOlxcCpUgSaTYtbdN";
    os << "J0oQGm6+fi9lXYhxnk+//LorD+7fuWN3XcvG9VctNMqqHispTauditluU3bbmZjutKT4iZ/4ibnbdp/";
    os << "8bD6xe09d8GLHjOq0pzuRdFZOz6ZceaBBo9tEiC4Le6fpTDeoColu3oIpIrJsia18iv/04U/wyz//U1";
    os << "w6d4zDt/8gRVnRajTwOsIpgWo3gpo9jUIqinf4og69GVG4Y8pMYTcn+NoS7WkhtMIOChCC7LW78Fs1f";
    os << "lQTfesCqvZ4aYmfG3HiI09hjKfZiJmZ7jAej/DOcnD/HsrKMhyPiRPJoD9BCMXMdJuqqlhe7dFoZDQi";
    os << "hYgk3gkGgzFWGlppg1hqdBrR7w1IdEzaTIO1z4ZEU+8ccaIZ5ttvttK0201ircknNeO6JE1Sdu+cQQn";
    os << "BZJAzMRVp2mSq06bdblBZ6A37KOfR3rPc6zOcjGgnSYj4qh3WhCxG6y1lXbG+2kNrxWc++iH+7C/uIc";
    os << "0ibjl8DR/923vZs2OOd7/jh/iOu36E6alprBMgQAmL8wKtY6IIyrLEmtCh4hE4W+McJEkS0si9JY7Dq";
    os << "lvVhro2JIlmfm6GjfUtPJ5Ws4kxlrn5OQaDAc45kjhGa0Wrkf19ArY1KHyY8p3BOsP58yWHD8X81i9M";
    os << "8ZH/dI6/eUEydessL2/MMC+nudAz3P6qaziwo8nKyjp3f/wB9i7OsbSyBkIzNeX5/quG3P3VMeeug7t";
    os << "+bYavXGuofsvyqk8oPjuKuHbPPL3BiOX+hKl2m5waIyQ+zeigMdbSTWNm0gZPImlnEbHXRJEgiSQ+a+";
    os << "GKCStHHiGaEsG3WzuiKMGUkv1X3QFVTRwJskgH04AIE+2orFkaT/DWsL+TsbmxzIQRo2oI0bb2cCpGt";
    os << "+PtaVGiuzF6PsEbi+6muEihCotIIuykDmtyQ+JLix8ahBCIRIb9TmrcZo3zlnhXEzuxeFcHMmXskLHE";
    os << "1haMRTZkcDpZ8MbjNnO88KhUYZ3H5xYzrALWP66oRhahPeJ8zq72DuzqiLWTpxgbg1aaXXNzTJynnSU";
    os << "0kwSbRLTbTbrtFLxkeWNCEkm6bU2eG1rtkHCv8Jy6vEEsNNdcMc9mb4j3lmF/g4eePMt3v+X1PPTkM+";
    os << "S1ZZwbomHOsB6TaM3Rpz9F2ljkt/7tH/KBD7wT4XsU5T6kNOj8IUglW5sF/fU+xWSN/qbj7MkNJuMaE";
    os << "cV+MprYIycvlTdfv9Ps3XdNsbxenvCivuQtW/q5556z4xX7m8fO5IcO7d/1nvsee2J+OAkf4kZTsn9x";
    os << "ikh5Lm95di40mZpN6a/mzO9YoB5ZXMPyQ9/7eqbjCXXxOE98/Su89dtu4MD8Lv7D3Z9h0h8yfWA3Qgl";
    os << "kkuJyg4gA6xBSITP9UruY6qSILMEs91A7W/iJC9jA+T7FFy8S72lR9wzysU2Yz5DTEX3rUXGEMSPGk4";
    os << "pGlm6DvRpjDQ5LFElGownWOprNlAuXVhiMJ0RRRFdkoejcePqjEVVVMtNu08yaFGWB9I7uVJPN/ghZq";
    os << "eBmURLpBaWtWFoZ4mxNLAVaSYQA7zzGhiSYdiPGOUdde0pvUGo7xmvbbhZFkkFvwHA8CCXuGhpzcwx6";
    os << "W/TWe7S6TVzItMYaj69D8MNg2OeTD3yF22+9iU/fdz+z8y3ufPn1fPq+h3jmmaPccMNVHD16mtnZWcq";
    os << "ywCsZLGnbGKOxHmtdWJVFCCZSSgFQ11XwTyNxNmga8Z5ut01dG8qqpNFqUDtHnKYUecFWb0Cn295mlj";
    os << "VFXQc4QYQmN7PN+ivhWFm37JiO+Y2fzfiTv1rlntMZczdk1P2cXTOL3Pkt13F8pce542e5/74lLi6vs";
    os << "rq+waiqeft3vYZDB3bw2YcucM+zX+Q1uzLWzxR84asjJh9SvN4LjpVD9sxeS5ZJap8FTWskSAgNe7Xx";
    os << "DGJYiGL64dXgFWmLR2zQ0MUiYkol5OsDTAzZDa9m/Nh9tK6awcUW0VHIseXyxtNMzcxRkxGpRdaGE/K";
    os << "qJE0V3WabPVHB6XPnONErMZFFdSJkEiGVRLVTdDcCLUP+ZSwRiYYkTJ2+AmEtop3grQ3aPvuil1ghYw";
    os << "eRRKQR3hv8uA5OQy/wVcCCbeXxI4PQChKJ2ma8belRicD3a+rVnGhXSjSbUa+MkZlCTjVhshmCLqYax";
    os << "K5ESEe9y3N+/TLp/hRruhyScwzynOXlZebm5pAyRsYJDs+4qOm2msRRzFTLUbkaHcfMpFmAzkxN6QzW";
    os << "RWSdDCEF0zMdtvoTRrSZnZ/nvi88zOxMi2v3dxgPV2lM38jhhXkePfU8n/qbu7nr7a/h+kMN7v/Ml/j";
    os << "D3/sz9hy6hrwsGa9v0F7M6G8ZLq9ssmsqY1gYLq1t0R/VRHjRaDb1oatv2/jkV9e/IPyDq/1Bvm5q25";
    os << "dKDgXQ3cYSs263O+u9WXjlTbd8e5X3/8mp8xfa51a3ANjZjsgizcbEIjDMNBLiVsK5y0Ou35vyxtfey";
    os << "f/4zNfZKuDvPvqLvOxli/jsZt7zs/+ST3z6GTo7p0Px1PYBJHCoJMZrgdcalSr0TIpZzzFrA9JD07jS";
    os << "hra90uJPD2m85wpszyC3avTBTojevzDm0kce5Zff9w6ee/RZPnnvEWanu3TbTbpTXTyGrc0Rq5s9TF1";
    os << "T15Zsdh9XHGqzcuQYpZGgNKU12NqxMNei3WljjQ9f5LImihXtNGEwmjAsAgeHMxS1QW8HF3gpwvieBJ";
    os << "C7qiuUEOzctQuAyTinrEuiKCZLmiglSdOYNEm5uLyCNxVaa/IiR6gI4xzj0ZgsSUhiFaQdzlEYUFHE2";
    os << "tIy3/baW/np976LX/mNP2bnjlluPnwljz1xgsWFeQ4e3Mtvf+CP6HamsBYQliTWWCfx3mNshdkWU3sv";
    os << "tokYHX5v+9eVCjhiPinQsWZxYYrNzQFFXtNuN7He026nDPsTlI5IkgitQattXNJD5bY5Sw9SwXDkyEe";
    os << "KZz6XcM+n1/mFfzfk0DtfBoXjwieOI5VkcWEOfMRwNGbPrlluvfUaDuxbxNWKXm+Tv/7cV9nsjXDW8u";
    os << "YrBL6pePC44eXfBUtPTVgZX8fb7rqdp587iUbz5edP0O000DJilJfYSGG1QkQpO5VmSygONBs0dczXB";
    os << "Aip0FXJdVOKtc0lVpYL3OomV7ziOsrGBiYJShZfbjP39kXW1wd7nwDhJc7U23a/oL8UiULGGpVFiEgG";
    os << "/C6TyEaE0sFxK6cShJdQ13gh0Ast/KTCFgapFG5cIeMI0VJYY5G1QMbgrcN7j69cYKuTCD8q8YlAxYo";
    os << "gJjKoVGMnNiBoscD2DLodo7oau14gtKAeW6S0eGNwVuBziy0q3Nhg1wtoi/Ad7htcYWi7BqMXlmnHGV";
    os << "dcuQ8ZJ1gpaDUS2o2McV6x0Z+wMNukmQa7JtQMxhWr6312755m99w043zCibM9rDVkiSSLakaDgrSpu";
    os << "Ly0ygf+r19nRh1j0pjmB7//A0SR4Cfuei33fvFBnj7T55p9s2xujOiXJcJHtDKPrSXL4wKAhW7Gyw/f";
    os << "WD19/Mxjy2sbR7WOelVVbgJ9/t7dMlJAg+0knLIso7Ks5cnz55eXNwdfumrvlUvv/UfvuCauJ8nWeMy";
    os << "FrQkqUuzZu4emyhnnhttv3MW4kHzigaNcfWWLVEb83p/cx+996Av80Qf/DKMFY5mirMN7EFmEdAIZh8";
    os << "J0ZEhZFmWNLWrq1X5whEiBbsQQCezlMQKFvmEOeiXVUo6cyojmYnxtGT65zNrBJlfefgunHjrOZDRmc";
    os << "2vA2nqPYW8LJ8B2WriWYHpvh2ha8fv//l9gNpd47PlLSC1IlEZIjdYxWawp8pzaGMqqZjgc0xuMGeUl";
    os << "kyKnLEuMcygpkFIhdUy70w6dJNbgHWRJSneqQzUJ5MykLFFeksSCeDspJpSZh3CLIs/J4lApWjpPXRm";
    os << "yNKasK6raoQQoGVJXVJogfMR4WPLt334j5y+tMhnVGB/Sg4yz7JyZ5uuPPxWqTrUOh5KMgnzJGKwL+k";
    os << "cQRJFGSEWkNJOiIIljkjjGmApjDV5IptoNnIPBYEyWJQi/nYFAmE6kinAutPtJGVKNpBQkUUxtPVJIv";
    os << "PWsr8PP/9NprtyR845/2WPXK/Zijq6zeW6TOE3552/7Hr7nh17HwUO7ef1rXsGNNxwgTTs89PBjfOxv";
    os << "P8/jT73A6+68nle/5jZcEvPA0xcZ9Ca8bL/kxBHLuryB33jfO9laWSeLY9b6Qy5tDkmTiCiKMCKEweI";
    os << "FlVREjSYzWnIJaEYx+7KMSx7EZEy5eYpKTWhOS9RURLe1SOdAm0kxRk9niFaM2K6AMGVFuTKgOLtJtT";
    os << "am2hhRrY0wgzIUaymBzF7EEMOEKFsRuqOJ5pKwJTmQWYRIBMKFMBPvHdQWqSWYgB3KNHQzCxsqHJz1g";
    os << "ajMFL4U21tZoA28ARlrZGkRsQwkmxUoLZHd7UT2zRxvHSQKO7GoSCDaQTcsvUVogYiCRU8oHYqqUgla";
    os << "4RUUssa1NbMLC4wmI4pJTrs7TV3b4PJKM4QXFHWJiuJtDaxgkluQnk4jQ+mYpY0J3liyRsSuxTbaKyb";
    os << "GUE0KvLR87cFj3POFc3zkw19k93zCFbOaj376WRqtBlfvn2JttcfiVEpnx042hkN6o5qFmS53Xv8y/9";
    os << "o7XlH0C/vEI88cfWwwnqw450pr7TeKuF+S52yX874k3n6pzKUoCvPwkWeeO7e28kdv/dY7f/oLTzwzt";
    os << "TDX5r/99l3c/JY38vSxih+46xc4d6nPdYeu5LlzPR59todUkt9533v5zre/kg/957/lw3/8GZiZg0RB";
    os << "ZXArQ0gleqqFG5WYzSGq1UB0ElyvIt7VRXVTzNIAI8pAB+WS6LU7kJGiWq+RsUY0NCiNaml2/5OXMxx";
    os << "XfP6xr7P3jddz4dFjfNtr38CRF54nzyfkqgbt0T5DJJo/f98reeJvjvKxT75AtrNNEkWwUmOVYTAaYe";
    os << "qSreGY22+7jm95+fXs3jXL7sUZPBbvHN7VPPzY8zx//AKPPnkS4RRZnJCP8iCDiEQIQEWS16EvRMmQ/";
    os << "6cFDPOCdruNExE1MDU1hTc1RTlBk9HttEmjKMTz+1Bx6ZGMJzlRI6bVzMi3Ci4uLyFkiztuvYlPfPbL";
    os << "CNsiiiIm44Kz55dZXFzg0uVVdByFNGrhqEuDB4SIKKsB7e40SmUIb6jLCVpK0jghrypMafEIWo2IKNa";
    os << "srfaRQqC1oDaWVEfkeYXSCluOmJ6ZRqoYYx2uMqRpFGRLHrwwbIwsr7tF84/vKrjtjUuYZodJDS5JSO";
    os << "ZT/ErBs6eXOTXY4tmjx+htGNY2L2/HkUmUThHe0Gg02Lm4h3df8zL+6U+9i3//u3/OiX7FG9/5WmaTP";
    os << "s89+wLOC2oczThByBiLRqqYWBuwnkrCVBpjk4ixsXQ9nKgth2PHYV9zzA8xukaisMKRzsSMFtaY2AiV";
    os << "Ri/lO04uDBgfWeWavbv59je8jttu3EtdDLCuRiAY54bjp9c4duoyjzx6koGwZPunaR7eQXqgBcaH2t7";
    os << "pCKMEflKjd3ZRzZj86BrKK0QzCnKcYQhgjvYk2IHFFC6QMwrwFjUdI5oWt1rivEDNZMi8xPZLaCZID6";
    os << "IKbheHw69PwpreSRCxQ0SSaFeTan0IBcgsxvoaoQW6GaAu26tx3iCdwscgIokd19CIWBr3oWVBwOXeK";
    os << "RZ1FycEjYWM3bunWN8csbmtWnA2ZAk10ybdTovKOIoy/P9irXBCgYpZmJ4iLzRHnrjI/l1tfumXfoST";
    os << "jx3ldz70MZ6zPtSKLM5x+twKPRvx8Y/+JtdcvY/H7vkoP/ar93Lxco/DN3bzuz/xua+VdVlss8ziGx7";
    os << "/UKstFaHt6hslOTGQNJtpa3q6O90fDOWRU/1rXnlrd/rH33I1f/v5c3zva2sO3XInb3/nr/Ff/+PHqY";
    os << "sG1x/eh6LB1EzNH/zp+zm/fJJvf+2dzLViPvPFo8TNKJBkzuMri8fhnQ9TRRIFJmwmQ6YJojTIqQbUF";
    os << "nNkk/hbF0iumsKeHGDXJkS7s0DKeEJBU+lxpcXljtwXqMUGF7YuM3IFpgE+lQgd1tUPvuMwB/e9k1/+";
    os << "4JdYWz1Gc7aDt+BHFbW1xJHmPe96Kx/6/V/ibW++jeuvTNg5NaIpL9OUq7SjHjPNmtsO7+NNr7+Jt7/";
    os << "5IMdPnOLcch7CYD1Y70kbMTp+kVTytJOUoixxCKIow5sah0VpTSNr0Gyk5Cas9845pFIkSUyr2SKSGm";
    os << "E9g0kRwmRbKVJLRoMxV+xbRKM5efYSs9MzbA0neO/JsoyNXp/1tXWytBkKh2obiuAFFGWO0JJuZzoEF";
    os << "1hPaSs67TZ1XVIVJQKF0jDVaTApasaTkiSOt5NsImwd+p6tMXS6LaIodLBESiKVxDpLUZnQLCgczkVs";
    os << "jkac/to6O+OEPbs1w15OhWM0hKIUHD91guPHjtPr9Xnl3oK8khS+RaszTbvVorSWc+fOc81V+zm/ssm";
    os << "hvbt51R2HmOsmzDahNgWDcY7Win5Rc8VUk5PPHqdvoNlohq0xiUijGKEUeV3RR9OMU7JIckFCx0mmmz";
    os << "DsbSATiZ7J0GmYjkSqkI2I8vwWW/ef5S3X3sSHPvCLvPttt7Ovc4lTLzzEo08+x6NPHOP0qdMUgyW+9";
    os << "Sb4vjffzs+89wd406uuJN26xJOn+yT7ZqCZImOJshbdzbBWhE1pVOPrQJDICHxugyYxjXAe3LhG5CEV";
    os << "R0YKdIxwEGkgVvjxNoPcTqjPD1HzMTqNsAOPaGwH3w5qZBRCnJ0N3SuineAnFjEJ0IBsaIQOLLBqBWZ";
    os << "ayGCLlA5QAtlKUKlCSEKvkg6ZAUNZsK5GrGxskooG+3bvYmFuNuD73qHwRIkmSmLy3MC27jOLEyaTKh";
    os << "zGOuaLj7zAq2+5mje87hqkGnL49pdz5oVn2RpJXn3nYSialAP42wce4Oabpth8+o/5pd/4Ou/87mt54";
    os << "fwmTz53blRUxYVmI1WmNo6QkFMTAiDKf/Co9DeIFl8cHw1gm3E2v9hNvqPMh48IXPPW6w6yMqx44tgm";
    os << "xfQdfPDdP8WDG6/iXP8Sp9fO87//yF2856du4V/9i7/jB7/7R7n/oUsopZk+sJPufBuUxEyKIBxNEqg";
    os << "NMo5QTYEtQ3qM1Bo/qQLwj8ELT/rKnajdTcqvLmNODRBthT7UxFmHHJSo6QiRJFDVyPkY52PEoMAAUZ";
    os << "IGy5wMK8Qvv+IAb/2h7+dXP3iCc8fuZWZ3BzGTwtaEUVVz8OAu/vK//BaxnLBy9os8+ODDPPjUJkvrJ";
    os << "WXp8HiUEsx0Yg7ta/CK66eYbkfcdn2HBx4/x+4dMxSmpqgsWkUI66nrmjSJKX3481Ec0Ywj1jcmZO0G";
    os << "WMNouEkSJcxOTTMpC2xtqOoQSZYkDZIsRceQlkWIhLKOZivFy5RHH3mON772FcSdJkVtsLZmfWuT3Ts";
    os << "XaLSz7fU2ML+I8C4bV+GNYXFxP1J58smA0gS1QF0ZijI4cKz1NFoZxoc4sTgK3lnvQ4CEcBpnS1pZhh";
    os << "Ax43FJu5GgtKKsa/K8CoZ/L7BGEGvLZl/zP1YTDu4QzC6PubopsJlivluTNSK+ttzgXa9JSdH8+RdKx";
    os << "kCrGSOlw9iatNFiMuzhfEWcpTz25DEaDcXy6hqRTpme66CjIIIfFCVX7dpNkkRATdzS6Dp4zWvvmXhD";
    os << "ZSSR8kwSzQyOGaG4mCZ0fcr1B1ocG7wQMGMdVm7vID+zifnyJe7+w/dxyw27uXz6S/z5X32Fz3x1lbN";
    os << "LOVuFI5oOGkNX97n7U5e4+Zqz/Nw7r2B6eoZrD0SM7lmh/cq9JJ0IKkVde7Sy6EjgLo8xE4ecj5CtGN";
    os << "sroPbIVqhbtf0cGWt8M0I4jzMheYahp44cbFthTa+AyqNnEygdxteIFrhJGabCThSKp8pyu9fVUi+HA";
    os << "jUxk+CsQytwNaGmdeiQSYxPBMI5fGyRVsPIYUqPmGkgC4Mbb+OflcFJT1/mPLh6hGeXT/DmW17Fdddd";
    os << "zcrKCpPBEK+hqA1lXeMFJHGEjmXoMU8bbPT77Jye4ysPH+HUhT6T0YB2+xPkecTf/s17eeD+s/zmb34";
    os << "ShOPn3v6T/OJPprzhnd/Fs+cf5daVHInm2it2u2PnTskfeOu33vLf/+Lek8Yz4H+2+L34qIH6mx2KNV";
    os << "BtjQdLOxr9m2+4cmprXO888cyxyfyrrhXiVS+f4vDN/wdvvev1/PpPJDz+9Dyrqys89fCzvObQqylNy";
    os << "SNHK372bW9hz4GUX//IF7BS02hnCC1Df2xpQn+DsBgEdlITq1CCZI1FRBIxKCBV1GZC8eE19N6U+MZp";
    os << "3MhQn8iJrouQCzFWyjD4NmIkNtzFKgdIRCQCEC5gf6fJT77rdfyPT/b4/Q/8W6ZnE0SqscbhpKR2hj/";
    os << "6P38GUa9y4dT9/PVnnubeh9ZYWisY55ZvvJQUPPVCn+dPj3jlTdOUtcMYw2gUhOB5WXLs5LmXni+lpN";
    os << "tu0spSRsOco1v94BIB2s2MdqvB/Ows03OLpDHk9QQtFUZ48nzExmbB5uYWRREA462tTWbnZ7Bln/u+/";
    os << "BD3ffkhGo2M2dlp9u7ZyWRcUZYVq8ur4fn9TYSQJEkcIrvqCucdy5fPAMH8HycJeIutHXhFWRqyTJHF";
    os << "MYPhBG9BaxnCMAissrXBz73Z75FVJTsWF1BKMS5KTF0T6TA99DZ7hKqMv79OXIQTfPPrX5+EZqoZlwR";
    os << "Q7BuutNGkrEru//IR3vqWN3Jpo8fXHznGk08+w2g0eel5OorYvW8Xe1pNtoqaWqQsrxfYjWWq3hqBfQ";
    os << "LZbNHYfQDd7nLRwS4t6TrD0sYGS+fPYJbOh9coUjSvmiM7MIX5ykX+4iP/hgO7G5x64m4+/FfP8fjzf";
    os << "c4t5STXzjF/zVwo5FJBO2j6JU+d3OTf/eeT/PQP7mfHbIorDJd+7+H/xSsQrs7tu+l+25XoLEMmHicd";
    os << "frMKm5WWiCRgtXZoKI6vMXj6Mp0795Hu7qCbMXWvREUSn0TkpwZMnl2i/8gl3KT+//y5AM2bdzD1qn2";
    os << "MHl+i//DF/9/nf+PVuHqWxlWzxPs6uNxAr0DEnpEx/MXzX2ROdHjdNa9genae2uZMJjljWVM5TzPSRF";
    os << "GMkBqFQlnJ3M4uFy7BjVfv5uJZydGz55mbgvMnLvP5ex7Herjuqr184Hev5C8/u8U/v/k3uHZPiyyLE";
    os << "Fb7g1de3T9wYHrX1z//wEwzEb1+4b/xQHxxYnzp8eKh+A8brypr7VBJ7a/Yf/DGLz3df9zV67eP9ixE";
    os << "d3/qOO/8x9/NO37gDp565i85c+rned0df8FnP/8E733LAX7yZ97Gj7z3LoQ9xrnzkuZHHmRts0emgq3";
    os << "PbRf7iO3ED5SCFExpiFoxKovx4zJUAqzmCAe+6RFNBUnIlBO5wa5X4B16JsZ7QTSVYEdV6KBtJyEVRg";
    os << "QXRaIkP3fz1Tz15E5+9hd/lUQLFmfnODfsM3vTIpuPLHPrjfuZbhRsXT7O8qVzbA1rzlya4BF0W41w1";
    os << "4wV7WaDSVFRVjUPPzsg1pIbDrW5/mCLRqo4fP3V/PDb3syhq67D1BP6qy9w9viD3PPF85y6WHLgiqv4";
    os << "xZ97N1PTM4x657jvc5/gV37vMbrdDsPRKJQ+CYFSikajRVlWXLi4zN/999/hyr0d1s4/xMrSST794IS";
    os << "feu+v0Om0Wb/wCE889jX+26cu0dscoOOYT372c7z1O2/l137515n0L3HyyH385WfPceJixT/98R/ida";
    os << "97Hd57epef5eTRL/GBPzvF6SWLcxbnKnQiaTZb5EVFURSoWJCkwQNujQnThBLUk5K/+JOf40f/2Qcpq";
    os << "zDJOh8ExV5FFJOC73jdTfzm+97JqHcW7z2Nzi6S5ixCSEw1Jh9eppxsoHRK3JjBO8vRo8eofJPXvOZV";
    os << "AIy3LrB+4et8/NNPc+FyzsW1Fb7y8KM88dhTvOcfvZlf+2ffQTvapK7HJOkUtW9w31ee5QN//DfkueP";
    os << "Ggwe5clfFBS35j//1XzO3uJ+q2OLRB+/nPe+/FzPVpWx1Oackc/010mce5jd++ft503f+K0a985x94S";
    os << "v86d9c4kvPrfAf3v+THNjdYPnkfXz8syd4/Pk+F2tJ8449xIvNQHzgkYlEZppISuS1c5x/4BRPPN/nr";
    os << "v+HrvOMkuyqzvbQ+WpaAAAgAElEQVRzzo2VO/f0TPfknKRRzgkQCAkkQAKJYEBggkm2ccAGPsA22WAM";
    os << "tgkGgRESFggjEBKSZpQnJ01OPalzrK6uePO9349TGiQZ11q1umetrtU9Vffsu8O7n/dVi9j+649iplr";
    os << "wnBnc2iRJEmNYeTIt80nne4gCl/sf+B++9S/baP3gxVgLchDFaB0GcQhxIwRLkogQf7ICD5/hk3ddxj";
    os << "e37Cd113lqwGJpRCREMy7lH+3irlsv5E8f+iht7d14jSKNyiihXyNp3qClpmPaLUShy2ObtvDlR/pZm";
    os << "2vn3u3fpzpzhjjySeXmYGc6EFInChyc2iRefRqEwEq1EYUO9z/wEN/dNknr/BakASJvo+V0Yi8gKHmU";
    os << "4gb/c/wpVhkL6O3oJG1bFCKBIQ2IAnwvaFZRMUiNarlKe0eBrm6DCy9bzl8veTv7tp1g+9PPs/PgNGs";
    os << "Wnc9Tj17Pxof38s6bLmVJe4aPfe4+lnSnEJqXHDi8x5lXKLcGWi6s+aUKJH8sIJ4rof+vTDGIIrxlK5";
    os << "ZPjg1Oped2WtIiTr7xi2P82fsu52t/s5y1V/0/zky5tOR7+O+HL2b9eQfZtrdIW6/JV770Hzz22+c5O";
    os << "VTEtnXMzl4Sr4zQIfYFBCBzJsKwESRoaZ1YWgTTDsINMTozJCJWpA9bEjkhsZuQBCj1fquFHsdEZ6v4";
    os << "oxbW1e1IKYhqYKR0ElNCm0C6ITghd/TN5fWXb+BDn32c2fIIa5YvVmtjIYQzDl65wXmXLaJeHqJRHcO";
    os << "2NC4/r5Vn9xQZnfIBlGeI1EhilflZpgFxwtb9JRb3pXH9mJm6wakn+nlm27f46b/cSeDV2LbjBX700B";
    os << "CTM5C2LfafOMi6pb/iVVefh1OfpC1TwzZj6lWHMIxpWBaapujNUiTU6nXOW7OQRfPSjJ54nJ/8Ygu/e";
    os << "WaCWMsQ+j/kvXecx8andvLtn58hk+tg8aJWZmZKXHflNTzy1B76On/E8rkB//Sf/YxOx6TsNB/7f7/k";
    os << "K385yPmrutm55wD/8cAA4zMKpxUnKmu0TGVe1Wg4JEgMTRL6fhOWoSw1647LRecvZdXy+bzh2i4efr5";
    os << "ELl9Ak1Jl7ImO6zZYtVDnyd99jx88OEhCQm93ig/eeT6dcxZQLw2wefcZfvnEGEIIOltNbry8g8mSz3";
    os << "f+e5BffLtET1eeMyd288DGMR7dXMSyMziNGqviFM898i3qkzt5/Imf89AzE4xPe6Qsye2v6eGNN13OZ";
    os << "d97O//wL4/RaJSZk2rlyeEq7/jgV/m3z99MS1sbPW0Of/LqVv5rxwD28jXEQyPIrgpGbzd/8Y8PUime";
    os << "pLvg8e2fn2Ei28sVfX1cc9liZicOc/DQcU6crTE65ZK6aC5GW4bES9ByJlpWR6Y0tFYLEUEwaxJfuYh";
    os << "7f3eKdcvyLNdPUC4XeXDTGL99ZoK0rYzOutpM3nVLL5detJ4rVsccu7KDJ3eOkFq0mqgSgCGUaKPFRC";
    os << "QxUTVg9vlBPvOht3D9hXn+/ccbqe8bx+zJYnbaSntq6nR8/Ep+ueMsR//um3zlUzcxMbiPnz0ywtHTN";
    os << "cWBRFVA6ZTGpWtbWNKXxh0occU7V7B903f5wYMDzFZD5nXZvOvWlSxeuhKnOs72PSf49ZPj1J2IQk7n";
    os << "Ddd0Y+kR7tlZtbJr6eitFknVRzNNRLdOVA4IfZfD1dMcHDrGxYvWY+kmLS1taokgdPGcECtl0dHVyqn";
    os << "TZwn9kO/+8HE6Cja3vG6KJb0F5nR0YqXGuf8HnWzZ9AJ3feIxblqxjR//5kOcLt3Mf373UV57zRIO9d";
    os << "e0bC6rTxwaKUbxy1BhfzRTfHHy8jILAsDQdd3snDP/xt0HTy3esKYtc/fdq9uXLlgpvvDXF7P9mUHu2";
    os << "9iJ54zzi98c5O73XMPqS+dwct80Ob3G1/79Kc7v68LxM2x9/H2cKg5weFcRK6UjTJMkjEjCRJW3SYxw";
    os << "I6KaSxIESBSsM/ZDpeMyNEScQMFCNhvEcSMAJ0LYOlpvFq3HQvghwtSVPgzQ0xrSkGQ1gy9efB7Hhzr";
    os << "48r9+n1w+TSGXY2a2TJBKiMKYyInQgzpXrLUIvBqajGlryfC6a5eTzrZw6MQ4vh9hGRamYagmsdQQxA";
    os << "RhwkTRpTVvMDnj09HexqmBSe56XQ9T42fZtGOaQ/0VLEut6wVhSFebwepFFk5ljKmZOrVGxOnhBumUW";
    os << "jdMYvBDnySOGRuf5B/+9k5a0w77927m8a3TDE2r9a7Vi3Ks7IOndoyx50iZ3oXzCYOIKE5YvmQ+I0Nj";
    os << "dLZCPp3w5M4iVrpAOp3CdR1ac7C8N2LLC9O8cKyM6yUYuo5ppdA0SRCE+L4CvwopiGMIA4FhaGimhh+";
    os << "oC/uLn34HnQWJGZzh0efHMAwbKUE3lQRmanKCN1+X5ss/OsXYTMJMNeHkkMOKXmixy7xwZIKN26fZe7";
    os << "RM1TMYnvDZf6LEvC6boTGHtYsNskaR5/ZMs3VfiZqvIXWdrtY0D/74M9Qmd/C9nzzGf/56kMDoYc6i1";
    os << "cz4Otv3TpKRM6xe1sEV61I8+twATx+c4ta3/gl7Dx1nTR+0pl0Cr8qiuRanjo0zNl4nLlZ42523c+DI";
    os << "PoSe0JOWtOUEz+0tMTHp8LW/fTO5VEhxeBfb909x+FSVSQ/sRa2IOEFqAr1goRVstHxa9dBTGlrWQrd";
    os << "0QikYO1Tkqg0Fjp4ps3VfibFYkrphEWJJO7OGwZHNo1y2WjBb8Tg+UOfIqSqZZZ3EXkzixEjdRKR0pW";
    os << "UMIur/08/n/uqt+LUhyjPj7D9QIremm9iP0bImwg+VKDtvoh+d5oaL29i5b4RHN08x3dNKrS1NNW1Rz";
    os << "aSYsS12PT3Cwrlpxqdczlsq+MGDgxz3NWodOc5M+/SlAuYUGhw9OcbjW6fZur+Ev7SDyURj53OjzO20";
    os << "GR5zSHpb0Fts5WgYC6SpKQKREyK8hNhXrpwD+/sZGR2jWqsSBgGablBozUEscOshSThGIWtQ0CWhJ9h";
    os << "xcIDeLkn/GZdPfOICFq5o54Zbf4VMcow5S7jmyix33jiXwXIfn/zAHO55cNiem0kVpqZKfj3iJOA2nw";
    os << "2g/oqvjvxjWSLg5zMZOTo4nUpJz/jV748ucUqx/OSHL+bee47w44fP8JoLbe754d8zf8VrueSSb3Ls8";
    os << "cM0KjHz5vSxYvkCqpPTbP39e1i0eC75gdPKvrISEtccNRPXJNJPEH5EGCmwpW6byqtZQFz3wfWJqi5h";
    os << "wyfsn8HdMoy7ZQT/UIkkY6B1pZB+QHSsoTRfPRmwwMiZyEQgnJBbOnP0LZ/Lvb/Zgec59HV1UXcauDU";
    os << "Hrc3C6shiZm32HSty9OQoZ4fLzFYDICGXTfG+t17IU7/4BH//8Ztob7cZny7ScFziOMI0TfL5LLMNnZ";
    os << "lyQJzE1Os1NCkplsHQlTudHySKbScVrl43M+hWFiE18mmdi9a0UKnX8f0A3w8Jo0Bp+ool1q6cx4XrF";
    os << "9KojlGph1imAosmUYym2+hmBtuUKpuNEizDIlfIkySQKeTRzTRdbRaGJgiDkFq9Bgh0M4ud7SRja2ia";
    os << "0irqukkUempKHqm+rEoikqblqEQzdMIoIUkESxd3sWZxmvLUMWxL46oNbdQbdaSmk8QhjXqFBT02v9o";
    os << "0Ts03yeby5LMFNF2j5qcw7Bx+mFB3QnTdwLZSWHaK2bpk56FZVi7KcuDELFIzmzIipaLwHIePvf8W8M";
    os << "d5fONmntldxIsKbLh4CSvbx1jalaZ13kIe3zqlStM44rYb5pDLFVg71yZwXXQzTeBVqTccTEPy0bsW0";
    os << "lIfJbZ1tj65C+GqXrVuZpnfkyJta8xJw5IFLTjVcYrFMqVKwEwlwOhIkzgxWlpH70qjFSxk1kBYmsr6";
    os << "pUQzJFpHGqMnhxvoZArzKGR0bEsqFYahoaVM7AXtjJoWe/sFpikxDYk3UsGvKH/nOIJYxMQlh7ghmdk";
    os << "0wDvfcjnCH8OpjnPJ2hbM0RLO6RmCkkcwUlftjCghmvYRUsdMtZBOaaQtjdgLSS1sJbOsE72QItU/hX";
    os << "3JPDZun2b14ixbXigxkkmRv7iX9MoO9ILNZFnDtHIKIhIlCEOSWtlO7vw5+H1tPL2ryJolORonppsqk";
    os << "wijO4soWAhTR0uZSlYXCYSrKq+g4TAwNMD+Y4fYtnMHu3fs49Tp09TqDvXJUa67ppe77lxDISVYsqCH";
    os << "lYv70KXB7m3DrL/0AVoWvYr/+N5nuGRdO5seGuJLn32Mb371HXz6bzaK6Ynh3L4Tp7X2OW35bDaj8b8";
    os << "Hyy/LFF8ZFM+ZRjfq9SQti+bS+Tk0TfCxv3+a7c8dYqaWcODQFNuPbuKut/VxZNcnuOt9N/Gdn53g4c";
    os << "0nuOSKdoKag1x9HlqhyoYLP8NNFyxk+RVdNDCVp10YE3s+waxLUvUQXoDU1VAkrPoIN0CzDBI/gXqsG";
    os << "sstFqJgI9tSaFmLcLRBMN4gnPYIiz5BLUBGEULX0XIWRBG2YfKB9Rdy/GQ3D/3218xpb1XMwkZApj1N";
    os << "cLKGnpXobSaBL/nyj1TPZ+/RMkdPzzI1Pkhlup/67CA3XtXHfd95D/d+5/3c+trz0Q2YKpVxPY+UqVN";
    os << "v6GRTaXSpoWmS8emamuwlCY6nAj8ov5cL180j9KpUq2WkFHS0GKxflqNUrRF4AZVaDcfxqFZrfOTum/";
    os << "Dq09RKZ3G9SAFEE7VGmMQhcRyq7REglUopa4Hm5DpuSrI0qWQ4nu8RhRFhpGCrUhrI5lAawPFcZcNAo";
    os << "ibGUUzcFGabpkKbxTFNmnTInbesZ9vzj7F7735MQ/KqS9uJQ5fA8wiDmDAMaMkZHDgdkcvn0DUDw7DQ";
    os << "pcZUKURqJlGUNAMv+GFALAXpXJ7hoo4moVr3zsE3ak6IJi3SpuD1169keng3+09UmJzxufbiLBfOO8L";
    os << "4yDDZxgDz0hXOTiuknabb9HbbzG8P+Np3f4mmacRRyPb9U+w+XKbWCGkvWPzluxbD7CRHDp/EzrRCrG";
    os << "hOKUuVtheunUfgVvHqRWpOhBfEuF6saNeWVCt8rTbGnAx62lTit7SOZhggBFqbpdb5pIZuZshnDVpzh";
    os << "uIjOhGJFxKUGsROwLIV6wjDhJNDDcyuDJHjkeigm5LE9SGO0QxBefMAt93Qy4O/+jWTE6PkMzqvuayT";
    os << "8p5hkiRSwxnTgIyCYICyXGgrmLTkDZIoJqoE1PaPs3gi4MYre4mDmPGOHL4fs+vwLOnl7Ri9GaQtEab";
    os << "O1EyA1C3CMKHmhOg5i7DsEzkBmRUdlJd0Ml70sPoKyqM6o5GI5rKDqWN0ppGGQDgBNCL09hzaPKXNjH";
    os << "WB4zmcOtHPnl17GB/cQUtrO489vB8/Fiy5ah2zM7Ncf0UvOw4N8qNfHOPuu1/DyZ0f5vbbLI6d3sy9v";
    os << "zvI3NUbOPH8vTy1v8T8rhTFSky2pV20t+ZeGfNeDIbBK4PiS/uKERDqENhmFAP0dVpUnIjJCZdbXt1J";
    os << "sRxTKnXw9hs/zObvf5CvfngFR499hlDAx/9iKz/53Gu5dHWKVRd/n/XLCrzmtoUsyejEuomWz6C3pNB";
    os << "bbGSLrUSlGRNsJT/Q2zIY7RnMBS1YK7qw1nZj9bZidOQwurMYy1rQF7eg65qSEWQNZIdBPBGhdSdYi2";
    os << "2icgSzggtb8sxbO4dv/OARGo0GuUKWSsNRF6gmldm9LbDyqgc0WfS595ERnt5d5Pm9RTbtmGbHvmFO9";
    os << "R+lOPIClekT9HZEfOLuq3jkp5/gZ9++m6su6mO2XAMSDEvDtlIYus74lJqEBqEi6kRhrEjXAqLQpzoz";
    os << "wMbtU1TqIfmMweXnteI0XLwwIoli6o06i+Z3cN6KDmbG9jMxWVRcQpTlZ9pO8cLRiSbkVIW1mdI0s5U";
    os << "ZauUKfhTRqNV46UOgdKFqk+XljyhSgAHNsNBQWktDM7Esq+k3IgmDhNANiaKY9haLa863uefXp3hi6x";
    os << "SZlEZvd4qrNrRRq9dJpE46W2B0NkV7eweaNDBMA02X6IbJ+HRdDVqihGpDqQaEkBiajq7p6OkM+87En";
    os << "BluIKVBFKusJIo9Llw3H69RZGxsjKmSRyaX4vU3d/DAg9NsPi7YUhLk3SnyWYMdB6voZoa0rbGkN40h";
    os << "fYRQO96jky73PTrCVEn1jVcuyvKu61sQsUti6MrKqPnOAXS32wR+lTB0mrvjCZ4fI3MG0pAgmx7QYYS";
    os << "mCfSUprxbDEkSaVBN0K0XeaI67a15Vi7KEnsh078+yvh/H8TeNc0/fegdrFpk8vwLM0wWPYStQ80nrk";
    os << "ZKviYkaBrFJ/q55eo1+OWj/Ov9Zzh+tk57i8XVF7Zhj1fwJypotoRIEYqw1P9DaDpzuzL0dFrUD08x/";
    os << "dujtBQjPvvxV7Nz/wTpRW1kz+tmV2TQescarM4MUtMQuuoPjk/XEVLBI1w/Vn+fH5MECbJgkV4/h6lV";
    os << "c7Hn5ZEm6LqOMEEWTOKqYh8k0YuuhLrqO2YshKkRz3gIL0ZPGQR+jOO4vPaWPjwv5gf37GBBPuGT77m";
    os << "Oz359G+V6yKG9H+ZLH17O8//+t7zxhk8xMm3Q8D3e/u6V3P/DrSRA2oKMBW2anwRR/FJc2Ct7i2Ez9q";
    os << "lz9srgaBXSrqZlnTCcZc38DJm0xl98dSP3fvtt1LwEP2rg+QnDJxt84/vf5Xv/+mp2Pn0jDzwQsO3QE";
    os << "Bt/u5+33LiIH/zo/Rx89FkGj40QuwaRllOZRiKQZoIwdaRhENZ8EhGhA7GQJCWPOGgg8iZaHCMMnbDs";
    os << "k8w4GB0pZMYmLrskLkRlDc1MExzUSd2Uwl5vER60eCcraLhz2PTsFtpacoBQAwxdgp9gtNvMPlNk3k3";
    os << "zGX16oHn4YN+xKsfP1JnbadHbnaK73aK326ZvTpnerhT5lnasdDu9nWn+5v0bWNkn+N4vz5ATGkGkNj";
    os << "zGpxuIFTa1eogmpWofxH/w3J6c8dh7pAIJXHdRO0t603S06tQ9h1Taxqt7vPP21zE7eZQtO08wU/bob";
    os << "LOQAnRNJ5VOIUSCZqTQNXWxew0HYSjf4pSdwX2F9ELXDWzbJIr8/xUUBVIBZoVGnICmCzRNb04mkyYu";
    os << "DaSp4QcOt77uCkbHiuw7XqWz1STT0oduFLn+4nY2vzCDiGNMXcfKFwijCNOwiGVCjMC0bcan6gihDlY";
    os << "YxkipfHKEpjxONJFQ8xUoROomUZzQcBX3cenCVnyvwmwlIEkEhazg978ZZLiuk+vpotbicawtQ7tTo+";
    os << "EJdDONZUgsU1KaKaGZBlIzCOOE8ZrGph3TvLVg0FpI8cbrujly+iR7h8axW2yl7yQhaVq7RoFLEiuZk";
    os << "CYFjhdRMJVERssZyi7Dh1gHYQkSP0CkDISWEDdChP2HG5KVbufyi0zuvfAqzr/gIoSQePUiE2ef5z//";
    os << "6yjP7Z1hZNIld2WXEt3rkjiIMBINmTIp7xjmA99+P4/87mHk4na2H6pz/VXn0e2e5tWXdfLwvgns7gL";
    os << "SDJHtKbSMqQ57HJHEEa+5tIP3v2kB7b0XIqTOcP8W6m5I7McY3Tny1yxUVgZxTFTzkaaOvSDP+O7RJv";
    os << "EowfNipKVWE9WAyUCmTLSlnSRxhCykSHSQQULs+USlBsFBl7DmI3QdUhKChHiqRuIGaFIZlkVNJVax5";
    os << "NCh6XzpK2/gB/ds5fmnDtB502VsuLSPL39xBfLEUf7sy5u5YLlOyhYkicMFGy7ltw9s47+3TrJ4bpqe";
    os << "doveriwzw2PMTIfRK+LdK82rIj1JkkQIkbwiOCaVYsVfdcUGd2hwP/msyc0LUhTLPj/+6SO85TqL+qx";
    os << "goqjx4I6EqXrM+jc/Qmd7lqUdNp1tCT/55wtZuWYu3vgQu/aOE2cKzO2ImBidIt3aiq5DInQSLyCuuq";
    os << "AZICPCRoiGJNYkiSaREch8iiSJ0A0bmUgl13FCZIuBcGPiyQC5WhJMeOjH6nB9nkt7e1gfZvnWj3ZQq";
    os << "04zryNDkiSYpk6j4mAYAoKYxA6J6h6tyzopnZhCGmpY4wcJZ0ZdTo806Gq16O6w6Ou2mddts6S3QV/3";
    os << "FIV8Gs2wuXxdmq37UvQPO2RsQ4l9k7i5byyapvRqgBJHCfl8hvJExHjRZeN2n1dd2kFnm8Vl61p5ZHM";
    os << "RkLS3pHn9tcvY8sxDPLhplJuv7qKtYJwrn2vVOrRmkFrTEQ2wLJs4TggiSVshT8M7l+qowCdEU3KT8M";
    os << "qH1FSJKLWmtWucEMUxSRIhhUQXAs0wlHWrAW+7eT3/89BvWLUog2GlOTNcp7fTorvd4qoNbew4UiWTS";
    os << "UEi0S0bQ7eIIgVIiEyL8elZtc0E1BoRmm4RITCkwI0iapMJaxak2HdsGE230aSg7kSYaYukGZxeDKhh";
    os << "4LD7GHzio59k385neVo/RnJhC/6pCqeHqmhGmiQB11e7wgXLRggNgVrV27R9mkvWtGAakkza5CN3LuR";
    os << "T/3qMatiJ1Ax1JBJU+dl86pogZatebpKo91YYksTUmyJ9XQndU8oUXrRZhJYkHo+ar4mJIhdLuhSyVU";
    os << "aO/54jxwfYvHeGgycrlCohM1InffFcZU+QMpBpgZ42iAOo7x7lmrWLaUk5HOyvsiSMcDwbBGQyKa48v";
    os << "5VN20/gDc1iL+tQ16GnPvc48jk9Uufnvx9louhhW4dpyRqsWJghCJU9hpHSicJACcENnbgWoKUNTD3L";
    os << "qak/ZPnBi1svpq4oQCkDzdLBiok92XSNlCT1iHDaBychqAYo/LwFs1VEEKuWjRc1pXQxbrmOaVq0ZDT";
    os << "GpwLmz3f4wp9v4OyxcX5y/16cGH52X0ClUmVZh07JM6k2JO+9sQ0tNcjvHqvy+ktbaWkxGB0LcT1By5";
    os << "zeSX/ijP+KgBi/8qm/5Fy89KQkAURHRorb2o3U+nymVZuYLTGvw2ZlQcdKG5T9BPOsx+xMyJLVWZYaA";
    os << "ResynHHq5bSsaCNs4OSs2MdVMf38dz2g5zpj7j/O3ezf2CIz3/pMQw7S6Ejpw6JVDERoSMtgdANtLRE";
    os << "mgahExF6EZouwA8Jg4AkZag3sR6TBMrzNqoHRNMO8UAd62yNzrUdeO15tu86hG1H6Lpq1gdhAFIiEgU";
    os << "r8GY9wkhgzLHgxIurhxpRFGEaGromma3HTM3WODFQo687xXTJp1rPsaQvpqPFx/UiLl3Xwq5Dg+R7e7";
    os << "CsiP3Hprn16gVU6qFahQsC4mYvcNnCDjYPxvhBwsikw/EzNTasKnDRmhY2bp/CcRz+7uO3Mj2yh6d2T";
    os << "nPkdIN33JyiLW+eswcI/JeLml88nBEwp62FuXM7zom9w1j17eI4VkDZJP5fr9V0Dd00FAY/FiQaxCJB";
    os << "JAZRHJLSHSpuRBIGvOl16xg++Synhhos6U0zt9NmYOAMWT1LIas3s8V+orCDdDqNbigqTBAKPNdFNwy";
    os << "Gp+pqitoM6LGQSEOJxusljz95cwt//Wcm614NUrPQmi0CTZpq0JREWIYkndKQFQMZw7e+9TWMtWmW3D";
    os << "CP2eUaQaRx4fr5CCEp10KqdfWeVVxlSSoUyIew0MH3HhzgM3+6DNOQdLSYfPaDq/jEN48j9SUI2dzBT";
    os << "2KF6UIFRdvUaMsbBH6IzBkkqMVZvTtHUHIQjRCjPUNcaSD1CC1lnMOzJXFErTxJ/2Ady5whjhM2bS/y";
    os << "yPMTaFkTe0EL+Xl5tFwTHmEZyCgmtiUypVF+fpDb776Gxzc+R8qSzOuy6WrT2L3nEEvnZ2jJGypbPDJ";
    os << "Bank7YckjqfnnLpRqPWRk0mU6m0LoggEHakcrtOYNJqZrhAsLaJZJkx6M1WoQhU3bWCCJY+JYGdcZC1";
    os << "NI0cT/GUbT+s5EM1WWmVQj4glXZduzAULEhH6AbpmIACLHQ+QsjERQnyjjRwGf/PNX0dbWwj996WH2P";
    os << "r+HdcsupZJaRlevwQ/vWcXs8Bg//81RDh7Po6cMGnWPdatT9LYLWk0BC1IMj7sUKzEpO8X48EQsu+eP";
    os << "xfGpl1K241d8n4CS37zsXL30B1443v/wGy9Y/bZ6xW+PzQwTpQDfj+nsARFJLluTp6M1RTYvWdbdRpy";
    os << "ymPUtiqcMJkcHGB3cTG0i4fFDCRsuWsDy1QW+d98WCoUM11y1hCeePIE0DVJZm9BpoOdssFLItInMas";
    os << "rExhDErk8ilHcLaYnZliZxQ2InQLbaSFvHH65g5NP4toDN0yxNWRz3JRPDo2R0E8s2cD2fwFVaL9/TS";
    os << "Bsa1Tiienaa1YZJUVPBUNMNIhkqQonQkU0yth8EnB5xGRhzCIKEzlaTXFonSlQp1ZrXcBoOQeAjREYJ";
    os << "sROwTBNNl+f0YHEU4AcK9WSYBk/tKnLBmjZWLsxw5QU9HD7tcNO1S/jVL3ew/eCsMoBKtWFbf9jYcD0";
    os << "HRX37wyOdtqg3PJYu7mPH3n3Nv1/g+TFxovqG8YsWl6946JqOZVgYpuI/ep6HjAV+6NHTrrN2aTePbZ";
    os << "3E8z1uva6bb92zhT1HFQIfyoRhwBc/tow1i/PM6VDZ4s5jJTLZPKauEwmhzpdu4gdN7acQ+EFMEMboh";
    os << "objBAQ1l3/95nJuf0OKDZc2NymS6Fw/1a1XiSOfOPRJ2xqZlEbDjXBD8M00y6/uQ7/VouGGpIomQmok";
    os << "cch40WN0ykOYytBJ/X51sdudc5gcj/jtsxO89TU9dLbZLF3QxsffcTENTw3HoighCj2i0FOZoi6wLYl";
    os << "pStU31v5gySsNXZWGUUhUcklCiEMPmbWaBvXqqM2UA57aMQ0Cbry8k9tu6Ga65LG/BulVHYogBZA2mh";
    os << "+SDk6CMzrDmlye3vwon31ggFrWJjnjgkhojyK+8clVtOaMc9li4+wMqb6WP1QIQqJrgrSlep6ZtV0kf";
    os << "kz/4/30dqnzF1VDhKOADzJOEN0pFZRLHqklrRw4MYNtKdMu0bRVjYOYxEzQ0sqpMZYxiQ9JxSUkIhp2";
    os << "CcbqaJYgGqkjOiKVYYYGjckaTtXn+ksWs/PgIP1HpvnYRxZz0aWLeGrPac7fPUB79zjz5y+h3yhgJyZ";
    os << "333EBldkqoxN1ZsshkxWXat2j5idMjgbUfUksskyMTlH3Cv7JQ95Ao7sAACAASURBVEdO8397s5yLe/";
    os << "IVwfBltXa97pWt9nn/NT014SehSaVmMzgt6e93GR2uc3aozolj09RnPLYdGOOJJ4fpPz3Gxoc28uzmY";
    os << "WaGfJ7YM4ofRLxm7UKODpxk94EBNt33US5a3UnouySBah4LIYiCmKjWIKr6hNUAYRuIJCEKI5IoVH0u";
    os << "XSOqeSRSInIGyWQdf7CMFBLRbiBFjBiOWLqwl0Pjk0xWJ7B0C0MzSJoXhUgEifSJ/RgrrVMfKjNvTht";
    os << "rl7eRoAKIlLqivcTKVY44JmunME0DhFQar1kfy1R3fseNsC1JpVFXWZwQSN1C11S5m7JtVU42T2MQqq";
    os << "MpEEokPtkgV2jn/XddwgfefgWj/Zt4dk+RyZkABEjDalq/KpR/ttkfAs4NWhoND1PXyOdtfvfo0wr6K";
    os << "XWCMKFUCbAsE4FASO1/BUUhBZquYxomqXSWTCaLaWo4jRp/92dXsHnPKGkrw+tvWE1QH2b/iQq5XJ62";
    os << "1jba2jpIZ7LsOVJGagZtBYPrL27Hd8oEkU8iJKZpYpnKFMtKpZo3h1ABX+shrh8R1mO+9HeLedV1Jhd";
    os << "cdoSJoHru56ZKHqlshptvv5m9h0cJvArZtE5Ph02tHrFmZQtSZJkdcRlYEBP+o4eTr7Jy2TwCv8bZ0Q";
    os << "azFR/dsnj5mElgWibpeQt5fHeV/qE61bpPkkTc9qpeLlmbZWTSJQhj9h2dIgpdkiTG1CW5jE4+oxN7I";
    os << "YkUJIkkCSTBdB3d1BCh8tnRCraSKc360Oz/IiRhFFOphzy5o0j/YJ2OFpPPf/Q8rp5jUds9irQlss1G";
    os << "T5mYWQuRNxGGSW3vGDddt5Tfb5mkahm0vW45LZcvoOONK5mKBOPTHoaZprWZLdb2jjd9kV68GUjlPqg";
    os << "piVPiJ0hLo+Xtayku78ZcWEAzBURKW4ilEzcUkVEzNCUe1wysphQMQygtZhOWgaHo4CIWJFMBwahHMu";
    os << "2RzDokFU9ZKoQh/kiZuOzglOqEQYSuRVx+yRLu/c7b2PT8ERpnx3jLVYsJ/IRDh2fpP1Fl665TbHl8E";
    os << "8dPlnjk6VF27h9hZqbOycFZTp2cpTjscPKkz0gRpkoW9WpMGPphSTc3V+p1h1fsOfNy3+cEiOVLro7/";
    os << "FRSB8CcP/e7xqSj37UqpVE9bEl1LU6ykODmasHtfld2Ha/zst6M89swEz209ycZHj7L/tEOnLLO7v8a";
    os << "ThwI+/Vc38K37tvKBDz7KR95/DfPWzuPH9+/lPbev59XXz6FSrEKIEm8LxZHTDAONhKAcKNFqmBDVAq";
    os << "JZBxlD7PqEE3Wiho+WNjDmZEn8GDQNkTY5PVZmQWcXGSOtJOlCuewhFAdQ0zU8JyGTMfGrESdOT/K5v";
    os << "3gtQkAUKWQUKEQWsSAkIUpi0paNoetIKRmeSZNp6SOOE2arISTgeSF+EDSPW3PaJwRhFEAY09OVbwZc";
    os << "xcaLowhN6tz/6AiBV6Ul5XBe3xi/2jjI4VNV7FRKuRjKPyT1mhRcfdnFQIIQmpJ1AI2Gw5w5Hfz+ia3";
    os << "ESUwmJblo/QIaTkQQxmqw0WyL0YSppm3lZieFQuTHiYKkptJpHNfhA++6gXppiNlaQrVW5o7XLefnj4";
    os << "0itDSpTLbp0ZGQTqV4bvcMAKaZoqvN5KoNbZRmimBqamKaSBJAN1VGOj5VPTck0iKDj763i7DqcMkVZ";
    os << "5jsbSG73lYXZqJ8XjKZNCPjkxw8McvExAQJCSsWZmjNGrToDebPSRjf6eO90eHNazXGx2tcf8UqxodP";
    os << "cnKwzsSMj5kvND8TDdEsoTXDxk7nSHXP5acPD1OqBpTLVRqVETrsSVUZhAmnh2uEfp04CrBMVWbPabc";
    os << "Iaz74ISKlgRapY0ZCFAC2QegGqtyMYjXko7ktIVX2nGRN7n90lOMDNeIo4JPvXcOVc0yq20eRCIzuNF";
    os << "qXhdQ1/KkK2YEG127IsHHbNJm13YqFaCgyTnppK8/uniEhoTVvcuX5raSmqjhnSuf27V/2eDF5NHQ02";
    os << "ya9oh0jbaJ3ZdA6TUV7DxOCUoNg0gOhKTsOZLPaQakZbB0tbRDP+EQlj6AREPRX8QaqqrU1WCcs1QkD";
    os << "D+90UVkT6xpBPaBRr/Ohd6/hMx+5kn+75ymWrlvHxz70Bu765GN8/77tvP1tG3j6uEdlxsWbmeXw6Tp";
    os << "PP3uSZ545wdPbZ/n5I+Nse6HM/mN1jg4KSg2btN2CZQRJw60FZ93UrjNDQ5P8ke2VlwTHc9mj/CNvUQ";
    os << "IvFzbuOX5m2/MnZ/5+Jkq9YMRBuGF+C8t7+1jQM4e03Y5MMpAY9HS20GKbLO9O8/MdDk8ccrjt2vP5s";
    os << "/ddy8bf3klsSEbGKjz1+53U3JBPvfMCVs9fQCJ93EYdd9YlrntEjkNYrBBMVIgcBxlExDW3eQVrRBWH";
    os << "aNZBWBrGvJxq5FYD8GLiMEFqCu9umzqJ42PoenNfRyi2XwxxFGNnUiSeoHtOjr2HRpjf18ef3LoCZfI";
    os << "UIA2LJElwXZcooul+F2LqFgkJS5auxMq0M1XyGZ1yKVUDteImXmzAq4tG9Zp06o7HvDltaLpN2taUnE";
    os << "FKkDrP7J5hcLTG5PgYj2+dYuehWWarEVa7+l3TswlWuoVMWiOfz1Ku1UniiCj0WDY/Q2veICEmZduMj";
    os << "I+yuLtAypR0tZucHKojhMDzfaIkBpGgaRaGlWPhvDTZtI7rOpTLJcrlEtPTk4wMnuXVV6/j3W9axY4D";
    os << "k1h2iqsuXc7CuTY7D83S3tpB2rbIZHK0t3fR0dFFxdE42F9GSI22gsmrLm2nMjuFU28QhiHStjAzGWS";
    os << "soRsWE0X3XLbx+c/1EPgBn/1uEe2SHpavXUjaaUVISbXuomuyCdyI6F6wgB/9eoihcZe2vMkdr+lhy1";
    os << "GPefmYixaneN08wbOPDfOmmy+nUVabGwNjDpGRRjOVybxpF8illbBaaiaJbmD1LKAo2nngsTFmq6qJP";
    os << "z0bUK6FhFGCZxk8t2uEIPDQNUFr3mDtsjzBTENNaYMQoyOtNJ7TDnquSdMWMeQ0pK6hCYEQGppuk7I0";
    os << "smkdhMAtpPj2fWc5O1whCj3+4l3LWJIEVHaOqBRF04hrMbNPnOFj738DZ88OcLYakVnfjRQJRoeJ1pU";
    os << "is24Om3ZMA6CbadoKBjdd3UV9/7gyTJMGupEil9YxDaH0ji0mWkYZmhltKfS00i+igdGTRhOg6wKRM4";
    os << "ibN/okDvH8mOlZH7svp2weqiFJLSCZ9QlP1/DGGsQVj3imTlDzCfwEGSlKeBQnhDGEQkMaFr12htddu";
    os << "Yi6G7D76Rc4uPsYRlbnn3/wVr715bvYcP5yHtrv8OQRl8WdGVo0jc62NpLQxpJp8ulWFsztZfXiPtZ2";
    os << "5al5fjjtGoMbD04+eWpwfOgVgfAcJqz5fCloNnllpvgyreJLXuRWat7Mw8/s/fEvdw0/9s8P72OyWET";
    os << "aalKkm9DwIw6fcvjv52b40oND9A8FPPCtO/jGF6/mO1/fQovIceLQpzm0/wTv/PC91OsOXmRQm6qRbb";
    os << "X5+fdvY2mnSaVUhUaoFPv1AF0khDLBd2OiuksShMSxQDMNjEIaEat95CgKldg7jJA9Jmg6g8ODePVZp";
    os << "GkhhUSTCZEIkWhKEqQnNMo+pqH4f8Mj43zo3TfwnjevJSYi9j0EyvA7CkOCIML1Air1Kn9y10289lWX";
    os << "0CgPs/94hcFxh4abKJKMrkMSq6yiifdXKsGEFUt7CP0GmZRq0odhSBgGCKnxwBOjjM/4HD9bZ2TSRbN";
    os << "1nCggpWmsXDYPgaA1p0g3/aeHOTlQYmigH0MX/P37luI6NTY9uxUhBGU34PZbr8QtHWHLvlIzUxMI1P";
    os << "Q+n88ShS6FrM4t1/bwjlvX8663XM7b33QZ73zLlXz9C3fz6Y9ex+Dx53luT5GeDpt/+tQdNCqj+GGC6";
    os << "9WbUhpJJpumo6ODnp65PLmzSBS6uF5Eyta49sI2RocGCMMY3/XVYAxFLgoDB9dT2cuxg1X+474quXXz";
    os << "aPUl9UMlXLeOYac5PVTD0AWZXIYlq5ewYOUKdp2G+x8dYWjC4fyVBT5y+wK2vTDB0QPjbNwyRFbm+Pi";
    os << "7L+bxTc+y/UCJsSkPq6WNyA+Iw5C+vh56Om2SMCKqV1UFYZm0rLmAbacSDvZXmSh6jE27lKuqp2h0pH";
    os << "noqXEajupxZlM6C+emmN9q4k/ViP2EuB4Te4HyxCmYSFMQJwkiEUQCnOEKQmrEcUiSJOQzOsQJ1rwCR";
    os << "cPgpw8PMz5VxXFc/uY9i+mcrDDz+EmCqQbTjx7lxgWLufmGVcxWfbWlMqssBDAk2Dp62iSek2PHwVl8";
    os << "t0YYJVyxvpVu16V+Ykr1WJMY25Rk0zpJmCi5T1pDZnRk3lbe6lJJbDAh1pUXjFYwkRm16eN7FRpuc5L";
    os << "uRiR+QlL0CSd9/JMOwekaUbFO42gRf7IGxMi6j0iU/KvqNOibY/H4L95Ea0Fj5/EZpg1JFIXc/bf3Up";
    os << "ud4uShf2BpS55vf+kJfvi16/j2F97MidGQr/xykIe2znJ8qIHjR+i6alHl8ganh8/w9Yf38cQLgzs27";
    os << "Tx+0PX8xksCoYtSn7645vfKwBgBkfb5z3+eL3zhCy8l0b6UwP3iLrT+4vfzFy42xicmLjs8WGRf/ziH";
    os << "B4ocHSpxfLjCwEwDP1H2ox9843rOjMzw4O9OMj1Z455f7ONP33kJt9+4lCMjVQI35OcPH+PpPSeZl+3";
    os << "mH//qOqar02zeOYqpW+iGhu8EVMoB7lSVwBek86YqqzOWck2ruOpDlQLN0ImjEHSNeDbkvAXLcavjHD";
    os << "gwgqYJLEvH8wOCMELXZbOUUGvfxVKFmIgVvZK+OWkuXNfHrTduoOHGVGsulVqDOI5Jpy2uuvw8/umzH";
    os << "+KOWy5mdnQHv3tiL9sOlBgYc9T0TUqiJKa9YHDx6iyPbZmi2oioN1xWL+vkw3fM48DBQxzsr9LdbuJ6";
    os << "MbO1CCE0JosNlvZlOHSyytCki9GZwZ9q8MG3nseyznGe3z2K40V0tEiGRmfRzRx7jkw1p+oJ77utlzt";
    os << "ev47rr1zFHa9dRE4O86NfD7HvRIU4NtB1jSgKuO3Vy3jT1Tpbdh5nz5Ey2bTG+sWwdoHH8p46C9orOL";
    os << "Mn2fT0LjbtmKZYDvjiR5YwNXKErS9MUsjoHD1VJJ3JK8G3FFhmSrnYlSfo7TLZdqDE6WGH7g6Ler3BT";
    os << "DUhX8gTSzCtNNOTY6yZD8fP1hia8CielWSXpohsHReLuD2DTGvUh0u89tI5jE+V2XlwhhOHzzA5MYuZ";
    os << "yTE8Uef4SVWyd7UavO3GHlYs6uFNr1nFnTdYPPToTn65aYyBUQejrYsYE60yxl+97zJ6swOcHamTMRN";
    os << "OHDiF1jkXaaexrBRIycFdJ+hqMzlyqsbZMYeZckCUNpkuB3SkJW0FUw1aDGVWtvfgLOnVnQgvRqQMtK";
    os << "yl3CpN1ZZIyhFh0SF5pJ+/eu8aRgePs3V/iShOaEtLBqZc8ufPZXCwiqj7zJQDhiZcLl/fSrvrsf+X/";
    os << "dx1+SruvrWHfbuf4dCpKiKMGRl3sBa0IqSOCEHPW/gTNeYlLpV6yJ4jFUanPK7c0Ioz4vDeNy1idKif";
    os << "XYfLtOYNNMdndNIjs7wDrSNNMush8rZiN/oJVEKwdWRWBy9G77QJSg6cmSKMYg6cqJJZ2A4eCC9GCxP";
    os << "iskM4WSOpusyOlXBLNXwnxtSkgi5HEV4ccestK/nTW67iOz/czf7jQ2x7eoD2DovzLlvBQ/e/GysOuf";
    os << "3OnzIwXGX/oSnGRmdYuaSFF45O4CI4PVHl2NAsh4dmOTxYZNexMY6PVAmVt/xgFEW1VwTD2kue1ebXF";
    os << "3ee3WYZHYokSRAvLpWqKGE0nzaKyp0F8kALUFi6aNHigaGhL8RRqF+9vptTUw2GRhsIJLe9ehEL5rby";
    os << "g1/u5/c/uoP9J6t86VvP8Mx9b2b1zffwjlvO57v/9m6CiUm2Hy3y9R9uYuuOcRbPXcBX/3IRj+8p8d1";
    os << "7DxBFDUwrRRDFZG2ND7/zYuYuSvGpHxzA0HVEGFGthCRRSPvcLGbKRORskpRCx4tDM3z0zjfgVYd5Ys";
    os << "8kXrWEbZnMVh1qjRqWaeJ5AbquYVkaJ/tHEEbC9Re28vqrusikNBb0dlLoWkWm0Ith55VeLYEwaFAtn";
    os << "uLs8S385plxDp2sMjzhUqkrFzsEJELDcxt845Or+eqPT1GqxKxYaLO4N83olMdE0cMP1N26vcWkXA04";
    os << "NewACZeua2FgzKGUaIROzEVLbPJZg5EJl5mKmtrmMzpteZN9JyoIYRKGHm0FA9uS2KbqEYZRwtSMx0w";
    os << "lpK01S6UeI4i5cGWKtrzOyKTLVMknenH4JDg3yEng3KSy4Ub0diudYLWhSqaUpZHP6JwYrLNg8Wqy6R";
    os << "S6ZnC6fzerF2eZnvXPbYloUtCSM2i4Ednu1RTa2kikwcTIMNnwDKVKwEglS8/cDPM6JXrewrANJgLBm";
    os << "cmY+PgZPvOuuew+XOaRHVXaFi3HyqYxDZ16uc70mX60oEZLzqAlp2NbaohUqgRMFD1yc+ax5Px1TE+U";
    os << "mDi4i+svbsdxI8amPaoNZZCUS+ucHXPQ1lxPpr2NRCRMH9hDmzOsNlfihEotRCxoRViS+ESRv3vfUua";
    os << "0W7S3GDhuxBd/eJLhliyt1yxEFlLoaQuCqEm5FnjDFbyf7uGqDa2MT3uMTrnnMq1CVscyJUPzO/C3DH";
    os << "L+ijxj0x4NN8IyJW15g7aCSZIkFGcDZio+YZSQyyidZ3HdPDKruhFSEMw6tG46RkeryciES7WhZEiWI";
    os << "ck3M8BKLcTxIjQpKOSUp/TUmrkULp9PEsSKsJ3WoOKDB1qnUngEJQeZM3EOTGE+dIB0SuNMaJBd1EUQ";
    os << "RMyMh+ihIJOOCccrhPWIr3/uao6emeG/frYb1zeRIiQMPfS+ubz98l7uuqaLP//OWc6e6ed1r1/Ge26";
    os << "+iGsvm4fV2snrX/8Vjk6G/PtfX82nv7mZt962lg2rurnzLx/ky5+4lr0Hh/n5Y/0IIZg/N8ucgsmeY1";
    os << "PkWzviRqP2rOM4Uy8JhlWUQVUZmG0+X/x37SUZZPDHdIovltBBMzv0mi8wAP3kmTNDl15w0b37Dh149";
    os << "4mRuvzyJy7mka0lfv34ADlLp6clhSZiDvdX+cjd13DFil6E1LhsWY57f7ufmHt463VLSWcKvO21a1jU";
    os << "s5gVrf08s3kb9/6uh0jazFloURxyuO26LB959zLmrvn/lL1plF1nfeX9e54znztW3ZpLVZoHS5Zly/K";
    os << "I8cBgYzDgYAKk8RsCYQi8GcicdIB0IAmdlySd9BtCgEBDICRMMcFggw3YxkaWbcmy5lmqkmqe7nzumZ";
    os << "/+cKqEECadvmudVbWqPtRw791nP/u//3tv4cjzEwTjc/ixgWZIhl2HysYKC1FM9dQUXhLRtW4QI5EIS";
    os << "1ByWszVdWpNDUcJYiWJyRK9U5VZBjQhsC0HNOjrrfDcyQ7jU+OsGrAp5eYZ7j/LcK9NMZ8VP9VbEZOz";
    os << "PqcnPGYWsvDZ2cWAdidBCEnEchfv8s7xvqM1ojglThSHz3Q4fMZbTsHO/t1KKcanO0BWgGW4BgdONjL";
    os << "Td9kiDT2ePVQHobh25zZKPSbP7jtI29eptSVdpQqFfI41a1bx5J59zC56y30fJmbOZMeWAfY8O0F3pY";
    os << "RtdZitRhweS1GpT5ymKJVN1TO2lykpmqaj65nnTKmUQtlleqm9vAaY6U5+rGgEBqtGh3EsG9fJ45Tzb";
    os << "CvdwczkJCEBuuMThAFRkjLbEOTLZexcHpVKYqEoVHqZHmsSy5Du4TxzLZivS1aXYGRIsa5bkC+CtrHE";
    os << "noM1xqc7OOUyKYrAD4nClIbnIcwiN7z+1Zw9eIS5eh3VluRKLlqfya7bN+PkiwgRo6TF1Mx6nhiP0RA";
    os << "kKDAU1RTwNPKbhzHzBTShITSNvit3sTRWJK4v0Wm1oNvB1VK0ik4wFPKJr4zxS68fQeEw2GPzx7+ymf";
    os << "/6/x9nfvcFul+2lkQDvWCTdrJeH73Phrft4gcvTEHZInXzJO2s1KoRpRh5i9yGXsz+IgePz0GfRRolt";
    os << "BQs6RKptMyi0+tAvyT1E1pxglFxcQeKWUVE0cCy8lRv2cDkWI24x0QFMWkQIzTJghQITSL6JSpKUVFK";
    os << "LUkxhwq4qwskXoC1tot0sQPVGJnXM6P1bAe6HYgV4bkWIpSEN2xg9ugsoS3wmh2KusWVozbzB3zmJtu";
    os << "oqI5AsWlLiTvv6uMXboP3f+QIh8ctKgMui7M1vvVIjtXyBe6/YZTjO27h5bvKOFHM4w+9wN89cJwnDk";
    os << "5zzbo8N+7cwLe/uIZKb47vfuckgpTBosZgt4ukxF23DfGqnUU+8r8O4DguveXS0RNLC9XlY3Fn+Wpfd";
    os << "nnLX185Qq8MW1Kh1ApTuMgWNX6ys8VeZosrjLEElN/51re+6fNf/uqrrl5X4jfffRv/8vAcjzz1LJ/4";
    os << "nRv4xAOnOHRmgV++92re9oYdbL12mLM1m6t2/QEaZZy8zx3b89x39y0cPTdFY+ooN92yg9NLOn/12WP";
    os << "8wR9t5+HP7mWoG7aNWnxlX4fjZ5uQwMZNfdx5S4XffkOZZ1+I+aU/PYDva6zrH+Hs7EkKuRzOQBf337";
    os << "OTrcN9fPQzLyCjJXJ5Fz+Jadfq2Zqh0FBKUSoUGJ+YotPxWLNplHqtyfzUPIVcliLiOj9pX2l3EsIov";
    os << "fgRkc2YL3rfLqoQ2Y4yIjOKXzQLpwq1rKllKTQZWyiP9ND/xrU0ji+x9PQcYXN5nUwprtq2lTtuv57P";
    os << "fP5rOLZNX2+ZjhcRhSnXXLOJk2fHOH78DNLWMXtymKMWb7/pDbj+Pj768b30Dw+RMxVTSwGaAk0zSDU";
    os << "DESeQdtDNbOBg6jq2aWZSsxCkKFSa9VcrpYjiJFsz1A0cx8Z2XBzHwbAdpKVh6zaGzIKEfT+gUa8TRQ";
    os << "FprFCaxHLyuMUiCYpmu0McBvhtj8APMF2TZgReR4DScfOSVd0aCR1OHZlEKYFTyiF1F7W8eZMi6SzUu";
    os << "enVtxGFIUnHo9Bfwe+E5IoOzYaHTBWWaxKFMUeePYpZKGI6LulyordKstBc3bKRjos0zawWY9k2Qugx";
    os << "szRLZ26RdVe4LAV1ZM7EO73IsNfh7lv62DiaY91IiU4o+L2/foGldT0U71iLZmVyinR10khBEJPqgrg";
    os << "eoJoRSSciqmW7+NKQCCHQKy5CEySLHZQGiRejWVrG3jSJ1DVSP0ZFCWkjJI0UetHEWlNEs3SSdowom9";
    os << "CKCJY8lB8TVX1UnGT+X0PLSu69hGjaQzgG9o4upJJoto41WiQKEkQ9glQRz4VZVkCPRbKQOT6c9WWqY";
    os << "zUWv3sYS3QRdJlYcciX3rWaHaMp/+2BOk/uXWB8YhFNF9w04nD/3T188l8nKY9WeOUvXMVH/uR5fve9";
    os << "NzAkxtj3/Dl6197A2p48X/v+bnYfDmh6CbaV8PV/fh+vum0Nj35lLw8/dYrPPniMoYrLe35+E3/8iRe";
    os << "4Zdc2Xrqzh2995zlOTnWoFCpnTk6OH1JKrcSBtcjqS1cYYp2fZIgtfqwvRqxoisCKrsjFd/RPf7w0d1";
    os << "GePHfuwv33vWX0B8/t7T83PstrXrqe/SfmePrIDF/4q9fx6A9P853dZ/jyw0doLSXc8aodfPLTj5Iqw";
    os << "eZVqzg1b3NifIZceoFjYwmz8yGbh1O2rIoZlXD9zn66Kv30j47y0ut6eeUr13PfnUO88aVdaCGk4SK/";
    os << "81enWWz18Yqr1vDAP97Flbe+jq9+7WG0QNEzNMBrbxrgR4emaTQ6GFq226tpkigJsmCGRKFbOlJoVGt";
    os << "VGrUm5d5u+of6WVxsUOkbJox0JqfrLNZDqo0Yz1f4kcrSY0Q2ab54SYkQOlLqGfNSK99bjr1aDm+QQl";
    os << "/eKlE4XXn6rluFNZTDW/JoHa/iz2XxXqgEQ9f55fvv49Spc5w9N8HwYC9pnBDHMWvXj9Bo1Tl8+DTln";
    os << "m5Uv4U+YLG2e4h7rvN5+lmDMxcW6KSSmCzNO0qzEiLL0LI7n6Fl4RqAadmYtkUqsgmhFII4TbKwX6ln";
    os << "Q6ckQTN1HLeAbhjYtoNhmNiGhSIlShVCami6hqZniTBSM7OIeSHRdR1pZCuclmZkFipNQ5gWbt7GzZn";
    os << "YeQc0m1qoM9+Q5LrKuAUHJUw0w0SaJoZj4ZRLxEJQa7bZdP2VKEQ2zdcMNMMgDBNMx0UYJoWhXqozS/";
    os << "gKzHwJPxYEcRbiInRtua7TRNM1pGEgDCNjVZrEMh2avkdPT5lQ95AlC2u0xOKiz/Hn57EtSRzH9HVpv";
    os << "PIlo/zo4XNUZz2cdWW0vJHZWDoxQma1BiyHJUstS+HRXGO5+lRHOnoWZpysBHgs90OnAmFmAQqpl2Rp";
    os << "O6aeeQ7zFtI0IFZoRSPz8LajrJZUExmoWjoIDT1vIrSs0U8rmGiOgV4wEXb2uo3aIdKQpK0UfNDKWlY";
    os << "KV09I2wFGl0NjrkXtm0f4w1+6l996xTCTJzucPx/zzN4x7rwxxFAO9969mpfdPcJr79jEri1DpLkKV2";
    os << "7r5rqtJm47YPWoRbfq8OieBaaqATm1yAPPNBhbyFHJl1iszzM8UOIDH3oLH//7h/j1D3+bH+6/wBUj3";
    os << "Xzlb+/i9/9uH0rpvPONG/nXh17gmSMzOLp5fmJp/pBSaoUdrgBjcxkYL3Y7X8IWV5jixenzRVC8ZOBy";
    os << "6WPlSC0v/xiGoZqcnjh94w0v2fHE0wfyptbmpuu3cPB4nbmZFn/7oTuZm2jywuk5dr8wxUPfGGPDqOL";
    os << "X31rkyQMeUVBGJG1m5qpYbp6h3piRgTxve9MdVEa62LhliKt39DM6UmHdcIGdmwfZsLbA4UMLPPf4cT";
    os << "7374ssxFv4xw+U+MDvbqX3mvvYfs0rsEtnePjbB8lLyapV/cw0NRYWlhAyBaWwLJM4jknTLCOQJMV2T";
    os << "Ko1jySOadYa2EWLRCWs7hqkXCqxOBhSHCkTTHmolcrOzAad8UIhs8+FWF53EqjlDQ4hBNnoSWaMK81W";
    os << "xQzLIj9aICL4mgAAIABJREFUovKSIZxVeWqnl6g/M0+wFCI1cXH75Rff8lqkpnjgoSdYM9KPbdk0PJ+";
    os << "+nhJBGPD8/qMMDQ7SLKfZbmrF4j2vfAV//MEHSHNdlHsMFhoxMRJNGkjLzCbkaEjdQDd1NC0zpKs0zf";
    os << "qsdQtBtie8YmFJooRUJWhSUsiXsBwHTTMxLAPbdXBcF9NxEJqZ1aBKQYIgSbNCMqFrIHQEWZRWkqSEf";
    os << "kSUhIRxRCeMSVJBqkDIFFPPfM6WIRAk6KaF6ThIU0OzHAwnh2ZamVWm3mH0ivWkUiNK0gxkLRPdMFFS";
    os << "EiWKYqWbTrvF3LyHMHMUc1AuZLvWpq6DrmFYBppmIo0sqScDb428U6DebtKpBVhmQGqDRMMeLdOJFQd";
    os << "3T+OHKa12TN5R3PuKtcydaHL84AzSNpBCQ3OsrPApStBzIiugkhrSFGgRSNtAK5uAhhJg9GbGe2Fk3k";
    os << "5ZyNr4VJKiORJpLU+KbT0ruw9ipKUhu21UmKK8EBWQySNquWe9mZWeyW4r+3klE6PXzna0dYlMsxBbl";
    os << "iLSKDOkR1Me0b45omqArEZ0zjdZeOQw7//F7fzRn/8tI9tXc+e2Cbb11fnmc2V+8KMxumgwvK6fO29d";
    os << "x6bhblavqbB6pIurrl7NyIb1FAsJd9y8jdNHD+Eri/MzEWcv1Gh1coShSxAv8jd/2Es7Mvnwn+7lwe8";
    os << "eoRN2eM2t6/n7//ZyPvQ3z+D58J43XsVje47ygz3n+aW33D/z3MH9z6dpemmArLcMgpdflwJicAkgpk";
    os << "B6ERQvY4uXPsRln68wRtH2PLW4MH9kfc/gjoOnp3LtRpOff+U1fOWxE+w/cJ47btnMW16zhRcOXuD8X";
    os << "I2P/95qjp2DBx8fJ46XeOXVGrfeMsTNV/fz5vt2sXrjMI12gO+HNKsdFiYnaDUTzh8/zd4D5/nSF/ez";
    os << "95kx9p3XeNVdV/DVf7+BXbe/nUgNcergYxx66qvcuHUzh0/PUmunbFrdQ2u+ycySDyIkSVJ03SBJEuJ";
    os << "IIUQ2zjdNi2Zj2UunTJrVGuvXj+B7HQ6cOkmxv4TT51K4tgdNCKJqnKXHLPcxZ5aLzGScpsnF8AeVZu";
    os << "kyciVMQCjsikPPDUMM3rMetz9H/UiVxf0LRDNtEj8LX0iSCCkU//UPfwXHNvjqV35ApbeLQq5As9GiV";
    os << "C7S9jocPnqKvK7jD1gIV2IM25jdFoO+zv6Dc2zd2s/MfEDLV1i2jdAzO4UmJMgMFDO2K9AMDVM30HQT";
    os << "nYz1KiHRpbbydCNNfXnrxca2MzOxNAw0Wyfn5pG6JI2TrA7TtDKWKCRSN0ikwLCzJGqh6yil8IiJoiy";
    os << "jMRWSWMuYT5xCgEAaOuhguC5St0h0iabpGJaFME2QGqmmIXVBub8Xw7ayfhvdJFEgNINYClKp4bg5ml";
    os << "5KEjXYvsnCsSVKSgxbp1C0KRctigWLREiCyMB2dXTdQNd1dNMgQNKuLTLsFmlUq8hcxr6MPhdZshgba";
    os << "3H6RI3ZxQBJxGtf2s2QmeepLx4iAqyNXUg/IY1SYk9lxVNhivKz5CEtb2CUzWzHP4rRenMYfQ7Ki9HK";
    os << "JrJkIlORAVzZBD9FBSBM0CxIPUXaipEKVByjEg1Ny/aORZSg2ik0ImTORO+3SacDVCNGtZOMncYCtZC";
    os << "doJSXkiyGMN8hOTRPMOWRtn2SqsfSsQledvtVfOzP3sy+b36W1GxR7t/GDffczbvf7jF+KOWbz9VoT0";
    os << "1yaP80C7OL+H5Ip91ifuI8iXDxvTYYOVav7eIlN27FShdZv7abHD7PHr9A0/N542uv4Ka1Dv/4b8dZP";
    os << "+zyN39wF6Zl8+S+0zz0dI3fuP8a/vnBZ3ns2Uluu+76xe899cTeIAwvB8SVAcvlLHFl2rxyZL4UFNVF";
    os << "TfEi6glx+XFZ5yf1xRw/OZEul1x36OU33/KbDz72vUpP2eGmq1ez+/AMG/pz/P77bmLL1UPc+9b/xUf";
    os << "e1c9Xny/w2PdO8LFfG6BSKTE+Y9DbbTA80ocf+gT1BsKyiTtt0B3a9QbT0/McOtmmt2c1m269ldf83J";
    os << "10d1l06jGhH/DEN/6OuZpEC0wefuwkM6rD6bGQd9yzg05ksvvYIu3aLFHHJ5fLITVBtVYjTkCIlFKxx";
    os << "PTMIguLC2jSIUljLFtDsw0MR0CqE3ohpq2Tv7kHTeokQUQUJLRPLhHNB5l2mEpSGaHi5c4LoWcZgj0m";
    os << "ZjFH6boSSUthDeZpHlykdmCORCQk9STrZEYBIbZt8flP/hkH9x7h4194kL5KmVLBpe2FlLvyTM/OMX5";
    os << "+gny5AP0WuAZaUUfvt5GG5F3rdvKJL+1lx6Zezl5ostT00Awna+JLYpIke+NYQsMydAwzxTQtdEMnDi";
    os << "KkAsOwQAriwGd5H5FUCAypoxkahmlgOA66knRVesgVcnTaHRIpELpERxIFER1CEiXRlUQZikRI4lQQx";
    os << "TGJ7xG2AzpRQBjHpHoWQJCGMeQc9FyONI4zLVbJ5VU60DSRsScgCmLiIOaKm6/BtEzCMHsuZJoQxima";
    os << "0FGk5LoKNGsRcyf2MzhY4PhkRILCtA2UVKhOjCk1DN1EGhbFnEWisgARgSSUkpPP7uaqdf1MLR5HlDS";
    os << "kZWZJTTokjQjv9AKdsSrFOGbrujx3XFdhsMfmH742yRk08tsGMNYWwU8QsUTaYrlNkiwXMwJRlBgFHW";
    os << "Vk+qCqh4iiThomqJgM3DSBSlNkqMAUJHMhSRyjSaCTkhYyhimVQlgGcdOHZoxyNRQSuTx8Ic5iuhAgD";
    os << "AMVhNk633AeWgHhWI2lpQ6WISn3WHhKJ7Alt44U6Jr2uft120GPKXQrbr/n7RhaGbfi02zafO2LX+Xs";
    os << "c08TeQsMDeqMru6lkDdQK1X06OS7K+imxviJ8yw0Ulav0pgYm+WPP9PkXW8ZpWTP85Xvxnz2z17P2ck";
    os << "a//zAPn54sM49N/XywPdO0QgTXnbdzYs/3P/cM77vrxyZV6bNKzri5VriytF5ZciyAooXAyF+ChRfBB";
    os << "jlJcC4YtPJAYVLgLFULhYH77j+9t84fOzpiiHauLbJ4fMt4jhlZGiQ+27Q2XOmxT3X9/ErvzXE04+0e";
    os << "eGkz8iAQzGvZfUEStEJIwxNEAQx8wseod9iz/6QajjMLfdewb5DBzh+up9/+sdPsm3kk3z8w59H5wb8";
    os << "OGCu0WBuepGFVpMjvsG7X3MbRhTwte+fJWjXUUGE0HWcvEO9XsePIiQKy3Zotj1mpmbQZLZjnCpFksb";
    os << "ouoZWkJDqaAmkUYph6nTdMYjfjHCKNsW1ZWrHFzB6TPShPHpbkHQCElKQAhlBEkSQs2jtn2PhyFwWqp";
    os << "skxK0kO7qqGCEShoYH+PRff5Ddew7zlx//DP19/RRKDoZhUSrlOXP2PNNT03QN99GxEoSbaX2aYyAMH";
    os << "bUU8v433MFffuE51q/KM7cU0wljdKEhNEjSzFyQqhRiQc6wcCzQpMTO2yRhTBol6JqOYRjEKiJNFIZu";
    os << "kCQxUreQWrZe6Do2pVIZ3bTRdZnJEZqOUjFxFEAi8VWC0EyEFAQqJlWZ0TYMQqJGnYbnEUQJURwSJoC";
    os << "uIRGYlotyDJQfk8QRWs7BLBcxYp8oSgkxMF2HKAjBj9i4ayvSdLJtE6ERK4UE/MBHSyXlwS6CMGLvt5";
    os << "6m2FvCtqGcM7AMnU6oCKMQlQg6KmPRhm5mfz/ZMM2xHA698BzrBnqoTR4k8H3MVS5Gl0vSikltkfkSO";
    os << "xF+1cM/V0VvdVg77PLKG3uoNiO++sg0xTuuRHQSRK8O1ZS0nSAdUKaOdCUiSxRFtVJEUUeRkCzF6GQJ";
    os << "OUgNYWrgSpJqAPUIWTCJ2zHSkqiWT7wUIFpZj7JWMCFVJGGK6LdQ9ZB0MasPloZAJRFpqKGZEnSJ2ef";
    os << "QPLGQ3UA7MesGuzhfj4isJJMhdMmNAzqaYbJuwyBDPRUsW4fgad7xW+/ijP9e3vOu9yDi/Vx9xQ7q55";
    os << "to7QNcv6Ob1LAYGShknlkjQhp5iAKEYbG45DGzFHL9tjzbtqX8xf+c5MxEk77uPE8dDzl1ZhYhJNdsy";
    os << "FNtxSy0EraPXLnwzKmDz/lBcClDfDH7zeWDlUsBMeLHq80opX6aKf4MYFwxcZvLjNFdBsaVaXQJKBWL";
    os << "+cFX3njTrz6xe2/FtouMDvew/+QYpEvcdEWBkb4Sb7mzTKgcFpsJORcCPwtP0ASEUZY9OH6uRstL0HV";
    os << "otds8vD9hYlFhiJguTbAYeKSGybcf/DDtk2fZd/AUiZbynSfOctu2QcbmfR7afZz77r2LDaUOJxcSnj";
    os << "8yjUlCJ/DJl4uQxNSqdRQ6bs4kTBVnT51DkyaC5Yz+lbgt4ixgs9tF+FngaNQKUArs4RJmycDIS5Rpk";
    os << "Hop7poCaTOLmIrqIcFMm9jrEC6FGIZJKCLidrSsSEoUIbomue32m7nn9pvZc/AIDzz4CKPDQ1i2TT6X";
    os << "J+9YPPf8AYQm2bxzG0fGTqISkDkTzTUhThFI3FSwaWAjk9UFQFJtZVKBrWvEKs5Ct1VmByIVuFLiWjp";
    os << "CKgzLxDB0UKCLbCUySlJ0TSOJwQ/bOK6L0CzSNMWxDCpDw7iWiRApUZJ10hhWgCvr1Os5ogQCoUi05Q";
    os << "GDUFnXdxQRpiFtP8IPIuIwJFIKJTWEUsSRItUElr6s1+oaImlx/XqDiUXBhUUDaWnEKIJQsWnnlRS6S";
    os << "0RRjJCCME6yDSbfRxgWbiGPk3N49qHd5NyQDWsKdBKddqNJFCbki91YOYGGZHJB4kWCQk4HqSMF5EyX";
    os << "E2dOEV44ztBgjqnxc1kkWk7HXFdGeSmpSFFRnC0U+DFxo0M05xEsNsjrsGE0x4m2gT3UjWxGpFqCHC2";
    os << "jtVLCKQ9lZsMtlQiSWivTDDWRpexEIbRStA1l1JQHKkE5BsmFBubNw6ggIb2whNA0klpIHIZImRnHZc";
    os << "UlavhQD9Fsg0TFmYyiGyReiIojhKljDnbRajaJZxogwDENDj/9m+w5eIJfeNs3kZYBkc873/hSbr5ik";
    os << "D///G7e/Oo1tJqK9asGePWbb+eKG38HGfoMVsrMt1L8WLGmO+HnbjSRbjdB4NFbzlPpczHSBLdk0YlT";
    os << "pIK8peFHGvk8tOs+Dz6xwEytwTNH22hmD5vXjTB+YRrDNrlr15XB17//2JMNz1sxaK8MVVaOypcD4qV";
    os << "aos+PhysrFkTUMhj+TFC8DBhXbDorwGgvA+OlNp0SUC7nnHesqpR3DvX30Nvbw9nxc+TEAl3dOd72mj";
    os << "4mlhTT823WDznECjpBQqMeoVKoeQlRJ6DaEbTbMaaecmzc5+iEz85t13LPrbfw0GNf5+DJCcIoRbdc/";
    os << "urDr2f8+Bn+5eEzdOcFaeBQUyXe/fbNPP7kPLfs6ObQWJ3DR6cxZULDa+M4Lrm8Ra3Wou0F2LaJZWmc";
    os << "PHGeOAVdM4EkkwHJBiVKZcVU0jbRlkMlhGsStyJSP8Ys2sg4IQoSUgRCpkiVDS8SlQ1dsol3DMtl8ko";
    os << "pFBH5QpE3v/4uNmwY4NOfe4CJyVlWjwxjuw6u69Jsepw7dw6kpHuwj5m5WTSlEHE2LEpl5jeUKD712V";
    os << "/hgx/5Pm7Bwo+h3YlJUolhSOIozlbslCIVGSOzSMmZFqZlIKQCPdPkXN0kC8QIUUlE02uD0HFMByE1b";
    os << "MciZ9sUuvuxXYM0igmiBGmZbBmJ0GuTfG/vBPnKMMrOIxIDM1VEKiE0DRJNJ4pi4naH0G/jhyGdJAbN";
    os << "RioIVUoiNVAKQya4qoket3jTTRuo2mUeeHqelSDzTidg7a03USgViFptDFMnSbLnL41jDNNCJTHlwR6";
    os << "mxxapnzvE2k397D02i2q0qGxcS0pKlCasKzssLik6kUGxaBInWUCI69j4KI786AnWF4uc3/cUaZz9As";
    os << "o0yXomBUIXy4EJWep29j5KIU1JEWh6JgPIRKBpFsoVKC9FpDGaSLPuHZYlgnTZ7rV8IxMiq4kQKjPaX";
    os << "4y2kkb209MEtBykPhoRqQYkBkiBnsYIQxAGcXYzFiwb9xWaLtEyiwGOqfPeX7yHodEuPvTRf+K2bRv5";
    os << "7/91Bzf9yr9x28jVbL6um0998Rw3rDaZbk2ShDDcI/mLv/99dt36UcLmNN2OZPO6UV5+y6v4wTP7efr";
    os << "gc+xY77J9wGC2pbFujUkYJgwWLXRHJ00UPd0Ghqkh4pjZasyqwRw5XfGNx2Y5fLZFuWuQoYFVTMzNsD";
    os << "C7RAsxc35q/hl+7KO+dMp8KSCugOSlWuJP+BKX8fAiEP6HoHgJMK4MWFaO0SbZMfpyYCxv337NGy6cP";
    os << "vrajcOhiJRGt2vRVXRY32fSO2TTasYsVUNsU0PqkjDOrAdz1YRyXqPVUSzUfI6faTIfQL0dcsXwet77";
    os << "jvvZu/8gX/rOA9xy3SZUEvHUs+dYv26At756G1/79gt0ghisAt9+4Bc5vLfN577+LGt685w83+D8vIc";
    os << "tY9q+TxwkFIo2mm5QXawSpindjsnkQpWlWgtNcxBkeYcrelqma2WbHnLl5SiyPDqpZQMGTWYvrOzNQi";
    os << "b2q+XptEpRQqKWjd2pSpFawrVXb+VVt7+EielpvvrNx3Bsi1KlSDnvEicwPj6NH7QZ7KtQKWs4Aop9R";
    os << "QZ7u3EcSXdvicGeCjlXJ2cIyj3dnLzQYnx8iULJJU1jCiZ0YsFs3cPzO+imgYpDhEopOjkKhrZs5E6I";
    os << "45CaF9DpRDiWQSFnZXqi0jJLT6KhixjHMbByXegyxQ86aLoDuoWh+fS6AQ99az+67pKKmAnRg2UVCP0";
    os << "QpEksJbGhEwUh4VKNtu8RpVldQwJEMhtyFLSIViqRIuAVgyntekwad3jDm2/kwnybasfKXtl+i56uAn";
    os << "qqSAyLNFchTgS6rqFUmMkhaUIn9GnUPTp1n1XDJcJGg+HtNxPLlPNnjhATkzdN2pFAiZS86VK03Ithv";
    os << "XahwMLUeWxihp2QvGtilyxUomFIHxFF6LKAJzs0ZuYJQ8CwkFa2lyxqPoWBPLYCrVeijzUAjVNTTULD";
    os << "Zs3aEl1FjbjpU7cLRM06DgpdtwhaPmZXnq6eIu2lWZo1iNAJZYQ6s4RVzpP2WBTKOulMg/l6E2dTP8H";
    os << "ZNjIIaeMhinkqfkKQaljdkrIQNDw4N79EHBmsu2YTO0dL5BLIrSsycXaC46caLM0EtBYbTFQT3vvO27";
    os << "FzS9z8ms9hqZi6l/B3H3otf/2lIxw+cJhXv2Qrp9oBh58/w+tufwW377qWv/7855man8G1dFb1WGwcs";
    os << "qmUsyWMOFJUCjoiFZg2BGFCHMZUem2Krsn0dJuTswHVWpNGENNqppyvmuraG24+uPvJx08tg9zPAsQV";
    os << "hnipjng5IF48Nq9g3uUhsz/rcWl6zuU70tql19TU+e86xYFbxqbGu4dXWXRiid5RHJnsYC4mOELQDBJ";
    os << "0LcZ1DOJYAyXRNMnZyQ5hEPGjU226ywZPfuN9/MFfPM6TPzjB3IU6icrRVSjztS/9Pzz45SMcODzN5m";
    os << "GHJ/ZOsn51L4dOTlEuupzaM8Y7fvvrXH/NWkobehjt9Zmth6SJwtI1Ij+m1Q4o5gS5QgEnjnBtC0PWy";
    os << "e73KotvX7Ht8OPGuSy7UPz4v5JmacQCQZwKZKJg+S4ulEDILMdOiIz1JEmWt2HbFj9/390M9ffz/cef";
    os << "5eCJs+TyBfq6i6AJFpcaTExMk3NsNq8dxBIxIhYExLQWmyzEEV09DkVdo399Ga/RYrGaMnVhho4fMeC";
    os << "apFGHVqNFFYXrmmwsavh6yPzSEo5tUiha6LpHo+nTXE4h6nghi40OQgjcbpvFZkTHjynmbTSpsVT1yL";
    os << "kG3d05OrVpukoWjm4StH3CKKajDA6Mz9PuBLztLTdx/PgYu7+9n2t2bmTJi9F7NqGrlCCJSVWKUcpjm";
    os << "QaaH0ASE4Y+SRgQK0WX1aHbSKnW22wd3cjohiE+9dmH+c6Dz3HF5mGKKkI3Nay8RdyZo1b3EcQsVtsE";
    os << "nZRS0SSOI5qtCDdnYhkaXj0kUQnNlouuaVz4wRjFos3avImKEqqNgDSIGezNZ6uhcxEyjNClhl23GUl";
    os << "T4igGYeGHHfRYJ1+0mZlqIfSIuYWzBG1JHKZUegpYesjSTJvjp+YYqBRJT0Rs2NALp2O++cNj3HnrFQ";
    os << "TNlB8+f46H/IAbtvexZrjMmfPHCBV4PqR+zEK1Tcmx2Xb1MIZQPL7nHHGouHpjGU03iKYbnDwzRz6ns";
    os << "21jL4/sm6SvNEOETjGv89ShWQxd44Z1RcZmfcycw+/+/EY8L+CxJ+ao1jxyTy9iiYgDR88TxootI6tJ";
    os << "NZ+pWpO7rt/AsbPT3PFzz/CNj/88V20f5oV9Z7n92lG++p0TxM1Z8jmb97z9Bq69axubt/0pltbF7ud";
    os << "PsNBs8P5fu5f/cu9W3vaev+PR/TVuWF+gmDcolVzG5uLMGmWAt8yuJ+sBKQFxnBCFKX5qkqYa9XaHvu";
    os << "4B79D+veP8mCVeatReAcEVZviz2OGLAiL8J5gi/NQxeoUxruiLlw9eSmsHeq5LPO99IwN2znVA03V0K";
    os << "TEtG6/jk8QpuZyBa2QWhNhLGJ9vcPh8jUao+I13381H/+xKGrM65bWv4r7/8ksc3h0z2LORYnGKj/3h";
    os << "zWy8fg0f+8uneeQHR0kTqDXA1ZrsO5sFBNx81TCbRofYcdNOls6P8+DTMygRQxzhdUJSKdBIyDt5TF0";
    os << "SJyHVhRZnp6dh2dMnhEIJkYXT/p8YtcxinLXlJb+EBKkESIlSiiTJng9d13jJjVfz0l1XUW20+cLXvo";
    os << "+ua3RXihha5mucnp6hUOyitrSI6xjMLjQQAhzTphMGdBUdhvvy2LaBpmzafguRKsIwptH0MC2T7nKBd";
    os << "icmjmNc18DUBeWCjRIwt9Ak71oUcw6dIPNstjohSZLQbPsU8ha2Y2LoWY9JzjUp5izSROEHIYaVWWqC";
    os << "INMxi0UbU4dUJRiGgWtrrF3ThWkY/Pu39uPmTV71iu08/MQZwp5tYLiQJCSkKKERdwK8jkc7DAh8Hz9";
    os << "K0HTYWmhx165Bnth9Gl1JXvayLQRhzMnTCwRBTJwkSKnhB4p6w0MlCtvR0XVBGEZZ4rmEejPE8wPUct";
    os << "FSEEW0/BDHMHBcE9PQMXUwdYNGy8PzQ4b6SoBioeoRp9D2IkzLxDQkjXqLjh/QlbcplXJ4YUpvdwHDg";
    os << "iiKmJ5pc2q8Sj5nUW22MTSNa7YNUswZmK5k374F5mp1RleVqVUTXnvHCJWyzWLVpxUlHDzeoGwLrt7a";
    os << "xZkLbRrtiIH+PF05jaPjLQ4cO8+dN65i1UCZrz1+gU3DDru2DHD0XA3DhsnpDnnXYnKyychQAccxGKw";
    os << "U0bSEk1NzCGx2rO/j0WenKFUM7tg5wPxCxFBPgUazzdxim74elzMTdSoFk6u2DxMojdbCIk/tG2PPoS";
    os << "kAbtjSR09PP0fPTfHSXaMkrsPn/v4XOPn4bv77p09xaryLY2cO8bK7S3zxCw9QPffP5Hrr/MkfneRvP";
    os << "vMIfXmDTUMl1g+V0E1BrR0QJDGeF2GbNpaRDcpAx49DklBwdqrpzfs8HoRhlZ/0Il4+WFmx4Fw6aQ75";
    os << "ydRt9WKg+J9liiuPlZTKld1oeBHGeG5m4fmya/9/qSy9zHG1aw2ZWHnblkpEmqPl00Y7iMJAGVPTC7I";
    os << "dSaarPkvtgPfcfzN7947RXqoRJ73MTZ7k4HOfYXo85efe2sPRZ1qMn4sY3nU133vqPEoZ3HzNEOemqs";
    os << "wstpidbVDQQHM1zk03idQsHfsoO1YNkCbj+EFCoegS+x6WppOkGh2vTWJYWK7NwLDFdLOG12yTqQQr+";
    os << "pDK/FtptmGw/I/kx/eK7HsCQSKzkiMpNKQmiaOQVEUIYMPGUd59/30oAY/+YDc/3HOIwYE+HEen2W4j";
    os << "LAs/SCiWSnSVLYg18q7G/GIGlpauCGJBf6/LhtEitm3SrHtMnPKxhI7tGJRKOa6+cpiCa6NpcGG6wdh";
    os << "EFdu2EJqgVuuwad0A3WUbw9A5dnKWkxcW6K0USKTgio19bN3QB0IwPlXn6IlZqvUOtqVj6pL+/jxdxa";
    os << "xBsF7v8MKxWfKuyWBfjqm5FjlH46ado1yYrHP63AJnz1Z5+/030NvlEHke3doZFsP16JqVSQ2pAlNiY";
    os << "GOThV1KGTFYignmG/RXNrBlfR8PPXIMNI1VI2VKJZNn9s3QaAX0dReYmGkQJwnbt/RT6coRRQnVRsrk";
    os << "bI00VdRbIQLBlVv6GBkukSaKYyfnmJhvEaWK8xfmuWrTAGtXdxMlJeoNn/HzS3R3OeQLDo1GwNb1vax";
    os << "aVUJIyVK1zf7DkyAlc3WfxZrHujUua1Z1E3QiHEtyeqKKbWoYuobU4Kr1JRbbEV0Vm2uugR/u8VjV7T";
    os << "I2cYGvfy/gjXdvQpqCoW6DuQXJibMdpF6j1vDo7c7R3a8TtSPuu3OQo6enaPgppXyOoBPixya6LSjkT";
    os << "EZHLMYma4SBwYb1BRwLvDBmoMdlsdZkuL9MvRZQLEYUc4JGM6IVd1AiZnrJY2Kmzvy8h0rLHDk7zcYN";
    os << "g/RXa0zUApbGm0zN+XRZAi9SzFdjlrwaG1eXGRlwiO0uHn96lo1X3cATzz7Hhk0Bv/vBbfz7v51j9wN";
    os << "/QakQs2Pk5Zw69wRveuWVLHUivvfUCcYWOxRsg4qr09NXShwjUsWCoeVtiCNH1Vqp0o1CON6ojc20o1";
    os << "Nxohr8NEN8seSbF9tt/g8BEf6TTBF+gi2uHJ0v349e0RcvMkag1NNVWG3JtNc2TDdMknWmbrSXms2Ga";
    os << "7ulaie9yWs3TMu26M9ZfPlf3sk1u3axbtP7yOkGu5/+XY48dYLXvuuL/MWHruPtv/omXveWf6G6OM/c";
    os << "AnzqAzuZPN/kY184xr13buTTn9tNzgwZGSkjcJlvwo27uli1ahsnTx3jqUPzrOqr0KrV8aMQ03ZAJRC";
    os << "lWG6OSqVAx/N59vlDpAoMmbEjJdKLTFAuMz8ATZPLZe7LCcRCLYfM6oAijDpASqW7m1++//UMDfUxNb";
    os << "PEF/7pmyy1W6zfMIJUCY1mA9OwKJfy5OyUiek5PC/EtjSUW8mNAAAgAElEQVQc12Dr2m7mFv3sjb++l";
    os << "1MXFlEyRaVglXt48x1refCRQ8wvRszXOvR257AkGLrk7FSN2YU2m9f0YOo6C9U2lYqLaUryjsn4RI1z";
    os << "U3WG+8vMLDaplB02j5TRNMmF6TpnxxcpFh36+oqkqUKXgu6CQ6Hg4PsRSapwXYNCzuTMZBXXNukpu+R";
    os << "yNrouePa5MV5+xyaaTY+Z2QZXbhniu4c9utduIRaSOMxqSxOvQ+g1CeOAJG7wqm1FzhydYmR1F0IK9u";
    os << "6f4Obr11Jr+HQ6IfW2T7sVsmZVF612gO9na3SObdNqetS9DkGoMDSNmbkatabPulVdDPYXidOUk+NLe";
    os << "F5MuZDn/NQSI0MFRgYL+EGmEM0veHR3OQgU5yaq9FYcRvuLCCGpeQET0w26y3k2jOa4/dZNfPm7Zylo";
    os << "baQusU2D7Zt7OXSqypq+AtvWFfj2ngsEQUCjpdgw2sU7Xr+Lj3zi+2xa28XmdQM8d2COWIUsNiJW9+W";
    os << "5YWuFIxMdeksGrU7EzHyNOEip+wk3buplbN5joRGya0OBJw/MsdRo0V/JMzYfMdznsLpoIkwd2zQIk5";
    os << "ROIJmrdZCkIKDmBdy9c5AkjvnRC3O0wwDNMmn7IRuHi8w2Fd1OQhhpjK7O0WzHvHDAA9Wm4nT44cEF1";
    os << "m9cxXVXrmZiYoEN6wrced0a3vaRPYystdi8rsLn/+F1/MGHvsk/fup5xp75bXL967jpxj9hfL7KxJn/";
    os << "yQ+++X3e94EHmVlskyQpg6u2tNu1C8/rJMI1RWF4eFjOLi5V2+1O4ke0m16nsQxwK1FgKxacSz2J/5F";
    os << "J+/8IiP9XoHgZML6YsXvlGL0SHlFaBsfCJV9zli8TMNds3HrF3PT09QXZYc9D7+eh755h01W9bNrew+";
    os << "otH2Hb6grff/TXeN/7v863Hz3BZz91H9t3ruG3fuFr/NpHd/CSa9fxe7/6JF7QxnAcTp+awe+0EELDs";
    os << "CxyOZs1oy7Frn68esiDT4/RVzBwTYf5ZoMoSTANkyRJsurLQp6+rjKT89McPXYO0NCkhSJZ7nfJ2KFY";
    os << "9jFqQiNN02wtS88SldMEosQHEmzL4r3vfBNXXbGJyflFvvHvj3Pw6DEGe/oYWNXP+fFxFqtVrty6mTS";
    os << "oYVqKXD6PayZIFdP2BQN9DrVaRFeXy9WbKmgqoVzSOTvToavkcsX2tew5ME68WMe0DY6PN5g4X8W1HK";
    os << "I4ZqC/wNYN/XR1uQilOHc+Y9VSSLy2j2tnO75BmOA4OvPVFqamMdCTJ1XZ32kYOpahkSbZ0VOT2fjdt";
    os << "g0qJRspNcpdLmEYMTXdoN4IcB2D/v4Ci9U6Lxyc5vjJee5/4046QcrYYp242E8z6kfToJLXmVpoU296";
    os << "5Owabm2eXVuH0JTioUeP09+f4+odQ1S6i0xPN6jWPUoll9VDZYpFC9/PmODUfINqzUeQDW0s08x2uzV";
    os << "BJ4hIl6tbazUPP07I5yyazRABdHwfL4ywbStLB3J0Roa7CaKUqdkGfifEa4cIAc1OiKZJRocLbF9fpm";
    os << "XYvGTnao4dGCNVCTu2dKFSQYTGgRNVvJbPpjUFZqo+fQWD3t4i7cShXZ9jaSlhsZUSex6NKKGnnKfZD";
    os << "MnbiqFVFc5dqGG7GlcM5Dhyqs7MfAthQsExcByDWi3rFReaolKA2RosNkOkEoz0ORw8McXGNSX8CBa9";
    os << "DioEYRsMlFxEEqJI6HgaQsQM9fWTxlnsXb7bIi8F6DHK0ai3Yk6faTA2vohMEuqNFtfu2sTUfMj1m0o";
    os << "8+vQk+/b/Op/5zBM8/pUTfOzLP8cnP3ecD3/gG9x753Y+/Q9v5543/A8OHZ/i5JO/x/5DkyR+xBVDvb";
    os << "z0Fz/BQjtNV42u3n/+3OlZfrqo/mILAC+ej3jpkOVSQPyZBu2fBYj/16D4IsB4af7ipcC4ojGuXPnlr";
    os << "62Aog1YlmXZV+/cde/eZ3dX/vr37+KNr9nBB/78aX7n96/l+KHz3Pf/fp1rNw+REjI976BUyBtfMcDN";
    os << "167i5l2r8WPFp/71OHHis+fANK2GR9GRtP0QO+cyOFCk3QqQRoEdm9fw5e8cIA19Vo/248cxnSAi8aP";
    os << "syBsrUgFdpQJdpRzT0zOcOHM+Y0eGnRXDJxmLyKw0WVagrmXGc5UqPM9DEeM4Jq+/6zZeeedLCLw2X/";
    os << "/mk+zZexhpaQz196IbOufHJhgZWUWaJNSWZinmdCamPbwgpJCzEEpQ6XZZWAro6clx+zX9fPXRs5iOw";
    os << "UC3RXUpJWcYLHpNhioOXcUcx8eq+EGHrRsH2Lihgil0pExxbA0lodHKWJbj2liGQbvt43khpWLGiGvt";
    os << "TL9BSHqKOgpJpCQyCfGDmHA5CENokLckM7MerSjGUIqhgQIIQdPzKebNLBknTdm+dYBS0aJaC+mtFPj";
    os << "hnnO8dNd6njwxxnTgcOMV67huNM9TJyd46uQEW8oGV3RVOD4xx7VXDeK1AvI5k/mlNqfOLiKkolC0kV";
    os << "LQ6cSEYYLnh4CiWHQgyXRgP0jw/RDL0HHcTNC3LZOOH1Crd4jjlCCMKBczUJ+aaWev6jhhYtFjYqqKa";
    os << "Qq2b+ilq2xy7HiT7p4CrU4HlSg6vk+YSDavKTG/FFMuG9iGpOlHJHFKtREhBbzm9lG+9cMp5uYabB0t";
    os << "cWoy0xrPTS+wbqiX19y6ir0vLLLo+eRME1/FzC91SJWG32kz1JNjdkmQph1ecd0aQhLOXKgzN9/AMXT";
    os << "cnEN3t0tO09l7Yopil8uOdRX2HW+yZkAwV+1gOXmGuyxOTM7RbinKeYveLkkYKyzdZH6hyVLHZ1Wlm6";
    os << "qf0mq0QCWEScJAJUfYSdi2uYLm2Jw9NYOpAoqFHLVmwunZBn/8npfy2YdOcPLZP6K92OT48wd5358+x";
    os << "KFTESO9Jfr7FNMzLcan27zwyDuwDZf/8elDfPDXruFfHzjIBz/+BOtXr589eOzY80qpS0ulLr8uBcQV";
    os << "G84KKK5clwPipX7EFx2sXP74id3n/8zjkuCIZbMK8OPgiBfbnb605uAn6g6SJIltQ19YXKpu/t7TZ7W";
    os << "7b/3f1L13lGRXfe/72XufWKlz7p6enBRHGkWUkEBCgARCBIMIsshgwGBjTDLBJpkMIgiwAcsIEEEoC8";
    os << "VRQBIa5TCaPD0zPT2duyuftM9+f5xuaSTj+65t7lvv1lq1unpWd01XnVPf8wvfMExfl89nvvIkn/rwB";
    os << "lo8yW9u2cvkTJNEB5QKBbbtjXhk6xjbt02za3uZ/VNluntybN81RdjUdLa1EScxWivaihbVSpNarc5A";
    os << "TztLh0tsfvwgyqS0tOQxRiOkxCWLN5CWRGZqZlpaS/S0tSEjTbVZz4b6wsqstITAkgqdauIkJgybxHG";
    os << "I7ztc/JZX8rrzX0p/eyvX3vYgP/7Z9UxMlunoaKGrs5V6vcH4+CQvOO5YGuUx9o9OUszZHJyq0dbmct";
    os << "5Zy+ltLXDKxkHSRGMpyXlnDHLfE2M4Npx94hD9nQXecO4gdz05RqGoePeF6/nd3bu44IWDvPllh9HR5";
    os << "nPYEpdLr3iIPWNznH18O7++cTeHrWjh2PWdFHM201Nz/OrGrcyUa3Q4ijsfneCyz55OX1uejevbKVdC";
    os << "LvvFQ6zr89lzcI7Htk7xhXdvJDGKl5zYz8NPHeD+x8dY2pfn/sdGGZ0o87pzV7F8qJu1y4vcev9+dJI";
    os << "yMV5mdj7ghaesRMcpxZYcy5d38vgjk7S6hiXtLo5tI9MIak3anCKnnb6cOM6UTSuXt3Pv5hHuuGcP20";
    os << "emkUoSRlApNynmbMYm6+zcM0McRdz78Bhd7T4rBlrxXZv+niJ7xipMT1cxYcBvbtpKwbY47shO1q9q4";
    os << "/RjuvnqTx7DSmMe3zbOzv2znHNiF+e8YBlvOHc1y5eUuOvRMV5+Sj+P7Jhl/8Q8pxzVS29ngTVLW/Fs";
    os << "wZ4D8xy9rpNHt07QDELOPH6AfM5hSY/H2FSFRjXmgjOXcGC8yobD2vjAxSeyYiDPBy86ii0jU2x6cJz";
    os << "j1/UwOlnnnJO7OOO4QU4+upu3vWaYq27ZS0fRZWlfkc7WHPO1Okes6OTkjQN85JIVXHHTHrpbbGr1hL";
    os << "G5Jv/2hY2sXd7BWSd1cNcfx9k3HpD3Nd0tDgfmQn79z6ewZriTV581xIPbpkALTlnXzqbHDjDU1cK3P";
    os << "nQMQ705XrihjQd3zHDc6lba2vKgPNKwyTFr27n/8VlKnmLlyhXMzhtmKnO85iWruPvePZS3jPKWj17J";
    os << "z28eodHopODAgdlpRg7ME0SGy79/PsdvWMK7PryJc07pZWK6yjs/dT2W5eqlS5c+uXf/virPhkv9KQB";
    os << "cvP8pIDx027xo9PC/pN78Z7f/cqX4zC8+l7+4OF9c3EgvzhgPvft/4u5KKb0Xn37qGx596PGlzTTkfa";
    os << "9bT2upm2WrSiwZtLn3/llG99e496GdPDkySyHfjUwT5svTrFjVyUvPWk3BhWtvGaU9306kG+RyeZYt7";
    os << "+GeP2yhUpvniMP6CULD4NAg9Rhuv/0JLCuh1FYkFWAbgZSZKYEhRcQxjrSwfI80TRFRyPjMLPumqlm4";
    os << "t8neYykltmVx2JrVnP+qsylPz+OLlN/8/m4OTpQpFBxcRxGnKSLWxGlCe0c7lflZSDVhnGV/OEpRqQW";
    os << "0t/qsGG5BJ9Dfk+Php2ZxpcBxBdv3z9LdXuCIlV0EcYI2mgefnMDxLAZaffYcqNLXU6RUcAkDTZIm7B";
    os << "+vkvdsSkWHqbkmK4c7KfguYRQzMlZmfKqM71gIKUiNZOlAN9J20WHM2NQc9WadlqJPFGuUzPKig6jJU";
    os << "G8PU/NVeloLjM/MoUXCmy84l90j+7n34e0IZdFZ9Jidb/DWt76A4zYM8M1L76ZSqeC5DgbJisEiroDx";
    os << "SoDjF6lXygT1gNaOFi44dy39XT7zswHb9s1y6x920t1eYMv2OVpyNhPlOs1mxMkbVzM0OMC//uJGCl6";
    os << "eQsFfMOWwqdbnENIGEuJI4zsW87UI3/fp6SyhLBetQ0ZGJ1DCkCYaIwQdbXmWDrahLJtURzy9a4qe9n";
    os << "x2YU40a5a14zoWjmsxPVNjdKLKcH8rYzN1Go2Qk4/uxaQWji3Ztm+OyZka65Z1UGumGKk45agODhyso";
    os << "6Vkx546zWqDnsFWms2IMEppbfERKUQYpiYrVBsNjl49wOkbV/DNn91NS8mjpZBD2TbT03OkCF503Apm";
    os << "KjWqQYIlXLbvneIVZ67iva/byK9ue4JLr3gUYcHJ67o4OBczX6sxN1shb8PhR27g1rseROuE4b4uulp";
    os << "Tqg2LoqfYMzbL2153NoUWyT998zo62vq48l/ewxe/eh1TB8c4Yv1Kfvn7p7jz8uO59NdlGvEUP7vycX";
    os << "JOL7aVEqVzXHzeUVzwwvXkuhReQbFjZ8BDm/excijhg199CkcZ0zvYv3f79h1PGqMPDZU6FBCf//2h9";
    os << "8Vly+LjRcnfoSl9/4Gg/X8EFOEZYDx0+3xoK70Ijovt8qGPnwOOvue1v/bssy+ZmWt2PbXrEc46qZ8N";
    os << "65bQ0pLDCIGtBAP9BepBzJ49M1z9+8fZeTDiojeeRatdYWpshnseCHEdSRhGaOPwjredQJLY3HL9nRx";
    os << "5eDe/uXUXys6xbGk3PT3d/PsvrqMZadpailiWIm7WsQ2IBfK1ECBtaAQxQjn4rk2QpqRG0NlSwslZnH";
    os << "XScQwNL0GbhPv++DC33Xof1TCkva2E4zg0anVyOY/Wzg5co6g1alSrVSwps5Q/k2IpgetYKCmIEkPYj";
    os << "EnTFNuRFHybMIZ8zqUl74G0INXYSnBwuoowMDzYRjPWeEpRqQfUghAlFPUoZllPG47jgDGUGwGOm1l3";
    os << "RVGM1pmmuOi7KNeit+AxWYuZKTexLMHyDptUCcZmA1odiWMpHhqZZu/YJGcetZq+3gH2T8/w1J4DzFX";
    os << "KfPpj7+KkjUfwjrd+noOVaU5c00tqKW7fvJ1//OhZnHhYCxe882a+982/JJzcwld+tIW56VnOf8nRrD";
    os << "vxFDZvuoO779qCtGw++J6TKba3cced2znzlH6uvnOEW2/exenHryap19i8a5YWJ8+3v/Muntgxxmc+/";
    os << "UN62lvp7+lm/WAPIxMH2bxlN4PdnWwYLhFpzd7xiIFeH8+xODgfEkbmmeiGWj0kiGJ83yJNFQXPRzoy";
    os << "6zgiTc73MMbQjAIm5hq4SuLZisikDLbkmAliuos+j+7MuIBDPS3oNHNkEmnCxFyFJE5wLEWtHiJVZg/";
    os << "WVspnM10pMss2YeG5Lq7nouOYJInJ532MFJAYZst1PJUQaUE9TBhszyMdmyCOMKS4Tp6gmZAayOVclA";
    os << "VrVhfYuSegXJlh5WALI2MxLzx1DS2Wz3X37eYTH3kNf/vZf+XBP27n5CPP5uPvW8Z8tcIVVz7Kpke2U";
    os << "mxr49rvn83PfvY0L3/XG5msTvLmC7/DmuEBhrtsnhyp8abXOnS1D7KrOsyTjz2MPfMk3/zSG8kvHWR6";
    os << "3yg333yAvGcx0F5k265RHt65jyuu3cdZJ5xKT4tJrtp0z12z1driEuXQyvD5gHcoGAaH3Be/P7RCfE6";
    os << "V+L8LiPDnAUV4LiXnUNXLYuV4aAW5CIqL1WM+l8sN9LW2fWTt8mFrZGyCkYPTHL60wGtetpZ1aweoNz";
    os << "MHmXxRUCgYjj5pA7VoJd+79BpobGdkf0QUdeA4LnPzFQ4e3M7EfJnP/MMlrFndzXs+8C+UyxFL+3JMz";
    os << "FXYuPEY1qxawdPbdzF2cIpaPcCN6uwYm6S9rQ2ZGCqNGtVmyJLeLtqLDh3Dq2gp5jFpiGtLwlCzbe8Y";
    os << "T2/dTaNRp1gskcu5dHW2oXVKa3snWgcc2L0PFiSCSgikECglUXLhsRRYVmbn5dgWrpt9UDNjwUzOZSk";
    os << "LjMniFGKNbUl830GnhjjSCJXNOS0hQRhSA45tESbJAptIoE2mq7UtCwlEcUKcJFmpr7KEtyjKqirLFi";
    os << "grGxEkaUoUR0SRJu85rO72eXy0glIurgtF16bVFjywYxwtFMcctoYkmmHPgXlSLSh6FndvneIbnziNp";
    os << "0YanPmi45keG+VDn72Fvi7B2/7idLqWH8G+px/mJ7+5j6nJmH/4wNk8MaNJ58fJpzE3bRrhmFVdTDVC";
    os << "bB3Slffx27rZtn031VqTEw/vRyMIYsl8ZAiCGu05h3KgUSoLfZdCLUgrF2JnjVgw1xVIDEoIIp3iey7";
    os << "KsUl1StAIEUpg2QqtDUplJ3icGITKfi+MsiiKNEnJ+5lOuhklWQKkNkghECbNHHsUmbZcZgYNYaiz4y";
    os << "IBIbEsG9exMrlgmtmLWbZFkmiSIMGYFGmBkIokTEBkenWdaBKdLFDBJJajiMKUZhhTa8TUozhbPhmb3";
    os << "m6PjYcNsX234mXnHsNXv/Njhrtb2HjEap7aG6NVAR0/wNNPz3HRizdw82PzzDUr3HXHFXz7Oz/kO9/+";
    os << "BWuXtrNs5em0FXPkLMmNd9/Kb35yPD+7Ks9nvnIpIt3KLZe/l+nmUnRVsn5VF8pNefDhPfz4t0/yyLZ";
    os << "putp7ecmxw1RmRwgSa+amR0fuShK92P4uKlQW2+JFgFwExuen8R2ayvf8gPv/7bb5GVz7n4Ai/Ic2+l";
    os << "DzCEUGhjbPrSAPrRTznue1r1k69LXd23eUOtpcWoqttLf1Ml1J2Dcxxl9csI5XnLWOkR3jlNpzrDmsg";
    os << "+VHn8EXv3o9WzY/RPfAIM1Zl/ZSK7PVWWbn9tDiJ1gCJmdqrDn5WOYnQqxmhUTXaQQBew82sP0C/f09";
    os << "5EpFWr08/R2tPLDlaZKowcDwUqJYMzcxxZFHrmPbju3MVwNWL1/O5Ow01WpIvR4QxxHlSpPOzgJxGFK";
    os << "v1fBch0RrapXZDJCMQSq5AIYCy5LYUmI7Ctex8FwL11LYSmVu3cIghcl8CEU2upXCQiiDkoY4SrMPqC";
    os << "UWYkKzuahlZ8CmFxYcCJBIUtKsNWTBAVwu8NCTbGtuDAvgsPC84lkepmVnj1MktiVIF7KvlYAwijACL";
    os << "FuR83ykgHK9iVQCx7JJUkMYJviOZHSyjldw6WhxuWvzKJ3tRfJFl1hrbF/Q21diYrRKtRnTksuRNiKm";
    os << "56ocf2Qv9ZrGVimOo0hTsgD2JCbVKQZBS4uHNhIhQWeaUQSCSGcGtdK2smwTQMcarUEogSOz606kEyR";
    os << "kFbo2uAUXKRcH5vIZay3I8sKN1khHIci6EplmqiXlSHSUEMV6gZuYRTqgMwMRP+8RBTEoicDg2FZ2QR";
    os << "OZJ6fWGiUgFZkQFA2Wl8UoJI0IYWX/t+U4uE5mdyaEJIlTLLLXVKsF2E5WeYZRjKUkrqVwHBuTGrRI0";
    os << "UlKPYgRUmJLn/lylUJBYSlDGCsm5jRJVKaz4FIsuNiyyJzVRWuHz6abbqW9pLAkSKeF4eGN9Ha1MTl/";
    os << "EIoNxvdP0Nu1jO9f9hG23v8ztjwxhYnAuHDtbU9y46ZRenqWsKKnhQ2rLRwC9o2OMTXdoGftqXt/dvU";
    os << "Nd2qdPt8DcVG3fCj38FBAXATBRWL2cwxj+S+0zc9g2p8JFOG5wPin4lEPnTkuVor508940Yd3PHjni3";
    os << "o7XAJtiJKYnOezdMlqyg3J6NQ89WaVFivkolcczVv+6lQ+/Y+/58e/uI+WlhIvPvkUlvcvZXR8H7aYw";
    os << "ncjbMejq7uNgmu45s4RhnvbGeh2makERKGmWqnSCCPqQUIYaYyRtHR2kc+3EEYRURxkOSY6QUiLnsEl";
    os << "PHTv3RQLReK4SZokpDrNogykJA4jUp1khq0GpJ3JFrOM2ywL2rIUlpJZLIKSeJ6DZSs8O8tGAZOZCug";
    os << "UISVe3kEaiJOsMtTJQgDSgjW9IvMt1GGY2UnZClJDFCcoQ0YZUookTkgijRDg5pzs33SWTZxisniGJN";
    os << "uY2nZmLKBDjbQltmuhU4jCBJ3qhSgFk+Vmq8wlO+fZC24sKrN8i2Iinc3otMniCVwJ9UBjEFiuIg4TS";
    os << "jmfwAjcgk1rq8V8JSEqp/S0ZW7ZRhrCSgQixQhDEBocZZDGkKQCy17QnhqB7Sy4ZCeaJNSkKZl7uU5B";
    os << "SZQRpKRIawEc02yDrgxIBWGQAWJW/VnYORuTpmSBhgKZGliwXVs8VjpJ0XHmxygsQZqk6EQTVBtYrkO";
    os << "SZNp521HEcQxSYiuFpRTSt9GRxvbsBV/OODPrVdlFBw1OzsFxHOIwA9IkjgmrDSzXJg5jDAK/4GcWd6";
    os << "HGdiTKzgLHoiTGsRQSRZxmvp9REKGshQgNKVB2NjawhcH3cwgpaDQDjEnQwkJqizBs0NbiMzqp2Tk6y";
    os << "XFrSzRji9Hx2Syr3M7T33sUK5Yu5fbND/HgY5tpBg0+9I4z+MAHXsxPL72Fq27bwu6xJlJ4HL5qPaef";
    os << "2E6rLKPTiNgo5msRo3sPIIWfXvPH3b+sBfEUGSgemri3qEx5foTAIiA+vzJ8BhDhv1Ylwp8BFOFPAuO";
    os << "f0kb/B07jiccdt9JR6t+ndm22Oro6iOM0W1WbBNcxtLf04eW6GJ83HBifYLY8SV+3YM/+Br7r8oJjNt";
    os << "Ba6sexm5Rys0gRk6LI+T6FQo6urlZyHoxNlImMgjSl2YxohgnVakCl1iSKY8IwWlA8CxKdxQ1ISyFll";
    os << "g8tFhwhFg1PF9TQKJURunWskQosKRaPwjNtsuPYCAkKg+vZ2FLhOgrbtbCFwpICx1VIkxG/LZmdtEiJ";
    os << "UmKh9tcIbYiFwbUsdJJi2Qrl2sSNmCRJEJbEFpmztjYGE2qknYFB3NSkOkXZWYWaxNmH2rEUWhpMlCA";
    os << "dG9fLqrF6uYEONZankLYkDhOiQKMUOI5CWQ6Bzs471/XI+zZSyawKE4agGVOrhlSaAVprhJJYCw2Fl3";
    os << "NQ0sayFYV8niDWzJTrtJVytBddwjjBVoqco0hM5nQzP1dbWPZALm/j+FnSXaPWxLIUti2JI4NjK6IkQ";
    os << "cnMrSZesKUTCwYcBrDcLIpB6xRhUsJmTDOMKZa87O+0bGzLJtIxIgFtUpSlskwTmckedZQQhRFKKaSt";
    os << "kBiSxBA1ItyCR7G7SGWykunnTUpYrRM1Iyzfp5D3SVIN0iAthYnTLHzKsxAqO2eCRkyaaGzHwgiDkIL";
    os << "6XB3HtvFa/MyQWKdYrgPCENVD0IZcW4EoSWhUm5njjFREjZAozsyYpW1TavHRsSaMUqRtkcYa28qK2m";
    os << "YQE0Rx9h4oQVtHK3GqCapV/LxHtRpRnq+SpFAu15maK5NoQ1//RlpzPttGdvLAlq3EYcKyoTyjk5qCm";
    os << "2dJ31JOO2EZvT01dL2MbVk0w5RmEFGr1qkHEZW6qfz2rm2/awTRLM/K9eYWvv5nkr3/FRj+twAR/kyg";
    os << "CM8BRjgksoDnAuMzoGjbdultF73q+3+44eoTlgy2ESEIwgTIZj2WBIPG9xR9vWsoFNv4w8P7eXjr43i";
    os << "2ZOPh61k2sI6BAUFXSxnLzmRHJjF0dLXS01nCcTIVY6PRJE6zuZtOEsJUMD9fp94IM5lUHGdcIZ2Sao";
    os << "PW2Qlj2XaWryKzEz8KI5QQIDPn6lRrpMyqEmkkBo1lyaxtjWMsW+J5HiZNURJ830EKie/bCGNwbHth+";
    os << "WGw7IWwqwUQSZMUyxIIJNqkCxZmmfUE2qBsmfXCQNgIs024l+X6piLTYMdRgjApWqfEYZT9HZaFsxAi";
    os << "pTyLqBlhUk2uNZ+5WMcJUSMgiTO/xwX5NkhBmmSv1fNc0gXwtoxE2RaWbZEuuGJHYUKzGVIuN2mGIdL";
    os << "OqtMo1CAUhWIBP+9S8Fw8L9PE28rCshVxGJBEEbZSC6GChrmpCoYUx7NRlkKHMWEQYlKBnbOyUYMRWK";
    os << "5NFMUYTHZ80kyJJCyJ1JBojVDZqEIokQHifB3lWvhFPwvWcrM5o1BZ9RfVIxzfRToKrRNMsnBSy6y9j";
    os << "hKNTEG5kiTU+KU82hjCIEAgsByFlIrazBzKUli2TdgIcUseOkhRlsT2XeIg85RUTtZ9RI0A1OIxztya";
    os << "/IKHtBwEAtvKkgKr81XCRgBGoByFsiVhoLF9G+nYNOZrNBshTtHGs9zMnTxOaNYbJHGKX8ojk5goyLi";
    os << "otdZHQPMAACAASURBVEbWeZRKOdrbiqRJSjMOqTcjZqdq6CTKsoikoFJrsP/ANEhDX/dhJCLPzj07uf";
    os << "vhR0i04Yg1R3HCEWs45iiX9lLEzEyNsB5hjKARRNRqDRqNgFqtzuxs2UyZvjs33ffQI2SgOAfM8lwt8";
    os << "/MrxefL9p6pDuG/B4jwX9c+/6c3Y4w5BBgXNdILR/SZm2ThRaxf0t8my6Mb1q5opaW9jUoloCYiIpMB";
    os << "k0HgeTksSzI7u5tmkONlpx3NqqVDXH/P7SwZXMvalTlW9NeQTisJLn4+JNWGtvYWevq6aO3sQQqLJNQ";
    os << "gE1Jjk6YplfIcY6OjzM+ViRNDGMfEiSZF4ftFXL9ASmZ9L6VEWs4CP1FiKahVZmlU5kmTCIFGKoWSCm";
    os << "TmoiNM1j5KmRG8RZqBhesoEBmNx7YljuNguxboBKUsWDBfTdMsMCo1mVuPLSDVBlKdBRAhIF2ce6V4r";
    os << "VkeihApqckI5pLMJNak4PkKCjnShZmUZVmkKQjLYEuFTlNMkv08SZaLbOU8lMqs0Ywx2aJCSizXyion";
    os << "IVBSIWWWYyKEQCca17cRIos4LbVGhM0IoQRJYggTQ6I1pVKeVWuWU2ztxFI+StkYS2B0gusokihgfmI";
    os << "fsY6pzZVJShHCKKSjMn/KNHMgwoDjuVmVbmdRBo7rYDDoKMmiQrXOhoeWwVNZlKtQYNs2jhPjOTaWnV";
    os << "WfQlmkOsnI+VKCleJ7uSxWNNVETTAqu4DZjpONRxpNhJF4JQ+jDWG1QWoM/sLGmjRFSYHrO1momVLkS";
    os << "iobVdiLc16Bk1do18UAju9g8jmSNCUJYpQS2J6LbStSZGYuLCWO52JZDikpJhUkzZAUTb6QRaGa1OC2";
    os << "C0o5D6eYyxyd4hS/5NFSyi6Ctu/iOBZhrUmz1qRv0COMsjlrznVx2j2iMMGamMPusWkGAXGUkJLSogT";
    os << "FQo44biDlBLncWnS6ngeeepLTjzqBU49bQ0frDAUnpjafIrQkl88RRSFWLHDdLMAsjmJc1xIrc5y+s7";
    os << "tt/+jkXIVniyp4tvI7lOe8OD88FBCfwaL/CZb92UARDnGufS44ykO+ArB6WXfL0cs6v1Kd3u0sGezGd";
    os << "l0cJfE9m0o9ohnHpEn2ZlnKw1Y5jE6YnHma9cvXs2XvUXS0wsYjYHYOhPTxXRvPcbB8D9fxmZ2PmGlM";
    os << "MTS4gmJrK6SGajMGk5ISUSi0UPB9GkFInCaYVNKMU8YmY8JU0t/fjTAOyrGoNqoU8y6OstmyaysijFk";
    os << "61EfQqBE1G0ghcPN+Np+LY0SaIqVAJwmkKZaVRZsKIVAiixq13WxTKUUWyWlSkNJg2zaGTHqnE70w6M";
    os << "9mfwI3e56FbWpWSRhIRTYvE1lrly1cEpSyUa6VVZVpCkYg7AxYpMpqT2kbdBQvAKYFqUOqQ6SQKNvOD";
    os << "qJOMpejnI+TcxbMT7PNqGW7mYGuSRlcuiKjr+iQufk64wfHyBdihMiI7kE9orevi8Hh9eDnqScBSdjB";
    os << "I49s5akHb2ds3xgbTz2DF7zwRQwdvRbicerjs8zNjbN/ZC+WcjLCvWuTQxA0Q3QckaaQpAZpZ7NFkVp";
    os << "ZBKiSSJ3FKwjBQqLg4oXOwrJTUu3iuD6OZ4MSJFF2HbcchzTV2WhEZtGsrusveGqmpGn2njmORb6liJ";
    os << "d3qU5PE5kko+KYbHtvhCaoNZBIbNfLHK9VduyFUEhlZRfVBR29jhOUrdDEEAeZllZZSAQ6yShjUmbLH";
    os << "dKMOeA4HpbjUBdVjDA4rksSZrZbjpvF2lrSxvYcwloTx81hOzaJF5GEEUpYFIoFvFwO5XroMKAxX0WK";
    os << "7KKez3vYg51EcUKlUqPWaNJshNi2ha0kzZpNrOv41gQYj9WrTuHsk5fRUZgiX8rsAdOwjo4ihO+ghCT";
    os << "nu9iOjetnzkN+3uXAyJjY2K8uKvk9v9iyd+IhnnXlOhQgF4Ujh97/bIAIf2ZQXLw9r2qEZ1+UeM9Fr3";
    os << "j1zOjer8/MHXDaSgohswqjpeDS1VmgWouZqzapN2KaYYRlCWxH4nolpNB0tTc4bm032kzj+3mcoIgxo";
    os << "JME13dxXYdCoUCl2eBHl/2OPfv2smZZB4SCSIWMjkYsGejjE598L46qMz05nn2glIWlPCYrU/zgX35J";
    os << "s1ZheW8rGItmUCXneeybquC0lHjXO97IkuFOZicPkCa57GBbElsatJcNHZNIYzT4fi7TTYuM42griTQ";
    os << "STYoUGpOGKMuh0NqGkTFprIl1gjESSy0k6cms9Ut1ikk1rudnQeVhmCXjYdDJotejyaJIbQeRZhplx3";
    os << "YzX7pmE6GzSjRfLOAX8oS1gKauEmEQSuHlc2BMtkiybIwAx8mqQbNg4KCkhevncFwHZWWzyDWHHUfT7";
    os << "uZfL/suQT3gtBMPp6Orl6BZRmtNGEUMDQ3Rt3w5v7tpN9/9xqVU63VmayH1Sg3HaCzg5zf+gbaOy3nr";
    os << "JeeQBgn9S9byyteeiGO77Ns9guPnEcJgSUVv7wC27yFQma9ivkCqU/bu3ELQjLJq3bIxtoNl27S1lPA";
    os << "8l2q9DgZyvTmCZkC9FmA5Do5jo52IoFEn1QFpkmApBy+fz/JOlCSqNvCLRXqHh3AsB5IsSTCMUoJyky";
    os << "iOmD6whYnxgwiVw3LdLMdZgLIUQsqFc81CCQMyA0fLdYibEYlUuHmfsN7IZqG2RimF4zokKZRKRUqd7";
    os << "ThKUWrtwnZ8qvMzBLUKgwN9VOs1KvM1pFQ4IjM4NoUka3tRlDracBwPIQ1RLFGOi53PE+sYFiJ6W/sG";
    os << "yOVaSIXCdexnhmAz4/vZsXUHUlrkbAslwXYkNd8iiFwgZGZ+nKOXL+PIVTETU5owdLBl9t4px8KkoJM";
    os << "sblhHEZbl0NLeqsOJWWH5eRNOzqnV3YOvHpur7pyvNCo8u6BdXNgu7ioOBcpncOfPgV9/tpnif3jiP+";
    os << "3Y7R23cnB9yU1vjpJmc2igL1Y66CzmbVrb8hSKOYSU1Cohc5Um1ShGKYVCUCrk6OntoG8wT87NWgHXs";
    os << "0BYGKPBCPx8HmXZ5HI+S1etpyqW89GPfoLN995Mdzfk22DPDhgeXMNvrrmW5vjdTE5NkRpB1AwpFFvo";
    os << "XnEMm5+c4J8+/lHmx3bg+2BCODgPPUtX8J0ffI/lA4KxnY8R1Bo4rs3wYC/51g50LDFCIYxG6ogorrF";
    os << "nzwjluXmkSDNKiDE05+YYXNrJ8OoTKOR9pANJ2CCOkix7GJvdu/cwPTmOsCxc16GYy+PlPYhTEpOBjC";
    os << "MUynHwiwVIDeXyLCLNKB6NKKI6X15oqwXGBHT3dNLRsYJSSxHhaOJGlSg2WEaRaMOevbuYmpoADUkUA";
    os << "9kG2/YclG1TzBfp6OgkjUOm52ZwC+14lmLVEUcyOpfnvNe+m+OOOZrKAp3m4tedyOjeEYIwoLW1g47u";
    os << "Qb7+0z8wtm8Ha1euJl9q56brryMJGsgkRKqIfTNVRkfnsTS0+4LQybPx1Bfx0x99lpmdm9i3ay8d3T3";
    os << "sPThLPZKcfOppFNq6mDpwkDBysEQTgoNU58ezuSMG5VgkGsLYYsnKYZauWocQhjBI8ZyYmQP7GBvZQa";
    os << "l9kO72AnauQBBmFbXtwL7d22k2anR29dLZ24WxbabKEjdXYGLvAR56aieNyjRLV7STBB2cdebp5Jln5";
    os << "7Y72b9nDMvJL1R3ABJjBKX2dtasWY9X6Mh09HY2G43DkD07n2JqfAzHs2lWGsRxRL5UpKu7h9bWFnK9";
    os << "a1B2F7MzB4njOn0DK8EognAGR80xuu1Jpg9OY+c80hTyuTxLlg6TL3ZkM2mVLcawFFG9ys6nHiOo17F";
    os << "9h6ip2bJnlqM3HMfwqnUElTmmp8pZdrU5SNicZ2ZinNr8HEoZ0lSTJppaMyRMEmzbRiUGx00JgoSg3M";
    os << "SyFU7BQwNhPViIW5CU56pUm4aR8fr4lu17J1IpgjBRldXLlrQ99MTTf9y5f2orz84WDw2fWlSuPMfo4";
    os << "f/XoPinQq/WDXe2LC/wbt0M1hWWLdtcD7Q11OpcIpLGilLJw3Zs0tRQyLkZBywx1KOESi1EpIalSzvp";
    os << "6+/Acx2KRReTZol/Ti7LH7aUhZAKSzlICf29XSw77pV84Qvf43vf+RrrjxbYyqY+H7FjxOOr3/4hr37";
    os << "JcnY9fC9auaRJSmpSOtpydB/2Sn77q9v5yF+/ieElmmYd9k07XP7zKznj1PUcePRmZsp1cq7H3qk6m+";
    os << "5+moElvfT2LaEZQd4XxPUKTzy9g96+Vk5e240OajSbAWkasWzFEfSsPZLrb9rM3pEJGsEE5Zky7Z19j";
    os << "B/Yx+Hr13PChmFkUqYZ1sl5RQZWrCSOJVIp7FIBz25HG0ncnCWq1zIuXGsPOk6JKgfYP7INnQRZEp5M";
    os << "WbXmKNzuo/jd1TdQbdTZv3s71bmAJauWMLJ7G6ccfyJDAx5uWmN+dpbUGNIoQSqBsh1KLSUsO0csfdY";
    os << "edQyuaTLy9P30LT2Mur2KjSechWXbPPLAXUztvJf9Y3vo6yhRmZ/G9x0KxTauvGcUYVxecuZRrD18I/";
    os << "liLzppoBOT0YQSQ602zw3XXc2nPv0t5qf30d8BE1WLz3zpW7zvkmN58OqfYhcGiZ0Sv79/F3fdcR8iq";
    os << "tAIQ5ppyvx8k+uu/g35YDfjY3syGaeliGPDY3sDfn3NZga6Yf3hK9n84DY6uof4wj9+ghwH2XTrDVy/";
    os << "aYR83udNb3o9YTPgrnvv4Zi1XaxbPsCdm/ezfXqa6vg0W3btYc+uUZrVearVeRphhGNb9HX6HHniS3n";
    os << "NK1/HGS9YwdzIg+wf2YJy8tniKtF4uQKVUGXa8eXLcZ0SzTgkkQaZhqwfKFL0DLXqLEG9QapT/FIL09";
    os << "WQx7dP8MSjT7PzwG727J6iGcSceMIJnHbqOu7546OctOHl/PUHXsHkjnto1OrYrseWPTPcs3krS1csI";
    os << "+/kiVNFoS1HZbbOjj07OXJ9H6es7wcMlp3jtkfmueaaW6iUd6LclFq5wfhUg9e97iI+9fdvZM8DNzO6";
    os << "7yBYFmHQII0Sms0AnWbadmESqpVs1qqjCMuxcQs54iQhagakqUFIlYFpvUm1EZtqpZakKca4HXOeZ3m";
    os << "jO54o7K86N9++eftVWusZMnA81AVnkav4DC/x/xZQXNw6O6996Wkb5nZv3eR5sTjyyJV1KWy7Ua84Ot";
    os << "F4voelFK5rk/ddlKVwCx7lcsDMXB3Hc+hs8WlrLVBsL2ZrhAVFiNYG23NxXS9r+Wwb23JoLfksO/Y0/";
    os << "vHLv+Gy732T/i4oFCW+Z9ixy2CcPu6/9xb6CuPs3rYNpRyioEl7RwvtK17KX33oE/z2lz9g7WrJ7Kym";
    os << "HBf57RVXcsQSi/07txKnaTYD9EpseqzGl/7520zP7aVrQFKvgDA+b3jLO3jDSzZgBWPUy2WEMXR192J";
    os << "Kq/jYF3/MrTf8DiltXv7K8zjz+BWsOmojP7n8Gv7tJ//ON7/wd7z6rOVMzVeYCwt85HOXsevpnTiOhZ";
    os << "/rprurSGtrniiusOWpPTQDTb5YoqW1RD5n89G/ej3HrOxi+9OPsXrtYUymPXzii9/j7ltvRUiX177mN";
    os << "Zx6/BCDy9fww8t/y43X3cJlX/57jltT4vHHH8bLFzJDCjurvC3HYtdEyl2bR3j40R18+G/exYXnnUWU";
    os << "Ct7x3o9zxc+v4OrfXsG5527EVMeoz02x9cknaFTrtLYWmK6F1ELBSccdxQ13bOOqG27j1BPX4fstBJG";
    os << "mp6uP0dH9zE5N8tIzj2bJES/gjrse5Ltf/CTV5jRzQQdX/vznnLpB8Pi991Fo6aRr7Tn89d98il/9/N";
    os << "8ZXgJuDnaNwIWvfjPf+dpH2XHfr4mwQWeO4IPL1/DkmMOb3vg2JsfHGBo0TE4rTjjuRfz7z39AXNvDH";
    os << "x7ezi9+dgP33H0HSZpy+JHH8rtffJrHH3qIe7dUWbG0n/aWHvqGljFzcJTUCAYGe9m5Zz9vf+vbmJub";
    os << "ZHBIMjfncewxL+R73/g4pXSE/aO7ENKDVJNvacFtW8NrL/kkTzzyEP39AuMbZsdBaJ+bbrqKDassnnp";
    os << "oc8YHTKHQ2kpqdxBaQ+wf3ceDD99Cb8sQtTDhG5dexsGxg/g+2I7Hz678HS/cYLN/29NIy8W4bdxwzz";
    os << "jf/NqXmZ4ao71dUKlLWosdvO+Df8/5Z63CCidoNhq4jkvvmpO4c/M4f3HhuTh2QH8/HJwAyx7kzt9fR";
    os << "Zvax+MP3EsYx4TNJs16AyEEYbOJ1gm5lgJGZ5tAITLqEanJ4j2UJGyEJEG0MLPWhDql0Qgol+sYo6jX";
    os << "quzbewDllpJbHpv45Ey5uo9n6TmHVovP30L/WVpo+f/+I//t23Msxh7fvW/f4LoNm4JGzMT4ZL5WLTt";
    os << "pKkiRNJshYRxn9Akl8XIejm3T1l5gcKCdrtYCpbZitiEUIiv9DQTNkNpsmfpchUa1CiYj0BoJdi6PVJ";
    os << "qGrtHdM8hxG1/BgVlFOTX0DltIcZDTTnshD203DK9bizABg0NLaF96GpdffhX33HYVHd2GMDVUGuDLF";
    os << "ro6+0hTiXAywnIYRUT1Mm945Xpuv/V6jjr2SOrVgDAM2HDi4Xz8fW9nsKCJmnWUY1FsLdHSP8Q7Pvh5";
    os << "rvr1Lyi1NxhYmudjH/kYb3zLuziyJ+Yrn3k3555/HhNTB4miGM/2aOkY4vFHnsYg+MF3v833Lv0awtP";
    os << "cfPstbNp0P+vXruab3/oan/3sp5ivzvLQI0+Qa+ui1F6kd8kQsn0ZF7/rI9z6++uwnSYD/a185GN/z+";
    os << "svfgtH9Gu+8bn3cf555/H09i04VrbdNHGaLVKUxOiQ4eG1nPWyi/ELDk9t+SNvf/d7+O4Pf86NN9/Id";
    os << "df8isH+Tg4//DBmpg7y5W9fwWW/vB2pskiD6ak5XGFY2lNgbuoA55y5mov/8mK+8o2f88G/+zxf+uqX";
    os << "ePdffZBPfOYrNFSJtWt7GXAO8qrzzsPkBkklKDHDG9/8Fu7cPM+yNWtwfYtUh0xOz1FoAZXLlszLlsP";
    os << "11/+amXnJ4IojsBA4rg9SMTuxn5M3FLnr7ls4csMJzM7C0KDmkcdv5swzziHQXbz6wlfwrz/9Oieecj";
    os << "KNZsTnP/Nh8o6g6Ane9JJVVObK/Oraa3n08buYKs8wMzvJnQ/exOTYdv6w6RYuuvjtTE0ZWjoa/OGBG";
    os << "7j4vZ9CdB7P4NAKpMm8D12Z0tkzzGGHHU6SBFh+kzQO6OkOqNXn+Onl15LaXXR1t+O4Hn4xT5IKfnPn";
    os << "U1x/041oq8z6I0+gb8kQrfkct992HVf99gaOP/YwtAp433vfx2xjgGJLO1EUYSUV3vnmk7nxtls47Oi";
    os << "1hHFE2Aw466wzeeclF1KyA8IgQFkWQqRYosHY/gMkRLT2ghHQ0Qnl6gF+cc1N5PuG6eptx/U8lLKxXQ";
    os << "cn75Av5cmVisRBRKp1NhJQmUzRCIFUAp1qdJri5VyQhmYQY5JsXq61pl6rETYbWE6OSHbuKNebKc9Kh";
    os << "RcVcs+fLx6KN//j2/9JUIRDuIpbt45Ej2/b/dF8x7KZg2OzaNIFYnRGO5FSEIYxzSBGa00URpCm5PMu";
    os << "re15CnkPx3XQkc5S4MKIsNrEsq1nNrWJXvggW4okjrGloehboDw+/PHPsHrdqWzbJgjTlNaSoBpOccF";
    os << "rXs/YeCsrNpxNXDySN13ycTbdeRfnvvICyg1BM4FYQb2q0UETJVJIdeatGGrq1SrzoyN0FJu05lupNU";
    os << "H70NQ5iGZpVKcAiS1tbNtlbGKW0QNjCNsgc1APDnLK6Sfxj//8A6L8Gpx0lq995BUcd9gQE7NVkJLxg";
    os << "7vpGxjgJ//yXc46cxUnHb+UE084HmSmhx5cdTjnv/wcXnf+kTxw56288IwzeOqJp7A8C9fz2D82znyl";
    os << "ggby7TBX38sJJ23km9/+LaL9SKzmDP/0/rM5YvUgB6emae/uwMllHpJCGpatPJyKWsJJp7yYm2/exEv";
    os << "PPRnfb/LZz32Gz33hS9ilGsgKr7rw1bzt/f+McAqcc8JKpFwIuW8GREGToFGnUa8RzI1zzhlH8vZ3vQ";
    os << "UjNHlfLyQE5nnl2S+grWiTuG187vNfYc+Bp1AuRAlUqjUe2PwglpT0LVnGnXfdx33334G0oVz2mKnaR";
    os << "AKQDd73N39Hceh4WjvyKNvC8TwSA/ueepBlA4JLL/1nUquFg9OCjgHDeHkrLzr7xex8epS21iX8xYUv";
    os << "o62jDawmSaVO0fcxccjLT1vO6iVL+NCHPs3Fb7uYN739Ej73yW+zsrfIku4pvvaFT/DSV/wlE1OCvqW";
    os << "GzQ/eyns+9DGsjhX09A6AU6A4cBQj+w6y6Z6rUUUY6FnF2We8HmPlUB7csel2Kg2L1s4BHNtBSgs/7/";
    os << "HOi86j2kh57WvfzSXvegeve/Nb+db3f0R3Tz/nnX8OL3rp68m3WOzdv4N3vu+jFPuGUNIQ1MuE03vpy";
    os << "DVoVA2NeUhSqIUpQtdIo1pGQ9KGnF8gTFr49dVXEOsU6TtUgxxGCfIthksv/Tbbts7T278KZVu4OZ98";
    os << "sYDr5nBcF2vBoT6JEypT8wTNENuxsRaWTEZnoodUGOJY06w3mZsuUys3aDYiGs2AiYlZUtlWv/3BJ25";
    os << "JkkTyLCAu7iYWO9A/uXD5n97+PwNFQD60ZUc5zbd+OgiVOTg2nUUBZLCY0RMshdaaROts22nApBlZOU";
    os << "3S7CpjMhdsJSVeMYfluXh5H9f3sZTKrnYyI9pKoWhrH6Baq9HWUeIn3/sBR6w9mr37DOXQ0NGpqAczf";
    os << "PHr32HkgGLjxjO4446b+eQnP4ZMPcqBIfZtrAKU4ynueeSxTMKHyBy3lcJxXJr1JiYsc9RRR+C5ivoU";
    os << "pBVNmkqwvIw+IQTVyhz9BcH9d1/PxZe8n6mxAqPjhtQv8/kvf4pTX/yXPLzTo3/ZMGuWtSGFoVarkxd";
    os << "NfvGvH+HYVYrdj2yiOTvOgb1jGBdSAaMH9hHO7mT/E/cRTdzPV/7hYoa6c0zsn2Bycop1Qx733X0LF1";
    os << "74FmamC8zOG1J7jo/9w4c4+xXvZddMG6WWPCuGikRJlHEpncyEoqVYwuns5VUXvZV6vcIf7nmAT33sO";
    os << "8xXLF5+/sv4yY9/yeR+OOHkC7n11pv4+WUf5+2vOIY0DqjX6xl3MO9iUk0SRehEkyYpSXWUiekJtIAg";
    os << "gSCAfL5AX7eN6j2da25+gu/88PvEacye/QIjO/nlFT/l7/76YsanZohUHyMHJynXGjTjAt/79pVceMH";
    os << "bGB1VOEXYdPdtPPjkHoYPP4k0qWG7HrbrYoSPCed56omH6BkocM45L2Vy3MPNwVRljFdf9DbmpucZHF";
    os << "xHvuBRqTVRKssmLpfLeNLwt3/zfo47+RRUi8brkHzwb/6WU1/ycnY88SDN0Xv42mf/irNfeAGN2MJvM";
    os << "1x37bVsfng72yabXHn7Nj78hR/xzo99gLHaOGEKqw47hQ9/6FOccOzLiCLYseMJrr3lPrz2XvIFD2U7";
    os << "pElIgXne995L6F0yACrBadG896/fSbHYxY3XXMVXv/JlKrUE3/fp7eshCeponcki69U6rqU4/pT10AG";
    os << "JgshuLMT4qgwMLIXf0srofJnHHnmUuC4499R38P1v/IbyXBEDTM5Octkvr6EwsJqWgr8gPLBJtSZqhs";
    os << "RhjJvzsR07EzZIQRLHVGbL1OtNkiAjf1uLckmRcULNAs+1Vq5RnQ/07Y/uun6+Vo94tipcVMUt7ikOB";
    os << "cT/a0DxT/2h4oZN9z3kdK/5ZiMQev/BWeI4xqSaKIzRcYqf87A9B8u2cBZoBcoSJEmCSTJFhlSCJE4y";
    os << "4wNMRqxe4AGmiUYYgZ/LobGYnJ5mdmaOe++5nuHlw3zr659FNAvMV6GZGAo9goe338SJLzyWffv386E";
    os << "PvZ8lw8sY2beHOIKgkSK1JKgn3Pfw46R2AWlJdKpRroWwJVrHCDvBs32CWKJjqFfqJGEDZWX51sqGnt";
    os << "5erJZOnthyP5/9u1dx8w1Xs3r4BBrzDqUheHr7A7zk3PO57f45eleuQqIJmg18GdEqQ8ZG94BwsQo+u";
    os << "WKRKAUt4ISjjyJXyiMsj/n5MnYyx+Ere5mdmWZ4aCmR1cqWJ+/n6194B7/95eUM9G+gXrbIdxsefvxO";
    os << "Xvqq1/LYPofB7kFq01PoOEXHmSLE83PMTlcZ23+QF73oTKQU/Ojy7xI1NeeedQH1oE6qYWiwi5JXZc/";
    os << "91/Lo5vuYnZggCgLiJCLW2XMlWhPHMWEYkEYNWkttYCTNBmgLBpd2Eqs2fvpvV/Lu93+CoBFDUmTF8g";
    os << "388qc/5pz/p7s3D5PkKM99fxm51tbb9Dq7RpoZtIM2kMQi9sU2CGQum9gx2Ng+QmAMGHwM2OaAD4bDP";
    os << "Ubo6hiwkcA2Fxsum4ABIUBCCG0jtMyi2We6p/euPbfIiPNHVI1qStU9I1kSz7nxPPlUdXZWVFZmxBvf";
    os << "9+X3ve9zxtl3z/dZqEZ89T++xw3/egM6NWlPZz/tXF75e1dg2R5NBVHc4JOf+ByJWM/6zU9Bt9KcbMc";
    os << "G4fDQrodwVJ5P/M3n+L0XvYHpIxbFUdix5x5e9pJXcOjQPkbH15FzCyBMorrr+EThEmk4TX+hQFwT6M";
    os << "jn7NPPQIUHSWKoVqYYG8lx9XuuorHgoVyI4pD/+0v/iu16vOpZm/mdF76AIwdmSSvg5wJu+NrX+ehHP";
    os << "kJGTN+4RWpJvvwvN1Br5MgXBrBsDdoibobQnMfP52k0ICj6DA0Ncd211/LGd7yDpWYFKyvwPz59Hdd+";
    os << "5iOUZ6bAskzCu4zRaYRSNokDKoClpSpJGJsSVp1hYXHTPbv5zN9fw/TRowhbsHnTmWw983ROfcoWmgJ";
    os << "y4/C1r93AvTtnmBjbiGubuH4SxqRSgmXqtLM0ozjUh59zaVTqNKpN4mZMmknSOCVJU9JUEscp9UYDKV";
    os << "OiZgh2Tu8o2z+dq1QrdIXgeCQAPq5g2G5PJCj2CnhqKaW68dbbf7BjTr3fdvNZrdpASm0K25UiTQ2Th";
    os << "8wUnmcqEDBpTSRhQtI0UpzCsVoxCxulTYmeiTeaqxYHRwAAIABJREFUfa7jgRgmyQRJlLB//ww628dZ";
    os << "Z2zlnHOeTmMeGtJIbO6ammLucI1Xv+o1vO99/wVHZAz0r0JLiKQm8QTY4Od9LOG0fppAp/IYkQBS8eD";
    os << "+vdQrKeRgemmKmcmDZM0YlUqGhkYIJi7k7798O29/+0eYmzrAxef18YsffYU/fucHqc/5DJ8JtXSRt7";
    os << "39PUzNBOT7ViFlSiMMmZ0vo7UgDus4jmLDho1kEegUwtRobQjLLA5hGNMM66w7ZQPu6ot5/6e+ybvf8";
    os << "3Eqh/fy/AtK3PHz/5fXvPoPqM75DJ8Gc7Vp3vDH76Hpn8KqVUNEYYM0ScGGeqPM+Jo8V77pjXx/201c";
    os << "dtmFfPlL/8Kzn/lMXv6qF/PjH92I7XhcfPZWKvvuY6HaxM0VSFNJo1KjUa4SVmqkaYrSGUkUU1uq4Gj";
    os << "BJRddjG25NCPIBCwsLjIzWeUnN/2CvqLH6658HV+//lp+se2LXHIW7LjtJqq1mC0bN7JhwyZ27jiAbT";
    os << "sMD4/x+te9ibe9+02oggQXggHND77/Lb7yr99nYOMl5D1Dk2YphWULzjrnbGbmFqg1Frjm2s/ytLOex";
    os << "uwB6FsDd+y+lQ9+7M9x3Ix8qYRSYLsWwnWJYonn53np8y7BsmziOGXq6D5TPiianHL6+ezaXebN73ob";
    os << "DZpUj0IQ9HPp+RdwzoYhnnrBVs4/50KO7ptjuLiab1z/Xa64/C3c8I1/56v/9m2KIzbBGNz+859xzwM";
    os << "7KA32YUlJlpmSTUsIwjAFy5BszM8s8vObbmFpqYIuwfi69bzy8ucRTt9Os1ZBCLtV8iHRcY0jB+eJQz";
    os << "OE5w+VCct1dGZCWVqnPG3zWcwt1lCpZu3mdXz5X/6Z5//Oc3hw3y6UD8UhQWVhkQ997HME6y6gvy+Hs";
    os << "JQpQmjrGLXISCwLZJyCJcj1F44VHrg5H4WZ81qByiyiWoPy0hw7jzbvOThbnuH4GuZHkDw8ke2Jdp+7";
    os << "f9Sx8px9k/NHdGH9d+fnalTrJlHVawX20zCFTJJZikSmKK2PVYWgNUmSIaV6+EsyWlKkxgXQWUZpoB9";
    os << "tD3DfvTsRQvD0pz8LK65QDCpc+78+xcTYWsqHoSElcVXwile9hi/+0z/gcADBNOs3bkQ7kPiCLGeDA1";
    os << "KC443jtSxZIWxT4pal5EsTPPtZz0L4FkgYHRtlzeanYtkOvm0znwie+bJ38jcf/RgTGzeyeetzOHTXz";
    os << "TSnfs7ffuSdvOM1f8LSfhdnFKan9nLDv3yXXJA3IlFAJhOa9SaOELjeWhKZHnMk9h6eBXuMfL6AjBJ0";
    os << "mhG4ATsmI55+2e/zj//zc5xx4bmsPu08tt98I8nhm7jm0x/gda96IwuTAmcYjuzby9e++T0GhzeSZSm";
    os << "ZzEBqavWYo7t28rEPvIk/evOVLFUs3vam1/Pdb15Ps1Hj2mtv4CUv/V1e8pJnMTm5x+gqN5vILCWJE5";
    os << "IoRilTmWOYZCS2a6F9i7vuvps0jrFzBhTnZ6sMFTVf/p/v59YffJZrP/UunrnVI5y6i53330MzNTmXx";
    os << "VIBsoSwEfLyV7yC733n27zvg2/jz676c154wQtoNgWZgNRJ+Nhf/g0zU03Wnn0RtpXhBx4EE5y6+Uxq";
    os << "1ZDt991LsVjiRz/4LlvXn83SIXDHNVNTSyzUKpy2aQ3aytDY6ExSLBXRVoEfb7uLNJVkSlItR1j5Mzn";
    os << "90t+nLDdx5Xvex/4de5FLFqsKY3zxms9z9bteyr4H76aSeLzrPX9Elik+/cmP8rIXnMc//N17+bMP/A";
    os << "VFr58jRyRuEZJmyBe+9C2kt5l8oYhjO5TGJnCKq8h0BkBzPsWyFV/52j9xxStfDTXYu2MHH/7wf8MZO";
    os << "gPH0sgkxUZBFlMa2cKFF12MbrEVbli3iYE16xGOxmollw+NTjB1+AiDQ4N855v/ztf+8VNc/Qdv5T1v";
    os << "uZphbzX1msJfBz/96Q/5zo9+xWnnX4CjDalF2kyIamHLEkyIY5OOkyvmyDJFmKToTJsQSqpQaYrru5R";
    os << "GN8Rx5rJjX+3oHTsP7+Vh5pteLDjdBBDHyB8er/ZkgWJn3WKbFTfZdtud3+9bd/Z+U6pk4boCxxV4gY";
    os << "NlO0SxJI4TpFJo1RKmNxFIlDQKfIZQxjpWRqeSjP6hEfITF3DDv32bW27+KUplfOlr17PUGCEpVzh7S";
    os << "5Ft277PqRvOon5E0+8Mce3n/46CM8XSvvvB6qdcrUMMlXLCYjkGC7523Zf44U+2M7D2LAqlvHGLHZux";
    os << "9ZvZfbjGNdddj2pqSODgQ4f49YNT+IND+EGOytE53vDKF7J6zTr27NzN7MIsE1tOpzx3CM9f4vcvfyl";
    os << "ZVcFsjrf94R/xhlc/m1qjgtaaNJWkUYRrw9D4qfz7jbfw2f/xeTNUgO986z+44ZvbEP2jgCCOQ2q1Gr";
    os << "KywBWXv5CJ1Wv59U13Ua3HbNx4BvsfuI18IeSKl72UrKZxyiXed9V7eckl6zl4YAdZokjCiDiMkYnk0";
    os << "J59lA/dwZ9e9Tbe9+fv471Xv5VYJbzrj69m3YZNfOW6TzC96yamjkyhsowoMoQShs/A8ApqDJWWbcO6";
    os << "NesRpXM4MDMDQGoktrG9In2Dg7h6Gqs+w5EHf8kDD/yGvbseMozSqcQmI/N9tv3sLizH5QPv/1PWr1G";
    os << "88sXn8q4/eCdb1p5NPKWoT5tRNzX5EC/7vSuZqY2wesMm8kMT3H3PPv7ir/6aJA753Kf/gVtuvZlSX5";
    os << "5bbvs+r3rZ61BHc+CCH+RN0BaLJE0QAnx/gJ/ffjs33nojytdIL+Mb3/0G2278BTf+9BAve83ruOtnv";
    os << "6Rv1QDv/oOr+PlPvsXlLx7ioTt/Qq44wFJFsG3bLbzutVfy6stfyOSvvsDinm/ziQ9fyddvuJ5hez21";
    os << "/YANX7/+S/zz17dRmNiMtn1uP7jEP/7z15nZf/AYq+C2n9zKYnmJz33u77nk/GdDDF+5/p/YvnOasTU";
    os << "bUVmCb3uU1mzhR7/aznXX/S9Dq5DCnTvv5Kf33It2c3iOSR6fWZxk966dXHHFKzh781pOKc7z7ndczh";
    os << "ve8BbiOYfmAUhSIMl457vfw813JZx66pkEeQevkCco5o9VqYW1kCRJ8X3XlBXGkrAR0Wg0ieMEpTRZI";
    os << "vXcQjnShQ1Lh+Lcdh4GwzZpbJtNu5tAthMQH1dQfNLyFDGP1fMYVb+2LvRgMeePXHHpxr8eKOiB0bFR";
    os << "HNfBz/v4gWEDsSzDVuy5hgihzRMohGUK830XIRxsx8XzfQb6Bin0jfPFH+zhhq9cTxw3ka5EhZrLXvB";
    os << "K3vGmF3HxqTB+xgvYfschnv2Sl/CRv/gQf/buK1g6dBteboQbfriTT/3dNdSjBTI7Y3T1OAXXZ9/9+x";
    os << "g9bQtvfc3vcdHWfjYN53jg4DzbD1e5974D7HxgB7lBh9WjozTLKTnH552vfRanjPhoFBvPegpL0Vr+8";
    os << "Kq/Yu/+Sb7wmT/jOZdcysziAs98zusoV+b420/+LW+8/HlM7fgxMzNHCat1tAVBLmDV6Dh7F2P+9dt3";
    os << "43kO9WaFcrXKUKGf6ZkKl//upVx25irq5TIqk/QV82y96ALmGqt44x9+kEot49pPvI2LnnEph49M8qw";
    os << "XX4lMJZ/59Md58SWr2XfP7Rw9ctTIuLae6GNB3g8YO2UdN/9mls9/5f/j0osvIec4pJbLx9/3SsKpve";
    os << "y4/x4cr0AmE6IwRmfZMa1sSwhTGeN59A+t4nA14BvbHuRb3/w6SSMyzxNTsD2Pt779Xbz7zc/FaRyiW";
    os << "auQJAm6xc2IthgZ7Gfj+Zfwijd8lINHF7j9pi+TDw+x/8gct+4os337bm67/V4uvuwZNKtLzC0uUF5K";
    os << "6Bsa4AsffysHp+f50F//Ew/c9xtEwSdrxIyNrOYd73wNV7/9ckSwno//zbV8/ON/Qd9AH9/85j9z8Wk";
    os << "l9u+9H9u2Gewf4zu/muLDn/3vzB6ZMlkIGohb9eU2rN54Cl/+/Ge47BlnUD5yBzNTh6mUl3j6i36Ht/";
    os << "2Xa/j1nbv45Y//jejorRw6fADP9envc9lw7rO57Y55/utHP4UuQVoLGR5Zw/NecD7nnTrO927ZzeyRI";
    os << "xydmmFk3TBxFFNfaDK8up/XveZlXPS05/OBj3yKL/4/X6B/sJ89D96OnL6N796+i71lxe67dnH08GFW";
    os << "rRlhoD/P0kyVvoEh3vnqi1k7EpDzA+48kPD6d3yQH9/4VZ6+yWPH9tvYNQe33z3Jr+7aTmlkmK2nnMK";
    os << "+Q3uwpM2ByQp//JbL2FSIqZeXSLOMMGySRoaBqhnG5HMBpcEitUaTRrVhWH5sG2VbzB+d5sF9c0dv3L";
    os << "5wdzOM29RgbaLZNltOuWNbiS3ncclTfNIqWjhe0KrEw8A4sG58aPPzT+/7yMRIyQr6SgQ5n1whZ7gDM";
    os << "0PXblxWt/VUWeDnAmyrRY4gXIK8eQJdzBdpJILt+5a48JmXMTI0wfSRQywlNbJQYlsxW1cHDA949K15";
    os << "Ov/tk9dy1VVvJ5y5lyzLCBPNL7YfYO3609i4+VTChQbDpUHcQDJTX6Rar3Fw1y76ixanjRSZrzaZqmv";
    os << "WTgyxacN6tATPsdCELE4eZnGxjueAUhlxWGXtulMZOu1srvnHH7Htpl+xZesWfrNjDzYZX/z8X7Gmv8";
    os << "5Dd97N7Ny8iR816qRxYnLCnBzCDzjjjM0MbthKEplaaa9YZO7wDvbs3IdHQhKGYJkSLM/WnHbmuRTXb";
    os << "+FTn/0Ptj+4mzPOPouf3/xLVo32cd3ff5iB+AA7H9hOudpEJhLQYAuSemSYb/IBfYP9rB1fzZ6qxZ0P";
    os << "7OSUtRt40fnrmJycZOrQPmw3QMoUmUqUypBpaqjKbIFShpXG9wOs4iC/PNDg7l/vJJOaUsnGtfqQwuK";
    os << "cM9ZTb0aUGzUu3jzMlomAeqWKJQSZUiitWbt6HFEc5OPX/Zp3vfnlnL9BMn10hkjaSMvhlDO2kKZ5Bg";
    os << "fGkWmdTGekKfzq9l8y6EuKOR/yw2R2EduxUUrTqFVp1BbYNGgzsWkjKjiNK954FT/5wXf52lev5SVPG";
    os << "+OhHXejLYHvBqzeeAoHG/3cfPM93L7915yx9SnIVHLffffz7Oecz8tf9Hwmiosc3v0bFsshcSLZsHYd";
    os << "2eB6/vD913DdJ98LCw+w/9B+XM+ESOzAw3cFq/onyI0OYeWGjbbOwiT3P7iXU8cHGSwVCQb7EW4Rxy8";
    os << "hLIswrrLv8GEWZ2e5+JzTcFedz9Uf+Sxf/MK1/Omfvpm/+69X8sMfbCNxBjjv9NUUiyXILGwgi5rUKw";
    os << "tMz5eRUYORoRF+eH+ZB/dP898//FoO3nMribRpxpJVwyUGRkdxshz+QJ64Wsb2Anbv3cW+hybpFzFp0";
    os << "qRRN/XfcTMijhMajZA0kpQKeXIDORq1kLSVuB0nTaJmjRu3V+64e+fkIR7WZmkzb7eTtTsBscbxIvf/";
    os << "Z1S0wLK1z53a0EUMKPYD/a949kVvGbTnn13s81g1NEg+HxgS19bje8cxXHHtIG4ul8Px3RbbiIuf8/E";
    os << "CH9vx6R8cYv2G9XheCSwHz3cNtYZMqNbLVKs1omZIob9IPlciDJtEYYjWhsZpZGSYYn6QNFNg28g0Qq";
    os << "YxQtikUYSUIYsLC1SWFhjo76dQKhJHKVqnaK1pVCtoJXH8gDjNUJnG8zziOCKq1Vi7YZgtT7uUu3ckv";
    os << "OkdV3PehRfwhc98ADW/mx333kGUWDi+QxI3aZRrRLUIx/cI8nn6B0q4foDOFIW+Ap7v0qg00FoRhxG1";
    os << "eoNMpoa2zPdxAx+dJQz1FTn17HO59YEmf3L1R3nRC5/LJz78ehb23s+enQ8ivACZZSRhiJIZqTS5mCZ";
    os << "gblid+/qKjI2txbFtMstiYfoQtXrzmFUZhZGh/hcY1muNefKZSlw/ICjmyZWKrFuzmtFN6xDeEAIjlY";
    os << "CVYSURSbXOnr0HWVooY2UxUdhskb/a5hxKfeyZD3Fzozz3qRuZnNxLFGsKuRyF/hJCQL5Uol6uYGEjb";
    os << "MjSGC8oUW/W0UrTVyqRK5QQwpAdZElMGsXUojppGrJ+42mE/mb+5EOf4KUXP4Xnnj3O1OQ+EDYyTvBc";
    os << "mw1bzjE6PJ6LI7Th6lQJIokoLy0xNbWfNNYopSBLOf30LXzo2p/y1K2beMVFY9x33724ftEwKWnDawi";
    os << "CqFZl1egQvp8jjZOWISCI4ghLmBxZr0Ur5+UDXOHhBIbEY2l2ivF1ExRXn88PfraDv/zLT3H9dR/kzA";
    os << "mXg/sP4hQCwuoiMpHEUQwaHMdDqoxMaVwn4J7JOpdffBaLlWmmDk8TFIpGliAfoGVKUm8Qywwv8EjDE";
    os << "D9fojy/wOLMItICmSSG+DZOqS7VCKPYhE6wKBbzSKlphiFZkjA/M8PoUy6s/u0137opkbKtwVLnYWH7";
    os << "TvbtXkSz/+fUPsMjXOg2MHbKERTosBh91x153vlPfftEX+W8VUN5CrkclqVxgxYPnQWBb+O6HrZlGZA";
    os << "oBLiehy0MeaftODieT6FQMBxzmSkhFLYHmalttn1jYcrEiP309RumFKVF67zNySYqASXIpDQaz1oTpR";
    os << "EyjHC0wPJdZJIgbMfkTmaSLDIDXCozKJQyn3VzPp7rtdi6bZRWlHIu6zacTuoJSr7N3PRRJg8ewfELL";
    os << "bKCmMrsrOFcswSWbVPoL9JCbnzPhxZbeJqkhLUaUko8z8UteIZR2XPxij5ZrInqNQYG8mzadCZJwSYX";
    os << "Svbt3s2R6aMUBgYMCa0FKklIw8QQBwiwHYHKAG3hBS6ea/REMpnhul6L3tiIQTVbeYmWZdh50MoAqmX";
    os << "IB2zHxhE2Od/BcRxyfXlc28MS5unnwmKFuB4zsKqfOMmoVuqgFbbjolWG7Zradi1choYGSKMaGU5LRq";
    os << "HFx2oJE8fUAuEIUykhE5I0RdgOtm3YzbPMhKVsYaNaqV5OEGBjI+OQ4TXrKOT72LP3AGS1Vow1bF2HA";
    os << "NvRuJZF4BfQVoZMzEIUK0mmwMnnsTAPOhzLiE3smVri9PExFhePIII8WkqzCEErwdxoskSNED8XkCvk";
    os << "SOKYZiOhkMtjuYYFyfO9Y1yfWWY0ejzbxvECFClDQyU2bD2DX9w5zXd+fg9//spzmZzcR6UckqXKZCq";
    os << "4jhHVwkK4NpayiOKEfCHH0KoSk0eO4jqeIdtNNcIRpGliyJBtszhkWUbYDMmUavGAKhzPcJbG9ZDKYp";
    os << "V6MzQ8o46D6zmoTJGmkgN7DxLHKrtxR3Xb3HylzCPd5l4WYp3eIvePm+sMTw4odrJvt4GxLV5V5GFgH";
    os << "PB9d/iKS9Z/crAo8iMjw0ZgyTJAolvVLflCAdexsQDbcwjyhhvOcQ0noet6WI5AJZLCYB+5oEAqJRqF";
    os << "7XioNDVWo86Mmp1wsDJFmqUGuV0XyxJGmySVWFlmbrQlkEmMhcYWVqssyjbUVEqiM0XUjIibsRGEkpo";
    os << "0ihGuZchAbRuZJARBgO37pGmKFwhGx8aIqg0WyzVsxwWtiGOTBBuFIUEhR76UI2kkJHGEcBxyOVPT5v";
    os << "guwnaIGiFpnCBlirAgKBliUD/nH6OuzxUCZIuJe3z9OEszCywsVHByOWzPplmpk0lpyHXjFN26YzrTC";
    os << "CM3h+caXRlLaIRwSGJpJphhokXYgGWZNAxh3tuOY4hqlelHSQW2g+s6KEujlcBxRKuiQpFKSRRFreC8";
    os << "g8oyfD8gafFE5vN5coGHTCRJYmqzXd81rOTaIghcw7JuKQQWcS3Cck01RSZTXM+MkyRN8X2jWSNaFFt";
    os << "YRj1RZ4YYdnBogFq1QXmpDBh+T63B8z1czwPbNuNT2CRRTBanCM/By3kobdJRlEwNyW6mKQYBSksajc";
    os << "hQeqUpUTMyKV22hVKGT1MIy4BI4COjhFQpPNsxRQ2Bh+s6hpE9SbB914BUlOLlPJwgh04TPDtlw6Yz+";
    os << "ck9ewhUwubVJaaPzqOlIo1jtDBWtHAMyKlUgoAgMIJiWkMaJWRpCraFbdkI21D9Cdsy2R4WRLF5ACUz";
    os << "CUqTqYxGpUFYbyLTFKkhkdmxtB/bdSkvVliqhvp7tx++eWqpMcfKbnM363bE8VIEj1mLZVnseqJAEZY";
    os << "Fxk5d6LYbXaIVX3zRM8972Ya++msHB/IEfkAqZUuHwzxs8TxTsoXKDBmE6xLkg9YKJtBKoCwjwuQHPp";
    os << "YlsBzXBP61xnYEaWpo4G1XgNTINEMpMxAsW5gJoqDeqFFvooeH+6ycD2li4puO7yKjxBBga01UD7FdG";
    os << "21ZVBarKCmxXQdajNuuY5PL+VjCWC5uEGDyuSyUwtSJGtUiwjCkvFTF8VvEtAgcz8FSJo9TAb7nHcsD";
    os << "E7ZDJpNWvqY2w8TSJEnaSmMyqoBatZ4EC/PwyjAeJ8hUYglhkmej1IBLmpFlGY7v4DktK8xqSXYKgee";
    os << "ZCo8kkfiBh+eZSRnkPLTIaNRibOGgaKVSOYIkkjieS5ZlRFGK69p4vo+2REuuwcgJKKBRrpkFxxboTB";
    os << "6rebc0hpjU945dy7gWkqYpfjEPQpAlKdqxsQVmEbBESwZUtWKeLcLZVs6fl/NavJWGPDaThlszlw+o1";
    os << "+sIwHMFKmtJmSoD3nZL2MRx3dYDQYskMXo5bs4li6WxnrQhRBCuYxYorSFTLUvbaPrIKEW4NlEUobVF";
    os << "kM8Rh7HhX3RMHp+Fbo1PG8cTyMgYSX1DA2RSksQpQhuGc6/gojOLgaECrptjqVI1184x11MmKbYQpHF";
    os << "KKiX5YgDKjAktQKuMIJ8nDmNQCssxYGgJQdqIWzpAZq6kMkVFactIsYnilLgZYwl9jLm9ulSnXjHUZL";
    os << "FMOHpkgZt3VG556PDsDMcDYrdgVS+6sLbb/IQAIjzBoAiPeOjSCYwePdxo17WHXn3ZUz/q6Nm1a1ePY";
    os << "Vk2aG0mn+uQSWniXVrhOG2gdE1pn1LIlluXK+aQsqXVa5uSwTQ2eiyFYp5CMY/A1HuqFm28Ejae6yKl";
    os << "ZHZ6ipnFlESMzJxxihixkcJxCuTyBiRMJY6p8TRSpMqkD0nF7OQ8jUbI2k0TFAoBjuOhkwQ7cPE9zyg";
    os << "AJkY4yXFdhG0bWqUoImo0SeIUS2uThuRAFiuEa1PoyyMz8325XB7bcYgbBhQcz8HzPKQFqmk468Jm2N";
    os << "JDyZFmRtXPzxk9lSzTNCsNklhiOxAnGWmckqkMmUoypfE9E6sVvm0kIlTryb9WRFFK4NiMrR3Fciwa8";
    os << "zVm5xd46Eg96y8Vow2jXkG4rmG71hDGaUv4y4iDCcB2DY+jEAI/57asTImUEsdxyFKJlArXsbCwCQou";
    os << "pf4+c989F8sy+jRRFOEFPlpY1Kv1h++pgCCXR2cZSRwTJ8a1bWcuOAK07dBXKoBMWZqvHNNXSaVFoRg";
    os << "wtnqQwDUEsNoyTNcWRlZBpQoLSNMMr+BhCbMIYRtr2dLaSApkJh1JASpJTR+WhXAAZZHEZnFSWYbCeC";
    os << "tYGGsryQhKOWwh0JZGS3WMxt4Rhrg1TTPjJXmO0ZXJJL7ntdxvSZzE1Co1fD9gcLiI2yK4TZOYynyF4";
    os << "dWjBIUCSZS09EM0tu2QKYlU0uiOK0OenCmF1IokSsmkNGM5jLCFwMv7aG0de8jm5w0lYJqkLMyXOXpk";
    os << "hqVyjV0z6Y5bfnN4BwbgGhzvNrcFq1aKI3bmKT6ugAhPAihCT2A8RjrL8cDYD/SvGR9ef8nGvk96buq";
    os << "csnk1juWQSpP4KaVsSYqaJ1h+LsDxfFDgOMK4GC3rwMRDMoRrk2UpUSMmFOOVwUJUygdKBEEBDSRSks";
    os << "YSZVk0Gg0atZByPU0XktH9jSitDoW7zl9zyrg1MjZK1EiwPbAcF7elWZL3fSrzZSpLFTSaw4cX8Fdtm";
    os << "jt1nT/UrC7axVLB1EujyQcBwhE0mxGOLegbLJHGEWkkaTQSlJJGHAuMVSI0nu8iUxPMt42il6kesIxu";
    os << "i+26OLaDjFLiOMXxbDSaVGbYjk3UlGhLEbgOhSCP1x9IS4aOwkLGGoRFUCpSLzdQOiPNMtI4NvHaFrB";
    os << "liUSnKbbrGGvMgqHhfvJ9eRZmqlQWG5SjZvqLB2rbX/vSc8705UJe2DnDkIJCp5JmnBra/ZYcqJ/z0b";
    os << "bRDbE1SGnisUb8C8IoRqYKP3DQWUaxb4BVIwX8XGuSakDYhK2HZLYQJJkiboZEzRitNfm+HGki0Uq33";
    os << "H0Tp7ZtMxSV0niuAKkIk4jFhRq1OFc+MFOee8bpA5u3bF2HhUPUaCIcge/aKC1QFiYXE4swiomjBL+Q";
    os << "x8sHpDIlDSOyVKK1RaY1jmsRh4mxADELdHFogL5BoxkehSnCFahMmxw+rbASTdxS9bMci6geARo/H5j";
    os << "QUqZJM4VMJTYaqYx6oRCgJMYjyjnMT08jrVVyaMMZi809PxvN5SShlSdMctqSKikNFf2JsT5KRZ/h0Q";
    os << "HIYPLAUcIkQzk2w4N5Vq3qR6YQxuapchIapu4g75GpDJ1mZMpAiucKcvk8aG0FBZc4TJk8MslDO44gx";
    os << "XDlG7+8/+eJlO34YFvwvjuO2Mmd2AmIKU8gIJpf8CSAIiybptOOL7bzF4+50evGh0972lr/oyNDOTEy";
    os << "3NcCRUUYxmRSIjNNbOWkEIXUI/ECP7bzOVOGZ9utGJXKjOyagKgZ6d9MB/ds37Fv/k3PXfO8Yinv5It";
    os << "9ys0VsnI5bh6dna8rnbpRPfIOzCd7b7/v4H4A3/etF1z81NNGc5XT8oGbE45PKlOsLLM0YFuWblVsWK";
    os << "7nUV5csGaXwmx3JfjFBadvGKe8d/PwxBqKhbzl+65Vnp1RSZZma9etc6I4FM2lGqtGx3RYL2e1elMX+";
    os << "/scKbWVpKkk00LKRLiusSzSSCK1UfLTuuVOIcjlA+PiSUnLOzOJ1yojVRqjj5ARLS2QP/XSfV/59rYd";
    os << "r3/R+U9ZW5jfUBxca4dJZNm2TdxMkZmhXtNolDKxQscW+L6HyjLq9Sb1RhOEg+v5VMqRLNfCWk07kz+";
    os << "7a8++5194+rrLLt684eZ7du+7YPPaU9PFmUHba8nDOib+lcQJOjOutZQZYWTABWFSl2zbAVsQR0bUTt";
    os << "LIAAALPUlEQVQ6z3Ms/CCf1rw1h85bLzdhS8vPr6JeqRGGCY7jk6ZSzU3PimYS4wgHy9aGJMSyCMO0V";
    os << "fucmXz31gKaSIkjHAQQyRgZhdSS0YXv3XH/He9++fnPc/S8N7x6DVpBmmQEgYtl22itWzK4gqgZo4SJ";
    os << "RcZRSqPSRLRUGhv1BmkSG8nUNCNsRhRzLoW+8QwtLW0lol4PwXbI+04rLmuhsGg0Y4TKjM5K4AJCh80";
    os << "IN19IGmHcDBxRjKOGk8QKYTtWKecSNWpUa01s1/x+mUrSqmRW5rMlZ+C2w5OTjVf/7os2BYGjG/XY23";
    os << "bTz+Ym5xdnL7vk6adtHmhuKRYDK3A9RkZLhM0qS0tl0tyZVZ2VEY3ZvmLeY2Ckj2Y1JGqG+IUCllCE9";
    os << "cjkFdsOCk/e9dD83udddMqGZGEqyJUK1CsROx7cQ2nkzOirN9+5rdpotIGu+2lzdxyxfVzI8fmIj+uD";
    os << "lUdg1ZMFivCINJ3u/MVHxBe3rFtz7sZh571O1lCjIwPKcbQrhJPNl8NmxMB8/9Bwsc+RQ0QztuuBtgS";
    os << "Z1NqyrSxLYtwgr6XIV+u1arr9oZn9DxxcmgL0WZvXjGwYcYfv2jl3oFxPmlGStpLzjgngdNZYHrtAq0";
    os << "p+MZcPnExmlkVma6mEtESmFCrwLK+vVPLq9Zpaqib1eixjIYTl2QhLONZQXyEI8r44Oj0fpVLJibFV+";
    os << "dNPG1s7d2QqVYWJ+QOHDkbNRphNjAwWxoZLxQf3TM0Xc17wtM2jmzwVDzp+LkkyonJlyfIdy5upptMT";
    os << "E2uDseGR4r7DBxeUbCK0ZadpqtygxOqxiZIlYPLQbjvw8/aGDZv8u+7ftf/evUcn27/H9xx7y8a1pfN";
    os << "OH92UNWNnYWleqkyJIMgxMLYxPXzkYKNSXVK2JVSp2G+Pj4/3JSrTUimlUuwjR2YqOw5OHmrGMgG069";
    os << "pCZVplyhTBCiGsZ2wZ3bxxrG9930C/6znCD+sLrmO7Npa2bNuhXqkZKRlhI3SGtmzjgimF5efD+YXq0";
    os << "sBgyf7xXYe2z5cbjbf+Xy/c7NuBP1+PaotzB1TaaDhWMBzu2ndwseTI4ULOC9LMSlbl7fHi8Ej90JHp";
    os << "qiMcK1OZ0JkUaybGS1Gj4stMqiQOdZAvaNf1rbVjw6vKS/Px9++e+oVSSl12yTlrzn/KyIakETuxVDJ";
    os << "JImt+do7McoirNWtiYjgrjYxkYWXJCTM7zFvRGKlwFpvxfDOMkuHVa4SX66M6c8guFgpuobTaW1isp9";
    os << "W0eeT+PYdmCvmCGAh0X2NxtoDjJ7Zja1QiXNu21qzflNMyDeam9wstJaPjq9n+0NH9USLjxWbakDJTv";
    os << "ufYA4HdrxTZ+HBf36rR0WBk9Vrt2LZ2bM+ZmZmOBweH3LCylPvlvbv2zS8uJo7jWFLKzgmvLcvSWms9";
    os << "MTJYeuZFZ6/dvXNfefMp46vGhweKwgqdWx5Yuv+e+3aUzz9l5CkXPnXzxsCz3PritFAKoS1tCYW2bNv";
    os << "yAtdyLc3S0iLNZLh5oBE+eMmWvnPDesWtVOugbHX3UXvbQ4eOzGGsvs5YYqVrawPisnHEJwoQ4bcDin";
    os << "Di/MV2Ynf/xMjY1np5zndc1183ktsyU5Nzc+V6DMJ97WVbX2C7gbtQrc8Oj4wGu/Ydnp2aLVcqzShUa";
    os << "ZZlWFkYSyM28kht2O4yoeX20/HaSVUkul7peO1svS5w93En+rtzX69jT/omCiFami2tHLrH3rqvT6/r";
    os << "1T4/AOHatp3zRBDkXN8Rljs4MOBvGCpuSuqL3uj4aCD8vNx5YG5/pbqUKoU+NFubTTOVtT7ffV+6v/t";
    os << "EbTlGlfb+E/Wx4udNNaOwskwt109PgpST/O7l+uh1nXuNTZHL5YQQwsqyTGeZIQrIsuyYG6rMQtY5IH";
    os << "S7T9u2rSwzqoaeY7sC5RbyXmBpZSulVS7wnJxr56IkkUXfKmXNeNX+mrXd92yxesAdi5oNt0FhcrHaX";
    os << "FRKtUv3Gq2tysOWYvt9nd5xxKz9m/9/A4qwbHyxM3+xyMPA2N5KGMAsYKxKzxbC0yihFHa7646vaf+o";
    os << "5Zg2uovJVdcxvSbfsZ/Qce6dA285XrfHAord+1cCxOW+Y8UmhPjPgmL7e3tt3a17onayJnfu79V/t/V";
    os << "+ovvzaNpK1FO9AGel9yfTukHw0Z77iRaC48ak7/tOmqbYtm0rpawsy6zWfT8ZY+AYKHL82La6NtFjf/";
    os << "c5t3MJ2/XMEQ+7zu00nM5cxO44Ymf6zRMKiPAESZyu1DrkTxXmArZJIizMZAk5frLAwxdVYi6Unynl8";
    os << "DDpZOex8EgLsNeW8chB0b2ve+J1Tm6bR072ZX92x+d7tROBXvv9Shbpo2qPk5XYOUl7hR46F8BOQOwG";
    os << "xV5ced0L23Kv9HhttxNd78fSTrRQ9WorneejtRKX+2y3sSGEEK7JMcUWQogsy1BKtT0LzfHjvdd17ey";
    os << "719ZtEPS6Ju3vac/zmONBsQ2MndZhGzyf0NSb5dqTDorwCGCEhwEv4ZEXu32j2oCY43ithm5Q7JysvY";
    os << "Cw83WlbSVQXG5CdwPjSu5k9/vl/u4FjCc7Ef/T4LlMv+3X5Szy7tYNiCsBY2frtuB7Td6TtbpO1jo/U";
    os << "XssoAjLW3mPxmpcCVy74/XCsiyv9dre130OveZC9z3sBXiPcM+XOZaOftsUYJ2g2HahOwGxu6b5OFB8";
    os << "MtpvBRQ7WvfNSVr7O8GlEzDbAdo2KLYnVy8ro9dk6uZkW8mt7mWJdA68XgOjl+vQ+QorT6ZegLmcC/N";
    os << "Y26OxbFb6fy9LsZdL221d9wLGTqu78zu6rfjlJu9KbvSTDYbLjYHO948GzDs/291XL1A8dm2bzWava9";
    os << "1paHRf125Kru7ftBwg9gLGduvsu00J1p7HTR5+4NIJiMfVNLfP5cmwEuG3CIrLWIsWD7vSnZOhDYohJ";
    os << "vbYqei1EihmXf10T6xeLvSJYlbLDYrl4imdr3QdczITbKXJuNIE7O5npe/p1br7Ws5tW8517mzdoLic";
    os << "G71cCKRzsvbi0ns0k2Wl6/doPtv590r9rHTdHu25d36uu7/Oxbpb16T92u19dc6JXuStnX13jsWVDIL";
    os << "OY7uNnjZXYpsjMeraOmuan9AE7ZXab9VS7ADGzgt37N88coVxO7b2ze7ltsLyrtdKgNg9wdvn0dlOFh";
    os << "CP+6kdn+3VVrJmernPy/V1stZSr/+djHXY/b7b8umesN0TqbOqqZfr3G0pdlv83Qtd53En0x4vq7Hzs";
    os << "ytZ/Cdykx+LtbjcZzuvYScQdgJkp6XYDYbdrNbdY3YlUOzlYneeZ7cL3WkxJl1btxf3pAIi/Pbd53br";
    os << "jmO097VvXtu1boNht+5r583onCTdgLdSYHkli6d7gJzMYOj+bGc70eRcaaKdyDrpBeLLte7BezKtFyC";
    os << "2X5cDqO6FpHMxWyn80H1vei1eJwMsJwpVPJb2aKzvlazFXu+7+13puM6x2bnodFuJnR5VL1DsfN95fZ";
    os << "cL37QXsF5WYvt998LWthY7GfjbW6fswG/FQmy33zoo9rAW4ZGuU8bxN7cbEDtvxnLWSzcwLretNNm6b";
    os << "/qJAPHYz1zhmMey77FM6McKAr0G5XJWTy9Q7F48OkFwJUDs7rN7AVvOjTxRe6zX4WQXl17tZK/hidpy";
    os << "/XSDVa/Yrc3xc6QbGHt5T52t17hfaT50nl/3XO7ceoHhk/pgpbs96XmKy7Ueid2dN3g5t2sl13UlwFt";
    os << "pcp2MO7YcGD5WS3Gl/Y/m2JP5vkdr2ay0r/N/K03yXqC4nLXRq99uy7/X/x5tezxc55Ptaznr7j8z+Z";
    os << "YDrV5zptN1breTybzo1X/7fa85cCJQ7LQYez3oOQ6QfxtWIsD/BothfD9Va4q7AAAAAElFTkSuQmCC";
    os << ") 0px 0px no-repeat; }\n"
       << "\t\t\t#masthead h2 {margin-left: 355px;color: 268f45; }\n"
       << "\t\t\t#masthead ul.params {margin: 20px 0 0 345px; }\n"
       << "\t\t\t#masthead p {color: #fff;margin: 20px 20px 0 20px; }\n"
       << "\t\t\t#notice h2 {margin-bottom: 10px; }\n"
       << "\t\t\t.alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #333;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 0px 0px 6px #C11B17;-webkit-box-shadow: inset 0px 0px 6px #C11B17;box-shadow: inset 0px 0px 6px #C11B17; }\n"
       << "\t\t\t.alert p {margin-bottom: 0px; }\n"
       << "\t\t\t.section .toggle-content {padding-left: 18px; }\n"
       << "\t\t\t.player > .toggle-content {padding-left: 0; }\n"
       << "\t\t\t.toc {float: left;padding: 0; }\n"
       << "\t\t\t.toc-wide {width: 560px; }\n"
       << "\t\t\t.toc-narrow {width: 375px; }\n"
       << "\t\t\t.toc li {margin-bottom: 10px;list-style-type: none; }\n"
       << "\t\t\t.toc li ul {padding-left: 10px; }\n"
       << "\t\t\t.toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
       << "\t\t\t#raid-summary .toggle-content {padding-bottom: 0px; }\n"
       << "\t\t\t#raid-summary ul.params,#raid-summary ul.params li:first-child {margin-left: 0; }\n"
       << "\t\t\t.charts {float: left;width: 541px;margin-top: 10px; }\n"
       << "\t\t\t.charts-left {margin-right: 40px; }\n"
       << "\t\t\t.charts img {background-color: #333;padding: 5px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
       << "\t\t\t.talents div.float {width: auto;margin-right: 50px; }\n"
       << "\t\t\ttable.sc {background-color: #333;padding: 4px 2px 2px 2px;margin: 10px 0 20px 0;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
       << "\t\t\ttable.sc tr {color: #fff;background-color: #1a1a1a; }\n"
       << "\t\t\ttable.sc tr.head {background-color: #aaa;color: #fff; }\n"
       << "\t\t\ttable.sc tr.odd {background-color: #222; }\n"
       << "\t\t\ttable.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #333;color: #fff; }\n"
       << "\t\t\ttable.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
       << "\t\t\ttable.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; }\n"
       << "\t\t\ttable.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; }\n"
       << "\t\t\ttable.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
       << "\t\t\ttable.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
       << "\t\t\ttable.sc tr.details td {padding: 0 0 15px 15px;text-align: left;background-color: #333;font-size: 11px; }\n"
       << "\t\t\ttable.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
       << "\t\t\ttable.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #222; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
       << "\t\t\ttable.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
       << "\t\t\ttable.sc tr.details td div.float {width: 350px; }\n"
       << "\t\t\ttable.sc tr.details td div.float h5 {margin-top: 4px; }\n"
       << "\t\t\ttable.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
       << "\t\t\ttable.sc td.filler {background-color: #333; }\n"
       << "\t\t\ttable.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
       << "\t\t\ttr.details td table.details {padding: 0px;margin: 5px 0 10px 0; }\n"
       << "\t\t\ttr.details td table.details tr th {background-color: #222; }\n"
       << "\t\t\ttr.details td table.details tr td {background-color: #2d2d2d; }\n"
       << "\t\t\ttr.details td table.details tr.odd td {background-color: #292929; }\n"
       << "\t\t\ttr.details td table.details tr td {padding: 1px 3px 1px 3px; }\n"
       << "\t\t\ttr.details td table.details tr td.right {text-align: right; }\n"
       << "\t\t\t.player-thumbnail {float: right;margin: 8px;border-radius: 12px;-moz-border-radius: 12px;-khtml-border-radius: 12px;-webkit-border-radius: 12px; }\n"
       << "\t\t</style>\n";
  }
}

// print_html_masthead ====================================================

void print_html_masthead( report::sc_html_stream& os, sim_t* sim )
{
  // Begin masthead section
  os << "\t\t<div id=\"masthead\" class=\"section section-open\">\n\n";

  os.printf(
    "\t\t\t<span id=\"logo\"></span>\n"
    "\t\t\t<h1><a href=\"%s\">SimulationCraft %s-%s</a></h1>\n"
    "\t\t\t<h2>for World of Warcraft %s %s (build level %s)</h2>\n\n",
    ( sim -> hosted_html ) ? "http://www.simulationcraft.org/" : "http://code.google.com/p/simulationcraft/",
    SC_MAJOR_VERSION, SC_MINOR_VERSION, dbc_t::wow_version( sim -> dbc.ptr ), ( sim -> dbc.ptr ?
#if SC_BETA
    "BETA"
#else
    "PTR"
#endif
    : "Live" ), util::to_string( dbc_t::build_level( sim -> dbc.ptr ) ).c_str() );

  time_t rawtime;
  time ( &rawtime );

  os << "\t\t\t<ul class=\"params\">\n";
  os.printf(
    "\t\t\t\t<li><b>Timestamp:</b> %s</li>\n",
    ctime( &rawtime ) );
  os.printf(
    "\t\t\t\t<li><b>Iterations:</b> %d</li>\n",
    sim -> iterations );

  if ( sim -> vary_combat_length > 0.0 )
  {
    timespan_t min_length = sim -> max_time * ( 1 - sim -> vary_combat_length );
    timespan_t max_length = sim -> max_time * ( 1 + sim -> vary_combat_length );
    os.printf(
      "\t\t\t\t<li class=\"linked\"><a href=\"#help-fight-length\" class=\"help\"><b>Fight Length:</b> %.0f - %.0f</a></li>\n",
      min_length.total_seconds(),
      max_length.total_seconds() );
  }
  else
  {
    os.printf(
      "\t\t\t\t<li><b>Fight Length:</b> %.0f</li>\n",
      sim -> max_time.total_seconds() );
  }
  os.printf(
    "\t\t\t\t<li><b>Fight Style:</b> %s</li>\n",
    sim -> fight_style.c_str() );
  os << "\t\t\t</ul>\n"
     << "\t\t\t<div class=\"clear\"></div>\n\n"
     << "\t\t</div>\n\n";
  // End masthead section
}

void print_html_errors( report::sc_html_stream& os, sim_t* sim )
{
  if ( ! sim -> error_list.empty() )
  {
    os << "\t\t<pre style=\"color: black; background-color: white; font-weight: bold;\">\n";
    size_t num_errors = sim -> error_list.size();
    for ( size_t i=0; i < num_errors; i++ )
      os <<  sim -> error_list[ i ] << "\n";

    os << "\t\t</pre>\n\n";
  }
}

void print_html_beta_warning( report::sc_html_stream& /* os */ )
{
#if SC_BETA
  os << "\t\t<div id=\"notice\" class=\"section section-open\">\n"
     << "\t\t\t<h2>Beta Release</h2>\n"
     << "\t\t\t<ul>\n";

  for ( size_t i = 0; i < sizeof_array( report::beta_warnings ); ++i )
    os << "\t\t\t\t<li>" << report::beta_warnings[ i ] << "</li>\n";

  os << "\t\t\t</ul>\n"
     << "\t\t</div>\n\n";
#endif
}

void print_html_image_load_scripts( report::sc_html_stream& os, sim_t* sim )
{
  // Toggles, image load-on-demand, etc. Load from simulationcraft.org if
  // hosted_html=1, otherwise embed
  if ( sim -> hosted_html )
  {
    os << "\t\t<script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/ga.js\"></script>\n"
       << "\t\t<script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/rep.js\"></script>\n"
       << "\t\t<script type=\"text/javascript\" src=\"http://static.wowhead.com/widgets/power.js\"></script>\n";
  }
  else
  {
    os << "\t\t<script type=\"text/javascript\">\n"
       << "\t\t\tfunction load_images(containers) {\n"
       << "\t\t\t\tvar $j = jQuery.noConflict();\n"
       << "\t\t\t\tcontainers.each(function() {\n"
       << "\t\t\t\t\t$j(this).children('span').each(function() {\n"
       << "\t\t\t\t\t\tvar j = jQuery.noConflict();\n"
       << "\t\t\t\t\t\timg_class = $j(this).attr('class');\n"
       << "\t\t\t\t\t\timg_alt = $j(this).attr('title');\n"
       << "\t\t\t\t\t\timg_src = $j(this).html().replace(/&amp;/g, '&');\n"
       << "\t\t\t\t\t\tvar img = new Image();\n"
       << "\t\t\t\t\t\t$j(img).attr('class', img_class);\n"
       << "\t\t\t\t\t\t$j(img).attr('src', img_src);\n"
       << "\t\t\t\t\t\t$j(img).attr('alt', img_alt);\n"
       << "\t\t\t\t\t\t$j(this).replaceWith(img);\n"
       << "\t\t\t\t\t\t$j(this).load();\n"
       << "\t\t\t\t\t});\n"
       << "\t\t\t\t});\n"
       << "\t\t\t}\n"
       << "\t\t\tfunction open_anchor(anchor) {\n"
       << "\t\t\t\tvar img_id = '';\n"
       << "\t\t\t\tvar src = '';\n"
       << "\t\t\t\tvar target = '';\n"
       << "\t\t\t\tanchor.addClass('open');\n"
       << "\t\t\t\tvar section = anchor.parent('.section');\n"
       << "\t\t\t\tsection.addClass('section-open');\n"
       << "\t\t\t\tsection.removeClass('grouped-first');\n"
       << "\t\t\t\tsection.removeClass('grouped-last');\n"
       << "\t\t\t\tif (!(section.next().hasClass('section-open'))) {\n"
       << "\t\t\t\t\tsection.next().addClass('grouped-first');\n"
       << "\t\t\t\t}\n"
       << "\t\t\t\tif (!(section.prev().hasClass('section-open'))) {\n"
       << "\t\t\t\t\tsection.prev().addClass('grouped-last');\n"
       << "\t\t\t\t}\n"
       << "\t\t\t\tanchor.next('.toggle-content').show(150);\n"
       << "\t\t\t\tchart_containers = anchor.next('.toggle-content').find('.charts');\n"
       << "\t\t\t\tload_images(chart_containers);\n"
       << "\t\t\t\tsetTimeout(\"var ypos=0;var e=document.getElementById('\" + section.attr('id') + \"');while( e != null ) {ypos += e.offsetTop;e = e.offsetParent;}window.scrollTo(0,ypos);\", 500);\n"
       << "\t\t\t}\n"
       << "\t\t\tjQuery.noConflict();\n"
       << "\t\t\tjQuery(document).ready(function($) {\n"
       << "\t\t\t\tvar chart_containers = false;\n"
       << "\t\t\t\tvar anchor_check = document.location.href.split('#');\n"
       << "\t\t\t\tif (anchor_check.length > 1) {\n"
       << "\t\t\t\t\tvar anchor = anchor_check[anchor_check.length - 1];\n"
       << "\t\t\t\t}\n"
       << "\t\t\t\t$('a.ext').mouseover(function() {\n"
       << "\t\t\t\t\t$(this).attr('target', '_blank');\n"
       << "\t\t\t\t});\n"
       << "\t\t\t\t$('.toggle').click(function(e) {\n"
       << "\t\t\t\t\tvar img_id = '';\n"
       << "\t\t\t\t\tvar src = '';\n"
       << "\t\t\t\t\tvar target = '';\n"
       << "\t\t\t\t\te.preventDefault();\n"
       << "\t\t\t\t\t$(this).toggleClass('open');\n"
       << "\t\t\t\t\tvar section = $(this).parent('.section');\n"
       << "\t\t\t\t\tif (section.attr('id') != 'masthead') {\n"
       << "\t\t\t\t\t\tsection.toggleClass('section-open');\n"
       << "\t\t\t\t\t}\n"
       << "\t\t\t\t\tif (section.attr('id') != 'masthead' && section.hasClass('section-open')) {\n"
       << "\t\t\t\t\t\tsection.removeClass('grouped-first');\n"
       << "\t\t\t\t\t\tsection.removeClass('grouped-last');\n"
       << "\t\t\t\t\t\tif (!(section.next().hasClass('section-open'))) {\n"
       << "\t\t\t\t\t\t\tsection.next().addClass('grouped-first');\n"
       << "\t\t\t\t\t\t}\n"
       << "\t\t\t\t\t\tif (!(section.prev().hasClass('section-open'))) {\n"
       << "\t\t\t\t\t\t\tsection.prev().addClass('grouped-last');\n"
       << "\t\t\t\t\t\t}\n"
       << "\t\t\t\t\t} else if (section.attr('id') != 'masthead') {\n"
       << "\t\t\t\t\t\tif (section.hasClass('final') || section.next().hasClass('section-open')) {\n"
       << "\t\t\t\t\t\t\tsection.addClass('grouped-last');\n"
       << "\t\t\t\t\t\t} else {\n"
       << "\t\t\t\t\t\t\tsection.next().removeClass('grouped-first');\n"
       << "\t\t\t\t\t\t}\n"
       << "\t\t\t\t\t\tif (section.prev().hasClass('section-open')) {\n"
       << "\t\t\t\t\t\t\tsection.addClass('grouped-first');\n"
       << "\t\t\t\t\t\t} else {\n"
       << "\t\t\t\t\t\t\tsection.prev().removeClass('grouped-last');\n"
       << "\t\t\t\t\t\t}\n"
       << "\t\t\t\t\t}\n"
       << "\t\t\t\t\t$(this).next('.toggle-content').toggle(150);\n"
       << "\t\t\t\t\t$(this).prev('.toggle-thumbnail').toggleClass('hide');\n"
       << "\t\t\t\t\tchart_containers = $(this).next('.toggle-content').find('.charts');\n"
       << "\t\t\t\t\tload_images(chart_containers);\n"
       << "\t\t\t\t});\n"
       << "\t\t\t\t$('.toggle-details').click(function(e) {\n"
       << "\t\t\t\t\te.preventDefault();\n"
       << "\t\t\t\t\t$(this).toggleClass('open');\n"
       << "\t\t\t\t\t$(this).parents().next('.details').toggleClass('hide');\n"
       << "\t\t\t\t});\n"
       << "\t\t\t\t$('.toggle-db-details').click(function(e) {\n"
       << "\t\t\t\t\te.preventDefault();\n"
       << "\t\t\t\t\t$(this).toggleClass('open');\n"
       << "\t\t\t\t\t$(this).parent().next('.toggle-content').toggle(150);\n"
       << "\t\t\t\t});\n"
       << "\t\t\t\t$('.help').click(function(e) {\n"
       << "\t\t\t\t\te.preventDefault();\n"
       << "\t\t\t\t\tvar target = $(this).attr('href') + ' .help-box';\n"
       << "\t\t\t\t\tvar content = $(target).html();\n"
       << "\t\t\t\t\t$('#active-help-dynamic .help-box').html(content);\n"
       << "\t\t\t\t\t$('#active-help .help-box').show();\n"
       << "\t\t\t\t\tvar t = e.pageY - 20;\n"
       << "\t\t\t\t\tvar l = e.pageX - 20;\n"
       << "\t\t\t\t\t$('#active-help').css({top:t,left:l});\n"
       << "\t\t\t\t\t$('#active-help').show(250);\n"
       << "\t\t\t\t});\n"
       << "\t\t\t\t$('#active-help a.close').click(function(e) {\n"
       << "\t\t\t\t\te.preventDefault();\n"
       << "\t\t\t\t\t$('#active-help').toggle(250);\n"
       << "\t\t\t\t});\n"
       << "\t\t\t\tif (anchor) {\n"
       << "\t\t\t\t\tanchor = '#' + anchor;\n"
       << "\t\t\t\t\ttarget = $(anchor).children('h2:first');\n"
       << "\t\t\t\t\topen_anchor(target);\n"
       << "\t\t\t\t}\n"
       << "\t\t\t\t$('ul.toc li a').click(function(e) {\n"
       << "\t\t\t\t\tanchor = $(this).attr('href');\n"
       << "\t\t\t\t\ttarget = $(anchor).children('h2:first');\n"
       << "\t\t\t\t\topen_anchor(target);\n"
       << "\t\t\t\t});\n"
       << "\t\t\t});\n"
       << "\t\t</script>\n\n";
  }
}


// print_html =====================================================
void print_html_( report::sc_html_stream& os, sim_t* sim )
{
  os << "<!DOCTYPE html>\n\n";
  os << "<html>\n";

  os << "\t<head>\n";
  os << "\t\t<title>Simulationcraft Results</title>\n";
  os << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n";

  print_html_styles( os, sim );

  os.printf( "\t\t<script type=\"text/javascript\">\n"
             "\t\tfunction HighlightText(obj){\n"
             "\t\t\tif (document.selection) {\n"
             "\t\t\t\tvar r1 = document.body.createTextRange();\n"
             "\t\t\t\tr1.moveToElementText(document.getElementById(obj));\n"
             "\t\t\t\tr1.select();\n"
             "\t\t\t} else {\n"
             "\t\t\t\tvar s = window.getSelection();\n"
             "\t\t\t\tvar r1 = document.createRange();\n"
             "\t\t\t\tr1.setStartBefore(document.getElementById(obj));\n"
             "\t\t\t\tr1.setEndAfter(document.getElementById(obj));\n"
             "\t\t\t\ts.addRange(r1);\n"
             "\t\t\t}\n"
             "\t\t}\n"
             "\t\t</script>\n" );

  os << "\t</head>\n\n";

  os << "\t<body>\n";

  print_html_errors( os, sim );

  // Prints div wrappers for help popups
  os << "\t\t<div id=\"active-help\">\n"
     << "\t\t\t<div id=\"active-help-dynamic\">\n"
     << "\t\t\t\t<div class=\"help-box\"></div>\n"
     << "\t\t\t\t<a href=\"#\" class=\"close\"><span class=\"hide\">close</span></a>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n\n";

  print_html_masthead( os, sim );

  print_html_beta_warning( os );

  size_t num_players = sim -> players_by_name.size();

  if ( num_players > 1 || sim -> report_raid_summary )
  {
    print_html_contents( os, sim );
  }

  if ( num_players > 1 || sim -> report_raid_summary )
  {
    print_html_raid_summary( os, sim, sim -> report_information );
    print_html_scale_factors( os, sim );
  }

  // Report Players
  for ( size_t i = 0; i < num_players; ++i )
  {
    report::print_html_player( os, sim -> players_by_name[ i ], i );

    // Pets
    if ( sim -> report_pets_separately )
    {
      std::vector<pet_t*>& pl = sim -> players_by_name[ i ] -> pet_list;
      for ( size_t i = 0; i < pl.size(); ++i )
      {
        pet_t* pet = pl[ i ];
        if ( pet -> summoned && !pet -> quiet )
          report::print_html_player( os, pet, 1 );
      }
    }
  }

  // Sim Summary
  print_html_sim_summary( os, sim, sim -> report_information );

  if ( sim -> report_raw_abilities )
    print_html_raw_ability_summary( os, sim );

  // Report Targets
  if ( sim -> report_targets )
  {
    for ( size_t i = 0; i < sim -> targets_by_name.size(); ++i )
    {
      report::print_html_player( os, sim -> targets_by_name[ i ], i );

      // Pets
      if ( sim -> report_pets_separately )
      {
        std::vector<pet_t*>& pl = sim -> targets_by_name[ i ] -> pet_list;
        for ( size_t i = 0; i < pl.size(); ++i )
        {
          pet_t* pet = pl[ i ];
          //if ( pet -> summoned )
          report::print_html_player( os, pet, 1 );
        }
      }
    }
  }

  // Help Boxes
  print_html_help_boxes( os, sim );

  // jQuery
  // The /1/ url auto-updates to the latest minified version
  os << "\t\t<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1/jquery.min.js\"></script>\n";

  print_html_image_load_scripts( os, sim );

  if ( num_players > 1 )
    print_html_raid_imagemaps( os, sim, sim -> report_information );

  os << "\t</body>\n\n"
     << "</html>\n";
}

} // UNNAMED NAMESPACE ====================================================

namespace report {

void print_html( sim_t* sim )
{
  if ( sim -> simulation_length.mean == 0 ) return;
  if ( sim -> html_file_str.empty() ) return;


  // Setup file stream and open file
  report::sc_html_stream s;
  s.open( sim, sim -> html_file_str );
  if ( ! s )
    return;

  report::generate_sim_report_information( sim, sim -> report_information );

  // Set floating point formatting
  s.precision( sim -> report_precision );
  s << std::fixed;

  // Print html report
  print_html_( s, sim );

  // Close file
  s.close();
}

} // END report NAMESPACE

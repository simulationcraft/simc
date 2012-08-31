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
        os << "\t\t\t\t\t<li><a href=\"#" << p -> name() << "\">" << p -> name() << "</a>";
        ci++;
        if ( sim -> report_pets_separately )
        {
          os << "\n\t\t\t\t\t\t<ul>\n";
          for ( size_t k = 0; k < sim -> players_by_name[ pi ] -> pet_list.size(); ++k )
          {
            pet_t* pet = sim -> players_by_name[ pi ] -> pet_list[ k ];
            if ( pet -> summoned )
            {
              os << "\t\t\t\t\t\t\t<li><a href=\"#" << pet -> name() << "\">" << pet -> name() << "</a></li>\n";
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
          os << "\t\t\t\t\t\t<li><a href=\"#auras-buffs\">Auras/Buffs</a></li>\n";
          ab = 1;
        }
        ci++;
        os << "\t\t\t\t\t\t<li><a href=\"#sim-info\">Simulation Information</a></li>\n";
        ci++;
        if ( sim -> report_raw_abilities )
        {
          os << "\t\t\t\t\t\t<li><a href=\"#raw-abilities\">Raw Ability Summary</a></li>\n";
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
            os << "\t\t\t\t\t<li><a href=\"#" << p -> name() << "\">" << p -> name() << "</a></li>\n";
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
     << "\t\t\t\t\t\t\t\t<td>" << sim -> confidence * 100.0 << "</td>\n"
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

  report::print_html_rng_information( os, sim -> default_rng(), sim -> confidence_estimator );

  report::print_html_sample_data( os, sim, sim -> simulation_length, "Simulation Length" );

  os << "\t\t\t\t\t\t</table>\n";

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
  std::string downtime_chart = chart::raid_downtime( sim -> players_by_name, sim -> print_styles );
  if ( !  downtime_chart.empty() )
  {
    os.printf(
      "\t\t\t\t\t<img src=\"%s\" alt=\"Raid Downtime Chart\" />\n",
      downtime_chart.c_str() );
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

void print_html_raw_action_damage( report::sc_html_stream& os, stats_t* s, player_t* /* p */, int j )
{
  int id = 0;

  os << "\t\t\t<tr";
  if ( j & 1 )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";

  for ( size_t i = 0; i < s -> player -> action_list.size(); ++i )
  {
    action_t* a = s -> player -> action_list[ i ];
    if ( a -> stats != s ) continue;
    id = a -> id;
    if ( ! a -> background ) break;
  }

  os.printf(
    "\t\t\t\t\t<td class=\"left  small\">%s</td>\n"
    "\t\t\t\t\t<td class=\"left  small\">%s</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%d</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.2fsec</td>\n"
    "\t\t\t\t\t<td class=\"right small\">%.2fsec</td>\n"
    "\t\t\t\t</tr>\n",
    s -> player -> name(),
    s -> name_str.c_str(),
    id,
    s -> total_amount.mean,
    s -> direct_results[ RESULT_HIT  ].actual_amount.mean,
    s -> direct_results[ RESULT_CRIT ].actual_amount.mean,
    s -> num_executes,
    s -> direct_results[ RESULT_CRIT ].pct,
    s -> direct_results[ RESULT_MISS ].pct + s -> direct_results[ RESULT_DODGE  ].pct + s -> direct_results[ RESULT_PARRY  ].pct,
    s -> direct_results[ RESULT_GLANCE ].pct,
    s -> direct_results[ RESULT_BLOCK  ].pct,
    s -> num_ticks,
    s -> tick_results[ RESULT_HIT  ].actual_amount.mean,
    s -> tick_results[ RESULT_CRIT ].actual_amount.mean,
    s -> tick_results[ RESULT_CRIT ].pct,
    s -> tick_results[ RESULT_MISS ].pct + s -> tick_results[ RESULT_DODGE ].pct + s -> tick_results[ RESULT_PARRY ].pct,
    s -> total_intervals.mean,
    s -> player -> fight_length.mean );
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
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ability\" class=\"help\">Ability</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-id\" class=\"help\">Id</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-total\" class=\"help\">Total</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-miss-pct\" class=\"help\">Avoid%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-glance-pct\" class=\"help\">G%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-block-pct\" class=\"help\">B%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks\" class=\"help\">Ticks</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-hit\" class=\"help\">T-Hit</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-crit\" class=\"help\">T-Crit</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-crit-pct\" class=\"help\">T-Crit%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-miss-pct\" class=\"help\">T-Avoid%</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
     << "\t\t\t\t\t<th class=\"small\"><a href=\"#help-duration\" class=\"help\">Duration</a></th>\n"
     << "\t\t\t\t</tr>\n";

  int count = 0;
  for ( size_t player_i = 0; player_i < sim -> players_by_name.size(); player_i++ )
  {
    player_t* p = sim -> players_by_name[ player_i ];
    for ( size_t i = 0; i < p -> stats_list.size(); ++i )
    {
      stats_t* s = p -> stats_list[ i ];
      if ( s -> num_executes > 1 || s -> compound_amount > 0 || sim -> debug )
      {
        print_html_raw_action_damage( os, s, p, count++ );
      }
    }

    for ( size_t pet_i = 0; pet_i < p -> pet_list.size(); ++pet_i )
    {
      pet_t* pet = p -> pet_list[ pet_i ];
      for ( size_t i = 0; i < pet -> stats_list.size(); ++i )
      {
        stats_t* s = pet -> stats_list[ i ];
        if ( s -> num_executes || s -> compound_amount > 0 || sim -> debug )
        {
          print_html_raw_action_damage( os, s, pet, count++ );
        }
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
        p -> name(),
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
    os << "\"" << player_list[i] -> name() << "\"";
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

  os << "\t\t<div id=\"help-crit\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Crit</h3>\n"
     << "\t\t\t\t<p>Average crit damage.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-crit-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Crit%%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in critical strikes.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-dodge-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>Dodge%%</h3>\n"
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
     << "\t\t\t\t<h3>DPS%%</h3>\n"
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
     << "\t\t\t\t<h3>G%%</h3>\n"
     << "\t\t\t\t<p>Percentage of executes that resulted in glancing blows.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-block-pct\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>G%%</h3>\n"
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
     << "\t\t\t\t<h3>M%%</h3>\n"
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
     << "\t\t\t\t<h3>Parry%%</h3>\n"
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
     << "\t\t\t\t<p>DPS gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>DPS loss</b> per unit stat <b>decrease</b>.</p>\n"
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
     << "\t\t\t\t<h3>T-Crit%%</h3>\n"
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
     << "\t\t\t\t<h3>T-M%%</h3>\n"
     << "\t\t\t\t<p>Percentage of ticks that resulted in misses, dodges or parries.</p>\n"
     << "\t\t\t</div>\n"
     << "\t\t</div>\n";

  os << "\t\t<div id=\"help-ticks-uptime\">\n"
     << "\t\t\t<div class=\"help-box\">\n"
     << "\t\t\t\t<h3>UpTime%%</h3>\n"
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
       << "\t\t  @import url('http://www.simulationcraft.org/css/styles-print.css');\n"
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
       << "\t\t\tbody {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background: #339999;color: #FFF;text-align: center; }\n"
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
       << "\t\t\t.section {position: relative;width: 1150px;padding: 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;border: 0;-moz-box-shadow: 0px 0px 8px #FDD017;-webkit-box-shadow: 0px 0px 8px #FDD017;box-shadow: 0px 0px 8px #FDD017;color: #fff;background-color: #663300;text-align: left; }\n"
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
       << "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"
       << "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"
       << "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"
       << "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"
       << "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"
       << "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"
       << "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"
       << "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"
       << "Jb3v7cS6b6F3sPvnpr1937UVVYrUp9MwMEQwCaBigCkQEP/7xteec8/+0d/c1Z6eQozGqPjn8pc+ctXjZvj+4+krtt7ch3nH3zduGJ66+8pIbfn7MjtHaJz/12fXbtt7xpx+3QX3r0Ktf/fLZs4mpKWI07rHPQYQw59Uplfc9vM8Rv/j1zUH0XEeV/PSmy5974bhrvvlFdC8XWUYoVUa95c1HL1oyUDRGmSBOeOGhOQWNVIuoGsWR1ppLqS2uPPiQKO564+HHaaUpCGfyqentv/nV3c+vvS/umPzoOZ/6469vLLKa0XrvPQZuv/W6R9es+9alF9bS4clR+OjHzvra58/dsen5PE85l87aZr1mnNWGGQAz7WVEk2Lkrj89eMIRB+Rp4il1COiQCuojB+BNoYIw4KHggudJ3tq9bo21RgvBmOR5rtA6pazxJLfAw5JJodC5/y+dFXu96qvQYS0zfbGIApBBAIitPdVCMg9AEUptZWcd46xcisKAl6olzhlaBPDOOJVklDLvgXhkUlAmgjDkQpYq7c+se3V0qnn2WV986rm100lNSOa0f/7ljeu37Tju2PcsXLBI+ehHv7hrYmJm46bNM5M7HSFae92IPvCBf+G+qYrUGo3GWKOnp0YOOmjVpdf+zJnCAg2D9ncf9ZaxbRsoJYRQzhjItlvv+JMhDRZCmqVPPfV0kvie7vaIFUsX95YCH0pKwLf3zr/rz6u/fuEXzz7z/W898vBtE43nXlyT5HDmx8/99iVfec/xRy/fY9+JSb1sycCigQ5ENzar7n1kdT2bDiJoZrUXX1qfZHZBf09Is4NX7m2KZntVqqJgQlBCg1IYl9qt7P3ej3799NN/PWDlstWrn3l2zZNrnnu2kW3rnzd/Ynxqy47hWq1Zm97Z2xErrVSSA3ignnFmi+Td7zt97UtrZ2tbpQQGUK5UTz35hIP2WZAruO7G360bfNo6awE2b5o4YL/9d1/UXq52rV2//fd//HWSZ3HQWbgcGRhntm2b/aeTT0wb04xSxhljbGZq9JiTTlYuWP3kw1Ozvq0bNg0Nrt8wvGKv/Zfts+/YyPatIzs+fNpp6fRwT2dHsz771yeeefTxNdf99Pqf3/yDW35zw6bNW1g+efK73nr4YUf+5emXd0yPxxU2NDycFrj/3vu2l4MoKvUt3H1ovHbD729mzDVz9qaDjnn6hUEqUcrq4YcfHgm0WjNKLYGhwR0PPPLs5y8+9467brjlFz95dd36D77/pA+ecc4DDzw2Njk4NjFZb9B3HXtoszbt0KItSm29DzyyZnhoh9HQ1tV7xMErksZUK/9IBpGG6Nbbb39hw4txNbLIHbFckkcee+7EE06WmBitrDat1QcZCkCvC+PBZ/UUgHDBuBQARGtDKeVCKqWa9TTPVZ4Vea6yvGjWs8HRZP1YovR/5Xibvn7/Pydptq2OtYYttEZsZZ8SQhkhVBurCkUIOOcQkXPmjAUPiA4RKacyCkUk42oclEqEkCAKKKcekUsJGEzPNo44YtUnP/aZfXY7NGli4UilDTZte+5bV/341e3ppy667Ps//tkRR7+tnjkbExtxI0BV80dWry6V2pw1rd1gjHPG2OTEjgMPONC4oFDOFNCqoG7FSzLObTb9s+suf+NBb29mcrJpn37prxdf9Z1LfvCbDduzvZctmt9bFZwxxms7h7/2+Y8cecheG557dNvQq+M7ZwoECGHT4OaRjS9vfuGJvReXv/yp00Nm8rSZpumC3sqXPvOpw1a9tZ5APXUPP/nnr176zUt/fPvGkSwAt3heW5amMgpFGACl1rj23gVfvfzaBx995IdXXX7i8R+YmYETTzr5He84sZ77Y9/x3mOPfOMJx7z5pGMPOfQNy7QuvEcecGuNNdpZ45ypj27ykhce0gxqNeBBx4L5Pf0Llz7411dv/O0thSlqGWQ6uvhrF5x07KF5riod87ZOzGwbmZytwRcuuKTSttQRIkP400P3jNV8V98CLlpBOZILmY1u7evubuZ4wvEnj+yEuA3+7en7L/7u1fXZrKdncVZY7TQXPM9Sa80Rh60896Mf2nOffZombxr9zmOOP/H448aG17XLxne/fuFui1c2C0dFcfc9d7+4eWSy7h5/YfD2h9d88/uXe6Iyj/N697n029fvve/K3MFjjz/6zAuvlqu9MgyYFLGURx52wCc/+YnR0cKSom7VQW86dOGyVXff/qsNW9fmxhMOUzOTnHKP6B0arYhTvX0dJAItwXNP/5ZnQAht6+zdvnNm7StrGw140xtOOOldH1Fa5KlXMPODG27ZY5+DCUAQSM45ZUxnCtHJOBCBQGOtMkAgbSRKa49IKHHeeQ9aW2MdAmhljLJjU+kLQxNZnv3Xio6+rvPbQ2OTG3bqqWk1Oduw1juLRjvvfBiHMpRhFLaCRAgBq433nnMGHvJ6wgRvvdIKgbTKABAmBFrU1o2M7hwf2XzGPx9z6sknJ7m3xBeeWCSPPvnAeRee8/s77j73vE8cfODe2pnCgvJEJT4riieeeTYIyk5btI5JTjn13hMC07MNpV2eQtZ0xgHnnHLCBDPWLFm6xHtzxntPuOIb3+/vWlpTQIL89vt++9mvfXPzOAaldu/RGju7c/SAZT3bN69jXARhICNhBRQaDtj/gNZUc31qXNjagr5Ks9kEoAsXDCzo6/jUh8743LkXdHct1hao1DffefOFl127ZuNEW1uXzhVai9YCgEfPg/je1Y8dtuqQFQccNTy6DQJYeeCbq11d9QIOXrnPfrv3VWm+fXD91Ni41so5i4AI6JzThUJElSXVUmfSBONAE+BxKTVww22PXH/zrdO1fNtm4LTzqm9dedLbV45sfaFWT6/+yc13/PGupoJDDz3kg6d//KyPfHqqxnIkxjcu+sZl7X27yzBkXBIg6FAG0fatW3bW/BWXXbvXbitHx4FIePTpuz77+c9PTDb33GMFp5wyTghQSooim57Z2cyU8kAdr8rIqJQx3mhMr1jUedYZH5sYB+fJRH3y4Sef53F1Xn/fvN6lq59YW2R2yw6YtSPnX/DFzkrVEZgs0nseeWh8cpYyziiz1qaNpByLgQXtzRyqXRXB+S4pGLIAAB9USURBVC9vueHCb31janaskcOXzv/WxZ/76NjYNsoookNnnLO5MooRx6GRpugJEEREi/invzz2wxtuHBwatBpOPeVD37rkyrg8z4aUBOSBJx5Zt2W2q3tea/rdFcYUGq23WlvjKt3VsC0uUpXUsiItdKGMNlYZpYxzTltrjbXGTU431w43E/Nfr7jXV8/gcct47fFXp5sFaGM9pejRoHPeG2OttYEMGGcePQHQuSpyDZQEpRIAcCEIJUYrShkhlDEuhAAqLIUkLwqj6tM7Tj7xnYx0ag8WwDnaoLXHn1/3TyeceOY/nyQiuXRgkXFEa2c5NYSMTe9E8IxzoLS1ZQgIUApZoZvWagrTRbp9eBv1BJ0rldqW7/eWH976l0uv+OHKfXc/ZM+uG7/33Y+d9rGJGpS74IWh9Vf8/EZe7hVBrIrCadNoNMB7dFiulLt6evMCTAF5URBCRMAJo9YU1uj2ju4Fy1Ze/uN7fvmH+yuBOunw/W/54VWrDjxqZBrK7bB245of3HSnZhUPYLW1ShMKjFM0+buPett9jzz2kbNP+87VVx995CFHH7Zy+9hWnUI5ZJMTw7OzU+BRFUVaa6aNZt5MrTKIqPLc5Gp6cvSN+x1Yq4P14DxYYgoFz6xdn6TmtFNO+vmPvvOr6y89eI/2xvR2bUxnR8eihUs2D281OWzfPrHqkFXX/fz6WmENIQbx/tX3/vSmu+ct2pOAp4xLKR2a7v5e76CR1H72o5/ut2LlVB2IhD/86bd3P/TrydlJD0g8cCkJoVzItnJ4yBtWZgVk2iqnADwhpFRqq5vSty77XtxG0twHUfe+y3cr0Wzl3kuHh4eSXKc1+dPvXVMtL7719795ct2TPKLlTnj4yYd2zKZChrsslikYa5LZwnrIM+0cuf2Pf3ppeNAI6Cz1fva8T3s16YxqxXojuoAzAy43noWwddMwAtV5Dh6Jc7stXCpK1dlZ39FV/eLX/vUt7zxqaHLYElJoP52Of+kbl3cN7N561HpEyhkBEFJywRhnVmljTFiJwHsqOBfct8xjGcubmTGumTRe3F7bWc9fD8G9znoGcM5NN4vBSTOzczpJMmswDAWlxBQGnAfim/UmocyDB0RKSZHmDi2A55Ib5RgXHp2MQwKE8SDL3PjsVFgRUdDhLPp88prLvzM1CAa9pTDbMG897MgrLvlaWWYAqtpdNZwbJmwkLaOluCqDGBhhjHHOCWXEe4r+6Le9pdkA64hnhHBulJGE1Sw/8SPnf/2nV+2crFeqldnJ4TbZPOOkd5x6zKljI1DthTt+de+ddz2iraOcUUkRnTbGWgvWEes9AESwbccYEG610blqFRcOjTePO/28i6+6OkNLCDand/B0+KbvX3rofiszC6wCazY/+fvbH+rv67fotNLgPKKfHBm6+uIvH3v4oQ898MAH//mU6y+/yJnmXx9d+/4TT6lyLLIkb6TNesM5Y4xRWeHQAgOrdeuT1NHdv/qZp6UEg4AhNJMEdPKls0+555bvnf+hEw5cEvaWdG1m1GpDvGOUZkqPbJ6+4Pzz/vynB778r+f9y/tOfeuBq3KDxkBh09/ec2dq4nJbJ2UMKLPW7bnnckhgx/jobr3sh1de3hfP2zkJpOKffGHt4uWLAsY8JXRXSZZXCjdtHrMIVoLzNAjKnEed8/c/5n2nbU+GlPczO+CUtx138tEHzk7u0Ibedt+9te3Jr2/55dsPXHzz5Zd++PTTdm7VWhDn2fCOqcHxmow60HlrLBDChISAGg/TMypNk5/88Noj3vgmE/IdydRZnzyzVO1RudJ54b3LsqSnq3Pp/CUIkKfAWOSsRocsFCIQcVzaNPTq8hWLb/rJL6+75ttnfvDUD594Wp5549EDvLB17e0PrVm6+77OWudc0SyKtMgbmcqVUTaMIxlJY6116LRF7412Hr13tsm7xndsf26osXWyeJ1KrF53PbdqjIYnatO+m7AwDCmlVAomI0kE08YheKUUpQwBHCITwhTaWWeNZZw6Y5kQKlVMiN6B3XYa/tRTfy0a5tXBLe098/O0cfyb977r7t+mO2kGbnH3Hl//zPntUZYlNYBKV3uPF8I6tOjRujXrX9y4bWbB/KWUM08gCoOe7v7OJXs/+PATQS/FMi2V40WL9+xbtIgI6bU9bNWqYtxOTs+WyvN0VmiVzuuMTjrhHeBhZgQ++NHTjjpqVcsSvUiyrJnotGjr6Nk80bztvntoBLIXbrvrdox6u3r6rHNGqSLLJYFjj3orpJA2XdzWh0iSxqyp7TjvzHObM5DMkqMPfNs7jz10ZMeIyQt0mNSb6DFrNqa3vfi7n19+/913n/y2I5ftvvCSy37U19N74fkfBD2dZymidc5a69EaAILOEyBAiLfee+xfvG8tzX0ZHAfroD3qGJjXlzfG0ukhb2e9y2tTO5szs4gWEQkXxpGuBV1HHnZwxY8dud/ABV+4+Ji3HJUpMA40hdUvrv7qpVf2Lt5HynDewMKgfemlV11NFsBV11wT9ex50P5L7vztTQfteXBt2GMJitSCp5QRYxx6T7kYS9SWsS0gwdRg7ZpXX9yaTpje93zorO1mazIOR6887rH7/vCZM06YHBns7OxdPzL58kuDp7733SsGwsbsEKSD3/zMh39w2Xdq211hXdgOn/7iZ8ZqtlrtKFfaehctW/PyhsZMBhQcAnjS3tl70afOl7MVz/AXv/ld3L27DAImWByWlu22Yryun3r5ZVWA1lDuizv6FrR1dVpjvfOOisnxxtvffNTyPqiaife/46CrvnNF2iCa0kK52aJ23U2/mXbxkqV7MMqjUkQpFVFglcnTXOVFFIetCrlCmTRR1hjifZEmEzPpTN28OqFeP639PfQMAIU2z726rdYsAMAjFrnW1hSFVkYTQrnk6IFxhoje+6AcMcFlLAkBKgQ6H8ZRGLXd+5eXjj/hfaMT06QTPvThsy+++lYr4iyZPmzfrvvuuldvhFX77/uG5fMb9Ykwrjz+bw/ecd89zVqeNFSiLTTlpsHB084474kN45THpSDaNDz1yNrNR5942pYt26mRoa1sWje06i3vevDfnqcWfXPnlz5x8gt/faBouk/+67f2Ouyd5XL3bOrvvPPPMAvnn3Xut88/Q08O6TydGZ9EgpzzsFJ6Zt3Q6Wed35yudYmuMK9opd901Al/fHiNMy5tJDrPaTZz9qlHP3j/rX955LH7n3hh/vID2noG5u224rvXXMmFOPu9H73xexfODq9XReasK9KUctacquXNhKI784tfefNbTtg6k37igisGt2296AunmdmtUyMjuiiKLM2T1OiMBYIJyhg1hXLWyTAody39zT0PP/z4w8jBUIASDE1ufX79lu7+pYzJvJmmWZpnOVCfNhNdqGa9+cTqJxd2DVS4bs6MZWnzuu9/4ye/+KXIw45oXl+lJ4a2X99258c/fYkLOu78yzN77nfYnfffT3L47R/vXrpk2R33PLLv0vbV9932iY+e5WbggdUPp2iNsoSBM0YV5qZbfvfgA48ICcEC+sPfXn/0MScf+vZjH9rwRD4C11929e0/u7Sihuo7t4LH/oVLbvv9/QsHBq749ueyqW0eUdtidmzDiUfve91lVxaTIIsKydlHPvn5x9dtfWr95pM/9OnTT/tIR6ktLDo6aNv1N950xpnvj6vs8XvvW9K5DKpm2UGHL1mxyiNZv33ymj889E8fOOeFJ9dWfXtn2L7hqVfn7X7w4NYpho4AGR4afnX9xgX9FYaJZOaBvzxx2un/EuogTKv97YsiqAxtH/r4p7/2yuhUub0tbi9V5nU5dCKKPCGzO2eTWtbT1xmWAl1oWxQAFB2GpXho6+iacW+Nfv2E9nf1G+qqhO84eGDpgg4RBKVKHEgZBMIhAmKpGlMgLUsKLgWjhAcBpSwMwyCOoqisUD67afyII49Wlm7YuIlw0lWOPSYH7z7fU19uX/TTXz3w8dPfk05vAvDT9eyZDcPdXYuXLluMhtSmZhYv6V83uHF+f/f6559tK4nFvdVmqjURPV097T3zkkbS2VYtVI46qc2Mq7zpPaos7+4f6F6837euuPF3v7v3m5dc8PLGHbf95raf/eiyPQfi0S0bkvqssyZtNoyxUSmSMiq1dy1atmdc7iwMMikpYt6c2bFlky0aeZIQRrzDMI66+wfa+pad+5XvvrR+w03XX/OFCy8bnd5x769viM3k4LrnnTMqy4BRJlk2k3qPUbVcKpf7Fy/fNpU/8MSTbzrogIP3XDKxbVOWNC2aIs2cs+jB6NwjcMkBiUfo6Ggfmmy8OJzedteDDFiu8jiuCEH33GNpX3ff5k0bPvLeI/Ze1JWmTSFl1kxlEJTb2njctfqVsUXz5h3+hkX1mZ1ApaxUu7sXzGR+oLOa5coB8TbfMryF20zGcdjW10gMEi+YJGiIy0pUV9vbN4/qcz799edeeW7DM3/G5tasNguUUsqrXYue31b/1W33vPjy0/uuWJErtXnjpuPffcIpxx69Yn60Y2gdAtrCBGFQiOrXL7/pc+ec0UlrRufGKOJBRhEXolTtrfYt8lRQgCKvb9u4cV5PW9zWHsVtIEqcUY9eG1Vvzqb1qfZyGLTvs88RRzfqjR9f+c23H7xkcudOC3LevPlMhJwxby3xzmI+tmWL1kkprs6Y6M9PvXLu6cdPDq/jnBcG+noHfFglxhHwCN5ZOzM1MTG6zeWNvFnX1iqlijRXhVLGOm3iUizjQGVKFcpZlKVgcnT8xoe3Tday11Vif1c9M0YPWNx5yF49cSmIS2GlGnNKrcMgEt64sBR5a0UYxOWYUgLgOQ9lJKUMhQg6unrmDSxoZkUgo7aOTu8hbdYBTLPeREDOGRcBWqeMQnRxFFfbOh16rRUiShlmWUPwIGnMCsFUliZJk7QKGJXhjFjnTJF79AhAKQsi6RGddWhcXIlXHHT4fY++dM4FF+27fMVPr7kkstNbBjc4Y5xzWmVFWhhVCBkQQqNyWcaxYFKE3Dt0DlVeqEIRsFZpT0EGkksZxJHgwbJ9Vv7iDw9fdMm1hx56wO9/euXGFx+tzUwCeFUoo3SrfN9ay5kglARxxLns6Owa6B/YOTVZa0w7o41SDr212v9tbqZVQuyMi6tlJmVXd8+yPZZXOnuJiJWyUanEmVdppnU+MzE2OjJSpPUiTSllMuIeiBShE6VNO+rvOuqgkeEhKQIAYEHAGOVCAFB0znt01gGgpxS1EVKIIBIyAOecM846KgQh0D9/6YaR9IKvfvv6S79QTG5L0gYAEE+YDBYs2r17/lIZhYDEe+dR6SzZsX1TvTZltbbaImJv/7xb7n1yyaLFByzuSBrTaIzRlkselmMCUKQFZ0xGkbOWUOadl6VAF4oQChZ5JCmjlDDaCs5mpFRpr87f95wvXvzK8xseueP7w6+uBQBPfNFMPCG6UAQIFxzQc8m1IVun8gX9XW2h1UUBnjjwMgqtKpxxXDKHDq1ziFk9dVoj8XmSegJJrWmsSRqZdSg4D+NAaWe0IYSkzcZwk/3yvudfd68B+PtSLpdX9McrFpYGBrrCgFNKuJStEhopGKWUAhFSxpWYUBKEIQHKAymDMIojQpmUERccdoU/eiSEEuK0BsYYpWgdtjJiCdCWXQxwdIYw5tFpVTht0RkP3horpCCMOm0oYyorgIBVGih4BKAkCAN0Fg0GcRhEpWp7Z+bCBQP941teSdPUg/fotFL1yRn06IyToYwqMaWcMSED6RE456q1daAoRMApY85aGQdRuVQkmXdeyGDJsmXbprKFPR3bNr7crNWjSqRV4dA7rVtp7M44EUmrDZdBHIeUEUTvnaeMWmO8987aIs9lIJzzhILzwFppyZSG5ZgQ1qpwlEHYqotCi8ZYrTQ6pIxYbTw6Qqh3KGJBgAOTbW0VZy1lhBBCKaVCMEKokM46zri11lrbMj0Wggshvd+VZNCyYeFCEMrQYVt7Vz1DVE1rEp2nDr3OVBTHIgrQegLABTPaWGtkKIAzyqnXxqFz1lEmUk2lc6ZosoChsR5A5ZmIIkYJIcwZywQXUhqlVa6kFCwMwHsRcGdQhhKdo1ywVnU6Y0LIpcsPvO+JV0aGB9/79gNGhrc441SuKKPeo/eEUWKtC8IgzzVw3tPd1mg00DrKWKu0XinFhPDOmVwTRq3RKi/SRkoooENZCvIkm9k5mxWGS9pyjvbeq0LXa8nYdPKHJ4YLbf6/pmcAKJVKh+5e3W9Zd7Wt1Ep2FIKhw3I5CqNQCE4JYZJHcUQZE1ICAGNCBAGgD6IoiEJE8IiMM2stJcQ564zjUnq01jpKCWUcPBDGVKE5JZ4QIMRoTT0iIqIDAoTSluFG3kg9grUWnTPKUE7DSHIhnLZxJXYOuGQApL2trVlvoCcA4JxVeW6MSWuNuKPKKUXEVoZOFMUeoOUDnicZELDGOGsqXe1ZPY2rsTWWUiq4YJJbbXsG+sa3jzjnrbM84MlMAyjxHlVacCl2PV8QgICUUkjeMglAdIQS4j3hzCrlPXHWcCmcQwLApSSUOOMopUwILnnLspBx7r03SmttEB1jlAA4Y4NSZI316ONyJBj33qPzlHMmeCtrnjOGnhBGALxWhrJdNdtccPTgjA1C2Yrj/ttMNkGHhLE4jNIkVbpArZTSjDEuhAwDILRVOm2VAYIeCBMUvbfGgPOI6HeteXhtDCUeEfN6QjkjnO/KNgHCAyHDQOfKgQ+CoMgKGYdCcuLBWccE55xbZcI4JIwxQU1h5i3Y7eY7Hz7qoD0F1UWS7XK5ABABt9YJKU2hvfdRHCIBZ6w1Fq3ziDwQjAmjirBaNlkOhGRpzgVzzhFKdK611mm9Cd430wIB0DnKGXifNot1Q6OPrZ+eaRR/Dy8g+Eew9+LeQ5a3D/S3UcosWik4YywMhRBcSLHr4xsGQSApo5QJjx4ooZSV4phy5gHAe0KpBwRPGWstdXtnnTUOwDPBCKHWOqNMI9W9vdVWQDZnhFKqckUpAUZVWlBGPEBjutGy3gTvqWBC8CgKVVYwxghjIpAAwCg1heFSoLNZludpTjl12ogwYIx67602hLAwDogHJjhlrGgkQTnSuQJKhGB5WggpwAMT3HughLRGsyrLnTWF0pwzrXWRqV1GZQAylN6hRy+l8ACAXgTSAxR5EcchoAePjDPCaJFkQkrnkQvhPKJxPJDOOoueM8YFByDokTHGGPXgm7VEBoJzpo0OwpBzZpSJypEUAg0CAauNNU5EgUd0DqlgjHNdKABCGfXgW95giOgdCil4IFhruISI6BllQnJtHDgHaFsLHUCo0TaIAkopE9w7TwhorZ3FsBLnzZQQ0Eq3bFusc4xQZ50HZN57SookD6IwLwogtFSOi7xggqNFwql3uxxgCCWA3jksVUsEiFaGMWqNC0sB8SQsl6IwmpyeZQQpePBeZYUQQheFJxBFodGGh9JZR7znoSySwiPyUKBDIWXeSEQoET1QYp1zSgMA4yxppNYYAHCInLEkK9J6SihxxG/fMfOnNaPjM8nfR1n8H6LnwYmkq0JLJVGpxACgjYuE8AhZWmBSeO8pJZxncRwSwhDRIVLKokqpUasTyhjniAjgiXcItFQuMUKMNh6AklYeO/UAWdIc27Fj3UTw1oO6S6Wy915KTimz1npEZywAEAJK2TzPk1pinOkd6GXGI6LKCtnaZOq9UtoUKoxCIJClidEmzwutDQHw1hZKIXqPWG6vOmtcYoM4csbYNDPG6JoVAfeeplM1wlme5kzwsBwjekI85ywQoQtFNp0YZT26LMmBEqetUQbRW2NlKKkUhW35E1Kb50ZbRkipEnmHiH52prZ58zBGnSv6QxQBEnDGKWUgLwilujCMs9fsgcNQAiHWWGut86hzhQR4M6eEhHEgA5lrG0QheETwxmjPCWEsqTcIpx4BwYdh6Jyz1mhtPQJQoJTxDFggSlEE6Oq1pkMAtNahCERnZ7sUzCpLGAWCTNAizwERENCDjGXLO0FZba3hnKNHpbX3YIz12DI29oIzb70Fr5PUWoeISikAyLPCe4jKMSACI946h0ABKKfKaKNMy5bVWgfTIKSImom2VhfGOSsFj0sxZ2Dy3Dmn87y17KSyAhEJpcCoBwRBtdKEEK+1I+C0UUqh996iM4ZxFkShiALCmXcmjssAwELp0Ndm6mmqnt08NfH3EvM/TM+qyF7c6tsj0d9levo7qfeq0ArRWquVIQBRLAFtloUtqyznkBDI84JSih4p44Amy8ymcbNsIJjX3008AiHOGe/QE1ooPTWbbxyeGR1vJE7OD+tdfR2MB7sS3CkwyiijUSgbs408LZz3U+Mzo3XzhqBEUYVBAOgZp0EYhHFYZJpQUNYFocySLEsLqzU6Rwg4h0AIk9RpVNM1j44RCvUm5xwtAvHEIXiw6ACBcqaNJYTAVJ1QEsWhB5KNNrqkEpUOij5Ns6ASp/XEGAuCOuMsgMoVc84jUPRCMlMYQmm1rZwqldUaheMvbxp56Mltpx7fyeMwz3LjQ6uUdS7PC8YYokftwnJo0YP3WZKCB2cdZZQA5Eqj82EYeDQ8qwBAta2qsgKdA06tR1coRKesQeVUqtD7SkelSHNPiSkMkxwIYYxpAqaZFXHhrE0bWabc4EjDaLVyxUB7tZxp67T2FDjbZZFPKKDBIi9cw4eliAnu8kynmhKitSaUOofokABQIIgoQiED4Zwr8tYcBypjEVGluUN01gGjqijA2VKlpIwFoDhTR/ScciTACLUeKSHoZghnabMxU1MkLA+0A6fMWJycyZvK91WHF+++WEpRLcfg/c6xyUYj1yoZWDgvikt5UeS5Mkp5IM44GQr0aArTyAqjbXdFjqeyxzQr3R0qK/I0q89mrwzXN400/d9RWf+Y8XaLaik6fI/SHssWBJx6cM6gtdY5p7JsuqlEuY9S3xXbarWMHl5b2aKMSEbHZt0ja7aOzOQfPHrxwkX9grHM+K1jycRkzRitjd+8fWaqljpPCPi9dp+/fH5lXhtyETl0rbho75ECOGusccbhdCNbP0E6q2L/hSUZhHEsKIEsSbI0rZRjEQRGaXDOgdPKRGGJMe7QWYPW6kp7m9UKEYBSD94Za7TxiJ09nVIKm1v0TluVpxoJ8UA9IgEgLn1lOnpp8+hh++/2hkVIaUCZiKulPNUI3qHL0jSuVlFbEQjBSNpM88Kkae6c45wbGg6PNTbsmJ1t5nGpss/Szsl6dtg+C/pkTsLQ5JoKzqXIkswoDCOJiFnWsiICj0A59QDOWEJpHHGl3RT0HroglZU2Qhk62CUY5VSmc120ImMpAyGCIssQAT1a4yhjYRRkWRHFkVWKCK5z9cKmmR0JO3ix3HeP3s6eDg9gchOVQwKMgAdA51DpliEPQWuLXItAAsWs0dTKcSEZp3lS6CITATfKVrvaitxIIbjgnBHGWJqm1nnGmFaWcyCMp43UydL2sanF8zpRZdZ5XWgC2nskhFqHXDBtkHif+3jjDBufnOrprIL3gvPZejJTqy9f3L9fH+/uCqUU/QM9WpvJ6frW2bhNJF0xa++sROU4axZpMw1KERBwxlLGwONEUzz54uCpb1vOjYrby5MjM7Oztc3j2VNDydRsDf5/omcA2GvpgsVx3tNdAUK6KmymqXc27GRGmTHL996L1Yfa20PvUAjZivAFKqcaxWwjG5vV28Zr1mFfZ7kc8ky7Zqac89ZhyxzbOvwf1smYYCSUNAykcw58K4CilfKOQRTneZYrV1hPACSnUopAMCF4vZlaY3u6Oxmx6JDKUpIkeZ5XyjGnJM11KFl7OWwLKRMiLawqCsporhzlQgqRK0UAEcF7ZExEgfTeW6Ol5KVSeXRydmw6NdZJwaKA9XZ3lgNfDoNmM/EeCeOeimYz9R4JIZSxaqlkrHPorMUsV/W0yLVrXWYrONM6V46D/rZgYU+lGov2asyoL3NPAT3xtVoWxpEqcsoYQReEcabNTE4mpxvIg8HRWqZwj4WdlEvvUSlltCGMZ1kBzra8ECsRIzJqNBLKGFoHBOJQckqcNa0pwPZqKeQ0L9Rzg9MWYaC7smxRV1sl8ojO2kIVHgHAc8ZZILXWiNBTEYTKkfHpRqZK1Sp4wsDGQRAEcraRT9Zz5TyhDGyGFltGgq1UnTAMGXjvNHjkMpycTZ3D1KAxNpRcckKACM7CKOSct4K+W7kmhJBaUjSz1g7clo848R5bjk59nW2hQADSUS1FUUDBDo42p2YbS3tKB+61sC1mKk8qAW/5+jPiheBSCqPN0y9Pbk/diQf1B8IXhdk2lj45VB//O460/6/QMyEk4LS1etpZDjJDGmneXg72WNAthaCCcUbGZ7Kp6dpMMwcPhAB6AN+6Ef+A8/2P224JaXUiaZnMQ2v6FVrBGP/rka/9mABB/5/+KrIr3ZsAvPb13/rqtdb/zwHg9LWUa4AoYG3VipRiYUfUrM0sXDjQbKaTOZ2dmWlkOlPGe+//1hAhuxr+TxPG/+NFkf9wN3dV4fh/P5N/P+jf30le68jWO3wr0vy15v/WUX7Xmfxn3f4/f/O/6xCyqzXy2smRXYMz/N9d5mvX0jq+dZsIAKW7Xookby1nRKHkFFoez+j8ZEOVIlmW4LxX2jUKhDnmmON1/fvxPzx35phjjjnmmGOOOeaYY4455phjjjnmmGOOOeaYY4455phjjjnmmGOOOeaYY4455pjj/yX8d0+lbXhXvOq/AAAAAElFTkSuQmCC) 7px 13px no-repeat; }\n"
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
       << "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"
       << "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"
       << "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"
       << "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"
       << "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"
       << "kGoVEEmrCJZA1HBqs+6sxAstmWaSLDXteosK02qv/n+svXmQped13nfOu33b3br79jrTPT37ihlgsO8gQFIkAVISSUmUKFKyJZWkOJFlx5VUSqU4rqTKlsvlRE5iRWZFTuTIkriIkimQIEWQWIhtAMxgBrN3T0/v2+2++7e+y8kfdySrUq5IkPX91VW3q6vvvd95v/c953men8MwK5zNjc01V4gAqJhLNHEMGvqE0levvrfV6D71+MRUo52s9IExSg05JEvkBgnfd1BZTHJZ8mQ1jNrFKSL/VD05OsoZbochn67IeiCqvhiNYgaIPCuor51FYL7gVX/PA1MXwN837mW78cBujR4HABigSAy5Tk4aeCUU5dArhR3KLs9fO//2u+fOXfBga+HqlbFK9shDU7VawDj5Qqxuds69t9ZO7b/6p5/71MdGy5G7cbvd3s10svNHX3+/XlXd1DC0YeB9gHq21gU+Co6IfKtZtHY6E1OjK6vNtbVmc+3GJx6s/MZ/89ChY7PXFna1cfecmNhsZV9//sbFa+0zR8dr9QA8yaRkitOAnakdWADGAAEJmM/FcMSqAR8JxXCJBUpMVPhIyCIPUk2ZtYUjTbaVM+l4SSJHC3B8pDbrjX37rfm0HwOD3GiG6Axxzop+xjxGyF6/2P3S11f+w8ubV271212Tpb2dzZtSL33sofL/8F/d/4ufv+enP33/cx85rSTqwijJlVJScmTYizUBdvtGayDnrDVa25HhcKvR/s7rjW5s9k2Fdx0Kd9pJYYxxNklSBPupZw5duPBmo5nin8fhCs6RDT5YkkJYa7M8y+IEiBq77XYnl5Jb6xDBEf1/UqAGsWUM73hmEDnnHBlH5OSQHDmiwwcn90+wf/uV9y7d7CLghx6ok0mMdcAYZwJ5RUMUhoEfhkEQlqNwYaUtVZDmthsbLgUXSkpVKoU7iWd5+eL1hhCq09fOgXWuFrEn75/cWF9b2cqHauqJM2GcxfNdO7fTHlK7o6ND15aJcTk1GphkOxQIYYiMnb/a/frLjSQtopB/4ZnhI2x961an6Ac4WNIAidmzd+/rtLbJWSKMY2MlV2MBCzghknXoCBGRAbFBFCCgRZRsICIUHH2P9ZvxxuWNlTduN19efPausw/ef+z67eZm19jCkbYgGACSdszj3ZXOkQxay+v/65evtxOnBPvJj06mV7ZQMgw48wXzOIqBPBGGyjL0Wfva9sZrS+0Lax8f34OoRTmIZ4YbJybE2SleUf6hYTVZ4RW/JzljKtcQa8DI42VP1nwxEgRPHegdn1IVDxgAhwHWkFJHFim1Ni5IO+SIgQBHtJMw7SzSzu7uaJScvXv23//+lfevbl5c6v7w00cuXF5f3U4fe/zoP/tvH54ImyvzNza2dlY32mEUXjp/LdbAEDmDQCHn7APMn7lUnuDOAyVZt1e8d2X16AnP84PXL21864Vr9ZI9cmzPz//I3oMTth3ne0aju/b7RZ585Jm7srhjk8QCl4qjJ4AsF8IxRCnJWkSOkiPCYPCJklHheCjJODKWRYo4g9wyDrZfABOQWzUdEJBQwRmqity7trBO5GBgp0FAAFdYRBZORb2FtrHs1kq2vp29e7VzbLZ0/EDp0ExYr7oi63MuT+8rnT1atTB17/Hg//i9860+IEOGzDiz2ciEGEkGyVEIxpgg4EDQ7aavvdc6NFN64FT19JHyu9fiNNNSicZO69f/4WeK3tILr6ycOVrbNxkCgef5gR8Jxgf75DzL+r2uEkJ6KvCDNMmF4INmCWMsCHz9lxOgCBBwoDxzzgDigFZPBM65Qfs1z/XhvXy32b56OyFkD58Zmax7zz4+8rWX4onxgAshFRhjGSATjDEWROGlGzveZ4/EqekmxvMjLoUQghD9QBUFW9vOOOP9xBoHxO3e8ajs53ESt7r25KFgYc28ebun6kOshGXOxM4GTFUYD6XoDIXQsxnECSILPHh9nu693H783vr4aPCFx4rffGVp9V1JfTEYtptU33N8Ikn6jKE1kOfWKqEqPirOBnieXkEcUXC0RM6BA5eZAbSRHIQe/8jj0+M7w1NTUzWf7xuOZqfDpdtXv/GdmxsZSM4YRyAiS6DQAahGcZyJpd3iplPrXTs7DicOlmZe2mps9cOJMhkChgMhNxFUIv4zP3Lk08WYkEEUeUNB9s3/64LYPyEqgT09yRKDDNECegIV9gVnKLSh1DisChYIDCT3OPN5dnxCOsd8Dpyhds44u5PabmH7BWiHnNmCwBjKLeWanGOMIRfWuD1j/iefO/nyudXI6Uo4ndbsL/z44QfPVOeu3lqz+mvfWc3cdLevpcfOnV+RgTcypCRCXlC/F3+Aei4KHfi1yC8Ai/17ImuSm1evKVRnD7B+NwPDV1+6zdhyeaS23tuJh9VDd9X27NlTHx999fXdkVq4stUHKVymUXJyRICQFaQtcXYnxB0BnOPlAD0OQqGzqBm5O6xdyhw4TSWlNwoxpbzD4cHh8KGloXfmG41Gc2Aq4oxpa4hQKsw7WTRdliVP9w0iT3N9ay1b39FvXGrPTPpnjpSPzpZmp4KyzSFuMa5OH4h+5Sf3/tr/NletlPM8I+eS3DEmBrnHlsgOZhjOaGN2e/p753afeXDk4N7o0HRwYS4rcZwcr3348aMvfecrb77fvvfMfjXYYgEOYoD/3IpKnlJGW53kTLA8zwH9wUuDGx3+ckcDkXPBGUdEx9AOYkaJAIBzgUja2KFaePr45NXry1muL1xrdfXZKk+febD+yoUeZ3zgPBFcIh/QOW0Y8YvXF4Vg3b7JcpIeB4TCESIZom7MOttN5BgnRluMIjW7t9rvddPUELBeYt55b/Wf/+a//PXf+B8rJ8aaER7WlOdWqFAIzhnsrm8wUSACEkFU/YNX2pN1dWR/7d7j1U+trf3erZVBu3KAUK6Vhc4TABwswcAQCdEAKkaFI4foc0QkNhB4IBmgxA2e70KwsyeHzooxhGJptbuwkH3vpY33r67PtwxMD4lhHwPBq8rFhhwVeXFmfPxMTb345evQt//77y88+E+eCoPWTzw9/uvPz81+/gyqASkNAQERdtrFGxd3i6KHjFdDMTqUmxxsM6VhnwsGZQ8EUO6Yz4GwxxCRp5nNHYBgTDHkgy+RiVCQdogMLbjY2t3ctnK9GbvMIeMu05AbG+euXyARMJamuhzIJDftmJ06OTVUZVs75vW3r1gnJMOv3Fh1Wnseqyhe8bJAuldfOVcpizPDXp44X/lbzTzL0w+kD6OultM1xXnqSRuFMoiEs5TsqW400mZmJVdxktd9eOZEbWrvUG1kVHlhY7s5P7+5sdx9/P6Zc++ta0sAZB1HQUSEShADspYxBr7goe8Kh5whOeDMJpr1NYaCkFFmqE8YSl1oJIOpnqxXmPMvXFvMsmyAznHoABAZ+UK109x089JsLWukRMA4JwJjWZzB1dvp3HJarzVPHCg9dHro8L4o8oteV+8ZUw+ciq4s5oIx66jdyQlZXjhylqzNcgQg5JIhWUOX5jrvXO0cnolmJtX5G3Gj0fwvfu6TpnPlxTfXN3et8suAKRBprZ0trBv4fcBYFyepUMr3PbKUZDlA4BzcEW9bO/jNO4cfBORCCAkIaNzAf+XIOQeS68JyADh9ZPTM0fqN61e+8KnDUvlW9/PcDFfU0/dXvvZSa3ZmChkfOJ8tOK0JnTXGkHO5do7AETAHXFCrV4yW8Je+GP2L37aM+9YRchGEUaBIF4ngzPN4nGgxUvr6i8/TsBfMKBEh39auIERWaJdmVnFyeqBXBa68jrf3q99f/YdTke+p5x4bv7G9/drV3NnZwQE17veASoOgnzDkigpAQJ+7wmIgmCdsZhCJeRLJomKkEbUDzgAhL2x7t1OOkn5c/N43lt9dSMATEHlsf9XfU5aRL0ciyjQrSZc5uVmMru20gR0e8h46MmY1tZrNamSP7C89tbd7Zbtf2V8jTWTtIEohTu2bFxtrTPKS9Dxx9HriCRFrTUjckygZERA4IHLaWcXa3URrlzvinrwTfiIQnEWFxDhosplxzczF1sWaCkIAmxjqp5Q7BECBLiNCOrB/aKQSNncaczcWRocOj03uHR6Xw1XY2EovXu/s9rXn8ziFib3V+hCLlGCI3Y5fOEgFszbKLBpnP8D5GQCuLmxaDZ4cMlpay1xO1bLnKXrg3j2nD5eefGzmxz5x+OkHJw8cmRkan84zd/vmzfPnb52/1goD9Xc+dbYaiG6jZ3JNqbZJTrmx/QwzwysBr0e8FiBjoJjrZi41VBggx2oeAkBu0VdiNKJEm80sX82KP9uZuCGaRanb7ZMlICiMM4W5E2mXWYHougXGGfociMixwcSaC+55HoHYasGLb7V++2vLL7290+lr5wgRTh4oddptBDDGFpYIRaGdkMo6Z40uhwpdEaeOAWYF+8q315XAe4/XxoZAKfXDT+9/5a2bl2/1S5UKAA5CRawxzg6a0kAAnpJhEPhSDpXLjVYr6bcAIE61No4Gzkbn/vLzWSmpPOV5nh94THAmmOf7oU/TY0gEYyPVM8eq3/j2m3/ycvebr279h5dW/ut//m6hHefwxNkRj/WzwgohPM/jkoNDN5Abo9puFbkeUAC4c67Z0nur3i9+uvLq+YGyvogz6ylliN1a2hHcVcuSAzEEWS5dWl4Yu28krGM2zlJCJRVy2Y11J7abfQJLMCDmAEAQXs3HvvrCKkPnB/JXfmSs7vUZKQRkks8t7Qo5yNND3+OCg80tSIaMuaTgIyEgMGCknUutTS0gQW5AO4aQZPbf/snK+3PtSll+5kNTwzXP7qn6h4b9fTU1VhKHKmSBCnKF1ZkJF1t7ff5b37z+Z1vuj+d2vr7a/v3vLWvrQl88eVcleXvVcXTa3pHbATBET3I5GkUnJ/jB4SUm99SkTixqcoWzuwX1NSINYtwgUls7WkoJhOgxcuAc2NQSQ+KMEJx2rp27vrXN3GynZq2r17uUFyzyyRBZQ6mxxsW99PSByR/98PGewXcv716/trg0d9MhG586cPzE/qcfGvvMc0ceumfssQenDs6UJ8fLgcfRMa5kUSgOlcKJjf5/Qr/9V9Rzq9ffjLHfTzX57a7sxrC9XfjKa++26/VyyE1UUpl2hVU337927dr6e5cbl641bq0Xn3n29Ff+9G0pzc9/8eGzJya6zR7lg2xTJM5sN7XdnHLtHHFE5AwDAcB4KQDjgDFkDByZzb7ZTVnVty1Na/mQqdxcb8ZxnzEmlSRnEXDQHXYFCYZ5JzlS8w+N+9YRAqGATFsi4Mg48pKvuJLbTfvlb29dW+gDEGM4WfezLOsnSWEKPpBTIjJkURSVSqGSApAVxvVSA4xtd+zSRnb8QOnwXvX3fv5TYHZffns9zgaGZVBK1MrcGU2ucOQQgAFkedFPEq2LycmxH7z+jucHAGgNWktAZJ0bzJn/PCwGGXLf90qlkvI8pZQnvWaz88kPzd5ez0pRpHi6bzJ8/tWG8gImglJpxLHqxZs952Coon762clmqz2wcwouhZJCSi44ACBTDNA6stYkGRw74P3yF4ZeeFe/0zLAxG4rsYY0YHW4stPJyRVCsEpJOMeYReYLSdj7rIJtCoZ8CcgZa+xmOx0LA+Mi3JFLSc9n9enXVv3zl3fywgxV1N957nC1WgNAFGJuqSWFGMRODVeVT+QKiwQutyzwTKOPAm1hKNHIGDhwqf2L0wgBpZn9za+uLK7Gh2crv/qjs6OFYZPl6OSot68qgIN1vOq71LGePhxjUMZlEv5dk+xovfTk7Mubplr2nKMje6MTqJvvbzPJiMEdyCsCAxBlT45H0f4ae3L/FRlUDwyRJYgNSgaOUAkuOBN33mya2cSBCCRyxhjymmIcWUHoADPnOoVt53YndbllwyHFmW0meqUNzrpukRId2jf8+U/f+60fXLq9tHF4dmxuNXv/WuvSjc6V81fnrl3tJzkwqUCPVGQeZ4xcazvpdqnRsrtNYV1QGLg6v7m2sfWB65mIXnnvRjNFjqqwspsEO20+dztZXi2Wlno3brYWlvrvnl9+/ZX3LlzZuXFzdXMrvrJclCrlP33hWpE5kauPPnTo1dduPHb/TLXCdC+jTLusQMExkMgFD5TuZDa3ZrsHDG03dblzCNZZvdjESLHIp8ICgEmc1YjICO94GZylQUMZiRAZt7C7lU6ND338qSNABTCGFp1xWjvnLDFyjAVShr7qxW6t4Qg9AMi1s0RJXuRpkaQFOXPHngROcFGtBIiO/lwsubSWPv/qlq/wF37qwU8/M/3uhcuXbrak8jEUQEB3vFJuz969UxNjiCgF85QSyMpRtLCwnOc5oqiUQ0cAyLgQjDGGf6meGQrJGeMD8EWpVAbEo4cmHr1nGDDwOL/n1N75ha3dtouiUErPkZ3ZO/71728RgOBwZCYYr+YWUPhKSMkQkKPwfODcmNw6AkBrxPQe/7mnqn/47c5bS91yicuwAizwFB47deS+h+6zqnru3cVyKE4cLEnlDg4FNoZ427gv6fwbhgPt21vtdjtzy52dHoGQ//EWQpCCB6WoXdr3rfNpHBdJpn/okckTM7nWOffw3UtbziaDDMPJcb8mXN5MnXGoOPUKyi2LJOWEIScHYAnIgSdB3ukyCKR0OPrS8xvr28mZ40NfvGeIXdtyjngoREXxSJhMYygrFj5y974X3rsh9w1zwXjJl8MeHB79k+9tCI7lkvzME6PpO2s2d8gB2V/kbhEKxn0BFd/fPxw9NcvLkpUE+IKMIwdmJ3OxJgDw+M3bDU9xIAIOrCzJOdcrbGJMbsxuXmylZjO126ntZXanb5baNrFkHAG51Fiwj98zmRXFo/fMztZHb8w3OVEz5deX4vX1+NatzQtXmudev3bhvZW5xd71G62NzeTWUrq+axpt0e2LwjBCtrITL3fz/7Rm5K8Di33xwvz5+Q2lpOCSYQiu1OnztQ24uZTfuL67veu2dk2/Z1e39evX+rXa8K/9vUejkvzmS3MP37+vGhSM8YdPzxyZredakyXQlpLC9TKXabMbU6K4/hkAACAASURBVG7AWRSCUg2GkIiaKcWaT1XAujuHM+RM8KzQ/X4MDjnnUgp3R5VMREwqAZb5gVhcbO8bD08eGXHWAjAQ3BgTp4WzoAsNiAO2Gw+nvGjUWFhcT4jQOciNjjNtiWX5gEEJeTEQ0w3yBoAIgXnnr/Wv3uqPVfK1+ZdfeGUxzgRKIHbnHzEWRkfHp6fGdnZ2yOlDM5ExRaaLoZHaxas3Bcf7Tk/MzoxvN1OtjTV2EJ8y2GkLzhCIiBBxsCt2zhKZf/XrH/k3//6NMIx83/7sZ84+/9LNicnJ0A8qlcrQ0EgpCltJlBcAgBN1/8GT/vLK2iAMlCvFuZAoyqXqRiPJtTMODx8Mn3uy9Lsv7NxK8fBjR6RfMs4QEBHOXb/95uvv7Kbyu+90tHEffqCOmjine8Z8vyum5/mDT/pX39z93Kfvv3Rl/vVL3dgI4Xl3VlQAITiCACb8+tilbv35VzaNcXFvZ3dzLs00aeedmXjxpUUhGDkaqqiJ0iCIyiAHVlYAaNsFBnfCtEEAOQB7ZzJPiEQgav765NC/+fryylr7I4+P/cypCl7btv2CEAEBHbpm7n93Mdfbb665cDziNU9NhIA4+fj0739/e7CV2FP3PjTr95dawBHwzmJBBEBADmQoXea4L0RJMU8gAipOhQPnICdyQJoKwxwKYoh4h9HhOtZ1tFlLzGps1vo21qaV2lg77Zw1A4ulcwShnD1Ue+DYTLfbGy7TIw/uefGthbGJ8Nd+6dHtxDt/q7e0mfe6ZqNRrG2b+dvxrVW9vg2drmCuCuRFpUgIvLSw8/rVpTTu/83r2Tl3ZWnna6/euLSwmRSaK1kpVQlk4FV6qdBGLm7oSwvJhdv5ieP7f/azZ7mT//i/++jPff6+ueXGr/yTb3PmxuqyFoZhidUixo21WeFSbba6tpuSI0o1xfkAwaJbCUjGKj4aC4TAkFLrCgO+BCbCgFur2WCIjXfyThCBIfe5GC17C+u7pYh94ZOHZyYD5zQO0maMybUpChOneZoXkxPDp+86EQbe0nr89pXOQGirvAAREYW1DhAZZ+1OvH/flOQs8jkAOGe1zte2izffb8f95NXzO1dvx0wxVvd6u30hfSl5pSSzXPfj5J1Li6tr2/ccKx+fVVEYXrk2n6X9yXrwkUdO33V0/N1Li7kmIscYN9ZKFXHGDs2UTh4dnRgtj9fD8ZFgYrQ8VOG/9c9+9tLFt9++mjMOP/7DD64svLO4JTwl/MAPwrBcrlRqtdrQ1IvndsMw8D3+yOnqRDXtdPqcC4ZSCs8hECPBFUccHVGP3B/9P99p5uVoZHYkXu3bRJMjncdxaienp+5/4uF7nnhioVt96e2GtvRjT9fzzLW3iv2+KNrF0kv8V3/+Q6srS1/99vLSDvJSVfglCPygXB8f8XVqkHMGnEup9hw4t129PNclIs5wgNoqHRr+zrs7COAIPMXuPVYdlWhz7QrLFCfrUFvuMxQcFQPFmOLOOc4VF8qTvBQKk7vo6OitqPSNV7bWG/EnH538eEDFpa18pw9cZFuxeH37V376Y69eXDOlQJaULCtR9UU94I7Yo7NzK4WUcqimzk55dH076xcIgnMVBcL3uEXCkDtLKBiPFFeA1qFCVhHAkPmCDSvmcfBYnhd5bhJNXHLqG9PI7G5mNzKzlppGYpup3ei5zLq4AENkCTgoxU88MO1VpW4XE6Nqazf57//F967fbPza33/yhx47MlKr/P0vPlyrT7x+Pb62nK42nNaqMErJUKpgqFY1SL3cvHZp4etvLl9Z3Ppb4LkDgLF2q1PcXG1eW9y8td5c2u4ubLUXNnpXltorjXSrUzx0eub+U1NG2/Wtbr+dPf7gbBiyS9e2lMBOJ2/3eqP16GOPH2334rWNnuIcEUk7ZGgdWIbMEhDxsk/aAQATnIAAHAECouTszNS+Isvm55YLrTnnaZY5cjjIfXOU56bXSxzD8Yr8iY/vO3povDC82y+0Ic4YAAgpKuXoxNH9/+CXP/3omWj51qVvfH/t3NUOEmOck6Ohqnrm4envvDq/3dKewnKkfukLT1G+QTZe2kx7fUvAjAUhcaLuvXWxeWM1w5JC64aU+MzHzlLeIKcX1/rkcGWr6Pb71QifenBvP3UEdu/kyOkT+/bt9d94651v/mAbmeRcSikmRtRnP/lA3lv1PXzyoRPPPnP3hx7e98T9e598YPqzn7g7bV7+2rfeb3bZZ5+7/2c/e/fctfdWtnVWAHJPSqmU8n0/8NX5Sws//SN3rW1sBb6sRHDlVhdFCADGGs5Vr9975IRa2Wzv9vTiqklU4AuvSK2QzCqWdfIfOlt5473GRl8uLW0szC81+u76fLvZjPdPhfefHFtdT9td8Av3mU8cnygn//Nvf/PyKmI4JKMSF+wTj8w+fNdk1du5dmXDhlUnFCoVlUo7fZ7tbs+MIGNsaSM7P9cTR+q7mZslM1b3OcdSIK7e7OxEgTdacokFIvQHyw6RAcqJYuMub//Q/kOzdRb3Gp7Epc1MVwO+f3h1uT+Fpj6sTu0r9Rc6jfXMKhi7nfzqY48eO4wL83NbbWsjxYYCZMwlGhBBskOpnJ1Q7W5aKwnTL5Yut545eODQqCDTBqRbKwnWQz4cMF+4uEApgDMQCIkBzphihIAeAwDv0vrUuPf6za43VmHIXFu7TqG3ErMbu1aWbfddahgQFYasA2M1Y3cdHPrih05cujjf6eoitzutZLRe+twPnzp9dOry1a2d7X6prKqhKrR77f3N1Z3s+kpnaau7vN1d2OhcWdq+cnv7+kprt68HOMH/P00nfJCrHAZEpLM4COROOxVcAMDIUHjmxJ7X3rldK3tLa22wVC35fza32WpnTzy8P8l4t9N77/r2uQvrT9x7KJJ8ZChE1h7MpXVhsy7Ux0qjQ+F6XjACSoo7bspAAiBGEhmBx/V2r9eJXZYwxpGAIXJklthfqLKUEv3YZdqtbuzenPem69Xf+EcPff/t3cWN3FhhCX1P7Nsz9Mi9+3xsn3vz5ZfeXnvrcidJyZNA5KwDrU2r1R4wGnSR3XNi5sKFt6/dXDGOHdtX7iduc0droPdv9kZrYqWRgy+JwM/0/unKiy+fu3J9sVqJjs6IS/PNamXs4lz/5tLW9Nj2mROzxw8d7bZzxumPn3/p3OVEKF8bJIRAJI+ePfTKK6/duHnbEgu9eSXsX7QuCu22W7SyrU8erE+Pi//pN37LOHZyv3ztwq7mklcq1lpr7NryzYkR9jtfOX9zselJLgX6tt9t4vjMYUmKUIzvnfnGd9/batHytjc7601VhahIw3lTC9PWFlhhSCpsrfSGJsKwWpFSdXa871/buLa6e2xP4/SBGWK+75evv//uv353I2Ujtelhy1XSWKsVqxPesS/9zh9mub7vMPvB++/QxDG2/y5ENjKz5+LNtvvuYkmYZls745AzPDL6f76++KsBn5kp1Ue8Zw4Gt683irGSKnsIgMhMK2UlxQTX3dS9ubJvqQfB+u+ev7XbM6ND6mwFX3l5nh47EExX37m4OXerlxp7aLo6Pbdz89zSvQ8cX5i/9O3nL3ph+SQW7762iJVjouKDc8QYfvfWwlRlZa6z1XJhyAXS3X0BGxtfubzeaCXjdW+q1V/91k3+hbNs0qMOkAMQQIkDTSwS6DHIDDhgSi3M7WiTAuMuttZkUJBLjOvktpVRpveNVzY2+63tXhhIgegQWDma3lPupllV1tbi1VZc/INffBrJHdw7/r2X5y7Pbd13fPL7r94KI3Xm+OT3z91+7Oy+q7e2Gs0sM05JjALV6+eekkme019VoR+sno011SiKrS4F4uiBfTfmmyPVWhQWdx2efOv8Yq3iP3r/wfWN9p6J8guvzS0stlPj9o1XbZqdOTEJFmcmcGtzIU+8Uljq9zq6D3sn/IfvGb33rslm3/7mV6+Hkd9vpRB4nkTpAKxF55Ax1yWRWcz7HmeECIjaOMBB0JsYhG+FntptYeR5t7fxd76+cHgmOjizsXdy9MRsTSoPkee57fR2Xn/5/bnbjQs3ujeXkp2WRsYdARlyhJ1e9oO3F3qJA4BM48tvLX73B/9xvqckGotMcKvEu4upQ+RKmNz2cnf+WmO1KYu81rzRVJKPDJU4w3KpioK/dGHhxXPvqWEl95V5GB7OhstlJgT2M2cMApcvvLLQ6mSOEMAR4Z1QwDtoJ2AInCm9HL/1v3ybUCKQksz3gkAqzrnyPOWpsFwqmP+N15M0E7rQCBCFUbkmrbFMKgKIouj9m+U8L7xKebPnVUmOIpUj41OelWR9Jnz/2sZuM/NLNS4lgTAOZRhaN84nht9c33hlflswK4NSYXF03z0j1bKQspOYPI3Br371rWa355wmKbBUqiNyYR3nAqUaOnzy6jxkq2s6B1GtomTeWJScmvqDl1Y/9zTOTJc+9MjExs7Sn1zdlg9OIwAUhknpCkIyQMhqwQa5L11byGPneppWC98T0VTVWMdGwqvK6+wUuqe/u9QoDXnh/uE/3t7ozOeuUFjEvi+8PRVkf278dk6MRT+wurlClGqbZ9IXYZnevDBf5ESFpaWiVPF5KFyiXT/nZUnWQU4DtAjTDoFR6gjItvX61PBqU/v1EPrGpJSnlO0mwlqZGJsVP/rUnlKIr7618vb77Z12LpAmomBrI16ii0cP1EZGjx08UA09IbT59ss3vvz8+x9+/MChA6O14WhirLKwtI0IZ49PppkpsnalosaGvbXNliiVUHlJnsPfbj2nacoA6pWa1Rk6evaZI+9fK3Zay1mSR4G6eLVxeKY+s6dWH42mxiuXb7Re+P6toRo7dXB8dLR8eDpUrjE6PrzUVK3W0g9/8lTWbFR9PT1MzcbmH7ywmzdyyu3Dp0cePBTe2jUvv9fuZnnR6HhREBC4UtAv4nqprLVxjgw5JrjLiXPCQbOEC0+IIisaPdztqgs3d0K5NTayPFRmUvBBK3u3Xex0bKtrksxpQwBAzhZECI6Idtv4Jy9ttbsGAIvc/aX+ggOAQqNQMtxTLh2ops1UNzPXT21cWAvDQ7X77zl169ayr7hSQklljBmfHFld3+IGvD1D4YOT/snKvTg1vdBc/25f+qIkqBu7doocpAwCshbACMGFEJxxIcSAdwOEA+PRcH3YWgvkGOdhGJaiklIKOXeI0/tPWGtr40Wv00nSzJFhnCs/RCm58hAxt+nwvqNJt1MYA1LFxOIW+jGVI48QvcroC+9vNfMq96UFRtaCNQ7BABs7eFDWquVS5KxVviJCY4wfhkE5kp0kB2FQKK2rWWKyFBCs9FRURnKDFpNUsnrwWDAy3u7sFkTD9SiWxjs4cn0n/dorjecepiMHSj/2iVn3auMPzq3OfPyw3k25x6GwoDgvS3v3hC1M2M69ZmbzwsWaBdJGKhwvgcfZ0wdL24mNM9KOBcqWBUpey52JNeWGLEHJR86BA1hkBPSJ"
       << "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"
       << "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"
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
}

// print_html_masthead ====================================================

void print_html_masthead( report::sc_html_stream& os, sim_t* sim )
{
  // Begin masthead section
  os << "\t\t<div id=\"masthead\" class=\"section section-open\">\n\n";

  os.printf(
    "\t\t\t<h1><a href=\"%s\">SimulationCraft %s-%s</a></h1>\n"
    "\t\t\t<h2>for World of Warcraft %s %s (build level %s)</h2>\n\n",
    ( sim -> hosted_html ) ? "http://www.simulationcraft.org/" : "http://code.google.com/p/simulationcraft/",
    SC_MAJOR_VERSION, SC_MINOR_VERSION, dbc_t::wow_version( sim -> dbc.ptr ), ( sim -> dbc.ptr ? 
#ifdef SC_BETA
    "BETA"
#else
    "PTR"
#endif
    : "Live" ), dbc_t::build_level( sim -> dbc.ptr ) );

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
    os << "\t\t<pre>\n";
    size_t num_errors = sim -> error_list.size();
    for ( size_t i=0; i < num_errors; i++ )
      os << sim -> error_list[ i ].c_str() << "\n";

    os << "\t\t</pre>\n\n";
  }
}

void print_html_beta_warning( report::sc_html_stream& os )
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
  os << "<html>\n\n";

  os << "\t<head>\n\n";
  os << "\t\t<title>Simulationcraft Results</title>\n\n";
  os << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n\n";

  print_html_styles( os, sim );

  os << "\t</head>\n\n";

  os << "\t<body>\n\n";

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
  os << "\t\t<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.7.1/jquery.min.js\"></script>\n";

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
  s.open_file( sim, sim -> html_file_str.c_str() );

  report::generate_sim_report_information( sim, sim -> report_information );

  // Set floating point formating
  s.precision( sim -> report_precision );
  s << std::fixed;

  // Print html report
  print_html_( s, sim );

  // Close file
  s.close();
}

} // END report NAMESPACE

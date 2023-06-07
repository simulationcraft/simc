// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "reports.hpp"
#include "report/report_helper.hpp"
#include "report/report_timer.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "data/report_data.inc"
#include "interfaces/sc_js.hpp"
#include "util/git_info.hpp"
#include "sim/scale_factor_control.hpp"
#include "sim/profileset.hpp"
#include "fmt/chrono.h"

#include <iostream>

namespace
{  // UNNAMED NAMESPACE ==========================================

// Experimental Raw Ability Output for Blizzard to do comparisons
namespace raw_ability_summary
{
template <class ResultRange>
double aggregate_damage( const ResultRange& results )
{
  return std::accumulate(begin(results), end(results), 0.0, [](auto l, auto r) {
    return l + r.fight_actual_amount.mean();
  });
}

/* Find the first action id associated with a given stats object
 */
int find_id( const stats_t& s )
{
  int id = 0;

  for ( const auto& a : s.action_list )
  {
    if ( a->id != 0 )
    {
      id = a->id;
      break;
    }
  }
  return id;
}

void print_raw_action_damage( report::sc_html_stream& os, const stats_t& s, const player_t& p, int j, const sim_t& sim )
{
  if ( s.num_executes.mean() == 0 && s.compound_amount == 0 && !sim.debug )
    return;

  if ( s.player->collected_data.fight_length.mean() == 0 )
    return;

  int id = find_id( s );

  char format[] =
      "<td class=\"left  small\">%s</td>\n"
      "<td class=\"left  small\">%s</td>\n"
      "<td class=\"left  small\">%s%s</td>\n"
      "<td class=\"right small\">%d</td>\n"
      "<td class=\"right small\">%.0f</td>\n"
      "<td class=\"right small\">%.0f</td>\n"
      "<td class=\"right small\">%.2f</td>\n"
      "<td class=\"right small\">%.0f</td>\n"
      "<td class=\"right small\">%.0f</td>\n"
      "<td class=\"right small\">%.1f</td>\n"
      "<td class=\"right small\">%.1f</td>\n"
      "<td class=\"right small\">%.1f%%</td>\n"
      "<td class=\"right small\">%.1f%%</td>\n"
      "<td class=\"right small\">%.1f%%</td>\n"
      "<td class=\"right small\">%.1f%%</td>\n"
      "<td class=\"right small\">%.2fsec</td>\n"
      "<td class=\"right small\">%.0f</td>\n"
      "<td class=\"right small\">%.2fsec</td>\n"
      "</tr>\n";

  double direct_total = aggregate_damage( s.direct_results );
  double tick_total   = aggregate_damage( s.tick_results );
  if ( direct_total > 0.0 || tick_total <= 0.0 )
  {
    os << "<tr";
    if ( j & 1 )
      os << " class=\"odd\"";
    os << ">\n";

    os.printf(
      format,
      util::encode_html( p.name() ).c_str(),
      util::encode_html( s.player->name() ).c_str(),
      util::encode_html( s.name_str ).c_str(),
      "",
      id,
      direct_total,
      direct_total / s.player->collected_data.fight_length.mean(),
      s.num_direct_results.mean() / ( s.player->collected_data.fight_length.mean() / 60.0 ),
      s.direct_results[ FULLTYPE_HIT ].actual_amount.mean(),
      s.direct_results[ FULLTYPE_CRIT ].actual_amount.mean(),
      s.num_executes.mean(),
      s.num_direct_results.mean(),
      s.direct_results[ FULLTYPE_CRIT ].pct,
      s.direct_results[ FULLTYPE_MISS ].pct + s.direct_results[ FULLTYPE_DODGE ].pct
        + s.direct_results[ FULLTYPE_PARRY ].pct,
      s.direct_results[ FULLTYPE_GLANCE ].pct,
      s.direct_results[ FULLTYPE_HIT_BLOCK ].pct + s.direct_results[ FULLTYPE_HIT_CRITBLOCK ].pct
        + s.direct_results[ FULLTYPE_GLANCE_BLOCK ].pct + s.direct_results[ FULLTYPE_GLANCE_CRITBLOCK ].pct
        + s.direct_results[ FULLTYPE_CRIT_BLOCK ].pct + s.direct_results[ FULLTYPE_CRIT_CRITBLOCK ].pct,
      s.total_intervals.mean(),
      s.total_amount.mean(),
      s.player->collected_data.fight_length.mean() );
  }

  if ( tick_total > 0.0 )
  {
    os << "<tr";
    if ( j & 1 )
      os << " class=\"odd\"";
    os << ">\n";

    os.printf(
      format,
      util::encode_html( p.name() ).c_str(),
      util::encode_html( s.player->name() ).c_str(),
      util::encode_html( s.name_str ).c_str(),
      " ticks",
      -id,
      tick_total,
      tick_total / sim.max_time.total_seconds(),
      s.num_ticks.mean() / sim.max_time.total_minutes(),
      s.tick_results[ RESULT_HIT ].actual_amount.mean(),
      s.tick_results[ RESULT_CRIT ].actual_amount.mean(),
      s.num_executes.mean(),
      s.num_ticks.mean(),
      s.tick_results[ RESULT_CRIT ].pct,
      s.tick_results[ RESULT_MISS ].pct + s.tick_results[ RESULT_DODGE ].pct + s.tick_results[ RESULT_PARRY ].pct,
      s.tick_results[ RESULT_GLANCE ].pct,
      0.0,
      s.total_intervals.mean(),
      s.total_amount.mean(),
      s.player->collected_data.fight_length.mean() );
  }

  for ( auto& elem : s.children )
  {
    print_raw_action_damage( os, *elem, p, j, sim );
  }
}

void print( report::sc_html_stream& os, const sim_t& sim )
{
  os << "<div id=\"raw-abilities\" class=\"section\">\n\n"
     << "<h2 class=\"toggle\">Raw Ability Summary</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  // Abilities Section
  os << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th class=\"left small\">Character</th>\n"
     << "<th class=\"left small\">Unit</th>\n"
     << "<th class=\"small help\" data-help=\"#help-ability\">Ability</th>\n"
     << "<th class=\"small help\" data-help=\"#help-id\">Id</th>\n"
     << "<th class=\"small help\" data-help=\"#help-total\">Total</th>\n"
     << "<th class=\"small help\" data-help=\"#help-dps\">DPS</th>\n"
     << "<th class=\"small help\" data-help=\"#help-ipm\">Imp/Min</th>\n"
     << "<th class=\"small help\" data-help=\"#help-hit\">Hit</th>\n"
     << "<th class=\"small help\" data-help=\"#help-crit\">Crit</th>\n"
     << "<th class=\"small help\" data-help=\"#help-execute\">Execute</th>\n"
     << "<th class=\"small help\" data-help=\"#help-count\">Count</th>\n"
     << "<th class=\"small help\" data-help=\"#help-crit-pct\">Crit%</th>\n"
     << "<th class=\"small help\" data-help=\"#help-miss-pct\">Avoid%</th>\n"
     << "<th class=\"small help\" data-help=\"#help-glance-pct\">G%</th>\n"
     << "<th class=\"small help\" data-help=\"#help-block-pct\">B%</th>\n"
     << "<th class=\"small help\" data-help=\"#help-interval\">Interval</th>\n"
     << "<th class=\"small help\" data-help=\"#help-combined\">Combined</th>\n"
     << "<th class=\"small help\" data-help=\"#help-duration\">Duration</th>\n"
     << "</tr>\n";

  int count = 0;
  for ( const auto& p : sim.players_by_name )
  {
    for ( const auto& stat : p->stats_list )
    {
      if ( stat->parent == nullptr )
        print_raw_action_damage( os, *stat, *p, count++, sim );
    }

    for ( const auto& pet : p->pet_list )
    {
      for ( const auto& stat : pet->stats_list )
      {
        if ( stat->parent == nullptr )
          print_raw_action_damage( os, *stat, *p, count++, sim );
      }
    }
  }

  // closure
  os << "</table>\n"
     << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";
}

}  // raw_ability_summary

/* Prints a array of strings line by line. Necessary because we can't have big
 * single text strings,
 * because VS limits the size of static objects.
 */
template <typename Range>
void print_text_array( report::sc_html_stream& os, const Range& range )
{
  for ( auto&& v : range )
  {
    os << v;
  }
}

// print_html_contents ======================================================

void print_html_contents( report::sc_html_stream& os, const sim_t& sim )
{
  size_t c = 2;  // total number of TOC entries
  if ( sim.scaling->has_scale_factors() )
    ++c;

  size_t num_players = sim.players_by_name.size();
  c += num_players;
  if ( sim.report_targets )
    c += sim.targets_by_name.size();

  if ( sim.report_pets_separately )
  {
    for ( const auto& player : sim.players_by_name )
    {
      for ( const auto& pet : player->pet_list )
      {
        if ( pet->summoned && !pet->quiet )
          ++c;
      }
    }
  }

  os << "<div id=\"table-of-contents\" class=\"section\">\n"
     << "<h2 class=\"toggle\">Table of Contents</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  // set number of columns
  int n;                  // number of columns
  const char* toc_class;  // css class
  if ( c < 6 )
  {
    n         = 1;
    toc_class = "toc-wide";
  }
  else if ( sim.report_pets_separately )
  {
    n         = 2;
    toc_class = "toc-wide";
  }
  else if ( c < 9 )
  {
    n         = 2;
    toc_class = "toc-narrow";
  }
  else
  {
    n         = 3;
    toc_class = "toc-narrow";
  }

  int pi = 0;  // player counter
  int ab = 0;  // auras and debuffs link added yet?
  for ( int i = 0; i < n; i++ )
  {
    int cs;  // column size
    if ( i == 0 )
    {
      cs = (int)ceil( 1.0 * c / n );
    }
    else if ( i == 1 )
    {
      if ( n == 2 )
      {
        cs = (int)( c - ceil( 1.0 * c / n ) );
      }
      else
      {
        cs = (int)ceil( 1.0 * c / n );
      }
    }
    else
    {
      cs = (int)( c - 2 * ceil( 1.0 * c / n ) );
    }

    os << "<ul class=\"toc " << toc_class << "\">\n";

    int ci = 1;  // in-column counter
    if ( i == 0 )
    {
      os << "<li><a href=\"#raid-summary\">Raid Summary</a></li>\n";
      ci++;

      os << "<li><a href=\"#apm-summary\">Actions per Minute / DPS Variance Summary</a></li>\n";
      ci++;
      if ( sim.scaling->has_scale_factors() )
      {
        os << "<li><a href=\"#raid-scale-factors\">Scale Factors</a></li>\n";
        ci++;
      }
    }
    while ( ci <= cs )
    {
      if ( pi < static_cast<int>( sim.players_by_name.size() ) )
      {
        player_t* p = sim.players_by_name[ pi ];
        os << "<li><a href=\"#player" << p->index << "\">" << util::encode_html( p->name() ) << "</a>";
        ci++;
        if ( sim.report_pets_separately )
        {
          os << "\n<ul>\n";
          for ( const auto* pet : sim.players_by_name[ pi ]->pet_list )
          {
             if ( pet->summoned )
            {
              os << "<li><a href=\"#player" << pet->index << "\">" << util::encode_html( pet->name() ) << "</a></li>\n";
              ci++;
            }
          }
          os << "</ul>";
        }
        os << "</li>\n";
        pi++;
      }
      if ( pi == static_cast<int>( sim.players_by_name.size() ) )
      {
        if ( ab == 0 )
        {
          //os << "<li><a href=\"#auras-buffs\">Auras/Buffs</a></li>\n";
          ab = 1;
        }
        ci++;
        os << "<li><a href=\"#sim-info\">Simulation Information</a></li>\n";
        ci++;
        if ( sim.report_raw_abilities )
        {
          os << "<li><a href=\"#raw-abilities\">Raw Ability Summary</a></li>\n";
          ci++;
        }
      }
      if ( sim.report_targets && ab > 0 )
      {
        if ( ab == 1 )
        {
          pi = 0;
          ab = 2;
        }
        while ( ci <= cs )
        {
          if ( pi < static_cast<int>( sim.targets_by_name.size() ) )
          {
            player_t* p = sim.targets_by_name[ pi ];
            os << "<li><a href=\"#player" << p->index << "\">" << util::encode_html( p->name() ) << "</a></li>\n";
          }
          ci++;
          pi++;
        }
      }
    }
    os << "</ul>\n";
  }

  os << "<div class=\"clear\"></div>\n"
     << "</div>\n\n"
     << "</div>\n\n";
}

// print_html_sim_summary ===================================================

void print_html_sim_summary( report::sc_html_stream& os, sim_t& sim )
{
  os << "<div id=\"sim-info\" class=\"section\">\n"
     << "<h2 id=\"sim-info-toggle\" class=\"toggle\" style=\"margin-bottom: 0;\">Simulation & Raid Information</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  os << "<div class=\"flexwrap\">\n";

  os << "<table class=\"sc\">\n"
     << "<tr class=\"left\">\n"
     << "<th>Iterations:</th>\n"
     << "<td>" << sim.iterations << "</td>\n"
     << "</tr>\n";

  os << "<tr class=\"left\">\n"
     << "<th>Threads:</th>\n"
     << "<td>" << ( ( sim.threads < 1 ) ? 1 : sim.threads ) << "</td>\n"
     << "</tr>\n";

  os << "<tr class=\"left\">\n"
     << "<th>Confidence:</th>\n"
     << "<td>" << sim.confidence * 100.0 << "%</td>\n"
     << "</tr>\n";

  os.printf( "<tr class=\"left\">\n"
             "<th class=\"help\" data-help=\"#help-fight-length\">Fight Length%s:</th>\n"
             "<td>%.0f - %.0f ( %.1f )</td>\n"
             "</tr>\n",
             ( sim.fixed_time ? " (fixed time)" : "" ),
             sim.simulation_length.min(),
             sim.simulation_length.max(),
             sim.simulation_length.mean() );

  os << "<tr class=\"left\">\n"
     << "<td colspan=\"2\"><h3>Performance:</h3></td>\n"
     << "</tr>\n";

  os.printf( "<tr class=\"left\">\n"
             "<th>Total Events Processed:</th>\n"
             "<td>%lu</td>\n"
             "</tr>\n",
             sim.event_mgr.total_events_processed );

  os.printf( "<tr class=\"left\">\n"
             "<th>Max Event Queue:</th>\n"
             "<td>%ld</td>\n"
             "</tr>\n",
             as<long>( sim.event_mgr.max_events_remaining ) );

  os.printf( "<tr class=\"left\">\n"
             "<th>Sim Seconds:</th>\n"
             "<td>%.0f</td>\n"
             "</tr>\n",
             sim.iterations * sim.simulation_length.mean() );

  os.printf( "<tr class=\"left\">\n"
             "<th>CPU Seconds:</th>\n"
             "<td>%.4f</td>\n"
             "</tr>\n",
             chrono::to_fp_seconds(sim.elapsed_cpu) );

  os.printf( "<tr class=\"left\">\n"
             "<th>Physical Seconds:</th>\n"
             "<td>%.4f</td>\n"
             "</tr>\n",
             chrono::to_fp_seconds(sim.elapsed_time) );

  os.printf( "<tr class=\"left\">\n"
             "<th>Speed Up:</th>\n"
             "<td>%.0f</td>\n"
             "</tr>\n",
             sim.iterations * sim.simulation_length.mean() / chrono::to_fp_seconds(sim.elapsed_cpu) );

  os << "<tr class=\"left\">\n"
     << "<td colspan=\"2\"><h3>Settings:</h3></td>\n"
     << "</tr>\n";

  os.printf( "<tr class=\"left\">\n"
             "<th>World Lag:</th>\n"
             "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
             "</tr>\n",
             as<double>( sim.world_lag.total_millis() ),
             as<double>( sim.world_lag_stddev.total_millis() ) );

  os.printf( "<tr class=\"left\">\n"
             "<th>Queue Lag:</th>\n"
             "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
             "</tr>\n",
             as<double>( sim.queue_lag.total_millis() ),
             as<double>( sim.queue_lag_stddev.total_millis() ) );

  if ( sim.strict_gcd_queue )
  {
    os.printf( "<tr class=\"left\">\n"
               "<th>GCD Lag:</th>\n"
               "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
               "</tr>\n",
               (double)sim.gcd_lag.total_millis(),
               (double)sim.gcd_lag_stddev.total_millis() );

    os.printf( "<tr class=\"left\">\n"
               "<th>Channel Lag:</th>\n"
               "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
               "</tr>\n",
               (double)sim.channel_lag.total_millis(),
               (double)sim.channel_lag_stddev.total_millis() );

    os.printf( "<tr class=\"left\">\n"
               "<th>Queue GCD Reduction:</th>\n"
               "<td>%.0f ms</td>\n"
               "</tr>\n",
               (double)sim.queue_gcd_reduction.total_millis() );
  }

  os << "</table>\n";

  // Timeline Distribution Chart
  if ( sim.iterations > 1 )
  {
    highchart::histogram_chart_t chart( "sim_length_dist", sim );
    if ( chart::generate_distribution( chart, nullptr, sim.simulation_length.distribution, "Timeline",
                                       sim.simulation_length.mean(), sim.simulation_length.min(),
                                       sim.simulation_length.max() ) )
    {
      chart.set_toggle_id( "sim-info-toggle" );
      os << chart.to_target_div();
      sim.add_chart_data( chart );
    }
  }

  os << "</div>\n";

  // Gear Charts
  highchart::bar_chart_t gear( "raid_gear", sim );
  if ( chart::generate_raid_gear( gear, sim ) )
  {
    gear.set_toggle_id( "sim-info-toggle" );
    os << gear.to_target_div();
    sim.add_chart_data( gear );
  }

  // Raid DPET Chart
  highchart::bar_chart_t dpet( "raid_dpet", sim );
  if ( chart::generate_raid_dpet( dpet, sim ) )
  {
    dpet.set_toggle_id( "sim-info-toggle" );
    os << dpet.to_target_div();
    sim.add_chart_data( dpet );
  }

  // closure
  os << "</div>\n"
     << "</div>\n\n";
}

// print_html_raid_summary ==================================================

void print_html_raid_summary( report::sc_html_stream& os, sim_t& sim )
{
  os << "<div id=\"raid-summary\" class=\"section\">\n\n"
     << "<h2 class=\"toggle open\">Raid Summary</h2>\n"
     << "<div class=\"toggle-content\">\n";

  os << "<ul class=\"params\">\n";
  os.format( "<li><b>Raid Damage:</b> {}</li>\n", static_cast<int64_t>( sim.total_dmg.mean() ) );
  os.format( "<li><b>Raid DPS:</b> {}</li>\n", static_cast<int64_t>(sim.raid_dps.mean()) );
  if ( sim.total_heal.mean() > 0 )
  {
    os.format( "<li><b>Raid Heal+Absorb:</b> {}</li>\n",
               static_cast<int64_t>( sim.total_heal.mean() + sim.total_absorb.mean() ) );
    os.format( "<li><b>Raid HPS+APS:</b> {}</li>\n",
        static_cast<int64_t>(sim.raid_hps.mean() + sim.raid_aps.mean()) );
  }
  os << "</ul>\n";

  os << "<div class=\"column-charts\">\n"; // Open DIV for charts

  highchart::bar_chart_t raid_dps( "raid_dps", sim );
  if ( chart::generate_raid_aps( raid_dps, sim, "dps" ) )
  {
    os << raid_dps.to_target_div();
    sim.add_chart_data( raid_dps );
  }

  if ( sim.enemy_targets > 1 )
  {
    highchart::bar_chart_t priority_dps( "priority_dps", sim );
    if ( chart::generate_raid_aps( priority_dps, sim, "prioritydps" ) )
    {
      os << priority_dps.to_target_div();
      sim.add_chart_data( priority_dps );
    }
  }

  highchart::bar_chart_t raid_dtps( "raid_dtps", sim );
  if ( chart::generate_raid_aps( raid_dtps, sim, "dtps" ) )
  {
    os << raid_dtps.to_target_div();
    sim.add_chart_data( raid_dtps );
  }

  highchart::bar_chart_t raid_hps( "raid_hps", sim );
  if ( chart::generate_raid_aps( raid_hps, sim, "hps" ) )
  {
    os << raid_hps.to_target_div();
    sim.add_chart_data( raid_hps );
  }

  os << "</div>\n"; // Close DIV for charts

  if ( !sim.raid_events_str.empty() )
  {
    os << "<table class=\"sc even\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th class=\"left\">Raid Event List</th>\n"
       << "</tr>\n";

    auto raid_event_names = util::string_split<util::string_view>( sim.raid_events_str, "/" );
    for ( size_t i = 0; i < raid_event_names.size(); i++ )
    {
      os.printf( "<tr>\n"
                 "<th class=\"right\">%d</th>\n"
                 "<td class=\"left\">%s</td>\n"
                 "</tr>\n",
                 as<int>( i ),
                 util::encode_html( raid_event_names[ i ] ).c_str() );
    }
    os << "</table>\n";
  }

  // RNG chart
  if ( sim.report_rng )
  {
    os << "<table class =\"sc even\">\n"
      << "<thead>\n"
      << "<tr>\n"
      << "<th>RNG Report</th>\n"
      << "<th>+/- of 90% Interval</th>\n"
      << "<th>% of Mean</th>\n"
      << "</tr>\n"
      << "</thead>\n";

    for ( const player_t* p : sim.players_by_name )
    {
       double range = ( p->collected_data.dps.percentile( 0.95 ) - p->collected_data.dps.percentile( 0.05 ) ) / 2.0;

      os.printf( "<tr>\n"
                "<td class=\"left\">%s</td>\n"
                "<td>%.1f</td>\n"
                "<td>%.1f%%</td>\n"
                "</td>\n"
                "</tr>\n",
                util::encode_html( p->name() ).c_str(),
                range,
                p->collected_data.dps.mean() ? ( range * 100 / p->collected_data.dps.mean() ) : 0 );
    }
    os << "</table>\n";
  }

  // closure
  os << "</div>\n"
     << "</div>\n";

  // Check if extra section is even needed
  highchart::bar_chart_t raid_apm( "raid_apm", sim );
  bool has_aps = chart::generate_raid_aps( raid_apm, sim, "apm" );

  highchart::bar_chart_t raid_stddev( "raid_stddev", sim );
  bool has_stddev = chart::generate_raid_aps( raid_stddev, sim, "stddev" );

  highchart::bar_chart_t raid_tmi( "raid_tmi", sim );
  bool has_tmi = chart::generate_raid_aps( raid_tmi, sim, "tmi" );

  highchart::bar_chart_t raid_waiting( "raid_waiting", sim );
  bool has_waiting = chart::generate_raid_downtime( raid_waiting, sim );

  if ( has_aps || has_stddev || has_tmi || has_waiting )
  {
    os << "<div id=\"apm-summary\" class=\"section\">\n"
       << "<h2 class=\"toggle\" id=\"apm-summary-toggle\">Additional Raid Information</h2>\n"
       << "<div class=\"toggle-content column-charts hide\">\n";

    if ( has_aps )
    {
      raid_apm.set_toggle_id( "apm-summary-toggle" );
      os << raid_apm.to_target_div();
      sim.add_chart_data( raid_apm );
    }

    if ( has_stddev )
    {
      raid_stddev.set_toggle_id( "apm-summary-toggle" );
      os << raid_stddev.to_target_div();
      sim.add_chart_data( raid_stddev );
    }

    if ( has_tmi )
    {
      raid_tmi.set_toggle_id( "apm-summary-toggle" );
      os << raid_tmi.to_target_div();
      sim.add_chart_data( raid_tmi );
    }

    if ( has_waiting )
    {
      raid_waiting.set_toggle_id( "apm-summary-toggle" );
      os << raid_waiting.to_target_div();
      sim.add_chart_data( raid_waiting );
    }

    os << "</div>\n"
       << "</div>\n";
  }
}

void print_html_scale_factors( report::sc_html_stream& os, const sim_t& sim )
{
  if ( !sim.scaling->has_scale_factors() )
    return;

  scale_metric_e sm = sim.scaling->scaling_metric;
  std::string sf    = util::scale_metric_type_abbrev( sm );
  std::string SF    = sf;
  std::transform( SF.begin(), SF.end(), SF.begin(), toupper );

  os << "<div id=\"raid-scale-factors\" class=\"section\">\n\n"
     << "<h2 class=\"toggle\">" << SF << " Scale Factors (" << sf << " increase per unit stat)</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  os << "<table class=\"sc even\">\n";

  // this next part is used to determine which columns to suppress
  std::vector<double> stat_effect_is_nonzero;

  // initialize accumulator to zero
  for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
  {
    stat_effect_is_nonzero.push_back( 0.0 );
  }

  // cycle through players
  for ( auto p : sim.players_by_name )
  {
    // add the absolute value of their stat weights to accumulator element
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      stat_effect_is_nonzero[ j ] += std::abs( p->scaling->scaling[ sm ].get_stat( j ) );
    }
  }
  // end column suppression section

  player_e prev_type = PLAYER_NONE;

  for ( size_t i = 0, players = sim.players_by_name.size(); i < players; i++ )
  {
    player_t* p = sim.players_by_name[ i ];

    if ( p->type == HEALING_ENEMY )
    {
      continue;
    }

    if ( p->type != prev_type )
    {
      prev_type = p->type;

      os << "<thead>\n"
         << "<tr>\n"
         << "<th class=\"left small\">Profile</th>\n";
      for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
      {
        if ( sim.scaling->stats->get_stat( j ) != 0 && stat_effect_is_nonzero[ j ] > 0 )
        {
          os << "<th class=\"small\">" << util::stat_type_abbrev( j )
             << "</th>\n";
        }
      }
      os << "<th class=\"small\">wowhead</th>\n"
#if LOOTRANK_ENABLED == 1
         << "<th class=\"small\">lootrank</th>\n"
#endif
         << "</tr>\n"
         << "</thead>\n";
    }

    os.printf( "<tr>\n"
               "<td class=\"left small\">%s</td>\n",
               util::encode_html( p->name() ).c_str() );
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( sim.scaling->stats->get_stat( j ) != 0 &&
           stat_effect_is_nonzero[ j ] > 0 )
      {
        if ( p->scaling->scaling[ sm ].get_stat( j ) == 0 )
        {
          os << "<td class=\"small\">-</td>\n";
        }
        else
        {
          os.printf( "<td class=\"small\">%.*f</td>\n", sim.report_precision, p->scaling->scaling[ sm ].get_stat( j ) );
        }
      }
    }
    os.printf( "<td class=\"small\"><a href=\"%s\" class=\"ext\"> wowhead </a></td>\n"
               "</tr>\n",
               p->report_information.gear_weights_wowhead_std_link[ sm ].c_str() );
  }
  os << "</table>\n";

  if ( sim.iterations < 10000 )
    os << "<div class=\"alert\">\n"
       << "<h3>Warning</h3>\n"
       << "<p>Scale Factors generated using less than 10,000 iterations will vary from run to run.</p>\n"
       << "</div>\n";

  os << "</div>\n"
     << "</div>\n\n";
}

struct help_box_t
{
  const char* abbreviation;
  const char* text;
};

/* Here simple help boxes with just a Title/Abbreviation and a Text can be added
 * the help-id will be a tokenized form of the abbreviation, spaces replaced
 * with '-',
 * everything lowerspace and '%' replaced by '-pct'
 */
const help_box_t help_boxes[] = {
  { "APM", "Average number of actions executed per minute." },
  { "APS", "Average absorption per active player duration." },
  { "Constant Buffs", "Buffs received prior to combat and present the entire fight." },
  { "Execute", "Average number of times an action is executed per iteration." },
  { "Crit", "Average crit damage." },
  { "Crit%", "Percentage of executes that resulted in critical strikes." },
  { "DPE", "Average damage per execution of an individual action." },
  { "DPET", "Average damage per execute time of an individual action; the amount of damage generated, divided by the "
            "time taken to execute the action, including time spent in the GCD." },
  { "DPR", "Average damage per resource point spent." },
  { "DPS", "Average damage per active player duration." },
  { "DPSE", "Average damage per fight duration." },
  { "DTPS", "Average damage taken per second per active player duration." },
  { "HPS", "Average healing (and absorption) per active player duration." },
  { "HPSE", "Average healing (and absorption) per fight duration." },
  { "HPE", "Average healing (or absorb) per execution of an individual action." },
  { "HPET", "Average healing (or absorb) per execute time of an individual action; the amount of healing generated, "
            "divided by the time taken to execute the action, including time spent in the GCD." },
  { "HPR", "Average healing (or absorb) per resource point spent." },
  { "Count", "Average count of impacts per iteration." },
  { "Dodge%", "Percentage of executes that resulted in dodges." },
  { "DPS%", "Percentage of total DPS contributed by a particular action." },
  { "HPS%", "Percentage of total HPS (including absorb) contributed by a particular action." },
  { "Theck-Meloree Index",
    "Measure of damage smoothness, calculated over entire fight length. Related to max spike damage, 1k TMI is roughly "
    "equivalent to 1% of your health. TMI ignores external healing and absorbs. Lower is better." },
  { "TMI bin size", "Time bin size used to calculate TMI and MSD, in seconds." },
  { "Type", "Direct or Periodic damage." },
  { "Dynamic Buffs", "Temporary buffs received during combat, perhaps multiple times." },
  { "Buff Benefit", "The percentage of times the buff had a actual benefit for its mainly intended purpose, eg. damage "
                    "buffed / spell executes." },
  { "Glance%", "Percentage of executes that resulted in glancing blows." },
  { "Block%", "Percentage of executes that resulted in blocking blows." },
  { "Id", "Associated spell-id for this ability." },
  { "Ability", "Name of the ability." },
  { "Total", "Total damage for this ability during the fight." },
  { "Hit", "Average non-crit damage." },
  { "Interval", "Average time between executions of a particular action." },
  { "Avg", "Average direct damage per execution." },
  { "Miss%", "Percentage of executes that resulted in misses, dodges or parries." },
  { "Origin", "The player profile from which the simulation script was generated. The profile must be copied into the "
              "same directory as this HTML file in order for the link to work." },
  { "Parry%", "Percentage of executes that resulted in parries." },
  { "RPS In", "Average primary resource points generated per second." },
  { "RPS Out", "Average primary resource points consumed per second." },
  { "Scale Factors", "Gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>Loss</b> per unit "
                     "stat <b>decrease</b>." },
  { "Gear Amount", "Amount from raw gear, before class, attunement, or buff modifiers. Amount from hybrid primary "
                   "stats (i.e. Agility/Intellect) shown in parentheses." },
  { "Stats Raid Buffed",
    "Amount after all static buffs have been accounted for. Dynamic buffs (i.e. trinkets, potions) not included." },
  { "Stats Unbuffed", "Amount after class modifiers and effects, but before buff modifiers." },
  { "Ticks", "Average number of periodic ticks per iteration. Spells that do not have a damage-over-time component "
             "will have zero ticks." },
  { "Ticks Crit", "Average crit tick damage." },
  { "Ticks Crit%", "Percentage of ticks that resulted in critical strikes." },
  { "Ticks Hit", "Average non-crit tick damage." },
  { "Ticks Miss%", "Percentage of ticks that resulted in misses, dodges or parries." },
  { "Ticks Uptime%", "Percentage of total time that DoT is ticking on target." },
  { "Ticks Avg", "Average damage per tick." },
  { "Timeline Distribution",
    "The simulated encounter's duration can vary based on the health of the target and variation in the raid DPS. This "
    "chart shows how often the duration of the encounter varied by how much time." },
  { "Waiting", "This is the percentage of time in which no action can be taken other than autoattacks. This can be "
               "caused by resource starvation, lockouts, and timers." },
  { "Scale Factor Ranking", "This row ranks the scale factors from highest to lowest, checking whether one scale "
                            "factor is higher/lower than another with statistical significance." },
  { "Uptime Average Duration", "The average duration of an instance of the tracked uptime." },
};

void print_html_help_boxes( report::sc_html_stream& os, const sim_t& sim )
{
  os << "<!-- Help Boxes -->\n";

  for ( auto& hb : help_boxes )
  {
    std::string tokenized_id = hb.abbreviation;
    util::replace_all( tokenized_id, " ", "-" );
    util::replace_all( tokenized_id, "%", "-pct" );
    util::tolower( tokenized_id );
    os << "<div id=\"help-" << tokenized_id << "\">\n"
       << "<div class=\"help-box\">\n"
       << "<h3>" << hb.abbreviation << "</h3>\n"
       << "<p>" << hb.text << "</p>\n"
       << "</div>\n"
       << "</div>\n";
  }

  // From here on go special help boxes with dynamic text / etc.

  os << "<div id=\"help-tmirange\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>TMI Range</h3>\n"
     << "<p>This is the range of TMI values containing " << sim.confidence * 100
     << "% of the data, roughly centered on the mean.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-tmiwin\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>TMI/MSD Window</h3>\n"
     << "<p>Window length used to calculate TMI and MSD, in seconds.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-msd\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Max Spike Damage</h3>\n"
     << "<p>Maximum amount of net damage taken in any N-second period (default 6sec), expressed as a percentage of max "
        "health. Calculated independently for each iteration. 'MSD Min/Mean/Max' are the lowest/average/highest MSDs "
        "out of all iterations.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-error\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Error</h3>\n"
     << "<p>Estimator for the " << sim.confidence * 100.0 << "% confidence interval.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-range\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Range</h3>\n"
     << "<p>This is the range of values containing " << sim.confidence * 100
     << "% of the data, roughly centered on the mean.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-fight-length\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Fight Length</h3>\n"
     << "<p>Fight Length: " << sim.max_time.total_seconds() << "<br />\n"
     << "Vary Combat Length: " << sim.vary_combat_length << "</p>\n"
     << "<p>Fight Length is the specified average fight duration. If vary_combat_length is set, the fight length will "
        "vary by +/- that portion of the value. See <a "
        "href=\"https://github.com/simulationcraft/simc/wiki/Options#combat-length\" class=\"ext\">Combat Length</a> "
        "in the wiki for further details.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<!-- End Help Boxes -->\n";
}

// print_html_styles ========================================================

void print_html_style( report::sc_html_stream& os, const sim_t& )
{
  // Logo
  os << "<style type=\"text/css\" media=\"all\">\n"
     << "#logo {background-image: url(data:image/png;base64,";
  print_text_array( os, __logo );
  os << "); background-repeat: no-repeat; position: absolute;width: 350px; height: 187px; background-size: cover; }\n"
     << "</style>\n";

  // Rest
  os << "<style type=\"text/css\" media=\"all\">\n";
  print_text_array( os, __html_stylesheet );
  os << "\n</style>\n";
}

// print_html_masthead ======================================================

void print_html_masthead( report::sc_html_stream& os, const sim_t& sim )
{
  // Begin masthead section
  os << "<div id=\"masthead\" class=\"section section-open\">\n\n";

  os.printf(
      "<span id=\"logo\"></span>\n"
      "<h1><a href=\"%s\">SimulationCraft %s</a></h1>\n",
      "https://www.simulationcraft.org/", SC_VERSION);

  const char* type =       ( sim.dbc->ptr ?
#if SC_BETA
                    "BETA"
#else
                    "PTR"
#endif
                    : "Live" );

  if ( !git_info::available())
  {
  os.printf(
      "<h2>for World of Warcraft %s %s (wow build level %d)</h2>\n\n",
      dbc::client_data_version_str( sim.dbc->ptr ), type,
      dbc::client_data_build( sim.dbc->ptr ) );
  }
  else
  {
    std::string commit_link = "https://github.com/simulationcraft/simc/commit/";
    commit_link += git_info::revision();
    os.printf("<h2>for World of Warcraft %s %s (hotfix %s/%d, git build <a href=\"%s\">%s</a>)</h2>\n\n",
        dbc::client_data_version_str( sim.dbc->ptr ), type,
        dbc::hotfix_date_str( sim.dbc->ptr ), dbc::hotfix_build_version( sim.dbc->ptr ),
        commit_link.c_str(), git_info::revision());
  }

  std::time_t rawtime = std::time(nullptr);
  const tm localtime  = fmt::localtime( rawtime );

  os << "<ul class=\"params\">\n";
  if constexpr ( SC_NO_NETWORKING_ON )
  {
    os.format( "<li><b>No Networking</b></li>\n" );
  }
  os.format( "<li><b>Timestamp:</b> {:%Y-%m-%d %H:%M:%S%z}</li>\n", localtime);
  os.printf( "<li><b>Iterations:</b> %d</li>\n", sim.iterations );

  if ( sim.vary_combat_length > 0.0 )
  {
    timespan_t min_length = sim.max_time * ( 1 - sim.vary_combat_length );
    timespan_t max_length = sim.max_time * ( 1 + sim.vary_combat_length );
    os.printf(
        "<li class=\"linked help\" data-help=\"#help-fight-length\"><b>Fight Length:</b> %.0f - %.0f</li>\n",
        min_length.total_seconds(), max_length.total_seconds() );
  }
  else
  {
    os.printf( "<li><b>Fight Length:</b> %.0f</li>\n", sim.max_time.total_seconds() );
  }
  os.printf( "<li><b>Fight Style:</b> %s</li>\n", util::encode_html( util::fight_style_string( sim.fight_style ) ).c_str() );
  os << "</ul>\n"
     << "<div class=\"clear\"></div>\n\n"
     << "</div>\n\n";
  // End masthead section
}

void print_html_errors( report::sc_html_stream& os, const sim_t& sim )
{
  if ( !sim.error_list.empty() )
  {
    os << "<pre class=\"section section-open\" style=\"color: black; background-color: white; font-weight: bold;\">\n";

    for ( const auto& error : sim.error_list )
      os << util::encode_html( error ) << "\n";

    os << "</pre>\n\n";
  }
}

void print_html_beta_warning( report::sc_html_stream& os )
{
#if SC_BETA
  os << "<div id=\"notice\" class=\"section section-open\">\n"
     << "<h2>Beta Release</h2>\n"
     << "<ul>\n";

  auto beta_warnings = report_helper::beta_warnings();
  for ( const auto& line : beta_warnings )
    os << "<li>" << line << "</li>\n";

  os << "</ul>\n"
     << "</div>\n\n";
#else
  (void)os;
#endif
}

void print_html_hotfixes( report::sc_html_stream& os, const sim_t& sim )
{
  std::vector<const hotfix::hotfix_entry_t*> entries = hotfix::hotfix_entries();

  os << "<div class=\"section\">\n";
  os << "<h2 class=\"toggle\">Current simulator hotfixes</h2>\n";
  os << "<div class=\"toggle-content hide\">\n";

  std::string current_group;
  bool first_group = true;

  for ( size_t i = 0; i < entries.size(); ++i )
  {
    const hotfix::hotfix_entry_t* entry = entries[ entries.size() - 1 - i ];
    if ( entry && ( entry->flags_ & hotfix::HOTFIX_FLAG_QUIET ) )
    {
      continue;
    }

    if ( sim.dbc->ptr &&
         !( entry && ( entry->flags_ & hotfix::HOTFIX_FLAG_PTR ) ) )
    {
      continue;
    }

    if ( entry && !sim.dbc->ptr &&
         !( entry->flags_ & hotfix::HOTFIX_FLAG_LIVE ) )
    {
      continue;
    }

    if ( entry && current_group != entry->group_ )
    {
      if ( !first_group )
      {
        os << "</table>\n";
      }

      os << "<h3>" << util::encode_html( entry->group_ ) << "</h3>\n"
         << "<table class=\"sc even\">\n"
         << "<thead>\n"
         << "<tr>\n"
         << "<th>Tag</th>\n"
         << "<th class=\"left\">Spell / Effect</th>\n"
         << "<th class=\"left\">Field</th>\n"
         << "<th class=\"left\">Hotfixed Value</th>\n"
         << "<th class=\"left\" colspan=\"2\">DBC Value</th>\n"
         << "</tr>\n"
         << "</thead>\n";
      current_group = entry->group_;
      first_group   = false;
    }

    if ( entry )
    {
      os << "<tr>\n"
         << "<td class=\"left\"><strong>" << util::encode_html( entry->tag_.substr( 0, 10 ) ) << "</strong></td>\n"
         << "<td class=\"left\" colspan=\"5\"><strong>" << util::encode_html( entry->note_ ) << "</strong></td>\n"
         << "</tr>\n";
    }
    if ( const hotfix::effect_hotfix_entry_t* e = dynamic_cast<const hotfix::effect_hotfix_entry_t*>( entry ) )
    {
      os << "<tr>\n"
         << "<td></td>\n";
      const spelleffect_data_t* effect = sim.dbc->effect( e->id_ );

      std::string name = report_decorators::decorated_spell_name( sim, *effect->spell() );
      name += " (effect#" + util::to_string( effect->index() + 1 ) + ")";
      os << "<td class=\"left\">" << name << "</td>\n";
    }
    else if ( const hotfix::spell_hotfix_entry_t* e = dynamic_cast<const hotfix::spell_hotfix_entry_t*>( entry ) )
    {
      os << "<tr>\n"
         << "<td></td>\n";
      const spell_data_t* spell = sim.dbc->spell( e->id_ );
      std::string name          = report_decorators::decorated_spell_name( sim, *spell );
      os << "<td class=\"left\">" << name << "</td>\n";
    }

    if ( const hotfix::dbc_hotfix_entry_t* e = dynamic_cast<const hotfix::dbc_hotfix_entry_t*>( entry ) )
    {
      os << "<td class=\"left\">" << util::encode_html( e->field_name_ ) << "</td>\n";
      os << "<td class=\"left\">" << e->hotfix_value_ << "</td>\n";
      if ( e->orig_value_ != -std::numeric_limits<double>::max() &&
           util::round( e->orig_value_, 6 ) != util::round( e->dbc_value_, 6 ) )
      {
        os << "<td class=\"left\">" << e->dbc_value_ << "</td>\n";
        os << "<td class=\"left\" style=\"color:red;\"><strong>Verification Failure ("
           << e->orig_value_ << ")</strong></td>";
      }
      else
      {
        os << "<td colspan=\"2\" class=\"left\">" << e->dbc_value_ << "</td>\n";
      }
      os << "</tr>\n";
    }
  }
  os << "</table>\n";
  os << "</div>\n";
  os << "</div>\n";
}

void print_html_overrides( report::sc_html_stream& os, const sim_t& sim )
{
  const auto& entries = sim.dbc_override->override_entries( sim.dbc->ptr );
  if ( entries.empty() )
  {
    return;
  }

  os << "<div class=\"section\">\n"
     << "<h2 class=\"toggle\">Current simulator-wide DBC data overrides</h2>\n"
     << "<div class=\"toggle-content hide\">\n"
     << "<table class=\"sc even\">\n"
     << "<thead>\n"
     << "<tr>\n"
     << "<th class=\"left\">Spell / Effect</th>\n"
     << "<th class=\"left\">Field</th>\n"
     << "<th class=\"left\">Override Value</th>\n"
     << "<th class=\"left\">DBC Value</th>\n"
     << "</tr>\n"
     << "</thead>\n";
  for ( const auto& entry : entries )
  {
    os << "<tr>\n";
    if ( entry.type_ == dbc_override_t::OVERRIDE_SPELL )
    {
      const spell_data_t* spell = hotfix::find_spell( sim.dbc->spell( entry.id_ ), sim.dbc->ptr );
      std::string name = report_decorators::decorated_spell_name( sim, *spell );
      os << "<td class=\"left\">" << name << "</td>\n";
      os << "<td class=\"left\">" << util::encode_html( entry.field_ ) << "</td>\n";
      os << "<td class=\"left\">" << entry.value_ << "</td>\n";
      os << "<td class=\"left\">" << spell->get_field( entry.field_ )
         << "</td>\n";
    }
    else if ( entry.type_ == dbc_override_t::OVERRIDE_EFFECT )
    {
      const spelleffect_data_t* effect = hotfix::find_effect( sim.dbc->effect( entry.id_ ), sim.dbc->ptr );
      const spell_data_t* spell        = effect->spell();
      std::string name                 = report_decorators::decorated_spell_name( sim, *spell );
      name += " (effect#" + util::to_string( effect->index() + 1 ) + ")";
      os << "<td class=\"left\">" << name << "</td>\n";
      os << "<td class=\"left\">" << util::encode_html( entry.field_ ) << "</td>\n";
      os << "<td class=\"left\">" << entry.value_ << "</td>\n";
      os << "<td class=\"left\">" << effect->get_field( entry.field_ )
         << "</td>\n";
    }

    os << "</tr>\n";
  }
  os << "</table>\n";
  os << "</div>\n";
  os << "</div>\n";
}

void print_html_report_scripts( report::sc_html_stream& os )
{
  print_text_array( os, __html_report_script );
}

void print_html_head( report::sc_html_stream& os, const sim_t& sim )
{
  os << "<title>Simulationcraft Results</title>\n";
  os << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n";

  os << "<meta name=\"color-scheme\" content=\"light dark\">\n";

  // Default target for links. Lets links to wowhead/armory escape iframes (wowhead does not display in iframes at all)
  // Should have no effect on GUI reports or directly viewing the HTML report in a browser
  os << "<base target=\"_top\">\n";

  os << "<script type=\"text/javascript\">\n";
  print_text_array( os, __jquery_include );
  os << "\n</script>\n"
     << "<script type=\"text/javascript\">\n";
  print_text_array( os, __highcharts_include );
  os << "\n</script>\n";

  print_html_style( os, sim );

  js::sc_js_t highcharts_theme;
  highchart::theme( highcharts_theme, highchart::THEME_DEFAULT );
  os << "<script type=\"text/javascript\">\n"
     << "Highcharts.setOptions(";
  highcharts_theme.print( os );
  os << ");\n";

  // Scripts that deal with chart click events. These needs to be loaded first so they're available as the page opens to
  // the starting charts
  std::string open_details_from_chart =  // Expand and scroll to action details
    "function open_details_from_chart(e){"
      "var anchor=jQuery(e.point.id);"
      "if(!anchor.length){return false;}"
      "if(!anchor.hasClass('open')){anchor.click()}"
      "jQuery('html, body').animate({scrollTop:anchor.offset().top-jQuery(window).height()/3},300)"
    "}";
  std::string open_player_from_chart =  // Expand and scroll to player
    "function open_player_from_chart(e){"
      "var anchor=jQuery(e.point.id);"
      "if(!anchor.length){return false;}"
      "if(!anchor.hasClass('open')){anchor.click()}"
      "jQuery('html, body').animate({scrollTop:anchor.parent().offset().top-anchor.height()},300)"
    "}";
  std::string cycle_chart =  // Data cycle & populator, called on chart load, and on click
    "function cycle_chart(c){"
      "if(c.series.length!==0){c.options.__current=(c.options.__current+1)%c.options.__data.length}"
      "var cidx=c.options.__current;"
      "var cdata=c.options.__data[cidx];"
      "c.setTitle(cdata.title,cdata.subtitle,false);"
      "while(c.series.length!==0){c.series[0].remove(false)}"
      "for(var i=0;i<cdata.series.length;++i){c.addSeries(cdata.series[i],false)}"
      "c.redraw()"
    "}";
  std::string setup_cycle_title =  // Add delegated listener to allow clicking on chart title
    "function setup_cycle_chart(){"
      "cycle_chart(this);"
      "if(this.options.__data.length>1){"
        "var $section=jQuery(this.renderTo);"
        "$section.addClass('cycle-chart');"
        "$section.on('click','.highcharts-title',function(e){"
          "cycle_chart(Highcharts.charts[e.delegateTarget.dataset.highchartsChart]);"
        "});"
      "}"
    "}";
  os << open_player_from_chart << "\n"
     << open_details_from_chart << "\n"
     << cycle_chart << "\n"
     << setup_cycle_title << "\n"
     << "</script>\n";
}

void print_nothing_to_report( report::sc_html_stream& os, std::string_view reason )
{
  os << "<div id=\"notice\" class=\"section section-open\">\n"
     << "<h2>Nothing to report</h2>\n"
     << "<p>" << reason << "<p>\n"
     << "</div>\n\n";
}

void print_profilesets_chart( std::ostream& out, const sim_t& sim )
{
  size_t chart_id                              = 0;
  const profileset::profilesets_t& profilesets = *sim.profilesets;

  auto results      = profilesets.generate_sorted_profilesets();
  auto results_mean = profilesets.generate_sorted_profilesets( true );
  if ( results.size() != results_mean.size() )
  {
    const_cast<sim_t&>( sim ).errorf( "Entry count mismatch between Median chart (%d) and Mean chart (%d)",
                                      results.size(), results_mean.size() );
  }

  while ( chart_id * chart::MAX_PROFILESET_CHART_ENTRIES < profilesets.n_profilesets() )
  {
    highchart::bar_chart_t chart( "profileset-" + util::to_string( chart_id ), sim );
    chart::generate_profilesets_chart( chart, sim, chart_id, results, results_mean );

    out << chart.to_string();
    ++chart_id;
    //    inserted = false;
  }
}

void print_profilesets( std::ostream& out, const profileset::profilesets_t& profilesets, const sim_t& sim )
{
  if ( profilesets.n_profilesets() == 0 )
  {
    return;
  }

  out << "<div class=\"section\">\n";
  out << "<h2 class=\"toggle open\">Profile sets</h2>\n";
  out << "<div class=\"toggle-content\">\n";

  print_profilesets_chart( out, sim );

  out << "</div>";
  out << "</div>";
}

/* Main function building the html document and calling subfunctions
 */
void print_html_( report::sc_html_stream& os, sim_t& sim )
{
  // Set floating point formatting
  os.precision( sim.report_precision );
  os << std::fixed;

  os << "<!DOCTYPE html>\n\n";
  os << "<html>\n";

  os << "<head>\n";
  print_html_head( os, sim );
  os << "</head>\n\n";

  os << "<body>\n";

  print_html_errors( os, sim );

  // Prints div wrappers for help popups
  os << "<div id=\"active-help\">\n"
     << "<div id=\"active-help-dynamic\">\n"
     << "<div class=\"help-box\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";

  print_html_masthead( os, sim );

  print_html_beta_warning( os );

  print_html_hotfixes( os, sim );
  print_html_overrides( os, sim );

  if ( sim.simulation_length.sum() == 0 )
  {
    print_nothing_to_report( os, "Sum of all Simulation Durations is zero." );
  }

  size_t num_players = sim.players_by_name.size();

  if ( num_players > 1 || sim.report_raid_summary )
  {
    print_html_contents( os, sim );
  }

  if ( num_players > 1 || sim.report_raid_summary || !sim.raid_events_str.empty() )
  {
    print_html_raid_summary( os, sim );
    print_html_scale_factors( os, sim );
  }

  print_profilesets( os, *sim.profilesets, sim );

  // Report Players
  for ( auto& player : sim.players_by_name )
  {
    report::print_html_player( os, *player );

    // Pets
    if ( sim.report_pets_separately )
    {
      for ( auto& pet : player->pet_list )
      {
        if ( pet->summoned && !pet->quiet )
          report::print_html_player( os, *pet );
      }
    }
  }

  print_html_sim_summary( os, sim );

  if ( sim.report_raw_abilities )
    raw_ability_summary::print( os, sim );

  // Report Targets
  if ( sim.report_targets )
  {
    for ( auto& player : sim.targets_by_name )
    {
      report::print_html_player( os, *player );

      // Pets
      if ( sim.report_pets_separately )
      {
        for ( auto& pet : player->pet_list )
        {
          // if ( pet -> summoned )
          report::print_html_player( os, *pet );
        }
      }
    }
  }

  print_html_help_boxes( os, sim );

  // jQuery
  // The /1/ url auto-updates to the latest minified version
  // os << "<script type=\"text/javascript\"
  // src=\"http://ajax.googleapis.com/ajax/libs/jquery/1/jquery.min.js\"></script>\n";

  if ( sim.decorated_tooltips )
  {
    //Apply the prettification stuff only if its a single report
    if ( num_players > 1 )
    {
      os << R"(<script>var whTooltips = {colorLinks: false, iconizeLinks: false, renameLinks: false};</script>)";
    }
    else
    {
      os << R"(<script>var whTooltips = {colorLinks: true, iconizeLinks: true, renameLinks: true};</script>)";
    }

    os << std::endl << R"(<script type="text/javascript" src="https://wow.zamimg.com/widgets/power.js"></script>)";
  }

  if ( sim.hosted_html )
  {
    // Google Analytics
    os << std::endl << R"(<script type="text/javascript" src="https://www.simulationcraft.org/js/ga.js"></script>)";
  }

  os << "\n<script type=\"text/javascript\">\n";
  print_html_report_scripts( os );
  os << "\n</script>\n";

  os << "<script type=\"text/javascript\">\n";
  os << "jQuery( document ).ready( function( $ ) {\n";
  for ( const auto& data : sim.on_ready_chart_data )
  {
    os << data << std::endl;
  }
  os << "});\n";
  os << "</script>\n";
  os << "<script type=\"text/javascript\">\n";
  os << "__chartData = {\n";
  for ( const auto& entry : sim.chart_data )
  {
    os << "\"" + entry.first + "\": [\n";
    const std::vector<std::string>& data = entry.second;
    for ( size_t j = 0; j < data.size(); ++j )
    {
      os << data[ j ];
      if ( j < data.size() - 1 )
      {
        os << ", ";
        os << "\n";
      }
    }
    os << "],\n";
  }
  os << "};\n";

  os << "</script>\n"
     << "</body>\n\n"
     << "</html>\n";
}

}  // UNNAMED NAMESPACE ====================================================

namespace report
{
void print_html( sim_t& sim )
{
  if ( sim.html_file_str.empty() )
    return;

  report_timer_t t( "html report", stdout );
  if ( ! sim.profileset_enabled )
  {
    t.start();
  }

  // Setup file stream and open file
  report::sc_html_stream s;
  s.open( sim.html_file_str );
  if ( !s )
  {
    sim.errorf( "Failed to open html output file '%s'.", sim.html_file_str.c_str() );
    return;
  }

  // Print html report
  print_html_( s, sim );
}

}  // report

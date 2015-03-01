// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include "data/report_data.inc"

// Experimental Raw Ability Output for Blizzard to do comparisons
namespace raw_ability_summary {

double aggregate_damage( const std::vector<stats_t::stats_results_t>& result )
{
  double total = 0;
  for ( size_t i = 0; i < result.size(); i++ )
  {
    total  += result[ i ].fight_actual_amount.mean();
  }
  return total;
}

/* Find the first action id associated with a given stats object
 */
int find_id( stats_t* s )
{
  int id = 0;

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


void print_raw_action_damage( report::sc_html_stream& os, stats_t* s, player_t* p, int j, sim_t* sim )
{
  if ( s -> num_executes.mean() == 0 && s -> compound_amount == 0 && !sim -> debug )
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

  double direct_total = aggregate_damage( s -> direct_results );
  double tick_total = aggregate_damage( s -> tick_results );
  if ( direct_total > 0.0 || tick_total <= 0.0 )
  {
    os << "<tr";
    if ( j & 1 )
      os << " class=\"odd\"";
    os << ">\n";

    os.format(
      format,
      util::encode_html( p -> name() ).c_str(),
      util::encode_html( s -> player -> name() ).c_str(),
      s -> name_str.c_str(), "",
      id,
      direct_total,
      direct_total / s -> player -> collected_data.fight_length.mean(),
      s -> num_direct_results.mean() / ( s -> player -> collected_data.fight_length.mean() / 60.0 ),
      s -> direct_results[ RESULT_HIT  ].actual_amount.mean(),
      s -> direct_results[ RESULT_CRIT ].actual_amount.mean(),
      s -> num_executes.mean(),
      s -> num_direct_results.mean(),
      s -> direct_results[ RESULT_CRIT ].pct,
      s -> direct_results[ RESULT_MISS ].pct + s -> direct_results[ RESULT_DODGE  ].pct + s -> direct_results[ RESULT_PARRY  ].pct,
      s -> direct_results[ RESULT_GLANCE ].pct,
      s -> direct_results_detail[ FULLTYPE_HIT_BLOCK ].pct + s -> direct_results_detail[ FULLTYPE_HIT_CRITBLOCK ].pct +
      s -> direct_results_detail[ FULLTYPE_GLANCE_BLOCK ].pct + s -> direct_results_detail[ FULLTYPE_GLANCE_CRITBLOCK ].pct +
      s -> direct_results_detail[ FULLTYPE_CRIT_BLOCK ].pct + s -> direct_results_detail[ FULLTYPE_CRIT_CRITBLOCK ].pct,
      s -> total_intervals.mean(),
      s -> total_amount.mean(),
      s -> player -> collected_data.fight_length.mean() );
  }

  if ( tick_total > 0.0 )
  {
    os << "<tr";
    if ( j & 1 )
      os << " class=\"odd\"";
    os << ">\n";

    os.format(
      format,
      util::encode_html( p -> name() ).c_str(),
      util::encode_html( s -> player -> name() ).c_str(),
      s -> name_str.c_str(), " ticks",
      -id,
      tick_total,
      tick_total / sim -> max_time.total_seconds(),
      s -> num_ticks.mean() / sim -> max_time.total_minutes(),
      s -> tick_results[ RESULT_HIT  ].actual_amount.mean(),
      s -> tick_results[ RESULT_CRIT ].actual_amount.mean(),
      s -> num_executes.mean(),
      s -> num_ticks.mean(),
      s -> tick_results[ RESULT_CRIT ].pct,
      s -> tick_results[ RESULT_MISS ].pct + s -> tick_results[ RESULT_DODGE  ].pct + s -> tick_results[ RESULT_PARRY  ].pct,
      s -> tick_results[ RESULT_GLANCE ].pct,
      s -> tick_results_detail[ FULLTYPE_HIT_BLOCK ].pct + s -> tick_results_detail[ FULLTYPE_HIT_CRITBLOCK ].pct +
      s -> tick_results_detail[ FULLTYPE_GLANCE_BLOCK ].pct + s -> tick_results_detail[ FULLTYPE_GLANCE_CRITBLOCK ].pct +
      s -> tick_results_detail[ FULLTYPE_CRIT_BLOCK ].pct + s -> tick_results_detail[ FULLTYPE_CRIT_CRITBLOCK ].pct,
      s -> total_intervals.mean(),
      s -> total_amount.mean(),
      s -> player -> collected_data.fight_length.mean() );
  }

  for ( size_t i = 0, num_children = s -> children.size(); i < num_children; i++ )
  {
    print_raw_action_damage( os, s -> children[ i ], p, j, sim );
  }
}

void print( report::sc_html_stream& os, sim_t* sim )
{
  os << "<div id=\"raw-abilities\" class=\"section\">\n\n";
  os << "<h2 class=\"toggle\">Raw Ability Summary</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  // Abilities Section
  os << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th class=\"left small\">Character</th>\n"
     << "<th class=\"left small\">Unit</th>\n"
     << "<th class=\"small\"><a href=\"#help-ability\" class=\"help\">Ability</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-id\" class=\"help\">Id</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-total\" class=\"help\">Total</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-ipm\" class=\"help\">Imp/Min</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-impacts\" class=\"help\">Impacts</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-miss-pct\" class=\"help\">Avoid%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-glance-pct\" class=\"help\">G%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-block-pct\" class=\"help\">B%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-combined\" class=\"help\">Combined</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-duration\" class=\"help\">Duration</a></th>\n"
     << "</tr>\n";

  int count = 0;
  for ( size_t player_i = 0; player_i < sim -> players_by_name.size(); player_i++ )
  {
    player_t* p = sim -> players_by_name[ player_i ];
    for ( size_t i = 0; i < p -> stats_list.size(); ++i )
    {
      stats_t* s = p -> stats_list[ i ];
      if ( s -> parent == NULL )
        print_raw_action_damage( os, s, p, count++, sim );
    }

    for ( size_t pet_i = 0; pet_i < p -> pet_list.size(); ++pet_i )
    {
      pet_t* pet = p -> pet_list[ pet_i ];
      for ( size_t i = 0; i < pet -> stats_list.size(); ++i )
      {
        stats_t* s = pet -> stats_list[ i ];
        if ( s -> parent == NULL )
          print_raw_action_damage( os, s, p, count++, sim );
      }
    }
  }

  // closure
  os << "</table>\n";
  os << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";
}

} // raw_ability_summary


namespace { // UNNAMED NAMESPACE ==========================================

/* Prints a array of strings line by line. Necessary because we can't have big single text strings,
 * because VS limits the size of static objects.
 */
template <typename T, std::size_t N>
void print_text_array( report::sc_html_stream& os, const T ( & data )[N]  )
{
  for ( size_t i = 0; i < sizeof_array( data ); ++i )
  {
    os << data[ i ];
  }
}

struct html_style_t
{
  virtual void print_html_styles( report::sc_html_stream& os) = 0;
  virtual ~html_style_t() {}
};

struct mop_html_style_t : public html_style_t
{
  void print_html_styles( report::sc_html_stream& os) override
  {
    // Logo
    os << "<style type=\"text/css\" media=\"all\">\n"
        << "#logo {display: block;position: absolute;top: 7px;left: 15px;width: 375px;height: 153px;background: ";
    print_text_array( os, __logo );
    os << " 0px 0px no-repeat; }\n"
        << "</style>\n";

    // Rest
    print_text_array( os, __mop_stylesheet );
  }
};

typedef mop_html_style_t default_html_style_t;

// print_html_contents ======================================================

void print_html_contents( report::sc_html_stream& os, const sim_t* sim )
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

  os << "<div id=\"table-of-contents\" class=\"section grouped-first grouped-last\">\n"
     << "<h2 class=\"toggle\">Table of Contents</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

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
  for ( int i = 0; i < n; i++ )
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

    os << "<ul class=\"toc " << toc_class << "\">\n";

    int ci = 1;    // in-column counter
    if ( i == 0 )
    {
      os << "<li><a href=\"#raid-summary\">Raid Summary</a></li>\n";
      ci++;
      if ( sim -> scaling -> has_scale_factors() )
      {
        os << "<li><a href=\"#raid-scale-factors\">Scale Factors</a></li>\n";
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
        os << "<li><a href=\"#player" << p -> index << "\">" << html_name << "</a>";
        ci++;
        if ( sim -> report_pets_separately )
        {
          os << "\n<ul>\n";
          for ( size_t k = 0; k < sim -> players_by_name[ pi ] -> pet_list.size(); ++k )
          {
            pet_t* pet = sim -> players_by_name[ pi ] -> pet_list[ k ];
            if ( pet -> summoned )
            {
              html_name = pet -> name();
              util::encode_html( html_name );
              os << "<li><a href=\"#player" << pet -> index << "\">" << html_name << "</a></li>\n";
              ci++;
            }
          }
          os << "</ul>";
        }
        os << "</li>\n";
        pi++;
      }
      if ( pi == static_cast<int>( sim -> players_by_name.size() ) )
      {
        if ( ab == 0 )
        {
          os << "<li><a href=\"#auras-buffs\">Auras/Buffs</a></li>\n";
          ab = 1;
        }
        ci++;
        os << "<li><a href=\"#sim-info\">Simulation Information</a></li>\n";
        ci++;
        if ( sim -> report_raw_abilities )
        {
          os << "<li><a href=\"#raw-abilities\">Raw Ability Summary</a></li>\n";
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
            os << "<li><a href=\"#player" << p -> index << "\">"
               << util::encode_html( p -> name() ) << "</a></li>\n";
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

void print_html_sim_summary( report::sc_html_stream& os, const sim_t* sim, const sim_report_information_t& ri )
{
  os << "<div id=\"sim-info\" class=\"section\">\n";

  os << "<h2 class=\"toggle\">Simulation & Raid Information</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  os << "<table class=\"sc mt\">\n";

  os << "<tr class=\"left\">\n"
     << "<th>Iterations:</th>\n"
     << "<td>" << sim -> iterations << "</td>\n"
     << "</tr>\n";

  os << "<tr class=\"left\">\n"
     << "<th>Threads:</th>\n"
     << "<td>" << ( ( sim -> threads < 1 ) ? 1 : sim -> threads ) << "</td>\n"
     << "</tr>\n";

  os << "<tr class=\"left\">\n"
     << "<th>Confidence:</th>\n"
     << "<td>" << sim -> confidence * 100.0 << "%</td>\n"
     << "</tr>\n";

  os.format( "<tr class=\"left\">\n"
             "<th><a href=\"#help-fight-length\" class=\"help\">Fight Length%s:</a></th>\n"
             "<td>%.0f - %.0f ( %.1f )</td>\n"
             "</tr>\n",
             (sim -> fixed_time ? " (fixed time)" : ""),
             sim -> simulation_length.min(),
             sim -> simulation_length.max(),
             sim -> simulation_length.mean() );

  os << "<tr class=\"left\">\n"
     << "<td><h2>Performance:</h2></td>\n"
     << "<td></td>\n"
     << "</tr>\n";

  os.format(
    "<tr class=\"left\">\n"
    "<th>Total Events Processed:</th>\n"
    "<td>%lu</td>\n"
    "</tr>\n",
    sim -> event_mgr.total_events_processed );

  os.format(
    "<tr class=\"left\">\n"
    "<th>Max Event Queue:</th>\n"
    "<td>%ld</td>\n"
    "</tr>\n",
    ( long ) sim -> event_mgr.max_events_remaining );

  os.format(
    "<tr class=\"left\">\n"
    "<th>Sim Seconds:</th>\n"
    "<td>%.0f</td>\n"
    "</tr>\n",
    sim -> iterations * sim -> simulation_length.mean() );
  os.format(
    "<tr class=\"left\">\n"
    "<th>CPU Seconds:</th>\n"
    "<td>%.4f</td>\n"
    "</tr>\n",
    sim -> elapsed_cpu );
  os.format(
    "<tr class=\"left\">\n"
    "<th>Physical Seconds:</th>\n"
    "<td>%.4f</td>\n"
    "</tr>\n",
    sim -> elapsed_time );
  os.format(
    "<tr class=\"left\">\n"
    "<th>Speed Up:</th>\n"
    "<td>%.0f</td>\n"
    "</tr>\n",
    sim -> iterations * sim -> simulation_length.mean() / sim -> elapsed_cpu );

  os << "<tr class=\"left\">\n"
     << "<td><h2>Settings:</h2></td>\n"
     << "<td></td>\n"
     << "</tr>\n";

  os.format(
    "<tr class=\"left\">\n"
    "<th>World Lag:</th>\n"
    "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
    "</tr>\n",
    ( double )sim -> world_lag.total_millis(), ( double )sim -> world_lag_stddev.total_millis() );
  os.format(
    "<tr class=\"left\">\n"
    "<th>Queue Lag:</th>\n"
    "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
    "</tr>\n",
    ( double )sim -> queue_lag.total_millis(), ( double )sim -> queue_lag_stddev.total_millis() );

  if ( sim -> strict_gcd_queue )
  {
    os.format(
      "<tr class=\"left\">\n"
      "<th>GCD Lag:</th>\n"
      "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
      "</tr>\n",
      ( double )sim -> gcd_lag.total_millis(), ( double )sim -> gcd_lag_stddev.total_millis() );
    os.format(
      "<tr class=\"left\">\n"
      "<th>Channel Lag:</th>\n"
      "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
      "</tr>\n",
      ( double )sim -> channel_lag.total_millis(), ( double )sim -> channel_lag_stddev.total_millis() );
    os.format(
      "<tr class=\"left\">\n"
      "<th>Queue GCD Reduction:</th>\n"
      "<td>%.0f ms</td>\n"
      "</tr>\n",
      ( double )sim -> queue_gcd_reduction.total_millis() );
  }

  int sd_counter = 0;
  report::print_html_sample_data( os, sim, sim -> simulation_length, "Simulation Length", sd_counter, 2 );

  os << "</table>\n";

  // Left side charts: dps, gear, timeline, raid events

  os << "<div class=\"charts charts-left\">\n";
  // Timeline Distribution Chart
  if ( sim -> iterations > 1 && ! ri.timeline_chart.empty() )
  {
    os.format(
      "<a href=\"#help-timeline-distribution\" class=\"help\"><img src=\"%s\" alt=\"Timeline Distribution Chart\" /></a>\n",
      ri.timeline_chart.c_str() );
  }

  // Gear Charts
  for ( size_t i = 0; i < ri.gear_charts.size(); i++ )
  {
    os << "<img src=\"" << ri.gear_charts[ i ] << "\" alt=\"Gear Chart\" />\n";
  }

  // Raid Downtime Chart
  if ( !  ri.downtime_chart.empty() )
  {
    os.format(
      "<img src=\"%s\" alt=\"Raid Downtime Chart\" />\n",
      ri.downtime_chart.c_str() );
  }

  os << "</div>\n";

  // Right side charts: dpet
  os << "<div class=\"charts\">\n";

  for ( size_t i = 0; i < ri.dpet_charts.size(); i++ )
  {
    os.format(
      "<img src=\"%s\" alt=\"DPET Chart\" />\n",
      ri.dpet_charts[ i ].c_str() );
  }

  os << "</div>\n";


  // closure
  os << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";
}

// print_html_raid_summary ==================================================

void print_html_raid_summary( report::sc_html_stream& os, const sim_t* sim, const sim_report_information_t& ri )
{
  os << "<div id=\"raid-summary\" class=\"section section-open\">\n\n";

  os << "<h2 class=\"toggle open\">Raid Summary</h2>\n";

  os << "<div class=\"toggle-content\">\n";

  os << "<ul class=\"params\">\n";

  os.format(
    "<li><b>Raid Damage:</b> %.0f</li>\n",
    sim -> total_dmg.mean() );
  os.format(
    "<li><b>Raid DPS:</b> %.0f</li>\n",
    sim -> raid_dps.mean() );
  if ( sim -> total_heal.mean() > 0 )
  {
    os.format(
      "<li><b>Raid Heal+Absorb:</b> %.0f</li>\n",
      sim -> total_heal.mean() + sim -> total_absorb.mean() );
    os.format(
      "<li><b>Raid HPS+APS:</b> %.0f</li>\n",
      sim -> raid_hps.mean() + sim -> raid_aps.mean() );
  }
  os << "</ul><p>&#160;</p>\n";

  // Left side charts: dps, raid events
  os << "<div class=\"charts charts-left\">\n";

  for ( size_t i = 0; i < ri.dps_charts.size(); i++ )
  {
    os.format(
      "<map id='DPSMAP%d' name='DPSMAP%d'></map>\n", ( int )i, ( int )i );
    os.format(
      "<img id='DPSIMG%d' src=\"%s\" alt=\"DPS Chart\" />\n",
      ( int )i, ri.dps_charts[ i ].c_str() );
  }

  bool dtps_chart_printed = false;
  if ( sim -> num_enemies > 1 || ( ( ( sim -> num_tanks * 2 ) >= sim -> num_players ) && sim -> num_enemies == 1 ) )
  { // Put this chart on the left side to prevent blank space.
    dtps_chart_printed = true;
    for ( size_t i = 0; i < ri.dtps_charts.size(); i++ )
    {
      os.format(
        "<map id='DTPSMAP%d' name='DTPSMAP%d'></map>\n", (int)i, (int)i );
      os.format(
        "<img id='DTPSIMG%d' src=\"%s\" alt=\"DTPS Chart\" />\n",
        (int)i, ri.dtps_charts[i].c_str() );
    }
  }

  if ( ! sim -> raid_events_str.empty() )
  {
    os << "<table>\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th class=\"left\">Raid Event List</th>\n"
       << "</tr>\n";

    std::vector<std::string> raid_event_names = util::string_split( sim -> raid_events_str, "/" );
    for ( size_t i = 0; i < raid_event_names.size(); i++ )
    {
      os << "<tr";
      if ( ( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";

      os.format(
        "<th class=\"right\">%d</th>\n"
        "<td class=\"left\">%s</td>\n"
        "</tr>\n",
        ( int )i,
        raid_event_names[ i ].c_str() );
    }
    os << "</table>\n";
  }
  os << "</div>\n";

  // Right side charts: hps+aps
  os << "<div class=\"charts\">\n";

  if ( sim -> num_enemies > 1 )
  {
    for ( size_t i = 0; i < ri.priority_dps_charts.size(); i++ )
    {
      os.format(
        "<map id='PRIORITYDPSMAP%d' name='PRIORITYDPSMAP%d'></map>\n", (int)i, (int)i );
      os.format(
        "<img id='PRIORITYDPSIMG%d' src=\"%s\" alt=\"Priority DPS Chart\" />\n",
        (int)i, ri.priority_dps_charts[i].c_str() );
    }
  }
  else if ( !dtps_chart_printed )
  {
    for ( size_t i = 0; i < ri.dtps_charts.size(); i++ )
    {
      os.format(
        "<map id='DTPSMAP%d' name='DTPSMAP%d'></map>\n", (int)i, (int)i );
      os.format(
        "<img id='DTPSIMG%d' src=\"%s\" alt=\"DTPS Chart\" />\n",
        (int)i, ri.dtps_charts[i].c_str() );
    }
  }
  for ( size_t i = 0; i < ri.hps_charts.size(); i++ )
  {
    os.format(  "<map id='HPSMAP%d' name='HPSMAP%d'></map>\n", ( int )i, ( int )i );
    os.format(
      "<img id='HPSIMG%d' src=\"%s\" alt=\"HPS Chart\" />\n",
      ( int )i, ri.hps_charts[ i ].c_str() );
  }

  for ( size_t i = 0; i < ri.tmi_charts.size(); i++ )
  {
    os.format(
      "<map id='TMIMAP%d' name='TMIMAP%d'></map>\n", ( int )i, ( int )i );
    os.format(
      "<img id='TMIIMG%d' src=\"%s\" alt=\"TMI Chart\" />\n",
      ( int )i, ri.tmi_charts[ i ].c_str() );
  }

  // RNG chart
  if ( sim -> report_rng )
  {
    os << "<ul>\n";
    for ( size_t i = 0; i < sim -> players_by_name.size(); i++ )
    {
      player_t* p = sim -> players_by_name[ i ];
      double range = ( p -> collected_data.dps.percentile( 0.95 ) - p -> collected_data.dps.percentile( 0.05 ) ) / 2.0;
      os.format(
        "<li>%s: %.1f / %.1f%%</li>\n",
        util::encode_html( p -> name() ).c_str(),
        range,
        p -> collected_data.dps.mean() ? ( range * 100 / p -> collected_data.dps.mean() ) : 0 );
    }
    os << "</ul>\n";
  }

  os << "</div>\n";

  // closure
  os << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";

}

// print_html_raid_imagemaps ================================================

void print_html_raid_imagemap( report::sc_html_stream& os, sim_t* sim, size_t num, int dps )
{
  std::vector<player_t*> player_list;
  if ( dps == 1 )
    player_list = sim -> players_by_dps;
  else if ( dps == 2 )
    player_list = sim -> players_by_priority_dps;
  else if ( dps == 3 )
    player_list = sim -> players_by_hps;

  size_t start = num * MAX_PLAYERS_PER_CHART;
  size_t end = start + MAX_PLAYERS_PER_CHART;

  for ( size_t i = 0; i < player_list.size(); i++ )
  {
    player_t* p = player_list[ i ];
    if ( dps == 1 && p -> collected_data.dps.mean() <= 0 )
    { player_list.resize( i ); break; }
    else if ( dps == 2 && p -> collected_data.prioritydps.mean() <= 0 )
    { player_list.resize( i ); break; }
    else if ( dps == 3 && p -> collected_data.hps.mean() <= 0 )
    { player_list.resize( i ); break; }
  }

  if ( end > player_list.size() ) end = player_list.size();

  os << "n = [";
  for ( int i = ( int )end - 1; i >= ( int )start; i-- )
  {
    os << "\"player" << player_list[i] -> index << "\"";
    if ( i != ( int )start ) os << ", ";
  }
  os << "];\n";

  std::string imgid = str::format( "%sIMG%u", ( dps == 1 ) ? "DPS" : ( ( dps == 2 ) ? "PRIORITYDPS" : "HPS" ), as<unsigned>( num ) );
  std::string mapid = str::format( "%sMAP%u", ( dps == 1 ) ? "DPS" : ( ( dps == 2 ) ? "PRIORITYDPS" : "HPS" ), as<unsigned>( num ) );

  os.format(
    "u = document.getElementById('%s').src;\n"
    "getMap(u, n, function(mapStr) {\n"
    "document.getElementById('%s').innerHTML += mapStr;\n"
    "$j('#%s').attr('usemap','#%s');\n"
    "$j('#%s area').click(function(e) {\n"
    "anchor = $j(this).attr('href');\n"
    "target = $j(anchor).children('h2:first');\n"
    "open_anchor(target);\n"
    "});\n"
    "});\n\n",
    imgid.c_str(), mapid.c_str(), imgid.c_str(), mapid.c_str(), mapid.c_str() );
}

void print_html_raid_imagemaps( report::sc_html_stream& os, sim_t* sim, sim_report_information_t& ri )
{

  os << "<script type=\"text/javascript\">\n"
     << "var $j = jQuery.noConflict();\n"
     << "function getMap(url, names, mapWrite) {\n"
     << "$j.getJSON(url + '&chof=json&callback=?', function(jsonObj) {\n"
     << "var area = false;\n"
     << "var chart = jsonObj.chartshape;\n"
     << "var mapStr = '';\n"
     << "for (var i = 0; i < chart.length; i++) {\n"
     << "area = chart[i];\n"
     << "area.coords[2] = 523;\n"
     << "mapStr += \"\\n  <area name='\" + area.name + \"' shape='\" + area.type + \"' coords='\" + area.coords.join(\",\") + \"' href='#\" + names[i] + \"'  title='\" + names[i] + \"'>\";\n"
     << "}\n"
     << "mapWrite(mapStr);\n"
     << "});\n"
     << "}\n\n";

  for ( size_t i = 0; i < ri.dps_charts.size(); i++ )
  {
    print_html_raid_imagemap( os, sim, i, 1 );
  }

  for ( size_t i = 0; i < ri.priority_dps_charts.size(); i++ )
  {
    print_html_raid_imagemap( os, sim, i, 2 );
  }

  for ( size_t i = 0; i < ri.hps_charts.size(); i++ )
  {
    print_html_raid_imagemap( os, sim, i, 3 );
  }

  os << "</script>\n";
}

// print_html_scale_factors =================================================

void print_html_scale_factors( report::sc_html_stream& os, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;
  
  scale_metric_e sm = sim -> scaling -> scaling_metric;
  std::string sf = util::scale_metric_type_string( sm );
  std::string SF = sf;
  std::transform(SF.begin(), SF.end(), SF.begin(), toupper);
  
  os << "<div id=\"raid-scale-factors\" class=\"section grouped-first\">\n\n"
     << "<h2 class=\"toggle\">" 
     << SF
     << " Scale Factors ("
     << sf
     << " increase per unit stat)</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  os << "<table class=\"sc\">\n";

  // this next part is used to determine which columns to suppress
  std::vector<double> stat_effect_is_nonzero;

    //initialize accumulator to zero
  for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
  {
    stat_effect_is_nonzero.push_back( 0.0 );
  }

  // cycle through players
  for ( size_t i = 0, players = sim -> players_by_name.size(); i < players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    // add the absolute value of their stat weights to accumulator element
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      stat_effect_is_nonzero[ j ] += std::abs( p -> scaling[ sm ].get_stat( j ) );
    }
  }
  // end column suppression section

  player_e prev_type = PLAYER_NONE;

  for ( size_t i = 0, players = sim -> players_by_name.size(); i < players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if ( p -> type == HEALING_ENEMY )
    {
      continue;
    }

    if ( p -> type != prev_type )
    {
      prev_type = p -> type;

      os << "<tr>\n"
         << "<th class=\"left small\">Profile</th>\n";
      for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
      {
        if ( sim -> scaling -> stats.get_stat( j ) != 0 && stat_effect_is_nonzero[ j ] > 0 )
        {
          os << "<th class=\"small\">" << util::stat_type_abbrev( j ) << "</th>\n";
        }
      }
      os << "<th class=\"small\">wowhead</th>\n"
#if LOOTRANK_ENABLED == 1
         << "<th class=\"small\">lootrank</th>\n"
#endif
         << "</tr>\n";
    }

    os << "<tr";
    if ( ( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.format(
      "<td class=\"left small\">%s</td>\n",
      p -> name() );
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 && stat_effect_is_nonzero[ j ] > 0 )
      {
        if ( p -> scaling[ sm ].get_stat( j ) == 0 )
        {
          os << "<td class=\"small\">-</td>\n";
        }
        else
        {
          os.format(
            "<td class=\"small\">%.*f</td>\n",
            sim -> report_precision,
            p -> scaling[ sm ].get_stat( j ) );
        }
      }
    }
    os.format(
      "<td class=\"small\"><a href=\"%s\" class=\"ext\"> wowhead </a></td>\n",
      chart::gear_weights_wowhead( p )[sm].c_str() );
#if LOOTRANK_ENABLED == 1
    os.format(
      "<td class=\"small\"><a href=\"%s\"> lootrank</a></td>\n",
      p -> report_information.gear_weights_lootrank_link[ sm ].c_str() );
#endif
    os.format( "</tr>\n" );
  }
  os << "</table>\n";

  if ( sim -> iterations < 10000 )
    os << "<div class=\"alert\">\n"
       << "<h3>Warning</h3>\n"
       << "<p>Scale Factors generated using less than 10,000 iterations will vary from run to run.</p>\n"
       << "</div>\n";

  os << "</div>\n"
     << "</div>\n\n";
}

struct help_box_t
{
  std::string abbreviation, text;
};

/* Here simple help boxes with just a Title/Abbreviation and a Text can be added
 * the help-id will be a tokenized form of the abbreviation, spaces replaced with '-',
 * everything lowerspace and '%' replaced by '-pct'
 */
static const help_box_t help_boxes[] =
{
    { "APM", "Average number of actions executed per minute."},
    { "APS", "Average absorption per active player duration."},
    { "Constant Buffs", "Buffs received prior to combat and present the entire fight."},
    { "Count", "Average number of times an action is executed per iteration."},
    { "Crit", "Average crit damage."},
    { "Crit%", "Percentage of executes that resulted in critical strikes."},
    { "DPE", "Average damage per execution of an individual action."},
    { "DPET", "Average damage per execute time of an individual action; the amount of damage generated, divided by the time taken to execute the action, including time spent in the GCD."},
    { "DPR", "Average damage per resource point spent."},
    { "DPS", "Average damage per active player duration."},
    { "DPSE", "Average damage per fight duration."},
    { "DTPS", "Average damage taken per second per active player duration."},
    { "HPS", "Average healing (and absorption) per active player duration."},
    { "HPSE", "Average healing (and absorption) per fight duration."},
    { "HPE", "Average healing (or absorb) per execution of an individual action."},
    { "HPET", "Average healing (or absorb) per execute time of an individual action; the amount of healing generated, divided by the time taken to execute the action, including time spent in the GCD."},
    { "HPR", "Average healing (or absorb) per resource point spent."},
    { "Impacts", "Average number of impacts against a target (for attacks that hit multiple times per execute) per iteration."},
    { "Dodge%", "Percentage of executes that resulted in dodges."},
    { "DPS%", "Percentage of total DPS contributed by a particular action."},
    { "HPS%", "Percentage of total HPS (including absorb) contributed by a particular action."},
    { "Theck-Meloree Index", "Measure of damage smoothness, calculated over entire fight length. Related to max spike damage, 1k TMI is roughly equivalent to 1% of your health. TMI ignores external healing and absorbs. Lower is better."},
    { "TMI bin size", "Time bin size used to calculate TMI and MSD, in seconds."},
    { "Type", "Direct or Periodic damage." },
    { "Max Spike Damage Frequency", "This is roughly how many spikes as large as MSD Mean you take per iteration. Calculated from TMI and MSD values."},
    { "Dynamic Buffs", "Temporary buffs received during combat, perhaps multiple times."},
    { "Glance%", "Percentage of executes that resulted in glancing blows."},
    { "Block%", "Percentage of executes that resulted in blocking blows."},
    { "Id", "Associated spell-id for this ability."},
    { "Ability", "Name of the ability."},
    { "Total", "Total damage for this ability during the fight."},
    { "Hit", "Average non-crit damage."},
    { "Interval", "Average time between executions of a particular action."},
    { "Avg", "Average direct damage per execution."},
    { "Miss%", "Percentage of executes that resulted in misses, dodges or parries."},
    { "Origin", "The player profile from which the simulation script was generated. The profile must be copied into the same directory as this HTML file in order for the link to work."},
    { "Parry%", "Percentage of executes that resulted in parries."},
    { "RPS In", "Average primary resource points generated per second."},
    { "RPS Out", "Average primary resource points consumed per second."},
    { "Scale Factors", "Gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>Loss</b> per unit stat <b>decrease</b>."},
    { "Gear Amount", "Amount from raw gear, before class, attunement, or buff modifiers. Amount from hybrid primary stats (i.e. Agility/Intellect) shown in parentheses."},
    { "Stats Raid Buffed", "Amount after all static buffs have been accounted for. Dynamic buffs (i.e. trinkets, potions) not included."},
    { "Stats Unbuffed", "Amount after class modifiers and effects, but before buff modifiers."},
    { "Ticks", "Average number of periodic ticks per iteration. Spells that do not have a damage-over-time component will have zero ticks."},
    { "Ticks Crit", "Average crit tick damage."},
    { "Ticks Crit%", "Percentage of ticks that resulted in critical strikes."},
    { "Ticks Hit", "Average non-crit tick damage."},
    { "Ticks Miss%", "Percentage of ticks that resulted in misses, dodges or parries."},
    { "Ticks Uptime%", "Percentage of total time that DoT is ticking on target."},
    { "Ticks Avg", "Average damage per tick."},
    { "Timeline Distribution", "The simulated encounter's duration can vary based on the health of the target and variation in the raid DPS. This chart shows how often the duration of the encounter varied by how much time."},
    { "Waiting", "This is the percentage of time in which no action can be taken other than autoattacks. This can be caused by resource starvation, lockouts, and timers."},
    { "Scale Factor Ranking", "This row ranks the scale factors from highest to lowest, checking whether one scale factor is higher/lower than another with statistical significance."},
};

void print_html_help_boxes( report::sc_html_stream& os, sim_t* sim )
{
  os << "<!-- Help Boxes -->\n";

  for ( size_t i = 0; i < sizeof_array( help_boxes ); ++i )
  {
    const help_box_t& hb = help_boxes[ i ];
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
     << "<p>This is the range of TMI values containing " << sim -> confidence * 100 << "% of the data, roughly centered on the mean.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-tmiwin\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>TMI/MSD Window</h3>\n"
     << "<p>Window length used to calculate TMI and MSD, in seconds.</p>\n"
     << "</div>\n"
     << "</div>\n";

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    os << "<div id=\"help-msd" << sim -> actor_list[ i ] -> actor_index << "\">\n"
      << "<div class=\"help-box\">\n"
      << "<h3>Max Spike Damage</h3>\n"
      << "<p>Maximum amount of net damage taken in any " << sim -> actor_list[ i ] -> tmi_window << "-second period, expressed as a percentage of max health. Calculated independently for each iteration. "
      << "'MSD Min/Mean/Max' are the lowest/average/highest MSDs out of all iterations.</p>\n"
      << "</div>\n"
      << "</div>\n";
  }

  os << "<div id=\"help-error\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Error</h3>\n"
     << "<p>Estimator for the " << sim -> confidence * 100.0 << "% confidence interval.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-range\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Range</h3>\n"
     << "<p>This is the range of values containing " << sim -> confidence * 100 << "% of the data, roughly centered on the mean.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-fight-length\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Fight Length</h3>\n"
     << "<p>Fight Length: " << sim -> max_time.total_seconds() << "<br />\n"
     << "Vary Combat Length: " << sim -> vary_combat_length << "</p>\n"
     << "<p>Fight Length is the specified average fight duration. If vary_combat_length is set, the fight length will vary by +/- that portion of the value. See <a href=\"http://code.google.com/p/simulationcraft/wiki/Options#Combat_Length\" class=\"ext\">Combat Length</a> in the wiki for further details.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<!-- End Help Boxes -->\n";
}

// print_html_styles ========================================================

void print_html_styles( report::sc_html_stream& os, sim_t* )
{
  // First select the appropriate style
  std::shared_ptr<html_style_t> style = std::shared_ptr<html_style_t>( new default_html_style_t() );

  // Print
  style -> print_html_styles( os );
}

// print_html_masthead ======================================================

void print_html_masthead( report::sc_html_stream& os, sim_t* sim )
{
  // Begin masthead section
  os << "<div id=\"masthead\" class=\"section section-open\">\n\n";

  os.format(
    "<span id=\"logo\"></span>\n"
    "<h1><a href=\"%s\">SimulationCraft %s</a></h1>\n"
    "<h2>for World of Warcraft %s %s (build level %d)</h2>\n\n",
    "http://code.google.com/p/simulationcraft/",
    SC_VERSION, sim -> dbc.wow_version(), ( sim -> dbc.ptr ?
#if SC_BETA
        "BETA"
#else
        "PTR"
#endif
        : "Live" ), sim -> dbc.build_level() );

  time_t rawtime;
  time( &rawtime );

  os << "<ul class=\"params\">\n";
  os.format(
    "<li><b>Timestamp:</b> %s</li>\n",
    ctime( &rawtime ) );
  os.format(
    "<li><b>Iterations:</b> %d</li>\n",
    sim -> iterations );

  if ( sim -> vary_combat_length > 0.0 )
  {
    timespan_t min_length = sim -> max_time * ( 1 - sim -> vary_combat_length );
    timespan_t max_length = sim -> max_time * ( 1 + sim -> vary_combat_length );
    os.format(
      "<li class=\"linked\"><a href=\"#help-fight-length\" class=\"help\"><b>Fight Length:</b> %.0f - %.0f</a></li>\n",
      min_length.total_seconds(),
      max_length.total_seconds() );
  }
  else
  {
    os.format(
      "<li><b>Fight Length:</b> %.0f</li>\n",
      sim -> max_time.total_seconds() );
  }
  os.format(
    "<li><b>Fight Style:</b> %s</li>\n",
    sim -> fight_style.c_str() );
  os << "</ul>\n"
     << "<div class=\"clear\"></div>\n\n"
     << "</div>\n\n";
  // End masthead section
}

void print_html_errors( report::sc_html_stream& os, sim_t* sim )
{
  if ( ! sim -> error_list.empty() )
  {
    os << "<pre class=\"section section-open\" style=\"color: black; background-color: white; font-weight: bold;\">\n";

    for ( size_t i = 0; i < sim -> error_list.size(); i++ )
      os <<  sim -> error_list[ i ] << "\n";

    os << "</pre>\n\n";
  }
}

void print_html_beta_warning( report::sc_html_stream& os )
{
#if SC_BETA
  os << "<div id=\"notice\" class=\"section section-open\">\n"
     << "<h2>Beta Release</h2>\n"
     << "<ul>\n";

  for ( size_t i = 0; i < sizeof_array( report::beta_warnings ); ++i )
    os << "<li>" << report::beta_warnings[ i ] << "</li>\n";

  os << "</ul>\n"
     << "</div>\n\n";
#else
  (void)os;
#endif
}

void print_html_image_load_scripts( report::sc_html_stream& os )
{
  print_text_array( os, __image_load_script );
}

void print_html_head( report::sc_html_stream& os, sim_t* sim )
{
  os << "<title>Simulationcraft Results</title>\n";
  os << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n";
  if ( sim -> wowhead_tooltips )
  {
   os << "<script type=\"text/javascript\" src=\"http://static.wowhead.com/widgets/power.js\"></script>\n"
      << "<script>var wowhead_tooltips = { \"colorlinks\": true, \"iconizelinks\": true, \"renamelinks\": true }</script>\n";
  }

  print_html_styles( os, sim );
}

void print_nothing_to_report( report::sc_html_stream& os, const std::string& reason )
{
  os << "<div id=\"notice\" class=\"section section-open\">\n"
     << "<h2>Nothing to report</h2>\n"
     << "<p>" << reason << "<p>\n"
     << "</div>\n\n";
}

/* Main function building the html document and calling subfunctions
 */
void print_html_( report::sc_html_stream& os, sim_t* sim )
{
  // Set floating point formatting
  os.precision( sim -> report_precision );
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
     << "<a href=\"#\" class=\"close\"><span class=\"hide\">close</span></a>\n"
     << "</div>\n"
     << "</div>\n\n";

  print_html_masthead( os, sim );

  print_html_beta_warning( os );

  if ( sim -> simulation_length.sum() == 0 ) {
    print_nothing_to_report( os, "Sum of all Simulation Durations is zero." );
  }

  size_t num_players = sim -> players_by_name.size();

  if ( num_players > 1 || sim -> report_raid_summary )
  {
    print_html_contents( os, sim );
  }

  if ( num_players > 1 || sim -> report_raid_summary || ! sim -> raid_events_str.empty() )
  {
    print_html_raid_summary( os, sim, sim -> report_information );
    print_html_scale_factors( os, sim );
  }

  int k = 0; // Counter for both players and enemies, without pets.

  // Report Players
  for ( size_t i = 0; i < num_players; ++i )
  {
    report::print_html_player( os, sim -> players_by_name[ i ], k );

    // Pets
    if ( sim -> report_pets_separately )
    {
      std::vector<pet_t*>& pl = sim -> players_by_name[ i ] -> pet_list;
      for ( size_t j = 0; j < pl.size(); ++j )
      {
        pet_t* pet = pl[ j ];
        if ( pet -> summoned && !pet -> quiet )
          report::print_html_player( os, pet, 1 );
      }
    }
  }

  print_html_sim_summary( os, sim, sim -> report_information );

  if ( sim -> report_raw_abilities )
    raw_ability_summary::print( os, sim );

  // Report Targets
  if ( sim -> report_targets )
  {
    for ( size_t i = 0; i < sim -> targets_by_name.size(); ++i )
    {
      report::print_html_player( os, sim -> targets_by_name[ i ], k );
      ++k;

      // Pets
      if ( sim -> report_pets_separately )
      {
        std::vector<pet_t*>& pl = sim -> targets_by_name[ i ] -> pet_list;
        for ( size_t j = 0; j < pl.size(); ++j )
        {
          pet_t* pet = pl[ j ];
          //if ( pet -> summoned )
          report::print_html_player( os, pet, 1 );
        }
      }
    }
  }

  print_html_help_boxes( os, sim );

  // jQuery
  // The /1/ url auto-updates to the latest minified version
  //os << "<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1/jquery.min.js\"></script>\n";
  os << "<script type=\"text/javascript\" src=\"http://code.jquery.com/jquery-1.11.2.min.js\"></script>";

  if ( sim -> hosted_html )
  {
    // Google Analytics
    os << "<script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/ga.js\"></script>\n";
  }

  print_html_image_load_scripts( os );

  if ( num_players > 1 )
    print_html_raid_imagemaps( os, sim, sim -> report_information );

  os << "</body>\n\n"
     << "</html>\n";
}

} // UNNAMED NAMESPACE ====================================================

namespace report {

void print_html( sim_t* sim )
{
  if ( sim -> html_file_str.empty() ) return;

  // Setup file stream and open file
  report::sc_html_stream s;
  s.open( sim -> html_file_str );
  if ( ! s )
  {
    sim -> errorf( "Failed to open html output file '%s'.", sim -> html_file_str.c_str() );
    return;
  }

  report::generate_sim_report_information( sim, sim -> report_information );

  // Print html report
  print_html_( s, sim );
}

} // report

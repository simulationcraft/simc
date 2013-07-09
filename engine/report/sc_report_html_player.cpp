// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

// print_html_action_damage =================================================

double mean_damage( std::vector<stats_t::stats_results_t> result )
{
  double mean = 0;
  size_t count = 0;

  for ( size_t i = 0; i < result.size(); i++ )
  {
    mean  += result[ i ].actual_amount.sum();
    count += result[ i ].actual_amount.count();
  }

  if ( count > 0 )
    mean /= count;

  return mean;
}

void print_html_action_damage( report::sc_html_stream& os, stats_t* s, player_t* p, int j )
{
  int id = 0;

  os << "\t\t\t\t\t\t\t<tr";
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

  os << "\t\t\t\t\t\t\t\t<td class=\"left small\">";

  if ( id > 0 || p -> sim -> report_details )
  {
    std::string href = "#";

    if ( id > 0 )
    {
      href = "http://";
      href += ( p -> dbc.ptr ? "ptr" : "www" );
      href += ".wowhead.com/spell=";
      href += util::to_string( id );
      href += "?lvl=";
      href += util::to_string( p -> level );
    }

    const char* class_attr = "";
    if ( p -> sim -> report_details )
      class_attr = " class=\"toggle-details\"";

    os.printf( "<a href=\"%s\"%s>%s</a></td>\n",
               href.c_str(), class_attr, s -> name_str.c_str() );
  }
  else
    os.printf( "%s</td>\n", s -> name_str.c_str() );

  std::string compound_dps     = "";
  std::string compound_dps_pct = "";
  double cDPS    = s -> portion_aps.mean();
  double cDPSpct = s -> portion_amount;

  for ( size_t i = 0, num_children = s -> children.size(); i < num_children; i++ )
  {
    cDPS    += s -> children[ i ] -> portion_apse.mean();
    cDPSpct += s -> children[ i ] -> portion_amount;
  }

  if ( cDPS > s -> portion_aps.mean()  ) compound_dps     = "&nbsp;(" + util::to_string( cDPS, 0 ) + ")";
  if ( cDPSpct > s -> portion_amount ) compound_dps_pct = "&nbsp;(" + util::to_string( cDPSpct * 100, 1 ) + "%)";

  os.printf(
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f%s</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%%s</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.2fsec</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f%%</td>\n"
    "\t\t\t\t\t\t\t</tr>\n",
    s -> portion_aps.pretty_mean(), compound_dps.c_str(),
    s -> portion_amount * 100, compound_dps_pct.c_str(),
    s -> num_executes.pretty_mean(),
    s -> total_intervals.pretty_mean(),
    s -> ape,
    s -> apet,
    s -> direct_results[ RESULT_HIT  ].actual_amount.pretty_mean(),
    s -> direct_results[ RESULT_CRIT ].actual_amount.pretty_mean(),
    mean_damage( s -> direct_results ),
    s -> direct_results[ RESULT_CRIT ].pct,
    s -> direct_results[ RESULT_MISS ].pct +
    s -> direct_results[ RESULT_DODGE  ].pct +
    s -> direct_results[ RESULT_PARRY  ].pct,
    s -> direct_results[ RESULT_GLANCE ].pct,
    s -> direct_results[ RESULT_BLOCK  ].pct,
    s -> num_ticks.pretty_mean(),
    s -> tick_results[ RESULT_HIT  ].actual_amount.pretty_mean(),
    s -> tick_results[ RESULT_CRIT ].actual_amount.pretty_mean(),
    mean_damage( s -> tick_results ),
    s -> tick_results[ RESULT_CRIT ].pct,
    s -> tick_results[ RESULT_MISS ].pct +
    s -> tick_results[ RESULT_DODGE ].pct +
    s -> tick_results[ RESULT_PARRY ].pct,
    100 * s -> total_tick_time.mean() / p -> collected_data.fight_length.mean() );

  if ( p -> sim -> report_details )
  {
    std::string timeline_stat_aps_str                    = "";
    if ( ! s -> timeline_aps_chart.empty() )
    {
      timeline_stat_aps_str = "<img src=\"" + s -> timeline_aps_chart + "\" alt=\"" + ( s -> type == STATS_DMG ? "DPS" : "HPS" ) + " Timeline Chart\" />\n";
    }
    std::string aps_distribution_str                    = "";
    if ( ! s -> aps_distribution_chart.empty() )
    {
      aps_distribution_str = "<img src=\"" + s -> aps_distribution_chart + "\" alt=\"" + ( s -> type == STATS_DMG ? "DPS" : "HPS" ) + " Distribution Chart\" />\n";
    }
    os << "\t\t\t\t\t\t\t<tr class=\"details hide\">\n"
       << "\t\t\t\t\t\t\t\t<td colspan=\"21\" class=\"filler\">\n";

    // Stat Details
    os.printf(
      "\t\t\t\t\t\t\t\t\t<h4>Stats details: %s </h4>\n", s -> name_str.c_str() );

    os << "\t\t\t\t\t\t\t\t\t<table class=\"details\">\n"
       << "\t\t\t\t\t\t\t\t\t\t<tr>\n";

    os << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Type</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Executes</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Direct Results</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Ticks</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Tick Results</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Execute Time per Execution</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Tick Time per  Tick</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Actual Amount</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Total Amount</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Overkill %</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Amount per Total Time</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Amount per Total Execute Time</th>\n";

    os << "\t\t\t\t\t\t\t\t\t\t</tr>\n"
       << "\t\t\t\t\t\t\t\t\t\t<tr>\n";

    os.printf(
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%s</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.4f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.4f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n",
      util::stats_type_string( s -> type ),
      s -> num_executes.mean(),
      s -> num_direct_results.mean(),
      s -> num_ticks.mean(),
      s -> num_tick_results.mean(),
      s -> etpe,
      s -> ttpt,
      s -> actual_amount.mean(),
      s -> total_amount.mean(),
      s -> overkill_pct,
      s -> aps,
      s -> apet );
    os << "\t\t\t\t\t\t\t\t\t\t</tr>\n";
    os << "\t\t\t\t\t\t\t\t\t</table>\n";
    if ( ! s -> portion_aps.simple || ! s -> portion_apse.simple || ! s -> actual_amount.simple )
    {
      os << "\t\t\t\t\t\t\t\t\t<table class=\"details\">\n";
      int sd_counter = 0;
      report::print_html_sample_data( os, p -> sim, s -> actual_amount, "Actual Amount", sd_counter );

      report::print_html_sample_data( os, p -> sim, s -> portion_aps, "portion Amount per Second ( pAPS )", sd_counter );

      report::print_html_sample_data( os, p -> sim, s -> portion_apse, "portion Effective Amount per Second ( pAPSe )", sd_counter );

      os << "\t\t\t\t\t\t\t\t\t</table>\n";

      if ( ! s -> portion_aps.simple && p -> sim -> scaling -> has_scale_factors() )
      {
        int colspan = 0;
        os << "\t\t\t\t\t\t<table class=\"details\">\n";
        os << "\t\t\t\t\t\t\t<tr>\n"
           << "\t\t\t\t\t\t\t\t<th><a href=\"#help-scale-factors\" class=\"help\">?</a></th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p -> scales_with[ i ] )
          {
            os.printf(
              "\t\t\t\t\t\t\t\t<th>%s</th>\n",
              util::stat_type_abbrev( i ) );
            colspan++;
          }
        if ( p -> sim -> scaling -> scale_lag )
        {
          os << "\t\t\t\t\t\t\t\t<th>ms Lag</th>\n";
          colspan++;
        }
        os << "\t\t\t\t\t\t\t</tr>\n";
        os << "\t\t\t\t\t\t\t<tr>\n"
           << "\t\t\t\t\t\t\t\t<th class=\"left\">Scale Factors</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p -> scales_with[ i ] )
            os.printf(
              "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
              p -> sim -> report_precision,
              s -> scaling.get_stat( i ) );
        os << "\t\t\t\t\t\t\t</tr>\n";
        os << "\t\t\t\t\t\t\t<tr>\n"
           << "\t\t\t\t\t\t\t\t<th class=\"left\">Scale Deltas</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p -> scales_with[ i ] )
            os.printf(
              "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
              ( i == STAT_WEAPON_OFFHAND_SPEED || i == STAT_WEAPON_SPEED ) ? 2 : 0,
              p -> sim -> scaling -> stats.get_stat( i ) );
        os << "\t\t\t\t\t\t\t</tr>\n";
        os << "\t\t\t\t\t\t\t<tr>\n"
           << "\t\t\t\t\t\t\t\t<th class=\"left\">Error</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p -> scales_with[ i ] )
            os.printf(
              "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
              p -> sim -> report_precision,
              s -> scaling_error.get_stat( i ) );

        os << "\t\t\t\t\t\t\t</tr>\n";
        os << "\t\t\t\t\t\t</table>\n";

      }
    }

    os << "\t\t\t\t\t\t\t\t\t<table  class=\"details\">\n";
    if ( s -> num_direct_results.mean() > 0 )
    {
      // Direct Damage
      os << "\t\t\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Direct Results</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Count</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Pct</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Mean</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Min</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Max</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Average per Iteration</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Average Min</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Average Max</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Actual Amount</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Total Amount</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Overkill %</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t</tr>\n";
      int k = 0;
      for ( result_e i = RESULT_MAX; --i >= RESULT_NONE; )
      {
        if ( s -> direct_results[ i ].count.mean() )
        {
          os << "\t\t\t\t\t\t\t\t\t\t<tr";

          if ( k & 1 )
          {
            os << " class=\"odd\"";
          }
          k++;
          os << ">\n";

          os.printf(
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"left small\">%s</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f%%</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t</tr>\n",
            util::result_type_string( i ),
            s -> direct_results[ i ].count.mean(),
            s -> direct_results[ i ].pct,
            s -> direct_results[ i ].actual_amount.mean(),
            s -> direct_results[ i ].actual_amount.min(),
            s -> direct_results[ i ].actual_amount.max(),
            s -> direct_results[ i ].avg_actual_amount.mean(),
            s -> direct_results[ i ].avg_actual_amount.min(),
            s -> direct_results[ i ].avg_actual_amount.max(),
            s -> direct_results[ i ].fight_actual_amount.mean(),
            s -> direct_results[ i ].fight_total_amount.mean(),
            s -> direct_results[ i ].overkill_pct.mean() );
        }
      }

    }

    if ( s -> num_tick_results.mean() > 0 )
    {
      // Tick Damage
      os << "\t\t\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Tick Results</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Count</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Pct</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Mean</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Min</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Max</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Average per Iteration</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Average Min</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Average Max</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Actual Amount</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Total Amount</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<th class=\"small\">Overkill %</th>\n"
         << "\t\t\t\t\t\t\t\t\t\t</tr>\n";
      int k = 0;
      for ( result_e i = RESULT_MAX; --i >= RESULT_NONE; )
      {
        if ( s -> tick_results[ i ].count.mean() )
        {
          os << "\t\t\t\t\t\t\t\t\t\t<tr";
          if ( k & 1 )
          {
            os << " class=\"odd\"";
          }
          k++;
          os << ">\n";
          os.printf(
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"left small\">%s</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.1f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f%%</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.0f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t\t<td class=\"right small\">%.2f</td>\n"
            "\t\t\t\t\t\t\t\t\t\t</tr>\n",
            util::result_type_string( i ),
            s -> tick_results[ i ].count.mean(),
            s -> tick_results[ i ].pct,
            s -> tick_results[ i ].actual_amount.mean(),
            s -> tick_results[ i ].actual_amount.min(),
            s -> tick_results[ i ].actual_amount.max(),
            s -> tick_results[ i ].avg_actual_amount.mean(),
            s -> tick_results[ i ].avg_actual_amount.min(),
            s -> tick_results[ i ].avg_actual_amount.max(),
            s -> tick_results[ i ].fight_actual_amount.mean(),
            s -> tick_results[ i ].fight_total_amount.mean(),
            s -> tick_results[ i ].overkill_pct.mean() );
        }
      }


    }
    os << "\t\t\t\t\t\t\t\t\t</table>\n";

    os << "\t\t\t\t\t\t\t\t\t<div class=\"clear\"></div>\n";

    os.printf(
      "\t\t\t\t\t\t\t\t\t%s\n",
      timeline_stat_aps_str.c_str() );
    os.printf(
      "\t\t\t\t\t\t\t\t\t%s\n",
      aps_distribution_str.c_str() );

    os << "\t\t\t\t\t\t\t\t\t<div class=\"clear\"></div>\n";
    // Action Details
    std::vector<std::string> processed_actions;

    for ( size_t i = 0; i < s -> action_list.size(); i++ )
    {
      action_t* a = s -> action_list[ i ];

      bool found = false;
      size_t size_processed = processed_actions.size();
      for ( size_t k = 0; k < size_processed && !found; k++ )
        if ( processed_actions[ k ] == a -> name() )
          found = true;
      if ( found ) continue;
      processed_actions.push_back( a -> name() );

      os.printf(
        "\t\t\t\t\t\t\t\t\t<h4>Action details: %s </h4>\n", a -> name() );

      os.printf(
        "\t\t\t\t\t\t\t\t\t<div class=\"float\">\n"
        "\t\t\t\t\t\t\t\t\t\t<h5>Static Values</h5>\n"
        "\t\t\t\t\t\t\t\t\t\t<ul>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">id:</span>%i</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">school:</span>%s</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">resource:</span>%s</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">range:</span>%.1f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">travel_speed:</span>%.4f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">trigger_gcd:</span>%.4f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">min_gcd:</span>%.4f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">base_cost:</span>%.1f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">cooldown:</span>%.2f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">base_execute_time:</span>%.2f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">base_crit:</span>%.2f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">target:</span>%s</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">harmful:</span>%s</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">if_expr:</span>%s</li>\n"
        "\t\t\t\t\t\t\t\t\t\t</ul>\n"
        "\t\t\t\t\t\t\t\t\t</div>\n",
        a -> id,
        util::school_type_string( a-> school ),
        util::resource_type_string( a -> current_resource() ),
        a -> range,
        a -> travel_speed,
        a -> trigger_gcd.total_seconds(),
        a -> min_gcd.total_seconds(),
        a -> base_costs[ a -> current_resource() ],
        a -> cooldown -> duration.total_seconds(),
        a -> base_execute_time.total_seconds(),
        a -> base_crit,
        a -> target ? a -> target -> name() : "",
        a -> harmful ? "true" : "false",
        util::encode_html( a -> if_expr_str ).c_str() );

      // Spelldata
      if ( a -> data().ok() )
      {
        os.printf(
          "\t\t\t\t\t\t\t\t\t<div class=\"float\">\n"
          "\t\t\t\t\t\t\t\t\t\t<h5>Spelldata</h5>\n"
          "\t\t\t\t\t\t\t\t\t\t<ul>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">id:</span>%i</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">name:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">school:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">tooltip:</span><span class=\"tooltip\">%s</span></li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">description:</span><span class=\"tooltip\">%s</span></li>\n"
          "\t\t\t\t\t\t\t\t\t\t</ul>\n"
          "\t\t\t\t\t\t\t\t\t</div>\n"
          "\t\t\t\t\t\t\t\t\t<div class=\"float\">\n",
          a -> data().id(),
          a -> data().name_cstr(),
          util::school_type_string( a -> data().get_school_type() ),
          pretty_spell_text( a -> data(), a -> data().tooltip(), *p ).c_str(),
          util::encode_html( pretty_spell_text( a -> data(), a -> data().desc(), *p ) ).c_str() );
      }

      if ( a -> direct_power_mod || a -> base_dd_min || a -> base_dd_max )
      {
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<h5>Direct Damage</h5>\n"
          "\t\t\t\t\t\t\t\t\t\t<ul>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">may_crit:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">direct_power_mod:</span>%.6f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">base_dd_min:</span>%.2f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">base_dd_max:</span>%.2f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t</ul>\n",
          a -> may_crit ? "true" : "false",
          a -> direct_power_mod,
          a -> base_dd_min,
          a -> base_dd_max );
      }
      if ( a -> num_ticks )
      {
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<h5>Damage Over Time</h5>\n"
          "\t\t\t\t\t\t\t\t\t\t<ul>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">tick_may_crit:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">tick_zero:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">tick_power_mod:</span>%.6f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">base_td:</span>%.2f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">num_ticks:</span>%i</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">base_tick_time:</span>%.2f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">hasted_ticks:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">dot_behavior:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t</ul>\n",
          a -> tick_may_crit ? "true" : "false",
          a -> tick_zero ? "true" : "false",
          a -> tick_power_mod,
          a -> base_td,
          a -> num_ticks,
          a -> base_tick_time.total_seconds(),
          a -> hasted_ticks ? "true" : "false",
          util::dot_behavior_type_string( a -> dot_behavior ) );
      }
      // Extra Reporting for DKs
      if ( a -> player -> type == DEATH_KNIGHT )
      {
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<h5>Rune Information</h5>\n"
          "\t\t\t\t\t\t\t\t\t\t<ul>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">Blood Cost:</span>%d</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">Frost Cost:</span>%d</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">Unholy Cost:</span>%d</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">Runic Power Gain:</span>%.2f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t</ul>\n",
          a -> data().rune_cost() & 0x1,
          ( a -> data().rune_cost() >> 4 ) & 0x1,
          ( a -> data().rune_cost() >> 2 ) & 0x1,
          a -> rp_gain );
      }
      if ( a -> weapon )
      {
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<h5>Weapon</h5>\n"
          "\t\t\t\t\t\t\t\t\t\t<ul>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">normalized:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">weapon_power_mod:</span>%.6f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">weapon_multiplier:</span>%.2f</li>\n"
          "\t\t\t\t\t\t\t\t\t\t</ul>\n",
          a -> normalize_weapon_speed ? "true" : "false",
          a -> weapon_power_mod,
          a -> weapon_multiplier );
      }
      os << "\t\t\t\t\t\t\t\t\t</div>\n"
         << "\t\t\t\t\t\t\t\t\t<div class=\"clear\"></div>\n";
    }

    os << "\t\t\t\t\t\t\t\t</td>\n"
       << "\t\t\t\t\t\t\t</tr>\n";
  }
}

// print_html_action_resource ===============================================

int print_html_action_resource( report::sc_html_stream& os, stats_t* s, int j )
{
  for ( size_t i = 0; i < s -> player -> action_list.size(); ++i )
  {
    action_t* a = s -> player -> action_list[ i ];
    if ( a -> stats != s ) continue;
    if ( ! a -> background ) break;
  }

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( s -> resource_gain.actual[ i ] > 0 )
    {
      os << "\t\t\t\t\t\t\t<tr";
      if ( j & 1 )
      {
        os << " class=\"odd\"";
      }
      ++j;
      os << ">\n";
      os.printf(
        "\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
        "\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
        "\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n"
        "\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n"
        "\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n",
        s -> resource_gain.name(),
        util::inverse_tokenize( util::resource_type_string( i ) ).c_str(),
        s -> resource_gain.count[ i ],
        s -> resource_gain.actual[ i ],
        s -> resource_gain.actual[ i ] / s -> resource_gain.count[ i ] );
      os.printf(
        "\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n"
        "\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n",
        s->rpe[ i ],
        s->apr[ i ] );
      os << "\t\t\t\t\t\t\t</tr>\n";
    }
  }
  return j;
}

// print_html_gear ==========================================================

void print_html_gear ( report::sc_html_stream& os, player_t* p )
{
  os.printf(
    "\t\t\t\t\t\t<div class=\"player-section gear\">\n"
    "\t\t\t\t\t\t\t<h3 class=\"toggle\">Gear</h3>\n"
    "\t\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
    "\t\t\t\t\t\t\t\t<table class=\"sc\">\n"
    "\t\t\t\t\t\t\t\t\t<tr>\n"
    "\t\t\t\t\t\t\t\t\t\t<th>Source</th>\n"
    "\t\t\t\t\t\t\t\t\t\t<th>Slot</th>\n"
    "\t\t\t\t\t\t\t\t\t\t<th>Average Item Level: %.2f</th>\n"
    "\t\t\t\t\t\t\t\t\t</tr>\n",
    p -> avg_ilvl );

  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
  {
    item_t& item = p -> items[ i ];

    std::string domain = p -> dbc.ptr ? "ptr" : "www";
    std::string item_string;
    if ( item.active() )
    {
      std::string rel_str = "";
      if ( item.upgrade_level() ) rel_str = " rel=\"upgd=" + util::to_string( item.upgrade_level() ) + "\"";
      item_string = ! item.parsed.data.id ? item.options_str : "<a href=\"http://" + domain + ".wowhead.com/item=" + util::to_string( item.parsed.data.id ) + "\"" + rel_str + ">" + item.encoded_item() + "</a>";
    }
    else
    {
      item_string = "empty";
    }

    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">%s</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">%s</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      item.source_str.c_str(),
      util::inverse_tokenize( item.slot_name() ).c_str(),
      item_string.c_str() );
  }

  os << "\t\t\t\t\t\t\t\t</table>\n"
     << "\t\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t\t</div>\n";
}

// print_html_profile =======================================================

void print_html_profile ( report::sc_html_stream& os, player_t* a )
{
  if ( a -> collected_data.fight_length.mean() > 0 )
  {
    std::string profile_str;
    a -> create_profile( profile_str, SAVE_ALL );
    profile_str = util::encode_html( profile_str );
    util::replace_all( profile_str, "\n", "<br>" );

    os << "\t\t\t\t\t\t<div class=\"player-section profile\">\n"
       << "\t\t\t\t\t\t\t<h3 class=\"toggle\">Profile</h3>\n"
       << "\t\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
       << "\t\t\t\t\t\t\t\t<div class=\"subsection force-wrap\">\n"
       << "\t\t\t\t\t\t\t\t\t<p>" << profile_str << "</p>\n"
       << "\t\t\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t\t</div>\n";
  }
}


// print_html_stats =========================================================

void print_html_stats ( report::sc_html_stream& os, player_t* a )
{
  if ( a -> collected_data.fight_length.mean() > 0 )
  {
    int j = 1;

    os << "\t\t\t\t\t\t<div class=\"player-section stats\">\n"
       << "\t\t\t\t\t\t\t<h3 class=\"toggle\">Stats</h3>\n"
       << "\t\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
       << "\t\t\t\t\t\t\t\t<table class=\"sc\">\n"
       << "\t\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th></th>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th>Raid-Buffed</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th>Unbuffed</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th>Gear Amount</th>\n"
       << "\t\t\t\t\t\t\t\t\t</tr>\n";

    for ( attribute_e i = ATTRIBUTE_NONE; ++i < ATTRIBUTE_MAX; )
    {
      os.printf(
        "\t\t\t\t\t\t\t\t\t<tr%s>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">%s</th>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
        "\t\t\t\t\t\t\t\t\t</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        util::inverse_tokenize( util::attribute_type_string( i ) ).c_str(),
        a -> buffed.attribute[ i ],
        a -> get_attribute( i ),
        a -> initial.stats.attribute[ i ] );
      j++;
    }
    for ( resource_e i = RESOURCE_NONE; ++i < RESOURCE_MAX; )
    {
      if ( a -> resources.max[ i ] > 0 )
        os.printf(
          "\t\t\t\t\t\t\t\t\t<tr%s>\n"
          "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">%s</th>\n"
          "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
          "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
          "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
          "\t\t\t\t\t\t\t\t\t</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          util::inverse_tokenize( util::resource_type_string( i ) ).c_str(),
          a -> buffed.resource[ i ],
          a -> resources.max[ i ],
          0.0 );
      j++;
    }

    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Spell Power</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      a -> buffed.spell_power,
      a -> composite_spell_power( SCHOOL_MAX ) * a -> composite_spell_power_multiplier(),
      a -> initial.stats.spell_power );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Spell Hit</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.spell_hit,
      100 * a -> composite_spell_hit(),
      a -> initial.stats.hit_rating  );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Spell Crit</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.spell_crit,
      100 * a -> composite_spell_crit(),
      a -> initial.stats.crit_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Spell Haste</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * ( 1 / a -> buffed.spell_haste - 1 ),
      100 * ( 1 / a -> composite_spell_haste() - 1 ),
      a -> initial.stats.haste_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Spell Speed</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * ( 1 / a -> buffed.spell_speed - 1 ),
      100 * ( 1 / a -> composite_spell_speed() - 1 ),
      a -> initial.stats.haste_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">ManaReg per Second</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">0</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      a -> buffed.manareg_per_second,
      a -> mana_regen_per_second() );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Attack Power</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      a -> buffed.attack_power,
      a -> composite_melee_attack_power() * a -> composite_attack_power_multiplier(),
      a -> initial.stats.attack_power );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Melee Hit</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.attack_hit,
      100 * a -> composite_melee_hit(),
      a -> initial.stats.hit_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Melee Crit</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.attack_crit,
      100 * a -> composite_melee_crit(),
      a -> initial.stats.crit_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Melee Haste</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * ( 1 / a -> buffed.attack_haste - 1 ),
      100 * ( 1 / a -> composite_melee_haste() - 1 ),
      a -> initial.stats.haste_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Swing Speed</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * ( 1 / a -> buffed.attack_speed - 1 ),
      100 * ( 1 / a -> composite_melee_speed() - 1 ),
      a -> initial.stats.haste_rating );
    j++;
    if ( a -> dual_wield() )
    {
      os.printf(
        "\t\t\t\t\t\t\t\t\t<tr%s>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Expertise</th>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%% / %.2f%%</td>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%% / %.2f%% </td>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f </td>\n"
        "\t\t\t\t\t\t\t\t\t</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * a -> buffed.mh_attack_expertise,
        100 * a -> buffed.oh_attack_expertise,
        100 * a -> composite_melee_expertise( &( a -> main_hand_weapon ) ),
        100 * a -> composite_melee_expertise( &( a -> off_hand_weapon ) ),
        a -> initial.stats.expertise_rating );
    }
    else
    {
      os.printf(
        "\t\t\t\t\t\t\t\t\t<tr%s>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Expertise</th>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%% </td>\n"
        "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f </td>\n"
        "\t\t\t\t\t\t\t\t\t</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * a -> buffed.mh_attack_expertise,
        100 * a -> composite_melee_expertise( &( a -> main_hand_weapon ) ),
        a -> initial.stats.expertise_rating );
    }
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Armor</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      a -> buffed.armor,
      a -> composite_armor(),
      a -> initial.stats.armor );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Tank-Miss</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.miss,
      100 * ( a -> cache.miss() ),
      0.0  );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Tank-Dodge</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.dodge,
      100 * ( a -> composite_dodge() ),
      a -> initial.stats.dodge_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Tank-Parry</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.parry,
      100 * ( a -> composite_parry() ),
      a -> initial.stats.parry_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Tank-Block</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.block,
      100 * a -> composite_block(),
      a -> initial.stats.block_rating );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Tank-Crit</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100 * a -> buffed.crit,
      100 * a -> cache.crit_avoidance(),
      0.0 );
    j++;
    os.printf(
      "\t\t\t\t\t\t\t\t\t<tr%s>\n"
      "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">Mastery</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"right\">%.0f</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      ( j % 2 == 1 ) ? " class=\"odd\"" : "",
      100.0 * a -> buffed.mastery_value,
      100.0 * a -> cache.mastery_value(),
      a -> initial.stats.mastery_rating );
    j++;

    os << "\t\t\t\t\t\t\t\t</table>\n"
       << "\t\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t\t</div>\n";
  }
}


// print_html_talents_player ================================================

void print_html_talents( report::sc_html_stream& os, player_t* p )
{
  if ( p -> collected_data.fight_length.mean() > 0 )
  {
    os << "\t\t\t\t\t\t<div class=\"player-section talents\">\n"
       << "\t\t\t\t\t\t\t<h3 class=\"toggle\">Talents</h3>\n"
       << "\t\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
       << "\t\t\t\t\t\t\t\t<table class=\"sc\">\n"
       << "\t\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th>Level</th>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th></th>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th></th>\n"
       << "\t\t\t\t\t\t\t\t\t\t<th></th>\n"
       << "\t\t\t\t\t\t\t\t\t</tr>\n";

    for ( uint32_t row = 0; row < MAX_TALENT_ROWS; row++ )
    {
      os.printf(
        "\t\t\t\t\t\t\t\t\t<tr>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">%d</th>\n",
        ( row + 1 ) * 15 );
      for ( uint32_t col = 0; col < MAX_TALENT_COLS; col++ )
      {
        talent_data_t* t = talent_data_t::find( p -> type, row, col, p -> dbc.ptr );
        const char* name = ( t && t -> name_cstr() ) ? t -> name_cstr() : "none";
        if ( p -> talent_points.has_row_col( row, col ) )
        {
          os.printf(
            "\t\t\t\t\t\t\t\t\t\t<td class=\"filler\">%s</td>\n",
            name );
        }
        else
        {
          os.printf(
            "\t\t\t\t\t\t\t\t\t\t<td>%s</td>\n",
            name );
        }
      }
      os << "\t\t\t\t\t\t\t\t\t</tr>\n";
    }

    os << "\t\t\t\t\t\t\t\t</table>\n"
       << "\t\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t\t</div>\n";
  }
}


// print_html_player_scale_factors ==========================================

void print_html_player_scale_factors( report::sc_html_stream& os, sim_t* sim, player_t* p, player_processed_report_information_t& ri )
{

  if ( !p -> is_pet() )
  {
    if ( p -> sim -> scaling -> has_scale_factors() )
    {
      int colspan = 0; // Count stats
      for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          ++colspan;

      os << "\t\t\t\t\t\t<table class=\"sc mt\">\n";

      os << "\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t<th colspan=\"" << util::to_string( 1 + colspan ) << "\"><a href=\"#help-scale-factors\" class=\"help\">Scale Factors for " << p -> scales_over().name << "</a></th>\n"
         << "\t\t\t\t\t\t\t</tr>\n";

      os << "\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t<th></th>\n";
      for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
        {
          os.printf(
            "\t\t\t\t\t\t\t\t<th>%s</th>\n",
            util::stat_type_abbrev( i ) );
        }
      if ( p -> sim -> scaling -> scale_lag )
      {
        os << "\t\t\t\t\t\t\t\t<th>ms Lag</th>\n";
        colspan++;
      }
      os << "\t\t\t\t\t\t\t</tr>\n";
      os << "\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t<th class=\"left\">Scale Factors</th>\n";
      for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          os.printf(
            "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
            p -> sim -> report_precision,
            p -> scaling.get_stat( i ) );
      if ( p -> sim -> scaling -> scale_lag )
        os.printf(
          "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
          p -> sim -> report_precision,
          p -> scaling_lag );
      os << "\t\t\t\t\t\t\t</tr>\n";
      os << "\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t<th class=\"left\">Normalized</th>\n";
      for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          os.printf(
            "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
            p -> sim -> report_precision,
            p -> scaling_normalized.get_stat( i ) );
      os << "\t\t\t\t\t\t\t</tr>\n";
      os << "\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t<th class=\"left\">Scale Deltas</th>\n";
      for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          os.printf(
            "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
            ( i == STAT_WEAPON_OFFHAND_SPEED || i == STAT_WEAPON_SPEED ) ? 2 : 0,
            p -> sim -> scaling -> stats.get_stat( i ) );
      if ( p -> sim -> scaling -> scale_lag )
        os << "\t\t\t\t\t\t\t\t<td>100</td>\n";
      os << "\t\t\t\t\t\t\t</tr>\n";
      os << "\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t<th class=\"left\">Error</th>\n";
      for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        if ( p -> scales_with[ i ] )
          os.printf(
            "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
            p -> sim -> report_precision,
            p -> scaling_error.get_stat( i ) );
      if ( p -> sim -> scaling -> scale_lag )
        os.printf(
          "\t\t\t\t\t\t\t\t<td>%.*f</td>\n",
          p -> sim -> report_precision,
          p -> scaling_lag_error );
      os << "\t\t\t\t\t\t\t</tr>\n";

      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th>Gear Ranking</th>\n"
        "\t\t\t\t\t\t\t\t<td colspan=\"%i\" class=\"filler\">\n"
        "\t\t\t\t\t\t\t\t\t<ul class=\"float\">\n",
        colspan );
      if ( !ri.gear_weights_wowhead_std_link.empty() )
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<li><a href=\"%s\" class=\"ext\">wowhead</a></li>\n",
          ri.gear_weights_wowhead_std_link.c_str() );
      if ( !ri.gear_weights_wowhead_alt_link.empty() )
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<li><a href=\"%s\" class=\"ext\">wowhead (caps merged)</a></li>\n",
          ri.gear_weights_wowhead_alt_link.c_str() );
      if ( !ri.gear_weights_lootrank_link.empty() )
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<li><a href=\"%s\" class=\"ext\">lootrank</a></li>\n",
          ri.gear_weights_lootrank_link.c_str() );
      if ( !ri.gear_weights_wowupgrade_link.empty() )
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<li><a href=\"%s\" class=\"ext\">wowupgrade</a></li>\n",
          ri.gear_weights_wowupgrade_link.c_str() );
      os << "\t\t\t\t\t\t\t\t\t</ul>\n";
      os << "\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t</tr>\n";

      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th>Optimizers</th>\n"
        "\t\t\t\t\t\t\t\t<td colspan=\"%i\" class=\"filler\">\n"
        "\t\t\t\t\t\t\t\t\t<ul class=\"float\">\n"
        "\t\t\t\t\t\t\t\t\t\t<li><a href=\"%s\" class=\"ext\">wowreforge</a></li>\n"
        "\t\t\t\t\t\t\t\t\t</ul>\n"
        "\t\t\t\t\t\t\t\t</td>\n"
        "\t\t\t\t\t\t\t</tr>\n",
        colspan,
        ri.gear_weights_wowreforge_link.c_str() );

      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th><a href=\"#help-sf-ranking\" class=\"help\">Ranking</a></th>\n"
        "\t\t\t\t\t\t\t\t<td colspan=\"%i\" class=\"filler\">\n"
        "\t\t\t\t\t\t\t\t\t<ul class=\"float\">\n"
        "\t\t\t\t\t\t\t\t\t\t<li>",
        colspan );

      for ( size_t i = 0; i < p -> scaling_stats.size(); i++ )
      {
        if ( i > 0 )
        {
          if ( ( fabs( p -> scaling.get_stat( p -> scaling_stats[ i - 1 ] ) - p -> scaling.get_stat( p -> scaling_stats[ i ] ) )
                 > sqrt ( p -> scaling_compare_error.get_stat( p -> scaling_stats[ i - 1 ] ) * p -> scaling_compare_error.get_stat( p -> scaling_stats[ i - 1 ] ) / 4 + p -> scaling_compare_error.get_stat( p -> scaling_stats[ i ] ) * p -> scaling_compare_error.get_stat( p -> scaling_stats[ i ] ) / 4 ) * 2 ) )
            os << " > ";
          else
            os << " ~= ";
        }

        os.printf( "%s", util::stat_type_abbrev( p -> scaling_stats[ i ] ) );
      }
      os << "\t\t\t\t\t\t\t\t\t</ul>\n"
         << "\t\t\t\t\t\t\t\t</td>\n"
         << "\t\t\t\t\t\t\t</tr>\n";

      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th>Pawn string</th>\n"
        "\t\t\t\t\t\t\t\t<td onClick=\"HighlightText('pawn_std_%i');\" colspan=\"%i\" class=\"filler\" style=\"vertical-align: top;\">\n"
        "\t\t\t\t\t\t\t\t<div style=\"position: relative;\">\n"
        "\t\t\t\t\t\t\t\t<div style=\"position: absolute; overflow: hidden; width: 100%%; white-space: nowrap;\">\n"
        "\t\t\t\t\t\t\t\t\t<ul class=\"float\">\n"
        "\t\t\t\t\t\t\t\t\t\t<li><small id=\"pawn_std_%i\" title='%s'>%s</small></li>\n"
        "\t\t\t\t\t\t\t\t\t</ul>\n"
        "\t\t\t\t\t\t\t\t</div>\n"
        "\t\t\t\t\t\t\t\t</div>\n"
        "\t\t\t\t\t\t\t\t</td>\n"
        "\t\t\t\t\t\t\t</tr>\n",
        p -> index,
        colspan,
        p -> index,
        ri.gear_weights_pawn_std_string.c_str(),
        ri.gear_weights_pawn_std_string.c_str() );

      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th>Zero hit/exp</th>\n"
        "\t\t\t\t\t\t\t\t<td onClick=\"HighlightText('pawn_alt_%i');\" colspan=\"%i\" class=\"filler\" style=\"vertical-align: top;\">\n"
        "\t\t\t\t\t\t\t\t<div style=\"position: relative;\">\n"
        "\t\t\t\t\t\t\t\t<div style=\"position: absolute; overflow: hidden; width: 100%%; white-space: nowrap;\">\n"
        "\t\t\t\t\t\t\t\t\t<ul class=\"float\">\n"
        "\t\t\t\t\t\t\t\t\t\t<li><small id=\"pawn_alt_%i\" title='%s'>%s</small></li>\n"
        "\t\t\t\t\t\t\t\t\t</ul>\n"
        "\t\t\t\t\t\t\t\t</div>\n"
        "\t\t\t\t\t\t\t\t</div>\n"
        "\t\t\t\t\t\t\t\t</td>\n"
        "\t\t\t\t\t\t\t</tr>\n",
        p -> index,
        colspan,
        p -> index,
        ri.gear_weights_pawn_alt_string.c_str(),
        ri.gear_weights_pawn_alt_string.c_str() );

      os << "\t\t\t\t\t\t</table>\n";
      if ( sim -> iterations < 10000 )
      {
        os << "\t\t\t\t<div class=\"alert\">\n"
           << "\t\t\t\t\t<h3>Warning</h3>\n"
           << "\t\t\t\t\t<p>Scale Factors generated using less than 10,000 iterations may vary significantly from run to run.</p>\n<br>";
        // Scale factor warnings:
        if ( sim -> scaling -> scale_factor_noise > 0 &&
             sim -> scaling -> scale_factor_noise < p -> scaling_lag_error / fabs( p -> scaling_lag ) )
          os.printf( "<p>Player may have insufficient iterations (%d) to calculate scale factor for lag (error is >%.0f%% delta score)</p>\n",
                     sim -> iterations, sim -> scaling -> scale_factor_noise * 100.0 );
        for ( size_t i = 0; i < p -> scaling_stats.size(); i++ )
        {
          double value = p -> scaling.get_stat( p -> scaling_stats[ i ] );
          double error = p -> scaling_error.get_stat( p -> scaling_stats[ i ] );
          if ( sim -> scaling -> scale_factor_noise > 0 &&
               sim -> scaling -> scale_factor_noise < error / fabs( value ) )
            os.printf( "<p>Player may have insufficient iterations (%d) to calculate scale factor for stat %s (error is >%.0f%% delta score)</p>\n",
                       sim -> iterations, util::stat_type_string( p -> scaling_stats[ i ] ), sim -> scaling -> scale_factor_noise * 100.0 );
        }
        os   << "\t\t\t\t</div>\n";
      }
    }
  }
  os << "\t\t\t\t\t</div>\n"
     << "\t\t\t\t</div>\n";
}

// print_html_player_action_priority_list ===================================

void print_html_player_action_priority_list( report::sc_html_stream& os, sim_t* sim, player_t* p )
{
  os << "\t\t\t\t\t\t<div class=\"player-section action-priority-list\">\n"
     << "\t\t\t\t\t\t\t<h3 class=\"toggle\">Action Priority List</h3>\n"
     << "\t\t\t\t\t\t\t<div class=\"toggle-content hide\">\n";

  action_priority_list_t* alist = 0;

  for ( size_t i = 0; i < p -> action_list.size(); ++i )
  {
    action_t* a = p -> action_list[ i ];
    if ( a -> signature_str.empty() || ! a -> marker ) continue;

    if ( ! alist || a -> action_list != alist -> name_str )
    {
      if ( alist )
      {
        os << "\t\t\t\t\t\t\t\t</table>\n";
      }

      alist = p -> find_action_priority_list( a -> action_list );

      if ( ! alist -> used ) continue;

      std::string als = util::encode_html( ( alist -> name_str == "default" ) ? "Default action list" : ( "actions." + alist -> name_str ).c_str() );
      if ( ! alist -> action_list_comment_str.empty() )
        als += "&nbsp;<small><em>" + util::encode_html( alist -> action_list_comment_str.c_str() ) + "</small></em>";
      os.printf(
        "\t\t\t\t\t\t\t\t<table class=\"sc\">\n"
        "\t\t\t\t\t\t\t\t\t<tr>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"right\"></th>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"right\"></th>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">%s</th>\n"
        "\t\t\t\t\t\t\t\t\t</tr>\n"
        "\t\t\t\t\t\t\t\t\t<tr>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"right\">#</th>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"right\">count</th>\n"
        "\t\t\t\t\t\t\t\t\t\t<th class=\"left\">action,conditions</th>\n"
        "\t\t\t\t\t\t\t\t\t</tr>\n",
        als.c_str() );
    }

    if ( ! alist -> used ) continue;

    os << "\t\t\t\t\t\t\t\t<tr";
    if ( ( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    std::string as = util::encode_html( a -> signature -> action_.c_str() );
    if ( ! a -> signature -> comment_.empty() )
      as += "<br/><em><small>" + util::encode_html( a -> signature -> comment_.c_str() ) + "</small></em>";

    os.printf(
      "\t\t\t\t\t\t\t\t\t\t<th class=\"right\" style=\"vertical-align:top\">%c</th>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"left\" style=\"vertical-align:top\">%.2f</td>\n"
      "\t\t\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
      "\t\t\t\t\t\t\t\t\t</tr>\n",
      a -> marker,
      a -> total_executions / ( double ) sim -> iterations,
      as.c_str() );
  }

  if ( alist )
  {
    os << "\t\t\t\t\t\t\t\t</table>\n";
  }

  if ( ! p -> collected_data.action_sequence.empty() )
  {
    os << "\t\t\t\t\t\t\t\t<div class=\"subsection subsection-small\">\n"
       << "\t\t\t\t\t\t\t\t\t<h4>Sample Sequence</h4>\n"
       << "\t\t\t\t\t\t\t\t\t<div class=\"force-wrap mono\">\n";

    std::vector<std::string> targets;

    targets.push_back( "none" );
    targets.push_back( p -> target -> name() );

    for ( size_t i = 0; i < p -> collected_data.action_sequence.size(); ++i )
    {
      player_collected_data_t::action_sequence_data_t* data = p -> collected_data.action_sequence[ i ];
      if ( ! data -> action -> harmful ) continue;
      bool found = false;
      for ( size_t j = 0; j < targets.size(); ++j )
      {
        if ( targets[ j ] == data -> target -> name() )
        {
          found = true;
          break;
        }
      }
      if ( ! found ) targets.push_back( data -> target -> name() );
    }

    os << "\t\t\t\t\t\t\t\t\t\t<style type=\"text/css\" media=\"all\">\n";

    char colors[12][7] = { "eeeeee", "ffffff", "ff5555", "55ff55", "5555ff", "ffff55", "55ffff", "ff9999", "99ff99", "9999ff", "ffff99", "99ffff" };

    int j = 0;

    for ( size_t i = 0; i < targets.size(); ++i )
    {
      if ( j == 12 ) j = 2;
      os.printf(
        "\t\t\t\t\t\t\t\t\t\t\t.%s_seq_target_%s { color: #%s; }\n",
        p -> name(),
        targets[ i ].c_str(),
        colors[ j ] );
      j++;
    }

    os << "\t\t\t\t\t\t\t\t\t\t</style>\n";

    for ( size_t i = 0; i < p -> collected_data.action_sequence.size(); ++i )
    {
      player_collected_data_t::action_sequence_data_t* data = p -> collected_data.action_sequence[ i ];

      std::string targetname = ( data -> action -> harmful ) ? data -> target -> name() : "none";
      os.printf(
        "<span class=\"%s_seq_target_%s\" title=\"[%d:%02d] %s%s\n|",
        p -> name(),
        targetname.c_str(),
        ( int ) data -> time.total_minutes(),
        ( int ) data -> time.total_seconds() % 60,
        data -> action -> name(),
        ( targetname == "none" ? "" : " @ " + targetname ).c_str() );

      for ( resource_e j = RESOURCE_HEALTH; j < RESOURCE_MAX; ++j )
      {
        if ( ! p -> resources.is_infinite( j ) && data -> resource_snapshot[ j ] >= 0 )
        {
          if ( j == RESOURCE_HEALTH || j == RESOURCE_MANA )
            os.printf( " %d%%", ( int ) ( ( data -> resource_snapshot[ j ] / p -> resources.max[ j ] ) * 100 ) );
          else
            os.printf( " %.1f", data -> resource_snapshot[ j ] );
          os.printf( " %s |", util::resource_type_string( j ) );
        }
      }

      for ( size_t j = 0; j < data -> buff_list.size(); ++j )
      {
        buff_t* buff = data -> buff_list[ j ];
        if ( ! buff -> constant ) os.printf( "\n%s", buff -> name() );
      }

      os.printf( "\">%c</span>", data -> action -> marker );
    }

    os << "\n\t\t\t\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t\t\t\t</div>\n";
  }

  os << "\t\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t\t</div>\n";

}

// print_html_player_statistics =============================================

void print_html_player_statistics( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{
  // Statistics & Data Analysis

  os << "\t\t\t\t\t<div class=\"player-section gains\">\n"
     "\t\t\t\t\t\t<h3 class=\"toggle\">Statistics & Data Analysis</h3>\n"
     "\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
     "\t\t\t\t\t\t\t<table class=\"sc\">\n";

  int sd_counter = 0;
  report::print_html_sample_data( os, p -> sim, p -> collected_data.fight_length, "Fight Length", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dps, "DPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dpse, "DPS(e)", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dmg, "Damage", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.dtps, "DTPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.hps, "HPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.hpse, "HPS(e)", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.heal, "Heal", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.htps, "HTPS", sd_counter );
  report::print_html_sample_data( os, p -> sim, p -> collected_data.theck_meloree_index, "TMI", sd_counter );

  os << "\t\t\t\t\t\t\t\t<tr>\n"
     "\t\t\t\t\t\t\t\t<td>\n";
  if ( ! ri.timeline_dps_error_chart.empty() )
    os << "<img src=\"" << ri.timeline_dps_error_chart << "\" alt=\"Timeline DPS Error Chart\" />\n";

  if ( ! ri.dps_error_chart.empty() )
    os << "<img src=\"" << ri.dps_error_chart << "\" alt=\"DPS Error Chart\" />\n";

  os << "\t\t\t\t\t\t\t\t</td>\n"
     "\t\t\t\t\t\t\t\t</tr>\n"
     "\t\t\t\t\t\t\t</table>\n"
     "\t\t\t\t\t\t</div>\n"
     "\t\t\t\t\t</div>\n";
}

void print_html_gain( report::sc_html_stream& os, gain_t* g, std::array<double, RESOURCE_MAX>& total_gains, int& j, bool report_overflow = true )
{
  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( g -> actual[ i ] != 0 || g -> overflow[ i ] != 0 )
    {
      double overflow_pct = 100.0 * g -> overflow[ i ] / ( g -> actual[ i ] + g -> overflow[ i ] );
      os.tabs() << "<tr";
      if ( j & 1 )
      {
        os << " class=\"odd\"";
      }
      ++j;
      os << ">\n";
      ++os;
      os.tabs() << "<td class=\"left\">" << g -> name() << "</td>\n";
      os.tabs() << "<td class=\"left\">" << util::inverse_tokenize( util::resource_type_string( i ) ) << "</td>\n";
      os.tabs() << "<td class=\"right\">" << g -> count[ i ] << "</td>\n";
      os.tabs().printf( "<td class=\"right\">%.2f (%.2f%%)</td>\n",
                        g -> actual[ i ], g -> actual[ i ] ? g -> actual[ i ] / total_gains[ i ] * 100.0 : 0.0 );
      os.tabs() << "<td class=\"right\">" << g -> actual[ i ] / g -> count[ i ] << "</td>\n";

      if ( report_overflow )
      {
        os.tabs() << "<td class=\"right\">" << g -> overflow[ i ] << "</td>\n";
        os.tabs() << "<td class=\"right\">" << overflow_pct << "%</td>\n";
      }
      --os;
      os.tabs() << "</tr>\n";
    }
  }
}

void get_total_player_gains( player_t& p, std::array<double, RESOURCE_MAX>& total_gains )
{
  for ( size_t i = 0; i < p.gain_list.size(); ++i )
  {
    // Wish std::array had += operator overload!
    for ( size_t j = 0; j < total_gains.size(); ++j )
      total_gains[ j ] += p.gain_list[ i ] -> actual[ j ];
  }
}
// print_html_player_resources ==============================================

void print_html_player_resources( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{
  os.set_level( 4 );

  // Resources Section

  os.tabs() << "<div class=\"player-section gains\">\n";
  ++os;
  os.tabs() << "<h3 class=\"toggle open\">Resources</h3>\n";
  os.tabs() << "<div class=\"toggle-content\">\n";
  ++os;
  os.tabs() << "<table class=\"sc mt\">\n";
  ++os;
  os.tabs() << "<tr>\n";
  ++os;
  os.tabs() << "<th>Resource Usage</th>\n";
  os.tabs() << "<th>Type</th>\n";
  os.tabs() << "<th>Count</th>\n";
  os.tabs() << "<th>Total</th>\n";
  os.tabs() << "<th>Average</th>\n";
  os.tabs() << "<th>RPE</th>\n";
  os.tabs() << "<th>APR</th>\n";
  --os;
  os.tabs() << "</tr>\n";

  os.tabs() << "<tr>\n";
  ++os;
  os.tabs() << "<th class=\"left small\">" << util::encode_html( p -> name() ) << "</th>\n";
  os.tabs() << "<td colspan=\"4\" class=\"filler\"></td>\n";
  --os;
  os.tabs() << "</tr>\n";

  int k = 0;
  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    if ( s -> rpe_sum > 0 )
    {
      k = print_html_action_resource( os, s, k );
    }
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    bool first = true;

    for ( size_t i = 0; i < pet -> stats_list.size(); ++i )
    {
      stats_t* s = pet -> stats_list[ i ];
      if ( s -> rpe_sum > 0 )
      {
        if ( first )
        {
          first = false;
          os.tabs() << "<tr>\n";
          ++os;
          os.tabs() << "<th class=\"left small\">pet - " << util::encode_html( pet -> name_str ) << "</th>\n";
          os.tabs() << "<td colspan=\"4\" class=\"filler\"></td>\n";
          --os;
          os.tabs() << "</tr>\n";
        }
        k = print_html_action_resource( os, s, k );
      }
    }
  }

  --os;
  os.tabs() << "</table>\n";

  // Resource Gains Section
  os.tabs() << "<table class=\"sc\">\n";
  ++os;
  os.tabs() << "<tr>\n";
  ++os;
  os.tabs() << "<th>Resource Gains</th>\n";
  os.tabs() << "<th>Type</th>\n";
  os.tabs() << "<th>Count</th>\n";
  os.tabs() << "<th>Total</th>\n";
  os.tabs() << "<th>Average</th>\n";
  os.tabs() << "<th colspan=\"2\">Overflow</th>\n";
  --os;
  os.tabs() << "</tr>\n";

  {
    int j = 0;

    std::array<double, RESOURCE_MAX> total_player_gains = std::array<double, RESOURCE_MAX>();
    get_total_player_gains( *p, total_player_gains );
    for ( size_t i = 0; i < p -> gain_list.size(); ++i )
    {
      gain_t* g = p -> gain_list[ i ];
      print_html_gain( os, g, total_player_gains, j );
    }
    for ( size_t i = 0; i < p -> pet_list.size(); ++i )
    {
      pet_t* pet = p -> pet_list[ i ];
      if ( pet -> collected_data.fight_length.mean() <= 0 ) continue;
      bool first = true;
      std::array<double, RESOURCE_MAX> total_pet_gains = std::array<double, RESOURCE_MAX>();
      get_total_player_gains( *pet, total_pet_gains );
      for ( size_t i = 0; i < pet -> gain_list.size(); ++i )
      {
        gain_t* g = pet -> gain_list[ i ];
        if ( first )
        {
          bool found = false;
          for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
          {
            if ( g -> actual[ i ] != 0 || g -> overflow[ i ] != 0 )
            {
              found = true;
              break;
            }
          }
          if ( found )
          {
            first = false;
            os.tabs() << "<tr>\n";
            ++os;
            os.tabs() << "<th>pet - " << util::encode_html( pet -> name_str ) << "</th>\n";
            os.tabs() << "<td colspan=\"6\" class=\"filler\"></td>\n";
            --os;
            os.tabs() << "</tr>\n";
          }
        }
        print_html_gain( os, g, total_pet_gains, j );
      }
    }
  }
  --os;
  os.tabs() << "</table>\n";

  // Resource Consumption Section
  os.tabs() << "<table class=\"sc\">\n";
  ++os;
  os.tabs() << "<tr>\n";
  ++os;
  os.tabs() << "<th>Resource</th>\n";
  os.tabs() << "<th>RPS-Gain</th>\n";
  os.tabs() << "<th>RPS-Loss</th>\n";
  --os;
  os.tabs() << "</tr>\n";
  int j = 0;
  for ( resource_e rt = RESOURCE_NONE; rt < RESOURCE_MAX; ++rt )
  {
    double rps_gain = p -> resource_gained[ rt ] / p -> collected_data.fight_length.mean();
    double rps_loss = p -> resource_lost[ rt ] / p -> collected_data.fight_length.mean();
    if ( rps_gain <= 0 && rps_loss <= 0 )
      continue;

    os.tabs() << "<tr";
    if ( j & 1 )
    {
      os << " class=\"odd\"";
    }
    ++j;
    os << ">\n";
    ++os;
    os.tabs() << "<td class=\"left\">" << util::inverse_tokenize( util::resource_type_string( rt ) ) << "</td>\n";
    os.tabs() << "<td class=\"right\">" << rps_gain << "</td>\n";
    os.tabs() << "<td class=\"right\">" << rps_loss << "</td>\n";
    --os;
    os.tabs() << "</tr>\n";
  }
  --os;
  os.tabs() << "</table>\n";

  // Resource End Section
  os.tabs() << "<table class=\"sc\">\n";
  ++os;
  os.tabs() << "<tr>\n";
  ++os;
  os.tabs() << "<th>Combat End Resource</th>\n";
  os.tabs() << "<th> Mean </th>\n";
  os.tabs() << "<th> Min </th>\n";
  os.tabs() << "<th> Max </th>\n";
  --os;
  os.tabs() << "</tr>\n";
  j = 0;
  for ( resource_e rt = RESOURCE_NONE; rt < RESOURCE_MAX; ++rt )
  {
    if ( p -> resources.base[ rt ] <= 0 )
      continue;

    os.tabs() << "<tr";
    if ( j & 1 )
    {
      os << " class=\"odd\"";
    }
    ++j;
    os << ">\n";
    ++os;
    os.tabs() << "<td class=\"left\">" << util::inverse_tokenize( util::resource_type_string( rt ) ) << "</td>\n";
    os.tabs() << "<td class=\"right\">" << p -> collected_data.combat_end_resource[ rt ].mean() << "</td>\n";
    os.tabs() << "<td class=\"right\">" << p -> collected_data.combat_end_resource[ rt ].min() << "</td>\n";
    os.tabs() << "<td class=\"right\">" << p -> collected_data.combat_end_resource[ rt ].max() << "</td>\n";
    --os;
    os.tabs() << "</tr>\n";
  }
  --os;
  os.tabs() << "</table>\n";


  os.tabs() << "<div class=\"charts charts-left\">\n";
  ++os;
  for ( resource_e j = RESOURCE_NONE; j < RESOURCE_MAX; ++j )
  {
    // hack hack. don't display RESOURCE_RUNE_<TYPE> yet. only shown in tabular data.  WiP
    if ( j == RESOURCE_RUNE_BLOOD || j == RESOURCE_RUNE_UNHOLY || j == RESOURCE_RUNE_FROST ) continue;
    double total_gain = 0;
    for ( size_t i = 0; i < p -> gain_list.size(); ++i )
    {
      gain_t* g = p -> gain_list[ i ];
      if ( g -> actual[ j ] > 0 )
        total_gain += g -> actual[ j ];
    }

    if ( total_gain > 0 )
    {
      if ( ! ri.gains_chart[ j ].empty() )
      {
        os.tabs() << "<img src=\"" << ri.gains_chart[ j ] << "\" alt=\"Resource Gains Chart\" />\n";
      }
    }
  }
  --os;
  os.tabs() << "</div>\n";


  os.tabs() << "<div class=\"charts\">\n";
  ++os;
  for ( unsigned j = RESOURCE_NONE + 1; j < RESOURCE_MAX; j++ )
  {
    if ( p -> resources.max[ j ] > 0 && ! ri.timeline_resource_chart[ j ].empty() )
    {
      os.tabs() << "<img src=\"" << ri.timeline_resource_chart[ j ] << "\" alt=\"Resource Timeline Chart\" />\n";
    }
  }
  if ( p -> primary_role() == ROLE_TANK ) // Experimental, restrict to tanks for now
  {
    os.tabs() << "<img src=\"" << ri.health_change_chart << "\" alt=\"Health Change Timeline Chart\" />\n";
    os.tabs() << "<img src=\"" << ri.health_change_sliding_chart << "\" alt=\"Health Change Sliding Timeline Chart\" />\n";

    // Tmp Debug Visualization
    histogram tmi_hist;
    tmi_hist.create_histogram( p -> collected_data.theck_meloree_index, 50 );
    std::string dist_chart = chart::distribution( p -> sim -> print_styles, tmi_hist.data(), "TMI", p -> collected_data.theck_meloree_index.mean(), tmi_hist.min(), tmi_hist.max() );
    os.tabs() << "<img src=\"" << dist_chart << "\" alt=\"Theck meloree distribution Chart\" />\n";
  }
  --os;
  os.tabs() << "</div>\n";
  --os;
  os.tabs() << "<div class=\"clear\"></div>\n";

  os.tabs() << "</div>\n";
  --os;
  os.tabs() << "</div>\n";

  os.set_level( 0 );
}

// print_html_player_charts =================================================

void print_html_player_charts( report::sc_html_stream& os, sim_t* sim, player_t* p, player_processed_report_information_t& ri )
{
  size_t num_players = sim -> players_by_name.size();

  os << "\t\t\t\t<div class=\"player-section\">\n"
     << "\t\t\t\t\t<h3 class=\"toggle open\">Charts</h3>\n"
     << "\t\t\t\t\t<div class=\"toggle-content\">\n"
     << "\t\t\t\t\t\t<div class=\"charts charts-left\">\n";

  if ( ! ri.action_dpet_chart.empty() )
  {
    const char* fmt;
    if ( num_players == 1 )
      fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"Action DPET Chart\" />\n";
    else
      fmt = "\t\t\t\t\t\t\t<span class=\"chart-action-dpet\" title=\"Action DPET Chart\">%s</span>\n";
    os.printf( fmt, ri.action_dpet_chart.c_str() );
  }

  if ( ! ri.action_dmg_chart.empty() )
  {
    const char* fmt;
    if ( num_players == 1 )
      fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"Action Damage Chart\" />\n";
    else
      fmt = "\t\t\t\t\t\t\t<span class=\"chart-action-dmg\" title=\"Action Damage Chart\">%s</span>\n";
    os.printf( fmt, ri.action_dmg_chart.c_str() );
  }

  if ( ! ri.scaling_dps_chart.empty() )
  {
    const char* fmt;
    if ( num_players == 1 )
      fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"Scaling DPS Chart\" />\n";
    else
      fmt = "\t\t\t\t\t\t\t<span class=\"chart-scaling-dps\" title=\"Scaling DPS Chart\">%s</span>\n";
    os.printf( fmt, ri.scaling_dps_chart.c_str() );
  }

  sc_timeline_t timeline_dps_taken;
  p -> collected_data.timeline_dmg_taken.build_derivative_timeline( timeline_dps_taken );
  std::string timeline_dps_takenchart = chart::timeline( p, timeline_dps_taken.data(), "dps_taken", timeline_dps_taken.mean(), "FDD017" );
  if ( ! timeline_dps_takenchart.empty() )
  {
    os << "<img src=\"" << timeline_dps_takenchart << "\" alt=\"DPS Taken Timeline Chart\" />\n";
  }

  os << "\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t\t<div class=\"charts\">\n";

  if ( ! ri.reforge_dps_chart.empty() )
  {
    std::string fmt;
    if ( p -> sim -> reforge_plot -> reforge_plot_stat_indices.size() == 2 )
    {
      if ( ri.reforge_dps_chart.length() < 2000 )
      {
        if ( num_players == 1 )
          fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"Reforge DPS Chart\" />\n";
        else
          fmt = "\t\t\t\t\t\t\t<span class=\"chart-reforge-dps\" title=\"Reforge DPS Chart\">%s</span>\n";
      }
      else
        os << "<p> Reforge Chart: Can't display charts with more than 2000 characters.</p>\n";
    }
    else
    {
      if ( true )
        fmt = "\t\t\t\t\t\t\t%s";
      else
      {
        if ( num_players == 1 )
          fmt = "\t\t\t\t\t\t\t<iframe>%s</iframe>\n";
        else
          fmt = "\t\t\t\t\t\t\t<span class=\"chart-reforge-dps\" title=\"Reforge DPS Chart\">%s</span>\n";
      }
    }
    if ( ! fmt.empty() )
      os.printf( fmt.c_str(), ri.reforge_dps_chart.c_str() );
  }

  if ( ! ri.scale_factors_chart.empty() )
  {
    const char* fmt;
    if ( num_players == 1 )
      fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"Scale Factors Chart\" />\n";
    else
      fmt = "\t\t\t\t\t\t\t<span class=\"chart-scale-factors\" title=\"Scale Factors Chart\">%s</span>\n";
    os.printf( fmt, ri.scale_factors_chart.c_str() );
  }

  if ( ! ri.timeline_dps_chart.empty() )
  {
    const char* fmt;
    if ( num_players == 1 )
      fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"DPS Timeline Chart\" />\n";
    else
      fmt = "\t\t\t\t\t\t\t<span class=\"chart-timeline-dps\" title=\"DPS Timeline Chart\">%s</span>\n";
    os.printf( fmt, ri.timeline_dps_chart.c_str() );
  }

  std::string vengeance_timeline_chart = chart::timeline( p, p -> vengeance_timeline().data(), "vengeance", 0, "ff0000", static_cast<size_t>( p -> collected_data.fight_length.max() ) );
  if ( ! vengeance_timeline_chart.empty() )
  {
    os << "<img src=\"" << vengeance_timeline_chart << "\" alt=\"Vengeance Timeline Chart\" />\n";
  }

  if ( ! ri.distribution_dps_chart.empty() )
  {
    const char* fmt;
    if ( num_players == 1 )
      fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"DPS Distribution Chart\" />\n";
    else
      fmt = "\t\t\t\t\t\t\t<span class=\"chart-distribution-dps\" title=\"DPS Distribution Chart\">%s</span>\n";
    os.printf( fmt, ri.distribution_dps_chart.c_str() );
  }

  if ( ! ri.time_spent_chart.empty() )
  {
    const char* fmt;
    if ( num_players == 1 )
      fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"Time Spent Chart\" />\n";
    else
      fmt = "\t\t\t\t\t\t\t<span class=\"chart-time-spent\" title=\"Time Spent Chart\">%s</span>\n";
    os.printf( fmt, ri.time_spent_chart.c_str() );
  }
  
  for ( size_t i = 0; i < ri.timeline_stat_chart.size(); ++i )
  {
    if ( ri.timeline_stat_chart[ i ].length() > 0 )
    {
      const char* fmt;
      if ( num_players == 1 )
        fmt = "\t\t\t\t\t\t\t<img src=\"%s\" alt=\"Stat Chart\" />\n";
      else
        fmt = "\t\t\t\t\t\t\t<span class=\"chart-scaling-dps\" title=\"Stat Chart\">%s</span>\n";
      os.printf( fmt, ri.timeline_stat_chart[ i ].c_str() );
    }
  }

  os << "\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t\t<div class=\"clear\"></div>\n"
     << "\t\t\t\t\t</div>\n"
     << "\t\t\t\t</div>\n";
}

// This function MUST accept non-player buffs as well!
void print_html_player_buff( report::sc_html_stream& os, buff_t* b, int report_details, size_t i, bool constant_buffs = false )
{
  std::string buff_name;
  assert( b );
  if ( ! b ) return; // For release builds.

  if ( b -> player && b -> player -> is_pet() )
  {
    buff_name += b -> player -> name_str + '-';
  }
  buff_name += b -> name_str.c_str();

  os << "\t\t\t\t\t\t\t<tr";
  if ( i & 1 )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  if ( report_details )
    os.printf(
      "\t\t\t\t\t\t\t\t<td class=\"left\"><a href=\"#\" class=\"toggle-details\"%s>%s</a></td>\n",
      b -> data().ok() ? ( " rel=\"spell=" + util::to_string( b -> data().id() ) + "\"" ).c_str() : "",
      buff_name.c_str() );
  else
    os.printf(
      "\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n",
      buff_name.c_str() );
  if ( !constant_buffs )
    os.printf(
      "\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n"
      "\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n"
      "\t\t\t\t\t\t\t\t<td class=\"right\">%.1fsec</td>\n"
      "\t\t\t\t\t\t\t\t<td class=\"right\">%.1fsec</td>\n"
      "\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n",
      b -> avg_start.pretty_mean(),
      b -> avg_refresh.pretty_mean(),
      b -> start_intervals.pretty_mean(),
      b -> trigger_intervals.pretty_mean(),
      b -> uptime_pct.pretty_mean(),
      ( b -> benefit_pct.sum() > 0 ? b -> benefit_pct.pretty_mean() : b -> uptime_pct.pretty_mean() ) );

  os << "\t\t\t\t\t\t\t</tr>\n";


  if ( report_details )
  {
    os.printf(
      "\t\t\t\t\t\t\t<tr class=\"details hide\">\n"
      "\t\t\t\t\t\t\t<td colspan=\"7\" class=\"filler\">\n"
      "\t\t\t\t\t\t\t<table><tr>\n"
      "\t\t\t\t\t\t\t\t<td colspan=\"3\" valign=\"top\" class=\"filler\">\n"
      "\t\t\t\t\t\t\t\t\t<h4>Buff details</h4>\n"
      "\t\t\t\t\t\t\t\t\t<ul>\n"
      "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">buff initial source:</span>%s</li>\n"
      "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">cooldown name:</span>%s</li>\n"
      "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">max_stacks:</span>%.i</li>\n"
      "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">duration:</span>%.2f</li>\n"
      "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">cooldown:</span>%.2f</li>\n"
      "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
      "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">default_value:</span>%.2f</li>\n",
      b -> source ? util::encode_html( b -> source -> name() ).c_str() : "",
      b -> cooldown -> name_str.c_str(),
      b -> max_stack(),
      b -> buff_duration.total_seconds(),
      b -> cooldown -> duration.total_seconds(),
      b -> default_chance * 100,
      b -> default_value );



    if ( stat_buff_t* stat_buff = dynamic_cast<stat_buff_t*>( b ) )
    {
      os << "\t\t\t\t\t\t\t\t\t<h4>Stat Buff details</h4>\n"
         << "\t\t\t\t\t\t\t\t\t<ul>\n";

      for ( size_t i = 0; i < stat_buff -> stats.size(); ++i )
      {
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">stat:</span>%s</li>\n"
          "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">amount:</span>%.2f</li>\n",
          util::stat_type_string( stat_buff -> stats[ i ].stat ),
          stat_buff -> stats[ i ].amount );
      }
    }
    os << "\t\t\t\t\t\t\t\t\t</ul>\n";

    os << "\t\t\t\t\t\t\t\t\t<h4>Stack Uptimes</h4>\n"
       << "\t\t\t\t\t\t\t\t\t<ul>\n";
    for ( unsigned int j = 0; j < b -> stack_uptime.size(); j++ )
    {
      double uptime = b -> stack_uptime[ j ] -> uptime_sum.mean();
      if ( uptime > 0 )
      {
        os.printf(
          "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">%s_%d:</span>%.2f%%</li>\n",
          b -> name_str.c_str(), j,
          uptime * 100.0 );
      }
    }
    os << "\t\t\t\t\t\t\t\t\t</ul>\n";

    if ( b -> trigger_pct.mean() > 0 )
    {
      os << "\t\t\t\t\t\t\t\t\t<h4>Trigger Attempt Success</h4>\n"
         << "\t\t\t\t\t\t\t\t\t<ul>\n";
      os.printf(
        "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">trigger_pct:</span>%.2f%%</li>\n",
        b -> trigger_pct.mean() );
      os << "\t\t\t\t\t\t\t\t\t</ul>\n";
    }
    os  << "\t\t\t\t\t\t\t\t</td>\n";

    // Spelldata
    if ( b -> data().ok() )
    {
      os.printf(
        "\t\t\t\t\t\t\t\t<td colspan=\"3\" valign=\"top\" class=\"filler\">\n"
        "\t\t\t\t\t\t\t\t\t<h4>Spelldata details</h4>\n"
        "\t\t\t\t\t\t\t\t\t<ul>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">id:</span>%i</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">name:</span>%s</li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">tooltip:</span><span class=\"tooltip\">%s</span></li>\n"
        "\t\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">description:</span><span class=\"tooltip\">%s</span></li>\n"
        "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">max_stacks:</span>%.i</li>\n"
        "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">duration:</span>%.2f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">cooldown:</span>%.2f</li>\n"
        "\t\t\t\t\t\t\t\t\t\t<li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
        "\t\t\t\t\t\t\t\t\t\t</ul>\n"
        "\t\t\t\t\t\t\t\t\t</td>\n",
        b -> data().id(),
        b -> data().name_cstr(),
        b -> player ? util::encode_html( pretty_spell_text( b -> data(), b -> data().tooltip(), *b -> player ) ).c_str() : b -> data().tooltip(),
        b -> player ? util::encode_html( pretty_spell_text( b -> data(), b -> data().desc(), *b -> player ) ).c_str() : b -> data().desc(),
        b -> data().max_stacks(),
        b -> data().duration().total_seconds(),
        b -> data().cooldown().total_seconds(),
        b -> data().proc_chance() * 100 );
    }
    os << "\t\t\t\t\t\t\t\t</tr>";

    if ( ! b -> constant && ! b -> overridden && b -> sim -> buff_uptime_timeline )
    {
      std::string uptime_chart = chart::timeline( b -> player, b -> uptime_array.data(), "Average Uptime", 0, "ff0000", static_cast<size_t>( b -> sim -> simulation_length.max() ) );
      if ( ! uptime_chart.empty() )
      {
        os << "\t\t\t\t\t\t\t\t<tr><td colspan=\"7\" class=\"filler\"><img src=\"" << uptime_chart << "\" alt=\"Average Uptime Timeline Chart\" />\n</td></tr>\n";
      }
    }
    os << "\t\t\t\t\t\t\t</table></td>\n";

    os << "\t\t\t\t\t\t\t</tr>\n";
  }
}

// print_html_player_buffs ==================================================

void print_html_player_buffs( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{
  // Buff Section
  os << "\t\t\t\t<div class=\"player-section buffs\">\n"
     << "\t\t\t\t\t<h3 class=\"toggle open\">Buffs</h3>\n"
     << "\t\t\t\t\t<div class=\"toggle-content\">\n";

  // Dynamic Buffs table
  os << "\t\t\t\t\t\t<table class=\"sc mb\">\n"
     << "\t\t\t\t\t\t\t<tr>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"left\"><a href=\"#help-dynamic-buffs\" class=\"help\">Dynamic Buffs</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th>Start</th>\n"
     << "\t\t\t\t\t\t\t\t<th>Refresh</th>\n"
     << "\t\t\t\t\t\t\t\t<th>Interval</th>\n"
     << "\t\t\t\t\t\t\t\t<th>Trigger</th>\n"
     << "\t\t\t\t\t\t\t\t<th>Up-Time</th>\n"
     << "\t\t\t\t\t\t\t\t<th>Benefit</th>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  for ( size_t i = 0; i < ri.dynamic_buffs.size(); i++ )
  {
    buff_t* b = ri.dynamic_buffs[ i ];

    print_html_player_buff( os, b, p -> sim -> report_details, i );
  }
  os << "\t\t\t\t\t\t\t</table>\n";

  // constant buffs
  if ( !p -> is_pet() )
  {
    os << "\t\t\t\t\t\t\t<table class=\"sc\">\n"
       << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<th class=\"left\"><a href=\"#help-constant-buffs\" class=\"help\">Constant Buffs</a></th>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";
    for ( size_t i = 0; i < ri.constant_buffs.size(); i++ )
    {
      buff_t* b = ri.constant_buffs[ i ];

      print_html_player_buff( os, b, p -> sim->report_details, i, true );
    }
    os << "\t\t\t\t\t\t\t</table>\n";
  }


  os << "\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t</div>\n";

}

// print_html_player ========================================================

void print_html_player_description( report::sc_html_stream& os, sim_t* sim, player_t* p, int j )
{
  int num_players = ( int ) sim -> players_by_name.size();

  // Player Description
  os << "\t\t<div id=\"player" << p -> index << "\" class=\"player section";
  if ( num_players > 1 && j == 0 && ! sim -> scaling -> has_scale_factors() && p -> type != ENEMY && p -> type != ENEMY_ADD )
  {
    os << " grouped-first";
  }
  else if ( ( p -> type == ENEMY || p -> type == ENEMY_ADD ) && j == ( int ) sim -> targets_by_name.size() - 1 )
  {
    os << " final grouped-last";
  }
  else if ( num_players == 1 )
  {
    os << " section-open";
  }
  os << "\">\n";

  if ( ! p -> report_information.thumbnail_url.empty() )
  {
    os.printf(
      "\t\t\t<a href=\"%s\" class=\"toggle-thumbnail ext%s\"><img src=\"%s\" alt=\"%s\" class=\"player-thumbnail\"/></a>\n",
      p -> origin_str.c_str(), ( num_players == 1 ) ? "" : " hide",
      p -> report_information.thumbnail_url.c_str(), p -> name_str.c_str() );
  }

  os << "\t\t\t<h2 class=\"toggle";
  if ( num_players == 1 )
  {
    os << " open";
  }

  const std::string n = util::encode_html( p -> name() );
  if ( p -> collected_data.dps.mean() >= p -> collected_data.hps.mean() )
    os.printf( "\">%s&nbsp;:&nbsp;%.0f dps",
               n.c_str(),
               p -> collected_data.dps.mean() );
  else
    os.printf( "\">%s&nbsp;:&nbsp;%.0f hps",
               n.c_str(),
               p -> collected_data.hps.mean() );

  // if player tank, print extra metrics
  if ( p -> primary_role() == ROLE_TANK && p -> type != ENEMY )
    os.printf( ", %.0f dtps, %.1f TMI\n",
               p -> collected_data.dtps.mean(),
               p -> collected_data.theck_meloree_index.mean() );

  os << "</h2>\n";

  os << "\t\t\t<div class=\"toggle-content";
  if ( num_players > 1 )
  {
    os << " hide";
  }
  os << "\">\n";

  os << "\t\t\t\t<ul class=\"params\">\n";
  if ( p -> dbc.ptr )
  {
#ifdef SC_USE_PTR
    os << "\t\t\t\t\t<li><b>PTR activated</b></li>\n";
#else
    os << "\t\t\t\t\t<li><b>BETA activated</b></li>\n";
#endif
  }
  const char* pt;
  if ( p -> is_pet() )
    pt = util::pet_type_string( p -> cast_pet() -> pet_type );
  else
    pt = util::player_type_string( p -> type );
  os.printf(
    "\t\t\t\t\t<li><b>Race:</b> %s</li>\n"
    "\t\t\t\t\t<li><b>Class:</b> %s</li>\n",
    util::inverse_tokenize( p -> race_str ).c_str(),
    util::inverse_tokenize( pt ).c_str()
  );

  if ( p -> specialization() != SPEC_NONE )
    os.printf(
      "\t\t\t\t\t<li><b>Spec:</b> %s</li>\n",
      util::inverse_tokenize( dbc::specialization_string( p -> specialization() ) ).c_str() );

  os.printf(
    "\t\t\t\t\t<li><b>Level:</b> %d</li>\n"
    "\t\t\t\t\t<li><b>Role:</b> %s</li>\n"
    "\t\t\t\t\t<li><b>Position:</b> %s</li>\n"
    "\t\t\t\t</ul>\n"
    "\t\t\t\t<div class=\"clear\"></div>\n",
    p -> level, util::inverse_tokenize( util::role_type_string( p -> primary_role() ) ).c_str(), p -> position_str.c_str() );
}

// print_html_player_results_spec_gear ======================================

void print_html_player_results_spec_gear( report::sc_html_stream& os, sim_t* sim, player_t* p )
{
  const player_collected_data_t& cd = p -> collected_data;

  // Main player table
  os << "\t\t\t\t<div class=\"player-section results-spec-gear mt\">\n";
  if ( p -> is_pet() )
  {
    os << "\t\t\t\t\t<h3 class=\"toggle open\">Results</h3>\n";
  }
  else
  {
    os << "\t\t\t\t\t<h3 class=\"toggle open\">Results, Spec and Gear</h3>\n";
  }
  os << "\t\t\t\t\t<div class=\"toggle-content\">\n"
     << "\t\t\t\t\t\t<table class=\"sc\">\n"
     << "\t\t\t\t\t\t\t<tr>\n";
  // Damage
  if ( cd.dps.mean() > 0 )
    os << "\t\t\t\t\t\t\t\t<th><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-dpse\" class=\"help\">DPS(e)</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-error\" class=\"help\">DPS Error</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-range\" class=\"help\">DPS Range</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-dpr\" class=\"help\">DPR</a></th>\n";
  // Heal
  if ( cd.hps.mean() > 0 )
    os << "\t\t\t\t\t\t\t\t<th><a href=\"#help-dps\" class=\"help\">HPS</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-dpse\" class=\"help\">HPS(e)</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-error\" class=\"help\">HPS Error</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-range\" class=\"help\">HPS Range</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-dpr\" class=\"help\">HPR</a></th>\n";
  // Tank
  if ( p -> primary_role() == ROLE_TANK )
    os << "\t\t\t\t\t\t\t\t<th><a href=\"#help-dtps\" class=\"help\">DTPS</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-error\" class=\"help\">DTPS Error</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-range\" class=\"help\">DTPS Range</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-tmi\" class=\"help\">TMI</a></th>\n"
       << "\t\t\t\t\t\t\t\t<th><a href=\"#help-error\" class=\"help\">TMI Error</a></th>\n";
   

  os << "\t\t\t\t\t\t\t\t<th><a href=\"#help-rps-out\" class=\"help\">RPS Out</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th><a href=\"#help-rps-in\" class=\"help\">RPS In</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th>Primary Resource</th>\n"
     << "\t\t\t\t\t\t\t\t<th><a href=\"#help-waiting\" class=\"help\">Waiting</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th><a href=\"#help-apm\" class=\"help\">APM</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th>Active</th>\n"
     << "\t\t\t\t\t\t\t\t<th>Skill</th>\n"
     << "\t\t\t\t\t\t\t</tr>\n"
     << "\t\t\t\t\t\t\t<tr>\n";

  // Damage
  if ( cd.dps.mean() > 0 )
  {
    double range = ( p -> collected_data.dps.percentile( 0.95 ) - p -> collected_data.dps.percentile( 0.05 ) ) / 2;
    double dps_error = sim_t::distribution_mean_error( *sim, p -> collected_data.dps );
    os.printf(
      "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.2f / %.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.0f / %.1f%%</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.1f</td>\n",
      cd.dps.mean(),
      cd.dpse.mean(),
      dps_error,
      cd.dps.mean() ? dps_error * 100 / cd.dps.mean() : 0,
      range,
      cd.dps.mean() ? range / cd.dps.mean() * 100.0 : 0,
      p -> dpr );
  }
  // Heal
  if ( cd.hps.mean() > 0 )
  {
    double range = ( cd.hps.percentile( 0.95 ) - cd.hps.percentile( 0.05 ) ) / 2;
    double hps_error = sim_t::distribution_mean_error( *sim, p -> collected_data.hps );
    os.printf(
      "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.2f / %.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.0f / %.1f%%</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.1f</td>\n",
      cd.hps.mean(),
      cd.hpse.mean(),
      hps_error,
      cd.hps.mean() ? hps_error * 100 / cd.hps.mean() : 0,
      range,
      cd.hps.mean() ? range / cd.hps.mean() * 100.0 : 0,
      p -> hpr );
  }
  // Tank
  if ( p -> primary_role() == ROLE_TANK )
  {
    double dtps_range = ( cd.dtps.percentile( 0.95 ) - cd.dtps.percentile( 0.05 ) ) / 2;
    double dtps_error = sim_t::distribution_mean_error( *sim, p -> collected_data.dtps );
    double tmi_error = sim_t::distribution_mean_error( *sim, p -> collected_data.theck_meloree_index );
    os.printf(
      "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.2f / %.2f%%</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.0f / %.1f%%</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.5g</td>\n"
      "\t\t\t\t\t\t\t\t<td>%.3g / %.2f%%</td>\n",
      cd.dtps.mean(),
      dtps_error,
      cd.dtps.mean() ? dtps_error * 100 / cd.dtps.mean() : 0,
      dtps_range,
      cd.dtps.mean() ? dtps_range / cd.dtps.mean() * 100.0 : 0,
      cd.theck_meloree_index.mean(),
      tmi_error,
      cd.theck_meloree_index.mean() ? tmi_error * 100 / cd.theck_meloree_index.mean() : 0 );
  }
  os.printf(
    "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
    "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
    "\t\t\t\t\t\t\t\t<td>%s</td>\n"
    "\t\t\t\t\t\t\t\t<td>%.2f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td>%.1f</td>\n"
    "\t\t\t\t\t\t\t\t<td>%.1f%%</td>\n"
    "\t\t\t\t\t\t\t\t<td>%.0f%%</td>\n"
    "\t\t\t\t\t\t\t</tr>\n"
    "\t\t\t\t\t\t</table>\n",
    p -> rps_loss,
    p -> rps_gain,
    util::inverse_tokenize( util::resource_type_string( p -> primary_resource() ) ).c_str(),
    cd.fight_length.mean() ? 100.0 * cd.waiting_time.mean() / cd.fight_length.mean() : 0,
    cd.fight_length.mean() ? 60.0 * cd.executed_foreground_actions.mean() / cd.fight_length.mean() : 0,
    sim -> simulation_length.mean() ? cd.fight_length.mean() / sim -> simulation_length.mean() * 100.0 : 0,
    p -> initial.skill * 100.0 );

  // Spec and gear
  if ( ! p -> is_pet() )
  {
    os << "\t\t\t\t\t\t<table class=\"sc mt\">\n";
    if ( ! p -> origin_str.empty() )
    {
      std::string html = util::encode_html( p -> origin_str );
      std::string enc_url = html;
      util::urlencode( enc_url );
      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th><a href=\"#help-origin\" class=\"help\">Origin</a></th>\n"
        "\t\t\t\t\t\t\t\t<td><a href=\"%s\" class=\"ext\">%s</a></td>\n"
        "\t\t\t\t\t\t\t</tr>\n",
        p -> origin_str.c_str(),
        enc_url.c_str() );
    }
    if ( ! p -> talents_str.empty() )
    {
      std::string enc_url = util::encode_html( p -> talents_str );
      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th>Talents</th>\n"
        "\t\t\t\t\t\t\t\t<td><a href=\"%s\" class=\"ext\">%s</a></td>\n"
        "\t\t\t\t\t\t\t</tr>\n",
        enc_url.c_str(),
        enc_url.c_str() );
    }

    // Glyphs
    if ( !p -> glyph_list.empty() )
    {
      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th>Glyphs</th>\n"
        "\t\t\t\t\t\t\t\t<td>\n"
        "\t\t\t\t\t\t\t\t\t<ul class=\"float\">\n" );
      for ( size_t i = 0; i < p -> glyph_list.size(); ++i )
      {
        os << "\t\t\t\t\t\t\t\t\t\t<li>" << p -> glyph_list[ i ] -> name_cstr() << "</li>\n";
      }
      os << "\t\t\t\t\t\t\t\t\t</ul>\n"
         << "\t\t\t\t\t\t\t\t</td>\n"
         << "\t\t\t\t\t\t\t</tr>\n";
    }

    // Professions
    if ( ! p -> professions_str.empty() )
    {
      os.printf(
        "\t\t\t\t\t\t\t<tr class=\"left\">\n"
        "\t\t\t\t\t\t\t\t<th>Professions</th>\n"
        "\t\t\t\t\t\t\t\t<td>\n"
        "\t\t\t\t\t\t\t\t\t<ul class=\"float\">\n" );
      for ( profession_e i = PROFESSION_NONE; i < PROFESSION_MAX; ++i )
      {
        if ( p -> profession[ i ] <= 0 )
          continue;

        os << "\t\t\t\t\t\t\t\t\t\t<li>" << util::profession_type_string( i ) << ": " << p -> profession[ i ] << "</li>\n";
      }
      os << "\t\t\t\t\t\t\t\t\t</ul>\n"
         << "\t\t\t\t\t\t\t\t</td>\n"
         << "\t\t\t\t\t\t\t</tr>\n";
    }

    os << "\t\t\t\t\t\t</table>\n";
  }
}

// print_html_player_abilities ==============================================

void print_html_player_abilities( report::sc_html_stream& os, sim_t* sim, player_t* p )
{
  // Abilities Section
  os << "\t\t\t\t<div class=\"player-section\">\n"
     << "\t\t\t\t\t<h3 class=\"toggle open\">Abilities</h3>\n"
     << "\t\t\t\t\t<div class=\"toggle-content\">\n"
     << "\t\t\t\t\t\t<table class=\"sc\">\n"
     << "\t\t\t\t\t\t\t<tr>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"left small\">Damage Stats</th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-dps-pct\" class=\"help\">DPS%</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-dpe\" class=\"help\">DPE</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-dpet\" class=\"help\">DPET</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-avg\" class=\"help\">Avg</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-miss-pct\" class=\"help\">Avoid%</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-glance-pct\" class=\"help\">G%</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-block-pct\" class=\"help\">B%</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks\" class=\"help\">Ticks</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-hit\" class=\"help\">T-Hit</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-crit\" class=\"help\">T-Crit</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-avg\" class=\"help\">T-Avg</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-crit-pct\" class=\"help\">T-Crit%</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-miss-pct\" class=\"help\">T-Avoid%</a></th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"small\"><a href=\"#help-ticks-uptime\" class=\"help\">Up%</a></th>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  os << "\t\t\t\t\t\t\t<tr>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"left small\">" << util::encode_html( p -> name() ) << "</th>\n"
     << "\t\t\t\t\t\t\t\t<th class=\"right small\">" << util::to_string( p -> collected_data.dps.mean(), 0 ) << "</th>\n"
     << "\t\t\t\t\t\t\t\t<td colspan=\"19\" class=\"filler\"></td>\n"
     << "\t\t\t\t\t\t\t</tr>\n";

  int j = 0;

  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    if ( s -> num_executes.mean() > 1 || s -> compound_amount > 0 || sim -> debug )
    {
      print_html_action_damage( os, s, p, j++ );
    }
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    bool first = true;

    for ( size_t i = 0; i < pet -> stats_list.size(); ++i )
    {
      stats_t* s = pet -> stats_list[ i ];

      if ( s -> num_executes.sum() || s -> compound_amount > 0 || sim -> debug )
      {
        if ( first )
        {
          first = false;
          os.printf(
            "\t\t\t\t\t\t\t<tr>\n"
            "\t\t\t\t\t\t\t\t<th class=\"left small\">pet - %s</th>\n"
            "\t\t\t\t\t\t\t\t<th class=\"right small\">%.0f / %.0f</th>\n"
            "\t\t\t\t\t\t\t\t<td colspan=\"18\" class=\"filler\"></td>\n"
            "\t\t\t\t\t\t\t</tr>\n",
            pet -> name_str.c_str(),
            pet -> collected_data.dps.mean(),
            pet -> collected_data.dpse.mean() );
        }
        print_html_action_damage( os, s, p, j );
      }
    }
  }

  os.printf(
    "\t\t\t\t\t\t</table>\n"
    "\t\t\t\t\t</div>\n"
    "\t\t\t\t</div>\n" );
}

// print_html_player_benefits_uptimes =======================================

void print_html_player_benefits_uptimes( report::sc_html_stream& os, player_t* p )
{
  os << "\t\t\t\t\t<div class=\"player-section benefits\">\n"
     << "\t\t\t\t\t\t<h3 class=\"toggle\">Benefits & Uptimes</h3>\n"
     << "\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
     << "\t\t\t\t\t\t\t<table class=\"sc\">\n"
     << "\t\t\t\t\t\t\t\t<tr>\n"
     << "\t\t\t\t\t\t\t\t\t<th>Benefits</th>\n"
     << "\t\t\t\t\t\t\t\t\t<th>%</th>\n"
     << "\t\t\t\t\t\t\t\t</tr>\n";
  int i = 1;

  for ( size_t j = 0; j < p -> benefit_list.size(); ++j )
  {
    benefit_t* u = p -> benefit_list[ j ];
    if ( u -> ratio.mean() > 0 )
    {
      os << "\t\t\t\t\t\t\t\t<tr";
      if ( !( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";
      os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.1f%%</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        u -> name(),
        u -> ratio.mean() );
      i++;
    }
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    for ( size_t j = 0; j < p -> benefit_list.size(); ++j )
    {
      benefit_t* u = p -> benefit_list[ j ];
      if ( u -> ratio.mean() > 0 )
      {
        std::string benefit_name;
        benefit_name += pet -> name_str + '-';
        benefit_name += u -> name();

        os << "\t\t\t\t\t\t\t\t<tr";
        if ( !( i & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.printf(
          "\t\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
          "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.1f%%</td>\n"
          "\t\t\t\t\t\t\t\t</tr>\n",
          benefit_name.c_str(),
          u -> ratio.mean() );
        i++;
      }
    }
  }

  os << "\t\t\t\t\t\t\t\t<tr>\n"
     << "\t\t\t\t\t\t\t\t\t<th>Uptimes</th>\n"
     << "\t\t\t\t\t\t\t\t\t<th>%</th>\n"
     << "\t\t\t\t\t\t\t\t</tr>\n";

  for ( size_t j = 0; j < p -> uptime_list.size(); ++j )
  {
    uptime_t* u = p -> uptime_list[ j ];
    if ( u -> uptime_sum.mean() > 0 )
    {
      os << "\t\t\t\t\t\t\t\t<tr";
      if ( !( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";
      os.printf(
        "\t\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
        "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.1f%%</td>\n"
        "\t\t\t\t\t\t\t\t</tr>\n",
        u -> name(),
        u -> uptime_sum.mean() * 100.0 );
      i++;
    }
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    for ( size_t j = 0; j < p -> uptime_list.size(); ++j )
    {
      uptime_t* u = p -> uptime_list[ j ];
      if ( u -> uptime_sum.mean() > 0 )
      {
        std::string uptime_name;
        uptime_name += pet -> name_str + '-';
        uptime_name += u -> name();

        os << "\t\t\t\t\t\t\t\t<tr";
        if ( !( i & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.printf(
          "\t\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
          "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.1f%%</td>\n"
          "\t\t\t\t\t\t\t\t</tr>\n",
          uptime_name.c_str(),
          u -> uptime_sum.mean() * 100.0 );

        i++;
      }
    }
  }


  os << "\t\t\t\t\t\t\t</table>\n"
     << "\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t</div>\n";
}

// print_html_player_procs ==================================================

void print_html_player_procs( report::sc_html_stream& os, std::vector<proc_t*> pr )
{
  // Procs Section
  os << "\t\t\t\t\t<div class=\"player-section procs\">\n"
     << "\t\t\t\t\t\t<h3 class=\"toggle\">Procs</h3>\n"
     << "\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
     << "\t\t\t\t\t\t\t<table class=\"sc\">\n"
     << "\t\t\t\t\t\t\t\t<tr>\n"
     << "\t\t\t\t\t\t\t\t\t<th></th>\n"
     << "\t\t\t\t\t\t\t\t\t<th>Count</th>\n"
     << "\t\t\t\t\t\t\t\t\t<th>Interval</th>\n"
     << "\t\t\t\t\t\t\t\t</tr>\n";
  {
    int i = 1;
    for ( size_t j = 0; j < pr.size(); ++j )
    {
      proc_t* proc = pr[ j ];
      if ( proc -> count.mean() > 0 )
      {
        os << "\t\t\t\t\t\t\t\t<tr";
        if ( !( i & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.printf(
          "\t\t\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
          "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.1f</td>\n"
          "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.1fsec</td>\n"
          "\t\t\t\t\t\t\t\t</tr>\n",
          proc -> name(),
          proc -> count.mean(),
          proc -> interval_sum.mean() );
        i++;
      }
    }
  }
  os << "\t\t\t\t\t\t\t</table>\n"
     << "\t\t\t\t\t\t</div>\n"
     << "\t\t\t\t\t</div>\n";



}

// print_html_player_deaths =================================================

void print_html_player_deaths( report::sc_html_stream& os, player_t* p, player_processed_report_information_t& ri )
{
  // Death Analysis

  const extended_sample_data_t& deaths = p -> collected_data.deaths;

  if ( deaths.size() > 0 )
  {
    std::string distribution_deaths_str                = "";
    if ( ! ri.distribution_deaths_chart.empty() )
    {
      distribution_deaths_str = "<img src=\"" + ri.distribution_deaths_chart + "\" alt=\"Deaths Distribution Chart\" />\n";
    }

    os << "\t\t\t\t\t<div class=\"player-section gains\">\n"
       << "\t\t\t\t\t\t<h3 class=\"toggle\">Deaths</h3>\n"
       << "\t\t\t\t\t\t<div class=\"toggle-content hide\">\n"
       << "\t\t\t\t\t\t\t<table class=\"sc\">\n"
       << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<th></th>\n"
       << "\t\t\t\t\t\t\t\t\t<th></th>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\">death count</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\">" << deaths.size() << "</td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\">death count pct</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\">"
       << ( double ) deaths.size() / p -> sim -> iterations * 100
       << "</td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\">avg death time</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\">" << deaths.mean() << "</td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\">min death time</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\">" << deaths.min() << "</td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\">max death time</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\">" << deaths.max() << "</td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t\t<tr>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"left\">dmg taken</td>\n"
       << "\t\t\t\t\t\t\t\t\t<td class=\"right\">" << p -> collected_data.dmg_taken.mean() << "</td>\n"
       << "\t\t\t\t\t\t\t\t</tr>\n";

    os << "\t\t\t\t\t\t\t\t</table>\n";

    os << "\t\t\t\t\t\t\t\t\t<div class=\"clear\"></div>\n";

    os << "\t\t\t\t\t\t\t\t\t" << distribution_deaths_str << "\n";

    os << "\t\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t\t</div>\n";
  }
}

// print_html_player_ =======================================================

void print_html_player_( report::sc_html_stream& os, sim_t* sim, player_t* p, int j = 0 )
{
  report::generate_player_charts( p, p -> report_information );
  report::generate_player_buff_lists( p, p -> report_information );

  print_html_player_description( os, sim, p, j );

  print_html_player_results_spec_gear( os, sim, p );

  print_html_player_scale_factors( os, sim, p, p -> report_information );

  print_html_player_charts( os, sim, p, p -> report_information );

  print_html_player_abilities( os, sim, p );

  print_html_player_buffs( os, p, p -> report_information );

  print_html_player_resources( os, p, p -> report_information );

  print_html_player_benefits_uptimes( os, p );

  print_html_player_procs( os, p -> proc_list );

  print_html_player_deaths( os, p, p -> report_information );

  print_html_player_statistics( os, p, p -> report_information );

  print_html_player_action_priority_list( os, sim, p );

  if ( ! p -> is_pet() )
  {
    print_html_stats( os, p );

    print_html_gear( os, p );

    print_html_talents( os, p );

    print_html_profile( os, p );
  }

  // print_html_player_gear_weights( os, p, p -> report_information );

  os << "\t\t\t\t\t</div>\n"
     << "\t\t\t\t</div>\n\n";
}

} // UNNAMED NAMESPACE ====================================================

namespace report {

void print_html_player( report::sc_html_stream& os, player_t* p, int j = 0 )
{
  print_html_player_( os, p -> sim, p, j );
}

} // END report NAMESPACE

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_report.hpp"
#include "sc_highchart.hpp"
#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE ==========================================

enum stats_mask_e
{
  MASK_DMG     = 1 << STATS_DMG,
  MASK_HEAL    = 1 << STATS_HEAL,
  MASK_ABSORB  = 1 << STATS_ABSORB,
  MASK_NEUTRAL = 1 << STATS_NEUTRAL
};

bool has_avoidance( const std::array<stats_t::stats_results_t, RESULT_MAX>& s )
{
  return ( s[ RESULT_MISS ].count.mean() + s[ RESULT_DODGE ].count.mean() +
           s[ RESULT_PARRY ].count.mean() ) > 0;
}

bool has_avoidance( const std::array<stats_t::stats_results_t, FULLTYPE_MAX>& s )
{
  return ( s[ FULLTYPE_MISS ].count.mean() + s[ FULLTYPE_DODGE ].count.mean() +
           s[ FULLTYPE_PARRY ].count.mean() ) > 0;
}

bool has_block( const std::array<stats_t::stats_results_t, FULLTYPE_MAX>& s )
{
  return ( s[ FULLTYPE_HIT_BLOCK ].count.mean() +
           s[ FULLTYPE_HIT_CRITBLOCK ].count.mean() +
           s[ FULLTYPE_GLANCE_BLOCK ].count.mean() +
           s[ FULLTYPE_GLANCE_CRITBLOCK ].count.mean() +
           s[ FULLTYPE_CRIT_BLOCK ].count.mean() +
           s[ FULLTYPE_CRIT_CRITBLOCK ].count.mean() ) > 0;
}

bool has_glance( const std::array<stats_t::stats_results_t, FULLTYPE_MAX>& s )
{
  return s[ FULLTYPE_GLANCE ].count.mean() > 0 ||
         s[ FULLTYPE_GLANCE_BLOCK ].count.mean() > 0 ||
         s[ FULLTYPE_GLANCE_CRITBLOCK ].count.mean() > 0;
}

bool player_has_tick_results( const player_t& p, unsigned stats_mask )
{
  for ( const auto& stat : p.stats_list )
  {
    if ( !( stats_mask & ( 1 << stat->type ) ) )
      continue;

    if ( stat->num_tick_results.count() > 0 )
      return true;
  }

  for ( const auto& pet : p.pet_list )
  {
    if ( player_has_tick_results( *pet, stats_mask ) )
      return true;
  }

  return false;
}

bool player_has_avoidance( const player_t& p, unsigned stats_mask )
{
  for ( const auto& stat : p.stats_list )
  {
    if ( !( stats_mask & ( 1 << stat->type ) ) )
      continue;

    if ( has_avoidance( stat->direct_results ) )
      return true;

    if ( has_avoidance( stat->tick_results ) )
      return true;
  }

  for ( const auto& pet : p.pet_list )
  {
    if ( player_has_avoidance( *pet, stats_mask ) )
      return true;
  }

  return false;
}

bool player_has_block( const player_t& p, unsigned stats_mask )
{
  for ( const auto& stat : p.stats_list )
  {
    if ( !( stats_mask & ( 1 << stat->type ) ) )
      continue;

    if ( has_block( stat->direct_results ) )
      return true;
  }

  for ( const auto& pet : p.pet_list )
  {
    if ( player_has_block( *pet, stats_mask ) )
      return true;
  }

  return false;
}

bool player_has_glance( const player_t& p, unsigned stats_mask )
{
  for ( const auto& stat : p.stats_list )
  {
    if ( !( stats_mask & ( 1 << stat->type ) ) )
      continue;

    if ( has_glance( stat -> direct_results ) )
      return true;
  }

  for ( const auto& pet : p.pet_list )
  {
    if ( player_has_glance( *pet, stats_mask ) )
      return true;
  }

  return false;
}

std::string output_action_name( const stats_t& s, const player_t* actor )
{
  std::string class_attr;
  action_t* a            = nullptr;
  std::string stats_type = util::stats_type_string( s.type );

  if ( s.player->sim->report_details )
  {
    class_attr = " id=\"actor" + util::to_string( s.player->index ) + "_" +
                 s.name_str + "_" + stats_type +
                 "_toggle\" class=\"toggle-details\"";
  }

  for ( const auto& action : s.action_list )
  {
    if ( ( a = action )->id > 1 )
      break;
  }

  std::string name;
  if ( a )
  {
    name = report::action_decorator_t( a ).decorate();
  }
  else
  {
    name = s.name_str;
  }

  // If we are printing a stats object that belongs to a pet, for an actual
  // actor, print out the pet name too
  if ( actor && !actor->is_pet() && s.player->is_pet() )
    name += " (" + s.player->name_str + ")";

  return "<span " + class_attr + ">" + name + "</span>";
}

// print_html_action_info =================================================

template <typename T>
double mean_damage( const T& result )
{
  double mean  = 0;
  size_t count = 0;

  for ( size_t i = 0; i < result.size(); i++ )
  {
    mean += result[ i ].actual_amount.sum();
    count += result[ i ].actual_amount.count();
  }

  if ( count > 0 )
    mean /= count;

  return mean;
}

template <typename T, typename V>
double mean_value( const T& results, const std::initializer_list<V>& selectors )
{
  double sum = 0, count = 0;

  range::for_each( selectors, [ & ]( const V& selector ) {
    auto idx = static_cast<int>( selector );
    if ( idx < 0 )
    {
      return;
    }

    if ( results.size() < as<size_t>( idx ) )
    {
      return;
    }

    count += results[ idx ].actual_amount.count();
    sum   += results[ idx ].actual_amount.sum();
  } );

  return count > 0 ? sum / count : 0;
}

template <typename T, typename V>
double pct_value( const T& results, const std::initializer_list<V>& selectors )
{
  double sum = 0;

  range::for_each( selectors, [ & ]( const V& selector ) {
    auto idx = static_cast<int>( selector );
    if ( idx < 0 )
    {
      return;
    }

    if ( results.size() < as<size_t>( idx ) )
    {
      return;
    }

    sum += results[ idx ].pct;
  } );

  return sum;
}

void print_html_action_summary( report::sc_html_stream& os, unsigned stats_mask,
                                int result_type, const stats_t& s,
                                const player_t& p )
{
  using full_result_t = std::array<stats_t::stats_results_t, FULLTYPE_MAX>;
  using result_t = std::array<stats_t::stats_results_t, RESULT_MAX>;

  std::string type_str;
  if ( result_type == 1 )
    type_str = "Periodic";
  else
    type_str = "Direct";

  const auto& dr = s.direct_results;
  const auto& tr = s.tick_results;

  // Result type
  os.printf( "<td class=\"right small\">%s</td>\n", type_str.c_str() );

  os.printf( "<td class=\"right small\">%.1f</td>\n",
             result_type == 1
             ? s.num_tick_results.mean()
             : s.num_direct_results.mean() );

  // Hit results
  os.printf( "<td class=\"right small\">%.0f</td>\n",
             result_type == 1
             ? mean_value<result_t, result_e>( tr, { RESULT_HIT } )
             : mean_value<full_result_t, full_result_e>( dr, { FULLTYPE_HIT, FULLTYPE_HIT_BLOCK, FULLTYPE_HIT_CRITBLOCK } ) );

  // Crit results
  os.printf( "<td class=\"right small\">%.0f</td>\n",
             result_type == 1
             ? mean_value<result_t, result_e>( tr, { RESULT_CRIT } )
             : mean_value<full_result_t, full_result_e>( dr, { FULLTYPE_CRIT, FULLTYPE_CRIT_BLOCK, FULLTYPE_CRIT_CRITBLOCK } ) );

  // Mean amount
  os.printf( "<td class=\"right small\">%.0f</td>\n",
             result_type == 1
             ? mean_damage( tr )
             : mean_damage( dr ) );

  // Crit%
  os.printf( "<td class=\"right small\">%.1f%%</td>\n",
             result_type == 1
             ? pct_value<result_t, result_e>( tr, { RESULT_CRIT } )
             : pct_value<full_result_t, full_result_e>( dr, { FULLTYPE_CRIT, FULLTYPE_CRIT_BLOCK, FULLTYPE_CRIT_CRITBLOCK } ) );

  if ( player_has_avoidance( p, stats_mask ) )
    os.printf( "<td class=\"right small\">%.1f%%</td>\n",  // direct_results Avoid%
               result_type == 1
               ? pct_value<result_t, result_e>( tr, { RESULT_MISS, RESULT_DODGE, RESULT_PARRY } )
               : pct_value<full_result_t, full_result_e>( dr, { FULLTYPE_MISS, FULLTYPE_DODGE, FULLTYPE_PARRY } ) );

  if ( player_has_glance( p, stats_mask ) )
    os.printf( "<td class=\"right small\">%.1f%%</td>\n",  // direct_results Glance%
             result_type == 1
             ? pct_value<result_t, result_e>( tr, { RESULT_GLANCE } )
             : pct_value<full_result_t, full_result_e>( dr, { FULLTYPE_GLANCE, FULLTYPE_GLANCE_BLOCK, FULLTYPE_GLANCE_CRITBLOCK } ) );

  if ( player_has_block( p, stats_mask ) )
    os.printf( "<td class=\"right small\">%.1f%%</td>\n",  // direct_results Block%
        result_type == 1
        ? 0
        : pct_value<full_result_t, full_result_e>( dr,
          { FULLTYPE_HIT_BLOCK,
            FULLTYPE_HIT_CRITBLOCK,
            FULLTYPE_GLANCE_BLOCK,
            FULLTYPE_GLANCE_CRITBLOCK,
            FULLTYPE_CRIT_BLOCK,
            FULLTYPE_CRIT_CRITBLOCK } ) );

  if ( player_has_tick_results( p, stats_mask ) )
  {
    if ( util::str_in_str_ci( type_str, "Periodic" ) )
      os.printf( "<td class=\"right small\">%.1f%%</td>\n",  // Uptime%
                 100 * s.total_tick_time.mean() /
                     p.collected_data.fight_length.mean() );
    else
      os.printf( "<td class=\"right small\">&#160;</td>\n" );
  }
}

void print_html_action_info( report::sc_html_stream& os, unsigned stats_mask,
                             const stats_t& s, int j, int n_columns,
                             const player_t* actor = nullptr, int indentation = 0)
{
  const player_t& p = *s.player->get_owner_or_self();

  std::string row_class = ( j & 1 ) ? " class=\"odd\"" : "";

  os << "<tr" << row_class << ">\n";
  int result_rows = s.has_direct_amount_results() + s.has_tick_amount_results();
  if ( result_rows == 0 )
    result_rows = 1;

  // Ability name
  os << "<td class=\"left small\" rowspan=\"" << result_rows << "\">";
  if ( s.parent && s.parent->player == actor )
  {
    for( int i = 0; i< indentation; ++i)
    {
      os << "&#160;&#160;&#160;\n";
    }
  }

  os << output_action_name( s, actor );
  os << "</td>\n";

  // DPS and DPS %
  // Skip for abilities that do no damage
  if ( s.compound_amount > 0 || ( s.parent && s.parent->compound_amount > 0 ) )
  {
    std::string compound_aps     = "";
    std::string compound_aps_pct = "";
    double cAPS                  = s.portion_aps.mean();
    double cAPSpct               = s.portion_amount;

    for ( auto& elem : s.children )
    {
      cAPS += elem->portion_apse.mean();
      cAPSpct += elem->portion_amount;
    }

    if ( cAPS > s.portion_aps.mean() )
      compound_aps = "&#160;(" + util::to_string( cAPS, 0 ) + ")";
    if ( cAPSpct > s.portion_amount )
      compound_aps_pct = "&#160;(" + util::to_string( cAPSpct * 100, 1 ) + "%)";

    os.printf( "<td class=\"right small\" rowspan=\"%d\">%.0f%s</td>\n",
               result_rows, s.portion_aps.pretty_mean(), compound_aps.c_str() );
    os.printf( "<td class=\"right small\" rowspan=\"%d\">%.1f%%%s</td>\n",
               result_rows, s.portion_amount * 100, compound_aps_pct.c_str() );
  }

  // Number of executes
  os.printf( "<td class=\"right small\" rowspan=\"%d\">%.1f</td>\n",
             result_rows, s.num_executes.pretty_mean() );

  // Execute interval
  os.printf( "<td class=\"right small\" rowspan=\"%d\">%.2fsec</td>\n",
             result_rows, s.total_intervals.pretty_mean() );

  // Skip the rest of this for abilities that do no damage
  if ( s.compound_amount > 0 )
  {
    // Amount per execute
    os.printf( "<td class=\"right small\" rowspan=\"%d\">%.0f</td>\n",
               result_rows, s.ape );

    // Amount per execute time
    os.printf( "<td class=\"right small\" rowspan=\"%d\">%.0f</td>\n",
               result_rows, s.apet );

    bool periodic_only = false;
    if ( s.has_direct_amount_results() )
      print_html_action_summary( os, stats_mask, 0, s, p );
    else if ( s.has_tick_amount_results() )
    {
      periodic_only = true;
      print_html_action_summary( os, stats_mask, 1, s, p );
    }
    else
      os.printf( "<td class=\"right small\" colspan=\"%d\"></td>\n",
                 n_columns );

    os.printf( "</tr>\n" );

    if ( !periodic_only && s.has_tick_amount_results() )
    {
      os << "<tr" << row_class << ">\n";
      print_html_action_summary( os, stats_mask, 1, s, p );
      os << "</tr>\n";
    }
  }
  else
  {
    os << "</tr>\n";
  }

  if ( p.sim->report_details )
  {
    // TODO: Transitional; Highcharts
    std::string timeline_stat_aps_str = "";
    if ( !s.timeline_aps_chart.empty() )
    {
      timeline_stat_aps_str =
          "<img src=\"" + s.timeline_aps_chart + "\" alt=\"" +
          ( s.type == STATS_DMG ? "DPS" : "HPS" ) + " Timeline Chart\" />\n";
    }
    std::string aps_distribution_str = "";
    if ( !s.aps_distribution_chart.empty() )
    {
      aps_distribution_str = "<img src=\"" + s.aps_distribution_chart +
                             "\" alt=\"" +
                             ( s.type == STATS_DMG ? "DPS" : "HPS" ) +
                             " Distribution Chart\" />\n";
    }
    os << "<tr class=\"details hide\">\n"
       << "<td colspan=\"" << ( n_columns > 0 ? ( 7 + n_columns ) : 3 )
       << "\" class=\"filler\">\n";

    // Stat Details
    os.printf( "<h4>Stats details: %s </h4>\n", s.name_str.c_str() );

    os << "<table class=\"details\">\n"
       << "<tr>\n";

    os << "<th class=\"small\">Type</th>\n"
       << "<th class=\"small\">Executes</th>\n"
       << "<th class=\"small\">Direct Results</th>\n"
       << "<th class=\"small\">Ticks</th>\n"
       << "<th class=\"small\">Tick Results</th>\n"
       << "<th class=\"small\">Execute Time per Execution</th>\n"
       << "<th class=\"small\">Tick Time per  Tick</th>\n"
       << "<th class=\"small\">Actual Amount</th>\n"
       << "<th class=\"small\">Total Amount</th>\n"
       << "<th class=\"small\">Overkill %</th>\n"
       << "<th class=\"small\">Amount per Total Time</th>\n"
       << "<th class=\"small\">Amount per Total Execute Time</th>\n";

    os << "</tr>\n"
       << "<tr>\n";

    os.printf(
        "<td class=\"right small\">%s</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.4f</td>\n"
        "<td class=\"right small\">%.4f</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.2f</td>\n"
        "<td class=\"right small\">%.2f</td>\n",
        util::stats_type_string( s.type ), s.num_executes.mean(),
        s.num_direct_results.mean(), s.num_ticks.mean(),
        s.num_tick_results.mean(), s.etpe, s.ttpt, s.actual_amount.mean(),
        s.total_amount.mean(), s.overkill_pct, s.aps, s.apet );
    os << "</tr>\n";
    os << "</table>\n";
    if ( !s.portion_aps.simple || !s.portion_apse.simple ||
         !s.actual_amount.simple )
    {
      os << "<table class=\"details\">\n";
      int sd_counter = 0;
      report::print_html_sample_data( os, p, s.actual_amount, "Actual Amount",
                                      sd_counter );

      report::print_html_sample_data( os, p, s.portion_aps,
                                      "portion Amount per Second ( pAPS )",
                                      sd_counter );

      report::print_html_sample_data(
          os, p, s.portion_apse,
          "portion Effective Amount per Second ( pAPSe )", sd_counter );

      os << "</table>\n";

      if ( !s.portion_aps.simple && p.sim->scaling->has_scale_factors() &&
           s.scaling )
      {
        int colspan = 0;
        os << "<table class=\"details\">\n";
        os << "<tr>\n"
           << "<th><a href=\"#help-scale-factors\" class=\"help\">?</a></th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p.scaling->scales_with[ i ] )
          {
            os.printf( "<th>%s</th>\n", util::stat_type_abbrev( i ) );
            colspan++;
          }
        if ( p.sim->scaling->scale_lag )
        {
          os << "<th>ms Lag</th>\n";
          colspan++;
        }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Scale Factors</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p.scaling->scales_with[ i ] )
          {
            if ( s.scaling->value.get_stat( i ) > 1.0e5 )
              os.printf( "<td>%.*e</td>\n", p.sim->report_precision,
                         s.scaling->value.get_stat( i ) );
            else
              os.printf( "<td>%.*f</td>\n", p.sim->report_precision,
                         s.scaling->value.get_stat( i ) );
          }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Scale Deltas</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        {
          if ( !p.scaling->scales_with[ i ] )
            continue;

          double value = p.sim->scaling->stats.get_stat( i );
          std::string prefix;
          if ( p.sim->scaling->center_scale_delta == 1 && i != STAT_SPIRIT &&
               i != STAT_HIT_RATING && i != STAT_EXPERTISE_RATING )
          {
            prefix = "+/- ";
            value /= 2;
          }

          os.printf( "<td>%s%.0f</td>\n", prefix.c_str(), value );
        }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Error</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p.scaling->scales_with[ i ] )
          {
            scale_metric_e sm = p.sim->scaling->scaling_metric;
            if ( p.scaling->scaling_error[ sm ].get_stat( i ) > 1.0e5 )
              os.printf( "<td>%.*e</td>\n", p.sim->report_precision,
                         s.scaling->error.get_stat( i ) );
            else
              os.printf( "<td>%.*f</td>\n", p.sim->report_precision,
                         s.scaling->error.get_stat( i ) );
          }
        os << "</tr>\n";
        os << "</table>\n";
      }
    }

    // Detailed breakdown of damage or healing ability
    os << "<table  class=\"details\">\n";
    if ( s.num_direct_results.mean() > 0 )
    {
      os << "<tr>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "<th class=\"small\" colspan=\"3\">Simulation</th>\n"
         << "<th class=\"small\" colspan=\"3\">Iteration Average</th>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "</tr>\n";

      // Direct Damage
      os << "<tr>\n"
         << "<th class=\"small\">Direct Results</th>\n"
         << "<th class=\"small\">Count</th>\n"
         << "<th class=\"small\">Pct</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Actual Amount</th>\n"
         << "<th class=\"small\">Total Amount</th>\n"
         << "<th class=\"small\">Overkill %</th>\n"
         << "</tr>\n";
      int k = 0;
      for ( full_result_e i = FULLTYPE_MAX; --i >= FULLTYPE_NONE; )
      {
        if ( ! s.direct_results[ i ].count.mean() )
          continue;

        os << "<tr";

        if ( k & 1 )
        {
          os << " class=\"odd\"";
        }
        k++;
        os << ">\n";

        os.printf(
            "<td class=\"left small\">%s</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.2f%%</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "</tr>\n",
            util::full_result_type_string( i ),
            s.direct_results[ i ].count.mean(),
            s.direct_results[ i ].pct,
            s.direct_results[ i ].actual_amount.mean(),
            s.direct_results[ i ].actual_amount.min(),
            s.direct_results[ i ].actual_amount.max(),
            s.direct_results[ i ].avg_actual_amount.mean(),
            s.direct_results[ i ].avg_actual_amount.min(),
            s.direct_results[ i ].avg_actual_amount.max(),
            s.direct_results[ i ].fight_actual_amount.mean(),
            s.direct_results[ i ].fight_total_amount.mean(),
            s.direct_results[ i ].overkill_pct.mean() );
      }
    }

    if ( s.num_tick_results.mean() > 0 )
    {
      // Tick Damage
      os << "<tr>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "<th class=\"small\" colspan=\"3\">Simulation</th>\n"
         << "<th class=\"small\" colspan=\"3\">Iteration Average</th>\n"
         << "<th class=\"small\" colspan=\"3\">&#160;</th>\n"
         << "</tr>\n";

      os << "<tr>\n"
         << "<th class=\"small\">Tick Results</th>\n"
         << "<th class=\"small\">Count</th>\n"
         << "<th class=\"small\">Pct</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Mean</th>\n"
         << "<th class=\"small\">Min</th>\n"
         << "<th class=\"small\">Max</th>\n"
         << "<th class=\"small\">Actual Amount</th>\n"
         << "<th class=\"small\">Total Amount</th>\n"
         << "<th class=\"small\">Overkill %</th>\n"
         << "</tr>\n";
      int k = 0;
      for ( result_e i = RESULT_MAX; --i >= RESULT_NONE; )
      {
        if ( !s.tick_results[ i ].count.mean() )
          continue;

        os << "<tr";
        if ( k & 1 )
        {
          os << " class=\"odd\"";
        }
        k++;
        os << ">\n";
        os.printf(
            "<td class=\"left small\">%s</td>\n"
            "<td class=\"right small\">%.1f</td>\n"
            "<td class=\"right small\">%.2f%%</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.0f</td>\n"
            "<td class=\"right small\">%.2f</td>\n"
            "</tr>\n",
            util::result_type_string( i ), s.tick_results[ i ].count.mean(),
            s.tick_results[ i ].pct, s.tick_results[ i ].actual_amount.mean(),
            s.tick_results[ i ].actual_amount.min(),
            s.tick_results[ i ].actual_amount.max(),
            s.tick_results[ i ].avg_actual_amount.mean(),
            s.tick_results[ i ].avg_actual_amount.min(),
            s.tick_results[ i ].avg_actual_amount.max(),
            s.tick_results[ i ].fight_actual_amount.mean(),
            s.tick_results[ i ].fight_total_amount.mean(),
            s.tick_results[ i ].overkill_pct.mean() );
      }
    }

    os << "</table>\n";

    os << "<div class=\"clear\">&#160;</div>\n";

    if ( s.has_direct_amount_results() || s.has_tick_amount_results() )
    {
      highchart::time_series_t ts( highchart::build_id( s ), *s.player->sim );
      chart::generate_stats_timeline( ts, s );
      os << ts.to_target_div();
      s.player->sim->add_chart_data( ts );
    }

    os << "<div class=\"clear\">&#160;</div>\n";
    // Action Details
    std::vector<std::string> processed_actions;

    for ( const auto& a : s.action_list )
    {
      bool found            = false;
      size_t size_processed = processed_actions.size();
      for ( size_t k = 0; k < size_processed && !found; k++ )
        if ( processed_actions[ k ] == a->name() )
          found = true;
      if ( found )
        continue;
      processed_actions.push_back( a->name() );

      os.printf( "<h4>Action details: %s </h4>\n", a->name() );

      os.printf(
          "<div class=\"float\">\n"
          "<h5>Static Values</h5>\n"
          "<ul>\n"
          "<li><span class=\"label\">id:</span>%i</li>\n"
          "<li><span class=\"label\">school:</span>%s</li>\n"
          "<li><span class=\"label\">resource:</span>%s</li>\n"
          "<li><span class=\"label\">range:</span>%.1f</li>\n"
          "<li><span class=\"label\">travel_speed:</span>%.4f</li>\n"
          "<li><span class=\"label\">trigger_gcd:</span>%.4f</li>\n"
          "<li><span class=\"label\">min_gcd:</span>%.4f</li>\n"
          "<li><span class=\"label\">base_cost:</span>%.1f</li>\n"
          "<li><span class=\"label\">secondary_cost:</span>%.1f</li>\n"
          "<li><span class=\"label\">cooldown:</span>%.3f</li>\n"
          "<li><span class=\"label\">cooldown hasted:</span>%s</li>\n"
          "<li><span class=\"label\">base_execute_time:</span>%.2f</li>\n"
          "<li><span class=\"label\">base_crit:</span>%.2f</li>\n"
          "<li><span class=\"label\">target:</span>%s</li>\n"
          "<li><span class=\"label\">harmful:</span>%s</li>\n"
          "<li><span class=\"label\">if_expr:</span>%s</li>\n"
          "</ul>\n"
          "</div>\n",
          a->id, util::school_type_string( a->get_school() ),
          util::resource_type_string( a->current_resource() ), a->range,
          a->travel_speed, a->trigger_gcd.total_seconds(),
          a->min_gcd.total_seconds(), a->base_costs[ a->current_resource() ],
          a->secondary_costs[ a->current_resource() ],
          a->cooldown->duration.total_seconds(),
          a->cooldown->hasted ? "true" : "false",
          a->base_execute_time.total_seconds(), a->base_crit,
          a->target ? a->target->name() : "", a->harmful ? "true" : "false",
          util::encode_html( a->option.if_expr_str ).c_str() );

      // Spelldata
      if ( a->data().ok() )
      {
        os.printf(
            "<div class=\"float\">\n"
            "<h5>Spelldata</h5>\n"
            "<ul>\n"
            "<li><span class=\"label\">id:</span>%i</li>\n"
            "<li><span class=\"label\">name:</span>%s</li>\n"
            "<li><span class=\"label\">school:</span>%s</li>\n"
            "<li><span class=\"label\">tooltip:</span><span "
            "class=\"tooltip\">%s</span></li>\n"
            "<li><span class=\"label\">description:</span><span "
            "class=\"tooltip\">%s</span></li>\n"
            "</ul>\n"
            "</div>\n",
            a->data().id(), a->data().name_cstr(),
            util::school_type_string( a->data().get_school_type() ),
            report::pretty_spell_text( a->data(), a->data().tooltip(), p )
                .c_str(),
            util::encode_html(
                report::pretty_spell_text( a->data(), a->data().desc(), p ) )
                .c_str() );
      }

      if ( a->spell_power_mod.direct || a->base_dd_min || a->base_dd_max )
      {
        os.printf(
            "<div class=\"float\">\n"
            "<h5>Direct Damage</h5>\n"
            "<ul>\n"
            "<li><span class=\"label\">may_crit:</span>%s</li>\n"
            "<li><span "
            "class=\"label\">attack_power_mod.direct:</span>%.6f</li>\n"
            "<li><span "
            "class=\"label\">spell_power_mod.direct:</span>%.6f</li>\n"
            "<li><span class=\"label\">base_dd_min:</span>%.2f</li>\n"
            "<li><span class=\"label\">base_dd_max:</span>%.2f</li>\n"
            "<li><span class=\"label\">base_dd_mult:</span>%.2f</li>\n"
            "</ul>\n"
            "</div>\n",
            a->may_crit ? "true" : "false", a->attack_power_mod.direct,
            a->spell_power_mod.direct, a->base_dd_min, a->base_dd_max,
            a->base_dd_multiplier );
      }
      if ( a->dot_duration > timespan_t::zero() )
      {
        os.printf(
            "<div class=\"float\">\n"
            "<h5>Damage Over Time</h5>\n"
            "<ul>\n"
            "<li><span class=\"label\">tick_may_crit:</span>%s</li>\n"
            "<li><span class=\"label\">tick_zero:</span>%s</li>\n"
            "<li><span class=\"label\">attack_power_mod.tick:</span>%.6f</li>\n"
            "<li><span class=\"label\">spell_power_mod.tick:</span>%.6f</li>\n"
            "<li><span class=\"label\">base_td:</span>%.2f</li>\n"
            "<li><span class=\"label\">base_td_mult:</span>%.2f</li>\n"
            "<li><span class=\"label\">dot_duration:</span>%.2f</li>\n"
            "<li><span class=\"label\">base_tick_time:</span>%.2f</li>\n"
            "<li><span class=\"label\">hasted_ticks:</span>%s</li>\n"
            "<li><span class=\"label\">dot_behavior:</span>%s</li>\n"
            "</ul>\n"
            "</div>\n",
            a->tick_may_crit ? "true" : "false",
            a->tick_zero ? "true" : "false", a->attack_power_mod.tick,
            a->spell_power_mod.tick, a->base_td, a -> base_td_multiplier,
            a->dot_duration.total_seconds(), a->base_tick_time.total_seconds(),
            a->hasted_ticks ? "true" : "false",
            util::dot_behavior_type_string( a->dot_behavior ) );
      }
      if ( a->weapon )
      {
        os.printf(
            "<div class=\"float\">\n"
            "<h5>Weapon</h5>\n"
            "<ul>\n"
            "<li><span class=\"label\">normalized:</span>%s</li>\n"
            "<li><span class=\"label\">weapon_power_mod:</span>%.6f</li>\n"
            "<li><span class=\"label\">weapon_multiplier:</span>%.2f</li>\n"
            "</ul>\n"
            "</div>\n",
            a->normalize_weapon_speed ? "true" : "false", a->weapon_power_mod,
            a->weapon_multiplier );
      }
      os << "<div class=\"clear\">&#160;</div>\n";
    }

    os << "</td>\n"
       << "</tr>\n";
  }
}

// print_html_action_resource ===============================================

int print_html_action_resource( report::sc_html_stream& os, const stats_t& s,
                                int j )
{
  for ( const auto& a : s.action_list )
  {
    if ( a->stats != &s )
      continue;
    if ( !a->background )
      break;
  }

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( s.resource_gain.actual[ i ] > 0 )
    {
      os << "<tr";
      if ( j & 1 )
      {
        os << " class=\"odd\"";
      }
      ++j;
      os << ">\n";
      os.printf(
          "<td class=\"left\">%s</td>\n"
          "<td class=\"left\">%s</td>\n"
          "<td class=\"right\">%.1f</td>\n"
          "<td class=\"right\">%.1f</td>\n"
          "<td class=\"right\">%.1f</td>\n",
          s.resource_gain.name(),
          util::inverse_tokenize( util::resource_type_string( i ) ).c_str(),
          s.resource_gain.count[ i ], s.resource_gain.actual[ i ],
          s.resource_gain.actual[ i ] / s.resource_gain.count[ i ] );
      os.printf(
          "<td class=\"right\">%.1f</td>\n"
          "<td class=\"right\">%.1f</td>\n",
          s.rpe[ i ], s.apr[ i ] );
      os << "</tr>\n";
    }
  }
  return j;
}

// print_html_gear ==========================================================

void print_html_gear( report::sc_html_stream& os, const player_t& p )
{
  if ( p.items.empty() )
    return;

  os.printf(
      "<div class=\"player-section gear\">\n"
      "<h3 class=\"toggle\">Gear</h3>\n"
      "<div class=\"toggle-content hide\">\n"
      "<table class=\"sc\">\n"
      "<tr>\n"
      "<th>Source</th>\n"
      "<th>Slot</th>\n"
      "<th>Average Item Level: %.2f</th>\n"
      "</tr>\n",
      util::round( p.avg_item_level() ), 2 );

  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
  {
    const item_t& item = p.items[ i ];
    if ( !item.active() )
    {
      continue;
    }

    os.printf(
        "<tr>\n"
        "<th class=\"left\">%s</th>\n"
        "<th class=\"left\">%s</th>\n"
        "<td class=\"left\">%s</td>\n"
        "</tr>\n",
        item.source_str.c_str(),
        util::inverse_tokenize( item.slot_name() ).c_str(),
        report::item_decorator_t( item ).decorate().c_str() );

    std::string item_sim_desc =
        "ilevel: " + util::to_string( item.item_level() );

    if ( item.parsed.data.item_class == ITEM_CLASS_WEAPON )
    {
      item_sim_desc += ", weapon: { " + item.weapon_stats_str() + " }";
    }

    if ( item.has_stats() )
    {
      item_sim_desc += ", stats: { ";
      item_sim_desc += item.item_stats_str();
      item_sim_desc += " }";
    }

    if ( item.parsed.gem_stats.size() > 0 )
    {
      item_sim_desc += ", gems: { ";
      item_sim_desc += item.gem_stats_str();
      if ( item.socket_color_match() &&
           item.parsed.socket_bonus_stats.size() > 0 )
      {
        item_sim_desc += ", ";
        item_sim_desc += item.socket_bonus_stats_str();
      }
      item_sim_desc += " }";
    }

    if ( item.parsed.enchant_stats.size() > 0 )
    {
      item_sim_desc += ", enchant: { ";
      item_sim_desc += item.enchant_stats_str();
      item_sim_desc += " }";
    }
    else if ( !item.parsed.encoded_enchant.empty() )
    {
      item_sim_desc += ", enchant: " + item.parsed.encoded_enchant;
    }

    auto has_relics = range::find_if( item.parsed.relic_bonus_ilevel,
                                      []( unsigned v ) { return v != 0; } );
    if ( has_relics != item.parsed.relic_bonus_ilevel.end() )
    {
      item_sim_desc += ", relics: { ";
      auto first = true;
      for ( auto bonus : item.parsed.relic_bonus_ilevel )
      {
        if ( bonus == 0 )
        {
          continue;
        }

        if ( !first )
        {
          item_sim_desc += ", ";
        }
        item_sim_desc += "+" + util::to_string( bonus ) + " ilevels";
        first = false;
      }
      item_sim_desc += " }";
    }

    if ( item.parsed.azerite_ids.size() )
    {
      std::stringstream s;
      for ( size_t i = 0; i < item.parsed.azerite_ids.size(); ++i )
      {
        const auto& power = item.player -> dbc.azerite_power( item.parsed.azerite_ids[ i ] );
        if ( power.id == 0 || ! item.player -> azerite -> is_enabled( power.id ) )
        {
          continue;
        }

        const auto spell = item.player -> find_spell( power.spell_id );
        auto decorator = report::spell_data_decorator_t( item.player, spell );
        decorator.item( item );

        s << decorator.decorate();

        if ( i < item.parsed.azerite_ids.size() - 1 )
        {
          s << ", ";
        }
      }

      if ( ! s.str().empty() )
      {
        item_sim_desc += "<br/>";
        item_sim_desc += "azerite powers: { ";
        item_sim_desc += s.str();
        item_sim_desc += " }";
      }
    }

    os.printf(
        "<tr>\n"
        "<th class=\"left\" colspan=\"2\"></th>\n"
        "<td class=\"left small\">%s</td>\n"
        "</tr>\n",
        item_sim_desc.c_str() );
  }

  os << "</table>\n"
     << "</div>\n"
     << "</div>\n";
}

// print_html_profile =======================================================

void print_html_profile( report::sc_html_stream& os, const player_t& p,
                         const player_processed_report_information_t& ri )
{
  if ( p.collected_data.fight_length.mean() > 0 )
  {
    std::string profile_str = ri.html_profile_str;
    profile_str             = util::encode_html( profile_str );
    util::replace_all( profile_str, "\n", "<br>" );

    os << "<div class=\"player-section profile\">\n"
       << "<h3 class=\"toggle\">Profile</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<div class=\"subsection force-wrap\">\n"
       << "<p>" << profile_str << "</p>\n"
       << "</div>\n"
       << "</div>\n"
       << "</div>\n";
  }
}

// print_html_stats =========================================================

void print_html_stats( report::sc_html_stream& os, const player_t& p )
{
  const auto& buffed_stats = p.collected_data.buffed_stats_snapshot;
  std::array<double, ATTRIBUTE_MAX> hybrid_attributes;
  range::fill( hybrid_attributes, 0 );
  const std::array<stat_e, 4> hybrid_stats = {
      {STAT_STR_AGI_INT, STAT_STR_AGI, STAT_STR_INT, STAT_AGI_INT}};

  for ( const auto& item : p.items )
  {
    for ( auto hybrid_stat : hybrid_stats )
    {
      auto real_stat   = p.convert_hybrid_stat( hybrid_stat );
      attribute_e attr = static_cast<attribute_e>( real_stat );

      if ( p.gear.attribute[ attr ] >= 0 ||
           item.stats.get_stat( hybrid_stat ) <= 0 )
      {
        continue;
      }

      hybrid_attributes[ attr ] += item.stats.get_stat( hybrid_stat );
    }
  }

  if ( p.collected_data.fight_length.mean() > 0 )
  {
    int j = 1;

    os << "<div class=\"player-section stats\">\n"
       << "<h3 class=\"toggle\">Stats</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th><a href=\"#help-stats-level\" "
          "class=\"help\">Level Bonus (" << p.level() << ")</a></th>\n"
       << "<th><a href=\"#help-stats-race\" "
          "class=\"help\">Race Bonus (" << util::race_type_string(p.race) << ")</a></th>\n"
       << "<th><a href=\"#help-stats-raid-buffed\" "
          "class=\"help\">Raid-Buffed</a></th>\n"
       << "<th><a href=\"#help-stats-unbuffed\" "
          "class=\"help\">Unbuffed</a></th>\n"
       << "<th><a href=\"#help-gear-amount\" class=\"help\">Gear "
          "Amount</a></th>\n"
       << "</tr>\n";

    for ( attribute_e i = ATTRIBUTE_NONE; ++i < ATTR_AGI_INT; )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">%s</th>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          util::inverse_tokenize( util::attribute_type_string( i ) ).c_str(),
          ( ! p.is_enemy() && ! p.is_pet() ) ? util::floor( dbc::stat_data_to_attribute(p.dbc.attribute_base( p.type, p.level() ), i ) ) : 0,
          util::floor( dbc::stat_data_to_attribute(p.dbc.race_base( p.race ), i ) ),
          util::floor( buffed_stats.attribute[ i ] ),
          util::floor( p.get_attribute( i ) ),
          util::floor( p.total_gear.attribute[ i ] ) );
      // append hybrid attributes as a parenthetical if appropriate
      if ( hybrid_attributes[ i ] > 0 )
        os.printf( " (%.0f)", hybrid_attributes[ i ] );

      os.printf( "</td>\n</tr>\n" );

      j++;
    }
    for ( resource_e i = RESOURCE_NONE; ++i < RESOURCE_MAX; )
    {
      if ( p.resources.max[ i ] > 0 )
        os.printf(
            "<tr%s>\n"
            "<th class=\"left\">%s</th>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\">%.0f</td>\n"
            "<td class=\"right\">%.0f</td>\n"
            "<td class=\"right\">%.0f</td>\n"
            "</tr>\n",
            ( j % 2 == 1 ) ? " class=\"odd\"" : "",
            util::inverse_tokenize( util::resource_type_string( i ) ).c_str(),
            buffed_stats.resource[ i ], p.resources.max[ i ], 0.0 );
      j++;
    }
    if ( buffed_stats.spell_power > 0 )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Spell Power</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", buffed_stats.spell_power,
          p.composite_spell_power( SCHOOL_MAX ) *
              p.composite_spell_power_multiplier(),
          p.initial.stats.spell_power );
      j++;
    }
    if ( p.composite_melee_crit_chance() == p.composite_spell_crit_chance() )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Crit</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * buffed_stats.attack_crit_chance,
          100 * p.composite_melee_crit_chance(),
          p.composite_melee_crit_rating() );
      j++;
    }
    else
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Melee Crit</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * buffed_stats.attack_crit_chance,
          100 * p.composite_melee_crit_chance(),
          p.composite_melee_crit_rating() );
      j++;
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Spell Crit</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * buffed_stats.spell_crit_chance,
          100 * p.composite_spell_crit_chance(),
          p.composite_spell_crit_rating() );
      j++;
    }
    if ( p.composite_melee_haste() == p.composite_spell_haste() )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Haste</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 *
              ( 1 / buffed_stats.attack_haste -
                1 ),  // Melee/Spell haste have been merged into a single stat.
          100 * ( 1 / p.composite_melee_haste() - 1 ),
          p.composite_melee_haste_rating() );
      j++;
    }
    else
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Melee Haste</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * ( 1 / buffed_stats.attack_haste - 1 ),
          100 * ( 1 / p.composite_melee_haste() - 1 ),
          p.composite_melee_haste_rating() );
      j++;
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Spell Haste</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * ( 1 / buffed_stats.spell_haste - 1 ),
          100 * ( 1 / p.composite_spell_haste() - 1 ),
          p.composite_spell_haste_rating() );
      j++;
    }
    if ( p.composite_spell_speed() != p.composite_spell_haste() )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Spell Speed</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * ( 1 / buffed_stats.spell_speed - 1 ),
          100 * ( 1 / p.composite_spell_speed() - 1 ),
          p.composite_spell_haste_rating() );
      j++;
    }
    if ( p.composite_melee_speed() != p.composite_melee_haste() )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Swing Speed</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * ( 1 / buffed_stats.attack_speed - 1 ),
          100 * ( 1 / p.composite_melee_speed() - 1 ),
          p.composite_melee_haste_rating() );
      j++;
    }
    os.printf(
        "<tr%s>\n"
        "<th class=\"left\">Damage / Heal Versatility</th>\n"
        "<td class=\"right\"></td>\n"
        "<td class=\"right\"></td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100 * buffed_stats.damage_versatility,
        100 * p.composite_damage_versatility(),
        p.composite_damage_versatility_rating() );
    j++;
    if ( p.primary_role() == ROLE_TANK )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Mitigation Versatility</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          100 * buffed_stats.mitigation_versatility,
          100 * p.composite_mitigation_versatility(),
          p.composite_mitigation_versatility_rating() );
      j++;
    }
    if ( buffed_stats.manareg_per_second > 0 )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">ManaReg per Second</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">0</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "",
          buffed_stats.manareg_per_second, p.resource_regen_per_second( RESOURCE_MANA ) );
    }
    j++;
    if ( buffed_stats.attack_power > 0 )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Attack Power</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", buffed_stats.attack_power,
          p.composite_melee_attack_power() *
              p.composite_attack_power_multiplier(),
          p.initial.stats.attack_power );
      j++;
    }
    os.printf(
        "<tr%s>\n"
        "<th class=\"left\">Mastery</th>\n"
        "<td class=\"right\"></td>\n"
        "<td class=\"right\"></td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "",
        100.0 * buffed_stats.mastery_value, 100.0 * p.cache.mastery_value(),
        p.composite_mastery_rating() );
    j++;
    if ( buffed_stats.mh_attack_expertise > 7.5 )
    {
      if ( p.dual_wield() )
      {
        os.printf(
            "<tr%s>\n"
            "<th class=\"left\">Expertise</th>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\">%.2f%% / %.2f%%</td>\n"
            "<td class=\"right\">%.2f%% / %.2f%% </td>\n"
            "<td class=\"right\">%.0f </td>\n"
            "</tr>\n",
            ( j % 2 == 1 ) ? " class=\"odd\"" : "",
            100 * buffed_stats.mh_attack_expertise,
            100 * buffed_stats.oh_attack_expertise,
            100 * p.composite_melee_expertise( &( p.main_hand_weapon ) ),
            100 * p.composite_melee_expertise( &( p.off_hand_weapon ) ),
            p.composite_expertise_rating() );
        j++;
      }
      else
      {
        os.printf(
            "<tr%s>\n"
            "<th class=\"left\">Expertise</th>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\">%.2f%%</td>\n"
            "<td class=\"right\">%.2f%% </td>\n"
            "<td class=\"right\">%.0f </td>\n"
            "</tr>\n",
            ( j % 2 == 1 ) ? " class=\"odd\"" : "",
            100 * buffed_stats.mh_attack_expertise,
            100 * p.composite_melee_expertise( &( p.main_hand_weapon ) ),
            p.composite_expertise_rating() );
        j++;
      }
    }
    os.printf(
        "<tr%s>\n"
        "<th class=\"left\">Armor</th>\n"
        "<td class=\"right\"></td>\n"
        "<td class=\"right\"></td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "<td class=\"right\">%.0f</td>\n"
        "</tr>\n",
        ( j % 2 == 1 ) ? " class=\"odd\"" : "", buffed_stats.armor,
        p.composite_armor(), p.initial.stats.armor );
    j++;
    if ( buffed_stats.bonus_armor > 0 )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Bonus Armor</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", buffed_stats.bonus_armor,
          p.composite_bonus_armor(), p.initial.stats.bonus_armor );
      j++;
    }
    if ( buffed_stats.run_speed > 0 )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Run Speed</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", buffed_stats.run_speed,
          p.composite_run_speed(), p.composite_speed_rating() );
      j++;
    }
    if ( buffed_stats.leech > 0 )
    {
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Leech</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", 100 * buffed_stats.leech,
          100 * p.composite_leech(), p.composite_leech_rating() );
      j++;
    }
    if ( p.primary_role() == ROLE_TANK )
    {
      if ( buffed_stats.avoidance > 0 )
      {
        os.printf(
            "<tr%s>\n"
            "<th class=\"left\">Avoidance</th>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\"></td>\n"
            "<td class=\"right\">%.0f</td>\n"
            "<td class=\"right\">%.0f</td>\n"
            "<td class=\"right\">%.0f</td>\n"
            "</tr>\n",
            ( j % 2 == 1 ) ? " class=\"odd\"" : "", buffed_stats.avoidance,
            p.composite_avoidance(), p.composite_avoidance_rating() );
        j++;
      }
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Tank-Miss</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", 100 * buffed_stats.miss,
          100 * ( p.cache.miss() ), 0.0 );
      j++;
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Tank-Dodge</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", 100 * buffed_stats.dodge,
          100 * ( p.composite_dodge() ), p.composite_dodge_rating() );
      j++;
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Tank-Parry</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", 100 * buffed_stats.parry,
          100 * ( p.composite_parry() ), p.composite_parry_rating() );
      j++;
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Tank-Block</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", 100 * buffed_stats.block,
          100 * p.composite_block(), p.composite_block_rating() );
      j++;
      os.printf(
          "<tr%s>\n"
          "<th class=\"left\">Tank-Crit</th>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\"></td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.2f%%</td>\n"
          "<td class=\"right\">%.0f</td>\n"
          "</tr>\n",
          ( j % 2 == 1 ) ? " class=\"odd\"" : "", 100 * buffed_stats.crit,
          100 * p.cache.crit_avoidance(), 0.0 );
      j++;
    }

    os << "</table>\n"
       << "</div>\n"
       << "</div>\n";
  }
}

// print_html_talents_player ================================================

void print_html_talents( report::sc_html_stream& os, const player_t& p )
{
  if ( p.collected_data.fight_length.mean() > 0 )
  {
    os << "<div class=\"player-section talents\">\n"
       << "<h3 class=\"toggle\">Talents</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th>Level</th>\n"
       << "<th></th>\n"
       << "<th></th>\n"
       << "<th></th>\n"
       << "</tr>\n";

    for ( uint32_t row = 0; row < MAX_TALENT_ROWS; row++ )
    {
      if ( row == 6 )
      {
        os.printf(
            "<tr>\n"
            "<th class=\"left\">%d</th>\n",
            100 );
      }
      else
      {
        os.printf(
            "<tr>\n"
            "<th class=\"left\">%d</th>\n",
            ( row + 1 ) * 15 );
      }
      for ( uint32_t col = 0; col < MAX_TALENT_COLS; col++ )
      {
        talent_data_t* t = talent_data_t::find( p.type, row, col,
                                                p.specialization(), p.dbc.ptr );
        std::string name = "none";
        if ( t && t->name_cstr() )
        {
          name = t->name_cstr();
          if ( t->specialization() != SPEC_NONE )
          {
            name += " (";
            name += util::specialization_string( t->specialization() );
            name += ")";
          }
        }
        if ( p.talent_points.has_row_col( row, col ) )
        {
          os.printf( "<td class=\"filler\">%s</td>\n", name.c_str() );
        }
        else
        {
          os.printf( "<td>%s</td>\n", name.c_str() );
        }
      }
      os << "</tr>\n";
    }

    os << "</table>\n"
       << "</div>\n"
       << "</div>\n";
  }
}

// print_html_player_scale_factor_table =====================================

void print_html_player_scale_factor_table(
    report::sc_html_stream& os, const sim_t&, const player_t& p,
    const player_processed_report_information_t& ri, scale_metric_e sm )
{
  int colspan = static_cast<int>( p.scaling->scaling_stats[ sm ].size() );
  std::vector<stat_e> scaling_stats = p.scaling->scaling_stats[ sm ];

  os << "<table class=\"sc mt\">\n";

  os << "<tr>\n"
     << "<th colspan=\"" << util::to_string( 1 + colspan )
     << "\"><a href=\"#help-scale-factors\" class=\"help\">Scale Factors for "
     << p.scaling_for_metric( sm ).name << "</a></th>\n"
     << "</tr>\n";

  os << "<tr>\n"
     << "<th></th>\n";

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
  {
    os.printf( "<th>%s</th>\n", util::stat_type_abbrev( scaling_stats[ i ] ) );
  }
  if ( p.sim->scaling->scale_lag )
  {
    os << "<th>ms Lag</th>\n";
    colspan++;
  }
  os << "</tr>\n";
  os << "<tr>\n"
     << "<th class=\"left\">Scale Factors</th>\n";

  for ( const auto& stat : scaling_stats )
  {
    if ( std::abs( p.scaling->scaling[ sm ].get_stat( stat ) ) > 1.0e5 )
      os.printf( "<td>%.*e</td>\n", p.sim->report_precision,
                 p.scaling->scaling[ sm ].get_stat( stat ) );
    else
      os.printf( "<td>%.*f</td>\n", p.sim->report_precision,
                 p.scaling->scaling[ sm ].get_stat( stat ) );
  }
  if ( p.sim->scaling->scale_lag )
    os.printf( "<td>%.*f</td>\n", p.sim->report_precision,
               p.scaling->scaling_lag[ sm ] );
  os << "</tr>\n";
  os << "<tr>\n"
     << "<th class=\"left\">Normalized</th>\n";

  for ( const auto& stat : scaling_stats )
  {
    os.printf( "<td>%.*f</td>\n", p.sim->report_precision,
               p.scaling->scaling_normalized[ sm ].get_stat( stat ) );
  }
  os << "</tr>\n";
  os << "<tr>\n"
     << "<th class=\"left\">Scale Deltas</th>\n";

  for ( const auto& stat : scaling_stats )
  {
    double value = p.sim->scaling->stats.get_stat( stat );
    std::string prefix;
    if ( p.sim->scaling->center_scale_delta == 1 && stat != STAT_SPIRIT &&
         stat != STAT_HIT_RATING && stat != STAT_EXPERTISE_RATING )
    {
      value /= 2;
      prefix = "+/- ";
    }

    os.printf( "<td>%s%.0f</td>\n", prefix.c_str(), value );
  }
  if ( p.sim->scaling->scale_lag )
    os << "<td>100</td>\n";
  os << "</tr>\n";
  os << "<tr>\n"
     << "<th class=\"left\">Error</th>\n";

  for ( const auto& stat : scaling_stats )
    if ( std::abs( p.scaling->scaling[ sm ].get_stat( stat ) ) > 1.0e5 )
      os.printf( "<td>%.*e</td>\n", p.sim->report_precision,
                 p.scaling->scaling_error[ sm ].get_stat( stat ) );
    else
      os.printf( "<td>%.*f</td>\n", p.sim->report_precision,
                 p.scaling->scaling_error[ sm ].get_stat( stat ) );

  if ( p.sim->scaling->scale_lag )
    os.printf( "<td>%.*f</td>\n", p.sim->report_precision,
               p.scaling->scaling_lag_error[ sm ] );
  os << "</tr>\n";

  /*
  os.printf(
      "<tr class=\"left\">\n"
      "<th>Gear Ranking</th>\n"
      "<td colspan=\"%i\" class=\"filler\">\n"
      "<ul class=\"float\">\n",
      colspan );
  if ( !ri.gear_weights_wowhead_std_link[ sm ].empty() )
    os.printf( "<li><a href=\"%s\" class=\"ext\">wowhead</a></li>\n",
               ri.gear_weights_wowhead_std_link[ sm ].c_str() );
  os << "</ul>\n";
  os << "</td>\n";
  os << "</tr>\n";

  // Optimizers section
  os.printf(
      "<tr class=\"left\">\n"
      "<th>Optimizers</th>\n"
      "<td colspan=\"%i\" class=\"filler\">\n"
      "<ul class=\"float\">\n",
      colspan );

  // askmrrobot
  if ( !ri.gear_weights_askmrrobot_link[ sm ].empty() )
  {
    os.printf( "<li><a href=\"%s\" class=\"ext\">askmrrobot</a></li>\n",
               ri.gear_weights_askmrrobot_link[ sm ].c_str() );
  }

  // close optimizers section
  os << "</ul>\n";
  os << "</td>\n";
  os << "</tr>\n";
  */

  // Text Ranking
  os.printf(
      "<tr class=\"left\">\n"
      "<th><a href=\"#help-scale-factor-ranking\" "
      "class=\"help\">Ranking</a></th>\n"
      "<td colspan=\"%i\" class=\"filler\">\n"
      "<ul class=\"float\">\n"
      "<li>",
      colspan );

  for ( size_t i = 0; i < scaling_stats.size(); i++ )
  {
    if ( i > 0 )
    {
      // holy hell this was hard to read - splitting this out into
      // human-readable code
      double separation =
          fabs( p.scaling->scaling[ sm ].get_stat( scaling_stats[ i - 1 ] ) -
                p.scaling->scaling[ sm ].get_stat( scaling_stats[ i ] ) );
      double error_est = sqrt(
          p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i - 1 ] ) *
              p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i - 1 ] ) /
              4 +
          p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i ] ) *
              p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i ] ) /
              4 );
      if ( separation > ( error_est * 2 ) )
        os << " > ";
      else
        os << " ~= ";
    }

    os.printf( "%s", util::stat_type_abbrev( scaling_stats[ i ] ) );
  }
  os << "</table>\n";
  if ( !ri.gear_weights_pawn_string[ sm ].empty() )
  {
    os << "<table class=\"sc mt\">\n";
    os.printf(
        "<tr class\"left\">\n"
        "<th>Pawn string</th>\n"
        "<td colspan=\"%i\" class=\"filler\">\n"
        "%s\n"
        "</td>\n"
        "</tr>\n",
        colspan, ri.gear_weights_pawn_string[ sm ].c_str() );
    os << "</table>\n";
  }
}

// print_html_player_scale_factors ==========================================

void print_html_player_scale_factors(
    report::sc_html_stream& os, const player_t& p,
    const player_processed_report_information_t& ri )
{
  const sim_t& sim = *( p.sim );
  if ( !p.is_pet() )
  {
    if ( p.sim->scaling->has_scale_factors() )
    {
      // print the scale factors for the metric the user is requesting
      scale_metric_e default_sm = p.sim->scaling->scaling_metric;
      print_html_player_scale_factor_table( os, sim, p, ri, default_sm );

      // create a collapsable section containing scale factors for other metrics
      os << "<h3 class=\"toggle\">Scale Factors for other metrics</h3>\n"
         << "<div class=\"toggle-content hide\">\n";

      for ( scale_metric_e sm = SCALE_METRIC_DPS; sm < SCALE_METRIC_MAX; sm++ )
      {
        if ( sm != default_sm && sm != SCALE_METRIC_DEATHS )
          print_html_player_scale_factor_table( os, sim, p, ri, sm );
      }

      os << "</div>\n";

      // produce warning if sim iterations are too low to produce reliable scale
      // factors
      if ( sim.iterations < 10000 )
      {
        os << "<div class=\"alert\">\n"
           << "<h3>Warning</h3>\n"
           << "<p>Scale Factors generated using less than 10,000 iterations "
              "may vary significantly from run to run.</p>\n<br>";
        // Scale factor warnings:
        if ( sim.scaling->scale_factor_noise > 0 &&
             sim.scaling->scale_factor_noise <
                 p.scaling->scaling_lag_error[ default_sm ] /
                     fabs( p.scaling->scaling_lag[ default_sm ] ) )
          os.printf(
              "<p>Player may have insufficient iterations (%d) to calculate "
              "scale factor for lag (error is >%.0f%% delta score)</p>\n",
              sim.iterations, sim.scaling->scale_factor_noise * 100.0 );
        for ( size_t i = 0; i < p.scaling->scaling_stats[ default_sm ].size(); i++ )
        {
          scale_metric_e sm = sim.scaling->scaling_metric;
          double value =
              p.scaling->scaling[ sm ].get_stat( p.scaling->scaling_stats[ default_sm ][ i ] );
          double error = p.scaling->scaling_error[ sm ].get_stat(
              p.scaling->scaling_stats[ default_sm ][ i ] );
          if ( sim.scaling->scale_factor_noise > 0 &&
               sim.scaling->scale_factor_noise < error / fabs( value ) )
            os.printf(
                "<p>Player may have insufficient iterations (%d) to calculate "
                "scale factor for stat %s (error is >%.0f%% delta score)</p>\n",
                sim.iterations,
                util::stat_type_string( p.scaling->scaling_stats[ default_sm ][ i ] ),
                sim.scaling->scale_factor_noise * 100.0 );
        }
        os << "</div>\n";
      }
    }
  }
  os << "</div>\n"
     << "</div>\n";
}

bool print_html_sample_sequence_resource(
    const player_t& p,
    const player_collected_data_t::action_sequence_data_t& data, resource_e r )
{
  if ( !p.resources.active_resource[ r ] && !p.sim->maximize_reporting )
    return false;

  if ( data.resource_snapshot[ r ] < 0 )
    return false;

  if ( p.role == ROLE_TANK && r == RESOURCE_HEALTH )
    return true;

  if ( p.resources.is_infinite( r ) )
    return false;

  return true;
}

void print_html_sample_sequence_string_entry(
    report::sc_html_stream& os,
    const player_collected_data_t::action_sequence_data_t& data,
    const player_t& p, bool precombat = false )
{
  // Skip waiting on the condensed list
  if ( !data.action )
    return;

  std::string targetname = data.action->harmful ? data.target->name() : "none";
  std::string time_str;
  if ( precombat )
  {
    time_str = "Pre";
  }
  else
  {
    auto time_str = fmt::format("{:d}{:02d}.{:03d}", (int)data.time.total_minutes(),
                                                     (int)data.time.total_seconds() % 60,
                                                     (int)data.time.total_millis() % 1000 );
  }

  os.printf( "<span class=\"%s_seq_target_%s\" title=\"[%s] %s%s\n|", p.name(),
             targetname.c_str(), time_str, data.action->name(),
             ( targetname == "none" ? "" : " @ " + targetname ).c_str() );

  resource_e pr = p.primary_resource();

  if ( print_html_sample_sequence_resource( p, data, pr ) )
  {
    if ( pr == RESOURCE_HEALTH || pr == RESOURCE_MANA )
      os.printf( " %d%%", (int)( ( data.resource_snapshot[ pr ] /
                                   data.resource_max_snapshot[ pr ] ) *
                                 100 ) );
    else
      os.printf( " %.1f", data.resource_snapshot[ pr ] );
    os.printf( " %s |", util::resource_type_string( pr ) );
  }

  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; ++r )
  {
    if ( print_html_sample_sequence_resource( p, data, r ) && r != pr )
    {
      if ( r == RESOURCE_HEALTH || r == RESOURCE_MANA )
        os.printf( " %d%%", (int)( ( data.resource_snapshot[ r ] /
                                     data.resource_max_snapshot[ r ] ) *
                                   100 ) );
      else
        os.printf( " %.1f", data.resource_snapshot[ r ] );
      os.printf( " %s |", util::resource_type_string( r ) );
    }
  }

  for ( size_t b = 0; b < data.buff_list.size(); ++b )
  {
    buff_t* buff = data.buff_list[ b ].first;
    int stacks   = gsl::narrow_cast<int>(data.buff_list[ b ].second[0]);
    if ( !buff->constant )
    {
      os.printf( "\n%s", buff->name() );
      if ( stacks > 1 )
        os.printf( "(%d)", stacks );
    }
  }

  os.printf( "\">%c</span>", data.action ? data.action->marker : 'W' );
}
// print_html_sample_sequence_table_entry =====================================

void print_html_sample_sequence_table_entry(
    report::sc_html_stream& os,
    const player_collected_data_t::action_sequence_data_t& data,
    const player_t& p, bool precombat = false )
{
  os << "<tr>\n";

  if ( precombat )
    os << "<td class=\"right\">Pre</td>\n";
  else
    os.printf( "<td class=\"right\">%d:%02d.%03d</td>\n",
               (int)data.time.total_minutes(),
               (int)data.time.total_seconds() % 60,
               (int)data.time.total_millis() % 1000 );

  if ( data.action )
  {
    os.printf(
        "<td class=\"left\">%s</td>\n"
        "<td class=\"left\">%c</td>\n"
        "<td class=\"left\">%s</td>\n"
        "<td class=\"left\">%s</td>\n",
        data.action->action_list ? data.action->action_list->name_str.c_str() : "default",
        data.action->marker != 0 ? data.action->marker : ' ',
        data.action->name(),
        data.target->name() );
  }
  else
  {
    os.printf(
        "<td class=\"left\">Waiting</td>\n"
        "<td class=\"left\">&nbsp;</td>\n"
        "<td class=\"left\">&nbsp;</td>\n"
        "<td class=\"left\">%.3f sec</td>\n",
        data.wait_time.total_seconds() );
  }

  os.printf( "<td class=\"left\">" );

  bool first    = true;
  resource_e pr = p.primary_resource();

  if ( print_html_sample_sequence_resource( p, data, pr ) )
  {
    if ( first )
      first = false;

    os.printf( " %.1f/%.0f: <b>%.0f%%", data.resource_snapshot[ pr ],
               data.resource_max_snapshot[ pr ],
               data.resource_snapshot[ pr ] / data.resource_max_snapshot[ pr ] *
                   100.0 );
    os.printf( " %s</b>", util::resource_type_string( pr ) );
  }

  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; ++r )
  {
    if ( print_html_sample_sequence_resource( p, data, r ) && r != pr )
    {
      if ( first )
        first = false;
      else
        os.printf( " | " );

      os.printf( " %.1f/%.0f: <b>%.0f%%", data.resource_snapshot[ r ],
                 data.resource_max_snapshot[ r ],
                 data.resource_snapshot[ r ] / data.resource_max_snapshot[ r ] *
                     100.0 );
      os.printf( " %s</b>", util::resource_type_string( r ) );
    }
  }

  os.printf( "</td>\n<td class=\"left\">" );

  first = true;
  for ( size_t b = 0; b < data.buff_list.size(); ++b )
  {
    buff_t* buff = data.buff_list[ b ].first;
    int stacks   = gsl::narrow_cast<int>(data.buff_list[ b ].second[0]);

    if ( !buff->constant )
    {
      if ( first )
        first = false;
      else
        os.printf( ", " );

      os.printf( "%s", buff->name() );
      if ( stacks > 1 )
        os.printf( "(%d)", stacks );
    }
  }

  os << "</td>\n";
  os << "</tr>\n";
}

// print_html_player_action_priority_list =====================================

void print_html_player_action_priority_list( report::sc_html_stream& os,
                                             const player_t& p )
{
  const sim_t& sim = *( p.sim );
  os << "<div class=\"player-section action-priority-list\">\n"
     << "<h3 class=\"toggle\">Action Priority List</h3>\n"
     << "<div class=\"toggle-content hide\">\n";

  action_priority_list_t* alist = nullptr;

  for ( size_t i = 0; i < p.action_list.size(); ++i )
  {
    const action_t* a = p.action_list[ i ];

    if ( a->signature_str.empty() || !( a->marker || a->action_list ) )
      continue;

    if ( !alist || a->action_list->name_str != alist->name_str )
    {
      if ( alist && alist->used )
      {
        os << "</table>\n";
      }

      // alist = p -> find_action_priority_list( a -> action_list );
      alist = a->action_list;

      if ( !alist->used )
        continue;

      std::string als =
          util::encode_html( ( alist->name_str == "default" )
                                 ? "Default action list"
                                 : ( "actions." + alist->name_str ).c_str() );
      if ( !alist->action_list_comment_str.empty() )
        als += "&#160;<small><em>" +
               util::encode_html( alist->action_list_comment_str.c_str() ) +
               "</em></small>";
      os.printf(
          "<table class=\"sc\">\n"
          "<tr>\n"
          "<th class=\"right\"></th>\n"
          "<th class=\"right\"></th>\n"
          "<th class=\"left\">%s</th>\n"
          "</tr>\n"
          "<tr>\n"
          "<th class=\"right\">#</th>\n"
          "<th class=\"right\">count</th>\n"
          "<th class=\"left\">action,conditions</th>\n"
          "</tr>\n",
          als.c_str() );
    }

    if ( !alist->used )
      continue;

    os << "<tr";
    if ( ( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    std::string as = util::encode_html( a->signature->action_.c_str() );
    if ( !a->signature->comment_.empty() )
      as += "<br/><small><em>" +
            util::encode_html( a->signature->comment_.c_str() ) +
            "</em></small>";

    os.printf(
        "<td class=\"right\" style=\"vertical-align:top\">%c</td>\n"
        "<td class=\"left\" style=\"vertical-align:top\">%.2f</td>\n"
        "<td class=\"left\">%s</td>\n"
        "</tr>\n",
        a->marker ? a->marker : ' ',
        a->total_executions /
          (double)( sim.single_actor_batch ?
            a -> player -> collected_data.total_iterations + sim.threads :
            sim.iterations ),
        as.c_str() );
  }

  if ( alist && alist->used )
  {
    os << "</table>\n";
  }

  // Sample Sequences

  if ( !p.collected_data.action_sequence.empty() && !p.is_enemy()  )
  {
    std::vector<std::string> targets;

    targets.push_back( "none" );
    if ( p.target )
    {
      targets.push_back( p.target->name() );
    }

    for ( const auto& sequence_data : p.collected_data.action_sequence )
    {
      if ( !sequence_data.action || !sequence_data.action->harmful )
        continue;
      bool found = false;
      for ( size_t j = 0; j < targets.size(); ++j )
      {
        if ( targets[ j ] == sequence_data.target->name() )
        {
          found = true;
          break;
        }
      }
      if ( !found )
        targets.push_back( sequence_data.target->name() );
    }

    // Sample Sequence (text string)

    os << "<div class=\"subsection subsection-small\">\n"
       << "<h4>Sample Sequence</h4>\n"
       << "<div class=\"force-wrap mono\">\n";

    os << "<style type=\"text/css\" media=\"all\" scoped>\n";

    char colors[ 12 ][ 7 ] = {"eeeeee", "ffffff", "ff5555", "55ff55",
                              "5555ff", "ffff55", "55ffff", "ff9999",
                              "99ff99", "9999ff", "ffff99", "99ffff"};

    int j = 0;

    for ( size_t i = 0; i < targets.size(); ++i )
    {
      if ( j == 12 )
        j = 2;
      os.printf( ".%s_seq_target_%s { color: #%s; }\n", p.name(),
                 targets[ i ].c_str(), colors[ j ] );
      j++;
    }

    os << "</style>\n";

    for ( const auto& sequence_data :
          p.collected_data.action_sequence_precombat )
    {
      print_html_sample_sequence_string_entry( os, sequence_data, p, true );
    }

    for ( const auto& sequence_data : p.collected_data.action_sequence )
    {
      print_html_sample_sequence_string_entry( os, sequence_data, p );
    }

    os << "\n</div>\n"
       << "</div>\n";

    // Sample Sequence (table)

    // define collapsable section
    os << "<h3 class=\"toggle\">Sample Sequence Table</h3>\n"
       << "<div class=\"toggle-content hide\">\n";

    // create table header
    os.printf(
        "<table class=\"sc\">\n"
        "<tr>\n"
        "<th class=\"center\">time</th>\n"
        "<th class=\"center\">list</th>\n"
        "<th class=\"center\">#</th>\n"
        "<th class=\"center\">name</th>\n"
        "<th class=\"center\">target</th>\n"
        "<th class=\"center\">resources</th>\n"
        "<th class=\"center\">buffs</th>\n"
        "</tr>\n" );

    for ( const auto& sequence_data :
          p.collected_data.action_sequence_precombat )
    {
      print_html_sample_sequence_table_entry( os, sequence_data, p, true );
    }

    for ( const auto& sequence_data : p.collected_data.action_sequence )
    {
      print_html_sample_sequence_table_entry( os, sequence_data, p );
    }

    // close table
    os << "</table>\n";

    // close collapsable section
    os << "</div>\n";
  }

  // End Action Priority List section
  os << "</div>\n"
     << "</div>\n";
}

// print_html_player_statistics =============================================

void print_html_player_statistics(
    report::sc_html_stream& os, const player_t& p,
    const player_processed_report_information_t& )
{
  // Statistics & Data Analysis

  os << "<div class=\"player-section gains\">\n"
        "<h3 class=\"toggle\">Statistics & Data Analysis</h3>\n"
        "<div class=\"toggle-content hide\">\n"
        "<table class=\"sc\">\n";

  int sd_counter = 0;
  report::print_html_sample_data( os, p, p.collected_data.fight_length,
                                  "Fight Length", sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.dps, "DPS",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.prioritydps,
                                  "Priority Target DPS", sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.dpse, "DPS(e)",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.dmg, "Damage",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.dtps, "DTPS",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.hps, "HPS",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.hpse, "HPS(e)",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.heal, "Heal",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.htps, "HTPS",
                                  sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.theck_meloree_index,
                                  "TMI", sd_counter );
  report::print_html_sample_data(
      os, p, p.collected_data.effective_theck_meloree_index, "ETMI",
      sd_counter );
  report::print_html_sample_data( os, p, p.collected_data.max_spike_amount,
                                  "MSD", sd_counter );

  for ( const auto& sample_data : p.sample_data_list )
  {
    report::print_html_sample_data( os, p, *sample_data, sample_data->name_str,
                                    sd_counter );
  }

  os << "</table>\n"
        "</div>\n"
        "</div>\n";
}

void print_html_gain( report::sc_html_stream& os, const gain_t& g,
                      std::array<double, RESOURCE_MAX>& total_gains, int& j,
                      bool report_overflow = true )
{
  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( g.actual[ i ] != 0 || g.overflow[ i ] != 0 )
    {
      double overflow_pct =
          100.0 * g.overflow[ i ] / ( g.actual[ i ] + g.overflow[ i ] );
      os << "<tr";
      if ( j & 1 )
      {
        os << " class=\"odd\"";
      }
      ++j;
      os << ">\n";
      os << "<td class=\"left\">" << g.name() << "</td>\n";
      os << "<td class=\"left\">"
         << util::inverse_tokenize( util::resource_type_string( i ) )
         << "</td>\n";
      os << "<td class=\"right\">" << g.count[ i ] << "</td>\n";
      os.printf(
          "<td class=\"right\">%.2f (%.2f%%)</td>\n", g.actual[ i ],
          g.actual[ i ] ? g.actual[ i ] / total_gains[ i ] * 100.0 : 0.0 );
      os << "<td class=\"right\">" << g.actual[ i ] / g.count[ i ] << "</td>\n";

      if ( report_overflow )
      {
        os << "<td class=\"right\">" << g.overflow[ i ] << "</td>\n";
        os << "<td class=\"right\">" << overflow_pct << "%</td>\n";
      }
      os << "</tr>\n";
    }
  }
}

void get_total_player_gains( const player_t& p,
                             std::array<double, RESOURCE_MAX>& total_gains )
{
  for ( const auto& gain : p.gain_list )
  {
    // Wish std::array had += operator overload!
    for ( size_t j = 0; j < total_gains.size(); ++j )
      total_gains[ j ] += gain->actual[ j ];
  }
}
// print_html_player_resources ==============================================

void print_html_player_resources( report::sc_html_stream& os, const player_t& p,
                                  const player_processed_report_information_t& )
{
  // Resources Section

  os << "<div class=\"player-section gains\">\n";
  os << "<h3 class=\"toggle open\">Resources</h3>\n";
  os << "<div class=\"toggle-content\">\n";
  os << "<table class=\"sc mt\">\n";
  os << "<tr>\n";
  os << "<th>Resource Usage</th>\n";
  os << "<th>Type</th>\n";
  os << "<th>Count</th>\n";
  os << "<th>Total</th>\n";
  os << "<th>Average</th>\n";
  os << "<th>RPE</th>\n";
  os << "<th>APR</th>\n";
  os << "</tr>\n";

  os << "<tr>\n";
  os << "<th class=\"left small\">" << util::encode_html( p.name() )
     << "</th>\n";
  os << "<td colspan=\"6\" class=\"filler\"></td>\n";
  os << "</tr>\n";

  int k = 0;
  for ( const auto& stat : p.stats_list )
  {
    if ( stat->rpe_sum > 0 )
    {
      k = print_html_action_resource( os, *stat, k );
    }
  }

  for ( const auto& pet : p.pet_list )
  {
    bool first = true;

    for ( const auto& stat : pet->stats_list )
    {
      if ( stat->rpe_sum > 0 )
      {
        if ( first )
        {
          first = false;
          os << "<tr>\n";
          os << "<th class=\"left small\">pet - "
             << util::encode_html( pet->name_str ) << "</th>\n";
          os << "<td colspan=\"6\" class=\"filler\"></td>\n";
          os << "</tr>\n";
        }
        k = print_html_action_resource( os, *stat, k );
      }
    }
  }

  os << "</table>\n";

  std::array<double, RESOURCE_MAX> total_player_gains =
      std::array<double, RESOURCE_MAX>();
  get_total_player_gains( p, total_player_gains );
  bool has_gains = false;
  for ( size_t i = 0; i < RESOURCE_MAX; i++ )
  {
    if ( total_player_gains[ i ] > 0 )
    {
      has_gains = true;
      break;
    }
  }

  if ( has_gains )
  {
    // Resource Gains Section
    os << "<table class=\"sc\">\n";
    os << "<tr>\n";
    os << "<th>Resource Gains</th>\n";
    os << "<th>Type</th>\n";
    os << "<th>Count</th>\n";
    os << "<th>Total</th>\n";
    os << "<th>Average</th>\n";
    os << "<th colspan=\"2\">Overflow</th>\n";
    os << "</tr>\n";

    int j = 0;

    for ( const auto& gain : p.gain_list )
    {
      print_html_gain( os, *gain, total_player_gains, j );
    }
    for ( const auto& pet : p.pet_list )
    {
      if ( pet->collected_data.fight_length.mean() <= 0 )
        continue;
      bool first = true;
      std::array<double, RESOURCE_MAX> total_pet_gains =
          std::array<double, RESOURCE_MAX>();
      get_total_player_gains( *pet, total_pet_gains );
      for ( const auto& gain : pet->gain_list )
      {
        if ( first )
        {
          bool found = false;
          for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; r++ )
          {
            if ( gain->actual[ r ] != 0 || gain->overflow[ r ] != 0 )
            {
              found = true;
              break;
            }
          }
          if ( found )
          {
            first = false;
            os << "<tr>\n";
            os << "<th>pet - " << util::encode_html( pet->name_str )
               << "</th>\n";
            os << "<td colspan=\"6\" class=\"filler\"></td>\n";
            os << "</tr>\n";
          }
        }
        print_html_gain( os, *gain, total_pet_gains, j );
      }
    }
    os << "</table>\n";
  }

  // Resource Consumption Section
  os << "<table class=\"sc\">\n";
  os << "<tr>\n";
  os << "<th>Resource</th>\n";
  os << "<th>RPS-Gain</th>\n";
  os << "<th>RPS-Loss</th>\n";
  os << "</tr>\n";
  int j = 0;
  for ( size_t i = 0, end = p.collected_data.resource_gained.size(); i < end; ++i )
  {
    auto rt = static_cast<resource_e>( i );
    double rps_gain = p.collected_data.resource_gained[ rt ].mean() /
                      p.collected_data.fight_length.mean();
    double rps_loss = p.collected_data.resource_lost[ rt ].mean() /
                      p.collected_data.fight_length.mean();
    if ( rps_gain <= 0 && rps_loss <= 0 )
      continue;

    os << "<tr";
    if ( j & 1 )
    {
      os << " class=\"odd\"";
    }
    ++j;
    os << ">\n";
    os << "<td class=\"left\">"
       << util::inverse_tokenize( util::resource_type_string( rt ) )
       << "</td>\n";
    os << "<td class=\"right\">" << rps_gain << "</td>\n";
    os << "<td class=\"right\">" << rps_loss << "</td>\n";
    os << "</tr>\n";
  }
  os << "</table>\n";

  // Resource End Section
  os << "<table class=\"sc\">\n";
  os << "<tr>\n";
  os << "<th>Combat End Resource</th>\n";
  os << "<th> Mean </th>\n";
  os << "<th> Min </th>\n";
  os << "<th> Max </th>\n";
  os << "</tr>\n";
  j = 0;
  for ( size_t i = 0, end = p.collected_data.combat_end_resource.size(); i < end; ++i )
  {
    auto rt = static_cast<resource_e>( i );

    if ( p.resources.base[ rt ] <= 0 )
      continue;

    os << "<tr";
    if ( j & 1 )
    {
      os << " class=\"odd\"";
    }
    ++j;
    os << ">\n";
    os << "<td class=\"left\">"
       << util::inverse_tokenize( util::resource_type_string( rt ) )
       << "</td>\n";
    os << "<td class=\"right\">"
       << p.collected_data.combat_end_resource[ rt ].mean() << "</td>\n";
    os << "<td class=\"right\">"
       << p.collected_data.combat_end_resource[ rt ].min() << "</td>\n";
    os << "<td class=\"right\">"
       << p.collected_data.combat_end_resource[ rt ].max() << "</td>\n";
    os << "</tr>\n";
  }
  os << "</table>\n";

  os << "<div class=\"charts charts-left\">\n";
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    double total_gain = 0;
    size_t n_gains    = 0;
    for ( const auto& gain : p.gain_list )
    {
      if ( gain->actual[ r ] > 0 )
      {
        total_gain += gain->actual[ r ];
        ++n_gains;
      }
    }

    if ( n_gains > 1 && total_gain > 0 )
    {
      highchart::pie_chart_t pc(
          highchart::build_id( p, std::string( "resource_gain_" ) +
                                      util::resource_type_string( r ) ),
          *p.sim );
      if ( chart::generate_gains( pc, p, r ) )
      {
        os << pc.to_target_div();
        p.sim->add_chart_data( pc );
      }
    }
  }
  os << "</div>\n";

  os << "<div class=\"charts\">\n";
  for ( const auto& timeline : p.collected_data.resource_timelines )
  {
    if ( p.resources.max[ timeline.type ] == 0 )
      continue;

    if ( timeline.timeline.mean() == 0 )
      continue;

    // There's no need to print resources that never change
    bool static_resource = true;
    double ival          = std::numeric_limits<double>::min();
    for ( size_t i = 1; i < timeline.timeline.data().size(); i++ )
    {
      if ( ival == std::numeric_limits<double>::min() )
      {
        ival = timeline.timeline.data()[ i ];
      }
      else if ( ival != timeline.timeline.data()[ i ] )
      {
        static_resource = false;
      }
    }

    if ( static_resource )
    {
      continue;
    }

    std::string resource_str = util::resource_type_string( timeline.type );

    highchart::time_series_t ts(
        highchart::build_id( p, "resource_" + resource_str ), *p.sim );
    chart::generate_actor_timeline( ts, p, resource_str,
                                    color::resource_color( timeline.type ),
                                    timeline.timeline );
    ts.set_mean( timeline.timeline.mean() );

    os << ts.to_target_div();
    p.sim->add_chart_data( ts );
  }
  if ( p.primary_role() == ROLE_TANK &&
       !p.is_enemy() )  // Experimental, restrict to tanks for now
  {
    highchart::time_series_t chart( highchart::build_id( p, "health_change" ),
                                    *p.sim );
    chart::generate_actor_timeline(
        chart, p, "Health Change", color::resource_color( RESOURCE_HEALTH ),
        p.collected_data.health_changes.merged_timeline );
    chart.set_mean( p.collected_data.health_changes.merged_timeline.mean() );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );

    sc_timeline_t sliding_average_tl;
    p.collected_data.health_changes.merged_timeline
        .build_sliding_average_timeline( sliding_average_tl, 6 );
    highchart::time_series_t chart2(
        highchart::build_id( p, "health_change_ma" ), *p.sim );
    chart::generate_actor_timeline(
        chart2, p, "Health Change (moving average, 6s window)",
        color::resource_color( RESOURCE_HEALTH ), sliding_average_tl );
    chart2.set_mean( sliding_average_tl.mean() );

    os << chart2.to_target_div();
    p.sim->add_chart_data( chart2 );

    // Tmp Debug Visualization
    histogram tmi_hist;
    tmi_hist.create_histogram( p.collected_data.theck_meloree_index, 50 );
    highchart::histogram_chart_t tmi_chart(
        highchart::build_id( p, "tmi_dist" ), *p.sim );
    if ( chart::generate_distribution(
             tmi_chart, &p, tmi_hist.data(), "TMI",
             p.collected_data.theck_meloree_index.mean(), tmi_hist.min(),
             tmi_hist.max() ) )
    {
      os << tmi_chart.to_target_div();
      p.sim->add_chart_data( tmi_chart );
    }
  }
  os << "</div>\n";
  os << "<div class=\"clear\"></div>\n";

  os << "</div>\n";
  os << "</div>\n";
}

// print_html_player_charts =================================================

void print_html_player_charts( report::sc_html_stream& os, const player_t& p,
                               const player_processed_report_information_t& )
{
  os << "<div class=\"player-section\">\n"
     << "<h3 class=\"toggle open\">Charts</h3>\n"
     << "<div class=\"toggle-content\">\n"
     << "<div class=\"charts charts-left\">\n";

  if ( !p.stats_list.empty() )
  {
    highchart::bar_chart_t bc( highchart::build_id( p, "dpet" ), *p.sim );
    if ( chart::generate_action_dpet( bc, p ) )
    {
      os << bc.to_target_div();
      p.sim->add_chart_data( bc );
    }

    highchart::pie_chart_t damage_pie( highchart::build_id( p, "dps_sources" ),
                                       *p.sim );
    if ( chart::generate_damage_stats_sources( damage_pie, p ) )
    {
      os << damage_pie.to_target_div();
      p.sim->add_chart_data( damage_pie );
    }

    highchart::pie_chart_t heal_pie( highchart::build_id( p, "hps_sources" ),
                                     *p.sim );
    if ( chart::generate_heal_stats_sources( heal_pie, p ) )
    {
      os << heal_pie.to_target_div();
      p.sim->add_chart_data( heal_pie );
    }
  }

  highchart::chart_t scaling_plot( highchart::build_id( p, "scaling_plot" ),
                                   *p.sim );
  if ( chart::generate_scaling_plot( scaling_plot, p,
                                     p.sim->scaling->scaling_metric ) )
  {
    os << scaling_plot.to_target_div();
    p.sim->add_chart_data( scaling_plot );
  }

  if ( p.collected_data.timeline_dmg_taken.mean() > 0 )
  {
    highchart::time_series_t dps_taken( highchart::build_id( p, "dps_taken" ),
                                        *p.sim );
    sc_timeline_t timeline_dps_taken;
    p.collected_data.timeline_dmg_taken.build_derivative_timeline(
        timeline_dps_taken );
    dps_taken.set_yaxis_title( "Damage taken per second" );
    dps_taken.set_title( p.name_str + " Damage taken per second" );
    dps_taken.add_simple_series( "area", "#FDD017", "DPS taken",
                                 timeline_dps_taken.data() );
    dps_taken.set_mean( timeline_dps_taken.mean() );

    if ( p.sim->player_no_pet_list.size() > 1 )
    {
      dps_taken.set_toggle_id( "player" + util::to_string( p.index ) +
                               "toggle" );
    }

    os << dps_taken.to_target_div();
    p.sim->add_chart_data( dps_taken );
  }

  os << "</div>\n"
     << "<div class=\"charts\">\n";

  highchart::time_series_t ts( highchart::build_id( p, "dps" ), *p.sim );
  if ( chart::generate_actor_dps_series( ts, p ) )
  {
    os << ts.to_target_div();
    p.sim->add_chart_data( ts );
  }

  highchart::chart_t ac( highchart::build_id( p, "reforge_plot" ), *p.sim );
  if ( chart::generate_reforge_plot( ac, p ) )
  {
    os << ac.to_target_div();
    p.sim->add_chart_data( ac );
  }

  scaling_metric_data_t scaling_data =
      p.scaling_for_metric( p.sim->scaling->scaling_metric );
  std::string scale_factor_id = "scale_factor_";
  scale_factor_id += util::scale_metric_type_abbrev( scaling_data.metric );
  highchart::bar_chart_t bc( highchart::build_id( p, scale_factor_id ),
                             *p.sim );
  if ( chart::generate_scale_factors( bc, p, scaling_data.metric ) )
  {
    os << bc.to_target_div();
    p.sim->add_chart_data( bc );
  }

  highchart::histogram_chart_t chart( highchart::build_id( p, "dps_dist" ),
                                      *p.sim );
  if ( chart::generate_distribution(
           chart, &p, p.collected_data.dps.distribution, p.name_str + " DPS",
           p.collected_data.dps.mean(), p.collected_data.dps.min(),
           p.collected_data.dps.max() ) )
  {
    chart.set( "tooltip.headerFormat", "<b>{point.key}</b> DPS<br/>" );
    os << chart.to_target_div();
    p.sim->add_chart_data( chart );
  }

  if ( p.collected_data.hps.mean() > 0 || p.collected_data.aps.mean() > 0 )
  {
    highchart::histogram_chart_t chart( highchart::build_id( p, "hps_dist" ),
                                        *p.sim );
    if ( chart::generate_distribution(
             chart, &p, p.collected_data.hps.distribution, p.name_str + " HPS",
             p.collected_data.hps.mean(), p.collected_data.hps.min(),
             p.collected_data.hps.max() ) )
    {
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }
  }

  highchart::pie_chart_t time_spent( highchart::build_id( p, "time_spent" ),
                                     *p.sim );
  if ( chart::generate_spent_time( time_spent, p ) )
  {
    os << time_spent.to_target_div();
    p.sim->add_chart_data( time_spent );
  }

  for ( const auto& timeline : p.collected_data.stat_timelines )
  {
    if ( timeline.timeline.mean() == 0 )
      continue;

    std::string stat_str = util::stat_type_string( timeline.type );
    highchart::time_series_t ts( highchart::build_id( p, "stat_" + stat_str ),
                                 *p.sim );
    chart::generate_actor_timeline( ts, p, stat_str,
                                    color::stat_color( timeline.type ),
                                    timeline.timeline );
    ts.set_mean( timeline.timeline.mean() );

    os << ts.to_target_div();
    p.sim->add_chart_data( ts );
  }

  os << "</div>\n"
     << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n";
}

void print_html_player_buff_spelldata( report::sc_html_stream& os, const buff_t& b, const spell_data_t& data, std::string data_name )
{
  // Spelldata
   if ( data.ok() )
   {
     os.printf(
         "<td style=\"vertical-align: top;\" class=\"filler\">\n"
         "<h4>%s details</h4>\n"
         "<ul>\n"
         "<li><span class=\"label\">id:</span>%i</li>\n"
         "<li><span class=\"label\">name:</span>%s</li>\n"
         "<li><span class=\"label\">tooltip:</span><span "
         "class=\"tooltip\">%s</span></li>\n"
         "<li><span class=\"label\">description:</span><span "
         "class=\"tooltip\">%s</span></li>\n"
         "<li><span class=\"label\">max_stacks:</span>%.i</li>\n"
         "<li><span class=\"label\">duration:</span>%.2f</li>\n"
         "<li><span class=\"label\">cooldown:</span>%.2f</li>\n"
         "<li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
         "</ul>\n"
         "</td>\n",
         data_name.c_str(),
         data.id(), data.name_cstr(),
         b.player
             ? util::encode_html(
                   report::pretty_spell_text( data, data.tooltip(),
                                              *b.player ) )
                   .c_str()
             : data.tooltip(),
         b.player
             ? util::encode_html( report::pretty_spell_text(
                                      data, data.desc(), *b.player ) )
                   .c_str()
             : data.desc(),
         data.max_stacks(), data.duration().total_seconds(),
         data.cooldown().total_seconds(), data.proc_chance() * 100 );
   }
}
// This function MUST accept non-player buffs as well!
void print_html_player_buff( report::sc_html_stream& os, const buff_t& b,
                             int report_details, size_t i,
                             bool constant_buffs = false )
{
  std::string buff_name;

  if ( b.player && b.player->is_pet() )
  {
    buff_name += b.player->name_str + '-';
  }
  buff_name += b.name_str.c_str();

  os << "<tr";
  if ( i & 1 )
  {
    os << " class=\"odd\"";
  }
  os << ">\n";
  if ( report_details )
  {
    buff_name = report::buff_decorator_t( b ).decorate();
    os.printf(
        "<td class=\"left\"><span class=\"toggle-details\">%s</span></td>\n",
        buff_name.c_str() );
  }
  else
    os.printf( "<td class=\"left\">%s</td>\n", buff_name.c_str() );

  if ( !constant_buffs )
    os.printf(
        "<td class=\"right\">%.1f</td>\n"
        "<td class=\"right\">%.1f</td>\n"
        "<td class=\"right\">%.1fsec</td>\n"
        "<td class=\"right\">%.1fsec</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.2f%%</td>\n"
        "<td class=\"right\">%.1f(%.1f)</td>\n"
        "<td class=\"right\">%.1f</td>\n",
        b.avg_start.pretty_mean(), b.avg_refresh.pretty_mean(),
        b.start_intervals.pretty_mean(), b.trigger_intervals.pretty_mean(),
        b.uptime_pct.pretty_mean(),
        b.benefit_pct.mean(),
        b.avg_overflow_count.mean(), b.avg_overflow_total.mean(),
        b.avg_expire.pretty_mean() );

  os << "</tr>\n";

  if ( report_details )
  {
    os.printf(
        "<tr class=\"details hide\">\n"
        "<td colspan=\"%d\" class=\"filler\">\n"
        "<table><tr>\n"
        "<td style=\"vertical-align: top;\" class=\"filler\">\n"
        "<h4>Buff details</h4>\n"
        "<ul>\n",
        b.constant ? 1 : 9);
    os.printf(
        "<li><span class=\"label\">buff initial source:</span>%s</li>\n"
        "<li><span class=\"label\">cooldown name:</span>%s</li>\n"
        "<li><span class=\"label\">max_stacks:</span>%.i</li>\n"
        "<li><span class=\"label\">duration:</span>%.2f</li>\n"
        "<li><span class=\"label\">cooldown:</span>%.2f</li>\n"
        "<li><span class=\"label\">default_chance:</span>%.2f%%</li>\n"
        "<li><span class=\"label\">default_value:</span>%.2f</li>\n"
        "<li><span class=\"label\">activated:</span>%s</li>\n"
        "<li><span class=\"label\">reactable:</span>%s</li>\n"
        "<li><span class=\"label\">reverse:</span>%s</li>\n"
        "<li><span class=\"label\">refresh behavior:</span>%s</li>\n"
        "<li><span class=\"label\">stack behavior:</span>%s</li>\n"
        "<li><span class=\"label\">tick behavior:</span>%s</li>\n"
        "<li><span class=\"label\">tick_time behavior:</span>%s</li>\n"
        "<li><span class=\"label\">period:</span>%.2f</li>\n",
        b.source ? util::encode_html( b.source->name() ).c_str() : "",
        b.cooldown->name_str.c_str(), b.max_stack(),
        b.buff_duration.total_seconds(), b.cooldown->duration.total_seconds(),
        b.default_chance * 100, b.default_value,
        b.activated ? "true" : "false",
        b.reactable ? "true" : "false",
        b.reverse ? "true" : "false",
        util::buff_refresh_behavior_string(b.refresh_behavior),
        util::buff_stack_behavior_string(b.stack_behavior),
        util::buff_tick_behavior_string(b.tick_behavior),
        util::buff_tick_time_behavior_string(b.tick_time_behavior),
        b.buff_period.total_seconds());
    if ( b.item )
    {
      os.printf(
          "<li><span class=\"label\">associated item:</span>%s</li>\n",
          b.item->full_name().c_str());

    }
    os.printf(
        "</ul>\n");

    if ( b.rppm )
    {
      os << "<h4>RPPM Buff details</h4>\n"
         << "<ul>\n";

      os.printf(
          "<li><span class=\"label\">scaling:</span>%s</li>\n"
          "<li><span class=\"label\">frequency:</span>%.2f</li>\n"
          "<li><span class=\"label\">modifier:</span>%.2f</li>\n",
          util::rppm_scaling_string(b.rppm->get_scaling()).c_str(),
          b.rppm->get_frequency(),
          b.rppm->get_modifier());
      os << "</ul>\n";
    }

    if ( const stat_buff_t* stat_buff = dynamic_cast<const stat_buff_t*>( &b ) )
    {
      os << "<h4>Stat Buff details</h4>\n"
         << "<ul>\n";

      for ( size_t j = 0; j < stat_buff->stats.size(); ++j )
      {
        os.printf(
            "<li><span class=\"label\">stat:</span>%s</li>\n"
            "<li><span class=\"label\">amount:</span>%.2f</li>\n",
            util::stat_type_string( stat_buff->stats[ j ].stat ),
            stat_buff->stats[ j ].amount );
      }
      os << "</ul>\n";
    }

    os << "<h4>Stack Uptimes</h4>\n"
       << "<ul>\n";
    for ( unsigned int j = 0; j < b.stack_uptime.size(); j++ )
    {
      double uptime = b.stack_uptime[ j ].uptime_sum.mean();
      if ( uptime > 0 )
      {
        os.printf( "<li><span class=\"label\">%s_%d:</span>%.2f%%</li>\n",
                   b.name_str.c_str(), j, uptime * 100.0 );
      }
    }
    os << "</ul>\n";

    if ( b.trigger_pct.mean() > 0 )
    {
      os << "<h4>Trigger Attempt Success</h4>\n"
         << "<ul>\n";
      os.printf( "<li><span class=\"label\">trigger_pct:</span>%.2f%%</li>\n",
                 b.trigger_pct.mean() );
      os << "</ul>\n";
    }
    os << "</td>\n";

    print_html_player_buff_spelldata(os, b, b.data(), "Spelldata" );

    if ( b.get_trigger_data()->ok() && b.get_trigger_data()->id() != b.data().id() )
    {
      print_html_player_buff_spelldata(os, b, *b.get_trigger_data(), "Trigger Spell" );
    }

    os << "</tr>";

    if ( !b.constant && !b.overridden && b.sim->buff_uptime_timeline &&
         b.uptime_array.mean() > 0 )
    {
      highchart::time_series_t buff_uptime( highchart::build_id( b, "uptime" ),
                                            *b.sim );
      buff_uptime.set_yaxis_title( "Average uptime" );
      buff_uptime.set_title( b.name_str + " Uptime" );
      buff_uptime.add_simple_series( "area", "#FF0000", "Uptime",
                                     b.uptime_array.data() );
      buff_uptime.set_mean( b.uptime_array.mean() );
      if ( !b.player || !b.sim->single_actor_batch )
      {
        buff_uptime.set_xaxis_max( b.sim->simulation_length.max() );
      }
      else
      {
        buff_uptime.set_xaxis_max(
            b.player->collected_data.fight_length.max() );
      }

      os << "<tr><td colspan=\"2\" class=\"filler\">\n";
      os << buff_uptime.to_string();
      os << "</td></tr>\n";
    }
    os << "</table></td>\n";

    os << "</tr>\n";
  }
}

// print_html_player_buffs ==================================================

void print_html_player_buffs( report::sc_html_stream& os, const player_t& p,
                              const player_processed_report_information_t& ri )
{
  // Buff Section
  os << "<div class=\"player-section buffs\">\n"
     << "<h3 class=\"toggle open\">Buffs</h3>\n"
     << "<div class=\"toggle-content\">\n";

  // Dynamic Buffs table
  os << "<table class=\"sc mb\">\n"
     << "<tr>\n"
     << "<th class=\"left\"><a href=\"#help-dynamic-buffs\" "
        "class=\"help\">Dynamic Buffs</a></th>\n"
     << "<th>Start</th>\n"
     << "<th>Refresh</th>\n"
     << "<th>Interval</th>\n"
     << "<th>Trigger</th>\n"
     << "<th>Up-Time</th>\n"
     << "<th><a href=\"#help-buff-benefit\" class=\"help\">Benefit</a></th>\n"
     << "<th>Overflow</th>\n"
     << "<th>Expiry</th>\n"
     << "</tr>\n";

  for ( size_t i = 0; i < ri.dynamic_buffs.size(); i++ )
  {
    buff_t* b = ri.dynamic_buffs[ i ];

    print_html_player_buff( os, *b, p.sim->report_details, i );
  }
  os << "</table>\n";

  // constant buffs
  if ( !p.is_pet() )
  {
    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th class=\"left\"><a href=\"#help-constant-buffs\" "
          "class=\"help\">Constant Buffs</a></th>\n"
       << "</tr>\n";
    for ( size_t i = 0; i < ri.constant_buffs.size(); i++ )
    {
      buff_t* b = ri.constant_buffs[ i ];

      print_html_player_buff( os, *b, p.sim->report_details, i, true );
    }
    os << "</table>\n";
  }

  os << "</div>\n"
     << "</div>\n";
}

void print_html_player_custom_section(
    report::sc_html_stream& os, const player_t& p,
    const player_processed_report_information_t& /*ri*/ )
{
  p.report_extension->html_customsection( os );
}

// print_html_player ========================================================

void print_html_player_description( report::sc_html_stream& os,
                                    const player_t& p, int j )
{
  const sim_t& sim = *p.sim;
  int num_players  = (int)sim.players_by_name.size();

  // Player Description
  os << "<div id=\"player" << p.index << "\" class=\"player section";
  if ( num_players > 1 && j == 0 && !sim.scaling->has_scale_factors() &&
       !p.is_enemy() )
  {
    os << " grouped-first";
  }
  else if ( p.is_enemy() && j == (int)sim.targets_by_name.size() - 1 )
  {
    os << " final grouped-last";
  }
  else if ( num_players == 1 )
  {
    os << " section-open";
  }
  os << "\">\n";

  if ( !p.report_information.thumbnail_url.empty() )
  {
    os.printf(
        "<class=\"toggle-thumbnail ext%s\"><img src=\"%s\" "
        "alt=\"%s\" class=\"player-thumbnail\"/>\n",
        ( num_players == 1 ) ? "" : " hide",
        p.report_information.thumbnail_url.c_str(), p.name_str.c_str() );
  }

  os << "<h2 id=\""
     << "player" << p.index << "toggle\" class=\"toggle";
  if ( num_players == 1 )
  {
    os << " open";
  }

  const std::string n = util::encode_html( p.name() );
  if ( ( p.collected_data.dps.mean() >= p.collected_data.hps.mean() &&
         sim.num_enemies > 1 ) ||
       ( p.primary_role() == ROLE_TANK && sim.num_enemies > 1 ) )
  {
    os.printf( "\">%s&#160;:&#160;%.0f dps, %.0f dps to main target", n.c_str(),
               p.collected_data.dps.mean(),
               p.collected_data.prioritydps.mean() );
  }
  else if ( p.collected_data.dps.mean() >= p.collected_data.hps.mean() ||
            p.primary_role() == ROLE_TANK )
  {
    os.printf( "\">%s&#160;:&#160;%.0f dps", n.c_str(),
               p.collected_data.dps.mean() );
  }
  else
    os.printf( "\">%s&#160;:&#160;%.0f hps (%.0f aps)", n.c_str(),
               p.collected_data.hps.mean() + p.collected_data.aps.mean(),
               p.collected_data.aps.mean() );

  // if player tank, print extra metrics
  if ( p.primary_role() == ROLE_TANK && !p.is_enemy() )
  {
    // print DTPS & HPS
    os.printf( ", %.0f dtps", p.collected_data.dtps.mean() );
    os.printf( ", %.0f hps (%.0f aps)",
               p.collected_data.hps.mean() + p.collected_data.aps.mean(),
               p.collected_data.aps.mean() );
    // print TMI
    double tmi_display = p.collected_data.theck_meloree_index.mean();
    if ( tmi_display >= 1.0e7 )
      os.printf( ", %.2fM TMI", tmi_display / 1.0e6 );
    else if ( std::abs( tmi_display ) <= 999.9 )
      os.printf( ", %.3fk TMI", tmi_display / 1.0e3 );
    else
      os.printf( ", %.1fk TMI", tmi_display / 1.0e3 );
    // if we're using a non-standard window, append that to the label
    // appropriately (i.e. TMI-4.0 for a 4.0-second window)
    if ( p.tmi_window != 6.0 )
      os.printf( "-%1.1f", p.tmi_window );

    if ( sim.show_etmi || sim.player_no_pet_list.size() > 1 )
    {
      double etmi_display =
          p.collected_data.effective_theck_meloree_index.mean();
      if ( etmi_display >= 1.0e7 )
        os.printf( ", %.1fk ETMI", etmi_display / 1.0e6 );
      else if ( std::abs( etmi_display ) <= 999.9 )
        os.printf( ", %.3fk ETMI", etmi_display / 1.0e3 );
      else
        os.printf( ", %.1fk ETMI", etmi_display / 1.0e3 );
      // if we're using a non-standard window, append that to the label
      // appropriately (i.e. TMI-4.0 for a 4.0-second window)
      if ( p.tmi_window != 6.0 )
        os.printf( "-%1.1f", p.tmi_window );
    }

    os << "\n";
  }
  os << "</h2>\n";

  os << "<div class=\"toggle-content";
  if ( num_players > 1 )
  {
    os << " hide";
  }
  os << "\">\n";

  os << "<ul class=\"params\">\n";
  if ( p.dbc.ptr )
  {
#ifdef SC_USE_PTR
    os << "<li><b>PTR activated</b></li>\n";
#else
    os << "<li><b>BETA activated</b></li>\n";
#endif
  }
  const char* pt;
  if ( p.is_pet() )
    pt = util::pet_type_string( p.cast_pet()->pet_type );
  else
    pt = util::player_type_string( p.type );
  os.printf(
      "<li><b>Race:</b> %s</li>\n"
      "<li><b>Class:</b> %s</li>\n",
      util::inverse_tokenize( p.race_str ).c_str(),
      util::inverse_tokenize( pt ).c_str() );

  if ( p.specialization() != SPEC_NONE )
    os.printf( "<li><b>Spec:</b> %s</li>\n",
               util::inverse_tokenize(
                   dbc::specialization_string( p.specialization() ) )
                   .c_str() );

  std::string timewalk_str = " (";
  if ( sim.timewalk > 0 )
  {
    timewalk_str += util::to_string( p.true_level );
    timewalk_str += ")";
  }
  os.printf(
      "<li><b>Level:</b> %d%s</li>\n"
      "<li><b>Role:</b> %s</li>\n"
      "<li><b>Position:</b> %s</li>\n"
      "<li><b>Profile Source:</b> %s</li>\n"
      "</ul>\n"
      "<div class=\"clear\"></div>\n",
      p.level(), sim.timewalk > 0 && !p.is_enemy() ? timewalk_str.c_str() : "",
      util::inverse_tokenize( util::role_type_string( p.primary_role() ) )
          .c_str(),
      p.position_str.c_str(),
      util::profile_source_string( p.profile_source_ ) );
}

// print_html_player_results_spec_gear ======================================

void print_html_player_results_spec_gear( report::sc_html_stream& os,
                                          const player_t& p )
{
  const sim_t& sim                  = *( p.sim );
  const player_collected_data_t& cd = p.collected_data;

  // Main player table
  os << "<div class=\"player-section results-spec-gear mt\">\n";
  if ( p.is_pet() )
  {
    os << "<h3 class=\"toggle open\">Results</h3>\n";
  }
  else
  {
    os << "<h3 class=\"toggle open\">Results, Spec and Gear</h3>\n";
  }
  os << "<div class=\"toggle-content\">\n";

  if ( cd.dps.mean() > 0 || cd.hps.mean() > 0 || cd.aps.mean() > 0 )
  {
    // Table for DPS, HPS, APS
    os << "<table class=\"sc\">\n"
       << "<tr>\n";
    // First, make the header row
    // Damage
    if ( cd.dps.mean() > 0 )
    {
      os << "<th><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
         << "<th><a href=\"#help-dpse\" class=\"help\">DPS(e)</a></th>\n"
         << "<th><a href=\"#help-error\" class=\"help\">DPS Error</a></th>\n"
         << "<th><a href=\"#help-range\" class=\"help\">DPS Range</a></th>\n"
         << "<th><a href=\"#help-dpr\" class=\"help\">DPR</a></th>\n";
    }
    // spacer
    if ( cd.hps.mean() > 0 && cd.dps.mean() > 0 )
      os << "<th>&#160;</th>\n";
    // Heal
    if ( cd.hps.mean() > 0 )
    {
      os << "<th><a href=\"#help-hps\" class=\"help\">HPS</a></th>\n"
         << "<th><a href=\"#help-hpse\" class=\"help\">HPS(e)</a></th>\n"
         << "<th><a href=\"#help-error\" class=\"help\">HPS Error</a></th>\n"
         << "<th><a href=\"#help-range\" class=\"help\">HPS Range</a></th>\n"
         << "<th><a href=\"#help-hpr\" class=\"help\">HPR</a></th>\n";
    }
    // spacer
    if ( cd.hps.mean() > 0 && cd.aps.mean() > 0 )
      os << "<th>&#160;</th>\n";
    // Absorb
    if ( cd.aps.mean() > 0 )
    {
      os << "<th><a href=\"#help-aps\" class=\"help\">APS</a></th>\n"
         << "<th><a href=\"#help-error\" class=\"help\">APS Error</a></th>\n"
         << "<th><a href=\"#help-range\" class=\"help\">APS Range</a></th>\n"
         << "<th><a href=\"#help-hpr\" class=\"help\">APR</a></th>\n";
    }
    // end row, begin new row
    os << "</tr>\n"
       << "<tr>\n";

    // Now do the data row
    if ( cd.dps.mean() > 0 )
    {
      double range =
          ( p.collected_data.dps.percentile( 0.5 + sim.confidence / 2 ) -
            p.collected_data.dps.percentile( 0.5 - sim.confidence / 2 ) );
      double dps_error =
          sim_t::distribution_mean_error( sim, p.collected_data.dps );
      os.printf(
          "<td>%.1f</td>\n"
          "<td>%.1f</td>\n"
          "<td>%.1f / %.3f%%</td>\n"
          "<td>%.1f / %.1f%%</td>\n"
          "<td>%.1f</td>\n",
          cd.dps.mean(), cd.dpse.mean(), dps_error,
          cd.dps.mean() ? dps_error * 100 / cd.dps.mean() : 0, range,
          cd.dps.mean() ? range / cd.dps.mean() * 100.0 : 0, p.dpr );
    }
    // Spacer
    if ( cd.dps.mean() > 0 && cd.hps.mean() > 0 )
      os << "<td>&#160;&#160;&#160;&#160;&#160;</td>\n";
    // Heal
    if ( cd.hps.mean() > 0 )
    {
      double range = ( cd.hps.percentile( 0.5 + sim.confidence / 2 ) -
                       cd.hps.percentile( 0.5 - sim.confidence / 2 ) );
      double hps_error =
          sim_t::distribution_mean_error( sim, p.collected_data.hps );
      os.printf(
          "<td>%.1f</td>\n"
          "<td>%.1f</td>\n"
          "<td>%.2f / %.2f%%</td>\n"
          "<td>%.0f / %.1f%%</td>\n"
          "<td>%.1f</td>\n",
          cd.hps.mean(), cd.hpse.mean(), hps_error,
          cd.hps.mean() ? hps_error * 100 / cd.hps.mean() : 0, range,
          cd.hps.mean() ? range / cd.hps.mean() * 100.0 : 0, p.hpr );
    }
    // Spacer
    if ( cd.aps.mean() > 0 && cd.hps.mean() > 0 )
      os << "<td>&#160;&#160;&#160;&#160;&#160;</td>\n";
    // Absorb
    if ( cd.aps.mean() > 0 )
    {
      double range = ( cd.aps.percentile( 0.5 + sim.confidence / 2 ) -
                       cd.aps.percentile( 0.5 - sim.confidence / 2 ) );
      double aps_error =
          sim_t::distribution_mean_error( sim, p.collected_data.aps );
      os.printf(
          "<td>%.1f</td>\n"
          "<td>%.2f / %.2f%%</td>\n"
          "<td>%.0f / %.1f%%</td>\n"
          "<td>%.1f</td>\n",
          cd.aps.mean(), aps_error,
          cd.aps.mean() ? aps_error * 100 / cd.aps.mean() : 0, range,
          cd.aps.mean() ? range / cd.aps.mean() * 100.0 : 0, p.hpr );
    }
    // close table
    os << "</tr>\n"
       << "</table>\n";
  }

  // Tank table
  if ( p.primary_role() == ROLE_TANK && !p.is_enemy() )
  {
    os << "<table class=\"sc mt\">\n"
       // experimental first row for stacking the tables - wasn't happy with how
       // it looked, may return to it later
       //<< "<tr>\n" // first row
       //<< "<th colspan=\"3\"><a href=\"#help-dtps\"
       // class=\"help\">DTPS</a></th>\n"
       //<< "<td>&nbsp&nbsp&nbsp&nbsp&nbsp</td>\n"
       //<< "<th colspan=\"5\"><a href=\"#help-tmi\"
       // class=\"help\">TMI</a></th>\n"
       //<< "<td>&nbsp&nbsp&nbsp&nbsp&nbsp</td>\n"
       //<< "<th colspan=\"4\"><a href=\"#help-msd\"
       // class=\"help\">MSD</a></th>\n"
       //<< "</tr>\n" // end first row
       << "<tr>\n"  // start second row
       << "<th><a href=\"#help-dtps\" class=\"help\">DTPS</a></th>\n"
       << "<th><a href=\"#help-error\" class=\"help\">DTPS Error</a></th>\n"
       << "<th><a href=\"#help-range\" class=\"help\">DTPS Range</a></th>\n"
       << "<th>&#160;</th>\n"
       << "<th><a href=\"#help-theck-meloree-index\" "
          "class=\"help\">TMI</a></th>\n"
       << "<th><a href=\"#help-error\" class=\"help\">TMI Error</a></th>\n"
       << "<th><a href=\"#help-tmi\" class=\"help\">TMI Min</a></th>\n"
       << "<th><a href=\"#help-tmi\" class=\"help\">TMI Max</a></th>\n"
       << "<th><a href=\"#help-tmirange\" class=\"help\">TMI Range</a></th>\n"
       << "<th>&#160;</th>\n"
       << "<th><a href=\"#help-msd\" class=\"help\">MSD Mean</a></th>\n"
       << "<th><a href=\"#help-msd\" class=\"help\">MSD Min</a></th>\n"
       << "<th><a href=\"#help-msd\" class=\"help\">MSD Max</a></th>\n"
       << "<th>&#160;</th>\n"
       << "<th><a href=\"#help-tmiwin\" class=\"help\">Window</a></th>\n"
       << "<th><a href=\"#help-tmi-bin-size\" class=\"help\">Bin "
          "Size</a></th>\n"
       << "</tr>\n"  // end second row
       << "<tr>\n";  // start third row

    double dtps_range = ( cd.dtps.percentile( 0.5 + sim.confidence / 2 ) -
                          cd.dtps.percentile( 0.5 - sim.confidence / 2 ) );
    double dtps_error =
        sim_t::distribution_mean_error( sim, p.collected_data.dtps );
    os.printf(
        "<td>%.1f</td>\n"
        "<td>%.2f / %.2f%%</td>\n"
        "<td>%.0f / %.1f%%</td>\n",
        cd.dtps.mean(), dtps_error,
        cd.dtps.mean() ? dtps_error * 100 / cd.dtps.mean() : 0, dtps_range,
        cd.dtps.mean() ? dtps_range / cd.dtps.mean() * 100.0 : 0 );

    // spacer
    os << "<td>&#160;&#160;&#160;&#160;&#160;</td>\n";

    double tmi_error = sim_t::distribution_mean_error(
        sim, p.collected_data.theck_meloree_index );
    double tmi_range =
        ( cd.theck_meloree_index.percentile( 0.5 + sim.confidence / 2 ) -
          cd.theck_meloree_index.percentile( 0.5 - sim.confidence / 2 ) );

    // print TMI
    if ( std::abs( cd.theck_meloree_index.mean() ) > 1.0e8 )
      os.printf( "<td>%1.3e</td>\n", cd.theck_meloree_index.mean() );
    else
      os.printf( "<td>%.1fk</td>\n", cd.theck_meloree_index.mean() / 1e3 );

    // print TMI error/variance
    if ( tmi_error > 1.0e6 )
    {
      os.printf( "<td>%1.2e / %.2f%%</td>\n", tmi_error,
                 cd.theck_meloree_index.mean()
                     ? tmi_error * 100.0 / cd.theck_meloree_index.mean()
                     : 0.0 );
    }
    else
    {
      os.printf( "<td>%.0f / %.2f%%</td>\n", tmi_error,
                 cd.theck_meloree_index.mean()
                     ? tmi_error * 100.0 / cd.theck_meloree_index.mean()
                     : 0.0 );
    }

    // print  TMI min/max
    if ( std::abs( cd.theck_meloree_index.min() ) > 1.0e8 )
      os.printf( "<td>%1.2e</td>\n", cd.theck_meloree_index.min() );
    else
      os.printf( "<td>%.1fk</td>\n", cd.theck_meloree_index.min() / 1e3 );

    if ( std::abs( cd.theck_meloree_index.max() ) > 1.0e8 )
      os.printf( "<td>%1.2e</td>\n", cd.theck_meloree_index.max() );
    else
      os.printf( "<td>%.1fk</td>\n", cd.theck_meloree_index.max() / 1e3 );

    // print TMI range
    if ( tmi_range > 1.0e8 )
    {
      os.printf( "<td>%1.2e / %.1f%%</td>\n", tmi_range,
                 cd.theck_meloree_index.mean()
                     ? tmi_range * 100.0 / cd.theck_meloree_index.mean()
                     : 0.0 );
    }
    else
    {
      os.printf( "<td>%.1fk / %.1f%%</td>\n", tmi_range / 1e3,
                 cd.theck_meloree_index.mean()
                     ? tmi_range * 100.0 / cd.theck_meloree_index.mean()
                     : 0.0 );
    }

    // spacer
    os << "<td>&#160;&#160;&#160;&#160;&#160;</td>\n";

    // print Max Spike Size stats
    os.printf( "<td>%.1f%%</td>\n", cd.max_spike_amount.mean() );
    os.printf( "<td>%.1f%%</td>\n", cd.max_spike_amount.min() );
    os.printf( "<td>%.1f%%</td>\n", cd.max_spike_amount.max() );

    // spacer
    os << "<td>&#160;&#160;&#160;&#160;&#160;</td>\n";

    // print TMI window and bin size
    os.printf( "<td>%.2fs</td>\n", p.tmi_window );
    os.printf( "<td>%.2fs</td>\n", sim.tmi_bin_size );

    // End defensive table
    os << "</tr>\n"
       << "</table>\n";
  }

  // Resources/Activity Table
  if ( !p.is_pet() )
  {
    os << "<table class=\"sc ra\">\n"
       << "<tr>\n"
       << "<th><a href=\"#help-rps-out\" class=\"help\">RPS Out</a></th>\n"
       << "<th><a href=\"#help-rps-in\" class=\"help\">RPS In</a></th>\n"
       << "<th>Primary Resource</th>\n"
       << "<th><a href=\"#help-waiting\" class=\"help\">Waiting</a></th>\n"
       << "<th><a href=\"#help-apm\" class=\"help\">APM</a></th>\n"
       << "<th>Active</th>\n"
       << "<th>Skill</th>\n"
       << "</tr>\n"
       << "<tr>\n";

    os.printf(
        "<td>%.1f</td>\n"
        "<td>%.1f</td>\n"
        "<td>%s</td>\n"
        "<td>%.2f%%</td>\n"
        "<td>%.1f</td>\n"
        "<td>%.1f%%</td>\n"
        "<td>%.0f%%</td>\n"
        "</tr>\n"
        "</table>\n",
        p.rps_loss, p.rps_gain,
        util::inverse_tokenize(
            util::resource_type_string( p.primary_resource() ) )
            .c_str(),
        cd.fight_length.mean()
            ? 100.0 * cd.waiting_time.mean() / cd.fight_length.mean()
            : 0,
        cd.fight_length.mean()
            ? 60.0 * cd.executed_foreground_actions.mean() /
                  cd.fight_length.mean()
            : 0,
        sim.simulation_length.mean()
            ? cd.fight_length.mean() / sim.simulation_length.mean() * 100.0
            : 0,
        p.initial.skill * 100.0 );
  }

  // Spec and gear
  if ( !p.is_pet() )
  {
    os << "<table class=\"sc mt\">\n";
    if ( !p.origin_str.empty() )
    {
      std::string origin_url = util::encode_html( p.origin_str );
      os.printf(
          "<tr class=\"left\">\n"
          "<th><a href=\"#help-origin\" class=\"help\">Origin</a></th>\n"
          "<td><a href=\"%s\" class=\"ext\">%s</a></td>\n"
          "</tr>\n",
          p.origin_str.c_str(), origin_url.c_str() );
    }

    if ( !p.talents_str.empty() )
    {
      os.printf(
          "<tr class=\"left\">\n"
          "<th>Talents</th>\n"
          "<td><ul class=\"float\">\n" );
      for ( uint32_t row = 0; row < MAX_TALENT_ROWS; row++ )
      {
        for ( uint32_t col = 0; col < MAX_TALENT_COLS; col++ )
        {
          talent_data_t* t = talent_data_t::find(
              p.type, row, col, p.specialization(), p.dbc.ptr );
          std::string name = "none";
          if ( t && t->name_cstr() )
          {
            name = t->name_cstr();
            if ( t->specialization() != SPEC_NONE )
            {
              name += " (";
              name += util::specialization_string( t->specialization() );
              name += ")";
            }
          }
          if ( p.talent_points.has_row_col( row, col ) )
            os.printf( "<li><strong>%d</strong>:&#160;%s</li>\n",
                       row == 6 ? 100 : ( row + 1 ) * 15, name.c_str() );
        }
      }
      std::string url_string = p.talents_str;
      if ( !util::str_in_str_ci( url_string, "http" ) )
        url_string = util::create_blizzard_talent_url( p );

      std::string enc_url = util::encode_html( url_string );
      os.printf(
          "<li><a href=\"%s\" class=\"ext\">Talent Calculator</a></li>\n",
          enc_url.c_str() );

      os << "</ul></td>\n";
      os << "</tr>\n";
    }

    // Artifact
    if ( p.artifact )
    {
      p.artifact -> generate_report( os );
    }

    // Professions
    if ( !p.professions_str.empty() )
    {
      os.printf(
          "<tr class=\"left\">\n"
          "<th>Professions</th>\n"
          "<td>\n"
          "<ul class=\"float\">\n" );
      for ( profession_e i = PROFESSION_NONE; i < PROFESSION_MAX; ++i )
      {
        if ( p.profession[ i ] <= 0 )
          continue;

        os << "<li>" << util::profession_type_string( i ) << ": "
           << p.profession[ i ] << "</li>\n";
      }
      os << "</ul>\n"
         << "</td>\n"
         << "</tr>\n";
    }

    os << "</table>\n";
  }
}

// print_html_player_abilities ==============================================

bool is_output_stat( unsigned mask, bool child, const stats_t& s )
{
  if ( s.quiet )
    return false;

  if ( !( ( 1 << s.type ) & mask ) )
    return false;

  if ( s.num_executes.mean() <= 0.001 && s.compound_amount == 0 &&
       !s.player->sim->debug )
    return false;

  // If we are checking output on a non-child stats object, only return false
  // when parent is found, if the actor of the parent stats object, and this
  // object are the same.  The parent and child object types also must be
  // equal.
  if ( !child && s.parent && s.parent->player == s.player &&
       s.parent->type == s.type )
    return false;

  return true;
}

void output_player_action( report::sc_html_stream& os, unsigned& row,
                           unsigned cols, unsigned mask, const stats_t& s,
                           const player_t* actor, int level = 0 )
{
  if (level > 2 )
    return;

  if ( !is_output_stat( mask, level != 0, s ) )
    return;

  print_html_action_info( os, mask, s, row++, cols, actor, level );

  for ( auto child_stats : s.children )
  {
    output_player_action( os, row, cols, mask, *child_stats, actor, level + 1 );
  }
}

void output_player_damage_summary( report::sc_html_stream& os,
                                   const player_t& actor )
{
  if ( actor.collected_data.dmg.max() == 0 && !actor.sim->debug )
    return;

  // Number of static columns in table
  const int static_columns = 5;
  // Number of dynamically changing columns
  int n_optional_columns = 6;

  // Abilities Section - Damage
  os << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th class=\"left small\">Damage Stats</th>\n"
     << "<th class=\"small\"><a href=\"#help-dps\" "
        "class=\"help\">DPS</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dps-pct\" "
        "class=\"help\">DPS%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-Count\" "
        "class=\"help\">Execute</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-interval\" "
        "class=\"help\">Interval</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dpe\" "
        "class=\"help\">DPE</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dpet\" "
        "class=\"help\">DPET</a></th>\n"
     // Optional columns begin here
     << "<th class=\"small\"><a href=\"#help-type\" "
        "class=\"help\">Type</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-count\" "
        "class=\"help\">Count</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-hit\" "
        "class=\"help\">Hit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit\" "
        "class=\"help\">Crit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-avg\" "
        "class=\"help\">Avg</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit-pct\" "
        "class=\"help\">Crit%</a></th>\n";

  if ( player_has_avoidance( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-miss-pct\" "
          "class=\"help\">Avoid%</a></th>\n";
    n_optional_columns++;
  }

  if ( player_has_glance( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-glance-pct\" "
          "class=\"help\">G%</a></th>\n";
    n_optional_columns++;
  }

  if ( player_has_block( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-block-pct\" "
          "class=\"help\">B%</a></th>\n";
    n_optional_columns++;
  }

  if ( player_has_tick_results( actor, MASK_DMG ) )
  {
    os << "<th class=\"small\"><a href=\"#help-ticks-uptime-pct\" "
          "class=\"help\">Up%</a></th>\n"
       << "</tr>\n";
    n_optional_columns++;
  }

  os << "<tr>\n"
     << "<th class=\"left small\">" << util::encode_html( actor.name() )
     << "</th>\n"
     << "<th class=\"right small\">"
     << util::to_string( actor.collected_data.dps.mean(), 0 ) << "</th>\n"
     << "<td colspan=\"" << ( static_columns + n_optional_columns )
     << "\" class=\"filler\"></td>\n"
     << "</tr>\n";

  unsigned n_row = 0;
  for ( const auto& stat : actor.stats_list )
  {
    if ( stat->compound_amount > 0 )
      output_player_action( os, n_row, n_optional_columns, MASK_DMG, *stat,
                            &actor );
  }

  // Print pet statistics
  for ( const auto& pet : actor.pet_list )
  {
    bool first = true;
    for ( const auto& stat : pet->stats_list )
    {
      if ( !is_output_stat( MASK_DMG, false, *stat ) )
        continue;

      if ( stat->parent )
        continue;

      if ( stat->compound_amount == 0 )
        continue;

      if ( first )
      {
        first = false;
        os.printf(
            "<tr>\n"
            "<th class=\"left small\">pet - %s</th>\n"
            "<th class=\"right small\">%.0f / %.0f</th>\n"
            "<td colspan=\"%d\" class=\"filler\"></td>\n"
            "</tr>\n",
            pet->name_str.c_str(), pet->collected_data.dps.mean(),
            pet->collected_data.dpse.mean(),
            static_columns + n_optional_columns );
      }

      output_player_action( os, n_row, n_optional_columns, MASK_DMG, *stat,
                            pet );
    }
  }

  os << "</table>\n";
}

void output_player_heal_summary( report::sc_html_stream& os,
                                 const player_t& actor )
{
  if ( actor.collected_data.heal.max() == 0 && !actor.sim->debug &&
       actor.collected_data.absorb.max() == 0 )
    return;

  // Number of static columns in table
  const int static_columns = 5;
  // Number of dynamically changing columns
  int n_optional_columns = 6;

  // Abilities Section - Heal
  os << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th class=\"left small\">Healing and Absorb Stats</th>\n"
     << "<th class=\"small\"><a href=\"#help-dps\" "
        "class=\"help\">HPS</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dps-pct\" "
        "class=\"help\">HPS%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-count\" "
        "class=\"help\">Execute</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-interval\" "
        "class=\"help\">Interval</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dpe\" "
        "class=\"help\">HPE</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dpet\" "
        "class=\"help\">HPET</a></th>\n"
     // Optional columns begin here
     << "<th class=\"small\"><a href=\"#help-type\" "
        "class=\"help\">Type</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-count\" "
        "class=\"help\">Count</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-hit\" "
        "class=\"help\">Hit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit\" "
        "class=\"help\">Crit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-avg\" "
        "class=\"help\">Avg</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit-pct\" "
        "class=\"help\">Crit%</a></th>\n";

  if ( player_has_tick_results( actor, MASK_HEAL | MASK_ABSORB ) )
  {
    os << "<th class=\"small\"><a href=\"#help-ticks-uptime-pct\" "
          "class=\"help\">Up%</a></th>\n"
       << "</tr>\n";
    n_optional_columns++;
  }

  os << "<tr>\n"
     << "<th class=\"left small\">" << util::encode_html( actor.name() )
     << "</th>\n"
     << "<th class=\"right small\">"
     << util::to_string( actor.collected_data.hps.mean(), 0 ) << "</th>\n"
     << "<td colspan=\"" << ( static_columns + n_optional_columns )
     << "\" class=\"filler\"></td>\n"
     << "</tr>\n";

  unsigned n_row = 0;
  for ( const auto& stat : actor.stats_list )
  {
    if ( stat->compound_amount > 0 )
      output_player_action( os, n_row, n_optional_columns,
                            MASK_HEAL | MASK_ABSORB, *stat, &actor );
  }

  // Print pet statistics
  for ( const auto& pet : actor.pet_list )
  {
    bool first = true;
    for ( const auto& stat : pet->stats_list )
    {
      if ( !is_output_stat( MASK_HEAL | MASK_ABSORB, false, *stat ) )
        continue;

      if ( stat->parent && stat->parent->player == pet )
        continue;

      if ( stat->compound_amount == 0 )
        continue;

      if ( first )
      {
        first = false;
        os.printf(
            "<tr>\n"
            "<th class=\"left small\">pet - %s</th>\n"
            "<th class=\"right small\">%.0f / %.0f</th>\n"
            "<td colspan=\"%d\" class=\"filler\"></td>\n"
            "</tr>\n",
            pet->name_str.c_str(), pet->collected_data.dps.mean(),
            pet->collected_data.dpse.mean(),
            static_columns + n_optional_columns );
      }

      output_player_action( os, n_row, n_optional_columns,
                            MASK_HEAL | MASK_ABSORB, *stat, pet );
    }
  }

  os << "</table>\n";
}

void output_player_simple_ability_summary( report::sc_html_stream& os,
                                           const player_t& actor )
{
  if ( actor.collected_data.dmg.max() == 0 && !actor.sim->debug )
    return;

  // Abilities Section - Simple Actions
  os << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th class=\"left small\">Simple Action Stats</th>\n"
     << "<th class=\"small\"><a href=\"#help-count\" "
        "class=\"help\">Execute</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-interval\" "
        "class=\"help\">Interval</a></th>\n"
     << "</tr>\n";

  os << "<tr>\n"
     << "<th class=\"left small\">" << util::encode_html( actor.name() )
     << "</th>\n"
     << "<th colspan=\"2\" class=\"filler\"></th>\n"
     << "</tr>\n";

  unsigned n_row = 0;
  for ( const auto& stat : actor.stats_list )
  {
    if ( stat->compound_amount == 0 )
      output_player_action( os, n_row, 0, MASK_DMG | MASK_HEAL | MASK_ABSORB,
                            *stat, &actor );
  }

  // Print pet statistics
  for ( const auto& pet : actor.pet_list )
  {
    bool first = true;
    for ( const auto& stat : pet->stats_list )
    {
      if ( !is_output_stat( MASK_DMG | MASK_HEAL | MASK_ABSORB, false, *stat ) )
        continue;

      if ( stat->parent && stat->parent->player == pet )
        continue;

      if ( stat->compound_amount > 0 )
        continue;

      if ( first )
      {
        first = false;
        os.printf(
            "<tr>\n"
            "<th class=\"left small\">pet - %s</th>\n"
            "<th colspan=\"%d\" class=\"filler\"></th>\n"
            "</tr>\n",
            pet->name_str.c_str(), 2 );
      }

      output_player_action( os, n_row, 0, MASK_DMG | MASK_HEAL | MASK_ABSORB,
                            *stat, pet );
    }
  }

  os << "</table>\n";
}

void print_html_player_abilities( report::sc_html_stream& os,
                                  const player_t& p )
{
  // open section
  os << "<div class=\"player-section\">\n"
     << "<h3 class=\"toggle open\">Abilities</h3>\n"
     << "<div class=\"toggle-content\">\n";

  output_player_damage_summary( os, p );
  output_player_heal_summary( os, p );
  output_player_simple_ability_summary( os, p );

  // close section
  os << "</div>\n"
     << "</div>\n";
}

// print_html_player_benefits_uptimes =======================================

void print_html_player_benefits_uptimes( report::sc_html_stream& os,
                                         const player_t& p )
{
  os << "<div class=\"player-section benefits\">\n"
     << "<h3 class=\"toggle\">Benefits & Uptimes</h3>\n"
     << "<div class=\"toggle-content hide\">\n"
     << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th>Benefits</th>\n"
     << "<th>%</th>\n"
     << "</tr>\n";
  int i = 1;

  for ( const auto& benefit : p.benefit_list )
  {
    if ( benefit->ratio.mean() > 0 )
    {
      os << "<tr";
      if ( !( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";
      os.printf(
          "<td class=\"left\">%s</td>\n"
          "<td class=\"right\">%.1f%%</td>\n"
          "</tr>\n",
          benefit->name(), benefit->ratio.mean() );
      i++;
    }
  }

  for ( const auto& pet : p.pet_list )
  {
    for ( const auto& benefit : pet->benefit_list )
    {
      if ( benefit->ratio.mean() > 0 )
      {
        std::string benefit_name;
        benefit_name += pet->name_str + '-';
        benefit_name += benefit->name();

        os << "<tr";
        if ( !( i & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.printf(
            "<td class=\"left\">%s</td>\n"
            "<td class=\"right\">%.1f%%</td>\n"
            "</tr>\n",
            benefit_name.c_str(), benefit->ratio.mean() );
        i++;
      }
    }
  }

  os << "<tr>\n"
     << "<th>Uptimes</th>\n"
     << "<th>%</th>\n"
     << "</tr>\n";

  for ( const auto& uptime : p.uptime_list )
  {
    if ( uptime->uptime_sum.mean() > 0 )
    {
      os << "<tr";
      if ( !( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";
      os.printf(
          "<td class=\"left\">%s</td>\n"
          "<td class=\"right\">%.1f%%</td>\n"
          "</tr>\n",
          uptime->name(), uptime->uptime_sum.mean() * 100.0 );
      i++;
    }
  }

  for ( const auto& pet : p.pet_list )
  {
    for ( const auto& uptime : pet->uptime_list )
    {
      if ( uptime->uptime_sum.mean() > 0 )
      {
        std::string uptime_name;
        uptime_name += pet->name_str + '-';
        uptime_name += uptime->name();

        os << "<tr";
        if ( !( i & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.printf(
            "<td class=\"left\">%s</td>\n"
            "<td class=\"right\">%.1f%%</td>\n"
            "</tr>\n",
            uptime_name.c_str(), uptime->uptime_sum.mean() * 100.0 );

        i++;
      }
    }
  }

  os << "</table>\n"
     << "</div>\n"
     << "</div>\n";
}

// print_html_player_procs ==================================================

void print_html_player_procs( report::sc_html_stream& os,
                              const std::vector<proc_t*>& pr )
{
  // Procs Section
  os << "<div class=\"player-section procs\">\n"
     << "<h3 class=\"toggle open\">Procs</h3>\n"
     << "<div class=\"toggle-content\">\n"
     << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th></th>\n"
     << "<th>Count</th>\n"
     << "<th>Interval</th>\n"
     << "</tr>\n";
  {
    int i = 1;
    for ( const auto& proc : pr )
    {
      if ( proc->count.mean() > 0 )
      {
        os << "<tr";
        if ( !( i & 1 ) )
        {
          os << " class=\"odd\"";
        }
        os << ">\n";
        os.printf(
            "<td class=\"left\">%s</td>\n"
            "<td class=\"right\">%.1f</td>\n"
            "<td class=\"right\">%.1fsec</td>\n"
            "</tr>\n",
            proc->name(), proc->count.mean(), proc->interval_sum.mean() );
        i++;
      }
    }
  }
  os << "</table>\n"
     << "</div>\n"
     << "</div>\n";
}

// print_html_player_deaths =================================================

void print_html_player_deaths( report::sc_html_stream& os, const player_t& p,
                               const player_processed_report_information_t& )
{
  // Death Analysis

  const extended_sample_data_t& deaths = p.collected_data.deaths;

  if ( deaths.size() > 0 )
  {
    os << "<div class=\"player-section gains\">\n"
       << "<h3 class=\"toggle\">Deaths</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th></th>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">death count</td>\n"
       << "<td class=\"right\">" << deaths.size() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">death count pct</td>\n"
       << "<td class=\"right\">"
       << (double)deaths.size() / p.sim->iterations * 100 << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">avg death time</td>\n"
       << "<td class=\"right\">" << deaths.mean() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">min death time</td>\n"
       << "<td class=\"right\">" << deaths.min() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">max death time</td>\n"
       << "<td class=\"right\">" << deaths.max() << "</td>\n"
       << "</tr>\n";

    os << "<tr>\n"
       << "<td class=\"left\">dmg taken</td>\n"
       << "<td class=\"right\">" << p.collected_data.dmg_taken.mean()
       << "</td>\n"
       << "</tr>\n";

    os << "</table>\n";

    os << "<div class=\"clear\"></div>\n";

    highchart::histogram_chart_t chart( highchart::build_id( p, "death_dist" ),
                                        *p.sim );
    if ( chart::generate_distribution(
             chart, &p, p.collected_data.deaths.distribution,
             p.name_str + " Death", p.collected_data.deaths.mean(),
             p.collected_data.deaths.min(), p.collected_data.deaths.max() ) )
    {
      os << chart.to_target_div();
      p.sim->add_chart_data( chart );
    }

    os << "</div>\n"
       << "</div>\n";
  }
}

// print_html_player_ =======================================================

void print_html_player_( report::sc_html_stream& os, const player_t& p,
                         int player_index )
{
  print_html_player_description( os, p, player_index );

  print_html_player_results_spec_gear( os, p );

  print_html_player_scale_factors( os, p, p.report_information );

  print_html_player_charts( os, p, p.report_information );

  print_html_player_abilities( os, p );

  print_html_player_buffs( os, p, p.report_information );

  print_html_player_procs( os, p.proc_list );

  print_html_player_custom_section( os, p, p.report_information );

  print_html_player_resources( os, p, p.report_information );

  print_html_player_benefits_uptimes( os, p );

  print_html_player_deaths( os, p, p.report_information );

  print_html_player_statistics( os, p, p.report_information );

  print_html_player_action_priority_list( os, p );

  print_html_stats( os, p );

  print_html_gear( os, p );

  print_html_talents( os, p );

  print_html_profile( os, p, p.report_information );

  // print_html_player_gear_weights( os, p, p.report_information );

  os << "</div>\n"
     << "</div>\n\n";
}

void build_action_markers( player_t& p )
{
  int j = 0;

  for ( auto& action : p.action_list )
  {
    // Map markers to only used actions
    if ( !p.sim->separate_stats_by_actions && action->action_list )
    {
      if ( action->total_executions > 0 ||
           util::str_compare_ci( action->action_list->name_str, "precombat" ) ||
           util::str_in_str_ci( action->name_str, "_action_list" ) )
      {
        action->marker =
            (char)( ( j < 10 )
                        ? ( '0' + j )
                        : ( j < 36 )
                              ? ( 'A' + j - 10 )
                              : ( j < 66 )
                                    ? ( 'a' + j - 36 )
                                    : ( j < 79 ) ? ( '!' + j - 66 )
                                                 : ( j < 86 ) ? ( ':' + j - 79 )
                                                              : '.' );
        j++;
      }
    }
  }
}

void build_player_report_data( player_t& p )
{
  report::generate_player_charts( p, p.report_information );
  report::generate_player_buff_lists( p, p.report_information );
  build_action_markers( p );
}

}  // UNNAMED NAMESPACE ====================================================

namespace report
{
void print_html_player( report::sc_html_stream& os, player_t& p,
                        int player_index )
{
  build_player_report_data( p );
  print_html_player_( os, p, player_index );
}

}  // END report NAMESPACE

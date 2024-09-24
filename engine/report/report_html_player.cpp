// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc/item_set_bonus.hpp"
#include "dbc/sc_spell_info.hpp"
#include "dbc/temporary_enchant.hpp"
#include "dbc/trait_data.hpp"
#include "player/consumable.hpp"
#include "player/covenant.hpp"
#include "player/player_talent_points.hpp"
#include "player/scaling_metric_data.hpp"
#include "player/set_bonus.hpp"
#include "player/unique_gear_shadowlands.hpp"
#include "report/charts.hpp"
#include "report/decorators.hpp"
#include "report/highchart.hpp"
#include "report/report_helper.hpp"
#include "reports.hpp"
#include "sim/profileset.hpp"
#include "sim/scale_factor_control.hpp"
#include "util/util.hpp"

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
    if ( !( stats_mask & stat->mask() ) )
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
    if ( !( stats_mask & stat->mask() ) )
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
    if ( !( stats_mask & stat->mask() ) )
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
    if ( !( stats_mask & stat->mask() ) )
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

// Use smaller font & padding for Ability tables when dps/hps is over 1 million

bool use_small_table( const player_t* p )
{
  static constexpr double cutoff = 1000000;

  return p->collected_data.dps.max() >= cutoff || p->collected_data.hps.max() >= cutoff;
}

enum sort_flag_e : unsigned
{
  SORT_FLAG_NONE  = 0x00,
  SORT_FLAG_ASC   = 0x01,
  SORT_FLAG_ALPHA = 0x02,
  SORT_FLAG_LEFT  = 0x04,
  SORT_FLAG_BOTH  = 0x08,
};

void sorttable_header( report::sc_html_stream& os,                    // output stream to html report
                       std::string_view header,                     // column header name
                       unsigned flag = SORT_FLAG_NONE,                // bitflags to set header class
                       std::string_view helptext = {} )  // optional hover help id
{
  std::string class_str = "toggle-sort";
  std::string data_str;

  if ( flag & SORT_FLAG_ASC )
    data_str += " data-sortdir=\"asc\"";

  if ( flag & SORT_FLAG_ALPHA )
    data_str += " data-sorttype=\"alpha\"";

  if ( flag & SORT_FLAG_BOTH )
    data_str += " data-sortrows=\"both\"";

  if ( flag & SORT_FLAG_LEFT )
    class_str += " left";

  if ( !helptext.empty() )
  {
    class_str += " help";
    data_str += fmt::format(" data-help=\"#{}\"",  helptext );
  }

  os << "<th class=\"" << class_str << "\"" << data_str << ">" << header << "</th>\n";
}

void sorttable_help_header( report::sc_html_stream& os, std::string_view header, std::string_view helptext,
                            unsigned flag = SORT_FLAG_NONE )
{
  sorttable_header( os, header, flag, helptext );
}

std::string output_action_name( const stats_t& s, const player_t* actor, bool simple )
{
  std::string class_attr;
  action_t* a            = nullptr;
  std::string stats_type = util::stats_type_string( s.type );

  if ( s.player->sim->report_details )
  {
    class_attr = " id=\"actor" + util::to_string( s.player->index ) + "_" +
                 util::remove_special_chars( s.name_str ) + "_" + stats_type +
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
    if ( simple )
    {
      if ( auto con = dynamic_cast<dbc_consumable_base_t*>( a ); con && con->item_data )
        name = report_decorators::decorated_item_data( *s.player->sim, *con->item_data );
      else if ( a->item && &a->item->parsed.data != &dbc_item_data_t::nil() )
        name = report_decorators::decorated_item_data( *s.player->sim, a->item->parsed.data );
      else
        name = report_decorators::decorated_action( *a );
    }
    else
    {
      name = report_decorators::decorated_action( *a );
    }
  }
  else
  {
    name = util::encode_html( s.name_str );
  }

  // If we are printing a stats object that belongs to a pet, for an actual
  // actor, print out the pet name too unless the action already has a parent
  if ( actor && !actor->is_pet() && s.player->is_pet() && !s.parent )
    name += " (" + util::encode_html( s.player->name_str ) + ")";

  return "<span" + class_attr + ">" + name + "</span>";
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
  double sum = 0;
  double count = 0;

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

template <typename T, typename V>
int num_count( const T& results, const std::initializer_list<V>& selectors )
{
  double count = 0;

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
  } );

  return as<int>( count );
}

double vulnerable_fight_length( player_t* actor )
{
  double fight_length = actor->collected_data.fight_length.mean();

  auto it = range::find_if( actor->report_information.dynamic_buffs, []( buff_t* b ) {
    return b->name_str == "invulnerable";
  } );

  if ( it != actor->report_information.dynamic_buffs.end() )
  {
    fight_length *= 1.0 - ( *it )->uptime_pct.pretty_mean() / 100.0;
  }

  return fight_length;
}

double target_fight_length( sim_t* sim )
{
  double fight_length = 0.0;

  for ( auto t : sim->targets_by_name )
  {
    fight_length += vulnerable_fight_length( t );

    for ( auto pet : t->pet_list )
    {
      fight_length += vulnerable_fight_length( pet );
    }
  }

  return fight_length;
}

void collect_compound_stats( std::unique_ptr<stats_t>& compound_stats, const stats_t* stats, double& count,
                             double& tick_time )
{
  compound_stats->merge( *stats );
  count += stats->has_direct_amount_results()
    ? stats->num_direct_results.mean()
    : stats->num_tick_results.mean();
  tick_time += stats->has_tick_amount_results()
    ? stats->total_tick_time.mean()
    : 0;

  range::for_each( stats->children, [ & ]( stats_t* s ) {
    collect_compound_stats( compound_stats, s, count, tick_time );
  } );
}

void print_html_action_summary( report::sc_html_stream& os, unsigned stats_mask, int result_type, const stats_t& s,
                                const player_t& p )
{
  using full_result_t = std::array<stats_t::stats_results_t, FULLTYPE_MAX>;
  using result_t = std::array<stats_t::stats_results_t, RESULT_MAX>;

  const auto& dr = s.direct_results;
  const auto& tr = s.tick_results;

  double count = result_type == 1 ? s.num_tick_results.mean() : s.num_direct_results.mean();

  // Strings for merged stat reporting
  std::string count_str;
  std::string critpct_str;
  std::string uppct_str;
  bool show_uppct = player_has_tick_results( p, stats_mask ) && result_type == 1;

  // Create Merged Stat
  if ( !s.children.empty() )
  {
    auto compound_stats = std::make_unique<stats_t>( s.name_str + "_compound", s.player );

    double compound_count = 0.0;
    double compound_tick_time = 0.0;

    collect_compound_stats( compound_stats, &s, compound_count, compound_tick_time );
    compound_stats->analyze();

    count_str = fmt::format( "&#160;({:.1Lf})", compound_count );

    const auto& compound_dr = compound_stats->direct_results;
    const auto& compound_tr = compound_stats->tick_results;

    double compound_critpct = result_type == 1 ? pct_value<result_t, result_e>( compound_tr, { RESULT_CRIT } )
                                               : pct_value<full_result_t, full_result_e>( compound_dr,
                                                 { FULLTYPE_CRIT, FULLTYPE_CRIT_BLOCK, FULLTYPE_CRIT_CRITBLOCK } );
    critpct_str = fmt::format( "&#160;({:.1f}%)", compound_critpct );

    if ( show_uppct )
    {
      auto pct = 100 * s.total_tick_time.mean() / target_fight_length( p.sim );
      auto cmp_pct = 100 * compound_tick_time / target_fight_length( p.sim );

      if ( pct == cmp_pct )
        uppct_str = fmt::format( "{:.1f}%", pct );
      else if ( !pct && s.children.size() > 1 )
        uppct_str = fmt::format( "({:.1f}%)", cmp_pct );
    }
  }
  else
  {
    if ( show_uppct )
      uppct_str = fmt::format( "{:.1f}%", 100 * s.total_tick_time.mean() / target_fight_length( p.sim ) );
  }

  // Result type and count
  os.format( R"(<td class="left">{}</td><td>{:.1Lf}{}</td>)",
    result_type == 1 ? "Periodic" : "Direct", count, count_str );

  // Result amount
  os.format( "<td>{:.0Lf}</td><td>{:.0Lf}</td><td>{:.0Lf}</td>",
    result_type == 1
      ? mean_value<result_t, result_e>( tr, { RESULT_HIT } )
      : mean_value<full_result_t, full_result_e>( dr, { FULLTYPE_HIT, FULLTYPE_HIT_BLOCK, FULLTYPE_HIT_CRITBLOCK } ),
    result_type == 1
      ? mean_value<result_t, result_e>( tr, { RESULT_CRIT } )
      : mean_value<full_result_t, full_result_e>( dr, { FULLTYPE_CRIT, FULLTYPE_CRIT_BLOCK, FULLTYPE_CRIT_CRITBLOCK } ),
    result_type == 1
      ? mean_damage( tr )
      : mean_damage( dr ) );

  // Crit%
  os.format( "<td>{:.1f}%{}</td>",
    result_type == 1
      ? pct_value<result_t, result_e>( tr, { RESULT_CRIT } )
      : pct_value<full_result_t, full_result_e>( dr, { FULLTYPE_CRIT, FULLTYPE_CRIT_BLOCK, FULLTYPE_CRIT_CRITBLOCK } ),
    critpct_str );

  if ( player_has_avoidance( p, stats_mask ) )
    os.format( "<td>{:.1f}%</td>",  // direct_results Avoid%
      result_type == 1
        ? pct_value<result_t, result_e>( tr, { RESULT_MISS, RESULT_DODGE, RESULT_PARRY } )
        : pct_value<full_result_t, full_result_e>( dr, { FULLTYPE_MISS, FULLTYPE_DODGE, FULLTYPE_PARRY } ) );

  if ( player_has_glance( p, stats_mask ) )
    os.format( "<td>{:.1f}%</td>",  // direct_results Glance%
      result_type == 1
        ? pct_value<result_t, result_e>( tr, { RESULT_GLANCE } )
        : pct_value<full_result_t, full_result_e>( dr, { FULLTYPE_GLANCE, FULLTYPE_GLANCE_BLOCK, FULLTYPE_GLANCE_CRITBLOCK } ) );

  if ( player_has_block( p, stats_mask ) )
    os.format( "<td>{:.1f}%</td>",  // direct_results Block%
      result_type == 1
        ? 0
        : pct_value<full_result_t, full_result_e>(dr,
          { FULLTYPE_HIT_BLOCK, FULLTYPE_HIT_CRITBLOCK, FULLTYPE_GLANCE_BLOCK,
            FULLTYPE_GLANCE_CRITBLOCK, FULLTYPE_CRIT_BLOCK, FULLTYPE_CRIT_CRITBLOCK } ) );

  if ( show_uppct )
    os.format( "<td>{}</td>", uppct_str );
  else
    os.printf( "<td></td>" );
}

void collect_aps( const stats_t* stats, double& caps, double& capspct )
{
  caps += stats->portion_apse.mean();
  capspct += stats->portion_amount;

  range::for_each( stats->children, [&]( const stats_t* s ) {
    if (stats->type == s -> type)
    {
      collect_aps( s, caps, capspct );
    }
  } );
}

void print_html_action_info( report::sc_html_stream& os, unsigned stats_mask, const stats_t& s, int n_columns,
                             const player_t* actor = nullptr, int indentation = 0 )
{
  const player_t& p = *s.player->get_owner_or_self();
  bool hasparent = s.parent && s.parent->player == actor && ( s.mask() & s.parent->mask() );
  std::string row_class;
  std::string rowspan;

  if ( use_small_table( &p ) )
    row_class = " small";

  if ( hasparent )
    row_class += " childrow";

  os.format( R"(<tr class="toprow{} right">)", row_class );

  int result_rows = s.has_direct_amount_results() + s.has_tick_amount_results();

  if ( result_rows > 1 )
    rowspan = " rowspan=\"" + util::to_string( result_rows ) + "\"";

  // Ability name
  os.format( R"(<td style="white-space:nowrap" class="left"{}>)", rowspan );

  if ( hasparent )
  {
    for ( int i = 0; i < indentation; ++i )
    {
      os << "&#160;&#160;&#160;\n";
    }
  }

  auto simple = !( s.compound_amount > 0 || ( hasparent && s.parent->compound_amount > 0 ) );
  os << output_action_name( s, actor, simple ) << "</td>\n";

  // DPS and DPS %
  // Skip for abilities that do no damage
  if ( !simple )
  {
    std::string compound_aps;
    std::string compound_aps_pct;
    double cAPS                  = 0.0;
    double cAPSpct               = 0.0;

    collect_aps( &s, cAPS, cAPSpct );

    if ( cAPS > s.portion_aps.mean() )
      compound_aps = fmt::format( "&#160;({:.0Lf})", cAPS );

    // For stats not belonging to the original actor (eg. pet spells added as child to the owner), report aps / apse,
    // similar to the pet section.
    if ( s.player != actor )
      compound_aps = fmt::format( "&#160;/&#160;{:.0Lf}", s.portion_apse.mean() );

    if ( cAPSpct > s.portion_amount )
      compound_aps_pct = fmt::format( "&#160;({:.1f}%)", cAPSpct * 100 );

    os.format( R"(<td{}>{:.0Lf}{}</td><td{}>{:.1f}%{}</td>)",
               rowspan, s.portion_aps.pretty_mean(), compound_aps,
               rowspan, s.portion_amount * 100, compound_aps_pct );
  }

  // Number of executes and execute interval
  os.format( R"(<td{}>{:.1Lf}</td><td{}>{:.2Lf}s</td>)",
             rowspan, s.num_executes.pretty_mean(),
             rowspan, s.total_intervals.pretty_mean() );

  // Skip the rest of this for abilities that do no damage
  if ( s.compound_amount > 0 )
  {
    // Amount per execute & execute time
    os.format( R"(<td{}>{:.0Lf}</td><td{}>{:.0Lf}</td>)",
               rowspan, s.ape,
               rowspan, s.apet );

    bool periodic_only = false;
    if ( s.has_direct_amount_results() )
    {
      print_html_action_summary( os, stats_mask, 0, s, p );
    }
    else if ( s.has_tick_amount_results() )
    {
      periodic_only = true;
      print_html_action_summary( os, stats_mask, 1, s, p );
    }
    else
      os.format( R"(<td colspan="{}"></td>)", n_columns );

    os << "</tr>\n";

    if ( !periodic_only && s.has_tick_amount_results() )
    {
      os.format( R"(<tr class="childrow{} right">)", row_class );
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
    std::string timeline_stat_aps_str;
    if ( !s.timeline_aps_chart.empty() )
    {
      timeline_stat_aps_str = "<img src=\"" + s.timeline_aps_chart + "\" alt=\"" +
        ( s.type == STATS_DMG ? "DPS" : "HPS" ) + " Timeline Chart\" />\n";
    }
    std::string aps_distribution_str;
    if ( !s.aps_distribution_chart.empty() )
    {
      aps_distribution_str = "<img src=\"" + s.aps_distribution_chart + "\" alt=\"" +
        ( s.type == STATS_DMG ? "DPS" : "HPS" ) + " Distribution Chart\" />\n";
    }
    os << "<tr class=\"details hide\">\n"
       << "<td colspan=\"" << ( n_columns > 0 ? ( 7 + n_columns ) : 3 ) << "\" class=\"filler\">\n";

    // Stat Details
    os << "<h4>Stats Details: " << util::encode_html( util::inverse_tokenize( s.name_str ) ) << "</h4>\n";

    os << R"(<table class="details"><tr>)"
       << "<th>Type</th>"
       << "<th>Executes</th>"
       << "<th>Direct Results</th>"
       << "<th>Ticks</th>"
       << "<th>Tick Results</th>"
       << "<th>Refreshes</th>"
       << "<th>Execute Time per Execution</th>"
       << "<th>Tick Time per Tick</th>"
       << "<th>Actual Amount</th>"
       << "<th>Raw Amount</th>"
       << "<th>Mitigated</th>"
       << "<th>Amount per Total Time</th>"
       << "<th>Amount per Total Execute Time</th>"
       << "</tr>\n";

    os.format( R"(<tr class="right"><td class="left">{}</td>)"
               "<td>{:.2Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.4Lf}</td>"
               "<td>{:.4Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.2Lf}</td></tr></table>\n",
               util::stats_type_string( s.type ),
               s.num_executes.mean(),
               s.num_direct_results.mean(),
               s.num_ticks.mean(),
               s.num_tick_results.mean(),
               s.num_refreshes.mean(),
               s.etpe,
               s.ttpt,
               s.actual_amount.mean(),
               s.total_amount.mean(),
               s.overkill_pct,
               s.aps,
               s.apet );

    if ( !s.portion_aps.simple || !s.portion_apse.simple || !s.actual_amount.simple )
    {
      os << "<table class=\"details\">\n";
      report_helper::print_html_sample_data( os, p, s.actual_amount, "Actual Amount" );
      report_helper::print_html_sample_data( os, p, s.portion_aps, "portion Amount per Second ( pAPS )" );
      report_helper::print_html_sample_data( os, p, s.portion_apse, "portion Effective Amount per Second ( pAPSe )" );
      os << "</table>\n";

      if ( !s.portion_aps.simple && p.sim->scaling->has_scale_factors() && s.scaling )
      {
        os << "<table class=\"details\">\n";
        os << "<tr>\n"
           << "<th class=\"help\" data-help=\"#help-scale-factors\">?</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p.scaling->scales_with[ i ] )
          {
            os.printf( "<th>%s</th>\n", util::stat_type_abbrev( i ) );
          }
        if ( p.sim->scaling->scale_lag )
        {
          os << "<th>ms Lag</th>\n";
        }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Scale Factors</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
          if ( p.scaling->scales_with[ i ] )
          {
            if ( s.scaling->value.get_stat( i ) > 1.0e5 )
              os.printf( "<td>%.*e</td>\n", p.sim->report_precision, s.scaling->value.get_stat( i ) );
            else
              os.printf( "<td>%.*f</td>\n", p.sim->report_precision, s.scaling->value.get_stat( i ) );
          }
        os << "</tr>\n";
        os << "<tr>\n"
           << "<th class=\"left\">Scale Deltas</th>\n";
        for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
        {
          if ( !p.scaling->scales_with[ i ] )
            continue;

          double value = p.sim->scaling->stats->get_stat( i );
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
              os.printf( "<td>%.*e</td>\n", p.sim->report_precision, s.scaling->error.get_stat( i ) );
            else
              os.printf( "<td>%.*f</td>\n", p.sim->report_precision, s.scaling->error.get_stat( i ) );
          }
        os << "</tr>\n";
        os << "</table>\n";
      }
    }

    // Detailed breakdown of damage or healing ability
    os << R"(<table class="details">)";
    if ( s.num_direct_results.mean() > 0 )
    {
      // Direct Damage
      os << R"(<tr><th rowspan="2">Direct Results</th>)"
         << R"(<th colspan="4">Count</th>)"
         << R"(<th colspan="3">Simulation</th>)"
         << R"(<th colspan="3">Iteration Average</th>)"
         << R"(<th colspan="3">Amount</th></tr>)";

      os << "<tr>"
         << "<th>Percent</th>"
         << "<th>Mean</th>"
         << "<th>Min</th>"
         << "<th>Max</th>"
         << "<th>Mean</th>"
         << "<th>Min</th>"
         << "<th>Max</th>"
         << "<th>Mean</th>"
         << "<th>Min</th>"
         << "<th>Max</th>"
         << "<th>Actual</th>"
         << "<th>Raw</th>"
         << "<th>Mitigated</th>"
         << "</tr>\n";

      for ( full_result_e i = FULLTYPE_MAX; --i >= FULLTYPE_NONE; )
      {
        if ( !s.direct_results[ i ].count.mean() )
          continue;

        os.format( R"(<tr class="right"><td class="left">{}</td>)"
                   "<td>{:.2f}%</td>"
                   "<td>{:.2Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.2Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.2Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.2f}%</td></tr>\n",
                   util::full_result_type_string( i ),
                   s.direct_results[ i ].pct,
                   s.direct_results[ i ].count.mean(),
                   s.direct_results[ i ].count.min(),
                   s.direct_results[ i ].count.max(),
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
      os << R"(<tr><th rowspan="2">Tick Results</th>)"
         << R"(<th colspan="4">Count</th>)"
         << R"(<th colspan="3">Simulation</th>)"
         << R"(<th colspan="3">Iteration Average</th>)"
         << R"(<th colspan="3">Amount</th></tr>)";

      os << "<tr>"
         << "<th>Percent</th>"
         << "<th>Mean</th>"
         << "<th>Min</th>"
         << "<th>Max</th>"
         << "<th>Mean</th>"
         << "<th>Min</th>"
         << "<th>Max</th>"
         << "<th>Mean</th>"
         << "<th>Min</th>"
         << "<th>Max</th>"
         << "<th>Actual</th>"
         << "<th>Total</th>"
         << "<th>Mitigated</th>"
         << "</tr>\n";

      for ( result_e i = RESULT_MAX; --i >= RESULT_NONE; )
      {
        if ( !s.tick_results[ i ].count.mean() )
          continue;

        os.format( R"(<tr class="right"><td class="left">{}</td>)"
                   "<td>{:.2f}%</td>"
                   "<td>{:.2Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.2Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.2Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.0Lf}</td>"
                   "<td>{:.2f}%</td></tr>\n",
                   util::result_type_string( i ),
                   s.tick_results[ i ].pct,
                   s.tick_results[ i ].count.mean(),
                   s.tick_results[ i ].count.min(),
                   s.tick_results[ i ].count.max(),
                   s.tick_results[ i ].actual_amount.mean(),
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

    if ( s.has_direct_amount_results() || s.has_tick_amount_results() )
    {
      highchart::time_series_t ts( highchart::build_id( s, {} ), *s.player->sim );
      chart::generate_stats_timeline( ts, s );
      os << ts.to_target_div();
      s.player->sim->add_chart_data( ts );
    }

    // Action Details
    std::vector<std::string> processed_actions;
    std::vector<std::string> expressions;
    bool long_expr_label = false;

    for ( const auto& a : s.action_list )
    {
      bool found            = false;
      size_t size_processed = processed_actions.size();

      for ( size_t k = 0; k < size_processed && !found; k++ )
        if ( processed_actions[ k ] == a->name() )
          found = true;

      // Grab expression strings first in case we have multiple APL lines
      double action_count = a->total_executions /
        as<double>( p.sim->single_actor_batch ? a->player->collected_data.total_iterations + p.sim->threads
                                              : p.sim->iterations );
      if ( action_count > 0.0 )
      {
        std::string expression_str;

        if ( !a->option.if_expr_str.empty() )
        {
          expression_str += "<li><span class=\"label$SHORT$\">if_expr:</span><span class=\"tooltip\">" +
                            util::encode_html( a->option.if_expr_str ) + "</span></li>\n";
        }
        if ( !a->option.target_if_str.empty() )
        {
          long_expr_label = true;
          expression_str += "<li><span class=\"label\">target_if_expr:</span><span class=\"tooltip\">" +
                            util::encode_html( a->option.target_if_str ) + "</span></li>\n";
        }
        if ( !a->option.interrupt_if_expr_str.empty() )
        {
          long_expr_label = true;
          expression_str += "<li><span class=\"label\">interrupt_if_expr:</span><span class=\"tooltip\">" +
                            util::encode_html( a->option.interrupt_if_expr_str ) + "</span></li>\n";
        }
        if ( !a->option.early_chain_if_expr_str.empty() )
        {
          long_expr_label = true;
          expression_str += "<li><span class=\"label\">early_chain_if_expr:</span><span class=\"tooltip\">" +
                            util::encode_html( a->option.early_chain_if_expr_str ) + "</span></li>\n";
        }
        if ( !a->option.cancel_if_expr_str.empty() )
        {
          long_expr_label = true;
          expression_str += "<li><span class=\"label\">cancel_if_expr:</span><span class=\"tooltip\">" +
                            util::encode_html( a->option.cancel_if_expr_str ) + "</span></li>\n";
        }
        if ( !a->option.sync_str.empty() )
        {
          long_expr_label = true;
          expression_str += "<li><span class=\"label\">sync_expr:</span><span class=\"tooltip\">" +
                            util::encode_html( a->option.sync_str ) + "</span></li>\n";
        }
        if ( !a->option.target_str.empty() )
        {
          long_expr_label = true;
          expression_str += "<li><span class=\"label\">target_expr:</span><span class=\"tooltip\">" +
                            util::encode_html( a->option.target_str ) + "</span></li>\n";
        }

        std::string apl_name = util::encode_html(
          a->action_list && !a->action_list->name_str.empty() ? a->action_list->name_str : "unknown" );
        std::string marker_str = "<div class=\"flex label short\"><span class=\"mono\">[";
        if ( a->marker )
          marker_str += a->marker;
        else
          marker_str += "&#160;";
        marker_str += "]</span>:";
        marker_str += util::to_string( action_count, 2 );
        marker_str += "</div>\n";
        expression_str = "<div class=\"flex label\">" + apl_name + "</div>" +
                         ( marker_str.empty() ? "\n" : marker_str ) + "<div>" + expression_str + "</div>\n";
        expressions.push_back( expression_str );
      }

      if ( found )
        continue;

      processed_actions.emplace_back( a->name() );

      os << "<div class=\"flex\">\n";  // Wrap details, damage/weapon, spell_data

      os.format( R"(<div><h4>Action Details: {}</h4><ul class="label">)"
                 "<li><span>id:</span>{}</li>"
                 "<li><span>school:</span>{}</li>"
                 "<li><span>range:</span>{:.1f}</li>"
                 "<li><span>travel_speed:</span>{:.4f}</li>"
                 "<li><span>radius:</span>{:.1f}</li>"
                 "<li><span>trigger_gcd:</span>{:.4f}</li>"
                 "<li><span>gcd_type:</span>{}</li>"
                 "<li><span>min_gcd:</span>{:.4f}</li>"
                 "<li><span>cooldown:</span>{:.3f}</li>"
                 "<li><span>cooldown hasted:</span>{}</li>"
                 "<li><span>charges:</span>{}</li>"
                 "<li><span>base_recharge_multiplier:</span>{:.3f}</li>"
                 "<li><span>base_execute_time:</span>{:.2f}</li>"
                 "<li><span>base_crit:</span>{:.2f}</li>"
                 "<li><span>target:</span>{}</li>"
                 "<li><span>aoe:</span>{}</li>\n",
                 util::encode_html( util::inverse_tokenize( a->name() ) ),
                 a->id,
                 util::school_type_string( a->get_school() ),
                 a->range,
                 a->travel_speed,
                 a->radius,
                 a->trigger_gcd.total_seconds(),
                 util::gcd_haste_type_string(a->gcd_type),
                 a->min_gcd.total_seconds(),
                 a->cooldown->duration.total_seconds(),
                 a->cooldown->hasted ? "true" : "false",
                 a->cooldown->charges,
                 a->base_recharge_multiplier,
                 a->base_execute_time.total_seconds(),
                 a->base_crit,
                 a->target ? util::encode_html( a->target->name() ) : "",
                 a->aoe );

      if ( a->aoe > 1 || a->aoe < 0 )
      {
        fmt::print( os, "<li><span>split_aoe_damage:</span>{}</li>\n", a->split_aoe_damage ? "true" : "false" );

        if ( a->reduced_aoe_targets > 0 )
          fmt::print( os, "<li><span>reduced_aoe_targets:</span>{}</li>\n", a->reduced_aoe_targets );

        if ( a->full_amount_targets > 0 )
          fmt::print( os, "<li><span>full_amount_targets:</span>{}</li>\n", a->full_amount_targets );
      }

      fmt::print( os, "<li><span>harmful:</span>{}</li>\n", a->harmful ? "true" : "false" );

      os << "</ul>\n</div>\n";  // Close details

      os << "<div>\n";  // Wrap damage/weapon

      // resource details
      os.format( R"(<div><h4>Resources</h4><ul class="label">)"
                 "<li><span>resource:</span>{}</li>"
                 "<li><span>base_cost:</span>{}</li>"
                 "<li><span>secondary_cost:</span>{}</li>"
                 "<li><span>energize_type:</span>{}</li>"
                 "<li><span>energize_resource:</span>{}</li>"
                 "<li><span>energize_amount:</span>{:.1f}</li></ul></div>\n",
                 util::resource_type_string( a->current_resource() ),
                 a->base_costs[ a->current_resource() ],
                 a->secondary_costs[ a->current_resource() ],
                 a->energize_type,
                 a->energize_resource,
                 a->energize_amount );

      if ( a->spell_power_mod.direct || a->base_dd_min || a->base_dd_max )
      {
        os.format( R"(<div><h4>Direct Damage</h4><ul class="label">)"
                   "<li><span>may_crit:</span>{}</li>"
                   "<li><span>attack_power_mod.direct:</span>{:.6f}</li>"
                   "<li><span>spell_power_mod.direct:</span>{:.6f}</li>"
                   "<li><span>base_dd_min:</span>{:.2f}</li>"
                   "<li><span>base_dd_max:</span>{:.2f}</li>"
                   "<li><span>base_dd_mult:</span>{:.2f}</li>\n"
                   "<li><span>base_multiplier:</span>{:.2f}</li></ul></div>\n",
                   a->may_crit ? "true" : "false",
                   a->attack_power_mod.direct,
                   a->spell_power_mod.direct,
                   a->base_dd_min,
                   a->base_dd_max,
                   a->base_dd_multiplier,
                   a->base_multiplier );
      }

      if ( a->dot_duration > timespan_t::zero() )
      {
        os.format( R"(<div><h4>Damage Over Time</h4><ul class="label">)"
                   "<li><span>tick_may_crit:</span>{}</li>"
                   "<li><span>tick_zero:</span>{}</li>"
                   "<li><span>tick_on_application:</span>{}</li>"
                   "<li><span>rolling_periodic:</span>{}</li>"
                   "<li><span>attack_power_mod.tick:</span>{:.6f}</li>"
                   "<li><span>spell_power_mod.tick:</span>{:.6f}</li>"
                   "<li><span>base_td:</span>{:.2f}</li>"
                   "<li><span>base_td_mult:</span>{:.2f}</li>"
                   "<li><span>base_multiplier:</span>{:.2f}</li>"
                   "<li><span>dot_duration:</span>{:.2f}</li>"
                   "<li><span>base_tick_time:</span>{:.2f}</li>"
                   "<li><span>hasted_ticks:</span>{}</li>"
                   "<li><span>dot_behavior:</span>{}</li></ul></div>\n",
                   a->tick_may_crit ? "true" : "false",
                   a->tick_zero ? "true" : "false",
                   a->tick_on_application ? "true" : "false",
                   a->rolling_periodic ? "true" : "false",
                   a->attack_power_mod.tick,
                   a->spell_power_mod.tick,
                   a->base_td,
                   a->base_td_multiplier,
                   a->base_multiplier,
                   a->dot_duration.total_seconds(),
                   a->base_tick_time.total_seconds(),
                   a->hasted_ticks ? "true" : "false",
                   util::dot_behavior_type_string( a->dot_behavior ) );
      }

      if ( a->weapon )
      {
        os.format( R"(<div><h4>Weapon</h4><ul class="label">)"
                   "<li><span>normalized:</span>{}</li>"
                   "<li><span>weapon_power_mod:</span>{:.6f}</li>"
                   "<li><span>weapon_multiplier:</span>{:.2f}</li></ul></div>\n",
                   a->normalize_weapon_speed ? "true" : "false",
                   a->weapon_power_mod,
                   a->weapon_multiplier );
      }

      os << "</div>\n";  // Close damage/weapon

      if ( a->data().ok() )
      {
        const auto& spell_text = a->player->dbc->spell_text( a->data().id() );
        os.format( R"(<div class="shrink"><h4>Spelldata</h4><ul class="label">)"
                   "<li><span>id:</span>{}</li>"
                   "<li><span>name:</span>{}</li>"
                   "<li><span>school:</span>{}</li>"
                   R"(<li><span>tooltip:</span><span class="tooltip">{}</span></li>)"
                   R"(<li><span>description:</span><span class="tooltip">{}</span></li></ul></div>)",
                   a->data().id(),
                   util::encode_html( a->data().name_cstr() ),
                   util::school_type_string( a->data().get_school_type() ),
                   util::encode_html( report_helper::pretty_spell_text( a->data(), spell_text.tooltip(), p ) ),
                   util::encode_html( report_helper::pretty_spell_text( a->data(), spell_text.desc(), p ) ) );
      }

      os << "</div>\n";  // Close details, damage/weapon, spell_data
    }

    if ( !expressions.empty() )
    {
      os << "<div>\n"
         << "<h4>Action Priority List</h4>\n";

      for ( auto& str : expressions )
      {
        if ( long_expr_label )
          util::erase_all( str, "$SHORT$" );
        else
          util::replace_all( str, "$SHORT$", " short" );

        os << "<ul class=\"flex\">\n" << str << "</ul>\n";
      }

      os << "</div>\n";
    }

    if ( s.action_list.size() )
    {
      os << "<div class=\"flexwrap\">\n";

      s.action_list.front()->html_customsection( os );

      os << "</div>";
    }

    os << "</td>\n"
       << "</tr>\n";
  }
}

// print_html_action_resource ===============================================

void print_html_action_resource( report::sc_html_stream& os, const stats_t& s,
                                 std::array<double, RESOURCE_MAX>& total_usage )
{
  std::string decorated_name = report_decorators::decorated_action( *s.action_list.front() );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( s.resource_gain.actual[ i ] > 0 )
    {
      os.format( R"(<tr class="right"><td class="left">{}</td><td class="left">{}</td>)"
                 "<td>{:.2Lf}</td>"
                 "<td>{:.2Lf}</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2Lf}</td>"
                 "<td>{:.2Lf}</td>"
                 "<td>{:.2Lf}</td></tr>\n",
                 decorated_name, util::inverse_tokenize( util::resource_type_string( i ) ),
                 s.resource_gain.count[ i ],
                 s.resource_gain.actual[ i ],
                 ( s.resource_gain.actual[ i ] ? s.resource_gain.actual[ i ] / total_usage[ i ] * 100.0 : 0.0 ),
                 s.resource_gain.actual[ i ] / s.resource_gain.count[ i ],
                 s.rpe[ i ],
                 s.apr[ i ] );
    }
  }
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
      "<table class=\"sc odd\">\n"
      "<thead>\n"
      "<tr>\n"
      "<th>Source</th>\n"
      "<th>Slot</th>\n"
      "<th>Average Item Level: %.2f</th>\n"
      "</tr>\n"
      "</thead>\n",
      util::round( p.avg_item_level() ), 2 );

  os << "<tbody>\n";
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
        util::encode_html( item.source_str ).c_str(),
        util::inverse_tokenize( item.slot_name() ).c_str(),
        report_decorators::decorated_item( item ).c_str() );

    std::string item_sim_desc = "ilevel: " + util::to_string( item.item_level() );

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

    if ( !item.parsed.gem_stats.empty() )
    {
      item_sim_desc += ", gems: { ";
      item_sim_desc += item.gem_stats_str();
      if ( item.socket_color_match() && !item.parsed.socket_bonus_stats.empty() )
      {
        item_sim_desc += ", ";
        item_sim_desc += item.socket_bonus_stats_str();
      }
      item_sim_desc += " }";
    }

    if ( !item.parsed.enchant_stats.empty() )
    {
      item_sim_desc += ", enchant: { ";
      item_sim_desc += item.enchant_stats_str();
      if ( !item.parsed.encoded_enchant.empty() )
      {
        item_sim_desc += " (";
        item_sim_desc += item.parsed.encoded_enchant;
        item_sim_desc += ")";
      }
      item_sim_desc += " }";
    }
    else if ( !item.parsed.encoded_enchant.empty() )
    {
      item_sim_desc += ", enchant: " + item.parsed.encoded_enchant;
    }

    if ( item.selected_temporary_enchant() > 0 )
    {
      const auto& temp_enchant = temporary_enchant_entry_t::find_by_enchant_id(
          item.selected_temporary_enchant(), item.player->dbc->ptr );
      const auto spell = item.player->find_spell( temp_enchant.spell_id );
      item_sim_desc += ", temporary_enchant: ";
      item_sim_desc += report_decorators::decorated_spell_data_item( *item.sim, spell, item );
    }

    auto has_relics = range::any_of( item.parsed.gem_actual_ilevel, []( unsigned v ) { return v != 0; } );
    if ( has_relics )
    {
      item_sim_desc += ", relics: { ";
      auto first = true;
      for ( auto bonus : item.parsed.gem_actual_ilevel )
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

    if ( !item.parsed.azerite_ids.empty() )
    {
      std::stringstream s;
      for ( size_t i = 0; i < item.parsed.azerite_ids.size(); ++i )
      {
        const auto& power = item.player -> dbc->azerite_power( item.parsed.azerite_ids[ i ] );
        if ( power.id == 0 || ! item.player -> azerite -> is_enabled( power.id ) )
        {
          continue;
        }

        const auto spell = item.player -> find_spell( power.spell_id );

        s << report_decorators::decorated_spell_data_item(*item.sim, spell, item);

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

    if ( item.parsed.data.id == 158075 )
    {
      std::stringstream s;
      s << "level: " << item.parsed.azerite_level;

      if ( item.player->azerite_essence )
      {
        std::stringstream s2;
        auto spell_list = item.player->azerite_essence->enabled_essences();

        for ( size_t i = 0; i < spell_list.size(); ++i )
        {
          const auto spell = item.player->find_spell( spell_list[ i ] );

          s2 << report_decorators::decorated_spell_data_item(*item.sim, spell, item);

          if ( i < spell_list.size() - 1 )
            s2 << ", ";
        }

        if ( !s2.str().empty() )
          s << ", azerite essences: { " << s2.str() << " }";
      }

      if ( !s.str().empty() )
      {
        item_sim_desc += "<br/>";
        item_sim_desc += s.str();
      }
    }

    if ( !item.parsed.gem_color.empty() && range::find( item.parsed.gem_color, SOCKET_COLOR_PRIMORDIAL ) != item.parsed.gem_color.end() )
    {
      item_sim_desc += ", primordial_stones: { ";
      bool first = true;
      for ( const auto e : item.parsed.special_effects )
      {
        if ( !e->driver() || !e->driver()->affected_by_label( LABEL_PRIMORDIAL_STONE ) )
          continue;

        if ( !first )
          item_sim_desc += ", ";

        item_sim_desc += report_decorators::decorated_spell_data_item( *item.sim, e->driver(), item );
        first = false;
      }
      item_sim_desc += " }";
    }

    {
      std::stringstream s;
      for ( const item_effect_t& effect : item.parsed.data.effects )
      {
        if ( effect.spell_id == 0 )
          continue;

        if ( !s.str().empty() )
          s << ", ";

        s << util::item_spell_trigger_string( static_cast<item_spell_trigger_type>( effect.type ) ) << ": ";
        auto spell = item.player->find_spell( effect.spell_id );
        s << report_decorators::decorated_spell_data_item(*item.sim, spell, item);
      }

      if ( !s.str().empty() )
      {
        item_sim_desc += "<br/>";
        item_sim_desc += "item effects: { ";
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
  os << "</tbody>\n"
     << "</table>\n"
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
  const std::array<stat_e, 4> hybrid_stats = { { STAT_STR_AGI_INT, STAT_STR_AGI, STAT_STR_INT, STAT_AGI_INT } };

  for ( const auto& item : p.items )
  {
    for ( auto hybrid_stat : hybrid_stats )
    {
      auto real_stat   = p.convert_hybrid_stat( hybrid_stat );
      attribute_e attr = static_cast<attribute_e>( real_stat );

      if ( p.gear.attribute[ attr ] >= 0 || item.stats.get_stat( hybrid_stat ) <= 0 )
      {
        continue;
      }

      hybrid_attributes[ attr ] += item.stats.get_stat( hybrid_stat );
    }
  }

  if ( p.collected_data.fight_length.mean() > 0 )
  {
    os << "<div class=\"player-section stats\">\n"
       << "<h3 class=\"toggle\">Stats</h3>\n"
       << "<div class=\"toggle-content hide\">\n"
       << "<table class=\"sc even\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th>Level Bonus (" << p.level() << ")</th>\n"
       << "<th>Race Bonus (" << util::race_type_string(p.race) << ")</th>\n"
       << "<th class=\"help\" data-help=\"#help-stats-raid-buffed\">Raid-Buffed</th>\n"
       << "<th class=\"help\" data-help=\"#help-stats-unbuffed\">Unbuffed</th>\n"
       << "<th class=\"help\" data-help=\"#help-gear-amount\">Gear Amount</th>\n"
       << "</tr>\n"
       << "</thead>\n";

    os << "<tbody>\n";
    for ( attribute_e i = ATTRIBUTE_NONE; ++i < ATTR_AGI_INT; )  // ATTRIBUTE_NONE is skipped via ++i
    {
      os.format( R"(<tr class="right"><th class="left">{}</th>)"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}",
                 util::inverse_tokenize( util::attribute_type_string( i ) ),
                 ( !p.is_enemy() && !p.is_pet() )
                   ? util::floor( dbc::stat_data_to_attribute( p.dbc->attribute_base( p.type, p.level() ), i ) )
                   : 0,
                 util::floor( dbc::stat_data_to_attribute( p.dbc->race_base( p.race ), i ) ),
                 util::floor( buffed_stats.attribute[ i ] ), util::floor( p.get_attribute( i ) ),
                 util::floor( p.total_gear.attribute[ i ] ) );

      // append hybrid attributes as a parenthetical if appropriate
      if ( hybrid_attributes[ i ] > 0 )
        os.printf( " (%.0f)", hybrid_attributes[ i ] );

      os.printf( "</td>\n</tr>\n" );
    }
    for ( resource_e i = RESOURCE_NONE; ++i < RESOURCE_MAX; )
    {
      if ( p.resources.max[ i ] > 0 )
        os.format( R"(<tr class="right"><th class="left">{}</th>)"
                   "<td></td>"
                   "<td></td>"
                   "<td>{:.0f}</td>"
                   "<td>{:.0f}</td>"
                   "<td>{:.0f}</td></tr>\n",
                   util::inverse_tokenize( util::resource_type_string( i ) ), buffed_stats.resource[ i ],
                   p.resources.max[ i ], 0.0 );
    }
    if ( buffed_stats.spell_power > 0 )
    {
      os.format( R"(<tr class="right"><th class="left">Spell Power</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td></tr>\n",
                 buffed_stats.spell_power,
                 p.composite_total_spell_power( SCHOOL_MAX ),
                 p.initial.stats.spell_power );
    }
    if ( p.composite_melee_crit_chance() == p.composite_spell_crit_chance() )
    {
      os.format( R"(<tr class="right"><th class="left">Crit</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.attack_crit_chance,
                 100 * p.composite_melee_crit_chance(),
                 p.composite_melee_crit_rating() );
    }
    else
    {
      os.format( R"(<tr class="right"><th class="left">Melee Crit</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n"
                 R"(<tr class="right"><th class="left">Spell Crit</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.attack_crit_chance,
                 100 * p.composite_melee_crit_chance(),
                 p.composite_melee_crit_rating(),
                 100 * buffed_stats.spell_crit_chance,
                 100 * p.composite_spell_crit_chance(),
                 p.composite_spell_crit_rating() );
    }
    if ( p.composite_melee_haste() == p.composite_spell_haste() )
    {
      os.format( R"(<tr class="right"><th class="left">Haste</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * ( 1 / buffed_stats.attack_haste - 1 ),  // Melee/Spell haste have been merged into a single stat.
                 100 * ( 1 / p.composite_melee_haste() - 1 ),
                 p.composite_melee_haste_rating() );
    }
    else
    {
      os.format( R"(<tr class="right"><th class="left">Melee Haste</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n"
                 R"(<tr class="right"><th class="left">Spell Haste</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * ( 1 / buffed_stats.attack_haste - 1 ),
                 100 * ( 1 / p.composite_melee_haste() - 1 ),
                 p.composite_melee_haste_rating(),
                 100 * ( 1 / buffed_stats.spell_haste - 1 ),
                 100 * ( 1 / p.composite_spell_haste() - 1 ),
                 p.composite_spell_haste_rating() );
    }
    if ( p.composite_spell_cast_speed() != p.composite_spell_haste() )
    {
      os.format( R"(<tr class="right"><th class="left">Spell Cast Speed</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td>"
                 "</tr>\n",
                 100 * ( 1 / buffed_stats.spell_cast_speed - 1 ),
                 100 * ( 1 / p.composite_spell_cast_speed() - 1 ),
                 p.composite_spell_haste_rating() );
    }
    if ( p.composite_melee_auto_attack_speed() != p.composite_melee_haste() )
    {
      os.format( R"(<tr class="right"><th class="left">Swing Speed</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td>"
                 "</tr>\n",
                 100 * ( 1 / buffed_stats.auto_attack_speed - 1 ),
                 100 * ( 1 / p.composite_melee_auto_attack_speed() - 1 ),
                 p.composite_melee_haste_rating() );
    }
    os.format( R"(<tr class="right"><th class="left">Versatility</th>)"
               "<td></td>"
               "<td></td>"
               "<td>{:.2f}%</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.0f}</td></tr>\n",
               100 * buffed_stats.damage_versatility,
               100 * p.composite_damage_versatility(),
               p.composite_damage_versatility_rating() );
    if ( p.primary_role() == ROLE_TANK )
    {
      os.format( R"(<tr class="right"><th class="left">Mitigation Versatility</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.mitigation_versatility,
                 100 * p.composite_mitigation_versatility(),
                 p.composite_mitigation_versatility_rating() );
    }
    if ( buffed_stats.manareg_per_second > 0 )
    {
      os.format( R"(<tr class="right"><th class="left">Mana Regen</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>0</td></tr>\n",
                 buffed_stats.manareg_per_second,
                 p.resource_regen_per_second( RESOURCE_MANA ) );
    }
    if ( buffed_stats.attack_power > 0 )
    {
      os.format( R"(<tr class="right"><th class="left">Attack Power</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td></tr>\n",
                 buffed_stats.attack_power,
                 p.composite_melee_attack_power() * p.composite_attack_power_multiplier(),
                 p.initial.stats.attack_power );
    }
    os.format( R"(<tr class="right"><th class="left">Mastery</th>)"
               "<td></td>"
               "<td></td>"
               "<td>{:.2f}%</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.0f}</td></tr>\n",
               100.0 * buffed_stats.mastery_value,
               100.0 * p.cache.mastery_value(), p.composite_mastery_rating() );
    if ( buffed_stats.mh_attack_expertise > 7.5 )
    {
      if ( p.dual_wield() )
      {
        os.format( R"(<tr class="right"><th class="left">Expertise</th>)"
                   "<td></td>"
                   "<td></td>"
                   "<td>{:.2f}% / {:.2f}%</td>"
                   "<td>{:.2f}% / {:.2f}%</td>"
                   "<td>{:.0f}</td></tr>\n",
                   100 * buffed_stats.mh_attack_expertise,
                   100 * buffed_stats.oh_attack_expertise,
                   100 * p.composite_melee_expertise( &( p.main_hand_weapon ) ),
                   100 * p.composite_melee_expertise( &( p.off_hand_weapon ) ),
                   p.composite_expertise_rating() );
      }
      else
      {
        os.format( R"(<tr class="right"><th class="left">Expertise</th>)"
                   "<td></td>"
                   "<td></td>"
                   "<td>{:.2f}%</td>"
                   "<td>{:.2f}%</td>"
                   "<td>{:.0f}</td></tr>\n",
                   100 * buffed_stats.mh_attack_expertise,
                   100 * p.composite_melee_expertise( &( p.main_hand_weapon ) ),
                   p.composite_expertise_rating() );
      }
    }
    os.format( R"(<tr class="right"><th class="left">Armor</th>)"
               "<td></td>"
               "<td></td>"
               "<td>{:.0f}</td>"
               "<td>{:.0f}</td>"
               "<td>{:.0f}</td></tr>\n",
               buffed_stats.armor,
               p.composite_armor(), p.initial.stats.armor );
    if ( buffed_stats.bonus_armor > 0 )
    {
      os.format( R"(<tr class="right"><th class="left">Bonus Armor</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td></tr>\n",
                 buffed_stats.bonus_armor,
                 p.composite_bonus_armor(),
                 p.initial.stats.bonus_armor );
    }
    if ( buffed_stats.run_speed > 0 )
    {
      os.format( R"(<tr class="right"><th class="left">Run Speed</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td>"
                 "<td>{:.0f}</td></tr>\n",
                 buffed_stats.run_speed,
                 p.composite_run_speed(),
                 p.composite_speed_rating() );
    }
    if ( buffed_stats.leech > 0 )
    {
      os.format( R"(<tr class="right"><th class="left">Leech</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.leech,
                 100 * p.composite_leech(),
                 p.composite_leech_rating() );
    }
    if ( buffed_stats.corruption != 0 )
    {
      os.format( R"(<tr class="right"><th class="left">Total Corruption</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.0f} ({:.0f} - {:.0f})</td>"
                 "<td>{:.0f} ({:.0f} - {:.0f})</td>"
                 "<td>{:.0f}, {:.0f}</td></tr>\n",
                 buffed_stats.corruption - buffed_stats.corruption_resistance,
                 buffed_stats.corruption, buffed_stats.corruption_resistance,
                 p.composite_total_corruption(),
                 p.composite_corruption(), p.composite_corruption_resistance(),
                 p.composite_corruption_rating(), p.composite_corruption_resistance_rating() );
    }
    if ( p.primary_role() == ROLE_TANK )
    {
      if ( buffed_stats.avoidance > 0 )
      {
        os.format( R"(<tr class="right"><th class="left">Avoidance</th>)"
                   "<td></td>"
                   "<td></td>"
                   "<td>{:.0f}</td>"
                   "<td>{:.0f}</td>"
                   "<td>{:.0f}</td></tr>\n",
                   buffed_stats.avoidance,
                   p.composite_avoidance(),
                   p.composite_avoidance_rating() );
      }
      os.format( R"(<tr class="right"><th class="left">Tank-Miss</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.miss,
                 100 * ( p.cache.miss() ),
                 0.0 );
      os.format( R"(<tr class="right"><th class="left">Tank-Dodge</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.dodge,
                 100 * ( p.composite_dodge() ),
                 p.composite_dodge_rating() );
      os.format( R"(<tr class="right"><th class="left">Tank-Parry</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.parry,
                 100 * ( p.composite_parry() ),
                 p.composite_parry_rating() );
      os.format( R"(<tr class="right"><th class="left">Tank-Block</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.block,
                 100 * p.composite_block(),
                 p.composite_block_rating() );
      os.format( R"(<tr class="right"><th class="left">Tank-Crit</th>)"
                 "<td></td>"
                 "<td></td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.0f}</td></tr>\n",
                 100 * buffed_stats.crit,
                 100 * p.cache.crit_avoidance(),
                 0.0 );
    }
    os << "</tbody>\n"
       << "</table>\n"
       << "</div>\n"
       << "</div>\n";
  }
}

// print_html_talents_player ================================================

std::string base64_to_url( std::string_view s )
{
  std::string str( s );
  util::replace_all( str, "+", "%2B" );
  util::replace_all( str, "/", "%2F" );
  return str;
}

// TODO: update once TWW trees are finalized
int raidbots_talent_render_width( specialization_e spec, int height, bool mini = false )
{
  switch ( spec )
  {
    // narrower trees
    case HUNTER_BEAST_MASTERY: return static_cast<int>( mini ? height * 1.45 : height * 1.60 );
    // wider trees
    case DRUID_RESTORATION:    return static_cast<int>( mini ? height * 1.80 : height * 1.95 );
    // default size
    default:                   return static_cast<int>( mini ? height * 1.60 : height * 1.75 );
  }
}

std::string raidbots_domain( [[maybe_unused]] bool ptr )
{
#if SC_BETA
  return "mimiron";
#else
  return ptr ? "mimiron" : "www";
#endif
}

std::string raidbots_talent_render_src( std::string_view talent_str, unsigned level, int width, bool mini, bool ptr )
{
  return fmt::format( "https://{}.raidbots.com/simbot/render/talents/{}?bgcolor=160f0b&level={}&width={}{}",
                      raidbots_domain( ptr ), base64_to_url( talent_str ), level, width, mini ? "&mini=1" : "" );
}

template <typename T>
void print_html_talent_table( report::sc_html_stream& os, const player_t& p, std::string_view title, size_t points, const T& traits )
{
  os.format( "<table class=\"sc\"><tr><th></th><th>{} Talents [{}]</th></tr>\n", title, points );

  for ( unsigned row = 0; row < traits.size(); row++ )
  {
    os.format( "<tr><th class=\"right\" style=\"width: 1em\">{}</th><td class=\"left\"><ul class=\"float\">\n", row + 1 );

    for ( const auto& [ trait, rank ] : traits[ row ] )
    {
      bool partial = rank != trait->max_ranks;

      os.format( "<li class=\"nowrap{}\">{} [{}]{}</li>\n",
                 partial ? " filler" : "",
                 report_decorators::decorated_spell_data( *p.sim, p.find_spell( trait->id_spell ) ),
                 rank,
                 partial ? "<b>*</b>" : "" );
    }
    os << "</ul></td></tr>\n";
  }
  os << "</table>\n";
}

void print_html_talents( report::sc_html_stream& os, const player_t& p )
{
  if ( !p.collected_data.fight_length.mean() || p.player_traits.empty() )
    return;

  static constexpr unsigned TREE_ROWS = 10;
  static constexpr unsigned HERO_TREE_ROWS = 5;
  using talentrank_t = std::pair<const trait_data_t*, unsigned>;

  std::array<std::vector<talentrank_t>, TREE_ROWS> class_traits;
  std::array<std::vector<talentrank_t>, TREE_ROWS> spec_traits;
  std::map<unsigned, std::array<std::vector<talentrank_t>, HERO_TREE_ROWS>> hero_traits;
  size_t class_points = 0;
  size_t spec_points = 0;
  std::map<unsigned, size_t> hero_points;

  for ( const auto& [ _tree, _id, _rank ] : p.player_traits )
  {
    auto trait = trait_data_t::find( _id, maybe_ptr( p.dbc->ptr ) );

    switch ( _tree )
    {
      case talent_tree::CLASS:
        class_traits.at( trait->row - 1 ).emplace_back( trait, _rank );
        if ( !trait_data_t::is_granted( trait, p.type, p.specialization(), p.is_ptr() ) )
          class_points += _rank;
        break;

      case talent_tree::SPECIALIZATION:
        spec_traits.at( trait->row - 1 ).emplace_back( trait, _rank );
        if ( !trait_data_t::is_granted( trait, p.type, p.specialization(), p.is_ptr() ) )
          spec_points += _rank;
        break;

      case talent_tree::HERO:
        {
          auto id = trait->id_sub_tree;
          if ( range::contains( p.player_sub_trees, id ) ||
                range::contains( p.player_sub_traits, trait->id_trait_node_entry ) )
          {
            hero_traits[ id ].at( trait->row - 1 ).emplace_back( trait, _rank );
            if ( !trait_data_t::is_granted( trait, p.type, p.specialization(), p.is_ptr() ) )
              hero_points[ id ] += _rank;
          }
        }
        break;

      default:
        continue;
    }
  }

  for ( auto &row : class_traits )
    range::sort( row, []( talentrank_t a, talentrank_t b ) { return a.first->col < b.first->col; } );

  for ( auto &row : spec_traits )
    range::sort( row, []( talentrank_t a, talentrank_t b ) { return a.first->col < b.first->col; } );

  for ( auto& [ id, traits ] : hero_traits )
    for ( auto &row : traits )
      range::sort( row, []( talentrank_t a, talentrank_t b ) { return a.first->col < b.first->col; } );

  os << "<div class=\"player-section talents\">\n"
     << "<h3 class=\"toggle\">Talents</h3>\n"
     << "<div class=\"toggle-content hide\">\n";

  auto num_players = p.sim->players_by_name.size();
  if ( num_players == 1 )
  {
    auto w_ = raidbots_talent_render_width( p.specialization(), 600 );
    os.format( R"(<iframe src="{}" width="{}" height="600"></iframe>)",
               raidbots_talent_render_src( p.talents_str, p.true_level, w_, false, p.dbc->ptr ), w_ );

    // Hide the talent table only if the Raidbots talent iframe is present.
    os << "<h3 class=\"toggle\">Talent Tables</h3>\n"
       << "<div class=\"toggle-content hide\">\n";
  }

  if ( range::accumulate( class_traits, 0, &std::vector<talentrank_t>::size ) )
    print_html_talent_table( os, p, util::player_type_string_long( p.type ), class_points, class_traits );

  if ( range::accumulate( spec_traits, 0, &std::vector<talentrank_t>::size ) )
    print_html_talent_table( os, p, util::spec_string_no_class( p ), spec_points, spec_traits );

  if ( !hero_traits.empty() )
  {
    os << "<div class=\"flexwrap\">\n";

    for ( const auto& [ id, traits ] : hero_traits )
      if ( range::accumulate( traits, 0, &std::vector<talentrank_t>::size ) )
        print_html_talent_table( os, p, trait_data_t::get_hero_tree_name( id ), hero_points[ id ], traits );

    os << "</div>\n";
  }

  // Close the talent table div only if it exists.
  if ( num_players == 1 )
    os << "</div>\n";

  os << "</div>\n"
     << "</div>\n";
}

// print_html_player_scale_factor_table =====================================

void print_html_player_scale_factor_table( report::sc_html_stream& os, const sim_t&, const player_t& p,
                                           const player_processed_report_information_t& ri, scale_metric_e sm )
{
  if ( p.is_enemy() )
    return;

  int colspan = static_cast<int>( p.scaling->scaling_stats[ sm ].size() );
  std::vector<stat_e> scaling_stats = p.scaling->scaling_stats[ sm ];

  os << "<table class=\"sc even\">\n";

  os << "<tr>\n"
     << "<th colspan=\"" << util::to_string( 1 + colspan ) << "\" "
     << "class=\"help\" data-help=\"#help-scale-factors\">Scale Factors for "
     << util::encode_html( p.scaling_for_metric( sm ).name ) << "</th>\n"
     << "</tr>\n";

  os << "<tr>\n"
     << "<th></th>\n";

  for ( auto scaling_stat : scaling_stats )
  {
    os.printf( "<th>%s</th>\n", util::stat_type_abbrev( scaling_stat ) );
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
      os.printf( "<td>%.*e</td>\n", p.sim->report_precision, p.scaling->scaling[ sm ].get_stat( stat ) );
    else
      os.printf( "<td>%.*f</td>\n", p.sim->report_precision, p.scaling->scaling[ sm ].get_stat( stat ) );
  }

  if ( p.sim->scaling->scale_lag )
    os.printf( "<td>%.*f</td>\n", p.sim->report_precision, p.scaling->scaling_lag[ sm ] );
  os << "</tr>\n";

  os << "<tr>\n"
     << "<th class=\"left\">Normalized</th>\n";

  for ( const auto& stat : scaling_stats )
  {
    os.printf( "<td>%.*f</td>\n", p.sim->report_precision, p.scaling->scaling_normalized[ sm ].get_stat( stat ) );
  }

  os << "</tr>\n";

  os << "<tr>\n"
     << "<th class=\"left\">Scale Deltas</th>\n";

  for ( const auto& stat : scaling_stats )
  {
    double value = p.sim->scaling->stats->get_stat( stat );
    std::string prefix;
    if ( p.sim->scaling->center_scale_delta == 1 && stat != STAT_SPIRIT && stat != STAT_HIT_RATING &&
         stat != STAT_EXPERTISE_RATING )
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
  {
    if ( std::abs( p.scaling->scaling[ sm ].get_stat( stat ) ) > 1.0e5 )
      os.printf( "<td>%.*e</td>\n", p.sim->report_precision, p.scaling->scaling_error[ sm ].get_stat( stat ) );
    else
      os.printf( "<td>%.*f</td>\n", p.sim->report_precision, p.scaling->scaling_error[ sm ].get_stat( stat ) );
  }

  if ( p.sim->scaling->scale_lag )
    os.printf( "<td>%.*f</td>\n", p.sim->report_precision, p.scaling->scaling_lag_error[ sm ] );

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
  os.printf( "<tr class=\"left\">\n"
             "<th class=\"help\" data-help=\"#help-scale-factor-ranking\">Ranking</th>\n"
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
      double separation = fabs( p.scaling->scaling[ sm ].get_stat( scaling_stats[ i - 1 ] )
                              - p.scaling->scaling[ sm ].get_stat( scaling_stats[ i ] ) );
      double error_est  = sqrt( p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i - 1 ] )
                              * p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i - 1 ] ) / 4
                              + p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i ] )
                              * p.scaling->scaling_compare_error[ sm ].get_stat( scaling_stats[ i ] ) / 4 );
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
    os << "<table class=\"sc\">\n";
    os.printf( "<tr class\"left\">\n"
               "<th>Pawn string</th>\n"
               "<td class=\"filler\">\n"
               "%s\n"
               "</td>\n"
               "</tr>\n",
               ri.gear_weights_pawn_string[ sm ].c_str() );
    os << "</table>\n";
  }
}

// print_html_player_scale_factors ==========================================

void print_html_player_scale_factors( report::sc_html_stream& os,
                                      const player_t& p,
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
           << "<p>Scale Factors generated using less than 10,000 iterations may vary significantly from run to "
              "run.</p>\n<br>";

        // Scale factor warnings:
        if ( sim.scaling->scale_factor_noise > 0 &&
             sim.scaling->scale_factor_noise <
               p.scaling->scaling_lag_error[ default_sm ] / fabs( p.scaling->scaling_lag[ default_sm ] ) )
        {
          os.printf( "<p>Player may have insufficient iterations (%d) to calculate "
                     "scale factor for lag (error is >%.0f%% delta score)</p>\n",
                     sim.iterations,
                     sim.scaling->scale_factor_noise * 100.0 );
        }

        for ( size_t i = 0; i < p.scaling->scaling_stats[ default_sm ].size(); i++ )
        {
          scale_metric_e sm = sim.scaling->scaling_metric;
          double value      = p.scaling->scaling[ sm ].get_stat( p.scaling->scaling_stats[ default_sm ][ i ] );
          double error      = p.scaling->scaling_error[ sm ].get_stat( p.scaling->scaling_stats[ default_sm ][ i ] );

          if ( sim.scaling->scale_factor_noise > 0 && sim.scaling->scale_factor_noise < error / fabs( value ) )
          {
            os.printf( "<p>Player may have insufficient iterations (%d) to calculate "
                       "scale factor for stat %s (error is >%.0f%% delta score)</p>\n",
                       sim.iterations,
                       util::stat_type_string( p.scaling->scaling_stats[ default_sm ][ i ] ),
                       sim.scaling->scale_factor_noise * 100.0 );
          }
        }
        os << "</div>\n";
      }
    }
  }
  os << "</div>\n"
     << "</div>\n";
}

bool print_html_sample_sequence_resource( const player_t& p,
                                          const player_collected_data_t::action_sequence_data_t& data,
                                          resource_e r )
{
  if ( !p.resources.active_resource[ r ] && !p.sim->maximize_reporting )
    return false;

  if ( p.role == ROLE_TANK && r == RESOURCE_HEALTH )
    return true;

  if ( data.resource_snapshot[ r ] < 0 )
    return false;

  if ( p.resources.is_infinite( r ) )
    return false;

  return true;
}

void print_html_sample_sequence_string_entry( report::sc_html_stream& os,
                                              const player_collected_data_t::action_sequence_data_t& data,
                                              const player_t& p,
                                              bool precombat = false )
{
  // Skip waiting on the condensed list
  if ( !data.action )
    return;

  std::string targetname = data.action->harmful ? util::remove_special_chars( data.target->name_str ) : "none";
  std::string time_str;

  if ( precombat )
  {
    time_str = "Pre";
  }
  else
  {
    time_str = fmt::format( "{:d}:{:02d}.{:03d}",
                            static_cast<int>( data.time.total_minutes() ),
                            static_cast<int>( data.time.total_seconds() ) % 60,
                            static_cast<int>( data.time.total_millis() ) % 1000 );
  }

  os.printf( "<span class=\"%s_seq_target_%s\" title=\"[%s] %s%s\n|",
             util::remove_special_chars( p.name_str ).c_str(),
             targetname.c_str(),
             time_str,
             util::remove_special_chars( data.action->name_str ).c_str(),
             ( targetname == "none" ? "" : " @ " + targetname ).c_str() );

  resource_e pr = p.primary_resource();

  if ( print_html_sample_sequence_resource( p, data, pr ) )
  {
    if ( pr == RESOURCE_HEALTH || pr == RESOURCE_MANA )
      os.printf( " %.0f%%", ( data.resource_snapshot[ pr ] / data.resource_max_snapshot[ pr ] ) * 100 );
    else
      os.printf( " %.1f", data.resource_snapshot[ pr ] );

    os.printf( " %s |", util::resource_type_string( pr ) );
  }

  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; ++r )
  {
    if ( print_html_sample_sequence_resource( p, data, r ) && r != pr )
    {
      if ( r == RESOURCE_HEALTH || r == RESOURCE_MANA )
        os.printf( " %.0f%%", ( data.resource_snapshot[ r ] / data.resource_max_snapshot[ r ] ) * 100 );
      else
        os.printf( " %.1f", data.resource_snapshot[ r ] );

      os.printf( " %s |", util::resource_type_string( r ) );
    }
  }

  for ( const auto& b_data : data.buff_list )
  {
    buff_t* buff = b_data.object;
    int stacks   = b_data.value;

    if ( !buff->constant )
    {
      os.printf( "\n%s", util::encode_html( buff->name() ).c_str() );

      if ( stacks > 1 )
        os.printf( "(%d)", stacks );
    }
  }

  os.printf( "\">%c</span>", data.action ? data.action->marker : 'W' );
}
// print_html_sample_sequence_table_entry =====================================

void print_html_sample_sequence_table_entry( report::sc_html_stream& os,
                                             const player_collected_data_t::action_sequence_data_t& data,
                                             const player_t& p,
                                             bool precombat = false )
{
  os << R"(<tr class="left">)";

  if ( precombat )
  {
    os << R"(<td class="right">Pre</td>)";
  }
  else
  {
    os.format( R"(<td class="right">{:d}:{:02d}.{:03d}</td>)",
               static_cast<int>( data.time.total_minutes() ),
               static_cast<int>( data.time.total_seconds() ) % 60,
               static_cast<int>( data.time.total_millis() ) % 1000 );
  }

  if ( data.action )
  {
    os.format( "<td>{}</td>"
               "<td><b>{}</b>{}<br/>[{}]</td>"
               "<td>{}</td>",
               data.action->marker != 0 ? data.action->marker : ' ',
               util::encode_html( data.action->name() ), data.queue_failed ? " (queue failed)" : "",
               data.action->action_list ? util::encode_html( data.action->action_list->name_str ): "unknown",
               util::encode_html( data.target_name ) );
  }
  else
  {
    os.format( "<td>&#160;</td>"
               "<td>Waiting</td>"
               "<td>{:.3f}s</td>",
               data.wait_time.total_seconds() );
  }

  os << "<td>";
  bool first    = true;
  resource_e pr = p.primary_resource();

  if ( print_html_sample_sequence_resource( p, data, pr ) )
  {
    if ( first )
      first = false;

    os.format( " {:.1f}/{:.0f}&#160;<b>{:.0f}%</b>&#160;{}",
               data.resource_snapshot[ pr ],
               data.resource_max_snapshot[ pr ],
               data.resource_snapshot[ pr ] / data.resource_max_snapshot[ pr ] * 100.0,
               util::resource_type_abbrev( pr ) );
  }

  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; ++r )
  {
    if ( print_html_sample_sequence_resource( p, data, r ) && r != pr )
    {
      if ( first )
        first = false;
      else
        os << "<br/>";

      os.format( " {:.1f}/{:.0f}&#160;<b>{:.0f}%</b>&#160;{}",
                 data.resource_snapshot[ r ],
                 data.resource_max_snapshot[ r ],
                 data.resource_snapshot[ r ] / data.resource_max_snapshot[ r ] * 100.0,
                 util::resource_type_abbrev( r ) );
    }
  }
  os << "</td>\n";

  os << "<td>";
  first = true;
  for ( const auto& b_data : data.buff_list )
  {
    buff_t* buff = b_data.object;
    int stacks   = b_data.value;

    if ( !buff->constant )
    {
      if ( first )
        first = false;
      else
        os << ", ";

      os.format( "{}", util::encode_html( buff->name() ) );
      if ( stacks > 1 )
        os.format( "({:d})", stacks );
    }
  }
  os << "</td>\n"
     << "</tr>\n";
}

// print_html_player_action_priority_list =====================================

void print_html_player_action_priority_list( report::sc_html_stream& os, const player_t& p )
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

      std::string als = util::encode_html(
        ( alist->name_str == "default" ) ? "Default action list" : ( "actions." + alist->name_str ).c_str() );

      if ( !alist->action_list_comment_str.empty() )
        als += "&#160;<small><em>" +
               util::encode_html( alist->action_list_comment_str.c_str() ) +
               "</em></small>";
      os.printf(
          "<table class=\"sc even\">\n"
          "<thead>\n"
          "<tr>\n"
          "<th class=\"right\"></th>\n"
          "<th class=\"right\"></th>\n"
          "<th class=\"left\">%s</th>\n"
          "</tr>\n"
          "<tr>\n"
          "<th class=\"right\">#</th>\n"
          "<th class=\"right\">count</th>\n"
          "<th class=\"left\">action,conditions</th>\n"
          "</tr>\n"
          "</thead>\n",
          als.c_str() );
    }

    if ( !alist->used )
      continue;

    os << "<tr>\n";
    std::string as = util::encode_html( a->signature->action_.c_str() );
    if ( !a->signature->comment_.empty() )
      as += "<br/><small><em>" +
            util::encode_html( a->signature->comment_.c_str() ) +
            "</em></small>";

    os.printf(
        "<td class=\"right\">%c</td>\n"
        "<td class=\"left\">%.2f</td>\n"
        "<td class=\"left\">%s</td>\n"
        "</tr>\n",
        a->marker ? a->marker : ' ',
        a->total_executions / static_cast<double>( sim.single_actor_batch
                                                     ? a -> player -> collected_data.total_iterations + sim.threads
                                                     : sim.iterations ),
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

    targets.emplace_back("none" );
    if ( p.target )
    {
      targets.emplace_back(p.target->name() );
    }

    for ( const auto& sequence_data : p.collected_data.action_sequence )
    {
      if ( !sequence_data.action || !sequence_data.action->harmful )
        continue;
      bool found = false;
      for ( const auto& target : targets )
      {
        if ( target == sequence_data.target->name() )
        {
          found = true;
          break;
        }
      }
      if ( !found )
        targets.emplace_back(sequence_data.target->name() );
    }

    // Sample Sequence (text string)

    os << "<div class=\"subsection subsection-small\">\n"
       << "<h4>Sample Sequence</h4>\n"
       << "<div class=\"force-wrap mono\">\n";

    os << "<style type=\"text/css\" media=\"all\" scoped>\n";

    char colors[ 12 ][ 4 ] = { "999", "fff", "f55", "5f5", "55f", "ff5", "5ff", "f99", "9f9", "99f", "ff9", "9ff" };

    int j = 0;

    for ( const auto& target : targets )
    {
      if ( j == 12 )
        j = 2;
      os.printf( ".%s_seq_target_%s { color: #%s; }\n", util::remove_special_chars( p.name_str ).c_str(),
                 util::remove_special_chars( target ).c_str(), colors[ j ] );
      j++;
    }

    os << "</style>\n";

    for ( const auto& sequence_data : p.collected_data.action_sequence_precombat )
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
    os << "<table class=\"sc\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th>Time</th>\n"
       << "<th>#</th>\n"
       << "<th>Name [List]</th>\n"
       << "<th>Target</th>\n"
       << "<th>Resources</th>\n"
       << "<th>Buffs</th>\n"
       << "</tr>\n"
       << "</thead>\n";

    os << "<tbody>\n";
    for ( const auto& sequence_data : p.collected_data.action_sequence_precombat )
    {
      print_html_sample_sequence_table_entry( os, sequence_data, p, true );
    }

    for ( const auto& sequence_data : p.collected_data.action_sequence )
    {
      print_html_sample_sequence_table_entry( os, sequence_data, p );
    }
    os << "</tbody>\n";

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

void print_html_player_statistics( report::sc_html_stream& os, const player_t& p,
                                   const player_processed_report_information_t& )
{
  // Statistics & Data Analysis

  os << "<div class=\"player-section analysis\">\n"
        "<h3 class=\"toggle\">Statistics & Data Analysis</h3>\n"
        "<div class=\"toggle-content hide\">\n"
        "<table class=\"sc stripebody\">\n";

  report_helper::print_html_sample_data( os, p, p.collected_data.fight_length, "Fight Length" );
  report_helper::print_html_sample_data( os, p, p.collected_data.dps, "DPS" );
  report_helper::print_html_sample_data( os, p, p.collected_data.prioritydps, "Priority Target DPS" );
  report_helper::print_html_sample_data( os, p, p.collected_data.dpse, "DPS(e)" );
  report_helper::print_html_sample_data( os, p, p.collected_data.dmg, "Damage" );
  report_helper::print_html_sample_data( os, p, p.collected_data.dtps, "DTPS" );
  report_helper::print_html_sample_data( os, p, p.collected_data.hps, "HPS" );
  report_helper::print_html_sample_data( os, p, p.collected_data.hpse, "HPS(e)" );
  report_helper::print_html_sample_data( os, p, p.collected_data.heal, "Heal" );
  report_helper::print_html_sample_data( os, p, p.collected_data.htps, "HTPS" );

  for ( const auto& sample_data : p.sample_data_list )
  {
    report_helper::print_html_sample_data( os, p, *sample_data, util::encode_html( sample_data->name_str ) );
  }

  os << "</table>\n"
        "</div>\n"
        "</div>\n";
}

// print_html_player_statistics =============================================

void print_html_player_mixin_reports( report::sc_html_stream& os, const player_t& p )
{
  for ( const std::unique_ptr<player_report_extension_t> &mixin_report : p.mixin_reports )
    mixin_report->html_customsection( os );
}

std::string find_matching_decorator( const player_t& p, std::string_view n )
{
  std::string n_token = util::tokenize_fn( n );

  const auto* action    = p.find_action( n );
  if ( !action ) action = p.find_action( n_token );
  if ( action )
    return report_decorators::decorated_action( *action );

  const auto* buff  = buff_t::find( const_cast<player_t*>( &p ), n );
  if ( !buff ) buff = buff_t::find( const_cast<player_t*>( &p ), n_token );
  if ( buff )
    return report_decorators::decorated_buff( *buff );

  auto spell                = static_cast<const spell_data_t*>( p.find_talent_spell( talent_tree::CLASS, n_token, p.specialization(), true ) );
  if ( !spell->ok() ) spell = static_cast<const spell_data_t*>( p.find_talent_spell( talent_tree::SPECIALIZATION, n_token, p.specialization(), true ) );
  if ( !spell->ok() ) spell = static_cast<const spell_data_t*>( p.find_talent_spell( talent_tree::HERO, n ) );
  if ( !spell->ok() ) spell = p.find_specialization_spell( n );
  if ( !spell->ok() ) spell = p.find_specialization_spell( n_token );
  if ( !spell->ok() ) spell = p.find_class_spell( n );
  if ( !spell->ok() ) spell = p.find_class_spell( n_token );
  if ( !spell->ok() ) spell = p.find_runeforge_legendary( n );
  if ( !spell->ok() ) spell = p.find_conduit_spell( n );
  if ( !spell->ok() ) spell = p.find_conduit_spell( n_token );
  if ( spell->ok() )
    return report_decorators::decorated_spell_data( *p.sim, spell );

  return util::encode_html( n );
}

void print_html_gain( report::sc_html_stream& os, const player_t& p, const gain_t& g,
                      std::array<double, RESOURCE_MAX>& total_gains, bool report_overflow = true )
{
  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( g.actual[ i ] != 0 || g.overflow[ i ] != 0 )
    {
      os.format( R"(<tr class="right"><td class="left nowrap">{}</td><td class="left nowrap">{}</td>)"
                 "<td>{:.2Lf}</td>"
                 "<td>{:.2Lf}</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2Lf}</td>",
                 find_matching_decorator( p, g.name() ),
                 util::inverse_tokenize( util::resource_type_string( i ) ),
                 g.count[ i ],
                 g.actual[ i ],
                 ( g.actual[ i ] ? g.actual[ i ] / total_gains[ i ] * 100.0 : 0.0 ),
                 g.actual[ i ] / g.count[ i ] );

      if ( report_overflow )
      {
        os.format( "<td>{:.2Lf}</td>"
                   "<td>{:.2f}%</td>",
                   g.overflow[ i ],
                   100.0 * g.overflow[ i ] / ( g.actual[ i ] + g.overflow[ i ] ) );
      }
      os << "</tr>\n";
    }
  }
}

void get_total_player_gains( const player_t& p, std::array<double, RESOURCE_MAX>& total_gains )
{
  for ( const auto& gain : p.gain_list )
  {
    // Wish std::array had += operator overload!
    for ( size_t j = 0; j < total_gains.size(); ++j )
      total_gains[ j ] += gain->actual[ j ];
  }
}

void print_html_resource_gains_table( report::sc_html_stream& os, const player_t& p )
{
  std::array<double, RESOURCE_MAX> total_player_gains = std::array<double, RESOURCE_MAX>();
  get_total_player_gains( p, total_player_gains );

  os << "<table class=\"sc sort even\">\n"
      << "<thead>\n"
      << "<tr>\n";

  sorttable_header( os, "Gains", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_LEFT );
  sorttable_header( os, "Type", SORT_FLAG_ASC | SORT_FLAG_ALPHA );
  sorttable_header( os, "Count" );
  sorttable_header( os, "Total" );
  sorttable_header( os, "Tot%" );
  sorttable_header( os, "Avg" );
  sorttable_header( os, "Overflow" );
  sorttable_header( os, "Ovr%" );

  os << "</tr>\n"
      << "<tr>\n"
      << "<th colspan=\"8\" class=\"left name\">" << util::encode_html( p.name() ) << "</th>\n"
      << "</tr>\n"
      << "</thead>\n";

  os << "<tbody>\n";
  for ( const auto& gain : p.gain_list )
  {
    print_html_gain( os, p, *gain, total_player_gains );
  }

  for ( const auto& pet : p.pet_list )
  {
    if ( pet->collected_data.fight_length.mean() <= 0 )
      continue;
    bool first = true;
    std::array<double, RESOURCE_MAX> total_pet_gains = std::array<double, RESOURCE_MAX>();
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
          os << "<tr class=\"petrow\">\n"
              << "<th colspan=\"8\" class=\"left small\">pet - " << report_decorators::decorated_npc( *pet ) << "</th>\n"
              << "</tr>\n";
        }
      }
      print_html_gain( os, p, *gain, total_pet_gains );
    }
  }
  os << "</tbody>\n"
     << "</table>\n";
}

void get_total_player_usage( const player_t& p, std::array<double, RESOURCE_MAX>& total_usage )
{
  for ( const auto& stat : p.stats_list )
    for ( size_t j = 0; j < total_usage.size(); ++j )
      total_usage[ j ] += stat->resource_gain.actual[ j ];
}

void print_html_resource_usage_table( report::sc_html_stream& os, const player_t& p )
{
  std::array<double, RESOURCE_MAX> total_player_usage = std::array<double, RESOURCE_MAX>();
  get_total_player_usage( p, total_player_usage );

  os << "<table class=\"sc sort even\">\n"
    << "<thead>\n"
    << "<tr>\n";

  sorttable_header( os, "Usage", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_LEFT );
  sorttable_header( os, "Type", SORT_FLAG_ASC | SORT_FLAG_ALPHA );
  sorttable_header( os, "Count" );
  sorttable_header( os, "Total" );
  sorttable_header( os, "Tot%" );
  sorttable_header( os, "Avg" );
  sorttable_header( os, "RPE" );
  sorttable_header( os, "APR" );

  os << "</tr>\n"
     << "<tr>\n"
     << "<th <th colspan=\"8\" class=\"left name\">" << util::encode_html( p.name() ) << "</th>\n"
     << "</tr>\n"
     << "</thead>\n";

  os << "<tbody>\n";
  for ( const auto& stat : p.stats_list )
  {
    if ( stat->rpe_sum > 0 )
    {
      print_html_action_resource( os, *stat, total_player_usage );
    }
  }

  for ( const auto& pet : p.pet_list )
  {
    std::array<double, RESOURCE_MAX> total_pet_usage = std::array<double, RESOURCE_MAX>();
    get_total_player_usage( *pet, total_pet_usage );

    bool first = true;

    for ( const auto& stat : pet->stats_list )
    {
      if ( stat->rpe_sum > 0 )
      {
        if ( first )
        {
          first = false;
          fmt::print(os, "<tr class=\"petrow\">\n");
          fmt::print(os, "<th <th colspan=\"8\" class=\"left small\">pet - {}</th>\n", report_decorators::decorated_npc( *pet ));
          fmt::print(os, "</tr>\n");

        }
        print_html_action_resource( os, *stat, total_pet_usage );
      }
    }
  }
  os << "</tbody>\n"
     << "</table>\n";
}

void print_html_resource_changes_table( report::sc_html_stream& os, const player_t& p )
{
  os << "<table class=\"sc even\">\n"
     << "<thead>\n"
     << "<tr>\n"
     << "<th class=\"left\">Change</th>\n"
     << "<th>Start</th>\n"
     << "<th>Gain/s</th>\n"
     << "<th>Loss/s</th>\n"
     << "<th>Overflow</th>\n"
     << "<th>End (Avg)</th>\n"
     << "<th>Min</th>\n"
     << "<th>Max</th>\n"
     << "</tr>\n"
     << "</thead>\n";

  os << "<tbody>\n";
  for ( size_t i = 0, end = p.collected_data.resource_lost.size(); i < end; ++i )
  {
    if ( i >= p.collected_data.combat_start_resource.size() )
      continue;

    auto rt = static_cast<resource_e>( i );

    if ( p.collected_data.resource_lost[ rt ].mean() <= 0 )
      continue;

    os.format( R"(<tr class="right"><td class="left">{}</td>)"
               "<td>{:.1Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.2Lf}</td>"
               "<td>{:.1Lf}</td>"
               "<td>{:.1Lf}</td>"
               "<td>{:.1Lf}</td>"
               "<td>{:.1Lf}</td></tr>\n",
               util::inverse_tokenize( util::resource_type_string( rt ) ),
               p.collected_data.combat_start_resource[ rt ].mean(),
               p.collected_data.resource_gained[ rt ].mean() / p.collected_data.fight_length.mean(),
               p.collected_data.resource_lost[ rt ].mean() / p.collected_data.fight_length.mean(),
               p.collected_data.resource_overflowed[ rt ].mean(),
               p.collected_data.combat_end_resource[ rt ].mean(),
               p.collected_data.combat_end_resource[ rt ].min(),
               p.collected_data.combat_end_resource[ rt ].max() );
  }
  os << "</tbody>\n"
     << "</table>\n";
}

// print_html_player_resources ==============================================

void print_html_player_resources( report::sc_html_stream& os, const player_t& p )
{
  // Resources Section
  os << "<div class=\"player-section gains\">\n"
     << "<h3 class=\"toggle open\">Resources</h3>\n"
     << "<div class=\"toggle-content flexwrap\">\n";

  int count_gains   = 0;
  int count_usage   = 0;
  int count_changes = 0;

  for ( const auto& g : p.gain_list )
  {
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; r++ )
    {
      if ( g->actual[ r ] != 0 || g->overflow[ r ] != 0 )
        count_gains++;
    }
  }

  for ( const auto& s : p.stats_list )
  {
    if ( s->rpe_sum > 0 )
      count_usage++;
  }

  for ( const auto& d : p.collected_data.resource_lost )
  {
    if ( d.mean() > 0.0 )
      count_changes++;
  }

  if ( count_gains )
    print_html_resource_gains_table( os, p );

  if ( count_gains <= count_usage + 1 )
  {
    if ( count_changes )
      print_html_resource_changes_table( os, p );

    os << "<div>";

    if ( count_usage )
      print_html_resource_usage_table( os, p );
  }
  else
  {
    os << "<div>";

    if ( count_usage )
      print_html_resource_usage_table( os, p );
    if ( count_changes )
      print_html_resource_changes_table( os, p );
  }

  os << "</div><div class=\"column-charts\">\n"; // Open DIV for charts

  for ( resource_e r = RESOURCE_MAX; --r > RESOURCE_NONE; )
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
        highchart::build_id( p, std::string( "resource_gain_" ) + util::resource_type_string( r ) ), *p.sim );
      if ( chart::generate_gains( pc, p, r ) )
      {
        os << pc.to_target_div();
        p.sim->add_chart_data( pc );
      }
    }
  }

  for ( auto it = p.collected_data.resource_timelines.rbegin(); it != p.collected_data.resource_timelines.rend(); ++it )
  {
    auto& timeline = *it;

    if ( p.resources.max[ timeline.type ] == 0 )
      continue;

    if ( timeline.timeline.mean() == 0 )
      continue;

    if ( !p.resources.is_active( timeline.type ) )  // don't display disabled resources
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

    highchart::time_series_t ts( highchart::build_id( p, "resource_" + resource_str ), *p.sim );
    chart::generate_actor_timeline( ts, p, resource_str, color::resource_color( timeline.type ), timeline.timeline );
    ts.set_mean( timeline.timeline.mean() );

    os << ts.to_target_div();
    p.sim->add_chart_data( ts );

    if ( p.is_enemy() && timeline.type == RESOURCE_HEALTH )
    {
      highchart::time_series_t pct_ts( highchart::build_id( p, "resource_health_pct" ), *p.sim );
      chart::generate_actor_timeline( pct_ts, p, "health_pct", color::resource_color( RESOURCE_HEALTH ), p.collected_data.health_pct );
      pct_ts.set_mean( p.collected_data.health_pct.mean() );

      os << pct_ts.to_target_div();
      p.sim->add_chart_data( pct_ts );
    }
  }

  if ( p.primary_role() == ROLE_TANK && !p.is_enemy() )  // Experimental, restrict to tanks for now
  {
    highchart::time_series_t chart( highchart::build_id( p, "health_change" ), *p.sim );
    chart::generate_actor_timeline( chart, p, "Health Change", color::resource_color( RESOURCE_HEALTH ),
                                    p.collected_data.health_changes.merged_timeline );
    chart.set_mean( p.collected_data.health_changes.merged_timeline.mean() );

    os << chart.to_target_div();
    p.sim->add_chart_data( chart );

    sc_timeline_t sliding_average_tl;
    p.collected_data.health_changes.merged_timeline.build_sliding_average_timeline( sliding_average_tl, 6 );
    highchart::time_series_t chart2( highchart::build_id( p, "health_change_ma" ), *p.sim );
    chart::generate_actor_timeline( chart2, p, "Health Change (moving average, 6s window)",
                                    color::resource_color( RESOURCE_HEALTH ), sliding_average_tl );
    chart2.set_mean( sliding_average_tl.mean() );
    chart2.set_yaxis_title( "Moving Avg Health Change" );

    os << chart2.to_target_div();
    p.sim->add_chart_data( chart2 );
  }

  os << "</div>\n"; // Close DIV for charts

  os << "</div>\n"
     << "</div>\n";
}

// print_html_player_charts =================================================

void print_html_player_charts( report::sc_html_stream& os, const player_t& p,
                               const player_processed_report_information_t& )
{
  if ( p.stats_list.empty() )
    return;

  os << "<div class=\"player-section\">\n"
     << "<h3 class=\"toggle open\">Charts</h3>\n"
     << "<div class=\"toggle-content column-charts\">\n";

  highchart::time_series_t dps( highchart::build_id( p, "dps" ), *p.sim );
  if ( chart::generate_actor_dps_series( dps, p ) )
  {
    os << dps.to_target_div();
    p.sim->add_chart_data( dps );
  }

  highchart::bar_chart_t dpet( highchart::build_id( p, "dpet" ), *p.sim );
  if ( chart::generate_action_dpet( dpet, p ) )
  {
    os << dpet.to_target_div();
    p.sim->add_chart_data( dpet );
  }

  highchart::pie_chart_t damage_pie( highchart::build_id( p, "dps_sources" ), *p.sim );
  if ( chart::generate_damage_stats_sources( damage_pie, p ) )
  {
    os << damage_pie.to_target_div();
    p.sim->add_chart_data( damage_pie );
  }

  scaling_metric_data_t scaling_data = p.scaling_for_metric( p.sim->scaling->scaling_metric );
  std::string scale_factor_id = "scale_factor_";
  scale_factor_id += util::scale_metric_type_abbrev( scaling_data.metric );
  highchart::bar_chart_t scale_factor( highchart::build_id( p, scale_factor_id ), *p.sim );
  if ( chart::generate_scale_factors( scale_factor, p, scaling_data.metric ) )
  {
    os << scale_factor.to_target_div();
    p.sim->add_chart_data( scale_factor );
  }

  highchart::histogram_chart_t dps_dist( highchart::build_id( p, "dps_dist" ), *p.sim );
  if ( chart::generate_distribution( dps_dist, &p, p.collected_data.dps.distribution,
                                    util::encode_html( p.name_str ) + " DPS", p.collected_data.dps.mean(),
                                    p.collected_data.dps.min(), p.collected_data.dps.max() ) )
  {
    dps_dist.set( "tooltip.headerFormat", "<b>{point.key}</b> DPS<br/>" );
    os << dps_dist.to_target_div();
    p.sim->add_chart_data( dps_dist );
  }

  if ( p.collected_data.timeline_dmg_taken.mean() > 0 )
  {
    highchart::time_series_t dps_taken( highchart::build_id( p, "dps_taken" ), *p.sim );
    sc_timeline_t timeline_dps_taken;
    p.collected_data.timeline_dmg_taken.build_derivative_timeline( timeline_dps_taken );
    dps_taken.set_yaxis_title( "Damage taken per second" );
    dps_taken.set_title( util::encode_html( p.name_str ) + " Damage taken per second" );
    dps_taken.add_simple_series( "area", color::rgb{"FDD017"}, "DPS taken", timeline_dps_taken.data() );
    dps_taken.set_mean( timeline_dps_taken.mean() );

    if ( p.sim->player_no_pet_list.size() > 1 )
    {
      dps_taken.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
    }

    os << dps_taken.to_target_div();
    p.sim->add_chart_data( dps_taken );
  }

  highchart::pie_chart_t time_spent( highchart::build_id( p, "time_spent" ), *p.sim );
  if ( chart::generate_spent_time( time_spent, p ) )
  {
    os << time_spent.to_target_div();
    p.sim->add_chart_data( time_spent );
  }

  highchart::pie_chart_t heal_pie( highchart::build_id( p, "hps_sources" ), *p.sim );
  if ( chart::generate_heal_stats_sources( heal_pie, p ) )
  {
    os << heal_pie.to_target_div();
    p.sim->add_chart_data( heal_pie );
  }

  if ( p.collected_data.hps.mean() > 0 || p.collected_data.aps.mean() > 0 )
  {
    highchart::histogram_chart_t hps_dist( highchart::build_id( p, "hps_dist" ), *p.sim );
    if ( chart::generate_distribution( hps_dist, &p, p.collected_data.hps.distribution,
                                      util::encode_html( p.name_str ) + " HPS", p.collected_data.hps.mean(),
                                      p.collected_data.hps.min(), p.collected_data.hps.max() ) )
    {
      os << hps_dist.to_target_div();
      p.sim->add_chart_data( hps_dist );
    }
  }

  highchart::chart_t scaling_plot( highchart::build_id( p, "scaling_plot" ), *p.sim );
  if ( chart::generate_scaling_plot( scaling_plot, p, p.sim->scaling->scaling_metric ) )
  {
    os << scaling_plot.to_target_div();
    p.sim->add_chart_data( scaling_plot );
  }

  highchart::chart_t reforge_plot( highchart::build_id( p, "reforge_plot" ), *p.sim );
  if ( chart::generate_reforge_plot( reforge_plot, p ) )
  {
    os << reforge_plot.to_target_div();
    p.sim->add_chart_data( reforge_plot );
  }

  for ( const auto& timeline : p.collected_data.stat_timelines )
  {
    if ( timeline.timeline.mean() == 0 )
      continue;

    std::string stat_str = util::stat_type_string( timeline.type );
    highchart::time_series_t stat_timeline( highchart::build_id( p, "stat_" + stat_str ), *p.sim );
    chart::generate_actor_timeline( stat_timeline, p, stat_str, color::stat_color( timeline.type ), timeline.timeline );
    stat_timeline.set_mean( timeline.timeline.mean() );

    os << stat_timeline.to_target_div();
    p.sim->add_chart_data( stat_timeline );
  }

  os << "</div>\n"
     << "</div>\n";
}

void print_html_player_buff_spelldata( report::sc_html_stream& os, const buff_t& b, const spell_data_t& data,
                                       std::string_view data_name )
{
  // Spelldata
  if ( data.ok() )
  {
    // try to find a valid dbc pointer, going player -> source -> sim
    // if everything fails we fallback to a "nil" spelltext_data_t record
    const dbc_t* dbc = nullptr;
    if ( b.player )
      dbc = b.player -> dbc.get();
    if ( dbc == nullptr && b.source )
      dbc = b.source -> dbc.get();
    if ( dbc == nullptr && b.sim )
      dbc = b.sim -> dbc.get();

    auto player_ = b.player && b.player->is_enemy() ? b.source : b.player;

    const auto& spell_text = dbc ? dbc->spell_text( data.id() ) : spelltext_data_t::nil();
    os.printf( "<h4>%s</h4>\n"
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
               "</ul>\n",
               util::encode_html( data_name ).c_str(),
               data.id(),
               util::encode_html( data.name_cstr() ).c_str(),
               player_ ? util::encode_html( report_helper::pretty_spell_text( data, spell_text.tooltip(), *player_ ) ).c_str()
                       : util::encode_html( spell_text.tooltip() ).c_str(),
               player_ ? util::encode_html( report_helper::pretty_spell_text( data, spell_text.desc(), *player_ ) ).c_str()
                       : util::encode_html( spell_text.desc() ).c_str(),
               data.max_stacks(),
               data.duration().total_seconds(),
               data.cooldown().total_seconds(),
               data.proc_chance() * 100 );
  }
}

// This function MUST accept non-player buffs as well!
void print_html_player_buff( report::sc_html_stream& os, const buff_t& b, int report_details, const player_t& p,
                             bool constant_buffs = false )
{
  std::string buff_name;

  if ( b.player && b.player->is_pet() )
    buff_name += util::encode_html( b.player->name_str ) + "&#160;-&#160;";

  if ( b.data().id() )
  {
    if ( constant_buffs )
    {
      if ( auto con = dynamic_cast<const consumable_buff_item_data_t*>( &b ); con && con->item_data )
        buff_name += report_decorators::decorated_item_data( *b.player->sim, *con->item_data );
      else if ( b.item && &b.item->parsed.data != &dbc_item_data_t::nil() )
        buff_name += report_decorators::decorated_item_data( *b.player->sim, b.item->parsed.data );
      else
        buff_name += report_decorators::decorated_buff( b );
    }
    else
    {
      buff_name += report_decorators::decorated_buff( b );
    }
  }
  else
  {
    buff_name += util::encode_html( b.name_reporting() );
  }

  std::string toggle_name = highchart::build_id( b, "_toggle" );
  std::string span_str    = buff_name;

  if ( report_details )
    span_str = "<span id=\"" + toggle_name + "\" class=\"toggle-details\">" + buff_name + "</span>";

  os.format( R"(<tr class="right"><td class="left">{}</td>)", span_str );

  if ( !constant_buffs )
  {
    os.format( "<td>{:.1f}</td>"
               "<td>{:.1f}</td>"
               "<td>{:.1f}s</td>"
               "<td>{:.1f}s</td>"
               "<td>{:.1f}s</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.1f}&#160;({:.1f})</td>"
               "<td>{:.1f}</td>\n",
               b.avg_start.pretty_mean(),
               b.avg_refresh.pretty_mean(),
               b.start_intervals.pretty_mean(),
               b.trigger_intervals.pretty_mean(),
               b.duration_lengths.pretty_mean(),
               b.uptime_pct.pretty_mean(),
               b.benefit_pct.mean(),
               b.avg_overflow_count.mean(), b.avg_overflow_total.mean(),
               b.avg_expire.pretty_mean() );
  }

  os << "</tr>\n";

  if ( report_details )
  {
    const stat_buff_t* stat_buff = dynamic_cast<const stat_buff_t*>( &b );
    const auto* absorb_buff = dynamic_cast<const absorb_buff_t*>( &b );
    const auto* damage_buff = dynamic_cast<const damage_buff_t*>( &b );

    int first_rows    = 2 + ( b.item ? 16 : 15 );  // # of rows in the first column incl 2 for header (buff details)
    int second_rows   = ( b.rppm ? 5 : 0 ) +
                        ( stat_buff ? 2 + ( as<int>( stat_buff->stats.size() ) * 2 ) : 0 ) +
                        ( b.trigger_pct.mean() > 0 ? 5 : 0 );
    int stack_rows    = 2 + as<int>( range::count_if( b.stack_uptime, []( const uptime_simple_t& up ) {
                              return up.uptime_sum.mean() > 0;
                            } ) );
    bool break_first  = first_rows + second_rows > stack_rows;
    bool break_second = !break_first && stack_rows + second_rows > first_rows;

    os << "<tr class=\"details hide\">\n"
       << "<td colspan=\"" << ( b.constant ? 1 : 9 ) << "\" class=\"filler\">\n";

    os << "<div class=\"flex\">\n";  // Wrap everything

    os << "<div>\n";  // First column
    os.format( R"(<h4>Buff Details</h4><ul class="label">)"
               "<li><span>buff initial source:</span>{}</li>"
               "<li><span>cooldown name:</span>{}</li>"
               "<li><span>max_stacks:</span>{:d}</li>"
               "<li><span>base duration:</span>{:.2f}</li>"
               "<li><span>duration modifier:</span>{:.2f}</li>"
               "<li><span>base cooldown:</span>{:.2f}</li>"
               "<li><span>default_chance:</span>{:.2f}%</li>"
               "<li><span>default_value:</span>{:.2f}</li>"
               "<li><span>activated:</span>{}</li>"
               "<li><span>reactable:</span>{}</li>"
               "<li><span>reverse:</span>{}</li>"
               "<li><span>refresh behavior:</span>{}</li>"
               "<li><span>stack behavior:</span>{}</li>"
               "<li><span>tick behavior:</span>{}</li>"
               "<li><span>tick_time behavior:</span>{}</li>"
               "<li><span>period:</span>{:.2f}</li>",
               b.source ? util::encode_html( b.source->name() ) : "",
               util::encode_html( b.cooldown->name_str ),
               b.max_stack(),
               b.base_buff_duration.total_seconds(),
               b.buff_duration_multiplier,
               b.cooldown->duration.total_seconds(),
               b.default_chance * 100,
               b.default_value,
               b.activated ? "true" : "false",
               b.reactable ? "true" : "false",
               b.reverse ? "true" : "false",
               util::buff_refresh_behavior_string( b.refresh_behavior ),
               util::buff_stack_behavior_string( b.stack_behavior ),
               util::buff_tick_behavior_string( b.tick_behavior ),
               util::buff_tick_time_behavior_string( b.tick_time_behavior ),
               b.buff_period == timespan_t::min() ? 0 : b.buff_period.total_seconds() );
    if ( b.item )
      os.format( "<li><span>associated item:</span>{}</li>", util::encode_html( b.item->full_name() ) );

    os << "</ul>\n";

    if ( break_first )  // if first + second rows will overflow past stack rows
    {
      os << "</div>\n"  // Close first column and open second column
         << "<div>\n";
    }

    if ( stat_buff )
    {
      os << R"(<h4>Stat Details</h4><ul class="label">)";
      for ( const auto& stat : stat_buff->stats )
      {
        os.format( "<li><span>stat:</span>{}</li>"
                   "<li><span>amount:</span>{:.2f}</li>",
                   util::stat_type_string( stat.stat ),
                   stat.amount );
      }
      os << "</ul>\n";
    }
    else  // assume static stat_buff_t will never register to stat_pct_buff
    {
      bool show_header = false;

      for ( auto stat = STAT_PCT_BUFF_CRIT; stat < STAT_PCT_BUFF_MAX; stat++ )
      {
        if ( range::contains( p.buffs.stat_pct_buffs[ stat ], &b ) )
        {
          if ( !show_header )
          {
            os << R"(<h4>Stat Details</h4><ul class="label">)";
            show_header = true;
          }

          os.format( "<li><span>stat:</span>{}</li>"
                     "<li><span>amount:</span>{:.2f}%</li>",
                     util::stat_pct_buff_type_string( stat ),
                     b.default_value * ( stat == STAT_PCT_BUFF_MASTERY ? 1.0 : 100.0 ) );
        }
      }

      if ( show_header )
        os << "</ul>\n";
    }

    if ( damage_buff )
    {
      if ( damage_buff->is_stacking && damage_buff->max_stack() > 1 )
      {
        os.format( R"(<h4>Damage Modifiers</h4><ul class="label">)"
                   "<li><span>direct:</span>{:.2f} + {:.2f}/stack</li>"
                   "<li><span>periodic:</span>{:.2f} + {:.2f}/stack</li>"
                   "<li><span>auto_attack:</span>{:.2f} + {:.2f}/stack</li>"
                   "<li><span>crit_chance:</span>{:.2f}</li>"
                   "<li><span>is_stacking:</span>true</li></ul>\n",
                   damage_buff->direct_mod.initial_multiplier, damage_buff->direct_mod.multiplier - 1.0,
                   damage_buff->periodic_mod.initial_multiplier, damage_buff->periodic_mod.multiplier - 1.0,
                   damage_buff->auto_attack_mod.initial_multiplier, damage_buff->auto_attack_mod.multiplier - 1.0,
                   damage_buff->crit_chance_mod.multiplier );
      }
      else
      {
        os.format( R"(<h4>Damage Modifiers</h4><ul class="label">)"
                   "<li><span>direct:</span>{:.2f}</li>"
                   "<li><span>periodic:</span>{:.2f}</li>"
                   "<li><span>auto_attack:</span>{:.2f}</li>"
                   "<li><span>crit_chance:</span>{:.2f}</li>"
                   "<li><span>is_stacking:</span>false</li></ul>\n",
                   damage_buff->direct_mod.multiplier,
                   damage_buff->periodic_mod.multiplier,
                   damage_buff->auto_attack_mod.multiplier,
                   damage_buff->crit_chance_mod.multiplier );
      }
    }

    if ( !constant_buffs )
    {
      if ( b.rppm )
      {
        os.format( R"(<h4>RPPM Details</h4><ul class="label">)"
                   "<li><span>scaling:</span>{}</li>"
                   "<li><span>frequency:</span>{:.2f}</li>"
                   "<li><span>modifier:</span>{:.2f}</li></ul>\n",
                   util::rppm_scaling_string( b.rppm->get_scaling() ),
                   b.rppm->get_frequency(),
                   b.rppm->get_modifier() );
      }

      if ( absorb_buff )
      {
        os.format( R"(<h4>Absorb Details</h4><ul class="label">)"
                   "<li><span>school:</span>{}</li>"
                   "<li><span>high priority:</span>{}</li></ul>\n",
                   util::school_type_string( absorb_buff->absorb_school ),
                   absorb_buff->high_priority ? "yes" : "no" );
      }

      if ( b.trigger_pct.mean() > 0 )
      {
        os.format( R"(<h4>Trigger Details</h4><ul class="label">)"
                   "<li><span>interval_min/max:</span>{:.1f}s&#160;/&#160;{:.1f}s</li>"
                   "<li><span>trigger_min/max:</span>{:.1f}s&#160;/&#160;{:.1f}s</li>"
                   "<li><span>trigger_pct:</span>{:.2f}%</li>"
                   "<li><span>duration_min/max:</span>{:.1f}s&#160;/&#160;{:.1f}s</li>"
                   "<li><span>uptime_min/max:</span>{:.2f}%&#160;/&#160;{:.2f}%</li></ul>\n",
                   b.start_intervals.min(), b.start_intervals.max(),
                   b.trigger_intervals.min(), b.trigger_intervals.max(),
                   b.trigger_pct.mean(),
                   b.duration_lengths.min(), b.duration_lengths.max(),
                   b.uptime_pct.min(), b.uptime_pct.max() );
      }

      if ( break_second )  // if stack rows will overflow past first column
      {
        os << "</div>\n"  //  Close second column and start new column for stacks
           << "<div>\n";
      }

      os << "<h4>Stack Uptimes</h4>\n"
         << "<ul>\n";
      for ( unsigned int j = 0; j < b.stack_uptime.size(); j++ )
      {
        double uptime = b.stack_uptime[ j ].uptime_sum.mean();
        if ( uptime > 0 )
        {
          os.printf( "<li><span class=\"label\">%s_%d:</span>%.2f%%</li>\n",
                     util::encode_html( b.name_str ).c_str(), j, uptime * 100.0 );
        }
      }
      os << "</ul>\n";
    }

    os << "</div>\n";

    os << "<div class=\"shrink\">\n";
    print_html_player_buff_spelldata(os, b, b.data(), "Spelldata" );

    if ( b.get_trigger_data()->ok() && b.get_trigger_data()->id() != b.data().id() )
    {
      print_html_player_buff_spelldata(os, b, *b.get_trigger_data(), "Trigger Spelldata" );
    }
    os << "</div>\n"
       << "</div>\n";  // Close everything

    if ( !b.constant && !b.overridden && b.sim->buff_uptime_timeline && b.uptime_array.mean() != 0 )
    {
      std::string title = b.sim->buff_stack_uptime_timeline ? "Stack Uptime" : "Uptime";
      std::string chart_id = b.source ? highchart::build_id( b, "_uptime" )
                                      : "buff_" + util::remove_special_chars( b.name_str ) + "_actor" +
                                          util::to_string( p.index ) + "_uptime";

      highchart::time_series_t buff_uptime( chart_id, *b.sim );
      buff_uptime.set_yaxis_title( "Average uptime" );
      buff_uptime.set_title( util::encode_html( b.name_str ) + " " + title );
      buff_uptime.add_simple_series( "area", color::rgb{"FF0000"}, title, b.uptime_array.data() );
      buff_uptime.set_mean( b.uptime_array.mean() );

      if ( !b.player || !b.sim->single_actor_batch )
        buff_uptime.set_xaxis_max( b.sim->simulation_length.max() );
      else
        buff_uptime.set_xaxis_max( b.player->collected_data.fight_length.max() );

      buff_uptime.set_toggle_id( toggle_name );
      os << buff_uptime.to_target_div();
      b.sim->add_chart_data( buff_uptime );
    }

    os << "</td>\n"
       << "</tr>\n";
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
  os << "<table class=\"sc sort stripebody\">\n"
     << "<thead>\n"
     << "<tr>\n";

  sorttable_help_header( os, "Dynamic Buffs", "help-dynamic-buffs", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_LEFT );
  sorttable_header( os, "Start" );
  sorttable_header( os, "Refresh" );
  sorttable_header( os, "Interval", SORT_FLAG_ASC );
  sorttable_header( os, "Trigger", SORT_FLAG_ASC );
  sorttable_header( os, "Avg Dur" );
  sorttable_header( os, "Up-Time" );
  sorttable_help_header( os, "Benefit", "help-buff-benefit" );
  sorttable_header( os, "Overflow" );
  sorttable_header( os, "Expiry" );

  os << "</tr>\n"
     << "</thead>\n";

  for ( const auto* b : ri.dynamic_buffs )
  {
     os << "<tbody>\n";
    print_html_player_buff( os, *b, p.sim->report_details, p );
    os << "</tbody>\n";
  }
  os << "</table>\n";

  // constant buffs
  if ( !p.is_pet() && !ri.constant_buffs.empty() )
  {
    os << "<table class=\"sc stripebody\">\n"
       << "<thead>\n"
       << "<tr>\n"
       << "<th class=\"left help\" data-help=\"#help-constant-buffs\">Constant Buffs</th>\n"
       << "</tr>\n"
       << "</thead>\n";

    for ( const auto* b : ri.constant_buffs )
    {
       os << "<tbody>\n";
      print_html_player_buff( os, *b, p.sim->report_details, p, true );
      os << "</tbody>\n";
    }
    os << "</table>\n";
  }

  os << "</div>\n"
     << "</div>\n";
}

void print_html_player_custom_section( report::sc_html_stream& os, const player_t& p,
                                       const player_processed_report_information_t& /*ri*/ )
{
  if ( p.report_extension )
    p.report_extension->html_customsection( os );
}

// print_html_player ========================================================

void print_html_player_description( report::sc_html_stream& os, const player_t& p )
{
  const sim_t& sim = *p.sim;
  bool one_player = sim.players_by_name.size() == 1 && !p.is_enemy() && sim.profilesets->n_profilesets() == 0;

  // Player Description
  os << "<div id=\"player" << p.index << "\" class=\"player section\">\n";

  os << "<h2 id=\"player" << p.index << "toggle\" class=\"toggle";
  if ( one_player )
  {
    os << " open";
  }

  const std::string n = util::encode_html( p.name() );
  if ( ( p.collected_data.dps.mean() >= p.collected_data.hps.mean() && sim.enemy_targets > 1 ) ||
       ( p.primary_role() == ROLE_TANK && sim.enemy_targets > 1 ) )
  {
    os.format( "\">{}&#160;:&#160;{:.0Lf} dps, {:.0Lf} dps to main target",
               n, p.collected_data.dps.mean(), p.collected_data.prioritydps.mean() );
  }
  else if ( p.collected_data.dps.mean() >= p.collected_data.hps.mean() || p.primary_role() == ROLE_TANK )
  {
    os.format( "\">{}&#160;:&#160;{:.0Lf} dps", n, p.collected_data.dps.mean() );
  }
  else
  {
    os.format( "\">{}&#160;:&#160;{:.0Lf} hps ({:.0Lf} aps)",
               n, p.collected_data.hps.mean() + p.collected_data.aps.mean(), p.collected_data.aps.mean() );
  }

  // if player tank, print extra metrics
  if ( p.primary_role() == ROLE_TANK && !p.is_enemy() )
  {
    // print DTPS & HPS
    os.format( ", {:.0Lf} dtps, {:.0Lf} hps ({:.0Lf} aps)",
               p.collected_data.dtps.mean(),
               p.collected_data.hps.mean() + p.collected_data.aps.mean(),
               p.collected_data.aps.mean() );

    os << "\n";
  }
  os << "</h2>\n";

  os << "<div class=\"toggle-content";
  if ( !one_player )
  {
    os << " hide";
  }
  os << "\">\n";

  os << "<ul class=\"params\">\n";
#if SC_BETA
  os << "<li><b>BETA activated</b></li>\n";
#elif SC_USE_PTR
  if ( p.dbc->ptr )
    os << "<li><b>PTR activated</b></li>\n";
#endif
  fmt::print( os, "<li><b>Race:</b> {}</li>\n", util::inverse_tokenize( p.race_str ) );

  const char* pt;
  if ( p.is_pet() )
  {
    pt = util::pet_type_string( p.cast_pet()->pet_type );
  }
  else
  {
    pt = util::player_type_string( p.type );
  }

  fmt::print( os, "<li><b>Class:</b> {}</li>\n", util::inverse_tokenize( pt ) );

  if ( p.specialization() != SPEC_NONE )
  {
    os.printf( "<li><b>Spec:</b> %s</li>\n",
               util::inverse_tokenize( dbc::specialization_string( p.specialization() ) ) );
  }

  std::string timewalk_str = " (";
  if ( sim.timewalk > 0 )
  {
    timewalk_str += util::to_string( p.true_level );
    timewalk_str += ")";
  }
  os.printf( "<li><b>Level:</b> %d%s</li>\n"
             "<li><b>Role:</b> %s</li>\n"
             "<li><b>Position:</b> %s</li>\n"
             "<li><b>Profile Source:</b> %s</li>\n",
             p.level(),
             sim.timewalk > 0 && !p.is_enemy() ? timewalk_str.c_str() : "",
             util::inverse_tokenize( util::role_type_string( p.primary_role() ) ).c_str(),
             util::encode_html( p.position_str ).c_str(),
             util::profile_source_string( p.profile_source_ ) );

  if ( p.covenant && p.covenant->enabled() )
  {
    os.format( "<li><b>Covenant:</b> {}</li>\n",
               util::inverse_tokenize( util::covenant_type_string( p.covenant->type() ) ) );
  }

  os.format("</ul>\n");

  if ( !p.report_information.thumbnail_url.empty() )
  {
    os.printf( "<img src=\"%s\" alt=\"%s\" class=\"player-thumbnail\"/>\n",
               p.report_information.thumbnail_url.c_str(),
               util::remove_special_chars( p.name_str ).c_str() );
  }
}

// print_html_player_results_spec_gear ======================================

void print_html_player_results_spec_gear( report::sc_html_stream& os, const player_t& p )
{
  const sim_t& sim                  = *( p.sim );
  const player_collected_data_t& cd = p.collected_data;

  // Main player table
  os << "<div class=\"player-section results-spec-gear\">\n";

  if ( p.is_pet() )
    os << "<h3 class=\"toggle open\">Results</h3>\n";
  else
    os << "<h3 class=\"toggle open\">Results, Spec and Gear</h3>\n";

  os << "<div class=\"toggle-content\">\n";

  if ( p.sim->players_by_name.size() == 1 && p.is_player() )
  {
    auto w_ = raidbots_talent_render_width( p.specialization(), 125, true );
    os.format(
      R"(<iframe src="{}" width="{}" height="125" style="margin-right: 8px; margin-top: 5px; float: left"></iframe>)",
      raidbots_talent_render_src( p.talents_str, p.true_level, w_, true, p.dbc->ptr ), w_ );

    os << "\n";
  }

  os << "<div class=\"flexwrap\">\n";

  if ( cd.dps.mean() > 0 )
  {
    // Table for DPS
    os << "<table class=\"sc\"><tr>\n"
       << "<th class=\"help\" data-help=\"#help-dps\">DPS</th>\n"
       << "<th class=\"help\" data-help=\"#help-dpse\">DPS(e)</th>\n"
       << "<th class=\"help\" data-help=\"#help-error\">DPS Error</th>\n"
       << "<th class=\"help\" data-help=\"#help-range\">DPS Range</th>\n"
       << "<th class=\"help\" data-help=\"#help-dpr\">DPR</th></tr>\n";

    os << "<tr>\n";

    double dps_range =
        ( cd.dps.percentile( 0.5 + sim.confidence / 2 ) - cd.dps.percentile( 0.5 - sim.confidence / 2 ) );
    double dps_error = sim_t::distribution_mean_error( sim, cd.dps );
    os.format( "<td>{:.1Lf}</td><td>{:.1Lf}</td><td>{:.1Lf} / {:.3f}%</td><td>{:.1Lf} / {:.1f}%</td><td>{:.1Lf}</td>",
               cd.dps.mean(), cd.dpse.mean(),
               dps_error, cd.dps.mean() ? dps_error * 100 / cd.dps.mean() : 0,
               dps_range, cd.dps.mean() ? dps_range / cd.dps.mean() * 100.0 : 0,
               p.dpr );
    // close table
    os << "</tr>\n"
       << "</table>\n";
  }

  // Heal
  if ( cd.hps.mean() > 0 )
  {
    // Table for HPS
    os << "<table class=\"sc\">\n"
       << "<tr>\n";
    os << "<th class=\"help\" data-help=\"#help-hps\">HPS</th>\n"
       << "<th class=\"help\" data-help=\"#help-hpse\">HPS(e)</th>\n"
       << "<th class=\"help\" data-help=\"#help-error\">HPS Error</th>\n"
       << "<th class=\"help\" data-help=\"#help-range\">HPS Range</th>\n"
       << "<th class=\"help\" data-help=\"#help-hpr\">HPR</th>\n";
    os << "</tr>\n"
       << "<tr>\n";
    double hps_range =
      ( cd.hps.percentile( 0.5 + sim.confidence / 2 ) - cd.hps.percentile( 0.5 - sim.confidence / 2 ) );
    double hps_error = sim_t::distribution_mean_error( sim, cd.hps );
    os.format( "<td>{:.1Lf}</td><td>{:.1Lf}</td><td>{:.1Lf} / {:.3f}%</td><td>{:.1Lf} / {:.1f}%</td><td>{:.1Lf}</td>",
               cd.hps.mean(), cd.hpse.mean(),
               hps_error, cd.hps.mean() ? hps_error * 100 / cd.hps.mean() : 0,
               hps_range, cd.hps.mean() ? hps_range / cd.hps.mean() * 100.0 : 0,
               p.hpr );
    // close table
    os << "</tr>\n"
       << "</table>\n";
  }

  os << "</div>\n"
        "<div class=\"flexwrap\">\n";

  // Absorb
  if ( cd.aps.mean() > 0 )
  {
    // Table for APS
    os << "<table class=\"sc\">\n"
       << "<tr>\n";
    os << "<th class=\"help\" data-help=\"#help-aps\">APS</th>\n"
       << "<th class=\"help\" data-help=\"#help-error\">APS Error</th>\n"
       << "<th class=\"help\" data-help=\"#help-range\">APS Range</th>\n"
       << "<th class=\"help\" data-help=\"#help-hpr\">APR</th>\n";
    os << "</tr>\n"
       << "<tr>\n";
    double aps_range =
        ( cd.aps.percentile( 0.5 + sim.confidence / 2 ) - cd.aps.percentile( 0.5 - sim.confidence / 2 ) );
    double aps_error = sim_t::distribution_mean_error( sim, cd.aps );
    os.format( "<td>{:.1Lf}</td><td>{:.1Lf} / {:.3f}%</td><td>{:.1Lf} / {:.1f}%</td><td>{:.1Lf}</td>",
               cd.aps.mean(),
               aps_error, cd.aps.mean() ? aps_error * 100 / cd.aps.mean() : 0,
               aps_range, cd.aps.mean() ? aps_range / cd.aps.mean() * 100.0 : 0,
               p.hpr );
    // close table
    os << "</tr>\n"
       << "</table>\n";
  }

  // Tank table
  if ( p.primary_role() == ROLE_TANK && !p.is_enemy() )
  {
    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th class=\"help\" data-help=\"#help-dtps\">DTPS</th>\n"
       << "<th class=\"help\" data-help=\"#help-error\">DTPS Error</th>\n"
       << "<th class=\"help\" data-help=\"#help-range\">DTPS Range</th>\n"
       << "</tr>\n"
       << "<tr>\n";  // start third row

    double dtps_range = ( cd.dtps.percentile( 0.5 + sim.confidence / 2 ) -
                          cd.dtps.percentile( 0.5 - sim.confidence / 2 ) );
    double dtps_error = sim_t::distribution_mean_error( sim, p.collected_data.dtps );
    os.format( "<td>{:.1Lf}</td><td>{:.2Lf} / {:.2f}%</td><td>{:.0Lf} / {:.1f}%</td>",
               cd.dtps.mean(),
               dtps_error, cd.dtps.mean() ? dtps_error * 100 / cd.dtps.mean() : 0,
               dtps_range, cd.dtps.mean() ? dtps_range / cd.dtps.mean() * 100.0 : 0 );

    // End defensive table
    os << "</tr>\n"
       << "</table>\n";
  }

  // Resources/Activity Table
  if ( !p.is_pet() )
  {
    os << "<table class=\"sc\">\n"
       << "<tr>\n"
       << "<th>Resource</th>\n"
       << "<th class=\"help\" data-help=\"#help-rps-out\">Out</th>\n"
       << "<th class=\"help\" data-help=\"#help-rps-in\">In</th>\n"
       << "<th class=\"help\" data-help=\"#help-waiting\">Waiting</th>\n"
       << "<th class=\"help\" data-help=\"#help-apm\">APM</th>\n"
       << "<th>Active</th>\n"
       << "</tr>\n"
       << "<tr>\n";

    os.format( "<td>{}</td><td>{:.1Lf}</td><td>{:.1Lf}</td><td>{:.2f}%</td><td>{:.1Lf}</td><td>{:.1f}%</td>",
               util::inverse_tokenize( util::resource_type_string( p.primary_resource() ) ),
               p.rps_loss, p.rps_gain,
               cd.fight_length.mean() ? 100.0 * cd.waiting_time.mean() / cd.fight_length.mean() : 0,
               cd.fight_length.mean() ? 60.0 * cd.executed_foreground_actions.mean() / cd.fight_length.mean() : 0,
               sim.simulation_length.mean() ? cd.fight_length.mean() / sim.simulation_length.mean() * 100.0 : 0 );

    os << "</tr>\n"
       << "</table>\n";
  }

  os << "</div>\n";

  // Spec and gear
  if ( !p.is_pet() && !p.is_enemy() )
  {
    os << "<table class=\"sc spec\">\n";

    if ( !p.origin_str.empty() )
    {
      os.format(
        R"(<tr><th class="help" data-help="#help-origin">Origin</th><td class="left"><a href="{}" class="ext">{}</a></td></tr>)",
        p.origin_str, util::encode_html( p.origin_str ) );
    }

    // Talent Hash
    os.format( "<tr class=\"left\"><th>Talent</th><td>{}</td></tr>\n", p.talents_str );

    // Set Bonuses
    if ( p.sets )
    {
      auto bonuses = p.sets->enabled_set_bonus_data();

      if ( !bonuses.empty() )
      {
        int curr_tier = set_bonus_type_e::SET_BONUS_NONE;

        os << "<tr class=\"left\"><th>Set Bonus</th><td><ul class=\"float\">\n";

        for ( auto bonus : bonuses )
        {
          auto enum_id = as<int>( bonus->enum_id );
          if ( curr_tier != enum_id )
          {
            if ( curr_tier != set_bonus_type_e::SET_BONUS_NONE )
              os << "</ul></td></tr>\n<tr class=\"left\"><th></th><td><ul class=\"float\">\n";

            os.format( "<li class=\"nowrap\">{}</li>\n", report_decorators::decorated_set( sim, *bonus ) );

            curr_tier = enum_id;
          }

          os.format( "<li class=\"nowrap\">{} ({}pc)</li>\n",
                     report_decorators::decorated_spell_data( sim, p.find_spell( bonus->spell_id ) ), bonus->bonus );
        }

        os << "</ul></td></tr>\n";
      }
    }

    // Essence
    if ( p.azerite_essence )
    {
      p.azerite_essence->generate_report( os );
    }

    // Azerite
    if ( p.azerite )
    {
      p.azerite->generate_report( os );
    }

    // Covenant, Soulbinds, and Conduits
    if ( p.covenant && p.covenant->enabled() )
    {
      p.covenant->generate_report( os );
    }

    // Runeforge Legendaries
    runeforge::generate_report( p, os );

    // Shards of Domination (9.1)
    unique_gear::shadowlands::items::shards_of_domination::generate_report( p, os );

    // Professions
    if ( !p.professions_str.empty() )
    {
      os << "<tr class=\"left\">\n"
         << "<th>Professions</th>\n"
         << "<td>\n"
         << "<ul class=\"float\">\n";
      for ( profession_e i = PROFESSION_NONE; i < PROFESSION_MAX; ++i )
      {
        if ( p.profession[ i ] <= 0 )
          continue;

        os.format( "<li class=\"nowrap\">{}: {}</li>\n", util::profession_type_string( i ), p.profession[ i ] );
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

  if ( !( s.mask() & mask ) )
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

void output_player_action( report::sc_html_stream& os, unsigned cols, unsigned mask, const stats_t& s,
                           const player_t* actor, int level = 0 )
{
  if (level > 2 )
    return;

  if ( !is_output_stat( mask, level != 0, s ) )
    return;

  if ( level == 0 )
    os << "<tbody>\n";

  print_html_action_info( os, mask, s, cols, actor, level );

  for ( auto child_stats : s.children )
  {
    output_player_action( os, cols, mask, *child_stats, actor, level + 1 );
  }
  if ( level == 0 )
    os << "</tbody>\n";
}

void output_player_damage_summary( report::sc_html_stream& os, const player_t& actor )
{
  if ( actor.collected_data.dmg.max() == 0 && !actor.sim->debug )
    return;

  // Number of static columns in table
  const int static_columns = 5;
  // Number of dynamically changing columns
  int n_optional_columns = 6;

  // Abilities Section - Damage
  os << "<table class=\"sc sort stripetoprow\">\n"
     << "<thead>\n";

  if ( use_small_table( &actor ) )
    os << "<tr class=\"small\">\n";
  else
    os << "<tr>\n";

  sorttable_header( os, "Damage Stats", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_LEFT );
  sorttable_help_header( os, "DPS", "help-dps" );
  sorttable_help_header( os, "DPS%", "help-dps-pct" );
  sorttable_help_header( os, "Execute", "help-execute" );
  sorttable_help_header( os, "Interval", "help-interval", SORT_FLAG_ASC );
  sorttable_help_header( os, "DPE", "help-dpe" );
  sorttable_help_header( os, "DPET", "help-dpet" );
  // Optional columns begin here
  sorttable_help_header( os, "Type", "help-type", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_BOTH );
  sorttable_help_header( os, "Count", "help-count", SORT_FLAG_BOTH );
  sorttable_help_header( os, "Hit", "help-hit", SORT_FLAG_BOTH );
  sorttable_help_header( os, "Crit", "help-crit", SORT_FLAG_BOTH );
  sorttable_help_header( os, "Avg", "help-avg", SORT_FLAG_BOTH );
  sorttable_help_header( os, "Crit%", "help-crit-pct", SORT_FLAG_BOTH );

  if ( player_has_avoidance( actor, MASK_DMG ) )
  {
    sorttable_help_header( os, "Avoid%", "help-miss-pct" );
    n_optional_columns++;
  }

  if ( player_has_glance( actor, MASK_DMG ) )
  {
    sorttable_help_header( os, "G%", "help-glance-pct" );
    n_optional_columns++;
  }

  if ( player_has_block( actor, MASK_DMG ) )
  {
    sorttable_help_header( os, "B%", "help-block-pct" );
    n_optional_columns++;
  }

  if ( player_has_tick_results( actor, MASK_DMG ) )
  {
    sorttable_help_header( os, "Up%", "help-ticks-uptime-pct", SORT_FLAG_BOTH );
    n_optional_columns++;
  }

  os << "</tr>\n";

  if ( use_small_table( &actor ) )
    os << "<tr class=\"small\">\n";
  else
    os << "<tr>\n";

  os.format(
    R"(<th class="left name">{}</th><th class="right">{:.0Lf}</th><td colspan="{}" class="filler"></td></tr></thead>)",
    util::encode_html( actor.name() ),
    actor.collected_data.dps.mean(),
    static_columns + n_optional_columns );

  for ( const auto& stat : actor.stats_list )
  {
    if ( stat->compound_amount > 0 )
    {
      output_player_action( os, n_optional_columns, MASK_DMG, *stat, &actor );
    }
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

        os << "<tbody class=\"petrow\">\n";
        if ( use_small_table( &actor ) )
          os << "<tr class=\"small\">\n";
        else
          os << "<tr>\n";

        os.printf(
            "<th class=\"left small\">pet - %s</th>\n"
            "<th class=\"right small\">%.0f / %.0f</th>\n"
            "<td colspan=\"%d\" class=\"filler\"></td>\n"
            "</tr>\n",
            report_decorators::decorated_npc( *pet ), pet->collected_data.dps.mean(),
            pet->collected_data.dpse.mean(),
            static_columns + n_optional_columns );
        os << "</tbody>\n";
      }
      output_player_action( os, n_optional_columns, MASK_DMG, *stat, pet );
    }
  }
  os << "</table>\n";
}

void output_player_heal_summary( report::sc_html_stream& os, const player_t& actor )
{
  if ( actor.collected_data.heal.max() == 0 && !actor.sim->debug && actor.collected_data.absorb.max() == 0 )
    return;

  // Number of static columns in table
  const int static_columns = 5;
  // Number of dynamically changing columns
  int n_optional_columns = 6;

  // Abilities Section - Heal
  os << "<table class=\"sc sort stripetoprow\">\n"
     << "<thead>\n";

  if ( use_small_table( &actor ) )
    os << "<tr class=\"small\">\n";
  else
    os << "<tr>\n";

  sorttable_header( os, "Healing & Absorb Stats", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_LEFT );
  sorttable_help_header( os, "HPS", "help-hps" );
  sorttable_help_header( os, "HPS%", "help-hps-pct" );
  sorttable_help_header( os, "Execute", "help-execute" );
  sorttable_help_header( os, "Interval", "help-interval", SORT_FLAG_ASC );
  sorttable_help_header( os, "HPE", "help-hpe" );
  sorttable_help_header( os, "HPET", "help-hpet" );
  // Optional columns being here
  sorttable_help_header( os, "Type", "help-type", SORT_FLAG_ASC | SORT_FLAG_ALPHA );
  sorttable_help_header( os, "Count", "help-count" );
  sorttable_help_header( os, "Hit", "help-hit" );
  sorttable_help_header( os, "Crit", "help-crit" );
  sorttable_help_header( os, "Avg", "help-avg" );
  sorttable_help_header( os, "Crit%", "help-crit-pct" );

  if ( player_has_tick_results( actor, MASK_HEAL | MASK_ABSORB ) )
  {
    sorttable_help_header( os, "Up%", "help-ticks-uptime-pct", SORT_FLAG_BOTH );
    n_optional_columns++;
  }

  os << "</tr>";

  if ( use_small_table( &actor ) )
    os << "<tr class=\"small\">\n";
  else
    os << "<tr>\n";

  os << "<th class=\"left name\">" << util::encode_html( actor.name() ) << "</th>\n"
     << "<th class=\"right\">" << util::to_string( actor.collected_data.hps.mean() + actor.collected_data.aps.mean(), 0 ) << "</th>\n"
     << "<td colspan=\"" << ( static_columns + n_optional_columns ) << "\" class=\"filler\"></td>\n"
     << "</tr>\n"
     << "</thead>\n";

  for ( const auto& stat : actor.stats_list )
  {
    if ( stat->compound_amount > 0 )
    {
      output_player_action( os, n_optional_columns, MASK_HEAL | MASK_ABSORB, *stat, &actor );
    }
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

        os << "<tbody class=\"petrow\">\n";
        if ( use_small_table( &actor ) )
          os << "<tr class=\"small\">\n";
        else
          os << "<tr>\n";

        os.printf(
            "<th class=\"left small\">pet - %s</th>\n"
            "<th class=\"right small\">%.0f / %.0f</th>\n"
            "<td colspan=\"%d\" class=\"filler\"></td>\n"
            "</tr>\n",
            report_decorators::decorated_npc( *pet ), pet->collected_data.dps.mean(),
            pet->collected_data.dpse.mean(),
            static_columns + n_optional_columns );
        os << "</tbody>\n";
      }
      output_player_action( os, n_optional_columns, MASK_HEAL | MASK_ABSORB, *stat, pet );
    }
  }
  os << "</table>\n";
}

void output_player_simple_ability_summary( report::sc_html_stream& os, const player_t& actor )
{
  if ( actor.collected_data.dmg.max() == 0 && !actor.sim->debug )
    return;

  // Abilities Section - Simple Actions
  os << "<table class=\"sc sort stripetoprow\">\n"
     << "<thead>\n";

  if ( use_small_table( &actor ) )
    os << "<tr class=\"small\">\n";
  else
    os << "<tr>\n";

  sorttable_header( os, "Simple Action Stats", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_LEFT );
  sorttable_help_header( os, "Execute", "help-execute" );
  sorttable_help_header( os, "Interval", "help-interval", SORT_FLAG_ASC );

  os << "</tr>\n";

  if ( use_small_table( &actor ) )
    os << "<tr class=\"small\">\n";
  else
    os << "<tr>\n";

  os << "<th <th colspan=\"3\" class=\"left name\">" << util::encode_html( actor.name() ) << "</th>\n"
     << "</tr>\n"
     << "</thead>\n";

  for ( const auto& stat : actor.stats_list )
  {
    if ( stat->compound_amount == 0 )
    {
      output_player_action( os, 0, MASK_DMG | MASK_HEAL | MASK_ABSORB, *stat, &actor );
    }
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

        os << "<tbody class=\"petrow\">\n";
        if ( use_small_table( &actor ) )
          os << "<tr class=\"small\">\n";
        else
          os << "<tr>\n";

        os << "<th <th colspan=\"3\" class=\"left small\">pet - " << report_decorators::decorated_npc( *pet ) << "</th>\n"
           << "</tr>\n"
           << "</tbody>\n";
      }
      output_player_action( os, 0, MASK_DMG | MASK_HEAL | MASK_ABSORB, *stat, pet );
    }
  }
  os << "</table>\n";
}

void print_html_player_abilities( report::sc_html_stream& os, const player_t& p )
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

void print_html_proc_table( report::sc_html_stream& os, const player_t& p )
{
  os << "<table class=\"sc sort stripebody\">\n"
     << "<thead>\n"
     << "<tr>\n";

  static constexpr int columns = 7; // Set number of columns to make distribution charts fill the table width
  sorttable_header( os, "Proc", SORT_FLAG_ASC | SORT_FLAG_ALPHA | SORT_FLAG_LEFT );
  sorttable_header( os, "Count" );
  sorttable_header( os, "Min" );
  sorttable_header( os, "Max" );
  sorttable_header( os, "Interval", SORT_FLAG_ASC );
  sorttable_header( os, "Min", SORT_FLAG_ASC );
  sorttable_header( os, "Max", SORT_FLAG_ASC );

  os << "</tr>\n"
     << "</thead>\n";

  for ( const auto& proc : p.proc_list )
  {
    if ( proc->count.mean() > 0 )
    {
      bool show_count    = !proc->count.simple;
      bool show_interval = !proc->interval_sum.simple;

      std::string name  = find_matching_decorator( p, proc->name_str );
      std::string span  = name;
      std::string token = util::tokenize_fn( highchart::build_id( p, "_" + util::remove_special_chars( name ) + "_proc" ) );

      os << "<tbody>\n";
      if ( p.sim->report_details && ( show_count || show_interval ) )
        span = "<span id=\"" + token + "_toggle\" class=\"toggle-details\">" + name + "</span>";

      os.format( R"(<tr class="right"><td class="left">{}</td>)"
                 "<td>{:.1f}</td>"
                 "<td>{:.1f}</td>"
                 "<td>{:.1f}</td>"
                 "<td>{:.1f}s</td>"
                 "<td>{:.1f}s</td>"
                 "<td>{:.1f}s</td></tr>\n",
                 span,
                 proc->count.mean(),
                 proc->count.min(),
                 proc->count.max(),
                 proc->interval_sum.mean(),
                 proc->interval_sum.min(),
                 proc->interval_sum.max() );

      if ( p.sim->report_details && ( show_count || show_interval ) )
      {
        os.format( R"(<tr class="details hide"><td colspan="{}">)", columns );

        if ( show_count )
          report_helper::print_distribution_chart( os, p, &proc->count, name, token, "_proc" );

        if ( show_interval )
          report_helper::print_distribution_chart( os, p, &proc->interval_sum, name, token, "_interval", true );

        os << "</td></tr>\n";
      }
      os << "</tbody>\n";
    }
  }
  os << "</table>\n";
}

void print_html_uptime_table( report::sc_html_stream& os, const player_t& p )
{
  os << "<table class=\"sc sort stripebody\">\n"
      << "<thead>\n"
      << "<tr>\n";

  static constexpr int columns = 7;
  sorttable_header( os, "Uptime", SORT_FLAG_ALPHA | SORT_FLAG_ASC | SORT_FLAG_LEFT );
  sorttable_header( os, "Avg %" );
  sorttable_header( os, "Min" );
  sorttable_header( os, "Max" );
  sorttable_help_header( os, "Avg Dur", "help-uptime-average-duration" );
  sorttable_header( os, "Min" );
  sorttable_header( os, "Max" );

  os << "</tr>\n"
     << "</thead>\n";

  for ( const auto& uptime : p.uptime_list )
  {
    if ( !uptime->uptime_sum.mean() )
      continue;

    bool show_uptime   = !uptime->uptime_sum.simple;
    bool show_duration = !uptime->uptime_instance.simple;

    std::string name  = find_matching_decorator( p, uptime->name_str );
    std::string span  = name;
    std::string token =
        util::tokenize_fn( highchart::build_id( p, "_" + util::remove_special_chars( name ) + "_benefit" ) );

    os << "<tbody>\n";
    if ( p.sim->report_details && ( show_uptime || show_duration ) )
      span = "<span id=\"" + token + "_toggle\" class=\"toggle-details\">" + name + "</span>";

    os.format( R"(<tr class="right"><td class="left">{}</td>)"
               "<td>{:.2f}%</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.1f}s</td>"
               "<td>{:.1f}s</td>"
               "<td>{:.1f}s</td></tr>\n",
               span,
               uptime->uptime_sum.pretty_mean() * 100.0,
               uptime->uptime_sum.min() * 100.0,
               uptime->uptime_sum.max() * 100.0,
               uptime->uptime_instance.pretty_mean(),
               uptime->uptime_instance.min(),
               uptime->uptime_instance.max() );

    if ( p.sim->report_details && ( show_uptime || show_duration ) )
    {
      os.format( R"(<tr class="details hide"><td colspan="{}")", columns );

      if ( show_uptime )
        report_helper::print_distribution_chart( os, p, &uptime->uptime_sum, name, token, "_uptime" );

      if ( show_duration )
        report_helper::print_distribution_chart( os, p, &uptime->uptime_instance, name, token, "_duration", true );

      os << "</td></tr>\n";
    }
    os << "</tbody>\n";
  }

  for ( const auto& pet : p.pet_list )
  {
    for ( const auto& uptime : pet->uptime_list )
    {
      if ( !uptime->uptime_sum.mean() )
        continue;

      bool show_uptime   = !uptime->uptime_sum.simple;
      bool show_duration = !uptime->uptime_instance.simple;

      std::string name  = util::encode_html( pet->name_str + " - " + uptime->name_str );
      std::string span  = name;
      std::string token = util::tokenize_fn( highchart::build_id( p, "_pet" + util::to_string( pet->index ) + "_" +
                                                    util::remove_special_chars( uptime->name_str ) + "_uptime" ) );

      os << "<tbody>\n";
      if ( p.sim->report_details && ( show_uptime || show_duration ) )
        span = "<span id=\"" + token + "_toggle\" class=\"toggle-details\">" + name + "</span>";

      os.format( R"(<tr class="right"><td class="left">{}</td>)"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.1f}s</td>"
                 "<td>{:.1f}s</td>"
                 "<td>{:.1f}s</td></tr>\n",
                 span,
                 uptime->uptime_sum.pretty_mean() * 100.0,
                 uptime->uptime_sum.min() * 100.0,
                 uptime->uptime_sum.max() * 100.0,
                 uptime->uptime_instance.pretty_mean(),
                 uptime->uptime_instance.min(),
                 uptime->uptime_instance.max() );

      if ( p.sim->report_details && ( show_uptime || show_duration ) )
      {
        os.format( R"(<tr class="details hide"><td colspan="{}")", columns );

        if ( show_uptime )
          report_helper::print_distribution_chart( os, p, &uptime->uptime_sum, name, token, "_uptime" );

        if ( show_duration )
          report_helper::print_distribution_chart( os, p, &uptime->uptime_instance, name, token, "_duration", true );

        os << "</td></tr>\n";
      }
      os << "</tbody>\n";
    }
  }
  os << "</table>\n";
}

void print_html_benefit_table( report::sc_html_stream& os, const player_t& p )
{
  os << "<table class=\"sc sort stripebody\">\n"
     << "<thead>\n"
     << "<tr>\n";

  static constexpr int columns = 4;
  sorttable_header( os, "Benefit", SORT_FLAG_ALPHA | SORT_FLAG_ASC | SORT_FLAG_LEFT );
  sorttable_header( os, "Avg %" );
  sorttable_header( os, "Min" );
  sorttable_header( os, "Max" );

  os << "</tr>\n"
     << "</thead>\n";

  for ( const auto& benefit : p.benefit_list )
  {
    if ( !benefit->ratio.mean() )
      continue;

    bool show_ratio = !benefit->ratio.simple;

    std::string name  = util::encode_html( benefit->name_str );
    std::string span  = name;
    std::string token = util::tokenize_fn( highchart::build_id( p, "_" + util::remove_special_chars( name ) + "_benefit" ) );

    os << "<tbody>\n";
    if ( p.sim->report_details && show_ratio )
      span = "<span id=\"" + token + "_toggle\" class=\"toggle-details\">" + name + "</span>";

    os.format( R"(<tr class="right"><td class="left">{}</td>)"
               "<td>{:.2f}%</td>"
               "<td>{:.2f}%</td>"
               "<td>{:.2f}%</td></tr>\n",
               span,
               benefit->ratio.pretty_mean(),
               benefit->ratio.min(),
               benefit->ratio.max() );

    if ( p.sim->report_details && show_ratio )
    {
      os.format( R"(<tr class="details hide"><td colspan="{}")", columns );

      report_helper::print_distribution_chart( os, p, &benefit->ratio, name, token, "_ratio" );

      os << "</td></tr>\n";
    }
    os << "</tbody>\n";
  }

  for ( const auto& pet : p.pet_list )
  {
    for ( const auto& benefit : pet->benefit_list )
    {
      if ( !benefit->ratio.mean() )
        continue;

      bool show_ratio = !benefit->ratio.simple;

      std::string name  = util::encode_html( pet->name_str + " - " + benefit->name_str );
      std::string span  = name;
      std::string token = util::tokenize_fn( highchart::build_id( p, "_pet" + util::to_string( pet->index ) + "_" +
                                                    util::remove_special_chars( benefit->name_str ) + "_benefit" ) );

      os << "<tbody>\n";
      if ( p.sim->report_details && show_ratio )
        span = "<span id=\"" + token + "_toggle\" class=\"toggle-details\">" + name + "</span>";

      os.format( R"(<tr class="right"><td class="left">{}</td>)"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td>"
                 "<td>{:.2f}%</td></tr>\n",
                 span,
                 benefit->ratio.pretty_mean(),
                 benefit->ratio.min(),
                 benefit->ratio.max() );

      if ( p.sim->report_details && show_ratio )
      {
        os.format( R"(<tr class="details hide"><td colspan="{}")", columns );

        report_helper::print_distribution_chart( os, p, &benefit->ratio, name, token, "_ratio" );

        os << "</td></tr>\n";
      }
      os << "</tbody>\n";
    }
  }
  os << "</table>\n";
}

// print_html_player_procs ==================================================

void print_html_player_procs( report::sc_html_stream& os, const player_t& p )
{
  auto proc_count = range::count_if( p.proc_list, []( const proc_t* pr ) { return pr->count.mean() > 0; } );

  auto benefit_count = range::count_if( p.benefit_list, []( const benefit_t* b ) { return b->ratio.mean() > 0; } );
  for ( const auto& pet : p.pet_list )
    benefit_count += range::count_if( pet->benefit_list, []( const benefit_t* b ) { return b->ratio.mean() > 0; } );

  auto uptime_count = range::count_if( p.uptime_list, []( const uptime_t* u ) { return u->uptime_sum.mean() > 0; } );
  for ( const auto& pet : p.pet_list )
    uptime_count += range::count_if( pet->uptime_list, []( const uptime_t* u ) { return u->uptime_sum.mean() > 0; } );

  if ( !benefit_count && !uptime_count && !proc_count )
    return;

  os << "<div class=\"player-section procs\">\n"
     << "<h3 class=\"toggle open\">Procs, Uptimes & Benefits</h3>\n"
     << "<div class=\"toggle-content flexwrap\">\n"; // Can't use columns as detail toggle will change widths

  bool new_div = false;

  os << "<div>\n"; // Open DIV#1

  // Procs
  if ( proc_count )
  {
    print_html_proc_table( os, p );
    if ( proc_count > benefit_count + uptime_count )
    {
      new_div = true;
      os << "</div>\n<div>\n";
    }
  }

  if ( uptime_count <= benefit_count && proc_count )
  {
    if ( uptime_count )
      print_html_uptime_table( os, p );

    if ( !new_div )
      os << "</div>\n<div>\n";

    if ( benefit_count )
      print_html_benefit_table( os, p );
  }
  else
  {
    if ( benefit_count )
      print_html_benefit_table( os, p );

    if ( !new_div )
      os << "</div>\n<div>\n";

    if ( uptime_count )
      print_html_uptime_table( os, p );
  }

  os << "</div>\n"; // Close DIV for all

  os << "</div>\n"
     << "</div>\n";
}

void print_html_player_cooldown_waste( report::sc_html_stream& os, const player_t& p )
{
  if ( !range::contains( p.cooldown_waste_data_list, true, &cooldown_waste_data_t::active ) )
    return;

  os << "<div class=\"player-section custom_section\">\n"
        "<h3 class=\"toggle open\">Cooldown Waste</h3>\n"
        "<div class=\"toggle-content\">\n"
        "<table class=\"sc sort even\">\n"
        "<thead>\n"
        "<tr>\n"
        "<th></th>\n"
        "<th colspan=\"3\">Seconds per Execute</th>\n"
        "<th colspan=\"3\">Seconds per Iteration</th>\n"
        "</tr>\n"
        "<tr>\n"
        "<th class=\"toggle-sort left\" data-sortdir=\"asc\" data-sorttype=\"alpha\">Ability</th>\n"
        "<th class=\"toggle-sort\">Average</th>\n"
        "<th class=\"toggle-sort\">Minimum</th>\n"
        "<th class=\"toggle-sort\">Maximum</th>\n"
        "<th class=\"toggle-sort\">Average</th>\n"
        "<th class=\"toggle-sort\">Minimum</th>\n"
        "<th class=\"toggle-sort\">Maximum</th>\n"
        "</tr>\n"
        "</thead>\n";

  for ( const auto& data : p.cooldown_waste_data_list )
  {
    if ( !data->active() )
      continue;

    std::string name = data->cd->name_str;
    if ( action_t* a = p.find_action( name ) )
    {
      name = report_decorators::decorated_action( *a );
    }
    else
    {
      std::vector<const action_t*> actions;
      range::for_each( p.action_list, [ &actions, &data ] ( const action_t* a )
      {
        if ( data->cd == a->cooldown )
        {
          auto it = range::find( actions, a->internal_id, &action_t::internal_id );
          if ( it == actions.end() )
          {
            actions.emplace_back( a );
          }
        }
      } );

      if ( actions.size() > 0 )
      {
        std::vector<std::string> names;
        range::for_each( actions, [ &names ] ( const action_t* a )
        {
          names.emplace_back( report_decorators::decorated_action( *a ) );
        } );

        name = util::string_join( names, "<br/>" );
      }
      else
      {
        name = util::encode_html( name );
      }
    }

    os.format( R"(<tr class="right"><td class="left">{}</td>)"
               "<td>{:.3f}</td>"
               "<td>{:.3f}</td>"
               "<td>{:.3f}</td>"
               "<td>{:.3f}</td>"
               "<td>{:.3f}</td>"
               "<td>{:.3f}</td></tr>\n",
               name,
               data->normal.mean(),
               data->normal.min(),
               data->normal.max(),
               data->cumulative.mean(),
               data->cumulative.min(),
               data->cumulative.max() );
  }

  os << "</table>\n"
        "</div>\n"
        "</div>\n";
}

// print_html_player_deaths =================================================

void print_html_player_deaths( report::sc_html_stream& os, const player_t& p,
                               const player_processed_report_information_t& )
{
  // Death Analysis

  const extended_sample_data_t& deaths = p.collected_data.deaths;

  if ( deaths.size() > 0 )
  {
    os << "<div class=\"player-section deaths\">\n"
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
       << "<td class=\"right\">" << as<double>( deaths.size() ) / p.sim->iterations * 100 << "</td>\n"
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
       << "<td class=\"right\">" << p.collected_data.dmg_taken.mean() << "</td>\n"
       << "</tr>\n";

    os << "</table>\n";

    os << "<div class=\"clear\"></div>\n";

    highchart::histogram_chart_t chart( highchart::build_id( p, "death_dist" ), *p.sim );
    if ( chart::generate_distribution( chart, &p, p.collected_data.deaths.distribution,
                                       util::encode_html( p.name_str ) + " Death", p.collected_data.deaths.mean(),
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

void print_html_player_( report::sc_html_stream& os, const player_t& p )
{
  print_html_player_description( os, p );

  print_html_player_results_spec_gear( os, p );

  print_html_player_scale_factors( os, p, p.report_information );

  print_html_player_charts( os, p, p.report_information );

  print_html_player_abilities( os, p );

  print_html_player_buffs( os, p, p.report_information );

  print_html_player_procs( os, p );

  print_html_player_cooldown_waste( os, p );

  print_html_player_custom_section( os, p, p.report_information );

  print_html_player_resources( os, p );

  print_html_player_deaths( os, p, p.report_information );

  print_html_player_statistics( os, p, p.report_information );

  print_html_player_mixin_reports( os, p );

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
        action->marker = (char)( ( j < 10 ) ? ( '0' + j )
                               : ( j < 36 ) ? ( 'A' + j - 10 )
                               : ( j < 66 ) ? ( 'a' + j - 36 )
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
  report_helper::generate_player_charts( p, p.report_information );
  report_helper::generate_player_buff_lists( p, p.report_information );
  build_action_markers( p );
}

}  // UNNAMED NAMESPACE ====================================================

namespace report
{
void print_html_player( report::sc_html_stream& os, player_t& p )
{
  build_player_report_data( p );
  print_html_player_( os, p );
}

}  // END report NAMESPACE

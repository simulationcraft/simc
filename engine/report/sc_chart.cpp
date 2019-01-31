// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_highchart.hpp"
#include "simulationcraft.hpp"

#include <clocale>
#include <cmath>

using namespace js;

namespace
{  // anonymous namespace ==========================================

struct filter_non_performing_players
{
  std::string type;
  filter_non_performing_players( std::string type_ ) : type( type_ )
  {
  }
  bool operator()( player_t* p ) const
  {
    if ( type == "dps" && p->collected_data.dps.mean() <= 0 )
      return true;
    else if ( type == "prioritydps" &&
              p->collected_data.prioritydps.mean() <= 0 )
      return true;
    else if ( type == "hps" && p->collected_data.hps.mean() <= 0 )
      return true;
    else if ( type == "dtps" && p->collected_data.dtps.mean() <= 0 )
      return true;
    else if ( type == "tmi" &&
              p->collected_data.theck_meloree_index.mean() <= 0 )
      return true;

    return false;
  }
};

struct filter_stats_dpet
{
  bool player_is_healer;
  filter_stats_dpet( const player_t& p )
    : player_is_healer( p.primary_role() == ROLE_HEAL )
  {
  }
  bool operator()( const stats_t* st ) const
  {
    if ( st->quiet )
      return true;
    if ( st->apet <= 0 )
      return true;
    if ( st->num_refreshes.mean() > 4 * st->num_executes.mean() )
      return true;
    if ( player_is_healer != ( st->type != STATS_DMG ) )
      return true;

    return false;
  }
};

struct filter_waiting_stats
{
  bool operator()( const stats_t* st ) const
  {
    if ( st->quiet )
      return true;
    if ( st->total_time <= timespan_t::zero() )
      return true;
    if ( st->background )
      return true;

    return false;
  }
};

bool compare_stats_by_mean( const stats_t* l, const stats_t* r )
{
  if ( l->actual_amount.mean() == r->actual_amount.mean() )
  {
    return l->name_str < r->name_str;
  }

  return l->actual_amount.mean() > r->actual_amount.mean();
}

void add_color_data( sc_js_t& data,
                     const std::vector<const player_t*>& player_list )
{
  for ( auto p : player_list )
  {
    std::string sanitized_name = p -> name_str;
    util::replace_all( sanitized_name, ".", "_" );
    data.set( "__colors.C_" + sanitized_name, color::class_color( p->type ).str() );
  }
}

enum metric_e
{
  METRIC_NONE,
  METRIC_DPS,
  METRIC_PDPS,
  METRIC_HPS,
  METRIC_DTPS,
  METRIC_TMI,
  METRIC_APM,
  METRIC_VARIANCE,
  METRIC_MAX
};

enum metric_value_e
{
  VALUE_MEAN = 0,
  VALUE_BURST_MAX,
  VALUE_METRIC_MAX
};

const std::array<unsigned, METRIC_MAX> enabled_values = {
    {0, ( 1 << VALUE_MEAN ) | ( 1 << VALUE_BURST_MAX ), ( 1 << VALUE_MEAN ),
     ( 1 << VALUE_MEAN ), ( 1 << VALUE_MEAN ) | ( 1 << VALUE_BURST_MAX ),
     ( 1 << VALUE_MEAN ), ( 1 << VALUE_MEAN ), ( 1 << VALUE_MEAN )}};

double apm_player_mean( const player_t* p )
{
  double fight_length = p->collected_data.fight_length.mean();
  double foreground_actions =
      p->collected_data.executed_foreground_actions.mean();
  if ( fight_length > 0 )
  {
    return 60 * foreground_actions / fight_length;
  }

  return 0;
}

double variance( const player_t* p )
{
  return p->collected_data.dps.mean() > 0
    ? ( p->collected_data.dps.std_dev / p->collected_data.dps.pretty_mean() * 100 )
    : 0;
}

double compute_player_burst_max( const sc_timeline_t& container )
{
  sc_timeline_t timeline;
  container.build_derivative_timeline( timeline );
  double m = 0;
  for ( size_t i = 0; i < timeline.data().size(); ++i )
  {
    if ( timeline.data()[ i ] > m )
    {
      m = timeline.data()[ i ];
    }
  }

  return m;
}

metric_e populate_player_list( const std::string& type, const sim_t& sim,
                               std::vector<const player_t*>& pl,
                               std::string& name )
{
  const std::vector<player_t*>* source_list = nullptr;
  metric_e m                                = METRIC_NONE;

  if ( util::str_compare_ci( type, "dps" ) )
  {
    name        = "Damage per Second";
    source_list = &sim.players_by_dps;
    m           = METRIC_DPS;
  }
  else if ( util::str_compare_ci( type, "prioritydps" ) )
  {
    name        = "Priority Target/Boss Damage ";
    source_list = &sim.players_by_priority_dps;
    m           = METRIC_PDPS;
  }
  else if ( util::str_compare_ci( type, "hps" ) )
  {
    name        = "Heal & Absorb per Second";
    source_list = &sim.players_by_hps;
    m           = METRIC_HPS;
  }
  else if ( util::str_compare_ci( type, "dtps" ) )
  {
    name        = "Damage Taken per Second";
    source_list = &sim.players_by_dtps;
    m           = METRIC_DTPS;
  }
  else if ( util::str_compare_ci( type, "tmi" ) )
  {
    name        = "Theck-Meloree Index";
    source_list = &sim.players_by_tmi;
    m           = METRIC_TMI;
  }
  else if ( util::str_compare_ci( type, "apm" ) )
  {
    name        = "Actions Per Minute";
    source_list = &sim.players_by_apm;
    m           = METRIC_APM;
  }
  else if ( util::str_compare_ci( type, "variance" ) )
  {
    // Variance implies there is DPS output in the simulator
    if ( sim.raid_dps.mean() > 0 )
    {
      name        = "DPS Variance Percentage";
      source_list = &sim.players_by_variance;
      m           = METRIC_VARIANCE;
    }
  }

  if ( source_list != nullptr )
  {
    range::remove_copy_if( *source_list, back_inserter( pl ),
                           filter_non_performing_players( type ) );
  }

  return pl.size() > 1 ? m : METRIC_NONE;
}

std::vector<double> get_data_summary( const player_collected_data_t& container,
                                      metric_e metric,
                                      double percentile = 0.25 )
{
  const extended_sample_data_t* c = nullptr;
  std::vector<double> data( 6, 0 );
  switch ( metric )
  {
    case METRIC_DPS:
      c = &container.dps;
      break;
    case METRIC_PDPS:
      c = &container.prioritydps;
      break;
    case METRIC_HPS:
      c = &container.hps;
      break;
    case METRIC_DTPS:
      c = &container.dtps;
      break;
    case METRIC_TMI:
      c = &container.theck_meloree_index;
      break;
    // APM is gonna have a mean only
    case METRIC_APM:
    case METRIC_VARIANCE:
    default:
      return data;
  }

  data[ 0 ] = c->min();
  data[ 1 ] = c->percentile( percentile );
  data[ 2 ] = c->mean();
  data[ 3 ] = c->percentile( 1 - percentile );
  data[ 4 ] = c->max();
  data[ 5 ] = c->percentile( .5 );

  return data;
}

double get_data_value( const player_collected_data_t& container,
                       metric_e metric, metric_value_e val )
{
  const extended_sample_data_t* c = nullptr;
  switch ( metric )
  {
    case METRIC_DPS:
      c = &container.dps;
      break;
    case METRIC_PDPS:
      c = &container.prioritydps;
      break;
    case METRIC_HPS:
      c = &container.hps;
      break;
    case METRIC_DTPS:
      c = &container.dtps;
      break;
    case METRIC_TMI:
      c = &container.theck_meloree_index;
      break;
    // APM is gonna have a mean only
    case METRIC_APM:
    {
      if ( val != VALUE_MEAN )
      {
        return 0;
      }

      double fight_length       = container.fight_length.mean();
      double foreground_actions = container.executed_foreground_actions.mean();
      if ( fight_length > 0 )
      {
        return 60 * foreground_actions / fight_length;
      }
      return 0;
    }
    case METRIC_VARIANCE:
    {
      if ( val != VALUE_MEAN )
      {
        return 0;
      }

      if ( container.dps.mean() > 0 )
      {
        return ( container.dps.std_dev / container.dps.pretty_mean() * 100 );
      }

      return 0;
    }
    default:
      return 0;
  }

  switch ( val )
  {
    case VALUE_MEAN:
      return c->mean();
    // Burst max only computed for damage and damage taken, as those are the
    // only data collection
    // that has timelines available
    case VALUE_BURST_MAX:
    {
      if ( metric != METRIC_DPS && metric != METRIC_DTPS )
      {
        return 0;
      }

      if ( metric == METRIC_DPS )
      {
        return compute_player_burst_max( container.timeline_dmg );
      }
      else
      {
        return compute_player_burst_max( container.timeline_dmg_taken );
      }
    }
    default:
      return 0;
  }
}

struct player_list_comparator_t
{
  metric_e metric_;
  metric_value_e value_;

  player_list_comparator_t( metric_e m, metric_value_e v )
    : metric_( m ), value_( v )
  {
  }

  bool operator()( const player_t* p1, const player_t* p2 )
  {
    const extended_sample_data_t *d_p1 = nullptr, *d_p2 = nullptr;
    switch ( metric_ )
    {
      case METRIC_DPS:
        d_p1 = &p1->collected_data.dps;
        d_p2 = &p2->collected_data.dps;
        break;
      case METRIC_PDPS:
        d_p1 = &p1->collected_data.prioritydps;
        d_p2 = &p2->collected_data.prioritydps;
        break;
      case METRIC_HPS:
        d_p1 = &p1->collected_data.hps;
        d_p2 = &p2->collected_data.hps;
        break;
      case METRIC_DTPS:
        d_p1 = &p1->collected_data.dtps;
        d_p2 = &p2->collected_data.dtps;
        break;
      case METRIC_TMI:
        d_p1 = &p1->collected_data.theck_meloree_index;
        d_p2 = &p2->collected_data.theck_meloree_index;
        break;
      // APM has no full data collection, so sort always based on mean
      case METRIC_APM:
        return apm_player_mean( p1 ) > apm_player_mean( p2 );
      case METRIC_VARIANCE:
        return variance( p1 ) > variance( p2 );
      default:
        return true;
    }

    double rv, lv;
    switch ( value_ )
    {
      case VALUE_MEAN:
        lv = d_p1->mean();
        rv = d_p2->mean();
        break;
      // This is going to be slow.
      case VALUE_BURST_MAX:
      {
        if ( metric_ != METRIC_DPS && metric_ != METRIC_DTPS )
        {
          return true;
        }

        if ( metric_ == METRIC_DPS )
        {
          lv = compute_player_burst_max( p1->collected_data.timeline_dmg );
          rv = compute_player_burst_max( p2->collected_data.timeline_dmg );
        }
        else
        {
          lv = compute_player_burst_max( p1->collected_data.timeline_dmg_taken );
          rv = compute_player_burst_max( p2->collected_data.timeline_dmg_taken );
        }
        break;
      }
      default:
        return true;
    }

    if ( lv == rv )
    {
      return p1->actor_index < p2->actor_index;
    }

    return lv > rv;
  }
};

std::string get_metric_value_name( metric_value_e val )
{
  switch ( val )
  {
    case VALUE_MEAN:
      return "";
    case VALUE_BURST_MAX:
      return "Maximum burst";
    default:
      return "Unknown " + util::to_string( val );
  }
}

}  // anonymous namespace ====================================================

// ==========================================================================
// Chart
// ==========================================================================

bool chart::generate_raid_downtime( highchart::bar_chart_t& bc,
                                    const sim_t& sim )
{
  std::vector<const player_t*> players;
  range::copy_if( sim.players_by_name, std::back_inserter( players ),
                  []( const player_t* player ) {
                    return ( player->collected_data.waiting_time.mean() /
                             player->collected_data.fight_length.mean() ) >
                           0.01;
                  } );

  if ( players.empty() )
  {
    return false;
  }

  range::sort( players, []( const player_t* l, const player_t* r ) {
    if ( l->collected_data.waiting_time.mean() == r->collected_data.waiting_time.mean() )
    {
      return l->actor_index < r->actor_index;
    }

    return l->collected_data.waiting_time.mean() >
           r->collected_data.waiting_time.mean();
  } );

  for ( const auto p : players )
  {
    const auto& c      = color::class_color( p->type );
    double waiting_pct = ( 100.0 * p->collected_data.waiting_time.mean() /
                           p->collected_data.fight_length.mean() );
    sc_js_t e;
    e.set( "name", report::decorate_html_string( p->name_str, c ) );
    e.set( "color", c.str() );
    e.set( "y", waiting_pct );

    bc.add( "series.0.data", e );
  }

  bc.set( "series.0.name", "Downtime" );
  bc.set( "series.0.tooltip.pointFormat",
          "<span style=\"color:{point.color}\">\xE2\x97\x8F</span> "
          "{series.name}: <b>{point.y}</b>%<br/>" );

  bc.set_title( "Raid downtime" );
  bc.set_yaxis_title( "Downtime% (of total fight length)" );
  bc.height_ = 92 + players.size() * 20;

  return true;
}

bool chart::generate_raid_gear( highchart::bar_chart_t& bc, const sim_t& sim )
{
  if ( sim.players_by_name.empty() )
  {
    return false;
  }

  std::array<std::vector<double>, STAT_MAX> data_points;
  std::array<bool, STAT_MAX> has_stat;
  range::fill( has_stat, false );

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    data_points[ i ].reserve( sim.players_by_name.size() + 1 );
    for ( const auto& player : sim.players_by_name )
    {
      double total = player->total_gear.get_stat( i );
      total += player->enchant.get_stat( i );
      total += sim.enchant.get_stat( i );

      if ( total > 0 )
      {
        has_stat[ i ] = true;
      }

      data_points[ i ].push_back( total * gear_stats_t::stat_mod( i ) );
      bc.add( "xAxis.categories",
              report::decorate_html_string(
                  player->name_str, color::class_color( player->type ) ) );
    }
  }

  for ( stat_e i = STAT_NONE; i < STAT_MAX; ++i )
  {
    if ( has_stat[ i ] )
    {
      bc.add( "colors", color::stat_color( i ).str() );
    }
  }

  int series_idx = 0;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( !has_stat[ i ] )
    {
      continue;
    }

    std::string series_str = "series." + util::to_string( series_idx );
    bc.add( series_str + ".name", util::stat_type_abbrev( i ) );
    for ( const auto& data_point : data_points[ i ] )
    {
      bc.add( series_str + ".data", data_point );
    }

    ++series_idx;
  }

  bc.height_ = 95 + sim.players_by_name.size() * 24 + 55;

  bc.set_title( "Raid Gear" );
  bc.set_yaxis_title( "Cumulative stat amount" );
  bc.set( "legend.enabled", true );
  bc.set( "legend.layout", "horizontal" );
  bc.set( "plotOptions.series.stacking", "normal" );
  bc.set( "tooltip.pointFormat", "<b>{series.name}</b>: {point.y:.0f}" );

  return true;
}

bool chart::generate_reforge_plot( highchart::chart_t& ac, const player_t& p )
{
  if ( p.reforge_plot_data.empty() )
  {
    return false;
  }

  double max_dps   = 0.0;
  double min_dps   = std::numeric_limits<double>::max();
  double baseline  = 0.0;
  size_t num_stats = p.reforge_plot_data.front().size() - 1;

  for ( const auto& pdata : p.reforge_plot_data )
  {
    assert( num_stats < pdata.size() );
    if ( pdata[ num_stats ].value < min_dps )
      min_dps = pdata[ num_stats ].value;
    if ( pdata[ num_stats ].value > max_dps )
      max_dps = pdata[ num_stats ].value;

    if ( pdata[ 0 ].value == 0 && pdata[ 1 ].value == 0 )
    {
      baseline = pdata[ num_stats ].value;
    }
  }

  double yrange = std::max( std::fabs( baseline - min_dps ),
                            std::fabs( max_dps - baseline ) );

  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    ac.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  ac.set_title( p.name_str + " Reforge Plot" );
  ac.set_yaxis_title( "Damage Per Second" );
  std::string from_stat, to_stat, from_color, to_color;
  from_stat = util::stat_type_abbrev(
      p.sim->reforge_plot->reforge_plot_stat_indices[ 0 ] );
  to_stat = util::stat_type_abbrev(
      p.sim->reforge_plot->reforge_plot_stat_indices[ 1 ] );
  from_color =
      color::stat_color( p.sim->reforge_plot->reforge_plot_stat_indices[ 0 ] );
  to_color =
      color::stat_color( p.sim->reforge_plot->reforge_plot_stat_indices[ 1 ] );

  std::string span_from_stat = "<span style=\"color:" + from_color +
                               ";font-weight:bold;\">" + from_stat + "</span>";
  std::string span_from_stat_abbrev = "<span style=\"color:" + from_color +
                                      ";font-weight:bold;\">" +
                                      from_stat.substr( 0, 2 ) + "</span>";
  std::string span_to_stat = "<span style=\"color:" + to_color +
                             ";font-weight:bold;\">" + to_stat + "</span>";
  std::string span_to_stat_abbrev = "<span style=\"color:" + to_color +
                                    ";font-weight:bold;\">" +
                                    to_stat.substr( 0, 2 ) + "</span>";

  ac.set( "yAxis.min", baseline - yrange );
  ac.set( "yAxis.max", baseline + yrange );
  ac.set( "yaxis.minPadding", 0.01 );
  ac.set( "yaxis.maxPadding", 0.01 );
  ac.set( "xAxis.labels.overflow", "false" );
  ac.set_xaxis_title( "Reforging between " + span_from_stat + " and " +
                      span_to_stat );

  std::string formatter_function = "function() {";
  formatter_function += "if (this.value == 0) { return 'Baseline'; } ";
  formatter_function +=
      "else if (this.value < 0) { return Math.abs(this.value) + '<br/>" +
      span_from_stat_abbrev + " to " + span_to_stat_abbrev + "'; } ";
  formatter_function += "else { return Math.abs(this.value) + '<br/>" +
                        span_to_stat_abbrev + " to " + span_from_stat_abbrev +
                        "'; } ";
  formatter_function += "}";

  ac.set( "xAxis.labels.formatter", formatter_function );
  ac.value( "xAxis.labels.formatter" ).SetRawOutput( true );

  ac.add_yplotline( baseline, "baseline", 1.25, "#FF8866" );

  std::vector<std::pair<double, double> > mean;
  std::vector<highchart::data_triple_t> range;

  for ( const auto& pdata : p.reforge_plot_data )
  {
    double x = util::round( pdata[ 0 ].value, p.sim->report_precision );
    double v = util::round( pdata[ 2 ].value, p.sim->report_precision );
    double e = util::round( pdata[ 2 ].error / 2, p.sim->report_precision );

    mean.push_back( std::make_pair(x, v ) );
    range.push_back( highchart::data_triple_t(x, v + e, v - e ) );
  }

  ac.add_simple_series( "line", from_color, "Mean", mean );
  ac.add_simple_series( "arearange", to_color, "Range", range );

  ac.set( "series.0.zIndex", 1 );
  ac.set( "series.0.marker.radius", 0 );
  ac.set( "series.0.lineWidth", 1.5 );
  ac.set( "series.1.fillOpacity", 0.5 );
  ac.set( "series.1.lineWidth", 0 );
  ac.set( "series.1.linkedTo", ":previous" );
  ac.height_ = 500;

  return true;
}

// chart::distribution_dps ==================================================

bool chart::generate_distribution( highchart::histogram_chart_t& hc,
                                   const player_t* p,
                                   const std::vector<size_t>& dist_data,
                                   const std::string& distribution_name,
                                   double avg, double min, double max )
{
  int max_buckets = as<int>( dist_data.size() );

  if ( !max_buckets )
    return false;

  hc.set( "plotOptions.column.color", color::YELLOW.str() );
  hc.set_title( distribution_name + " Distribution" );
  if ( p && p->sim->player_no_pet_list.size() > 1 )
  {
    hc.set_toggle_id( "player" + util::to_string( p->index ) + "toggle" );
  }

  hc.set( "xAxis.tickInterval", 25 );
  hc.set( "xAxis.tickAtEnd", true );
  hc.set( "yAxis.title.text", "# Iterations" );
  hc.set( "tooltip.headerFormat", "Values: <b>{point.key}</b><br/>" );

  double step = ( max - min ) / dist_data.size();

  std::vector<int> tick_indices;

  for ( int i = 0; i < max_buckets; i++ )
  {
    double begin = min + i * step;
    double end   = min + ( i + 1 ) * step;

    sc_js_t e;

    e.set( "y", static_cast<double>( dist_data[ i ] ) );
    if ( i == 0 )
    {
      tick_indices.push_back( i );
      e.set( "name", "min=" + util::to_string( static_cast<unsigned>( min ) ) );
      e.set( "color", color::YELLOW.dark().str() );
    }
    else if ( avg >= begin && avg <= end )
    {
      tick_indices.push_back( i );
      e.set( "name",
             "mean=" + util::to_string( static_cast<unsigned>( avg ) ) );
      e.set( "color", color::YELLOW.dark().str() );
    }
    else if ( i == max_buckets - 1 )
    {
      tick_indices.push_back( i );
      e.set( "name", "max=" + util::to_string( static_cast<unsigned>( max ) ) );
      e.set( "color", color::YELLOW.dark().str() );
    }
    else
    {
      e.set( "name", util::to_string( util::round( begin, 0 ) ) + " to " +
                         util::to_string( util::round( end, 0 ) ) );
    }

    hc.add( "series.0.data", e );
  }

  hc.set( "series.0.name", "Iterations" );

  for ( auto index : tick_indices )
  {
    hc.add( "xAxis.tickPositions", index );
  }

  return true;
}

bool chart::generate_gains( highchart::pie_chart_t& pc, const player_t& p,
                            resource_e type )
{
  std::string resource_name =
      util::inverse_tokenize( util::resource_type_string( type ) );

  // Build gains List
  std::vector<gain_t*> gains_list;
  range::copy_if(
      p.gain_list, std::back_inserter( gains_list ),
      [type]( const gain_t* gain ) { return gain->actual[ type ] > 0; } );

  range::sort( gains_list, []( const gain_t* l, const gain_t* r ) {
    if ( l->actual == r->actual )
    {
      return l->name_str < r->name_str;
    }

    return l->actual > r->actual;
  } );

  if ( gains_list.empty() )
  {
    return false;
  }

  pc.set_title( p.name_str + " " + resource_name + " Gains" );
  pc.set( "plotOptions.pie.dataLabels.format",
          "<b>{point.name}</b>: {point.y:.1f}" );
  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    pc.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  for ( size_t i = 0; i < gains_list.size(); ++i )
  {
    const gain_t* gain = gains_list[ i ];

    sc_js_t e;

    e.set( "color", color::resource_color( type )
                        .dark( i * ( 0.75 / gains_list.size() ) )
                        .str() );
    e.set( "y", util::round( gain->actual[ type ], p.sim->report_precision ) );
    e.set( "name", gain->name_str );

    pc.add( "series.0.data", e );
  }

  return true;
}

bool chart::generate_spent_time( highchart::pie_chart_t& pc, const player_t& p )
{
  pc.set_title( p.name_str + " Spent Time" );
  pc.set( "plotOptions.pie.dataLabels.format",
          "<b>{point.name}</b>: {point.y:.1f}s" );
  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    pc.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  std::vector<stats_t*> filtered_waiting_stats;

  // Filter stats we do not want in the chart ( quiet, background, zero
  // total_time ) and copy them to filtered_waiting_stats
  range::remove_copy_if( p.stats_list, back_inserter( filtered_waiting_stats ),
                         filter_waiting_stats() );

  size_t num_stats = filtered_waiting_stats.size();
  if ( num_stats == 0 && p.collected_data.waiting_time.mean() == 0 )
    return false;

  range::sort( filtered_waiting_stats,
               []( const stats_t* l, const stats_t* r ) {
                 if ( l->total_time == r->total_time )
                 {
                   return l->name_str < r->name_str;
                 }
                 return l->total_time > r->total_time;
               } );

  // Build Data
  if ( !filtered_waiting_stats.empty() )
  {
    for ( const stats_t* stats : filtered_waiting_stats )
    {
      std::string color = color::school_color( stats->school );
      if ( color.empty() )
      {
        p.sim->errorf(
            "chart::generate_stats_sources assertion error! School color "
            "unknown, stats %s from %s. School %s\n",
            stats->name_str.c_str(), p.name(),
            util::school_type_string( stats->school ) );
        assert( 0 );
      }

      sc_js_t e;
      e.set( "color", color );
      e.set( "y", util::round( stats->total_time.total_seconds(),
                               p.sim->report_precision ) );
      e.set( "name", report::decorate_html_string( stats->name_str, color ) );

      pc.add( "series.0.data", e );
    }
  }

  if ( p.collected_data.waiting_time.mean() > 0 )
  {
    sc_js_t e;
    e.set( "color", color::WHITE.str() );
    e.set( "y", util::round( p.collected_data.waiting_time.mean(),
                             p.sim->report_precision ) );
    e.set( "name", "Waiting" );
    pc.add( "series.0.data", e );
  }

  if ( p.collected_data.pooling_time.mean() > 0 )
  {
    sc_js_t e;
    e.set( "color", color::GREY3.str() );
    e.set( "y", util::round( p.collected_data.pooling_time.mean(),
                             p.sim->report_precision ) );
    e.set( "name", "Pooling" );
    pc.add( "series.0.data", e );
  }

  return true;
}

bool chart::generate_stats_sources( highchart::pie_chart_t& pc,
                                    const player_t& p, const std::string title,
                                    const std::vector<stats_t*>& stats_list )
{
  if ( stats_list.empty() )
  {
    return false;
  }

  pc.set_title( title );
  pc.set( "plotOptions.pie.dataLabels.format",
          "<b>{point.name}</b>: {point.percentage:.1f}%" );
  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    pc.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  for ( const stats_t* stats : stats_list )
  {
    const color::rgb& c = color::school_color( stats->school );

    sc_js_t e;
    e.set( "color", c.str() );
    e.set( "y", util::round( 100.0 * stats->portion_amount, 1 ) );
    std::string name_str;
    if ( stats->player->is_pet() )
    {
      name_str += stats->player->name_str;
      name_str += "<br/>";
    }
    name_str += report::decorate_html_string( stats->name_str, c );

    e.set( "name", name_str );
    pc.add( "series.0.data", e );
  }

  return true;
}

bool chart::generate_damage_stats_sources( highchart::pie_chart_t& chart,
                                           const player_t& p )
{
  std::vector<stats_t*> stats_list;

  auto stats_filter = []( const stats_t* stat ) {
    if ( stat->quiet )
      return false;
    if ( stat->actual_amount.mean() <= 0 )
      return false;
    if ( stat->type != STATS_DMG )
      return false;
    return true;
  };

  range::copy_if( p.stats_list, std::back_inserter( stats_list ),
                  stats_filter );

  for ( const auto& pet : p.pet_list )
  {
    range::copy_if( pet->stats_list, std::back_inserter( stats_list ),
                    stats_filter );
  }

  range::sort( stats_list, compare_stats_by_mean );

  if ( stats_list.empty() )
    return false;

  generate_stats_sources( chart, p, p.name_str + " Damage Sources",
                          stats_list );
  chart.set( "series.0.name", "Damage" );
  chart.set( "plotOptions.pie.tooltip.pointFormat",
             "<span style=\"color:{point.color}\">\xE2\x97\x8F</span> "
             "{series.name}: <b>{point.y}</b>%<br/>" );
  return true;
}

bool chart::generate_heal_stats_sources( highchart::pie_chart_t& chart,
                                         const player_t& p )
{
  std::vector<stats_t*> stats_list;

  auto stats_filter = []( const stats_t* stat ) {
    if ( stat->quiet )
      return false;
    if ( stat->actual_amount.mean() <= 0 )
      return false;
    if ( stat->type == STATS_DMG )
      return false;
    return true;
  };

  range::copy_if( p.stats_list, std::back_inserter( stats_list ),
                  stats_filter );

  for ( const auto& pet : p.pet_list )
  {
    range::copy_if( pet->stats_list, std::back_inserter( stats_list ),
                    stats_filter );
  }

  if ( stats_list.empty() )
    return false;

  range::sort( stats_list, compare_stats_by_mean );

  generate_stats_sources( chart, p, p.name_str + " Healing Sources",
                          stats_list );

  return true;
}

bool chart::generate_raid_aps( highchart::bar_chart_t& bc, const sim_t& s,
                               const std::string& type )
{
  // Prepare list, based on the selected metric
  std::vector<const player_t*> player_list;
  std::string chart_name;

  // Fetch player data
  metric_e chart_metric =
      populate_player_list( type, s, player_list, chart_name );

  // Nothing to visualize
  if ( chart_metric == METRIC_NONE )
  {
    return false;
  }

  // Add a map of colors so we save some space in data
  add_color_data( bc, player_list );

  // Figure out what decimal precision we show in the charts. Only variance uses
  // a single digit at
  // the moment
  size_t precision = 0;
  if ( chart_metric == METRIC_VARIANCE )
  {
    precision = 1;
  }

  double max_value  = 0;
  bool has_diff     = false;
  size_t series_idx = 0;
  // Loop through all metric values we have available
  for ( metric_value_e vm = VALUE_MEAN; vm < VALUE_METRIC_MAX; ++vm )
  {
    // Don't output a chart for metric values for a given metric that don't make
    // sense
    if ( !( enabled_values[ chart_metric ] & ( 1 << vm ) ) )
    {
      continue;
    }

    // Define series (metric value) identifier as a number
    std::string series_id_str = util::to_string( series_idx++ );

    // Sort list of actors in the chart based on the value metric (and metric)
    range::sort( player_list, player_list_comparator_t( chart_metric, vm ) );
    double lowest_value = get_data_value( player_list.back() -> collected_data, chart_metric, vm );

    bool candlebars = false;
    // Iterate over the players and output data
    for ( const auto p : player_list )
    {
      const color::rgb& c = color::class_color( p->type );
      double value = get_data_value( p->collected_data, chart_metric, vm );

      // Keep track of largest value in all of the outputted charts so we can
      // adjust maxPadding
      if ( value > max_value )
      {
        max_value = value;
      }

      sc_js_t e;
      e.set( "color", c.str() );
      e.set( "name", p->name_str );
      e.set( "y",
             util::round( value, static_cast<unsigned int>( precision ) ) );
      e.set( "id", "#player" + util::to_string( p->index ) + "toggle" );

      // If lowest_value is defined, add relative difference (in percent) to the data
      if ( lowest_value > 0 )
      {
        has_diff = true;
        e.set( "reldiff", 100.0 * value / lowest_value - 100.0 );
      }

      bc.add( "__data." + series_id_str + ".series.0.data", e );

      if ( vm == VALUE_MEAN )
      {
        std::vector<double> boxplot_data = get_data_summary(
            p->collected_data, chart_metric, s.chart_boxplot_percentile );
        if ( boxplot_data[ 2 ] != 0 && boxplot_data[ 0 ] != boxplot_data[ 2 ] )
        {
          candlebars = true;
          sc_js_t v;
          v.set( "name", p->name_str );
          v.set( "low", boxplot_data[ 0 ] );
          v.set( "q1", boxplot_data[ 1 ] );
          v.set( "median", boxplot_data[ 5 ] );
          v.set( "mean", boxplot_data[ 2 ] );
          v.set( "q3", boxplot_data[ 3 ] );
          v.set( "high", boxplot_data[ 4 ] );
          v.set( "color", c.dark( .4 ).str() );
          v.set( "fillColor", c.dark( .75 ).opacity( 0.5 ).rgb_str() );

          bc.add( "__data." + series_id_str + ".series.1.data", v );
        }
      }
    }
    bc.add( "__data." + series_id_str + ".series.0.name", chart_name );

    bc.set( "__data." + series_id_str + ".title.text",
            get_metric_value_name( vm ) + " " + chart_name );
    // Configure candlebars
    if ( candlebars )
    {
      std::string base_str = "__data." + series_id_str + ".series.1.";
      bc.set( base_str + "type", "boxplot" );
      bc.set( base_str + "name", chart_name );
    }
  }

  // __current holds the current data set shown by the chart. Each mouse click
  // on the chart
  // increases this by one, wrapping it around at the end.
  if ( series_idx > 0 )
  {
    bc.set( "__current", 0 );
  }

  bc.height_ = 40 + player_list.size() * 24;

  bc.set( "yAxis.title.enabled", false );
  bc.set( "yAxis.gridLineWidth", 0 );
  bc.set( "yAxis.labels.enabled", false );
  bc.set( "yAxis.maxPadding", 0 );
  bc.set( "yAxis.minPadding", 0 );

  // Compute a very naive offset for the X-axis (in the chart Y-axis) labels,
  // and the dataLabels of
  // the chart.
  int n_chars = 0;
  size_t i    = 1;
  while ( max_value / i > 1 )
  {
    n_chars++;
    i *= 10;
  }

  // Thousands separator
  if ( n_chars > 3 )
  {
    n_chars++;
  }

  if ( s.chart_show_relative_difference && has_diff )
  {
    n_chars += 6;
  }

  if ( precision > 0 )
  {
    n_chars += 2;
  }

  bc.set( "chart.marginLeft", 315 );

  bc.set( "xAxis.lineWidth", 0 );
  bc.set( "xAxis.offset", 10 * n_chars );
  bc.set( "xAxis.maxPadding", 0 );
  bc.set( "xAxis.minPadding", 0 );

  // Theme the bar chart
  bc.set( "plotOptions.bar.dataLabels.crop", false );
  bc.set( "plotOptions.bar.dataLabels.overflow", "none" );
  bc.set( "plotOptions.bar.dataLabels.inside", true );
  bc.set( "plotOptions.bar.dataLabels.x", -10 * n_chars - 3 );
  bc.set( "plotOptions.bar.dataLabels.y", -1 );
  bc.set( "plotOptions.bar.dataLabels.padding", 0 );
  bc.set( "plotOptions.bar.dataLabels.enabled", true );
  bc.set( "plotOptions.bar.dataLabels.align", "left" );
  bc.set( "plotOptions.bar.dataLabels.verticalAlign", "middle" );
  bc.set( "plotOptions.bar.dataLabels.style.fontSize", "12px" );
  bc.set( "plotOptions.bar.dataLabels.style.fontWeight", "none" );
  bc.set( "plotOptions.bar.dataLabels.style.textShadow", "none" );

  // Theme the candlebars
  bc.set( "plotOptions.boxplot.whiskerLength", "80%" );
  bc.set( "plotOptions.boxplot.whiskerWidth", 1 );
  bc.set( "plotOptions.boxplot.medianWidth", 1 );
  bc.set( "plotOptions.boxplot.pointWidth", 18 );
  bc.set( "plotOptions.boxplot.tooltip.pointFormat",
    "Maximum: {point.high}<br/>"
    "Upper quartile: {point.q3}<br/>"
    "Mean: {point.mean:,.1f}<br/>"
    "Median: {point.median}<br/>"
    "Lower quartile: {point.q1}<br/>"
    "Minimum: {point.low}<br/>"
  );

  // X-axis label formatter, fetches colors from a (chart-local) table, instead
  // of writing out span
  // tags to the data
  std::string xaxis_label = "function() {";
  xaxis_label += "var formatted_name = this.value;";
  xaxis_label += "if ( formatted_name.length > 30 ) {";
  xaxis_label +=
      "formatted_name = formatted_name.substring(0, 30).trim() + "
      "'\xe2\x80\xa6';";
  xaxis_label += "}";
  xaxis_label +=
      "return '<span style=\"color:' + this.chart.options.__colors['C_' + this.value.replace('.', '_')] "
      "+ ';\">' + formatted_name + '</span>';";
  xaxis_label += " }";

  bc.set( "xAxis.labels.formatter", xaxis_label );
  bc.value( "xAxis.labels.formatter" ).SetRawOutput( true );

  // Data cycler, called on chart load to populate initial data, and on click to
  // populate next data
  // series to chart
  std::string dataset = "function(chart) {";
  dataset += "if ( chart.series.length !== 0 ) { ";
  dataset +=
      "chart.options.__current = ( chart.options.__current + 1 ) % "
      "chart.options.__data.length;";
  dataset += " }";
  dataset += "var cidx = chart.options.__current;";
  dataset += "var cdata = chart.options.__data[cidx];";
  dataset += "chart.setTitle(cdata.title, '', false);";
  dataset += "while ( chart.series.length !== 0 ) { ";
  dataset += "chart.series[0].remove(false);";
  dataset += " }";
  dataset += "for (var i = 0; i < cdata.series.length; ++i) {";
  dataset += "chart.addSeries(cdata.series[i], false);";
  dataset += " }";
  dataset += "chart.redraw();";
  dataset += " }";

  bc.set( "setter", dataset );
  bc.value( "setter" ).SetRawOutput( true );

  std::string loader = "function(event) {";
  loader += "this.options.setter(this);";
  loader += "}";

  bc.set( "chart.events.load", loader );
  bc.value( "chart.events.load" ).SetRawOutput( true );

  // Install the click handler only if we have more than one series output to
  // the chart
  if ( series_idx > 1 )
  {
    bc.set( "chart.events.click", loader );
    bc.value( "chart.events.click" ).SetRawOutput( true );
  }

  // If relative difference is used, print out a absolutevalue (relative
  // difference%) label
  std::string formatter = "function() {";
  if ( s.chart_show_relative_difference )
  {
    formatter +=
        "var fmt = '<span style=\"color:' + "
        "this.series.chart.options.__colors['C_' + this.point.name.replace('.', '_')] + ';\">';";
    formatter +=
        "if (this.point.reldiff === undefined || this.point.reldiff === 0) { "
        "fmt += Highcharts.numberFormat(this.point.y, " +
        util::to_string( precision ) + "); }";
    formatter += "else {";
    formatter += "  fmt += Highcharts.numberFormat(this.point.y, " +
                 util::to_string( precision ) + ") + ";
    formatter +=
        "         ' (' + Highcharts.numberFormat(this.point.reldiff, 2) + "
        "'%)';";
    formatter += "}";
    formatter += "fmt += '</span>';";
    formatter += "return fmt;";
    formatter += "}";
  }
  else
  {
    formatter +=
        "return '<span style=\"color:' + "
        "this.series.chart.options.__colors['C_' + this.point.name.replace('.', '_')] + ';\">' + "
        "Highcharts.numberFormat(this.y, " +
        util::to_string( precision ) + ") + '</span>';";
    formatter += " }";
  }

  bc.set( "plotOptions.bar.dataLabels.formatter", formatter );
  bc.value( "plotOptions.bar.dataLabels.formatter" ).SetRawOutput( true );

  // Bar click action, opens (and scrolls) to the player section clicked on
  std::string js = "function(event) {";
  js += "var anchor = jQuery(event.point.id);";
  js += "anchor.click();";
  js +=
      "jQuery('html, body').animate({ scrollTop: anchor.offset().top }, "
      "'slow');";
  js += "}";
  bc.set( "plotOptions.bar.events.click", js );
  bc.value( "plotOptions.bar.events.click" ).SetRawOutput( true );

  return true;
}

bool chart::generate_raid_dpet( highchart::bar_chart_t& bc, const sim_t& s )
{
  bc.set_title( "Raid Damage per Execute Time" );
  bc.set( "plotOptions.bar.pointWidth", 30 );
  bc.set( "xAxis.labels.y", -4 );

  // Prepare stats list
  std::vector<stats_t*> stats_list;
  for ( const auto& player : s.players_by_dps )
  {
    // Copy all stats* from p -> stats_list to stats_list, which satisfy the
    // filter
    range::remove_copy_if( player->stats_list, back_inserter( stats_list ),
                           filter_stats_dpet( *player ) );
  }
  range::sort( stats_list, []( const stats_t* l, const stats_t* r ) {
    if ( l->apet == r->apet )
    {
      return l->name_str < r->name_str;
    }
    return l->apet > r->apet;
  } );

  if ( stats_list.size() > 30 )
  {
    stats_list.erase( stats_list.begin() + 30, stats_list.end() );
  }

  auto log = stats_list.size() > 0 &&
             stats_list.back() -> apet > 0
             ? stats_list.front()->apet / stats_list.back()->apet >= 100
             : false;

  if ( log )
  {
    bc.set( "yAxis.type", "logarithmic" );
    bc.set_yaxis_title( "Damage per Execute Time (log)" );
    bc.set( "yAxis.minorTickInterval", .1 );
  }
  else
  {
    bc.set_yaxis_title( "Damage per Execute Time" );
  }

  bool ret = generate_apet( bc, stats_list );

  bc.height_ = 95 + stats_list.size() * 40;

  return ret;
}

bool chart::generate_apet( highchart::bar_chart_t& bc,
                           const std::vector<stats_t*>& stats_list )
{
  if ( stats_list.empty() )
  {
    return false;
  }

  size_t num_stats = stats_list.size();
  std::vector<player_t*> players;

  for ( const stats_t* stats : stats_list )
  {
    if ( stats->player->is_pet() || stats->player->is_enemy() )
    {
      continue;
    }

    if ( range::find( players, stats->player ) == players.end() )
    {
      players.push_back( stats->player );
    }
  }

  bc.height_ = 92 + num_stats * 22;

  for ( const stats_t* stats : stats_list )
  {
    const color::rgb& c = color::school_color( stats->school );

    sc_js_t e;
    e.set( "color", c.str() );
    std::string name_str = report::decorate_html_string( stats->name_str, c );
    if ( players.size() > 1 )
    {
      name_str += "<br/>(";
      name_str += stats->player->name();
      name_str += ")";
    }
    e.set( "name", name_str );
    e.set( "y", util::round( stats->apet, 1 ) );

    bc.add( "series.0.data", e );
  }

  bc.set( "series.0.name", "Amount per Execute Time" );

  return true;
}

bool chart::generate_action_dpet( highchart::bar_chart_t& bc,
                                  const player_t& p )
{
  std::vector<stats_t*> stats_list;

  // Copy all stats* from p -> stats_list to stats_list, which satisfy the
  // filter
  range::remove_copy_if( p.stats_list, back_inserter( stats_list ),
                         filter_stats_dpet( p ) );
  range::sort( stats_list, []( const stats_t* l, const stats_t* r ) {
    if ( l->apet == r->apet )
    {
      return l->name_str < r->name_str;
    }
    return l->apet > r->apet;
  } );

  if ( stats_list.empty() )
    return false;

  auto log = stats_list.back() -> apet > 0
             ? stats_list.front()->apet / stats_list.back()->apet >= 100
             : false;

  if ( log )
  {
    bc.set( "yAxis.type", "logarithmic" );
    bc.set_yaxis_title( "Damage per Execute Time (log)" );
    bc.set( "yAxis.minorTickInterval", .1 );
  }
  else
  {
    bc.set_yaxis_title( "Damage per Execute Time" );
  }

  bc.set_title( p.name_str + " Damage per Execute Time" );
  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    bc.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  generate_apet( bc, stats_list );

  return true;
}

bool chart::generate_scaling_plot( highchart::chart_t& ac, const player_t& p,
                                   scale_metric_e metric )
{
  double max_dps = 0, min_dps = std::numeric_limits<double>::max();

  for ( const auto& plot_data_list : p.dps_plot_data )
  {
    for ( const auto& plot_data : plot_data_list )
    {
      if ( plot_data.value > max_dps )
        max_dps = plot_data.value;
      if ( plot_data.value < min_dps )
        min_dps = plot_data.value;
    }
  }

  if ( max_dps <= 0 )
  {
    return false;
  }

  scaling_metric_data_t scaling_data = p.scaling_for_metric( metric );

  ac.set_title( scaling_data.name + " Scaling Plot" );
  ac.set_yaxis_title( util::scale_metric_type_string( metric ) );
  ac.set_xaxis_title( "Stat delta" );
  ac.set( "chart.type", "line" );
  ac.set( "legend.enabled", true );
  ac.set( "legend.margin", 5 );
  ac.set( "legend.padding", 0 );
  ac.set( "legend.itemMarginBottom", 5 );
  ac.height_ = 500;
  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    ac.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    const std::vector<plot_data_t>& pd = p.dps_plot_data[ i ];
    // Odds of metric value being 0 is pretty far, so lets just use that to
    // determine if we need to plot the stat or not
    if ( pd.empty() )
    {
      continue;
    }

    std::string color = color::stat_color( i );

    std::vector<std::pair<double, double> > data;

    ac.add( "colors", color );

    for ( const auto& pdata : pd )
    {
      data.push_back( std::make_pair(
          pdata.plot_step,
          util::round( pdata.value, p.sim->report_precision ) ) );
    }

    ac.add_simple_series( "", "", util::stat_type_abbrev( i ), data );
  }

  return true;
}

// chart::generate_scale_factors ===========================================

bool chart::generate_scale_factors( highchart::bar_chart_t& bc,
                                    const player_t& p, scale_metric_e metric )
{
  if ( p.scaling == nullptr )
  {
    return false;
  }

  std::vector<stat_e> scaling_stats;
  range::copy_if( p.scaling->scaling_stats[ metric ],
                  std::back_inserter( scaling_stats ),
                  [&]( const stat_e& stat ) { return p.scaling->scales_with[ stat ]; } );

  if ( scaling_stats.empty() )
  {
    return false;
  }

  scaling_metric_data_t scaling_data = p.scaling_for_metric( metric );

  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    bc.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  bc.set_title( scaling_data.name + " Scale Factors" );
  bc.height_ = 92 + scaling_stats.size() * 24;

  bc.set_yaxis_title( util::scale_metric_type_string( metric ) +
                      std::string( " per point" ) );

  // bc.set( "plotOptions.bar.dataLabels.align", "center" );
  bc.set( "plotOptions.errorbar.stemColor", "#FF0000" );
  bc.set( "plotOptions.errorbar.whiskerColor", "#FF0000" );
  bc.set( "plotOptions.errorbar.whiskerLength", "75%" );
  bc.set( "plotOptions.errorbar.whiskerWidth", 1.5 );

  std::vector<double> data;
  std::vector<std::pair<double, double> > error;
  for ( auto stat : scaling_stats )
  {
    double value = util::round( p.scaling->scaling[ metric ].get_stat( stat ),
                                p.sim->report_precision );
    double error_value = util::round(
        p.scaling->scaling_error[ metric ].get_stat( stat ), p.sim->report_precision );
    data.push_back( value );
    error.push_back( std::make_pair( value - fabs( error_value ),
                        value + fabs( error_value ) ) );

    std::string category_str = util::stat_type_abbrev( stat );
    category_str +=
        " (" + util::to_string( util::round( value, p.sim->report_precision ),
                                p.sim->report_precision ) +
        ")";

    bc.add( "xAxis.categories", category_str );
  }

  bc.add_simple_series(
      "bar", color::class_color( p.type ),
      util::scale_metric_type_abbrev( metric ) + std::string( " per point" ),
      data );
  bc.add_simple_series( "errorbar", "", "Error", error );

  return true;
}

// Generate a "standard" timeline highcharts object as a string based on a
// stats_t object
highchart::time_series_t& chart::generate_stats_timeline(
    highchart::time_series_t& ts, const stats_t& s )
{
  if ( s.timeline_amount == nullptr )
  {
    return ts;
  }

  sc_timeline_t timeline_aps;
  s.timeline_amount -> build_derivative_timeline( timeline_aps );
  std::string stats_type = util::stats_type_string( s.type );
  ts.set_toggle_id( "actor" + util::to_string( s.player->index ) + "_" +
                    s.name_str + "_" + stats_type + "_toggle" );

  ts.height_ = 200;
  ts.set( "yAxis.min", 0 );
  if ( s.type == STATS_DMG )
  {
    ts.set_yaxis_title( "Damage per second" );
    if ( s.action_list.size() > 0 && s.action_list[ 0 ]->data().id() != 0 )
      ts.set_title( std::string( s.action_list[ 0 ]->data().name_cstr() ) +
                    " Damage per second" );
    else
      ts.set_title( s.name_str + " Damage per second" );
  }
  else
    ts.set_yaxis_title( "Healing per second" );

  std::string area_color = color::YELLOW;
  if ( s.action_list.size() > 0 )
    area_color = color::school_color( s.action_list[ 0 ]->school );

  ts.add_simple_series( "area", area_color, s.type == STATS_DMG ? "DPS" : "HPS",
                        timeline_aps.data() );
  ts.set_mean(
      util::round( s.portion_aps.mean(), s.player->sim->report_precision ) );

  return ts;
}

bool chart::generate_actor_dps_series( highchart::time_series_t& ts,
                                       const player_t& p )
{
  if ( p.collected_data.dps.mean() <= 0 )
  {
    return false;
  }

  sc_timeline_t timeline_dps;
  p.collected_data.timeline_dmg.build_derivative_timeline( timeline_dps );
  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    ts.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }

  ts.set( "yAxis.min", 0 );
  ts.set_yaxis_title( "Damage per second" );
  ts.set_title( p.name_str + " Damage per second" );
  ts.add_simple_series( "area", color::class_color( p.type ), "DPS",
                        timeline_dps.data() );
  ts.set_mean(
      util::round( p.collected_data.dps.mean(), p.sim->report_precision ) );

  return true;
}

highchart::time_series_t& chart::generate_actor_timeline(
    highchart::time_series_t& ts, const player_t& p,
    const std::string& attribute, const std::string& series_color,
    const sc_timeline_t& data )
{
  std::string attr_str = util::inverse_tokenize( attribute );

  if ( p.sim->player_no_pet_list.size() > 1 )
  {
    ts.set_toggle_id( "player" + util::to_string( p.index ) + "toggle" );
  }
  ts.set_title( p.name_str + " " + attr_str );
  ts.set_yaxis_title( "Average " + attr_str );
  ts.add_simple_series( "area", series_color, attr_str, data.data() );
  if ( !p.sim->single_actor_batch )
  {
    ts.set_xaxis_max( p.sim->simulation_length.max() );
  }
  else
  {
    ts.set_xaxis_max( p.collected_data.fight_length.max() );
  }

  return ts;
}

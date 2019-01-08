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
<<<<<<< HEAD
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
=======
    // Copy all stats* from p -> stats_list to stats_list, which satisfy the filter
    range::remove_copy_if( p -> stats_list, back_inserter( stats_list ), filter_stats_dpet( *p ) );
  }

  size_t num_stats = stats_list.size();
  if ( num_stats == 0 ) return 0;

  range::sort( stats_list, compare_apet() );

  double max_dpet = stats_list[ 0 ] -> apet;

  size_t max_actions_per_chart = 20;
  size_t max_charts = 4;

  std::string s;

  for ( size_t chart = 0; chart < max_charts; chart++ )
  {
    if ( num_stats > max_actions_per_chart ) num_stats = max_actions_per_chart;

    s = get_chart_base_url();
    s += chart_size( 500, as<unsigned>( num_stats ) * 15 + ( chart == 0 ? 20 : -10 ) ); // Set chart size
    s += "cht=bhg";
    s += amp;
    s += fill_chart( FILL_BACKGROUND, FILL_SOLID, "333333" );
    s += "chbh=10";
    s += amp;
    s += "chd=t:";
    for ( size_t i = 0; i < num_stats; i++ )
    {
      stats_t* st = stats_list[ i ];
      str::format( s, "%s%.0f", ( i ? "|" : "" ), st -> apet );
    }
    s += amp;
    str::format( s, "chds=0,%.0f", max_dpet * 2.5 );
    s += amp;
    s += "chco=";
    for ( size_t i = 0; i < num_stats; i++ )
    {
      if ( i ) s += ",";
      s += color::school_color( stats_list[ i ] -> school ).hex_str();
    }
    s += amp;
    s += "chm=";
    for ( size_t i = 0; i < num_stats; i++ )
    {
      stats_t* st = stats_list[ i ];
      std::string formatted_name = st -> player -> name_str;
      util::urlencode( formatted_name );

      str::format( s, "%st++%.0f++%s+(%s),%s,%d,0,10", ( i ? "|" : "" ),
		   st -> apet, st -> name_str.c_str(), formatted_name.c_str(), 
		   get_color( *(st -> player) ).c_str(), ( int )i );
    }
    s += amp;
    if ( chart == 0 )
    {
      s += chart_title( "Raid Damage Per Execute Time" ); // Set chart title
    }

    s += chart_title_formatting( chart_bg_color(), 18 );

    images.push_back( s );

    stats_list.erase( stats_list.begin(), stats_list.begin() + num_stats );
    num_stats = stats_list.size();
    if ( num_stats == 0 ) break;
  }

  return images.size();
}

// chart::action_dpet =======================================================

std::string chart::action_dpet( const player_t& p )
{
  std::vector<stats_t*> stats_list;

  // Copy all stats* from p.stats_list to stats_list, which satisfy the filter
  range::remove_copy_if( p.stats_list, back_inserter( stats_list ), filter_stats_dpet( p ) );

  int num_stats = ( int ) stats_list.size();
  if ( num_stats == 0 )
    return std::string();

  range::sort( stats_list, compare_apet() );

  std::string formatted_name = util::google_image_chart_encode( p.name_str );
  util::urlencode( formatted_name );
  sc_chart chart( formatted_name + " Damage Per Execute Time", HORIZONTAL_BAR );
  chart.set_height( num_stats * 30 + 30 );

  std::string s = chart.create();
  s += "chd=t:";
  double max_apet = 0;
  for ( int i = 0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    str::format( s, "%s%.0f", ( i ? "|" : "" ), st -> apet );
    if ( st -> apet > max_apet ) max_apet = st -> apet;
  }
  s += amp;
  str::format( s, "chds=0,%.0f", max_apet * 2 );
  s += amp;
  s += "chco=";
  for ( int i = 0; i < num_stats; i++ )
  {
    if ( i ) s += ",";


    std::string school = color::school_color( stats_list[ i ] -> school ).hex_str();
    if ( school.empty() )
    {
      p.sim -> errorf( "chart_t::action_dpet assertion error! School color unknown, stats %s from %s. School %s\n", stats_list[ i ] -> name_str.c_str(), p.name(), util::school_type_string( stats_list[ i ] -> school ) );
      assert( 0 );
    }
    s += school;
  }
  s += amp;
  s += "chm=";
  for ( int i = 0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    str::format( s, "%st++%.0f++%s,%s,%d,0,15", ( i ? "|" : "" ), st -> apet, st -> name_str.c_str(), color::school_color( st -> school ).hex_str().c_str(), i );
  }
  s += amp;

  return s;
}

// chart::action_dmg ========================================================

std::string chart::aps_portion( const player_t& p )
{
  std::vector<stats_t*> stats_list;

  for ( size_t i = 0; i < p.stats_list.size(); ++i )
  {
    stats_t* st = p.stats_list[ i ];
    if ( st -> quiet ) continue;
    if ( st -> actual_amount.mean() <= 0 ) continue;
    if ( ( p.primary_role() == ROLE_HEAL ) != ( st -> type != STATS_DMG ) ) continue;
    stats_list.push_back( st );
  }

  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    pet_t* pet = p.pet_list[ i ];
    for ( size_t j = 0; j < pet -> stats_list.size(); ++j )
    {
      stats_t* st = pet -> stats_list[ j ];
      if ( st -> quiet ) continue;
      if ( st -> actual_amount.mean() <= 0 ) continue;
      if ( ( p.primary_role() == ROLE_HEAL ) != ( st -> type != STATS_DMG ) ) continue;
      stats_list.push_back( st );
    }
  }

  int num_stats = ( int ) stats_list.size();
  if ( num_stats == 0 )
    return std::string();

  range::sort( stats_list, compare_amount() );

  std::string formatted_name = util::google_image_chart_encode(  p.name() );
  util::urlencode( formatted_name );
  sc_chart chart( formatted_name + ( p.primary_role() == ROLE_HEAL ? " Healing" : " Damage" ) + " Sources", PIE );
  chart.set_height( 275 );

  std::string s = chart.create();
  s += "chd=t:";
  for ( int i = 0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    str::format( s, "%s%.0f", ( i ? "," : "" ), 100.0 * st -> actual_amount.mean() / ( ( p.primary_role() == ROLE_HEAL ) ? p.collected_data.heal.mean() : p.collected_data.dmg.mean() ) );
  }
  s += amp;
  s += "chds=0,100";
  s += amp;
  s += "chdls=ffffff";
  s += amp;
  s += "chco=";
  for ( int i = 0; i < num_stats; i++ )
  {
    if ( i ) s += ",";

    std::string school = color::school_color( stats_list[ i ] -> school ).hex_str();
    if ( school.empty() )
    {
      p.sim -> errorf( "chart_t::action_dmg assertion error! School unknown, stats %s from %s.\n", stats_list[ i ] -> name_str.c_str(), p.name() );
      assert( 0 );
    }
    s += school;

  }
  s += amp;
  s += "chl=";
  for ( int i = 0; i < num_stats; i++ )
  {
    if ( i ) s += "|";
    if ( stats_list[ i ] -> player -> type == PLAYER_PET || stats_list[ i ] -> player -> type == PLAYER_GUARDIAN )
    {
      s += stats_list[ i ] -> player -> name_str.c_str();
      s += ": ";
    }
    s += stats_list[ i ] -> name_str.c_str();
  }
  s += amp;

  return s;
}

// chart::spent_time ========================================================

std::string chart::time_spent( const player_t& p )
{
  std::vector<stats_t*> filtered_waiting_stats;

  // Filter stats we do not want in the chart ( quiet, background, zero total_time ) and copy them to filtered_waiting_stats
  range::remove_copy_if( p.stats_list, back_inserter( filtered_waiting_stats ), filter_waiting_stats() );

  size_t num_stats = filtered_waiting_stats.size();
  if ( num_stats == 0 && p.collected_data.waiting_time.mean() == 0 )
    return std::string();

  range::sort( filtered_waiting_stats, compare_stats_time() );

  std::string formatted_name = util::google_image_chart_encode(  p.name() );
  util::urlencode( formatted_name );
  sc_chart chart( formatted_name + " Spent Time", PIE );
  chart.set_height( 275 );

  std::string s = chart.create();

  s += "chd=t:";
  for ( size_t i = 0; i < num_stats; i++ )
  {
    str::format( s, "%s%.1f", ( i ? "," : "" ), 100.0 * filtered_waiting_stats[ i ] -> total_time.total_seconds() / p.collected_data.fight_length.mean() );

  }
  if ( p.collected_data.waiting_time.mean() > 0 )
  {
    str::format( s, "%s%.1f", ( num_stats > 0 ? "," : "" ), 100.0 * p.collected_data.waiting_time.mean() / p.collected_data.fight_length.mean() );

  }
  s += amp;
  s += "chds=0,100";
  s += amp;
  s += "chdls=ffffff";
  s += amp;
  s += "chco=";
  for ( size_t i = 0; i < num_stats; i++ )
  {
    if ( i ) s += ",";

    std::string school = color::school_color( filtered_waiting_stats[ i ] -> school ).hex_str();
    if ( school.empty() )
    {
      p.sim -> errorf( "chart_t::time_spent assertion error! School unknown, stats %s from %s.\n", filtered_waiting_stats[ i ] -> name_str.c_str(), p.name() );
      assert( 0 );
    }
    s += school;
  }
  if ( p.collected_data.waiting_time.mean() > 0 )
  {
    if ( num_stats > 0 ) s += ",";
    s += "ffffff";
  }
  s += amp;
  s += "chl=";
  for ( size_t i = 0; i < num_stats; i++ )
  {
    stats_t* st = filtered_waiting_stats[ i ];
    if ( i ) s += "|";
    s += st -> name_str.c_str();
    str::format( s, " %.1fs", st -> total_time.total_seconds() );
  }
  if ( p.collected_data.waiting_time.mean() > 0 )
  {
    if ( num_stats > 0 )s += "|";
    s += "waiting";
    str::format( s, " %.1fs", p.collected_data.waiting_time.mean() );
  }
  s += amp;

  return s;
}

// chart::gains =============================================================

std::string chart::gains( const player_t& p, resource_e type )
{
  std::vector<gain_t*> gains_list;

  double total_gain = 0;

  for ( size_t i = 0; i < p.gain_list.size(); ++i )
  {
    gain_t* g = p.gain_list[ i ];
    if ( g -> actual[ type ] <= 0 ) continue;
    total_gain += g -> actual[ type ];
    gains_list.push_back( g );
  }

  int num_gains = ( int ) gains_list.size();
  if ( num_gains == 0 )
    return std::string();

  range::sort( gains_list, compare_gain() );

  std::ostringstream s;
  s.setf( std::ios_base::fixed ); // Set fixed flag for floating point numbers

  std::string formatted_name = util::google_image_chart_encode( p.name_str );
  util::urlencode( formatted_name );
  std::string r = util::resource_type_string( type );
  util::inverse_tokenize( r );

  sc_chart chart( formatted_name + "+" + r + " Gains", PIE );
  chart.set_height( 200 + num_gains * 10 );

  s <<  chart.create();

  // Insert Chart Data
  s << "chd=t:";
  for ( int i = 0; i < num_gains; i++ )
  {
    gain_t* g = gains_list[ i ];
    s << ( i ? "," : "" );
    s << std::setprecision( p.sim -> report_precision / 2 ) << 100.0 * ( g -> actual[ type ] / total_gain );
  }
  s << "&amp;";

  // Chart scaling, may not be necessary if numbers are not shown
  s << "chds=0,100";
  s << amp;

  // Series color
  s << "chco=";
  s << color::resource_color( type ).hex_str();
  s << amp;

  // Labels
  s << "chl=";
  for ( int i = 0; i < num_gains; i++ )
  {
    if ( i ) s << "|";
    s << gains_list[ i ] -> name();
  }
  s << amp;

  return s.str();
}

// chart::scale_factors =====================================================

std::string chart::scale_factors( const player_t& p )
{
  std::vector<stat_e> scaling_stats;
  // load the data from whatever the default metric is set 
  // TODO: eventually show all data
  scale_metric_e sm = p.sim -> scaling -> scaling_metric;

  for ( std::vector<stat_e>::const_iterator it = p.scaling_stats[ sm ].begin(), end = p.scaling_stats[ sm ].end(); it != end; ++it )
  {
    if ( p.scales_with[ *it ] )
      scaling_stats.push_back( *it );
  }

  size_t num_scaling_stats = scaling_stats.size();
  if ( num_scaling_stats == 0 )
    return std::string();

  std::string formatted_name = util::google_image_chart_encode( p.scaling_for_metric( sm ).name );
  util::urlencode( formatted_name );

  sc_chart chart( "Scale Factors|" + formatted_name, HORIZONTAL_BAR );
  chart.set_height( static_cast<unsigned>( num_scaling_stats ) * 30 + 60 );

  std::string s = chart.create();

  str::format( s, "chd=t%i:" , 1 );
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p.scaling[ sm ].get_stat( scaling_stats[ i ] );
    str::format( s, "%s%.*f", ( i ? "," : "" ), p.sim -> report_precision, factor );
  }
  s += "|";

  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p.scaling[ sm ].get_stat( scaling_stats[ i ] ) - fabs( p.scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );

    str::format( s, "%s%.*f", ( i ? "," : "" ), p.sim -> report_precision, factor );
  }
  s += "|";
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p.scaling[ sm ].get_stat( scaling_stats[ i ] ) + fabs( p.scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );

    str::format( s, "%s%.*f", ( i ? "," : "" ), p.sim -> report_precision, factor );
  }
  s += amp;
  s += "chco=";
  s += get_color( p );
  s += amp;
  s += "chm=";
  s += "E,FF0000,1:0,,1:20|";
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p.scaling[ sm ].get_stat( scaling_stats[ i ] );
    const char* name = util::stat_type_abbrev( scaling_stats[ i ] );
    str::format( s, "%st++++%.*f++%s,%s,0,%d,15,0.1,%s", ( i ? "|" : "" ),
		 p.sim -> report_precision, factor, name, get_color( p ).c_str(),
		 ( int )i, factor > 0 ? "e" : "s" /* If scale factor is positive, position the text right of the bar, otherwise at the base */
		 );
  }
  s += amp;

  // Obtain lowest and highest scale factor values + error
  double lowest_value = 0, highest_value = 0;
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double value = p.scaling[ sm ].get_stat( scaling_stats[ i ] );
    double error = fabs( p.scaling_error[ sm ].get_stat( scaling_stats[ i ] ) );
    double high_value = std::max( value * 1.2, value + error ); // add enough space to display stat name
    if ( high_value > highest_value )
      highest_value = high_value;
    if ( value - error < lowest_value ) // it is intended that lowest_value will be <= 0
      lowest_value = value - error;
  }
  if ( lowest_value < 0 )
  { highest_value = std::max( highest_value, -lowest_value / 4 ); } // make sure we don't scre up the text
  s += "chds=" + util::to_string( lowest_value - 0.01 ) + "," + util::to_string( highest_value + 0.01 );;
  s += amp;

  return s;
}

// chart::scaling_dps =======================================================

std::string chart::scaling_dps( const player_t& p )
{
  double max_dps = 0, min_dps = std::numeric_limits<double>::max();

  for ( size_t i = 0; i < p.dps_plot_data.size(); ++i )
  {
    const std::vector<plot_data_t>& pd = p.dps_plot_data[ i ];
    size_t size = pd.size();
    for ( size_t j = 0; j < size; j++ )
    {
      if ( pd[ j ].value > max_dps ) max_dps = pd[ j ].value;
      if ( pd[ j ].value < min_dps ) min_dps = pd[ j ].value;
    }
  }
  if ( max_dps <= 0 )
    return std::string();

  double step = p.sim -> plot -> dps_plot_step;
  int range = p.sim -> plot -> dps_plot_points / 2;
  const int end = 2 * range;
  size_t num_points = 1 + 2 * range;

  scaling_metric_data_t scaling_data = p.scaling_for_metric( SCALE_METRIC_DPS );
  std::string formatted_name = util::google_image_chart_encode( scaling_data.name );
  util::urlencode( formatted_name );

  sc_chart chart( "Stat Scaling|" + formatted_name, LINE );
  chart.set_height( 300 );

  std::string s = chart.create();

  s += "chd=t:";
  bool first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    const std::vector<plot_data_t>& pd = p.dps_plot_data[ i ];
    size_t size = pd.size();
    if ( size == 0 )
    {
      continue;
    }

    if ( size != num_points ) continue;
    if ( ! first ) s += "|";
    for ( size_t j = 0; j < size; j++ )
    {
      str::format( s, "%s%.0f", ( j ? "," : "" ), pd[ j ].value );
    }
    first = false;
  }
  s += amp;
  str::format( s, "chds=%.0f,%.0f", min_dps, max_dps );
  s += amp;
  s += "chxt=x,y";
  s += amp;
  if ( p.sim -> plot -> dps_plot_positive )
  {
    const int start = 0;  // start and end only used for dps_plot_positive
    str::format( s, "chxl=0:|0|%%2b%.0f|%%2b%.0f|%%2b%.0f|%%2b%.0f|1:|%.0f|%.0f", ( start + ( 1.0 / 4 )*end )*step, ( start + ( 2.0 / 4 )*end )*step, ( start + ( 3.0 / 4 )*end )*step, ( start + end )*step, min_dps, max_dps );
  }
  else if ( p.sim -> plot -> dps_plot_negative )
  {
    const int start = (int) - ( p.sim -> plot -> dps_plot_points * p.sim -> plot -> dps_plot_step );
    str::format( s, "chxl=0:|%d|%.0f|%.0f|%.0f|0|1:|%.0f|%.0f", start, start * 0.75, start * 0.5, start * 0.25, min_dps, max_dps );
  }
  else
  {
    str::format( s, "chxl=0:|%.0f|%.0f|0|%%2b%.0f|%%2b%.0f|1:|%.0f|%.0f|%.0f", ( -range * step ), ( -range * step ) / 2, ( +range * step ) / 2, ( +range * step ), min_dps, p.collected_data.dps.mean(), max_dps );
    s += amp;
    str::format( s, "chxp=1,1,%.0f,100", 100.0 * ( p.collected_data.dps.mean() - min_dps ) / ( max_dps - min_dps ) );
  }
  s += amp;
  s += "chdl=";
  first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    size_t size = p.dps_plot_data[ i ].size();
    if ( size == 0 )
    {
      continue;
    }
    if ( size != num_points ) continue;
    if ( ! first ) s += "|";
    s += util::stat_type_abbrev( i );
    first = false;
  }
  s += amp;
  s += "chdls=dddddd,12";
  s += amp;
  s += "chco=";
  first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    size_t size = p.dps_plot_data[ i ].size();
    if ( size == 0 )
    {
      continue;
    }
    if ( size != num_points ) continue;
    if ( ! first ) s += ",";
    first = false;
    s += color::stat_color( i ).hex_str();
  }
  s += amp;
  str::format( s, "chg=%.4f,10,1,3", floor( 10000.0 * 100.0 / ( num_points - 1 ) ) / 10000.0 );
  s += amp;

  return s;
}

// chart::reforge_dps =======================================================

std::vector<std::pair<unsigned,std::string>> chart::reforge_dps( const player_t& p )
{
  std::vector<std::pair<unsigned,std::string>> result;
  for ( const auto& pd_pair : p.reforge_plot_data )
  {
    const auto& pd = pd_pair.second;
    const std::vector<stat_e>& stat_indices = pd_pair.first->reforge_plot_stat_indices;

    if ( pd.empty() )
      continue;

    double dps_range = 0.0, min_dps = std::numeric_limits<double>::max(), max_dps = 0.0;

    size_t num_stats = pd[ 0 ].size() - 1;
    if ( num_stats != 3 && num_stats != 2 )
    {
      p.sim -> errorf( "You must choose 2 or 3 stats to generate a reforge plot.\n" );
      return result;
    }

    for ( size_t i = 0; i < pd.size(); i++ )
    {
      assert( num_stats < pd[ i ].size() );
      if ( pd[ i ][ num_stats ].value < min_dps )
        min_dps = pd[ i ][ num_stats ].value;
      if ( pd[ i ][ num_stats ].value > max_dps )
        max_dps = pd[ i ][ num_stats ].value;
    }

    dps_range = max_dps - min_dps;

    std::ostringstream s;
    s.setf( std::ios_base::fixed ); // Set fixed flag for floating point numbers
    if ( num_stats == 2 )
    {
      int num_points = ( int ) pd.size();
      const plot_data_t& baseline = pd[ num_points / 2 ][ 2 ];
      double min_delta = baseline.value - ( min_dps - baseline.error / 2 );
      double max_delta = ( max_dps + baseline.error / 2 ) - baseline.value;
      double max_ydelta = std::max( min_delta, max_delta );
      int ysteps = 5;
      double ystep_amount = max_ydelta / ysteps;

      int negative_steps = 0, positive_steps = 0, positive_offset = -1;

      for ( int i = 0; i < num_points; i++ )
      {
        if ( pd[ i ][ 0 ].value < 0 )
          negative_steps++;
        else if ( pd[ i ][ 0 ].value > 0 )
        {
          if ( positive_offset == -1 )
            positive_offset = i;
          positive_steps++;
        }
      }

      // We want to fit about 4 labels per side, but if there's many plot points, have some sane
      int negative_mod = static_cast<int>( std::max( std::ceil( negative_steps / 4 ), 4.0 ) );
      int positive_mod = static_cast<int>( std::max( std::ceil( positive_steps / 4 ), 4.0 ) );

      scaling_metric_data_t scaling_data = p.scaling_for_metric( SCALE_METRIC_DPS );
      std::string formatted_name = util::google_image_chart_encode( scaling_data.name );
      util::urlencode( formatted_name );

      sc_chart chart( "Reforge Scaling|" + formatted_name, XY_LINE );
      chart.set_height( 400 );

      s << chart.create();

      // Generate reasonable X-axis labels in conjunction with the X data series.
      std::string xl1, xl2;

      // X series
      s << "chd=t2:";
      for ( int i = 0; i < num_points; i++ )
      {
        s << static_cast< int >( pd[ i ][ 0 ].value );
        if ( i < num_points - 1 )
          s << ",";

        bool label = false;
        // Label start
        if ( i == 0 )
          label = true;
        // Label end
        else if ( i == num_points - 1 )
          label = true;
        // Label baseline
        else if ( pd[ i ][ 0 ].value == 0 )
          label = true;
        // Label every negative_modth value (left side of baseline)
        else if ( pd[ i ][ 0 ].value < 0 && i % negative_mod == 0 )
          label = true;
        // Label every positive_modth value (right side of baseline), if there is
        // enough room until the end of the graph
        else if ( pd[ i ][ 0 ].value > 0 && i <= num_points - positive_mod && ( i - positive_offset ) > 0 && ( i - positive_offset + 1 ) % positive_mod == 0 )
          label = true;

        if ( label )
        {
          xl1 += util::to_string( pd[ i ][ 0 ].value );
          if ( i == 0 || i == num_points - 1 )
          {
            xl1 += "+";
            xl1 += util::stat_type_abbrev( stat_indices[ 0 ] );
          }

          xl2 += util::to_string( pd[ i ][ 1 ].value );
          if ( i == 0 || i == num_points - 1 )
          {
            xl2 += "+";
            xl2 += util::stat_type_abbrev( stat_indices[ 1 ] );
          }

        }
        // Otherwise, "fake" a label by adding simply a space. This is required
        // so that we can get asymmetric reforge ranges to correctly display the
        // baseline position on the X axis
        else
        {
          xl1 += "+";
          xl2 += "+";
        }

        if ( i < num_points - 1 )
          xl1 += "|";
        if ( i < num_points - 1 )
          xl2 += "|";
      }

      // Y series
      s << "|";
      for ( int i = 0; i < num_points; i++ )
      {
        s << static_cast< int >( pd[ i ][ 2 ].value );
        if ( i < num_points - 1 )
          s << ",";
      }

      // Min Y series
      s << "|-1|";
      for ( int i = 0; i < num_points; i++ )
      {
        s << static_cast< int >( pd[ i ][ 2 ].value - pd[ i ][ 2 ].error / 2 );
        if ( i < num_points - 1 )
          s << ",";
      }

      // Max Y series
      s << "|-1|";
      for ( int i = 0; i < num_points; i++ )
      {
        s << static_cast< int >( pd[ i ][ 2 ].value + pd[ i ][ 2 ].error / 2 );
        if ( i < num_points - 1 )
          s << ",";
      }

      s << amp;

      // Axis dimensions
      s << "chds=" << ( int ) pd[ 0 ][ 0 ].value << "," << ( int ) pd[ num_points - 1 ][ 0 ].value << "," << static_cast< int >( floor( baseline.value - max_ydelta ) ) << "," << static_cast< int >( ceil( baseline.value + max_ydelta ) );
      s << amp;

      s << "chxt=x,y,x";
      s << amp;

      // X Axis labels (generated above)
      s << "chxl=0:|" << xl1 << "|2:|" << xl2 << "|";

      // Y Axis labels
      s << "1:|";
      for ( int i = ysteps; i >= 1; i -= 1 )
      {
        s << ( int ) util::round( baseline.value - i * ystep_amount ) << " (" << - ( int ) util::round( i * ystep_amount ) << ")|";
      }
      s << static_cast< int >( baseline.value ) << "|";
      for ( int i = 1; i <= ysteps; i += 1 )
      {
        s << ( int ) util::round( baseline.value + i * ystep_amount ) << " (%2b" << ( int ) util::round( i * ystep_amount ) << ")";
        if ( i < ysteps )
          s << "|";
      }
      s << amp;

      // Chart legend
      s << "chdls=dddddd,12";
      s << amp;

      // Chart color
      s << "chco=";
      s << color::stat_color( stat_indices[ 0 ] ).hex_str();
      s << amp;

      // Grid lines
      s << "chg=" << util::to_string( 100 / ( 1.0 * num_points ) ) << ",";
      s << util::to_string( 100 / ( ysteps * 2 ) );
      s << ",3,3,0,0";
      s << amp;

      // Chart markers (Errorbars and Center-line)
      s << "chm=E,FF2222,1,-1,1:5|h,888888,1,0.5,1,-1.0";
    }
    else if ( num_stats == 3 )
    {
      if ( max_dps == 0 )
      {
        continue;
      }

      std::vector<std::vector<double> > triangle_points;
      std::vector< std::string > colors;
      for ( int i = 0; i < ( int ) pd.size(); i++ )
      {
        std::vector<plot_data_t> scaled_dps = pd[ i ];
        int ref_plot_amount = p.sim -> reforge_plot -> reforge_plot_amount;
        for ( int j = 0; j < 3; j++ )
          scaled_dps[ j ].value = ( scaled_dps[ j ].value + ref_plot_amount ) / ( 3. * ref_plot_amount );
        triangle_points.push_back( ternary_coords( scaled_dps ) );
        colors.push_back( color_temperature_gradient( pd[ i ][ 3 ].value, min_dps, dps_range ) );
      }

      s << "<form action='";
      s << get_chart_base_url();
      s << "' method='POST'>";
      s << "<input type='hidden' name='chs' value='525x425' />";
      s << "\n";
      s << "<input type='hidden' name='cht' value='s' />";
      s << "\n";
      s << "<input type='hidden' name='chf' value='bg,s,333333' />";
      s << "\n";

      s << "<input type='hidden' name='chd' value='t:";
      for ( size_t j = 0; j < 2; j++ )
      {
        for ( size_t i = 0; i < triangle_points.size(); i++ )
        {
          s << triangle_points[ i ][ j ];
          if ( i < triangle_points.size() - 1 )
            s << ",";
        }
        if ( j == 0 )
          s << "|";
      }
      s << "' />";
      s << "\n";
      s << "<input type='hidden' name='chco' value='";
      for ( int i = 0; i < ( int ) colors.size(); i++ )
      {
        s << colors[ i ];
        if ( i < ( int ) colors.size() - 1 )
          s << "|";
      }
      s << "' />\n";
      s << "<input type='hidden' name='chds' value='-0.1,1.1,-0.1,0.95' />";
      s << "\n";

      s << "<input type='hidden' name='chdls' value='dddddd,12' />";
      s << "\n";
      s << "\n";
      s << "<input type='hidden' name='chg' value='5,10,1,3'";
      s << "\n";
      std::string formatted_name = p.name_str;
      util::urlencode( formatted_name );
      s << "<input type='hidden' name='chtt' value='";
      s << formatted_name;
      s << "+Reforge+Scaling' />";
      s << "\n";
      s << "<input type='hidden' name='chts' value='dddddd,18' />";
      s << "\n";
      s << "<input type='hidden' name='chem' value='";
      s << "y;s=text_outline;d=FF9473,18,l,000000,_,";
      s << util::stat_type_string( stat_indices[ 0 ] );
      s << ";py=1.0;po=0.0,0.01;";
      s << "|y;s=text_outline;d=FF9473,18,r,000000,_,";
      s << util::stat_type_string( stat_indices[ 1 ] );
      s << ";py=1.0;po=1.0,0.01;";
      s << "|y;s=text_outline;d=FF9473,18,h,000000,_,";
      s << util::stat_type_string( stat_indices[ 2 ] );
      s << ";py=1.0;po=0.5,0.9' />";
      s << "\n";
      s << "<input type='submit' value='Get Reforge Plot Chart'>";
      s << "\n";
      s << "</form>";
      s << "\n";
    }
    std::pair<unsigned,std::string> p(stat_indices.size(), s.str());
    result.push_back( p );
  }
  return result;
}

// chart::timeline ==========================================================

std::string chart::timeline(  const std::vector<double>& timeline_data,
                              const std::string& timeline_name,
                              double avg,
                              std::string color,
                              size_t max_length )
{
  if ( timeline_data.empty() )
    return std::string();

  if ( max_length == 0 || max_length > timeline_data.size() )
    max_length = timeline_data.size();

  static const size_t max_points = 600;
  static const double encoding_range  = 60.0;

  size_t max_buckets = max_length;
  int increment = ( ( max_buckets > max_points ) ?
                    ( ( int ) floor( ( double ) max_buckets / max_points ) + 1 ) :
                    1 );

  double timeline_min = std::min( ( max_buckets ?
                                    *std::min_element( timeline_data.begin(), timeline_data.begin() + max_length ) :
                                    0.0 ), 0.0 );

  double timeline_max = ( max_buckets ?
                          *std::max_element( timeline_data.begin(), timeline_data.begin() + max_length ) :
                          0 );

  double timeline_range = timeline_max - timeline_min;

  double encoding_adjust = encoding_range / ( timeline_max - timeline_min );


  sc_chart chart( timeline_name + " Timeline", LINE );
  chart.set_height( 200 );

  std::ostringstream s;
  s << chart.create();
  char * old_locale = setlocale( LC_ALL, "C" );
  s << "chd=s:";
  for ( size_t i = 0; i < max_buckets; i += increment )
  {
    s << simple_encoding( ( int ) ( ( timeline_data[ i ] - timeline_min ) * encoding_adjust ) );
  }
  s << amp;

  s << "chco=" << color;
  s << amp;

  s << "chds=0," << util::to_string( encoding_range, 0 );
  s << amp;

  if ( avg || timeline_min < 0.0 )
  {
    s << "chm=h," << color::YELLOW.hex_str() << ",0," << ( avg - timeline_min ) / timeline_range << ",0.4";
    s << "|h," << color::RED.hex_str() << ",0," << ( 0 - timeline_min ) / timeline_range << ",0.4";
    s << amp;
  }

  s << "chxt=x,y";
  s << amp;

  std::ostringstream f; f.setf( std::ios::fixed ); f.precision( 0 );
  f << "chxl=0:|0|sec=" << util::to_string( max_buckets ) << "|1:|" << ( timeline_min < 0.0 ? "min=" : "" ) << timeline_min;
  if ( timeline_min < 0.0 )
    f << "|0";
  if ( avg )
    f << "|avg=" << util::to_string( avg, 0 );
  else f << "|";
  if ( timeline_max )
    f << "|max=" << util::to_string( timeline_max, 0 );
  s << f.str();
  s << amp;

  s << "chxp=1,1,";
  if ( timeline_min < 0.0 )
  {
    s << util::to_string( 100.0 * ( 0 - timeline_min ) / timeline_range, 0 );
    s << ",";
  }
  s << util::to_string( 100.0 * ( avg - timeline_min ) / timeline_range, 0 );
  s << ",100";

  setlocale( LC_ALL, old_locale );
  return s.str();
}

// chart::timeline_dps_error ================================================

std::string chart::timeline_dps_error( const player_t& p )
{
  static const size_t min_data_number = 50;
  size_t max_buckets = p.dps_convergence_error.size();
  if ( max_buckets <= min_data_number )
    return std::string();

  size_t max_points  = 600;
  size_t increment   = 1;

  if ( max_buckets > max_points )
  {
    increment = ( ( int ) floor( max_buckets / ( double ) max_points ) ) + 1;
  }

  double dps_max_error = *std::max_element( p.dps_convergence_error.begin() + min_data_number, p.dps_convergence_error.end() );
  double dps_range  = 60.0;
  double dps_adjust = dps_range / dps_max_error;

  sc_chart chart( "Standard Error Confidence ( n >= 50 )", LINE );
  chart.set_height( 185 );

  std::string s = chart.create();

  s += "chco=FF0000,0000FF";
  s += amp;
  s += "chd=s:";
  for ( size_t i = 0; i < max_buckets; i += increment )
  {
    if ( i < min_data_number )
      s += simple_encoding( 0 );
    else
      s += simple_encoding( ( int ) ( p.dps_convergence_error[ i ] * dps_adjust ) );
  }
  s += amp;
  s += "chxt=x,y";
  s += amp;
  s += "chm=";
  for ( unsigned i = 1; i <= 5; i++ )
  {
    unsigned j = ( int ) ( ( max_buckets / 5 ) * i );
    if ( !j ) continue;
    if ( j >= max_buckets ) j = as<unsigned>( max_buckets - 1 );
    if ( i > 1 ) s += "|";
    str::format( s, "t%.1f,FFFFFF,0,%d,10", p.dps_convergence_error[ j ], int( j / increment ) );

>>>>>>> 1c5f9bd6725cdfece4184bf1f8645dc1aab69b9c
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

bool chart::generate_reforge_plot( highchart::chart_t& ac, const player_t& p, const std::pair<const reforge_plot_run_t*,std::vector<std::vector<plot_data_t>>>& plot_data )
{
  const auto& pd = plot_data.second;
  const auto& reforge_plot_stat_indices = plot_data.first->reforge_plot_stat_indices;
  if ( pd.empty() )
  {
    return false;
  }

<<<<<<< HEAD
  double max_dps   = 0.0;
  double min_dps   = std::numeric_limits<double>::max();
  double baseline  = 0.0;
  size_t num_stats = p.reforge_plot_data.front().size() - 1;
=======
  double max_dps = 0, min_dps = std::numeric_limits<double>::max(), baseline = 0;
  size_t num_stats = pd.front().size() - 1;
>>>>>>> 1c5f9bd6725cdfece4184bf1f8645dc1aab69b9c

  for ( const auto& pdata : pd )
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
<<<<<<< HEAD
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
=======
  from_stat = util::stat_type_abbrev( reforge_plot_stat_indices[ 0 ] );
  to_stat = util::stat_type_abbrev( reforge_plot_stat_indices[ 1 ] );
  from_color = color::stat_color( reforge_plot_stat_indices[ 0 ] );
  to_color = color::stat_color( reforge_plot_stat_indices[ 1 ] );

  std::string span_from_stat = "<span style=\"color:" + from_color + ";font-weight:bold;\">" + from_stat + "</span>";
  std::string span_from_stat_abbrev = "<span style=\"color:" + from_color + ";font-weight:bold;\">" + from_stat.substr( 0, 2 ) + "</span>";
  std::string span_to_stat = "<span style=\"color:" + to_color + ";font-weight:bold;\">" + to_stat + "</span>";
  std::string span_to_stat_abbrev = "<span style=\"color:" + to_color + ";font-weight:bold;\">" + to_stat.substr( 0, 2 ) + "</span>";
>>>>>>> 1c5f9bd6725cdfece4184bf1f8645dc1aab69b9c

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

  for ( const auto& pdata : pd )
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

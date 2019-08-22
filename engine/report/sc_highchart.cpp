#include "simulationcraft.hpp"

#include "sc_highchart.hpp"

namespace
{
// static const char* TEXT_OUTLINE      = "-1px -1px 0 #000000, 1px -1px 0
// #000000, -1px 1px 0 #000000, 1px 1px 0 #000000";
static const char* TEXT_COLOR     = "#CACACA";
static const char* TEXT_COLOR_ALT = "black";

static const char* SUBTEXT_COLOR = "#ACACAC";
static const char* SUBTEXT_COLOR_ALT = "grey";

static const char* TEXT_MEAN_COLOR = "#CC8888";
static const char* TEXT_MAX_COLOR  = "#8888CC";

static const char* CHART_BGCOLOR     = "#242424";
static const char* CHART_BGCOLOR_ALT = "white";
}

using namespace js;
using namespace highchart;

sc_js_t& highchart::theme( sc_js_t& json, highchart_theme_e theme )
{
  std::string _bg_color = theme == THEME_DEFAULT ? CHART_BGCOLOR : CHART_BGCOLOR_ALT;
  std::string _text_color = theme == THEME_DEFAULT ? TEXT_COLOR : TEXT_COLOR_ALT;
  std::string _subtext_color = theme == THEME_DEFAULT ? SUBTEXT_COLOR : SUBTEXT_COLOR_ALT;

  json.set( "credits", false );

  json.set( "lang.decimalPoint", "." );
  json.set( "lang.thousandsSep", "," );

  json.set( "legend.enabled", false );
  json.set( "legend.itemStyle.fontSize", "14px" );
  json.set( "legend.itemStyle.color", _text_color );
  // json.set( "legend.itemStyle.textShadow", TEXT_OUTLINE );

  json.set( "chart.borderRadius", 4 );
  json.set( "chart.backgroundColor", _bg_color );
  json.set( "chart.style.fontSize", "13px" );
  json.add( "chart.spacing", 10 )
      .add( "chart.spacing", 10 )
      .add( "chart.spacing", 10 )
      .add( "chart.spacing", 10 );

  json.set( "xAxis.lineColor", _text_color );
  json.set( "xAxis.tickColor", _text_color );
  json.set( "xAxis.title.style.color", _text_color );
  // json.set( "xAxis.title.style.textShadow", TEXT_OUTLINE );
  json.set( "xAxis.labels.style.color", _text_color );
  json.set( "xAxis.labels.style.fontSize", "14px" );
  // json.set( "xAxis.labels.style.textShadow", TEXT_OUTLINE );

  json.set( "yAxis.lineColor", _text_color );
  json.set( "yAxis.tickColor", _text_color );
  json.set( "yAxis.title.style.color", _text_color );
  // json.set( "yAxis.title.style.textShadow", TEXT_OUTLINE );
  json.set( "yAxis.labels.style.color", _text_color );
  json.set( "yAxis.labels.style.fontSize", "14px" );
  // json.set( "yAxis.labels.style.textShadow", TEXT_OUTLINE );

  json.set( "title.style.fontSize", "15px" );
  json.set( "title.style.color", _text_color );
  json.set( "title.style.textOverflow", "ellipsis" );
  // json.set( "title.style.textShadow", TEXT_OUTLINE );

  json.set( "subtitle.style.fontSize", "13px" );
  json.set( "subtitle.style.color", _subtext_color );
  // json.set( "subtitle.style.textShadow", TEXT_OUTLINE );

  json.set( "tooltip.backgroundColor", "#3F3E38" );
  json.set( "tooltip.style.color", _text_color );

  json.set( "plotOptions.series.shadow", true );
  json.set( "plotOptions.series.dataLabels.style.color", _text_color );
  // json.set( "plotOptions.series.dataLabels.style.textShadow", TEXT_OUTLINE );

  json.set( "plotOptions.pie.dataLabels.enabled", true );
  json.set( "plotOptions.pie.dataLabels.style.fontWeight", "none" );
  json.set( "plotOptions.pie.fillOpacity", 0.2 );

  json.set( "plotOptions.bar.borderWidth", 0 );
  json.set( "plotOptions.bar.pointWidth", 18 );

  json.set( "plotOptions.column.borderWidth", 0 );
  json.set( "plotOptions.column.pointWidth", 8 );

  json.set( "plotOptions.area.lineWidth", 1.25 );
  json.set( "plotOptions.area.states.hover.lineWidth", 1 );
  json.set( "plotOptions.area.fillOpacity", 0.2 );

  json.set( "tooltip.valueDecimals", 1 );

  return json;
}

std::string highchart::build_id( const stats_t& stats,
                                 const std::string& suffix )
{
  std::string s;

  s += "actor" + util::to_string( stats.player->index );
  s += "_" + util::remove_special_chars( stats.name_str );
  s += "_";
  s += util::stats_type_string( stats.type );
  s += suffix;

  return s;
}

std::string highchart::build_id( const player_t& actor,
                                 const std::string& suffix )
{
  std::string s = "actor" + util::to_string( actor.index );
  s += suffix;
  return s;
}

std::string highchart::build_id( const buff_t& buff, const std::string& suffix )
{
  std::string s = "buff_" + util::remove_special_chars( buff.name_str );
  if ( buff.player )
    s += "_actor" + util::to_string( buff.player->index );

  s += suffix;
  return s;
}

// Init default (shared) json structure
chart_t::chart_t( const std::string& id_str, const sim_t& sim )
  : sc_js_t(), id_str_( id_str ), height_( 250 ), width_( 575 ), sim_( sim )
{
  assert( !id_str_.empty() );
}

std::string chart_t::to_target_div() const
{
  std::string str_ = "<div class=\"charts\" id=\"" + id_str_ + "\"";
  str_ += " style=\"min-width: " + util::to_string( width_ ) + "px;";
  if ( height_ > 0 )
    str_ += " min-height: " + util::to_string( height_ ) + "px;";
  str_ += "\"></div>\n";

  return str_;
}

std::string chart_t::to_data() const
{
  rapidjson::StringBuffer b;
  sc_json_writer_t<rapidjson::StringBuffer> writer( b, sim_ );

  js_.Accept( writer );
  std::string str_ = "{ \"target\": \"" + id_str_ + "\", \"data\": ";
  str_ += b.GetString();
  str_ += " }\n";

  return str_;
}

std::string chart_t::to_aggregate_string( bool on_click ) const
{
  rapidjson::StringBuffer b;
  sc_json_writer_t<rapidjson::StringBuffer> writer( b, sim_ );

  js_.Accept( writer );
  std::string javascript = b.GetString();

  std::string str_;
  if ( on_click )
  {
    assert( !toggle_id_str_.empty() );
    str_ += "$('#" + toggle_id_str_ + "').one('click', function() {\n";
    // str_ += "console.log(\"Loading " + id_str_ + ": " + toggle_id_str_ + "
    // ...\" );\n";
  }
  str_ += "$('#" + id_str_ + "').highcharts(";
  str_ += javascript;
  str_ += ");\n";
  if ( on_click )
  {
    str_ += "});\n";
  }

  return str_;
}

std::string chart_t::to_string() const
{
  rapidjson::StringBuffer b;
  sc_json_writer_t<rapidjson::StringBuffer> writer( b, sim_ );

  js_.Accept( writer );
  std::string javascript = b.GetString();
  javascript.erase( std::remove( javascript.begin(), javascript.end(), '\n' ),
                    javascript.end() );
  std::string str_ = "<div class=\"charts\" id=\"" + id_str_ + "\"";
  str_ += " style=\"min-width: " + util::to_string( width_ ) + "px;";
  if ( height_ > 0 )
    str_ += " height: " + util::to_string( height_ ) + "px;";
  str_ += "\"></div>\n";
  str_ += "<script type=\"text/javascript\">\n";
  if ( !toggle_id_str_.empty() )
  {
    str_ += "jQuery( document ).ready( function( $ ) {\n";
    str_ += "$('#" + toggle_id_str_ + "').on('click', function() {\n";
    str_ += "console.log(\"Loading " + id_str_ + ": " + toggle_id_str_ + " ...\" );\n";
    str_ += "$('#" + id_str_ + "').highcharts(";
    str_ += javascript;
    str_ += ");\n});\n});\n";
  }
  else
  {
    str_ += "jQuery('#" + id_str_ + "').highcharts(";
    str_ += javascript;
    str_ += ");\n";
  }
  str_ += "</script>\n";

  return str_;
}

std::string chart_t::to_xml() const
{
  rapidjson::StringBuffer b;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( b );

  js_.Accept( writer );

  std::string str = "<!CDATA[";
  str += b.GetString();
  str += "]]>";

  return str;
}

void chart_t::set_xaxis_max( double max )
{
  set( "xAxis.max", max );
}

void chart_t::set_xaxis_title( const std::string& label )
{
  set( "xAxis.title.text", label );
}

void chart_t::set_yaxis_title( const std::string& label )
{
  set( "yAxis.title.text", label );
}

void chart_t::set_title( const std::string& title )
{
  set( "title.text", title );
}

void chart_t::add_data_series( const std::string& type, const std::string& name,
                               std::vector<sc_js_t>& d )
{
  rapidjson::Value obj( rapidjson::kObjectType );

  if ( !type.empty() )
    set( obj, "type", type );

  if ( !name.empty() )
    set( obj, "name", name );

  rapidjson::Value data( rapidjson::kArrayType );

  for ( size_t i = 0; i < d.size(); ++i )
  {
    data.PushBack( static_cast<rapidjson::Value&>( d[ i ].js_ ),
                   js_.GetAllocator() );
  }

  obj.AddMember( "data", data, js_.GetAllocator() );
  add( "series", obj );
}

void chart_t::add_data_series( std::vector<sc_js_t>& d )
{
  add_data_series( "", "", d );
}

void chart_t::add_simple_series( const std::string& type,
                                 const std::string& color,
                                 const std::string& name,
                                 const std::vector<data_triple_t>& series )
{
  rapidjson::Value obj( rapidjson::kObjectType );

  if ( !type.empty() )
  {
    set( obj, "type", type );
  }

  if ( !name.empty() )
  {
    set( obj, "name", name );
  }

  rapidjson::Value arr( rapidjson::kArrayType );
  for ( auto& serie : series )
  {
    rapidjson::Value pair( rapidjson::kArrayType );
    pair.PushBack( serie.x_, js_.GetAllocator() );
    pair.PushBack( serie.v1_, js_.GetAllocator() );
    pair.PushBack( serie.v2_, js_.GetAllocator() );

    arr.PushBack( pair, js_.GetAllocator() );
  }

  set( obj, "data", arr );

  if ( !color.empty() )
  {
    std::string color_hex = color;
    if ( color_hex[ 0 ] != '#' )
      color_hex = '#' + color_hex;

    add( "colors", color_hex );
  }

  add( "series", obj );
}

void chart_t::add_simple_series(
    const std::string& type, const std::string& color, const std::string& name,
    const std::vector<std::pair<double, double> >& series )
{
  rapidjson::Value obj( rapidjson::kObjectType );

  if ( !type.empty() )
  {
    set( obj, "type", type );
  }

  if ( !name.empty() )
  {
    set( obj, "name", name );
  }

  rapidjson::Value arr( rapidjson::kArrayType );
  for ( auto& serie : series )
  {
    rapidjson::Value pair( rapidjson::kArrayType );
    pair.PushBack( serie.first, js_.GetAllocator() );
    pair.PushBack( serie.second, js_.GetAllocator() );

    arr.PushBack( pair, js_.GetAllocator() );
  }

  set( obj, "data", arr );

  if ( !color.empty() )
  {
    std::string color_hex = color;
    if ( color_hex[ 0 ] != '#' )
      color_hex = '#' + color_hex;

    add( "colors", color_hex );
  }

  add( "series", obj );
}

void chart_t::add_simple_series( const std::string& type,
                                 const std::string& color,
                                 const std::string& name,
                                 const std::vector<double>& series )
{
  rapidjson::Value obj( rapidjson::kObjectType );

  set( obj, "type", type );
  set( obj, "data", series );
  set( obj, "name", name );

  if ( !color.empty() )
  {
    std::string color_hex = color;
    if ( color_hex[ 0 ] != '#' )
      color_hex = '#' + color_hex;
    add( "colors", color_hex );
  }

  add( "series", obj );
}

/**
 * Add y-axis plotline to the chart at value_ height. If name is given, a
 * subtitle will be added to the chart, stating name_=value_. Both the line and
 * the text use color_. line_width_ sets the line width of the plotline. If
 * there are multiple plotlines defined, the subtitle text will be concatenated
 * at the end.
 */
chart_t& chart_t::add_yplotline( double value_, const std::string& name_,
                                 double line_width_, const std::string& color_ )
{
  if ( rapidjson::Value* obj = path_value( "yAxis.plotLines" ) )
  {
    if ( obj->GetType() != rapidjson::kArrayType )
      obj->SetArray();

    std::string color = color_;
    if ( color.empty() )
      color = "#FFFFFF";

    if ( rapidjson::Value* text_v = path_value( "subtitle.text" ) )
    {
      std::string mean_str = "<span style=\"color: ";
      mean_str += color;
      mean_str += ";\">";
      if ( !name_.empty() )
        mean_str += name_ + "=";
      mean_str += util::to_string( value_ );
      mean_str += "</span>";

      if ( text_v->GetType() == rapidjson::kStringType )
        mean_str = std::string( text_v->GetString() ) + ", " + mean_str;

      text_v->SetString( mean_str.c_str(),
                         static_cast<rapidjson::SizeType>( mean_str.size() ),
                         js_.GetAllocator() );
    }

    rapidjson::Value new_obj( rapidjson::kObjectType );
    set( new_obj, "color", color );
    set( new_obj, "value", value_ );
    set( new_obj, "width", line_width_ );
    set( new_obj, "zIndex", 5 );

    obj->PushBack( new_obj, js_.GetAllocator() );
  }

  return *this;
}

time_series_t::time_series_t( const std::string& id_str, const sim_t& sim )
  : chart_t( id_str, sim )
{
  set( "chart.type", "area" );

  set( "tooltip.headerFormat", "Second: {point.key}<br/>" );
  set_xaxis_title( "Time (seconds)" );
}

time_series_t& time_series_t::set_mean( double value_,
                                        const std::string& color )
{
  std::string mean_color = color;
  if ( mean_color.empty() )
    mean_color = TEXT_MEAN_COLOR;

  add_yplotline( value_, "mean", 1.25, mean_color );
  return *this;
}

time_series_t& time_series_t::set_max( double value_, const std::string& color )
{
  std::string max_color = color;
  if ( max_color.empty() )
    max_color = TEXT_MAX_COLOR;

  add_yplotline( value_, "max", 1.25, max_color );
  return *this;
}

bar_chart_t::bar_chart_t( const std::string& id_str, const sim_t& sim )
  : chart_t( id_str, sim )
{
  set( "chart.type", "bar" );

  set( "xAxis.tickLength", 0 );
  set( "xAxis.type", "category" );
  set( "xAxis.labels.step", 1 );
  set( "xAxis.labels.y", 4 );
  set( "xAxis.offset", -10 );
}

pie_chart_t::pie_chart_t( const std::string& id_str, const sim_t& sim )
  : chart_t( id_str, sim )
{
  height_ = 300;  // Default Pie Chart height

  set( "chart.type", "pie" );

  // set( "plotOptions.bar.states.hover.lineWidth", 1 );
}

histogram_chart_t::histogram_chart_t( const std::string& id_str,
                                      const sim_t& sim )
  : chart_t( id_str, sim )
{
  height_ = 300;

  set( "chart.type", "column" );

  set( "xAxis.tickLength", 0 );
  set( "xAxis.type", "category" );
}

template <typename Stream>
sc_json_writer_t<Stream>::sc_json_writer_t( Stream& stream, const sim_t& s )
  : rapidjson::Writer<Stream>( stream ), sim( s )
{
}

template <typename Stream>
bool sc_json_writer_t<Stream>::Double( double d )
{
  this->Prefix( rapidjson::kNumberType );
  fmt::memory_buffer buffer;
  fmt::format_to(buffer, "{:.{}f}", d, sim.report_precision );
  for ( unsigned i = 0; i < buffer.size(); ++i )
  {
    this->os_->Put( buffer.data()[ i ] );
  }
  return true;
}

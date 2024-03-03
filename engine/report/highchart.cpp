// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "highchart.hpp"

#include "buff/buff.hpp"
#include "fmt/format.h"
#include "player/player.hpp"
#include "player/stats.hpp"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "sim/sim.hpp"
#include "util/util.hpp"
#include <iterator>

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


// Custom data formatter, we need to output doubles in a different way to save some room.
template <typename Stream>
struct sc_json_writer_t : public rapidjson::Writer<Stream>
{
  const sim_t& sim;

  sc_json_writer_t(Stream& stream, const sim_t& s);
  bool Double(double d);
};

template <typename Stream>
sc_json_writer_t<Stream>::sc_json_writer_t(Stream& stream, const sim_t& s)
  : rapidjson::Writer<Stream>(stream), sim(s)
{
}

template <typename Stream>
bool sc_json_writer_t<Stream>::Double(double d)
{
  this->Prefix(rapidjson::kNumberType);
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "{:.{}f}", d, sim.report_precision);
  auto start = this->os_->Push( buffer.size() );
  std::copy( std::begin( buffer ), std::end( buffer ), start );
  return true;
}
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
  json.set( "chart.style.fontFamily", "'Lucida Grande', 'Lucida Sans Unicode', Arial, Helvetica, sans-serif" );
  json.set( "chart.style.fontSize", "13px" );
  json.add( "chart.spacing", 10 )
      .add( "chart.spacing", 10 )
      .add( "chart.spacing", 10 )
      .add( "chart.spacing", 10 );

  json.set( "xAxis.lineColor", _text_color );
  json.set( "xAxis.tickColor", _text_color );
  json.set( "xAxis.title.style.color", _text_color );
  json.set( "xAxis.title.style.fontSize", "13px" );
  // json.set( "xAxis.title.style.textShadow", TEXT_OUTLINE );
  json.set( "xAxis.labels.style.color", _text_color );
  json.set( "xAxis.labels.style.fontSize", "14px" );
  // json.set( "xAxis.labels.style.textShadow", TEXT_OUTLINE );

  json.set( "yAxis.lineColor", _text_color );
  json.set( "yAxis.tickColor", _text_color );
  json.set( "yAxis.title.style.color", _text_color );
  json.set( "yAxis.title.style.fontSize", "13px" );
  // json.set( "yAxis.title.style.textShadow", TEXT_OUTLINE );
  json.set( "yAxis.labels.style.color", _text_color );
  json.set( "yAxis.labels.style.fontSize", "14px" );
  // json.set( "yAxis.labels.style.textShadow", TEXT_OUTLINE );

  json.set( "title.style.fontSize", "15px" );
  json.set( "title.style.fontWeight", "normal" );
  json.set( "title.style.color", _text_color );
  json.set( "title.style.textOverflow", "ellipsis" );
  // json.set( "title.style.textShadow", TEXT_OUTLINE );

  json.set( "subtitle.style.fontSize", "13px" );
  json.set( "subtitle.style.color", _subtext_color );
  // json.set( "subtitle.style.textShadow", TEXT_OUTLINE );

  json.set( "tooltip.backgroundColor", "#3F3E38" );
  json.set( "tooltip.borderWidth", 1 );
  json.set( "tooltip.style.fontSize", "12px" );
  json.set( "tooltip.style.color", _text_color );

  json.set( "plotOptions.series.shadow", true );
  json.set( "plotOptions.series.tooltip.followPointer", true );
  json.set( "plotOptions.series.dataLabels.style.color", _text_color );
  json.set( "plotOptions.series.dataLabels.style.fontSize", "12px" );
  json.set( "plotOptions.series.dataLabels.style.fontWeight", "normal" );
  json.set( "plotOptions.series.dataLabels.style.textOutline", "none" );
  // json.set( "plotOptions.series.dataLabels.style.textShadow", TEXT_OUTLINE );

  json.set( "plotOptions.pie.dataLabels.enabled", true );
  json.set( "plotOptions.pie.fillOpacity", 0.2 );
  json.set( "plotOptions.pie.borderRadius", 0 );

  json.set( "plotOptions.bar.borderWidth", 0 );
  json.set( "plotOptions.bar.borderRadius", 0 );
  json.set( "plotOptions.bar.pointWidth", 18 );

  json.set( "plotOptions.column.borderWidth", 0 );
  json.set( "plotOptions.column.borderRadius", 0 );
  json.set( "plotOptions.column.pointWidth", 8 );

  json.set( "plotOptions.area.lineWidth", 1.25 );
  json.set( "plotOptions.area.states.hover.lineWidth", 1 );
  json.set( "plotOptions.area.fillOpacity", 0.2 );

  json.set( "tooltip.valueDecimals", 1 );

  return json;
}

std::string highchart::build_id( const stats_t& stats,
                                 std::string_view suffix )
{
  return fmt::format( "actor{}_{}_{}{}", stats.player->index, util::remove_special_chars( stats.name_str ), stats.type,
                      suffix );
}

std::string highchart::build_id( const player_t& actor,
                                 std::string_view suffix )
{
  return fmt::format( "actor{}{}", actor.index, suffix );
}

std::string highchart::build_id( const buff_t& buff, std::string_view suffix )
{
  return fmt::format( "buff_{}{}{}{}", util::remove_special_chars( buff.name_str ),
                      buff.player ? fmt::format( "_actor{}", buff.player->index ) : "",
                      buff.source ? fmt::format( "_source{}", buff.source->index ) : "", suffix );
}

// Init default (shared) json structure
chart_t::chart_t( std::string id_str, const sim_t& sim )
  : sc_js_t(), id_str_( std::move( id_str ) ), height_( 250 ), width_( 575 ), sim_( sim )
{
  assert( !id_str_.empty() );
}

std::string chart_t::to_target_div() const
{
  return fmt::format( "<div class=\"charts\" id=\"{}\" style=\"min-width: {}px;{}\"></div>\n", id_str_, width_,
                      height_ > 0 ? fmt::format( " min-height: {}px;", height_ ) : "" );
}

std::string chart_t::to_data() const
{
  rapidjson::StringBuffer b;
  sc_json_writer_t<rapidjson::StringBuffer> writer( b, sim_ );

  js_.Accept( writer );
  return fmt::format( "{{ \"target\": \"{}\", \"data\": {} }}\n", id_str_, b.GetStringView() );
}

std::string chart_t::to_aggregate_string( bool on_click ) const
{
  rapidjson::StringBuffer b;
  sc_json_writer_t<rapidjson::StringBuffer> writer( b, sim_ );

  js_.Accept( writer );

  auto out = fmt::memory_buffer();
  if ( on_click )
  {
    assert( !toggle_id_str_.empty() );
    fmt::format_to( std::back_inserter( out ), "$('#{}').one('click', function() {{\n", toggle_id_str_ );
    // str_ += "console.log(\"Loading " + id_str_ + ": " + toggle_id_str_ + "
    // ...\" );\n";
  }
  fmt::format_to( std::back_inserter( out ), "$('#{}').highcharts({});\n", id_str_, b.GetStringView() );
  if ( on_click )
  {
    fmt::format_to( std::back_inserter( out ), "}});\n" );
  }
  return fmt::to_string( out );
}

std::string chart_t::to_string() const
{
  rapidjson::StringBuffer b;
  sc_json_writer_t<rapidjson::StringBuffer> writer( b, sim_ );

  js_.Accept( writer );
  auto javascript = std::string{ b.GetStringView() };
  range::erase_remove( javascript, '\n' );

  auto out = fmt::memory_buffer();
  fmt::format_to( std::back_inserter( out ), "<div class=\"charts\" id=\"{}\" style=\"min-width: {}px;", id_str_,
                  width_ );
  if ( height_ > 0 )
  {
    fmt::format_to( std::back_inserter( out ), " height: {}px;", height_ );
  }
  fmt::format_to( std::back_inserter( out ), "\"></div>\n" );
  fmt::format_to( std::back_inserter( out ), "<script type=\"text/javascript\">\n" );
  if ( !toggle_id_str_.empty() )
  {
    fmt::format_to( std::back_inserter( out ), "jQuery( document ).ready( function( $ ) {{\n" );
    fmt::format_to( std::back_inserter( out ), "$('#{}').on('click', function() {{\n", toggle_id_str_ );
    fmt::format_to( std::back_inserter( out ), "console.log(\"Loading {}: {} ...\" );\n", id_str_, toggle_id_str_ );
    fmt::format_to( std::back_inserter( out ), "$('#{}').highcharts({});\n}});\n}});\n", id_str_, javascript );
  }
  else
  {
    fmt::format_to( std::back_inserter( out ), "jQuery('#{}').highcharts({});\n", id_str_, javascript );
  }
  fmt::format_to( std::back_inserter( out ), "</script>\n" );

  return fmt::to_string( out );
}

std::string chart_t::to_xml() const
{
  rapidjson::StringBuffer b;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( b );

  js_.Accept( writer );

  return fmt::format("<!CDATA[{}]]>", b.GetStringView());
}

void chart_t::set_xaxis_max( double max )
{
  set( "xAxis.max", max );
}

void chart_t::set_xaxis_title( std::string_view label )
{
  set( "xAxis.title.text", label );
}

void chart_t::set_yaxis_title( std::string_view label )
{
  set( "yAxis.title.text", label );
}

void chart_t::set_title( std::string_view title )
{
  set( "title.text", title );
}

void chart_t::add_data_series( std::string_view type, std::string_view name,
                               std::vector<sc_js_t>& d )
{
  rapidjson::Value obj( rapidjson::kObjectType );

  if ( !type.empty() )
    set( obj, "type", type );

  if ( !name.empty() )
    set( obj, "name", name );

  rapidjson::Value data( rapidjson::kArrayType );

  for ( auto& entry : d )
  {
    data.PushBack( static_cast<rapidjson::Value&>( entry.js_ ),
                   js_.GetAllocator() );
  }

  obj.AddMember( "data", data, js_.GetAllocator() );
  add( "series", obj );
}

void chart_t::add_data_series( std::vector<sc_js_t>& d )
{
  add_data_series( "", "", d );
}

void chart_t::add_simple_series( std::string_view type,
                                 std::optional<color::rgb> color,
                                 std::string_view name,
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

  if ( color.has_value() )
  {
    add( "colors", color->str() );
  }
  add( "series", obj );
}

void chart_t::add_simple_series(
    std::string_view type, std::optional<color::rgb> color, std::string_view name,
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

  if ( color.has_value() )
  {
    add( "colors", color->str() );
  }
  add( "series", obj );
}

void chart_t::add_simple_series( std::string_view type,
                                 std::optional<color::rgb> color,
                                 std::string_view name,
                                 const std::vector<double>& series )
{
  rapidjson::Value obj( rapidjson::kObjectType );

  set( obj, "type", type );
  set( obj, "data", series );
  set( obj, "name", name );

  if ( color.has_value() )
  {
    add( "colors", color->str() );
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
chart_t& chart_t::add_yplotline( double value, std::string_view name,
                                 double line_width, std::optional<color::rgb> color )
{
  if ( rapidjson::Value* obj = path_value( "yAxis.plotLines" ) )
  {
    if ( obj->GetType() != rapidjson::kArrayType )
      obj->SetArray();

    if ( !color.has_value() )
      color = "FFFFFF";

    if ( rapidjson::Value* text_v = path_value( "subtitle.text" ) )
    {
      auto mean_str = fmt::format( "<span style=\"color: {};\">{}{}</span>", *color,
                                   name.empty() ? "" : fmt::format( "{}=", name ), value );

      if ( text_v->GetType() == rapidjson::kStringType )
        mean_str = std::string( text_v->GetString() ) + ", " + mean_str;

      text_v->SetString( mean_str.c_str(),
                         static_cast<rapidjson::SizeType>( mean_str.size() ),
                         js_.GetAllocator() );
    }

    rapidjson::Value new_obj( rapidjson::kObjectType );
    set( new_obj, "color", color->str() );
    set( new_obj, "value", value );
    set( new_obj, "width", line_width );
    set( new_obj, "zIndex", 5 );

    obj->PushBack( new_obj, js_.GetAllocator() );
  }

  return *this;
}

time_series_t::time_series_t( std::string id_str, const sim_t& sim )
  : chart_t( std::move( id_str ), sim )
{
  set( "chart.type", "area" );

  set( "tooltip.headerFormat", "Second: {point.key}<br/>" );
  set_xaxis_title( "Time (seconds)" );
}

time_series_t& time_series_t::set_mean( double value_,
                                        std::optional<color::rgb> color )
{
  if ( !color.has_value() )
    color = TEXT_MEAN_COLOR;

  add_yplotline( value_, "mean", 1.25, color );
  return *this;
}

time_series_t& time_series_t::set_max( double value_, std::optional<color::rgb> color )
{
  if ( !color.has_value() )
    color = TEXT_MAX_COLOR;

  add_yplotline( value_, "max", 1.25, color );
  return *this;
}

bar_chart_t::bar_chart_t( std::string id_str, const sim_t& sim )
  : chart_t( std::move( id_str ), sim )
{
  set( "chart.type", "bar" );

  set( "xAxis.tickLength", 0 );
  set( "xAxis.type", "category" );
  set( "xAxis.labels.step", 1 );
  set( "xAxis.labels.y", 4 );
  set( "xAxis.offset", -10 );
}

pie_chart_t::pie_chart_t( std::string id_str, const sim_t& sim )
  : chart_t( std::move(id_str), sim )
{
  height_ = 300;  // Default Pie Chart height

  set( "chart.type", "pie" );

  // set( "plotOptions.bar.states.hover.lineWidth", 1 );
}

histogram_chart_t::histogram_chart_t( std::string id_str,
                                      const sim_t& sim )
  : chart_t( std::move(id_str), sim )
{
  height_ = 300;

  set( "chart.type", "column" );

  set( "xAxis.tickLength", 0 );
  set( "xAxis.type", "category" );
}


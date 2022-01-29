// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HIGHCHART_HPP
#define SC_HIGHCHART_HPP

#include "interfaces/sc_js.hpp"
#include "color.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct sim_t;
struct stats_t;
struct player_t;
struct buff_t;

namespace highchart
{
std::string build_id( const stats_t& stats, std::string_view suffix );
std::string build_id( const player_t& actor, std::string_view suffix );
std::string build_id( const buff_t& buff, std::string_view suffix );

enum highchart_theme_e
{
  THEME_DEFAULT = 0,
  THEME_ALT
};

js::sc_js_t& theme( js::sc_js_t&, highchart_theme_e theme );

struct chart_t;

struct data_triple_t
{
  double x_;
  double v1_;
  double v2_;

  data_triple_t( double x, double v1, double v2 ) : x_( x ), v1_( v1 ), v2_( v2 )
  {
  }
};

struct chart_t : public js::sc_js_t
{
private:
  chart_t( const chart_t& );

public:
  std::string id_str_;
  std::string toggle_id_str_;
  size_t height_, width_;
  const sim_t& sim_;

  chart_t( std::string id_str, const sim_t& sim );

  void set_toggle_id( std::string tid )
  {
    toggle_id_str_ = std::move( tid );
  }

  void set_title( std::string_view title );
  void set_xaxis_title( std::string_view label );
  void set_yaxis_title( std::string_view label );
  void set_xaxis_max( double max );

  void add_simple_series( std::string_view type, std::optional<color::rgb> color, std::string_view name,
                          const std::vector<std::pair<double, double> >& series );
  void add_simple_series( std::string_view type, std::optional<color::rgb> color, std::string_view name,
                          const std::vector<double>& series );
  void add_simple_series( std::string_view type, std::optional<color::rgb> color, std::string_view name,
                          const std::vector<data_triple_t>& series );

  // Note: Ownership of data transfers from the vectors to this object after
  // call
  void add_data_series( std::string_view type, std::string_view name, std::vector<sc_js_t>& d );
  void add_data_series( std::vector<sc_js_t>& d );

  chart_t& add_yplotline( double value, std::string_view name, double line_width_ = 1.25,
                          std::optional<color::rgb> color = {} );

  virtual std::string to_string() const;
  virtual std::string to_aggregate_string( bool on_click = true ) const;
  virtual std::string to_data() const;
  virtual std::string to_target_div() const;
  virtual std::string to_xml() const;
};

struct time_series_t : public chart_t
{
private:
  time_series_t( const time_series_t& );

public:
  time_series_t( std::string id_str, const sim_t& sim );

  time_series_t& set_mean( double value_, std::optional<color::rgb> color = {} );
  time_series_t& set_max( double value_, std::optional<color::rgb> color = {} );
};

struct bar_chart_t : public chart_t
{
  bar_chart_t( std::string id_str, const sim_t& sim );
};

struct pie_chart_t : public chart_t
{
  pie_chart_t( std::string id_str, const sim_t& sim );
};

struct histogram_chart_t : public chart_t
{
  histogram_chart_t( std::string id_str, const sim_t& sim );
};

}  // namespace highchart

#endif

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HIGHCHART_HPP
#define SC_HIGHCHART_HPP

#include "interfaces/sc_js.hpp"

struct sim_t;
struct stats_t;
struct player_t;
struct buff_t;

namespace highchart
{
std::string build_id( const stats_t& stats, const std::string& suffix = "" );
std::string build_id( const player_t& actor, const std::string& suffix );
std::string build_id( const buff_t& buff, const std::string& suffix );

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

  data_triple_t( double x, double v1, double v2 )
    : x_( x ), v1_( v1 ), v2_( v2 )
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

  chart_t( const std::string& id_str, const sim_t& sim );
  virtual ~chart_t()
  {
  }

  void set_toggle_id( const std::string& tid )
  {
    toggle_id_str_ = tid;
  }

  void set_title( const std::string& title );
  void set_xaxis_title( const std::string& label );
  void set_yaxis_title( const std::string& label );
  void set_xaxis_max( double max );

  void add_simple_series(
      const std::string& type, const std::string& color,
      const std::string& name,
      const std::vector<std::pair<double, double> >& series );
  void add_simple_series( const std::string& type, const std::string& color,
                          const std::string& name,
                          const std::vector<double>& series );
  void add_simple_series( const std::string& type, const std::string& color,
                          const std::string& name,
                          const std::vector<data_triple_t>& series );

  // Note: Ownership of data transfers from the vectors to this object after
  // call
  void add_data_series( const std::string& type, const std::string& name,
                        std::vector<sc_js_t>& d );
  void add_data_series( std::vector<sc_js_t>& d );

  chart_t& add_yplotline( double value_, const std::string& name_,
                          double line_width_        = 1.25,
                          const std::string& color_ = std::string() );

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
  time_series_t( const std::string& id_str, const sim_t& sim );

  time_series_t& set_mean( double value_,
                           const std::string& color = std::string() );
  time_series_t& set_max( double value_,
                          const std::string& color = std::string() );
};

struct bar_chart_t : public chart_t
{
  bar_chart_t( const std::string& id_str, const sim_t& sim );
};

struct pie_chart_t : public chart_t
{
  pie_chart_t( const std::string& id_str, const sim_t& sim );
};

struct histogram_chart_t : public chart_t
{
  histogram_chart_t( const std::string& id_str, const sim_t& sim );
};
}

#endif

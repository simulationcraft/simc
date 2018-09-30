// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

#include "config.hpp"
#include "sc_enums.hpp"
#include "sc_highchart.hpp"
#include "util/io.hpp"
#include "sc_util.hpp"

struct player_t;
struct action_t;
struct buff_t;
struct item_t;
struct xml_node_t;
struct sc_timeline_t;
struct spell_data_t;
class extended_sample_data_t;
struct player_processed_report_information_t;
struct sim_report_information_t;
struct spell_data_expr_t;
struct artifact_power_t;

#include <chrono>
/**
 * Automatic Timer reporting the time between construction and desctruction of
 * the object.
 */
struct Timer
{
private:
  std::string title;
  std::ostream& out;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  bool started;

public:
  Timer( std::string title, std::ostream& out = std::cout )
    : title( std::move( title ) ),
      out( out ),
      start_time( std::chrono::high_resolution_clock::now() ),
      started( false )
  { }

  void start()
  {
    start_time = std::chrono::high_resolution_clock::now();
    started = true;
  }

  ~Timer()
  {
    if ( started )
    {
      auto end            = std::chrono::high_resolution_clock::now();
      auto diff           = end - start_time;
      using float_seconds = std::chrono::duration<double>;
      out << fmt::format("{} took {}seconds.",
          title, std::chrono::duration_cast<float_seconds>( diff ).count());
      out << std::endl;
    }
  }
};
namespace color
{
struct rgb
{
  unsigned char r_, g_, b_;
  double a_;

  rgb();

  rgb( unsigned char r, unsigned char g, unsigned char b );
  rgb( double r, double g, double b );
  rgb( const std::string& color );
  rgb( const char* color );

  std::string rgb_str() const;
  std::string str() const;
  std::string hex_str() const;

  rgb& adjust( double v );
  rgb adjust( double v ) const;
  rgb dark( double pct = 0.25 ) const;
  rgb light( double pct = 0.25 ) const;
  rgb opacity( double pct = 1.0 ) const;

  rgb& operator=( const std::string& color_str );
  rgb& operator+=( const rgb& other );
  rgb operator+( const rgb& other ) const;
  operator std::string() const;

private:
  bool parse_color( const std::string& color_str );
};

std::ostream& operator<<( std::ostream& s, const rgb& r );

rgb mix( const rgb& c0, const rgb& c1 );

rgb class_color( player_e type );
rgb resource_color( resource_e type );
rgb stat_color( stat_e type );
rgb school_color( school_e school );

// Class colors
const rgb COLOR_DEATH_KNIGHT = "C41F3B";
const rgb COLOR_DEMON_HUNTER = "A330C9";
const rgb COLOR_DRUID        = "FF7D0A";
const rgb COLOR_HUNTER       = "ABD473";
const rgb COLOR_MAGE         = "69CCF0";
const rgb COLOR_MONK         = "00FF96";
const rgb COLOR_PALADIN      = "F58CBA";
const rgb COLOR_PRIEST       = "FFFFFF";
const rgb COLOR_ROGUE        = "FFF569";
const rgb COLOR_SHAMAN       = "0070DE";
const rgb COLOR_WARLOCK      = "9482C9";
const rgb COLOR_WARRIOR      = "C79C6E";

const rgb WHITE  = "FFFFFF";
const rgb GREY   = "333333";
const rgb GREY2  = "666666";
const rgb GREY3  = "8A8A8A";
const rgb YELLOW = COLOR_ROGUE;
const rgb PURPLE = "9482C9";
const rgb RED    = COLOR_DEATH_KNIGHT;
const rgb TEAL   = "009090";
const rgb BLACK  = "000000";

// School colors
const rgb COLOR_NONE = WHITE;
const rgb PHYSICAL   = COLOR_WARRIOR;
const rgb HOLY       = "FFCC00";
const rgb FIRE       = COLOR_DEATH_KNIGHT;
const rgb NATURE     = COLOR_HUNTER;
const rgb FROST      = COLOR_SHAMAN;
const rgb SHADOW     = PURPLE;
const rgb ARCANE     = COLOR_MAGE;
const rgb ELEMENTAL  = COLOR_MONK;
const rgb FROSTFIRE  = "9900CC";
const rgb CHAOS      = "00C800";

// Item quality colors
const rgb POOR      = "9D9D9D";
const rgb COMMON    = WHITE;
const rgb UNCOMMON  = "1EFF00";
const rgb RARE      = "0070DD";
const rgb EPIC      = "A335EE";
const rgb LEGENDARY = "FF8000";
const rgb HEIRLOOM  = "E6CC80";

}  // color

namespace chart
{
// Highcharts stuff
bool generate_raid_gear( highchart::bar_chart_t&, const sim_t& );
bool generate_raid_downtime( highchart::bar_chart_t&, const sim_t& );
bool generate_raid_aps( highchart::bar_chart_t&, const sim_t&,
                        const std::string& type );
bool generate_distribution( highchart::histogram_chart_t&, const player_t* p,
                            const std::vector<size_t>& dist_data,
                            const std::string& distribution_name, double avg,
                            double min, double max );
bool generate_gains( highchart::pie_chart_t&, const player_t&, resource_e );
bool generate_spent_time( highchart::pie_chart_t&, const player_t& );
bool generate_stats_sources( highchart::pie_chart_t&, const player_t&,
                             const std::string title,
                             const std::vector<stats_t*>& stats_list );
bool generate_damage_stats_sources( highchart::pie_chart_t&, const player_t& );
bool generate_heal_stats_sources( highchart::pie_chart_t&, const player_t& );
bool generate_raid_dpet( highchart::bar_chart_t&, const sim_t& );
bool generate_action_dpet( highchart::bar_chart_t&, const player_t& );
bool generate_apet( highchart::bar_chart_t&, const std::vector<stats_t*>& );
highchart::time_series_t& generate_stats_timeline( highchart::time_series_t&,
                                                   const stats_t& );
highchart::time_series_t& generate_actor_timeline(
    highchart::time_series_t&, const player_t& p, const std::string& attribute,
    const std::string& series_color, const sc_timeline_t& data );
bool generate_actor_dps_series( highchart::time_series_t& series,
                                const player_t& p );
bool generate_scale_factors( highchart::bar_chart_t& bc, const player_t& p,
                             scale_metric_e metric );
bool generate_scaling_plot( highchart::chart_t& bc, const player_t& p,
                            scale_metric_e metric );
bool generate_reforge_plot( highchart::chart_t& bc, const player_t& p );

}  // chart

namespace gear_weights
{
std::array<std::string, SCALE_METRIC_MAX> wowhead( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> pawn( const player_t& );
std::array<std::string, SCALE_METRIC_MAX> askmrrobot( const player_t& );
}  // gear_weights

// Report
namespace report
{
using sc_html_stream = io::ofstream;

void generate_player_charts( player_t&,
                             player_processed_report_information_t& );
void generate_player_buff_lists( player_t&,
                                 player_processed_report_information_t& );

void print_html_sample_data( report::sc_html_stream&, const player_t&,
                             const extended_sample_data_t&,
                             const std::string& name, int& td_counter,
                             int columns = 1 );

bool output_scale_factors( const player_t* p );

std::string decoration_domain( const sim_t& sim );
std::string decorated_spell_name(
    const sim_t& sim, const spell_data_t& spell,
    const std::string& parms_str = std::string() );
std::string decorated_item_name( const item_t* item );
std::string decorate_html_string( const std::string& value,
                                  const color::rgb& color );

void print_spell_query( std::ostream& out, const sim_t& sim,
                        const spell_data_expr_t&, unsigned level );
void print_spell_query( xml_node_t* out, FILE* file, const sim_t& sim,
                        const spell_data_expr_t&, unsigned level );
bool check_gear( player_t& p, sim_t& sim );
void print_profiles( sim_t* );
void print_text( sim_t*, bool detail );
void print_html( sim_t& );
void print_json( sim_t& );
void print_html_player( report::sc_html_stream&, player_t&, int );
void print_suite( sim_t* );
std::vector<std::string> beta_warnings();
std::string pretty_spell_text( const spell_data_t& default_spell,
                               const std::string& text, const player_t& p );
inline std::string pretty_spell_text( const spell_data_t& default_spell,
                                      const char* text, const player_t& p )
{
  return text ? pretty_spell_text( default_spell, std::string( text ), p )
              : std::string();
}

template <typename T>
class html_decorator_t
{
  std::vector<std::string> m_parameters;

protected:
  bool find_affixes( std::string& prefix, std::string& suffix ) const
  {
    std::string tokenized_name = url_name();
    util::tokenize( tokenized_name );
    std::string obj_token = token();

    std::string::size_type affix_offset = obj_token.find( tokenized_name );

    // Add an affix to the name, if the name does not match the
    // spell name. Affix is either the prefix- or suffix portion of the
    // non matching parts of the stats name.
    if ( affix_offset != std::string::npos && tokenized_name != obj_token )
    {
      // Suffix
      if ( affix_offset == 0 )
        suffix = obj_token.substr( tokenized_name.size() );
      // Prefix
      else if ( affix_offset > 0 )
        prefix = obj_token.substr( 0, affix_offset );
    }
    else if ( affix_offset == std::string::npos )
      suffix = obj_token;

    return ! prefix.empty() || ! suffix.empty();
  }

public:
  html_decorator_t()
  { }

  virtual ~html_decorator_t() = default;

  virtual std::string url_params() const
  {
    std::stringstream s;

    // Add url parameters
    auto parameters = this -> parms();
    if ( parameters.size() > 0 )
    {
      s << "?";

      for ( size_t i = 0, end = parameters.size(); i < end; ++i )
      {
        s << parameters[ i ];

        if ( i < end - 1 )
        {
          s << "&";
        }
      }
    }

    return s.str();
  }

  T& add_parm( const std::string& n, const std::string& v )
  {
    m_parameters.push_back( n + "=" + v );
    return *this;
  }

  T& add_parm( const std::string& n, const char * v )
  { return add_parm( n, std::string( v ) ); }

  template <typename P_VALUE>
  T& add_parm( const std::string& n, P_VALUE& v )
  {
    m_parameters.push_back( n + "=" + util::to_string( v ) );
    return *this;
  }

  virtual std::vector<std::string> parms() const
  { return m_parameters; }

  virtual std::string base_url() const = 0;

  virtual bool can_decorate() const = 0;

  // Full blown name, displayed as the link name in the HTML report
  virtual std::string url_name() const = 0;

  // Tokenized name of the spell
  virtual std::string token() const = 0;

  // An optional prefix applied to url_name (included in the link)
  virtual std::string url_name_prefix() const
  { return std::string(); }

  virtual std::string decorate() const
  {
    if ( ! can_decorate() )
    {
      return "<a href=\"#\">" + token() + "</a>";
    }

    std::stringstream s;

    std::string prefix, suffix;
    find_affixes( prefix, suffix );

    if ( ! prefix.empty() )
    {
      s << "(" << prefix << ")&#160;";
    }

    // Generate base url
    s << base_url();
    s << url_params();

    // Close url
    s << "\">";

    // Add optional prefix to the url name
    s << url_name_prefix();

    // Add name of the spell
    s << url_name();

    // Close <a>
    s << "</a>";

    // Add suffix if present
    if ( ! suffix.empty() )
    {
      s << "&#160;(" << suffix << ")";
    }

    return s.str();
  }
};

template <typename T>
class spell_decorator_t : public html_decorator_t<spell_decorator_t<T>>
{
  using super = html_decorator_t<spell_decorator_t<T>>;
protected:
  const T* m_obj;

public:
  spell_decorator_t( const T* obj ) :
    super(), m_obj( obj )
  { }

  std::string base_url() const override
  {
    std::stringstream s;

    s << "<a href=\"https://" << decoration_domain( *this -> m_obj -> sim )
      << ".wowhead.com/spell=" << this -> m_obj -> data().id();

    return s.str();
  }

  bool can_decorate() const override
  {
    return this -> m_obj -> sim -> decorated_tooltips &&
           this -> m_obj -> data().id() > 0;
  }

  std::string url_name() const override
  { return m_obj -> data().id() ? m_obj -> data().name_cstr() : m_obj -> name(); }

  std::string token() const override
  { return m_obj -> name(); }

  std::vector<std::string> parms() const override
  {
    auto parameters = super::parms();

    if ( m_obj -> item )
    {
      parameters.push_back( "ilvl=" + util::to_string( m_obj -> item -> item_level() ) );
    }

    return parameters;
  }
};

// Generic spell data decorator, supports player, item, and artifact-power driven spell data
class spell_data_decorator_t : public html_decorator_t<spell_data_decorator_t>
{
  const sim_t* m_sim;
  const player_t* m_player;
  const spell_data_t* m_spell;
  const item_t* m_item;
  const artifact_power_t* m_power;

public:
  spell_data_decorator_t( const sim_t* obj, const spell_data_t* spell ) :
    html_decorator_t(), m_sim( obj ), m_player( nullptr ), m_spell( spell ),
    m_item( nullptr ), m_power( nullptr )
  { }

  spell_data_decorator_t( const sim_t* obj, const spell_data_t& spell ) :
    spell_data_decorator_t( obj, &spell )
  { }

  spell_data_decorator_t( const player_t* obj, const spell_data_t* spell );

  spell_data_decorator_t( const player_t* obj, const spell_data_t& spell ) :
    spell_data_decorator_t( obj, &spell )
  { }

  spell_data_decorator_t( const player_t* obj, const artifact_power_t& power );

  std::vector<std::string> parms() const override;
  std::string base_url() const override;

  spell_data_decorator_t& item( const item_t* obj )
  { m_item = obj; return *this; }

  spell_data_decorator_t& item( const item_t& obj )
  { m_item = &obj; return *this; }

  spell_data_decorator_t& artifact_power( const artifact_power_t* obj )
  { m_power = obj; return *this; }

  spell_data_decorator_t& artifact_power( const artifact_power_t& obj )
  { m_power = &obj; return *this; }

  bool can_decorate() const override;
  std::string url_name() const override;
  std::string token() const override;
};

class item_decorator_t : public html_decorator_t<item_decorator_t>
{
  const item_t* m_item;

public:
  item_decorator_t( const item_t* obj ) :
    html_decorator_t(), m_item( obj )
  { }

  item_decorator_t( const item_t& obj ) :
    item_decorator_t( &obj )
  { }

  std::vector<std::string> parms() const override;
  std::string base_url() const override;

  bool can_decorate() const override;
  std::string url_name() const override;
  std::string token() const override;
};

}  // report

#endif  // SC_REPORT_HPP

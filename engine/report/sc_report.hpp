// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include "simulationcraft.hpp"
#include <fstream>

#define MAX_PLAYERS_PER_CHART 20

#define LOOTRANK_ENABLED 1

namespace chart
{
enum chart_e { HORIZONTAL_BAR_STACKED, HORIZONTAL_BAR, VERTICAL_BAR, PIE, LINE, XY_LINE };

std::string resource_color( int type );
std::string raid_downtime ( std::vector<player_t*> &players_by_name, int print_styles = 0 );
size_t raid_aps ( std::vector<std::string>& images, sim_t*, std::vector<player_t*>&, bool dps );
size_t raid_dpet( std::vector<std::string>& images, sim_t* );
size_t raid_gear( std::vector<std::string>& images, sim_t*, int print_styles = 0 );

std::string action_dpet        ( player_t* );
std::string aps_portion        ( player_t* );
std::string time_spent         ( player_t* );
std::string gains              ( player_t*, resource_e );
std::string timeline           ( player_t*, const std::vector<double>&, const std::string&, double avg = 0, std::string color = "FDD017", size_t max_length = 0 );
std::string timeline_dps_error ( player_t* );
std::string scale_factors      ( player_t* );
std::string scaling_dps        ( player_t* );
std::string reforge_dps        ( player_t* );
std::string distribution       ( int /*print_style*/, const std::vector<size_t>& /*dist_data*/, const std::string&, double, double, double );
std::string normal_distribution(  double mean, double std_dev, double confidence, double tolerance_interval = 0, int print_styles = 0  );
std::string dps_error( player_t& );

#if LOOTRANK_ENABLED == 1
std::string gear_weights_lootrank  ( player_t* );
#endif
std::string gear_weights_wowhead   ( player_t*, bool hit_expertise );
std::string gear_weights_wowreforge( player_t* );
std::string gear_weights_askmrrobot( player_t* );
std::string gear_weights_wowupgrade( player_t* );
std::string gear_weights_pawn      ( player_t*, bool hit_expertise );

} // end namespace sc_chart

namespace report
{

class indented_stream : public io::ofstream
{
  int level_;
  virtual const char* _tabs() const = 0;

public:
  indented_stream() : level_( 0 ) {}

  int level() const { return level_; }
  void set_level( int l ) { level_ = l; }

  indented_stream& tabs()
  {
    *this << _tabs();
    return *this;
  }

  ofstream& operator+=( int c ) { assert( level_ + c >= 0 ); level_ += c; return *this; }
  ofstream& operator-=( int c ) { assert( level_ - c >= 0 ); level_ -= c; return *this; }

  ofstream& operator++() { ++level_; return *this; }
  ofstream& operator--() { assert( level_ > 0 ); --level_; return *this; }
};

struct sc_html_stream : public indented_stream
{
  virtual const char* _tabs() const
  {
    switch ( level() )
    {
      case  0: return "";
      case  1: return "\t";
      case  2: return "\t\t";
      case  3: return "\t\t\t";
      case  4: return "\t\t\t\t";
      case  5: return "\t\t\t\t\t";
      case  6: return "\t\t\t\t\t\t";
      case  7: return "\t\t\t\t\t\t\t";
      case  8: return "\t\t\t\t\t\t\t\t";
      case  9: return "\t\t\t\t\t\t\t\t\t";
      case 10: return "\t\t\t\t\t\t\t\t\t\t";
      case 11: return "\t\t\t\t\t\t\t\t\t\t\t";
      case 12: return "\t\t\t\t\t\t\t\t\t\t\t\t";
      default: assert( 0 ); return NULL;
    }
  }
};


void generate_player_charts         ( player_t*, player_processed_report_information_t& );
void generate_player_buff_lists     ( player_t*, player_processed_report_information_t& );
void generate_sim_report_information( sim_t*,       sim_t::report_information_t& );

void print_html_sample_data ( report::sc_html_stream&, sim_t*, extended_sample_data_t&, const std::string& name, int& td_counter );

void print_spell_query ( sim_t*, unsigned level );
void print_profiles    ( sim_t* );
void print_text        ( FILE*, sim_t*, bool detail );
void print_html        ( sim_t* );
void print_html_player ( report::sc_html_stream&, player_t*, int );
void print_xml         ( sim_t* );
void print_suite       ( sim_t* );
void print_csv_data( sim_t* );

struct compare_hat_donor_interval
{
  bool operator()( const player_t* l, const player_t* r ) const
  {
    return ( l -> procs.hat_donor -> interval_sum.mean() < r -> procs.hat_donor -> interval_sum.mean() );
  }
};

struct tabs_t
{
  int level;

  tabs_t( int l = 0 ) : level( l ) {}

  tabs_t& operator+=( int c ) { assert( level + c >= 0 ); level += c; return *this; }
  tabs_t& operator-=( int c ) { assert( level - c >= 0 ); level -= c; return *this; }

  tabs_t& operator++() { ++level; return *this; }
  tabs_t& operator--() { assert( level > 0 ); --level; return *this; }

  tabs_t operator++( int ) { tabs_t tmp = *this; ++*this; return tmp; }
  tabs_t operator--( int ) { tabs_t tmp = *this; --*this; return tmp; }

  friend tabs_t operator+( tabs_t t, int c ) { return t += c; }
  friend tabs_t operator-( tabs_t t, int c ) { return t -= c; }

  const char* operator*() const
  {
    switch ( level )
    {
      case  0: return "";
      case  1: return "\t";
      case  2: return "\t\t";
      case  3: return "\t\t\t";
      case  4: return "\t\t\t\t";
      case  5: return "\t\t\t\t\t";
      case  6: return "\t\t\t\t\t\t";
      case  7: return "\t\t\t\t\t\t\t";
      case  8: return "\t\t\t\t\t\t\t\t";
      case  9: return "\t\t\t\t\t\t\t\t\t";
      case 10: return "\t\t\t\t\t\t\t\t\t\t";
      case 11: return "\t\t\t\t\t\t\t\t\t\t\t";
      case 12: return "\t\t\t\t\t\t\t\t\t\t\t\t";
      default: assert( 0 ); return NULL;
    }
  }
};

#if SC_BETA
static const char* const beta_warnings[] =
{
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  "Not All classes are yet supported.",
  "Some class models still need tweaking.",
  "Some class action lists need tweaking.",
  "Some class BiS gear setups need tweaking.",
  "Some trinkets not yet implemented.",
  "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
  "Beta! Beta! Beta! Beta! Beta! Beta!",
};
#endif // SC_BETA
}

std::string pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p );
inline std::string pretty_spell_text( const spell_data_t& default_spell, const char* text, const player_t& p )
{ return text ? pretty_spell_text( default_spell, std::string( text ), p ) : std::string(); }

#endif // SC_REPORT_HPP

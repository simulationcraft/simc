// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include "simulationcraft.hpp"
#include <fstream>

#define MAX_PLAYERS_PER_CHART 20

#define LOOTRANK_ENABLED 0

namespace chart
{
std::string resource_color( int type );

std::string raid_downtime ( std::vector<player_t*> &players_by_name, int print_styles );
size_t raid_aps ( std::vector<std::string>& images, sim_t*, std::vector<player_t*>&, bool dps );
size_t raid_dpet( std::vector<std::string>& images, sim_t* );
size_t raid_gear( std::vector<std::string>& images, sim_t* );

std::string action_dpet        ( player_t* );
std::string aps_portion        ( player_t* );
std::string time_spent         ( player_t* );
std::string gains              ( player_t*, resource_e );
std::string timeline           ( player_t*, std::vector<double>&, const std::string&, double avg=0, std::string color="FDD017" );
std::string timeline_dps_error ( player_t* );
std::string scale_factors      ( player_t* );
std::string scaling_dps        ( player_t* );
std::string reforge_dps        ( player_t* );
std::string distribution       ( int /*print_style*/, std::vector<int>& /*dist_data*/, const std::string&, double, double, double );
std::string dps_error          ( player_t* );

#if LOOTRANK_ENABLED == 1
std::string gear_weights_lootrank  ( player_t* );
#endif
std::string gear_weights_wowhead   ( player_t* );
std::string gear_weights_wowreforge( player_t* );
std::string gear_weights_pawn      ( player_t*, bool hit_expertise=true );
}

namespace report
{

struct sc_ofstream : public std::ofstream
{
  int level;

  sc_ofstream() :
    std::ofstream(), level( 0 ) {}

  void set_level( int l )
  { level = l; }

  sc_ofstream& operator+=( int c ) { assert( level + c >= 0 ); level += c; return *this; }
  sc_ofstream& operator-=( int c ) { assert( level - c >= 0 ); level -= c; return *this; }

  sc_ofstream& operator++() { ++level; return *this; }
  sc_ofstream& operator--() { assert( level > 0 ); --level; return *this; }

  virtual const char* _tabs() const = 0;

  sc_ofstream& printf( const char* format, ... )
  {
    char buffer[ 16384 ];

    va_list fmtargs;
    va_start( fmtargs, format );
    int rval = ::vsnprintf( buffer, sizeof( buffer ), format, fmtargs );
    va_end( fmtargs );

    if ( rval >= 0 )
      assert( static_cast<size_t>( rval ) < sizeof( buffer ) );

    *this << buffer;
    return *this;
  }

  sc_ofstream& tabs()
  {
    *this << _tabs();
    return *this;
  }

  // Opens the file specified by filename
  void open_file( sim_t* sim, const char* filename )
  {
    exceptions( std::ofstream::failbit | std::ofstream::badbit );
    try
    {
      open( filename, std::ios_base::in|std::ios_base::out|std::ios_base::trunc );
    }
    catch ( std::ofstream::failure& )
    {
      sim -> errorf( "Unable to open html file '%s'.\n", filename );
      return;
    }
  }
};

struct sc_html_stream : public sc_ofstream
{

  sc_html_stream() :
    sc_ofstream() {}

  virtual const char* _tabs() const
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




void generate_player_charts         ( player_t*, player_t::report_information_t& );
void generate_player_buff_lists     ( player_t*, player_t::report_information_t& );
void generate_sim_report_information( sim_t*,       sim_t::report_information_t& );

void print_html_rng_information  ( report::sc_html_stream&, rng_t*, double confidence_estimator );
void print_html_sample_data      ( report::sc_html_stream&, sim_t*, sample_data_t&, const std::string& name );

void print_spell_query ( sim_t*, unsigned level );
void print_profiles    ( sim_t* );
void print_text        ( FILE*, sim_t*, bool detail );
void print_html        ( sim_t* );
void print_html_player ( report::sc_html_stream&, player_t*, int );
void print_xml         ( sim_t* );
void print_suite       ( sim_t* );

struct compare_hat_donor_interval
{
  bool operator()( const player_t* l, const player_t* r ) const
  {
    return ( l -> procs.hat_donor -> interval_sum.mean < r -> procs.hat_donor -> interval_sum.mean );
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

#endif // SC_REPORT_HPP

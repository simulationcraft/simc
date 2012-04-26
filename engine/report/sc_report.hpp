// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include "simulationcraft.hpp"

namespace chart {
  std::string resource_color( int type );

  std::string raid_downtime ( const std::vector<player_t*> &players_by_name, int print_styles );
  size_t raid_aps ( std::vector<std::string>& images, const sim_t*, const std::vector<player_t*>&, bool dps );
  size_t raid_dpet( std::vector<std::string>& images, const sim_t* );
  size_t raid_gear( std::vector<std::string>& images, const sim_t* );

  std::string action_dpet        ( const player_t* );
  std::string aps_portion        ( const player_t* );
  std::string time_spent         ( const player_t* );
  std::string gains              ( const player_t*, resource_type_e );
  std::string timeline           ( const player_t*, const std::vector<double>&, const std::string&, double avg=0, std::string color="FDD017" );
  std::string timeline_dps_error ( const player_t* );
  std::string scale_factors      ( const player_t* );
  std::string scaling_dps        ( const player_t* );
  std::string reforge_dps        ( const player_t* );
  std::string distribution       ( const sim_t*, const std::vector<int>&, const std::string&, double, double, double );
  std::string dps_error          ( const player_t* );

  std::string gear_weights_lootrank  ( const player_t* );
  std::string gear_weights_wowhead   ( const player_t* );
  std::string gear_weights_wowreforge( const player_t* );
  std::string gear_weights_pawn      ( const player_t*, bool hit_expertise=true );
}

namespace generate_report_information {
  void generate_player_charts  ( const player_t*, player_t::report_information_t& );
  void generate_player_buff_lists ( const player_t*, player_t::report_information_t& );
  void generate_sim_report_information     ( const sim_t*,       sim_t::report_information_t& );
}

namespace report {

  void print_html_rng_information  ( FILE*, const rng_t*, double confidence_estimator );
  void print_html_sample_data      ( FILE*, const player_t*, const sample_data_t&, const std::string& name );

  void print_spell_query ( sim_t*, unsigned level = MAX_LEVEL );
  void print_profiles    ( sim_t* );
  void print_text        ( FILE*, sim_t*, bool detail=true );
  void print_html        ( sim_t* );
  void print_html_player ( FILE*, player_t*, int );
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
    tabs_t( int l=0 ) : level(l) {}
    tabs_t operator+(int c) { return tabs_t(level+c); }
    tabs_t operator-(int c) { return tabs_t(level-c); }
    tabs_t operator++() { ++level; return *this; }
    tabs_t operator--() { --level; return *this; }
    tabs_t operator++(int c) { level+=c; return *this; }
    tabs_t operator--(int c) { level-=c; return *this; }
    const char* operator*()
    {
      switch(level)
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
      default: assert(0); break;
      }
      return NULL;
    }
  };

#if SC_BETA
  const std::string beta_warnings[] =
  {
    "Beta! Beta! Beta! Beta! Beta! Beta!",
    "Not All classes are yet supported.",
    "Some class models still need tweaking.",
    "Some class action lists need tweaking.",
    "Some class BiS gear setups need tweaking.",
    "Some trinkets not yet implemented.",
    "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
    "Beta! Beta! Beta! Beta! Beta! Beta!",
    ""
  };
#endif // SC_BETA
}

#endif // SC_REPORT_HPP

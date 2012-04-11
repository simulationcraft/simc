// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include "simulationcraft.hpp"

struct chart {
  static std::string resource_color( int type );

  static std::string raid_downtime ( const std::vector<player_t*> &players_by_name, int print_styles );
  static int raid_aps ( std::vector<std::string>& images, const sim_t*, const std::vector<player_t*>&, bool dps );
  static int raid_dpet( std::vector<std::string>& images, const sim_t* );
  static int raid_gear( std::vector<std::string>& images, const sim_t* );

  static std::string action_dpet        ( const player_t* );
  static std::string aps_portion        ( const player_t* );
  static std::string time_spent         ( const player_t* );
  static std::string gains              ( const player_t*, resource_type_e );
  static std::string timeline           ( const player_t*, const std::vector<double>&, const std::string&, double avg=0, std::string color="FDD017" );
  static std::string timeline_dps_error ( const player_t* );
  static std::string scale_factors      ( const player_t* );
  static std::string scaling_dps        ( const player_t* );
  static std::string reforge_dps        ( const player_t* );
  static std::string distribution       ( const sim_t*, const std::vector<int>&, const std::string&, double, double, double );
  static std::string dps_error          ( const player_t* );

  static std::string gear_weights_lootrank  ( const player_t* );
  static std::string gear_weights_wowhead   ( const player_t* );
  static std::string gear_weights_wowreforge( const player_t* );
  static std::string gear_weights_pawn      ( const player_t*, bool hit_expertise=true );
};

struct report {
  static void generate_player_charts  ( const player_t*, player_t::report_information_t& );
  static void generate_player_buff_lists ( const player_t*, player_t::report_information_t& );
  static void generate_sim_report_information     ( const sim_t*,       sim_t::report_information_t& );

  static void print_html_rng_information  ( FILE*, const rng_t* );
  static void print_html_sample_data      ( FILE*, const player_t*, const sample_data_t&, const std::string& name );

  static bool buff_comp( const buff_t* i, const buff_t* j );

  static void print_spell_query ( sim_t*, unsigned level = MAX_LEVEL );
  static void print_profiles    ( sim_t* );
  static void print_text        ( FILE*, sim_t*, bool detail=true );
  static void print_html        ( sim_t* );
  static void print_html_player ( FILE*, player_t*, int );
  static void print_xml         ( sim_t* );
  static void print_suite       ( sim_t* );
};

inline bool report::buff_comp( const buff_t* i, const buff_t* j )
{
  // Aura&Buff / Pet
  if ( ( ! i -> player || ! i -> player -> is_pet() ) && j -> player && j -> player -> is_pet() )
    return true;
  // Pet / Aura&Buff
  else if ( i -> player && i -> player -> is_pet() && ( ! j -> player || ! j -> player -> is_pet() ) )
    return false;
  // Pet / Pet
  else if ( i -> player && i -> player -> is_pet() && j -> player && j -> player -> is_pet() )
  {
    if ( i -> player -> name_str.compare( j -> player -> name_str ) == 0 )
      return ( i -> name_str.compare( j -> name_str ) < 0 );
    else
      return ( i -> player -> name_str.compare( j -> player -> name_str ) < 0 );
  }

  return ( i -> name_str.compare( j -> name_str ) < 0 );
}

#if SC_BETA
namespace {
// beta warning messages
static const std::string beta_warnings[] =
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
}
#endif // SC_BETA

struct compare_hat_donor_interval
{
  bool operator()( const player_t* l, const player_t* r ) const
  {
    return ( l -> procs.hat_donor -> interval_sum.mean < r -> procs.hat_donor -> interval_sum.mean );
  }
};

#endif // SC_REPORT_HPP

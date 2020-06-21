// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

#include "config.hpp"

#include "reports.hpp"
#include "sc_enums.hpp"
#include "sc_highchart.hpp"
#include "util/chrono.hpp"
#include "util/io.hpp"
#include "util/util.hpp"

#include <array>
#include <fstream>
#include <iosfwd>
#include <sstream>

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


#include "charts.hpp"

// Report helpers
namespace report_helper
{
void print_html_sample_data( report::sc_html_stream&, const player_t&, const extended_sample_data_t&,
                             const std::string& name, int columns = 1 );

bool output_scale_factors( const player_t* p );

void generate_player_charts( player_t&, player_processed_report_information_t& );
void generate_player_buff_lists( player_t&, player_processed_report_information_t& );
std::vector<std::string> beta_warnings();
std::string pretty_spell_text( const spell_data_t& default_spell, const std::string& text, const player_t& p );
inline std::string pretty_spell_text( const spell_data_t& default_spell, const char* text, const player_t& p )
{
  return text ? pretty_spell_text( default_spell, std::string( text ), p ) : std::string();
}

bool check_gear( player_t& p, sim_t& sim );

}  // namespace report_helper

#endif  // SC_REPORT_HPP

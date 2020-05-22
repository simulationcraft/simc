// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <iosfwd>
#include <string>

struct player_t;
struct sim_t;
struct spell_data_expr_t;
struct xml_node_t;
namespace io {
  class ofstream;
}

// Global report functions to be called after simulation finished.
namespace report
{

  enum class report_type {
    TEXT,
    HTML,
    JSON
  };

  class report_configuration_t
  {
    report_type _type;
    int _level;
    std::string _destination;
  public:
    report_configuration_t(report_type type, int level, std::string destination);

    bool is_greater_than(int min_level) const;
    bool is_between(int min_level, int max_level) const;
    bool is_less_than(int max_level) const;

    int level() const;
    std::string report_type_string() const;
    report_type report_type() const;
    std::string destination() const;
  };
  std::string get_report_type_string(report_type type);
  report_configuration_t create_report_entry(sim_t& sim, report_type type, int level, std::string destination);

  using sc_html_stream = io::ofstream;

  void print_spell_query(std::ostream& out, const sim_t& sim, const spell_data_expr_t&, unsigned level);
  void print_spell_query(xml_node_t* out, FILE* file, const sim_t& sim, const spell_data_expr_t&, unsigned level);
  void print_profiles(sim_t*);
  void print_text(sim_t*, bool detail);
  void print_html(sim_t&);
  void print_json(sim_t&, const report_configuration_t& entry);
  void print_html_player(report::sc_html_stream&, player_t&);
  void print_suite(sim_t*, const std::vector<report_configuration_t>& report_entries);
}
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <iosfwd>
#include <string>

#include "config.hpp"

struct player_t;
struct sim_t;
struct spell_data_expr_t;
struct xml_node_t;
namespace io
{
class ofstream;
}

// Global report functions to be called after simulation finished.
namespace report
{
using sc_html_stream = io::ofstream;

void print_spell_query( std::ostream& out, const sim_t& sim, const spell_data_expr_t&, unsigned level );
void print_spell_query( xml_node_t* out, FILE* file, const sim_t& sim, const spell_data_expr_t&, unsigned level );
void print_profiles( sim_t* );
void print_text( sim_t*, bool detail );
void print_html( sim_t& );
void print_json( sim_t& );
void print_html_player( report::sc_html_stream&, player_t& );
void print_suite( sim_t* );
}  // namespace report
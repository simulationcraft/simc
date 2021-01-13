// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/string_view.hpp"

#include <string>

struct action_t;
struct buff_t;
struct item_t;
struct sim_t;
struct spell_data_t;
struct pet_t;
namespace color {
  struct rgb;
}

namespace report_decorators {
  std::string decoration_domain(const sim_t& sim);
  std::string decorated_spell_name( const sim_t& sim, const spell_data_t& spell,
                                    util::string_view additional_parameters = {} );
  std::string decorated_item_name(const item_t* item);
  std::string decorate_html_string(util::string_view value, const color::rgb& color);
  std::string decorated_buff(const buff_t& buff);
  std::string decorated_action(const action_t& a);
  std::string decorated_spell_data(const sim_t& sim, const spell_data_t* spell);
  std::string decorated_spell_data_item(const sim_t& sim, const spell_data_t* spell, const item_t& item);
  std::string decorated_item(const item_t& item);
  std::string decorated_npc(const pet_t& pet);
}  // report_decorators

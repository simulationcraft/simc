// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "dbc/data_enums.hh"
#include "sc_enums.hpp"
#include "util/string_view.hpp"

#include <string>
#include <vector>

class dbc_t;
struct gem_property_data_t;
struct item_t;
struct item_enchantment_data_t;
struct stat_pair_t;

namespace enchant
{
struct enchant_db_item_t
{
  const char* enchant_name;
  unsigned enchant_id;
};

unsigned find_enchant_id( util::string_view name );
std::string find_enchant_name( unsigned enchant_id );
const std::string& encoded_enchant_name( const dbc_t&, const item_enchantment_data_t& );

const item_enchantment_data_t& find_item_enchant( const item_t& item, util::string_view name );
const item_enchantment_data_t& find_meta_gem( const dbc_t& dbc, util::string_view encoding );
meta_gem_e meta_gem_type( const dbc_t& dbc, const item_enchantment_data_t& );
bool passive_enchant( item_t& item, unsigned spell_id );

void initialize_item_enchant( item_t& item, std::vector<stat_pair_t>& stats, special_effect_source_e source,
                              const item_enchantment_data_t& enchant );
item_socket_color initialize_gem( item_t& item, size_t gem_idx );
item_socket_color initialize_relic( item_t& item, size_t relic_idx, const gem_property_data_t& gem_property );
}  // namespace enchant

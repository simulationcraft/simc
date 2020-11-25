// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <cstdint>
#include <string>


struct item_t;
struct dbc_item_data_t;
struct item_enchantment_data_t;
struct item_bonus_entry_t;
struct special_effect_t;
struct stat_pair_t;

// Item database ============================================================

namespace item_database
{
bool     download_item(      item_t& item );
bool     initialize_item_sources( item_t& item, std::vector<std::string>& source_list );

int      random_suffix_type( item_t& item );
int      random_suffix_type( const dbc_item_data_t& );
uint32_t armor_value(        const item_t& item );
uint32_t armor_value(        const dbc_item_data_t&, const dbc_t&, unsigned item_level = 0 );
// Uses weapon's own (upgraded) ilevel to calculate the damage
uint32_t weapon_dmg_min(     item_t& item );
// Allows custom ilevel to be specified
uint32_t weapon_dmg_min(     const dbc_item_data_t&, const dbc_t&, unsigned item_level = 0 );
uint32_t weapon_dmg_max(     item_t& item );
uint32_t weapon_dmg_max(     const dbc_item_data_t&, const dbc_t&, unsigned item_level = 0 );

bool     load_item_from_data( item_t& item );

// Parse anything relating to the use of ItemSpellEnchantment.dbc. This includes
// enchants, and engineering addons.
bool     parse_item_spell_enchant( item_t& item, std::vector<stat_pair_t>& stats, special_effect_t& effect, unsigned enchant_id );

std::string stat_to_str( int stat, int stat_amount );

// Stat scaling methods for items, or item stats
double approx_scale_coefficient( unsigned current_ilevel, unsigned new_ilevel );
int scaled_stat( const item_t& item, const dbc_t& dbc, size_t idx, unsigned new_ilevel );

stat_pair_t item_enchantment_effect_stats( const item_enchantment_data_t& enchantment, int index );
stat_pair_t item_enchantment_effect_stats( player_t* player, const item_enchantment_data_t& enchantment, int index );
double item_budget( const item_t* item, unsigned max_ilevel );
double item_budget( const player_t* player, unsigned ilevel );

inline bool heroic( unsigned f ) { return ( f & RAID_TYPE_HEROIC ) == RAID_TYPE_HEROIC; }
inline bool lfr( unsigned f ) { return ( f & RAID_TYPE_LFR ) == RAID_TYPE_LFR; }
inline bool warforged( unsigned f ) { return ( f & RAID_TYPE_WARFORGED ) == RAID_TYPE_WARFORGED; }
inline bool mythic( unsigned f ) { return ( f & RAID_TYPE_MYTHIC ) == RAID_TYPE_MYTHIC; }

bool apply_item_bonus( item_t& item, const item_bonus_entry_t& entry );

double curve_point_value( dbc_t& dbc, unsigned curve_id, double point_value );
void apply_item_scaling( item_t& item, unsigned scaling_id, unsigned player_level );
double apply_combat_rating_multiplier( const item_t& item, double amount );
double apply_combat_rating_multiplier( const player_t* player, combat_rating_multiplier_type type, unsigned ilevel, double amount );
double apply_stamina_multiplier( const item_t& item, double amount );
double apply_stamina_multiplier( const player_t* player, combat_rating_multiplier_type type, unsigned ilevel, double amount );

/// Convert stat values to stat allocation values based on the item data
void convert_stat_values( item_t& item );

// Return the combat rating multiplier category for item data
combat_rating_multiplier_type item_combat_rating_type( const dbc_item_data_t& data );

struct token_t
{
  std::string full;
  std::string name;
  double value;
  std::string value_str;
};
std::vector<token_t> parse_tokens( util::string_view encoded_str );

bool has_item_bonus_type( const item_t& item, item_bonus_type bonus_type );

inline bool is_crafted_item_mod( int mod )
{ return mod == ITEM_MOD_BONUS_STAT_1 || mod == ITEM_MOD_BONUS_STAT_2; }
}

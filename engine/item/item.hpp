// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "dbc/item_data.hpp"
#include "dbc/item_effect.hpp"
#include "player/gear_stats.hpp"
#include "sc_enums.hpp"
#include "util/timespan.hpp"
#include "util/generic.hpp"
#include "util/format.hpp"
#include "util/string_view.hpp"

#include <array>
#include <vector>
#include <memory>

class dbc_t;
struct player_t;
struct sim_t;
struct special_effect_t;
struct weapon_t;
struct xml_node_t;

struct stat_pair_t
{
  stat_e stat;
  int value;

  stat_pair_t() : stat( STAT_NONE ), value( 0 )
  { }

  stat_pair_t( stat_e s, int v ) : stat( s ), value( v )
  { }
};

struct parsed_item_data_t : dbc_item_data_t {
  std::array<int, MAX_ITEM_STAT> stat_type_e;
  std::array<int, MAX_ITEM_STAT> stat_alloc;
  std::array<item_effect_t, MAX_ITEM_EFFECT> effects;

  parsed_item_data_t()
    : dbc_item_data_t{}
  {
    range::fill( stat_type_e, -1 );
    range::fill( stat_alloc, 0 );
    range::fill( effects, item_effect_t::nil() );
  }

  void init( const dbc_item_data_t& raw, const dbc_t& dbc );

  size_t add_effect( unsigned spell_id, int type );
  size_t add_effect( const item_effect_t& effect );
};

struct item_t
{
  sim_t* sim;
  player_t* player;
  slot_e slot, parent_slot;
  bool unique, unique_addon, is_ptr;

  // Structure contains the "parsed form" of this specific item, be the data
  // from user options, or a data source such as the Blizzard API, or Wowhead
  struct parsed_input_t
  {
    int                                              item_level;
    // Note, temporary enchant is parsed outside of item option parsing. This is due to
    // coming from player-scope option, instead of being tied to individual items
    unsigned                                         enchant_id;
    unsigned                                         temporary_enchant_id;
    unsigned                                         addon_id;
    int                                              armor;
    unsigned                                         azerite_level;
    std::array<int, MAX_ITEM_STAT>                   stat_val;
    std::array<int, MAX_GEM_SLOTS>                   gem_id;
    std::array<std::vector<unsigned>, MAX_GEM_SLOTS> gem_bonus_id;
    std::array<unsigned, MAX_GEM_SLOTS>              gem_ilevel;
    std::array<int, MAX_GEM_SLOTS>                   gem_actual_ilevel;
    std::array<int, MAX_GEM_SLOTS>                   gem_color;
    std::vector<int>                                 bonus_id;
    std::vector<stat_pair_t>                         gem_stats, meta_gem_stats, socket_bonus_stats;
    std::string                                      encoded_enchant;
    std::vector<stat_pair_t>                         enchant_stats;
    std::vector<stat_pair_t>                         temp_enchant_stats;
    std::string                                      encoded_addon;
    std::vector<stat_pair_t>                         addon_stats;
    parsed_item_data_t                               data;
    auto_dispose< std::vector<special_effect_t*> >   special_effects;
    std::vector<std::string>                         source_list;
    timespan_t                                       initial_cd;
    unsigned                                         drop_level;
    std::vector<unsigned>                            azerite_ids;
    std::vector<int>                                 crafted_stat_mod;

    parsed_input_t();
    ~parsed_input_t();
  } parsed;

  std::string name_str;
  std::string icon_str;

  std::string source_str;
  std::string options_str;

  // Option Data
  std::string option_name_str;
  std::string option_stats_str;
  std::string option_gems_str;
  std::string option_enchant_str;
  std::string option_addon_str;
  std::string option_equip_str;
  std::string option_use_str;
  std::string option_weapon_str;
  std::string option_lfr_str;
  std::string option_warforged_str;
  std::string option_heroic_str;
  std::string option_mythic_str;
  std::string option_armor_type_str;
  std::string option_ilevel_str;
  std::string option_quality_str;
  std::string option_data_source_str;
  std::string option_enchant_id_str;
  std::string option_addon_id_str;
  std::string option_gem_id_str;
  std::string option_gem_bonus_id_str;
  std::string option_gem_ilevel_str;
  std::string option_bonus_id_str;
  std::string option_initial_cd_str;
  std::string option_drop_level_str;
  std::string option_azerite_powers_str;
  std::string option_azerite_level_str;
  std::string option_crafted_stat_str;
  double option_initial_cd;

  // Extracted data
  gear_stats_t base_stats, stats;

  item_t() : sim( nullptr ), player( nullptr ), slot( SLOT_INVALID ), parent_slot( SLOT_INVALID ),
    unique( false ), unique_addon( false ), is_ptr( false ),
    parsed(), option_initial_cd(0)
  { }

  item_t( player_t*, util::string_view options_str );

  bool active() const;
  const char* name() const;
  std::string full_name() const;
  const char* slot_name() const;
  weapon_t* weapon() const;
  void init();
  void parse_options();
  bool initialize_data(); // Initializes item data from a data source
  inventory_type inv_type() const;

  bool is_matching_type() const;
  bool is_valid_type() const;
  bool socket_color_match() const;

  unsigned item_level() const;
  unsigned base_item_level() const;
  stat_e stat( size_t idx ) const;
  int stat_value( size_t idx ) const;
  gear_stats_t total_stats() const;
  bool has_item_stat( stat_e stat ) const;

  std::string encoded_item() const;
  std::string encoded_comment();

  std::string encoded_stats() const;
  std::string encoded_weapon() const;
  std::string encoded_gems() const;
  std::string encoded_enchant() const;
  std::string encoded_addon() const;

  void decode_stats();
  void decode_gems();
  void decode_enchant();
  void decode_addon();
  void decode_weapon();
  void decode_warforged();
  void decode_lfr();
  void decode_heroic();
  void decode_mythic();
  void decode_armor_type();
  void decode_ilevel();
  void decode_quality();
  void decode_data_source();
  void decode_equip_effect();
  void decode_use_effect();


  bool verify_slot();

  void init_special_effects();

  static bool download_item( item_t& );

  static std::vector<stat_pair_t> str_to_stat_pair( const std::string& stat_str );
  static std::string stat_pairs_to_str( const std::vector<stat_pair_t>& stat_pairs );

  std::string item_stats_str() const;
  std::string weapon_stats_str() const;
  std::string gem_stats_str() const;
  std::string socket_bonus_stats_str() const;
  std::string enchant_stats_str() const;
  bool has_stats() const;
  bool has_special_effect( special_effect_source_e source = SPECIAL_EFFECT_SOURCE_NONE, special_effect_e type = SPECIAL_EFFECT_NONE ) const;
  bool has_use_special_effect() const
  { return has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ); }
  bool has_scaling_stat_bonus_id() const;

  const special_effect_t* special_effect( special_effect_source_e source = SPECIAL_EFFECT_SOURCE_NONE, special_effect_e type = SPECIAL_EFFECT_NONE ) const;
  const special_effect_t* special_effect_with_name( util::string_view name, special_effect_source_e source = SPECIAL_EFFECT_SOURCE_NONE, special_effect_e type = SPECIAL_EFFECT_NONE ) const;

  friend void format_to( const item_t&, fmt::format_context::iterator );
};

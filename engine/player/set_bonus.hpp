// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "dbc/data_definitions.hh"
#include "sc_enums.hpp"
#include "util/format.hpp"
#include "util/string_view.hpp"

#include <memory>
#include <string>
#include <vector>

struct expr_t;
struct item_set_bonus_t;
struct player_t;

// Set Bonus ================================================================

struct set_bonus_t
{
  // Some magic constants
  static constexpr unsigned N_BONUSES = 8;  // Number of set bonuses in tier gear

  struct set_bonus_data_t
  {
    const spell_data_t* spell;
    const item_set_bonus_t* bonus;
    int overridden;
    bool enabled;
    bool quiet;

    set_bonus_data_t( const spell_data_t* spell )
      : spell( spell ), bonus( nullptr ), overridden( -1 ), enabled( false ), quiet( false )
    {}
  };

  // Data structure definitions
  using bonus_t = std::vector<set_bonus_data_t>;
  using bonus_type_t = std::vector<bonus_t>;
  using set_bonus_type_t = std::vector<bonus_type_t>;

  using bonus_count_t = std::vector<unsigned int>;
  using set_bonus_count_t = std::vector<bonus_count_t>;

  player_t* actor;

  // Set bonus data structure
  set_bonus_type_t set_bonus_spec_data;
  // Set item counts
  set_bonus_count_t set_bonus_spec_count;

  set_bonus_t( player_t* p );

  // Collect item information about set bonuses, fully DBC driven
  void initialize_items();

  // Initialize set bonuses in earnest
  void initialize();

  // Override all set bonuses to be enabled
  void enable_all_sets();

  // Enable a specific set bonus, set quiet by default
  void enable_set_bonus( specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus, bool quiet = true );
  void enable_set_bonus( hero_talent_e hero_talent, set_bonus_type_e set_bonus, set_bonus_e bonus, bool quiet = true );

  std::unique_ptr<expr_t> create_expression( const player_t*, util::string_view type );

  std::vector<const item_set_bonus_t*> enabled_set_bonus_data() const;

  // Fast accessor to a set bonus spell, returns the spell, or spell_data_t::not_found()
  const spell_data_t* set( specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus ) const;
  const spell_data_t* set( hero_talent_e hero_talent, set_bonus_type_e set_bonus, set_bonus_e bonus ) const;

  // Fast accessor for checking whether a set bonus is enabled
  bool has_set_bonus( specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus ) const;
  bool has_set_bonus( hero_talent_e hero_talent, set_bonus_type_e set_bonus, set_bonus_e bonus ) const;

  bool parse_set_bonus_option( util::string_view opt_str, set_bonus_type_e& set_bonus, set_bonus_e& bonus );
  std::string to_string() const;
  std::string to_profile_string( const std::string& = "\n" ) const;
  std::string generate_set_bonus_options() const;

  friend void sc_format_to( const set_bonus_t&, fmt::format_context::iterator );
};

enum trait_sub_tree_e
  { TST_ONE };
/*
 * HERO_ANY, SPEC_ANY, CLASS_ANY
 * [class_e, spec_e, hero_e, set_bonus_type_e, set_bonus_e]
 */

struct set_bonus_pair_t
{
  std::vector<set_bonus_t*> sets;

  set_bonus_pair_t( player_t* ) : sets() {};

  void initialize();
  std::unique_ptr<expr_t> create_expression( const player_t*, util::string_view expr );
  std::vector<const item_set_bonus_t*> enabled_set_bonus_data() const;
  std::string to_profile_string( const std::string& = "\n" ) const;
  std::string generate_set_bonus_options() const;
  bool parse_set_bonus_option( util::string_view option, set_bonus_type_e& set_bonus, set_bonus_e& bonus );

private:
  template <typename T>
  constexpr set_bonus_t* get_bonuses()
  {
    if constexpr( std::is_same_v<T, specialization_e> )
      return sets[0];
    if constexpr( std::is_same_v<T, trait_sub_tree_e> )
      return sets[1];

    static_assert(false);
    return nullptr;
  }

public:
  template <typename T>
  void enable_set_bonus( T idx, set_bonus_type_e set_bonus, set_bonus_e bonus, bool quiet = true )
  { get_bonuses<T>()->enable_set_bonus( idx, set_bonus, bonus, quiet ); }

  template <typename T>
  const spell_data_t* set( T idx, set_bonus_type_e set_bonus, set_bonus_e bonus ) const
  { return get_bonuses<T>()->set( idx, set_bonus, bonus ); };

  template <typename T>
  bool has_set_bonus( T idx, set_bonus_type_e set_bonus, set_bonus_e bonus  ) const
  { return get_bonuses<T>()->has_set_bonus( idx, set_bonus, bonus ); };
};

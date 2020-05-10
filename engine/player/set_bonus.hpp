// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "dbc/data_definitions.hh"
#include <ostream>
#include <vector>
#include <memory>

struct expr_t;
struct item_set_bonus_t;
struct player_t;

// Set Bonus ================================================================

struct set_bonus_t
{
  // Some magic constants
  static const unsigned N_BONUSES = 8;       // Number of set bonuses in tier gear

  struct set_bonus_data_t
  {
    const spell_data_t* spell;
    const item_set_bonus_t* bonus;
    int overridden;
    bool enabled;

    set_bonus_data_t(const spell_data_t* spell) :
      spell(spell), bonus(nullptr), overridden(-1), enabled(false)
    { }
  };

  // Data structure definitions
  typedef std::vector<set_bonus_data_t> bonus_t;
  typedef std::vector<bonus_t> bonus_type_t;
  typedef std::vector<bonus_type_t> set_bonus_type_t;

  typedef std::vector<unsigned> bonus_count_t;
  typedef std::vector<bonus_count_t> set_bonus_count_t;

  player_t* actor;

  // Set bonus data structure
  set_bonus_type_t set_bonus_spec_data;
  // Set item counts
  set_bonus_count_t set_bonus_spec_count;

  set_bonus_t(player_t* p);

  // Collect item information about set bonuses, fully DBC driven
  void initialize_items();

  // Initialize set bonuses in earnest
  void initialize();

  std::unique_ptr<expr_t> create_expression(const player_t*, const std::string& type);

  std::vector<const item_set_bonus_t*> enabled_set_bonus_data() const;

  // Fast accessor to a set bonus spell, returns the spell, or spell_data_t::not_found()
  const spell_data_t* set(specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus) const;

  // Fast accessor for checking whether a set bonus is enabled
  bool has_set_bonus(specialization_e spec, set_bonus_type_e set_bonus, set_bonus_e bonus) const
  {
    if (specdata::spec_idx(spec) < 0)
    {
      return false;
    }

    return set_bonus_spec_data[set_bonus][specdata::spec_idx(spec)][bonus].enabled;
  }

  bool parse_set_bonus_option(const std::string& opt_str, set_bonus_type_e& set_bonus, set_bonus_e& bonus);
  std::string to_string() const;
  std::string to_profile_string(const std::string & = "\n") const;
  std::string generate_set_bonus_options() const;
};

std::ostream& operator<<(std::ostream&, const set_bonus_t&);

// engine/sim/pvp_scaling.hpp
#pragma once

#include <string>
#include <unordered_map>

// Forward declarations
struct sim_t;
struct spell_data_t;

namespace pvp
{

struct pvp_config_t
{
  bool enabled = false;
  std::string mode = "arena";

  // Sub-toggles
  bool coefficients    = true;
  bool trinket_bonus   = true;
  bool stat_scaling    = true;
  bool item_scaling    = true;
  bool tier_penalty    = true;

  // Spell 134735 effects (populated by subtype iteration at init)
  double healing_received_mod = 1.0;
  double absorb_received_mod  = 1.0;
  double absorb_done_mod      = 1.0;
  double crit_damage_mod      = 1.0;
  double mana_regen_mod       = 1.0;
  double pet_damage_mod       = 1.0;
  double resistance_mod       = 1.0;

  // Versatility PvP effectiveness
  double versatility_damage_mod  = 1.0;
  double versatility_healing_mod = 1.0;
  double versatility_dr_mod      = 1.0;

  // Stamina / Primary Stat
  double stamina_mod      = 1.0;
  double primary_stat_mod = 1.0;

  // Rating
  double rating_multiplier = 1.0;

  // Tier set
  double tier_set_effectiveness = 0.67;

  // Dampening (spell 110310)
  bool   dampening_enabled        = true;
  double dampening_start_sec      = 300.0;
  double dampening_stack_interval = 10.0;
  double dampening_pct_per_stack  = 0.01;
  double dampening_max_pct        = 1.0;

  // Coefficient overrides
  std::string coefficient_override_str;
  std::unordered_map<unsigned, double> coefficient_overrides;
  std::unordered_map<unsigned, double> effect_overrides;
  std::unordered_map<unsigned, double> item_bonus_coefficients;
};

// Parse "53:0.90/e280:0.85" into coefficient_overrides and effect_overrides maps
void parse_coefficient_overrides( pvp_config_t& pvp );

// Set format-specific defaults (dampening timing, etc.)
// user_set_dampening_start: true if user explicitly provided pvp_dampening_start option
void init_format_defaults( pvp_config_t& pvp, bool user_set_dampening_start = false );

// Populate pvp_config_t fields from spell 134735 effects by subtype
void init_modifiers_from_spell( pvp_config_t& pvp, const spell_data_t* pvp_rules, sim_t* sim );

} // namespace pvp

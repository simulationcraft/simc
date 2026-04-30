// engine/sim/pvp_scaling.hpp — PvP combat modifier configuration for WoW 12.0.1 (Midnight).
//
// Centralizes all PvP-specific scaling into pvp_config_t, which is stored on sim_t as sim->pvp.
// Enabled via the existing pvp=1 option or by setting any pvp_* option directly.
//
// Architecture:
//   sim_t::init() loads spell 134735 ("PvP Rules Enabled") and calls
//   pvp::init_modifiers_from_spell() to populate config fields from DBC aura subtypes.
//   pvp::init_format_defaults() then adjusts dampening timing for the chosen format.
//   pvp::parse_coefficient_overrides() parses any user-provided per-spell overrides.
//
// Where modifiers are applied (see diff against midnight branch):
//   action.cpp   — crit bonus reduction (total_crit_bonus), per-spell pvp_coeff
//                   multiplier in calculate_direct_amount / calculate_tick_amount,
//                   new virtual get_pvp_coefficient() with 4-priority lookup.
//   heal.cpp     — crit bonus reduction for heals (total_crit_bonus).
//   player.cpp   — versatility (damage/heal/DR), pet damage, healing received,
//                   absorb received, dampening, stamina, resistance, Gladiator's
//                   Distinction 2pc (set 1458 / spell 365043) attribute bonuses.
//   sim.cpp      — dampening_event_t (spell 110310), PvP init in sim_t::init(),
//                   dampening scheduling in combat_begin(), all pvp_* options.
//   sc_item_data — ITEM_BONUS_SET_ILEVEL_PVP (type 43) for Midnight PvP ilvl.
//   report_*     — HTML section and JSON block for PvP modifier reporting.
//
// All modifier defaults are 1.0 (no change). DBC data is authoritative; fields that
// are 0 in the DBC (everything except crit_damage_mod) resolve to 1.0. Users can
// override any value via pvp_* sim options. Tier set reduction and versatility
// effectiveness are handled by DBC pvp_coeff on the spells themselves. (2026-03-20)
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
  std::string mode = "arena";  // "arena", "bg", or "wargame"

  // Sub-toggles for individual PvP systems (all default on).
  // Each gates a block of logic in the engine; disable to isolate behavior.
  bool coefficients    = true;   // per-spell DBC pvp_coeff multipliers (action.cpp)
  bool trinket_bonus   = true;   // Gladiator's Distinction 2pc detection (player.cpp)
  bool stat_scaling    = true;   // global stat modifiers from spell 134735 (player.cpp)
  bool item_scaling    = true;   // PvP item level via bonus type 43 (sc_item_data.cpp)
  bool tier_penalty    = true;   // tier set effectiveness (DBC pvp_coeff on set spells)

  // Spell 134735 effects — populated from DBC by aura subtype at init.
  // As of 12.0.1 only crit_damage_mod (A_448, effect 3) is non-zero (-50%).
  // All others are 0 in DBC but can be hotfixed by Blizzard without a patch.
  // Remaining effects skipped: A_PROC_TRIGGER_SPELL (42), A_CHECK_PVP_STATE (119),
  // A_MOD_EXPERTISE (240), subtype 504 — none are gameplay-relevant to SimC.
  double healing_received_mod = 1.0;  // A_MOD_HEALING_RECEIVED_PCT (118)
  double absorb_received_mod  = 1.0;  // A_MOD_ABSORB_RECEIVED_PERCENT (422)
  double absorb_done_mod      = 1.0;  // outgoing absorb override (no DBC source, user-only)
  double crit_damage_mod      = 1.0;  // A_448 (448) — set to 0.50 from DBC at runtime
  double mana_regen_mod       = 1.0;  // A_MOD_MANA_REGEN_PCT (379)
  double pet_damage_mod       = 1.0;  // A_MOD_PET_DAMAGE_DONE (429)
  double resistance_mod       = 1.0;  // A_MOD_RESISTANCE_PCT (101)

  // Versatility PvP effectiveness (user-configurable override, default = full).
  // Not sourced from spell 134735; set manually if Blizzard reduces vers in PvP.
  double versatility_damage_mod  = 1.0;
  double versatility_healing_mod = 1.0;
  double versatility_dr_mod      = 1.0;

  // Stamina / Primary Stat (user-configurable override)
  double stamina_mod      = 1.0;
  double primary_stat_mod = 1.0;

  // Rating (user-configurable override)
  double rating_multiplier = 1.0;

  // Tier set effectiveness — reporting/override only. Actual reduction is applied
  // via DBC pvp_coeff on the tier set bonus spells through get_pvp_coefficient().
  double tier_set_effectiveness = 1.0;

  // Dampening (spell 110310): progressive healing/absorb reduction.
  // Implemented as dampening_event_t in sim.cpp, applied in player.cpp via
  // pvp_dampening_multiplier on composite_player_heal_multiplier and
  // composite_player_absorb_multiplier.
  // Arena: starts at 0s. Wargame: starts at 300s. BG: disabled. (2026-03-20)
  bool   dampening_enabled        = true;
  double dampening_start_sec      = 300.0;   // overridden to 0.0 for arena format
  double dampening_stack_interval = 10.0;    // seconds between stacks
  double dampening_pct_per_stack  = 0.01;    // 1% per stack
  double dampening_max_pct        = 1.0;     // 100% max reduction

  // Per-spell coefficient overrides — format: "spell_id:coeff/eeffect_id:coeff"
  // Parsed by parse_coefficient_overrides(). Looked up in get_pvp_coefficient()
  // (action.cpp) with priority: effect override > spell override > item bonus > DBC.
  std::string coefficient_override_str;
  std::unordered_map<unsigned, double> coefficient_overrides;   // spell-level
  std::unordered_map<unsigned, double> effect_overrides;        // effect-level (prefix 'e')
  std::unordered_map<unsigned, double> item_bonus_coefficients; // from item bonus data
};

// Parse user override string into coefficient_overrides and effect_overrides maps.
// Format: "53:0.90/e280:0.85" — 'e' prefix targets effect IDs, bare IDs target spells.
void parse_coefficient_overrides( pvp_config_t& pvp );

// Set dampening defaults based on PvP format. Called after init_modifiers_from_spell.
// user_set_dampening_start: true if user explicitly provided pvp_dampening_start option.
void init_format_defaults( pvp_config_t& pvp, bool user_set_dampening_start = false );

// Iterate spell 134735 effects by aura subtype and populate pvp_config_t fields.
// Called from sim_t::init(). Logs unhandled subtypes at debug level.
void init_modifiers_from_spell( pvp_config_t& pvp, const spell_data_t* pvp_rules, sim_t* sim );

} // namespace pvp

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include "util/format.hpp"

#include <cassert>

class dbc_t;

struct rating_t
{
  double  spell_haste,  spell_hit,  spell_crit;
  double attack_haste, attack_hit, attack_crit;
  double ranged_haste, ranged_hit, ranged_crit;
  double expertise;
  double dodge, parry, block;
  double mastery;
  double pvp_resilience, pvp_power;
  double damage_versatility, heal_versatility, mitigation_versatility;
  double leech, speed, avoidance;
  double corruption, corruption_resistance;

  double& get_mutable(rating_e r);

  double get(rating_e r) const;

  // Initialize all ratings to a very high number by default
  rating_t(double max = +1.0E+50);

  void init(dbc_t& dbc, int level);

  friend fmt::format_context::iterator format_to( const rating_t&, fmt::format_context& );
};
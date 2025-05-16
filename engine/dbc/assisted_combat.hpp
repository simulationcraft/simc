// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef ASSISTED_COMBAT_DATA_HPP
#define ASSISTED_COMBAT_DATA_HPP

#include "client_data.hpp"
#include "sc_enums.hpp"
#include "specialization.hpp"
#include "util/span.hpp"

struct assisted_combat_step_data_t
{
  unsigned id;
  unsigned spec_idx;
  unsigned order_index;
  unsigned spell_id;

  static util::span<const assisted_combat_step_data_t> data( bool ptr );
  static util::span<const assisted_combat_step_data_t> data( specialization_e spec, bool ptr );
};

struct assisted_combat_rule_data_t
{
  unsigned id;
  unsigned assisted_combat_step_id;
  unsigned order_index;
  unsigned condition_type;
  unsigned condition_value_1;
  unsigned condition_value_2;
  unsigned condition_value_3;

  static util::span<const assisted_combat_rule_data_t> data( bool ptr );
  static util::span<const assisted_combat_rule_data_t> data( unsigned assiasted_combat_step_id, bool ptr );
};

#endif /* ASSISTED_COMBAT_DATA_HPP */

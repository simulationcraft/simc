// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "action.hpp"

struct spell_base_t : public action_t
{
  // special item flags
  bool procs_courageous_primal_diamond;

  spell_base_t( action_e at, util::string_view token, player_t* p);
  spell_base_t( action_e at, util::string_view token, player_t* p, const spell_data_t* s );

  // Spell Base Overrides
  double cost() const override;
  timespan_t execute_time() const override;
  result_e   calculate_result( action_state_t* ) const override;
  void   execute() override;
  void   schedule_execute( action_state_t* execute_state = nullptr ) override;

  double composite_crit_chance() const override;

  double composite_haste() const override;

  double composite_crit_chance_multiplier() const override;

  double recharge_multiplier( const cooldown_t& cd ) const override;

  proc_types proc_type() const override;
};

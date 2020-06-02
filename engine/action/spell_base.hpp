// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_action.hpp"

struct spell_base_t : public action_t
{
  // special item flags
  bool procs_courageous_primal_diamond;

  spell_base_t( action_e at, util::string_view token, player_t* p);
  spell_base_t( action_e at, util::string_view token, player_t* p, const spell_data_t* s );

  // Spell Base Overrides
  virtual timespan_t execute_time() const override;
  virtual result_e   calculate_result( action_state_t* ) const override;
  virtual void   execute() override;
  virtual void   schedule_execute( action_state_t* execute_state = nullptr ) override;

  virtual double composite_crit_chance() const override;

  virtual double composite_haste() const override;

  virtual double composite_crit_chance_multiplier() const override;

  double recharge_multiplier( const cooldown_t& cd ) const override;

  virtual proc_types proc_type() const override;
};

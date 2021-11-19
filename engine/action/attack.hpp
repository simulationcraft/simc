// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_action.hpp"

constexpr double WEAPON_POWER_COEFFICIENT = 6; // WDPS -> Attack Power Coefficient used for BfA Attack Power calculations

struct attack_t : public action_t
{
  double base_attack_expertise;

  attack_t( util::string_view token, player_t* p );
  attack_t( util::string_view token, player_t* p, const spell_data_t* s );

  // Attack Overrides
  timespan_t execute_time() const override;
  void execute() override;
  result_e calculate_result( action_state_t* ) const override;
  void init() override;

  result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;

  double miss_chance( double hit, player_t* t ) const override;
  double dodge_chance( double /* expertise */, player_t* t ) const override;
  double block_chance( action_state_t* s ) const override;
  double crit_block_chance( action_state_t* s ) const override;

  double action_multiplier() const override;

  double composite_target_multiplier( player_t* target ) const override;
  double composite_hit() const override;
  double composite_crit_chance() const override;
  double composite_crit_chance_multiplier() const override;
  double composite_haste() const override;
  double composite_versatility( const action_state_t* state ) const override;
  double recharge_multiplier( const cooldown_t& cd ) const override;

  virtual double composite_expertise() const;
  virtual void reschedule_auto_attack( double old_swing_haste );

  void reset() override;

private:
  /// attack table generator with caching
  struct attack_table_t{
    std::array<double, RESULT_MAX> chances;
    std::array<result_e, RESULT_MAX> results;
    int num_results;
    double attack_table_sum; // Used to check whether we can use cached values or not.

    attack_table_t()
    {reset(); }

    void reset()
    { attack_table_sum = std::numeric_limits<double>::min(); }

    void build_table( double miss_chance, double dodge_chance,
                      double parry_chance, double glance_chance,
                      double crit_chance, sim_t* );
  };
  mutable attack_table_t attack_table;


};

// Melee Attack ===================================================================

struct melee_attack_t : public attack_t
{
  melee_attack_t( util::string_view token, player_t* p );
  melee_attack_t( util::string_view token, player_t* p, const spell_data_t* s );

  // Melee Attack Overrides
  void init() override;
  double parry_chance( double /* expertise */, player_t* t ) const override;
  double glance_chance( int delta_level ) const override;

  proc_types proc_type() const override;
};

// Ranged Attack ===================================================================

struct ranged_attack_t : public attack_t
{
  ranged_attack_t( util::string_view token, player_t* p );
  ranged_attack_t( util::string_view token, player_t* p, const spell_data_t* s );

  // Ranged Attack Overrides
  double composite_target_multiplier( player_t* ) const override;
  void schedule_execute( action_state_t* execute_state = nullptr ) override;

  proc_types proc_type() const override;
};
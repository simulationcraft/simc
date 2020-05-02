// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_action.hpp"

struct attack_t : public action_t
{
  double base_attack_expertise;

  attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Attack Overrides
  virtual timespan_t execute_time() const override;
  virtual void execute() override;
  virtual result_e calculate_result( action_state_t* ) const override;
  virtual void   init() override;

  virtual result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  virtual result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;

  virtual double  miss_chance( double hit, player_t* t ) const override;
  virtual double  dodge_chance( double /* expertise */, player_t* t ) const override;
  virtual double  block_chance( action_state_t* s ) const override;
  virtual double  crit_block_chance( action_state_t* s ) const override;

  virtual double action_multiplier() const override
  {
    double mul = action_t::action_multiplier();

    if ( ! special )
    {
      mul *= player -> auto_attack_multiplier;
    }

    return mul;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double mul = action_t::composite_target_multiplier( target );

    mul *= composite_target_damage_vulnerability( target );

    return mul;
  }

  virtual double composite_hit() const override
  { return action_t::composite_hit() + player -> cache.attack_hit(); }

  virtual double composite_crit_chance() const override
  { return action_t::composite_crit_chance() + player -> cache.attack_crit_chance(); }

  virtual double composite_crit_chance_multiplier() const override
  { return action_t::composite_crit_chance_multiplier() * player -> composite_melee_crit_chance_multiplier(); }

  virtual double composite_haste() const override
  { return action_t::composite_haste() * player -> cache.attack_haste(); }

  virtual double composite_expertise() const
  { return base_attack_expertise + player -> cache.attack_expertise(); }

  virtual double composite_versatility( const action_state_t* state ) const override
  { return action_t::composite_versatility( state ) + player -> cache.damage_versatility(); }

  double recharge_multiplier( const cooldown_t& cd ) const override;

  virtual void reschedule_auto_attack( double old_swing_haste );

  virtual void reset() override
  {
    attack_table.reset();
    action_t::reset();
  }

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
  melee_attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Melee Attack Overrides
  virtual void init() override;
  virtual double parry_chance( double /* expertise */, player_t* t ) const override;
  virtual double glance_chance( int delta_level ) const override;

  virtual proc_types proc_type() const override;
};

// Ranged Attack ===================================================================

struct ranged_attack_t : public attack_t
{
  ranged_attack_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Ranged Attack Overrides
  virtual double composite_target_multiplier( player_t* ) const override;
  virtual void schedule_execute( action_state_t* execute_state = nullptr ) override;

  virtual proc_types proc_type() const override;
};
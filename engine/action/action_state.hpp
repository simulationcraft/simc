// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <cstddef>
#include <string>
#include <iosfwd>
#include "config.hpp"
#include "util/generic.hpp"
#include "sc_enums.hpp"
#include "dbc/data_enums.hh"
#include "sim/event.hpp"

struct action_t;
struct player_t;

struct action_state_t : private noncopyable
{
  action_state_t* next;
  // Source action, target actor
  action_t*       action;
  player_t*       target;
  // Execution attributes
  unsigned        n_targets;            // Total number of targets the execution hits.
  int             chain_target;         // The chain target number, 0 == no chain, 1 == first target, etc.
  double original_x;
  double original_y;
  // Execution results
  result_amount_type           result_type;
  result_e        result;
  block_result_e  block_result;
  double          result_raw;             // Base result value, without crit/glance etc.
  double          result_total;           // Total unmitigated result, including crit bonus, glance penalty, etc.
  double          result_mitigated;       // Result after mitigation / resist. *NOTENOTENOTE* Only filled after action_t::impact() call
  double          result_absorbed;        // Result after absorption. *NOTENOTENOTE* Only filled after action_t::impact() call
  double          result_crit_bonus;      // Crit bonus multiplier used in the final calculation
  double          result_amount;          // Final (actual) result
  double          blocked_amount;         // The amount of damage reduced via block or critical block
  double          self_absorb_amount;     // The amount of damage reduced via personal absorbs such as shield_barrier.
  // Snapshotted stats during execution
  double          haste;
  double          crit_chance;
  double          target_crit_chance;
  double          attack_power;
  double          spell_power;
  // Snapshotted multipliers
  double          versatility;
  double          da_multiplier;
  double          ta_multiplier;
  double          rolling_ta_multiplier;
  double          player_multiplier;
  double          persistent_multiplier;
  double          pet_multiplier; // Owner -> pet multiplier
  double          target_da_multiplier;
  double          target_ta_multiplier;
  double          target_pet_multiplier;
  // Target mitigation multipliers
  double          target_mitigation_da_multiplier;
  double          target_mitigation_ta_multiplier;
  double          target_armor;

  static void release( action_state_t*& s );
  static std::string flags_to_str( unsigned flags );

  action_state_t( action_t*, player_t* );
  virtual ~action_state_t() = default;

  virtual void copy_state( const action_state_t* );
  virtual void initialize();

  virtual std::ostringstream& debug_str( std::ostringstream& s );
  virtual void debug();

  virtual double composite_crit_chance() const
  { return crit_chance + target_crit_chance; }

  virtual double composite_attack_power() const
  { return attack_power; }

  virtual double composite_spell_power() const
  { return spell_power; }

  virtual double composite_versatility() const
  { return versatility; }

  virtual double composite_da_multiplier() const
  {
    return da_multiplier * player_multiplier * persistent_multiplier * target_da_multiplier * versatility *
           pet_multiplier * target_pet_multiplier;
  }

  virtual double composite_ta_multiplier() const
  {
    return ta_multiplier * player_multiplier * persistent_multiplier * target_ta_multiplier * versatility *
           pet_multiplier * target_pet_multiplier;
  }

  virtual double composite_rolling_ta_multiplier() const
  {
    return rolling_ta_multiplier;
  }

  virtual double composite_target_mitigation_da_multiplier() const
  { return target_mitigation_da_multiplier; }

  virtual double composite_target_mitigation_ta_multiplier() const
  { return target_mitigation_ta_multiplier; }

  virtual double composite_target_armor() const
  { return target_armor; }

  // Inlined
  virtual proc_types proc_type() const;
  virtual proc_types2 execute_proc_type2() const;

  // Secondary proc type of the impact event (i.e., assess_damage()). Only
  // triggers the "amount" procs
  virtual proc_types2 impact_proc_type2() const
  {
    // Don't allow impact procs that do not do damage or heal anyone; they
    // should all be handled by execute_proc_type2(). Note that this is based
    // on the _total_ amount done. This is so that fully overhealed heals are
    // still alowed to proc things.
    if ( result_total <= 0 )
      return PROC2_INVALID;

    if ( result == RESULT_HIT )
      return PROC2_HIT;
    else if ( result == RESULT_CRIT )
      return PROC2_CRIT;
    else if ( result == RESULT_GLANCE )
      return PROC2_GLANCE;

    return PROC2_INVALID;
  }

  virtual proc_types2 cast_proc_type2() const;

  virtual proc_types2 interrupt_proc_type2() const;
};

struct travel_event_t : public event_t
{
  action_t* action;
  action_state_t* state;
  travel_event_t( action_t* a, action_state_t* state, timespan_t time_to_travel );
  ~travel_event_t() override;
  void execute() override;
  const char* name() const override
  {
    return "Stateless Action Travel";
  }
};

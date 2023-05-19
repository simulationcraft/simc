// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "spell_base.hpp"

struct heal_t : public spell_base_t
{
public:
  using base_t = spell_base_t;
  bool group_only;

private:
  // Determines if stats for healing actions are recorded. Initialized to player->record_healing()
  bool record_healing;

public:
  double base_pct_heal;
  double tick_pct_heal;
  gain_t* heal_gain;

  heal_t( util::string_view name, player_t* p );
  heal_t( util::string_view name, player_t* p, const spell_data_t* s );

  player_t* find_greatest_difference_player();
  player_t* find_lowest_player();
  std::vector<player_t*> find_lowest_players( int num_players ) const;
  player_t* smart_target() const;  // Find random injured healing target, preferring non-pets // Might need to move up
                                   // hierarchy if there are smart absorbs
  void init() override;
  void parse_heal_effect_data( const spelleffect_data_t& );

  virtual double composite_pct_heal( const action_state_t* ) const;

  // overrides
  void assess_damage( result_amount_type, action_state_t* ) override;
  result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;
  size_t available_targets( std::vector<player_t*>& ) const override;
  void activate() override;
  double calculate_direct_amount( action_state_t* state ) const override;
  double calculate_tick_amount( action_state_t* state, double dmg_multiplier ) const override;
  int num_targets() const override;
  double composite_da_multiplier( const action_state_t* s ) const override;
  double composite_ta_multiplier( const action_state_t* s ) const override;
  double composite_player_critical_multiplier( const action_state_t* /* s */ ) const override;
  double composite_versatility( const action_state_t* state ) const override;
  player_t* get_expression_target() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;
};

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "player/target_specific.hpp"
#include "spell_base.hpp"

struct action_state_t;
struct absorb_buff_t;

struct absorb_t : public spell_base_t
{
  target_specific_t<absorb_buff_t> target_specific;

  absorb_t(util::string_view name, player_t* p, const spell_data_t* s);

  // Allows customization of the absorb_buff_t that we are creating.
  virtual absorb_buff_t* create_buff(const action_state_t* s);

  void assess_damage(result_amount_type, action_state_t*) override;
  result_amount_type amount_type(const action_state_t* /* state */, bool /* periodic */ = false) const override;
  void impact(action_state_t*) override;
  void activate() override;
  size_t available_targets(std::vector< player_t* >&) const override;
  int num_targets() const override;

  double composite_da_multiplier(const action_state_t* s) const override;
  double composite_ta_multiplier(const action_state_t* s) const override;
  double composite_versatility(const action_state_t* state) const override;
  double composite_target_multiplier( player_t* target ) const override;
};
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "spell_base.hpp"
#include "player/target_specific.hpp"

struct action_state_t;

struct absorb_t : public spell_base_t
{
  target_specific_t<absorb_buff_t> target_specific;

  absorb_t(const std::string& name, player_t* p, const spell_data_t* s = spell_data_t::nil());

  // Allows customization of the absorb_buff_t that we are creating.
  virtual absorb_buff_t* create_buff(const action_state_t* s);

  virtual void assess_damage(result_amount_type, action_state_t*) override;
  virtual result_amount_type amount_type(const action_state_t* /* state */, bool /* periodic */ = false) const override
  {
    return result_amount_type::ABSORB;
  }
  virtual void impact(action_state_t*) override;
  virtual void activate() override;
  virtual size_t available_targets(std::vector< player_t* >&) const override;
  virtual int num_targets() const override;

  virtual double composite_da_multiplier(const action_state_t* s) const override
  {
    double m = action_multiplier() * action_da_multiplier() *
      player->composite_player_absorb_multiplier(s);

    return m;
  }
  virtual double composite_ta_multiplier(const action_state_t* s) const override
  {
    double m = action_multiplier() * action_ta_multiplier() *
      player->composite_player_absorb_multiplier(s);

    return m;
  }
  virtual double composite_versatility(const action_state_t* state) const override
  {
    return spell_base_t::composite_versatility(state) + player->cache.heal_versatility();
  }
};
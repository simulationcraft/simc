// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "spell_base.hpp"

struct spell_t : public spell_base_t
{
public:
  spell_t( const std::string& token, player_t* p, const spell_data_t* s = spell_data_t::nil() );

  // Harmful Spell Overrides
  virtual result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  virtual result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;
  virtual double miss_chance( double hit, player_t* t ) const override;
  virtual void   init() override;
  virtual double composite_hit() const override
  { return action_t::composite_hit() + player -> cache.spell_hit(); }
  virtual double composite_versatility( const action_state_t* state ) const override
  { return spell_base_t::composite_versatility( state ) + player -> cache.damage_versatility(); }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double mul = action_t::composite_target_multiplier( target );

    mul *= composite_target_damage_vulnerability( target );

    return mul;
  }

};
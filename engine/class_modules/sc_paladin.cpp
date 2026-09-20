// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO: reimplement Holy if anyone ever becomes interested in maintaining it
*/
#include "simulationcraft.hpp"
#include "action/dbc_proc_callback.hpp"
#include "action/parse_effects.hpp"
#include "item/special_effect.hpp"
#include <algorithm>

// ==========================================================================
// Paladin
// ==========================================================================
namespace paladin
{
struct paladin_t final : public player_t
{
  paladin_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, PALADIN, name, r ) {}
};

// PALADIN MODULE INTERFACE =================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new paladin_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_actor_initializers( sim_t* sim ) const override {}

  void register_hotfixes() const override {}
};
}  // end namespace paladin

const module_t* module_t::paladin()
{
  static paladin::paladin_module_t m;
  return &m;
}

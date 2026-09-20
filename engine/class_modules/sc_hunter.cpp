//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <memory>
#include <optional>

#include "simulationcraft.hpp"
#include "player/pet_spawner.hpp"
#include "class_modules/apl/apl_hunter.hpp"

namespace
{ // UNNAMED NAMESPACE
struct hunter_t final : public player_t
{
  hunter_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, HUNTER, name, r ) {}
};

// HUNTER MODULE INTERFACE ==================================================

struct hunter_module_t: public module_t
{
  hunter_module_t(): module_t( HUNTER ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto  p = new hunter_t( sim, name, r );
    return p;
  }

  bool valid() const override { return true; }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::hunter()
{
  static hunter_module_t m;
  return &m;
}

// ==========================================================================
// Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "simulationcraft.hpp"

namespace priestspace
{
struct priest_t final : public player_t
{
  priest_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, PRIEST, name, r ) {}
};

struct priest_module_t final : public module_t
{
  priest_module_t() : module_t( PRIEST ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new priest_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* sim ) const override {}
};

}  // namespace priestspace

const module_t* module_t::priest()
{
  static priestspace::priest_module_t m;
  return &m;
}

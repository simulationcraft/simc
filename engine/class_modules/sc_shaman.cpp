// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include "action/action.hpp"
#include "action/action_state.hpp"
#include "action/attack.hpp"
#include "action/dot.hpp"
#include "action/heal.hpp"
#include "action/residual_action.hpp"
#include "action/spell.hpp"
#include "class_modules/class_module.hpp"
#include "dbc/data_enums.hh"
#include "dbc/dbc.hpp"
#include "item/item.hpp"
#include "item/special_effect.hpp"
#include "player/action_priority_list.hpp"
#include "player/actor_target_data.hpp"
#include "player/ground_aoe.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "action/action_callback.hpp"
#include "action/parse_effects.hpp"
#include "report/highchart.hpp"
#include "player/player_scaling.hpp"
#include "player/set_bonus.hpp"
#include "report/decorators.hpp"
#include "sc_enums.hpp"
#include "sim/cooldown.hpp"
#include "sim/proc.hpp"
#include "sim/proc_rng.hpp"
#include "util/string_view.hpp"

#include <cassert>
#include <string>
#include <sstream>

namespace {

struct shaman_t final : public player_t
{
  shaman_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, SHAMAN, name, r ) {}
};

// SHAMAN MODULE INTERFACE ==================================================

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new shaman_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_actor_initializers( sim_t* sim ) const override
  {
    sim->register_actor_initializer( INIT_ACTOR_CREATE_BUFFS + offset(), []( player_t* p ) {
      p->buffs.bloodlust = make_buff( p, "bloodlust", p->find_spell( 2825 ) )
          ->set_cooldown( 0_ms )
          ->set_max_stack( 1 )
          ->set_default_value_from_effect_type( A_HASTE_ALL )
          ->add_invalidate( CACHE_HASTE );

      p->buffs.exhaustion = make_buff( p, "exhaustion", p->find_spell( 57723 ) )
          ->set_max_stack( 1 )
          ->set_quiet( true );
    }, "create_buffs_shaman" );
  }

  void register_hotfixes() const override {}
};

}  // namespace

const module_t* module_t::shaman()
{
  static ::shaman_module_t m;
  return &m;
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action_callback.hpp"
#include "sc_action.hpp"
#include "player/sc_player.hpp"

action_callback_t::action_callback_t( player_t* l, bool ap, bool asp ) :
  listener( l ), active( true ), allow_self_procs( asp ), allow_procs( ap )
{
  assert( l );
  if ( range::find( l->callbacks.all_callbacks, this ) == l->callbacks.all_callbacks.end() )
    l->callbacks.all_callbacks.push_back( this );
}

void action_callback_t::trigger(const std::vector<action_callback_t*>& v, action_t* a, action_state_t* state )
{
  if (a && !a->player->in_combat) return;

  std::size_t size = v.size();
  for (std::size_t i = 0; i < size; i++)
  {
    action_callback_t* cb = v[i];
    if (cb->active)
    {
      if (!cb->allow_procs && a && a->proc) return;
      cb->trigger(a, state);
    }
  }
}

void action_callback_t::reset(const std::vector<action_callback_t*>& v)
{
  std::size_t size = v.size();
  for (std::size_t i = 0; i < size; i++)
  {
    v[i]->reset();
  }
}

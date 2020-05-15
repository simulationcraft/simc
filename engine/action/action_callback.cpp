// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action_callback.hpp"
#include "sc_action.hpp"
#include "player/sc_player.hpp"

action_callback_t::action_callback_t(player_t* l, bool ap, bool asp) :
  listener(l), active(true), allow_self_procs(asp), allow_procs(ap)
{
  assert(l);
  if (std::find(l->callbacks.all_callbacks.begin(), l->callbacks.all_callbacks.end(), this)
    == l->callbacks.all_callbacks.end())
    l->callbacks.all_callbacks.push_back(this);
}

void action_callback_t::trigger(const std::vector<action_callback_t*>& v, action_t* a, void* call_data)
{
  if (a && !a->player->in_combat) return;

  std::size_t size = v.size();
  for (std::size_t i = 0; i < size; i++)
  {
    action_callback_t* cb = v[i];
    if (cb->active)
    {
      if (!cb->allow_procs && a && a->proc) return;
      cb->trigger(a, call_data);
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

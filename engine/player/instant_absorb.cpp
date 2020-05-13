// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "instant_absorb.hpp"

#include "action/sc_action_state.hpp"
#include "dbc/dbc.hpp"
#include "util/util.hpp"
#include "player/stats.hpp"
#include "player/sc_player.hpp"
#include <algorithm>

instant_absorb_t::instant_absorb_t(player_t* p, const spell_data_t* s, const std::string n, std::function<double(const action_state_t*)> handler) :
  /* spell( s ), */ absorb_handler(handler), player(p), name(n)
{
  util::tokenize(name);

  absorb_stats = p->get_stats(name);
  absorb_gain = p->get_gain(name);
  absorb_stats->type = STATS_ABSORB;
  absorb_stats->school = s->get_school_type();
}

double instant_absorb_t::consume(const action_state_t* s)
{
  double amount = std::min(s->result_amount, absorb_handler(s));

  if (amount > 0)
  {
    absorb_stats->add_result(amount, 0, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player);
    absorb_stats->add_execute(timespan_t::zero(), player);
    absorb_gain->add(RESOURCE_HEALTH, amount, 0);

    if (player->sim->debug)
      player->sim->out_debug.printf("%s %s absorbs %.2f", player->name(), name.c_str(), amount);
  }

  return amount;
}

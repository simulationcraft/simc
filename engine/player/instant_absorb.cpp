// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "instant_absorb.hpp"

#include "action/sc_action_state.hpp"
#include "dbc/dbc.hpp"
#include "player/sc_player.hpp"
#include "player/stats.hpp"
#include "util/util.hpp"

#include <algorithm>
#include <utility>


instant_absorb_t::instant_absorb_t( player_t* p, const spell_data_t* s, util::string_view n,
                                    std::function<double( const action_state_t* )> handler )
  : /* spell( s ), */ absorb_handler( std::move(handler) ), player( p ), name( util::tokenize_fn( n ) )
{
  absorb_stats         = p->get_stats( name );
  absorb_gain          = p->get_gain( name );
  absorb_stats->type   = STATS_ABSORB;
  absorb_stats->school = s->get_school_type();
}

double instant_absorb_t::consume( const action_state_t* s )
{
  double amount = std::min( s->result_amount, absorb_handler( s ) );

  if ( amount > 0 )
  {
    absorb_stats->add_result( amount, 0, result_amount_type::ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, player );
    absorb_stats->add_execute( timespan_t::zero(), player );
    absorb_gain->add( RESOURCE_HEALTH, amount, 0 );

    player->sim->print_debug( "{} instant absorb '{}' absorbs {}", *player, name, amount );
  }

  return amount;
}

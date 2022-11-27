// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "darkmoon_deck.hpp"

#include "dbc/dbc.hpp"

darkmoon_deck_t::darkmoon_deck_t( const special_effect_t& e, std::vector<unsigned> c )
  : effect( e ),
    player( e.player ),
    card_ids( std::move( c ) ),
    shuffle_event( nullptr ),
    shuffle_period( effect.driver()->effectN( 1 ).period() ),
    top_index( 0 )
{}

size_t darkmoon_deck_t::get_index( bool )
{
  top_index = player->rng().range( card_ids.size() );
  return top_index;
}

timespan_t shuffle_event_t::delta_time( sim_t& sim, bool initial, darkmoon_deck_t* deck )
{
  if ( initial )
  {
    return deck->shuffle_period * sim.rng().real();
  }
  else
  {
    return deck->shuffle_period;
  }
}

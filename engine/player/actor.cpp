// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "actor.hpp"

actor_t::actor_t( sim_t* s, util::string_view name ) :
  sim( s ), spawner( nullptr ), name_str( name ),
#ifdef ACTOR_EVENT_BOOKKEEPING
  event_counter( 0 ),
  event_stopwatch()
#else
  event_counter( 0 )
#endif
{

}
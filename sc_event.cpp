// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Event Memory Management
// ==========================================================================

#ifdef EVENT_MM

static event_t* free_list = 0;

// event_t::new =============================================================

void* event_t::operator new( size_t size )
{
  static size_t SIZE = 2 * sizeof( event_t );
  assert( SIZE > size );

  event_t* new_event = free_list;

  if( new_event )
  {
    free_list = free_list -> next;
  }
  else
  {
    new_event = (event_t*) malloc( SIZE );
  }

  return new_event;
}

// event_t::delete ==========================================================

void event_t::operator delete( void* p )
{
  event_t* e = (event_t*) p;
  e -> next = free_list;
  free_list = e;
}

#endif

// ==========================================================================
// Player Ready Event
// ==========================================================================

// player_ready_event_t::player_ready_event_t ===============================

player_ready_event_t::player_ready_event_t( sim_t*    sim, 
                                            player_t* p, 
                                            double     delta_time ) : 
  event_t( sim, p ) 
{
  name = "Player-Ready";
  if( sim -> debug ) report_t::log( sim, "New Player-Ready Event: %s", p -> name() );
  sim -> add_event( this, delta_time );
}

// player_ready_event_t::execute =============================================

void player_ready_event_t::execute()
{
   if( ! player -> execute_action() )
   {
     player -> schedule_ready( 0.1, true );
   }
}   

// ==========================================================================
// Action Execute Event
// ==========================================================================

// action_execute_event_t::action_execute_event_t ===========================

action_execute_event_t::action_execute_event_t( sim_t*    sim, 
						action_t* a,
						double    time_to_execute ) : 
  event_t( sim, a -> player ), action( a ) 
{
  name = "Action-Execute";
  if( sim -> debug ) report_t::log( sim, "New Action Execute Event: %s %s %.1f", player -> name(), a -> name(), time_to_execute );
  sim -> add_event( this, time_to_execute );
}

// action_execute_event_t::execute ===========================================

void action_execute_event_t::execute()
{
  action -> event = 0;
  action -> execute();

  if( ! action -> background && 
      ! player -> channeling &&
      ! player -> sleeping ) 
  {
    player -> schedule_ready( 0 );
  }
}   

// ==========================================================================
// Action Tick Event
// ==========================================================================

// action_tick_event_t::action_tick_event_t =================================

action_tick_event_t::action_tick_event_t( sim_t*    sim, 
					  action_t* a,
					  double    time_to_tick ) : 
  event_t( sim, a -> player ), action( a )
{
  name = "Action Tick";

  if( sim -> debug ) report_t::log( sim, "New Action Tick Event: %s %s %.2f %d %.2f", 
		   player -> name(), a -> name(), a -> time_remaining, a -> current_tick, time_to_tick );
  
  sim -> add_event( this, time_to_tick );
}   

// action_tick_event_t::execute ==============================================

void action_tick_event_t::execute()
{
   assert( action -> current_tick < action -> num_ticks );

   action -> event = 0;
   action -> time_remaining -= action -> time_remaining / ( action -> num_ticks - action -> current_tick );
   action -> current_tick++;
   action -> tick();

  if( action -> current_tick == action -> num_ticks )
  {
    action -> current_tick = 0;

    assert( action -> time_remaining == 0 );
    
    if( ! action -> background )
    {
      if( action -> channeled || player -> sleeping )
      {
	player -> schedule_ready( 0 );
      }
    }
  }
  else action -> schedule_tick();
}

// ===========================================================================
// Regen Event
// ===========================================================================

// regen_event_t::regen_event_t ==============================================

regen_event_t::regen_event_t( sim_t* sim ) : event_t( sim )
{
  name = "Regen Event";
  if( sim -> debug ) report_t::log( sim, "New Regen Event" );
  sim -> add_event( this, sim -> regen_periodicity );
}

// regen_event_t::execute ====================================================

void regen_event_t::execute()
{
  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> regen( sim -> regen_periodicity );
  }
  
  new regen_event_t( sim );
}   

// ===============================================================================
// Event
// ===============================================================================

void event_t::reschedule( double new_time )
{
  reschedule_time = sim -> current_time + new_time;

  if( sim -> debug ) report_t::log( sim, "Rescheduling event %s (%d) from %.2f to %.2f", name, id, time, reschedule_time );
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Event Memory Management
// ==========================================================================

// ::new ====================================================================

void* event_t::operator new( size_t size,
                             sim_t* sim )
{
  // This override of ::new is ONLY for event_t memory management!

  static size_t SIZE = 2 * sizeof( event_t );
  assert( SIZE > size );

  event_t* new_event = sim -> free_list;

  if ( new_event )
  {
    sim -> free_list = sim -> free_list -> next;
  }
  else
  {
    new_event = ( event_t* ) ::new char[ SIZE ];
  }

  return new_event;
}

// event_t::new =============================================================

void* event_t::operator new( size_t size ) throw()
{
  util_t::fprintf( stderr, "All events must be allocated via: new (sim) event_class_name_t()\n" );
  fflush( stderr );
  assert( 0 );
  return NULL;
}

// event_t::delete ==========================================================

void event_t::operator delete( void* p )
{
  event_t* e = ( event_t* ) p;
  sim_t* sim = e -> sim;
  e -> next = sim -> free_list;
  sim -> free_list = e;
}

// event_t::deallocate ======================================================

void event_t::deallocate( event_t* e )
{
  char* e2 = ( char * ) e;
  ::delete [] e2;
}

void event_t::cancel( event_t*& e ) 
{
  if ( e ) 
  { 
    if ( e -> player && ! e -> canceled ) 
    {
      e -> player -> events--;
      assert( e -> player -> events >= 0 );
    }
    e -> canceled = 1; 
    e = 0; 
  }
}

void event_t::early( event_t*& e )
{ 
  if ( e ) 
  { 
    if ( e -> player && ! e -> canceled ) 
    {
      e -> player -> events--;
      assert( e -> player -> events >= 0 );
    }
    e -> canceled = 1; 
    e -> execute(); 
    e = 0; 
  }
}

// ==========================================================================
// Player Ready Event
// ==========================================================================

// player_ready_event_t::player_ready_event_t ===============================

player_ready_event_t::player_ready_event_t( sim_t*    sim,
                                            player_t* p,
                                            double    delta_time ) :
                                            event_t( sim, p )
{
  name = "Player-Ready";
  if ( sim -> debug ) log_t::output( sim, "New Player-Ready Event: %s", p -> name() );
  sim -> add_event( this, delta_time );
}

// player_ready_event_t::execute =============================================

void player_ready_event_t::execute()
{
  if ( ! player -> execute_action() )
  {
    player -> schedule_ready( player -> available(), true );
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
  if ( sim -> debug ) log_t::output( sim, "New Action Execute Event: %s %s %.1f", player -> name(), a -> name(), time_to_execute );
  sim -> add_event( this, time_to_execute );
}

// action_execute_event_t::execute ===========================================

void action_execute_event_t::execute()
{
  action -> execute_event = 0;
  action -> execute();

  if ( ! action -> background &&
       ! player -> channeling )
  {
    player -> schedule_ready( 0 );
  }
}

// ==========================================================================
// DoT Tick Event
// ==========================================================================

// dot_tick_event_t::dot_tick_event_t =======================================

dot_tick_event_t::dot_tick_event_t( sim_t* sim,
                                    dot_t* d,
                                    double time_to_tick ) :
  event_t( sim, d -> player ), dot( d )
{
  name = "DoT Tick";

  if ( sim -> debug )
    log_t::output( sim, "New DoT Tick Event: %s %s %d-of-%d %.2f",
                   player -> name(), dot -> name(), dot -> current_tick + 1, dot -> num_ticks, time_to_tick );

  sim -> add_event( this, time_to_tick );
}

// dot_tick_event_t::execute =================================================

void dot_tick_event_t::execute()
{
  if ( dot -> current_tick >= dot -> num_ticks )
  {
    sim -> errorf( "Player %s has corrupt tick (%d of %d) event on action %s!\n",
                   player -> name(), dot -> current_tick, dot -> num_ticks, dot -> name() );
    sim -> cancel();
  }

  dot -> tick_event = 0;
  dot -> current_tick++;

  if ( dot -> action -> channeled &&
       dot -> current_tick == dot -> num_ticks &&
       player -> skill < 1.0 )
  {
    if ( sim -> roll( player -> skill ) )
    {
      dot -> action -> tick();
    }
  }
  else // No skill-check required
  {
    dot -> action -> tick();
  }

  if ( dot -> action -> channeled && ( dot -> ticks() > 0 ) )
  {
    action_expr_t* expr = dot -> action -> interrupt_if_expr;
    if ( expr && expr -> success() )
    {
      dot -> current_tick = dot -> num_ticks;
    }
    if ( dot -> action -> interrupt )
    {
      // Interrupt if any higher priority action is ready.
      for( action_t* a = player -> action_list; a != dot -> action; a = a -> next )
      {
        if ( a -> background ) continue;
        if ( a -> ready() )
        {
          dot -> current_tick = dot -> num_ticks;
          break;
        }
      }
    }
  }

  if ( dot -> current_tick == dot -> num_ticks )
  {
    dot -> action -> last_tick();

    if ( dot -> action -> channeled )
    {
      if( player -> readying ) fprintf( sim -> output_file, "Danger Will Robinson!  Danger!  %s\n", dot -> name() );

      player -> schedule_ready( 0 );
    }
  }
  else dot -> action -> schedule_tick();
}

// ==========================================================================
// Action Travel Event
// ==========================================================================

// action_travel_event_t::action_travel_event_t =============================

action_travel_event_t::action_travel_event_t( sim_t*    sim,
                                              player_t* t,
                                              action_t* a,
                                              double    time_to_travel ) :
                                              event_t( sim, a -> player ), action( a ), target( t )
{
  name   = "Action Travel";
  result = a -> result;
  damage = a -> direct_dmg;

  if ( sim -> debug )
    log_t::output( sim, "New Action Travel Event: %s %s %.2f",
                   player -> name(), a -> name(), time_to_travel );

  sim -> add_event( this, time_to_travel );
}

// action_travel_event_t::execute ===========================================

void action_travel_event_t::execute()
{
  action -> travel( target, result, damage );
  if ( action -> travel_event == this )
    action -> travel_event = NULL;
}

// ===========================================================================
// Regen Event
// ===========================================================================

// regen_event_t::regen_event_t ==============================================

regen_event_t::regen_event_t( sim_t* sim ) : event_t( sim )
{
  name = "Regen Event";
  if ( sim -> debug ) log_t::output( sim, "New Regen Event" );
  sim -> add_event( this, sim -> regen_periodicity );
}

// regen_event_t::execute ====================================================

void regen_event_t::execute()
{
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    if ( p -> sleeping ) continue;
    if ( p -> primary_resource() == RESOURCE_NONE ) continue;

    p -> regen( sim -> regen_periodicity );
  }

  new ( sim ) regen_event_t( sim );
}

// ===============================================================================
// Event
// ===============================================================================

void event_t::reschedule( double new_time )
{
  reschedule_time = sim -> current_time + new_time;

  if ( sim -> debug ) log_t::output( sim, "Rescheduling event %s (%d) from %.2f to %.2f", name, id, time, reschedule_time );

  if ( ! strcmp( name, "Rabid Expiration" ) ) assert( false );
}

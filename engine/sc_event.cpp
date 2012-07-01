// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Event Memory Management
// ==========================================================================

// event_freelist_t::allocate ===============================================

void* event_freelist_t::allocate( std::size_t size )
{
  // This override of ::new is ONLY for event_t memory management!
  static const std::size_t SIZE = 2 * sizeof( event_t );
  assert( SIZE > size ); ( void )size;

  free_event_t* new_event = list;

  if ( new_event )
  {
    list = list -> next;
  }
  else
  {
    new_event = static_cast<free_event_t*>( ::operator new( SIZE ) );
  }

  return new_event;
}

// event_freelist_t::deallocate =============================================

void event_freelist_t::deallocate( void* p )
{
  free_event_t* fe = new( p ) free_event_t;
  fe -> next = list;
  list = fe;
}

// event_freelist_t::~event_freelist_t ======================================

event_freelist_t::~event_freelist_t()
{
  while ( list )
  {
    free_event_t* p = list;
    list = list -> next;
    delete p;
  }
}

// ==========================================================================
// Event
// ==========================================================================

// event_t::new =============================================================

void* event_t::operator new( std::size_t /* size */ ) throw()
{
  util::fprintf( stderr, "All events must be allocated via: new (sim) event_class_name_t()\n" );
  fflush( stderr );
  assert( 0 );
  return NULL;
}

void event_t::reschedule( timespan_t new_time )
{
  reschedule_time = sim -> current_time + new_time;

  if ( sim -> debug ) sim -> output( "Rescheduling event %s (%d) from %.2f to %.2f", name, id, time.total_seconds(), reschedule_time.total_seconds() );

//  if ( ! strcmp( name, "Rabid Expiration" ) ) assert( false );
}

// event_t::cancel_ =========================================================

void event_t::cancel_( event_t* e )
{
  assert( e );
  if ( e -> player && ! e -> canceled )
  {
    e -> player -> events--;
    if ( e -> player -> events < 0 )
    {
      e -> sim -> errorf( "event_t::cancel assertion error: e -> player -> events < 0, event %s from %s.\n", e -> name, e -> player -> name() );
      assert( 0 );
    }
  }
  e -> canceled = 1;
}

// event_t::early_ ==========================================================

void event_t::early_( event_t* e )
{
  assert( e );
  if ( e -> player && ! e -> canceled )
  {
    e -> player -> events--;
    assert( e -> player -> events >= 0 );
  }
  e -> canceled = 1;
  e -> execute();
}

// event_t::execute =========================================================

void event_t::execute()
{
  util::printf( "event_t::execute() called for event \"%s\"\n",
                name ? name : "(no name)" );
  assert( 0 );
}

// ==========================================================================
// Player Ready Event
// ==========================================================================

// player_ready_event_t::player_ready_event_t ===============================

player_ready_event_t::player_ready_event_t( sim_t*    sim,
                                            player_t* p,
                                            timespan_t delta_time ) :
  event_t( sim, p, "Player-Ready" )
{
  if ( sim -> debug ) sim -> output( "New Player-Ready Event: %s", p -> name() );
  sim -> add_event( this, delta_time );
}

// player_ready_event_t::execute ============================================

void player_ready_event_t::execute()
{
  // Player that's checking for off gcd actions to use, cancels that checking when there's a ready event firing.
  if ( player -> off_gcd )
    event_t::cancel( player -> off_gcd );

  if ( ! player -> execute_action() )
  {
    if ( player -> ready_type == READY_POLL )
    {
      timespan_t x = player -> available();

      player -> schedule_ready( x, true );

      // Waiting Debug
      if ( sim -> debug )
        sim -> output( "%s is waiting for %.4f resource=%.2f",
                       player -> name(), x.total_seconds(),
                       player -> resources.current[ player -> primary_resource() ] );
    }
    else player -> started_waiting = sim -> current_time;
  }
}

// ==========================================================================
// Player GCD Event
// ==========================================================================

// player_gcd_event_t::player_ready_event_t =================================

player_gcd_event_t::player_gcd_event_t( sim_t*    sim,
                                        player_t* p,
                                        timespan_t delta_time ) :
  event_t( sim, p, "Player-Ready-GCD" )
{
  if ( sim -> debug ) sim -> output( "New Player-Ready-GCD Event: %s", p -> name() );
  sim -> add_event( this, delta_time );
}

// player_gcd_event_t::execute ==============================================

void player_gcd_event_t::execute()
{
  action_t* a = 0;

  for ( std::vector<action_t*>::const_iterator i = player -> active_off_gcd_list -> off_gcd_actions.begin();
        i < player -> active_off_gcd_list -> off_gcd_actions.end(); i++ )
  {
    a = *i;
    if ( a -> ready() )
    {
      action_priority_list_t* alist = player -> active_action_list;

      a -> execute();
      if ( ! a -> quiet )
      {
        player -> iteration_executed_foreground_actions++;
        if ( a -> marker && sim -> current_iteration == 0 )
          player -> report_information.action_sequence.push_back( new action_sequence_data_t( a, a -> target, sim -> current_time ) );
        if ( ! a -> label_str.empty() )
          player -> action_map[ a -> label_str ] += 1;
      }

      // Need to restart because the active action list changed
      if ( alist != player -> active_action_list )
      {
        player -> activate_action_list( player -> active_action_list, true );
        execute();
        player -> activate_action_list( alist, true );
        return;
      }
    }
  }

  if ( player -> restore_action_list != 0 )
  {
    player -> activate_action_list( player -> restore_action_list );
    player -> restore_action_list = 0;
  }

  player -> off_gcd = new ( sim ) player_gcd_event_t( sim, player, timespan_t::from_seconds( 0.1 ) );
}

// ==========================================================================
// Action Execute Event
// ==========================================================================

// action_execute_event_t::action_execute_event_t ===========================

action_execute_event_t::action_execute_event_t( sim_t*    sim,
                                                action_t* a,
                                                timespan_t time_to_execute ) :
  event_t( sim, a -> player, "Action-Execute" ), action( a )
{
  if ( sim -> debug )
    sim -> output( "New Action Execute Event: %s %s %.1f (target=%s, marker=%c)",
                   player -> name(), a -> name(), time_to_execute.total_seconds(), a -> target -> name(), ( a -> marker ) ? a -> marker : '0' );
  sim -> add_event( this, time_to_execute );
}

// action_execute_event_t::execute ==========================================

void action_execute_event_t::execute()
{
  action -> execute_event = 0;
  action -> execute();

  if ( ! action -> background &&
       ! player -> channeling )
  {
    if ( player -> readying ) fprintf( sim -> output_file, "Danger Will Robinson!  Danger!  action %s\n", action -> name() );

    player -> schedule_ready( timespan_t::zero() );
  }

  if ( player -> active_off_gcd_list == 0 )
    return;

  // Kick off the during-gcd checker, first run is immediately after
  event_t::cancel( player -> off_gcd );

  if ( ! player -> channeling )
    player -> off_gcd = new ( sim ) player_gcd_event_t( sim, player, timespan_t::zero() );
}

// ==========================================================================
// DoT Tick Event
// ==========================================================================

// dot_tick_event_t::dot_tick_event_t =======================================

dot_tick_event_t::dot_tick_event_t( sim_t* sim,
                                    dot_t* d,
                                    timespan_t time_to_tick ) :
  event_t( sim, d -> source, "DoT Tick" ), dot( d )
{
  if ( sim -> debug )
    sim -> output( "New DoT Tick Event: %s %s %d-of-%d %.2f",
                   player -> name(), dot -> name(), dot -> current_tick + 1, dot -> num_ticks, time_to_tick.total_seconds() );

  sim -> add_event( this, time_to_tick );
}

// dot_tick_event_t::execute ================================================

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

  if ( dot -> action -> player -> current.skill < 1.0 &&
       dot -> action -> channeled &&
       dot -> current_tick == dot -> num_ticks )
  {
    if ( sim -> roll( dot -> action -> player -> current.skill ) )
    {
      dot -> action -> tick( dot );
    }
  }
  else // No skill-check required
  {
    dot -> action -> tick( dot );
  }

  if ( dot -> action -> channeled && ( dot -> ticks() > 0 ) )
  {
    expr_t* expr = dot -> action -> interrupt_if_expr;
    if ( expr && expr -> success() )
    {
      dot -> current_tick = dot -> num_ticks;
    }
    if ( ( dot -> action -> interrupt || ( dot -> action -> chain && dot -> current_tick + 1 == dot -> num_ticks ) ) && ( dot -> action -> player -> gcd_ready <=  sim -> current_time ) )
    {
      // Interrupt if any higher priority action is ready.
      for ( size_t i = 0; dot -> action -> player -> active_action_list -> foreground_action_list[ i ] != dot -> action; ++i )
      {
        action_t* a = dot -> action -> player -> active_action_list -> foreground_action_list[ i ];
        if ( a -> id == dot -> action -> id ) continue;
        if ( a -> ready() )
        {
          dot -> current_tick = dot -> num_ticks;
          break;
        }
      }
    }
  }

  if ( dot -> ticking )
  {
    if ( dot -> current_tick == dot -> num_ticks )
    {
      dot -> time_to_tick = timespan_t::zero();
      dot -> action -> last_tick( dot );

      if ( dot -> action -> channeled )
      {
        if ( dot -> action -> player -> readying ) fprintf( sim -> output_file, "Danger Will Robinson!  Danger!  %s\n", dot -> name() );

        dot -> action -> player -> schedule_ready( timespan_t::zero() );
      }
    }
    else dot -> schedule_tick();
  }
}

// ==========================================================================
// Action Travel Event
// ==========================================================================

// action_travel_event_t::action_travel_event_t =============================

action_travel_event_t::action_travel_event_t( sim_t*    sim,
                                              player_t* t,
                                              action_t* a,
                                              timespan_t time_to_travel ) :
  event_t( sim, a -> player, "Action Travel" ), action( a ), target( t )
{
  result = a -> result;
  damage = a -> direct_dmg;

  if ( sim -> debug )
    sim -> output( "New Action Travel Event: %s %s %.2f",
                   player -> name(), a -> name(), time_to_travel.total_seconds() );

  sim -> add_event( this, time_to_travel );
}

// action_travel_event_t::execute ===========================================

void action_travel_event_t::execute()
{
  action -> impact( target, result, damage );
  action -> remove_travel_event( this );
}

// ==========================================================================
// Regen Event
// ==========================================================================

// regen_event_t::regen_event_t =============================================

regen_event_t::regen_event_t( sim_t* sim ) : event_t( sim, 0, "Regen Event" )
{
  if ( sim -> debug ) sim -> output( "New Regen Event" );
  sim -> add_event( this, sim -> regen_periodicity );
}

// regen_event_t::execute ===================================================

void regen_event_t::execute()
{
  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    player_t* p = sim -> actor_list[ i ];
    if ( p -> current.sleeping ) continue;
    if ( p -> primary_resource() == RESOURCE_NONE ) continue;

    p -> regen( sim -> regen_periodicity );
  }

  new ( sim ) regen_event_t( sim );
}

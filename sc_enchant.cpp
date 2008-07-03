// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

namespace { // ANONYMOUS NAMESPACE ==========================================

// Spellsurge Enchant =======================================================

static void trigger_spellsurge( spell_t* s )
{
  struct spellsurge_t : public spell_t
  {
    spellsurge_t( player_t* p ) : 
      spell_t( "spellsurge", p, RESOURCE_MANA, SCHOOL_ARCANE )
    {
      base_duration = 10.0;
      num_ticks = 10;
      valid = false;  // This spell can never be called directly.
      trigger_gcd = false;
      background = true;
    }
    virtual void execute() 
    {
      assert( current_tick == 0 );
      schedule_tick();
    }
    virtual void tick()
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> party == player -> party ) 
	{
	  report_t::log( sim, "Player %s gains mana from %s 's Spellsurge.", p -> name(), player -> name() );
	  p -> resource_gain( RESOURCE_MANA, 10.0, "spellsurge" );
	}
      }
    }
  };

  if( s -> player -> gear.spellsurge && 
      s -> player -> expirations.spellsurge <= s -> sim -> current_time &&
      wow_random( 0.15 ) )
  {
    for( player_t* p = s -> sim -> player_list; p; p = p -> next )
    {
      // Invalidate any existing Spellsurge procs.

      if( p -> gear.spellsurge && 
	  p -> party == s -> player -> party ) 
      {
	action_t* spellsurge = p -> find_action( "spellsurge" );

	if( spellsurge && spellsurge -> time_remaining > 0 )
	{
	  spellsurge -> current_tick = 0;
	  spellsurge -> time_remaining = 0;
	  spellsurge -> event -> invalid = true;
	  spellsurge -> event = 0;
	  break;
	}
      }
    }
    
    action_t* spellsurge = s -> player -> find_action( "spellsurge" );
    if( ! spellsurge ) spellsurge = new spellsurge_t( s -> player );
    
    spellsurge -> execute();

    s -> player -> expirations.spellsurge = s -> sim -> current_time + 60.0;
  }
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Enchant
// ==========================================================================

// enchant_t::spell_finish_event ============================================

void enchant_t::spell_finish_event( spell_t* s )
{
  trigger_spellsurge( s );
}


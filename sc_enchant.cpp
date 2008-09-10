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
      num_ticks     = 10;
      trigger_gcd   = 0;
      background    = true;
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
	  if( sim -> log ) report_t::log( sim, "Player %s gains mana from %s 's Spellsurge.", p -> name(), player -> name() );
	  p -> resource_gain( RESOURCE_MANA, 10.0, p -> gains.spellsurge );
	}
      }
    }
  };

  if( s -> player -> gear.spellsurge && 
      s -> player -> expirations.spellsurge <= s -> sim -> current_time &&
      rand_t::roll( 0.15 ) )
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
	  spellsurge -> cancel();
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

// Mongoose Enchant =========================================================

static void trigger_mongoose( attack_t* a )
{

  struct mongoose_expiration_t : public event_t
  {
    int8_t& buffs_mongoose;
    event_t*& expirations_mongoose;

    mongoose_expiration_t( sim_t* sim, player_t* player, int8_t& b_m, event_t*& e_m ) : 
      event_t( sim, player ), buffs_mongoose( b_m ), expirations_mongoose( e_m )
    {
      name = "Mongoose Expiration";
      player -> aura_gain( "Mongoose Lightning Speed" );
      player -> attribute[ ATTR_AGILITY ] += 120;
      buffs_mongoose = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Mongoose Lightning Speed" );
      player -> attribute[ ATTR_AGILITY ] -= 120;
      buffs_mongoose = 0;
      expirations_mongoose = 0;
    }
  };

  player_t* p = a -> player;
  weapon_t* w = a -> weapon;

  // Mongoose has a 1.2 PPM (proc per minute) which translates into 1 proc every 50sec on average
  // We cannot use the base swing time because that would over-value haste.  Instead, we use
  // the hasted swing time which is represented in the "time_to_execute" field.  When this field
  // is zero, we are dealing with a "special" attack in which case the base swing time is used.

  double swing_time = a -> time_to_execute;
  if( a -> time_to_execute == 0 ) swing_time = w -> swing_time;

  double proc_chance = 1.0 / ( 50.0 / swing_time );

  int8_t&    b = w -> main ? p ->       buffs.mongoose_mh : p ->       buffs.mongoose_oh;
  event_t*&  e = w -> main ? p -> expirations.mongoose_mh : p -> expirations.mongoose_oh;
  uptime_t*& u = w -> main ? p ->     uptimes.mongoose_mh : p ->     uptimes.mongoose_oh;

  if( rand_t::roll( proc_chance ) )
  {
    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new mongoose_expiration_t( a -> sim, p, b, e );
    }
  }

  if( ! u )  u = p -> get_uptime( w -> main ? "moongoose_mh" : "mongoose_oh" );

  u -> update( b );
}

// Executioner Enchant =========================================================

static void trigger_executioner( attack_t* a )
{
  struct executioner_expiration_t : public event_t
  {
    executioner_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Executioner Expiration";
      player -> aura_gain( "Executioner" );
      player -> buffs.executioner = 1;
      player -> attack_penetration += 840;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Executioner" );
      player -> buffs.executioner = 0;
      player -> attack_penetration -= 840;
      player -> expirations.executioner = 0;
    }
  };

  player_t* p = a -> player;
  weapon_t* w = a -> weapon;

  // Executioner has a 1.2 PPM (proc per minute) which translates into 1 proc every 50sec on average
  // We cannot use the base swing time because that would over-value haste.  Instead, we use
  // the hasted swing time which is represented in the "time_to_execute" field.  When this field
  // is zero, we are dealing with a "special" attack in which case the base swing time is used.

  double swing_time = a -> time_to_execute;
  if( a -> time_to_execute == 0 ) swing_time = w -> swing_time;

  double proc_chance = 1.0 / ( 50.0 / swing_time );

  if( rand_t::roll( proc_chance ) )
  {
    event_t*& e = p -> expirations.executioner;

    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new executioner_expiration_t( a -> sim, p );
    }
  }

  if( ! p -> uptimes.executioner ) 
  {
    p -> uptimes.executioner = p -> get_uptime( "executioner" );
  }

  p -> uptimes.executioner -> update( p -> buffs.executioner );
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

// enchant_t::attack_hit_event ==============================================

void enchant_t::attack_hit_event( attack_t* a )
{
  if( a -> weapon )
  {
    switch( a -> weapon -> enchant )
    {
    case MONGOOSE:    trigger_mongoose   ( a ); break;
    case EXECUTIONER: trigger_executioner( a ); break;
    }
  }
}

// ==========================================================================
// Totem Enchants
// ==========================================================================

// trigger_flametongue_totem ================================================

void enchant_t::trigger_flametongue_totem( attack_t* a )
{
  struct flametongue_totem_spell_t : public spell_t
  {
    flametongue_totem_spell_t( player_t* player ) :
      spell_t( "flametongue", player, RESOURCE_NONE, SCHOOL_FIRE )
    {
      trigger_gcd = 0;
      proc        = true;
      background  = true;
      may_crit    = true;
      reset();
    }
    virtual void get_base_damage()
    {
      base_dd = player -> buffs.flametongue_totem * weapon -> swing_time;
    }
  };

  if( a -> weapon &&
      a -> weapon -> buff == FLAMETONGUE_TOTEM )
  {
    player_t* p = a -> player;

    if( ! p -> actions.flametongue_totem )
    {
      p -> actions.flametongue_totem = new flametongue_totem_spell_t( p );
    }

    p -> actions.flametongue_totem -> weapon = a -> weapon;
    p -> actions.flametongue_totem -> execute();
  }
}

// trigger_windfury_totem ===================================================

void enchant_t::trigger_windfury_totem( attack_t* a )
{
  if( ! a -> proc &&
        a -> weapon &&
        a -> weapon -> buff == WINDFURY_TOTEM &&
      rand_t::roll( 0.20 ) )
  {
    a -> proc = true;
    player_t* p = a -> player;
    p -> procs.windfury -> occur();
    p -> attack_power += p -> buffs.windfury_totem;
    a -> execute();
    p -> attack_power -= p -> buffs.windfury_totem;
    a -> proc = false;
  }
}


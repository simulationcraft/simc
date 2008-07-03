// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Consumable
// ==========================================================================

// ==========================================================================
// Destruction Potion
// ==========================================================================

struct destruction_potion_t : public spell_t
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Destruction Potion Expiration";
      time = 15.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      player -> aura_loss( "Destruction Potion Buff" );
      player -> spell_power[ SCHOOL_MAX ] -= 120;
      player -> spell_crit -= 0.02;
    }
  };
  
  destruction_potion_t( player_t* p, const std::string& option_str ) : 
    spell_t( "destruction_potion", p )
  {
    cooldown = 120.0;
    cooldown_group = "potion";
    trigger_gcd = false;
    harmful = false;
  }
  
  virtual void execute()
  {
    player -> aura_gain( "Destruction Potion Buff" );
    player -> spell_power[ SCHOOL_MAX ] += 120;
    player -> spell_crit += 0.02;
    player -> share_cooldown( cooldown_group, cooldown );
    new expiration_t( sim, player );
  }
};

// ==========================================================================
// Mana Potion
// ==========================================================================

struct mana_potion_t : public spell_t
{
  double mana;

  mana_potion_t( player_t* p, const std::string& option_str ) : 
    spell_t( "mana_potion", p ), mana(0)
  {
    cooldown = 120.0;
    cooldown_group = "potion";
    trigger_gcd = false;
    harmful = false;
    mana = atof( option_str.c_str() );
  }
  
  virtual void execute()
  {
    report_t::log( sim, "%s uses Mana potion", player -> name() );
    player -> resource_gain( RESOURCE_MANA, mana, "mana_potion" );
    player -> share_cooldown( cooldown_group, cooldown );
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    return( player -> resource_initial[ RESOURCE_MANA ] - 
	    player -> resource_current[ RESOURCE_MANA ] ) > mana;
  }
};

// ==========================================================================
// Mana Gem
// ==========================================================================

struct mana_gem_t : public spell_t
{
  double mana;

  mana_gem_t( player_t* p, const std::string& option_str ) : 
    spell_t( "mana_gem", p ), mana(0)
  {
    cooldown = 120.0;
    cooldown_group = "rune";
    trigger_gcd = false;
    harmful = false;
    mana = atof( option_str.c_str() );
  }
  
  virtual void execute()
  {
    report_t::log( sim, "%s uses Mana Gem", player -> name() );
    player -> resource_gain( RESOURCE_MANA, mana, "mana_gem" );
    player -> share_cooldown( cooldown_group, cooldown );
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    return( player -> resource_initial[ RESOURCE_MANA ] - 
	    player -> resource_current[ RESOURCE_MANA ] ) > mana;
  }
};

// ==========================================================================
// Health Stone
// ==========================================================================

struct health_stone_t : public spell_t
{
  double health;

  health_stone_t( player_t* p, const std::string& option_str ) : 
    spell_t( "health_stone", p ), health(0)
  {
    cooldown = 120.0;
    cooldown_group = "rune";
    trigger_gcd = false;
    harmful = false;
    health = atof( option_str.c_str() );
  }
  
  virtual void execute()
  {
    report_t::log( sim, "%s uses Health Stone", player -> name() );
    player -> resource_gain( RESOURCE_HEALTH, health, "health_stone" );
    player -> share_cooldown( cooldown_group, cooldown );
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    return( player -> resource_initial[ RESOURCE_HEALTH ] - 
	    player -> resource_current[ RESOURCE_HEALTH ] ) > health;
  }
};

// ==========================================================================
// Dark Rune
// ==========================================================================

struct dark_rune_t : public spell_t
{
  double health;
  double mana;

  dark_rune_t( player_t* p, const std::string& options_str ) : 
    spell_t( "dark_rune", p ), health(0), mana(0)
  {
    option_t options[] =
    {
      { "mana",   OPT_FLT,  &mana   },
      { "health", OPT_FLT,  &health },
      { NULL }
    };
    parse_options( options, options_str );

    cooldown = 120.0;
    cooldown_group = "rune";
    trigger_gcd = false;
    harmful = false;
  }
  
  virtual void execute()
  {
    report_t::log( sim, "%s uses Dark Rune", player -> name() );
    player -> resource_gain( RESOURCE_MANA,   mana,   "dark_rune" );
    player -> resource_loss( RESOURCE_HEALTH, health, "dark_rune" );
    player -> share_cooldown( cooldown_group, cooldown );
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    if( player -> resource_current[ RESOURCE_HEALTH ] <= health )
      return false;

    return( player -> resource_initial[ RESOURCE_MANA ] - 
	    player -> resource_current[ RESOURCE_MANA ] ) > mana;
  }
};

// ==========================================================================
// consumable_t::create_action
// ==========================================================================

action_t* consumable_t::create_action( player_t*          p,
				       const std::string& name, 
				       const std::string& option_str )
{
  if( name == "dark_rune"          ) return new          dark_rune_t( p, option_str );
  if( name == "destruction_potion" ) return new destruction_potion_t( p, option_str );
  if( name == "health_stone"       ) return new       health_stone_t( p, option_str );
  if( name == "mana_potion"        ) return new        mana_potion_t( p, option_str );
  if( name == "mana_gem"           ) return new           mana_gem_t( p, option_str );

  return 0;
}

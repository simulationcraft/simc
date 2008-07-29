// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Consumable
// ==========================================================================

// ==========================================================================
// Flask
// ==========================================================================

struct flask_t : public action_t
{
  int8_t type;

  flask_t( player_t* p, const std::string& option_str ) : 
    action_t( ACTION_OTHER, "flask", p ), type( FLASK_NONE )
  {
    trigger_gcd = 0;
    harmful = false;
    for( int i=0; i < FLASK_MAX; i++ )
    {
      if( option_str == util_t::flask_type_string( i ) )
      {
	type = i;
	break;
      }
    }
    assert( type != FLASK_NONE );
  }
  
  virtual void execute()
  {
    report_t::log( sim, "%s uses Flask %s", player -> name(), util_t::flask_type_string( type ) );
    player -> flask = type;
    switch( type )
    {
    case FLASK_BLINDING_LIGHT:     
      player -> spell_power[ SCHOOL_ARCANE ] += 80;
      player -> spell_power[ SCHOOL_HOLY   ] += 80;
      player -> spell_power[ SCHOOL_NATURE ] += 80;
      break;
    case FLASK_DISTILLED_WISDOM:
      player -> attribute[ ATTR_INTELLECT ] += 65;
      break;
    case FLASK_MIGHTY_RESTORATION:
      player -> mp5 += 25;
      break;
    case FLASK_PURE_DEATH:
      player -> spell_power[ SCHOOL_FIRE   ] += 80;
      player -> spell_power[ SCHOOL_FROST  ] += 80;
      player -> spell_power[ SCHOOL_SHADOW ] += 80;
      break;
    case FLASK_RELENTLESS_ASSAULT:
      player -> attack_power += 120;
      break;
    case FLASK_SUPREME_POWER:
      player -> spell_power[ SCHOOL_MAX ] += 70;
      break;
    default: assert(0);
    }
  }

  virtual bool ready()
  {
    return( player -> flask           ==  FLASK_NONE &&
	    player -> elixir_guardian == ELIXIR_NONE &&
	    player -> elixir_battle   == ELIXIR_NONE );
  }
};

// ==========================================================================
// Destruction Potion
// ==========================================================================

struct destruction_potion_t : public action_t
{
  bool used;

  destruction_potion_t( player_t* p, const std::string& option_str ) : 
    action_t( ACTION_OTHER, "destruction_potion", p ), used( false )
  {
    cooldown = 120.0;
    cooldown_group = "potion";
    trigger_gcd = 0;
    harmful = false;
  }
  
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
	name = "Destruction Potion Expiration";
	player -> aura_gain( "Destruction Potion Buff" );
	player -> spell_power[ SCHOOL_MAX ] += 120;
	player -> spell_crit += 0.02;
	sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
	player -> aura_loss( "Destruction Potion Buff" );
	player -> spell_power[ SCHOOL_MAX ] -= 120;
	player -> spell_crit -= 0.02;
      }
    };
  
    player -> share_cooldown( cooldown_group, cooldown );
    new expiration_t( sim, player );
    used = true;
  }

  virtual bool ready()
  {
    if( sim_t::WotLK && used )
      return false;

    return( cooldown_ready > sim -> current_time );
  }

  virtual void reset()
  {
    action_t::reset();
    used = false;
  }
};

// ==========================================================================
// Mana Potion
// ==========================================================================

struct mana_potion_t : public action_t
{
  double mana;
  bool used;

  mana_potion_t( player_t* p, const std::string& option_str ) : 
    action_t( ACTION_OTHER, "mana_potion", p ), mana(0), used(false)
  {
    cooldown = 120.0;
    cooldown_group = "potion";
    trigger_gcd = 0;
    harmful = false;
    mana = atof( option_str.c_str() );
  }
  
  virtual void execute()
  {
    report_t::log( sim, "%s uses Mana potion", player -> name() );
    player -> resource_gain( RESOURCE_MANA, mana, "mana_potion" );
    player -> share_cooldown( cooldown_group, cooldown );
    used = true;
  }

  virtual bool ready()
  {
    if( sim_t::WotLK && used )
      return false;

    if( cooldown_ready > sim -> current_time ) 
      return false;

    return( player -> resource_max    [ RESOURCE_MANA ] - 
	    player -> resource_current[ RESOURCE_MANA ] ) > mana;
  }

  virtual void reset()
  {
    action_t::reset();
    used = false;
  }
};

// ==========================================================================
// Mana Gem
// ==========================================================================

struct mana_gem_t : public action_t
{
  double mana;

  mana_gem_t( player_t* p, const std::string& option_str ) : 
    action_t( ACTION_OTHER, "mana_gem", p ), mana(0)
  {
    cooldown = 120.0;
    cooldown_group = "rune";
    trigger_gcd = 0;
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
    if( cooldown_ready > sim -> current_time ) 
      return false;

    return( player -> resource_max    [ RESOURCE_MANA ] - 
	    player -> resource_current[ RESOURCE_MANA ] ) > mana;
  }
};

// ==========================================================================
// Health Stone
// ==========================================================================

struct health_stone_t : public action_t
{
  double health;

  health_stone_t( player_t* p, const std::string& option_str ) : 
    action_t( ACTION_OTHER, "health_stone", p ), health(0)
  {
    cooldown = 120.0;
    cooldown_group = "rune";
    trigger_gcd = 0;
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
    if( cooldown_ready > sim -> current_time ) 
      return false;

    return( player -> resource_max    [ RESOURCE_HEALTH ] - 
	    player -> resource_current[ RESOURCE_HEALTH ] ) > health;
  }
};

// ==========================================================================
// Dark Rune
// ==========================================================================

struct dark_rune_t : public action_t
{
  double health;
  double mana;

  dark_rune_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "dark_rune", p ), health(0), mana(0)
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
    trigger_gcd = 0;
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
    if( cooldown_ready > sim -> current_time ) 
      return false;

    if( player -> resource_current[ RESOURCE_HEALTH ] <= health )
      return false;

    return( player -> resource_max    [ RESOURCE_MANA ] - 
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
  if( name == "flask"              ) return new              flask_t( p, option_str );
  if( name == "health_stone"       ) return new       health_stone_t( p, option_str );
  if( name == "mana_potion"        ) return new        mana_potion_t( p, option_str );
  if( name == "mana_gem"           ) return new           mana_gem_t( p, option_str );

  return 0;
}

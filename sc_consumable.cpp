// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Consumable
// ==========================================================================

// ==========================================================================
// Flask
// ==========================================================================

struct flask_t : public action_t
{
  int type;

  flask_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "flask", p ), type( FLASK_NONE )
  {
    std::string type_str;

    option_t options[] =
    {
      { "type", OPT_STRING, &type_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    harmful = false;
    for ( int i=0; i < FLASK_MAX; i++ )
    {
      if ( type_str == util_t::flask_type_string( i ) )
      {
        type = i;
        break;
      }
    }
    assert( type != FLASK_NONE );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s uses Flask %s", player -> name(), util_t::flask_type_string( type ) );
    player -> flask = type;
    switch ( type )
    {
    case FLASK_BLINDING_LIGHT:
      player -> spell_power[ SCHOOL_ARCANE ] += 80;
      player -> spell_power[ SCHOOL_HOLY   ] += 80;
      player -> spell_power[ SCHOOL_NATURE ] += 80;
      break;
    case FLASK_DISTILLED_WISDOM:
      player -> stat_gain( STAT_INTELLECT, 65 );
      break;
    case FLASK_ENDLESS_RAGE:
      player -> stat_gain( STAT_ATTACK_POWER, 180 );
      break;
    case FLASK_FROST_WYRM:
      player -> stat_gain( STAT_SPELL_POWER, 125 );
      break;
    case FLASK_MIGHTY_RESTORATION:
      player -> stat_gain( STAT_MP5, 25 );
      break;
    case FLASK_PURE_DEATH:
      player -> spell_power[ SCHOOL_FIRE   ] += 80;
      player -> spell_power[ SCHOOL_FROST  ] += 80;
      player -> spell_power[ SCHOOL_SHADOW ] += 80;
      break;
    case FLASK_RELENTLESS_ASSAULT:
      player -> stat_gain( STAT_ATTACK_POWER, 120 );
      break;
    case FLASK_SUPREME_POWER:
      player -> stat_gain( STAT_SPELL_POWER, 70 );
      break;
    default: assert( 0 );
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
// Flask
// ==========================================================================

struct food_t : public action_t
{
  int type;

  food_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "food", p ), type( FOOD_NONE )
  {
    std::string type_str;

    option_t options[] =
    {
      { "type", OPT_STRING, &type_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    harmful = false;
    for ( int i=0; i < FOOD_MAX; i++ )
    {
      if ( type_str == util_t::food_type_string( i ) )
      {
        type = i;
        break;
      }
    }
    assert( type != FOOD_NONE );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s uses Food %s", player -> name(), util_t::food_type_string( type ) );
    player -> food = type;
    switch ( type )
    {
    case FOOD_TENDER_SHOVELTUSK_STEAK:
      player -> stat_gain( STAT_SPELL_POWER, 46 );
      player -> stat_gain( STAT_STAMINA, 40 );
      break;
    case FOOD_SNAPPER_EXTREME:
      player -> stat_gain( STAT_HIT_RATING, 40 );
      player -> stat_gain( STAT_STAMINA, 40 );
      break;
    case FOOD_SMOKED_SALMON:
      player -> stat_gain( STAT_SPELL_POWER, 35 );
      player -> stat_gain( STAT_STAMINA, 40 );
      break;
    case FOOD_POACHED_BLUEFISH:
      player -> stat_gain( STAT_SPELL_POWER, 23 );
      player -> stat_gain( STAT_SPIRIT, 20 );
      break;
    case FOOD_BLACKENED_BASILISK:
      player -> stat_gain( STAT_SPELL_POWER, 23 );
      player -> stat_gain( STAT_SPIRIT, 20 );
      break;
    case FOOD_GOLDEN_FISHSTICKS:
      player -> stat_gain( STAT_SPELL_POWER, 23 );
      player -> stat_gain( STAT_SPIRIT, 20 );
      break;
    case FOOD_CRUNCHY_SERPENT:
      player -> stat_gain( STAT_SPELL_POWER, 23 );
      player -> stat_gain( STAT_SPIRIT, 20 );
      break;
    case FOOD_BLACKENED_DRAGONFIN:
      player -> stat_gain( STAT_AGILITY, 40 );
      player -> stat_gain( STAT_STAMINA, 40 );
      break;
    case FOOD_DRAGONFIN_FILET:
      player -> stat_gain( STAT_STRENGTH, 40 );
      player -> stat_gain( STAT_STAMINA, 40 );
      break;
    case FOOD_HEARTY_RHINO:
      player -> stat_gain( STAT_STRENGTH, 40 );
      player -> stat_gain( STAT_ARMOR_PENETRATION_RATING, 40 );
      player -> stat_gain( STAT_STAMINA, 40 );
      break;
    case FOOD_GREAT_FEAST:
      player -> stat_gain( STAT_ATTACK_POWER, 60 );
      player -> stat_gain( STAT_SPELL_POWER,  35 );
      player -> stat_gain( STAT_STAMINA, 30 );
      break;
    case FOOD_FISH_FEAST:
      player -> stat_gain( STAT_ATTACK_POWER, 80 );
      player -> stat_gain( STAT_SPELL_POWER,  46 );
      player -> stat_gain( STAT_STAMINA, 40 );
      break;
    default: assert( 0 );
    }
  }

  virtual bool ready()
  {
    return( player -> food        ==  FOOD_NONE );
  }
};

// ==========================================================================
// Destruction Potion
// ==========================================================================

struct destruction_potion_t : public action_t
{
  destruction_potion_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "destruction_potion", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

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
        p -> aura_gain( "Destruction Potion Buff" );
        p -> stat_gain( STAT_SPELL_POWER, 120 );
        p -> spell_crit += 0.02;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        player_t* p = player;
        p -> aura_loss( "Destruction Potion Buff" );
        p -> stat_loss( STAT_SPELL_POWER, 120 );
        p -> spell_crit -= 0.02;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s uses %s", player -> name(), name() );
    new ( sim ) expiration_t( sim, player );
    player -> potion_used = 1;
  }

  virtual bool ready()
  {
    if ( player -> potion_used )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// Speed Potion
// ==========================================================================

struct speed_potion_t : public action_t
{
  speed_potion_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "speed_potion", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        name = "Speed Potion Expiration";
        p -> aura_gain( "Speed Potion Buff" );
	p -> stat_gain( STAT_HASTE_RATING, 500 );
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        player_t* p = player;
        p -> aura_loss( "Speed Potion Buff" );
	p -> stat_loss( STAT_HASTE_RATING, 500 );
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s uses %s", player -> name(), name() );
    new ( sim ) expiration_t( sim, player );
    player -> potion_used = 1;
  }

  virtual bool ready()
  {
    if ( player -> potion_used )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// Wild Magic Potion
// ==========================================================================

struct wild_magic_potion_t : public action_t
{
  wild_magic_potion_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "wild_magic_potion", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        name = "Wild Magic Potion Expiration";
        p -> aura_gain( "Wild Magic Potion Buff" );
        p -> stat_gain( STAT_CRIT_RATING, 200 );
        p -> stat_gain( STAT_SPELL_POWER, 200 );
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        player_t* p = player;
        p -> aura_loss( "Wild Magic Potion Buff" );
        p -> stat_loss( STAT_CRIT_RATING, 200 );
        p -> stat_loss( STAT_SPELL_POWER, 200 );
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s uses %s", player -> name(), name() );
    new ( sim ) expiration_t( sim, player );
    player -> potion_used = 1;
  }

  virtual bool ready()
  {
    if ( player -> potion_used )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// Mana Potion
// ==========================================================================

struct mana_potion_t : public action_t
{
  int trigger;
  int min;
  int max;

  mana_potion_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "mana_potion", p ), trigger( 0 ), min( 0 ), max( 0 )
  {
    option_t options[] =
    {
      { "min",     OPT_INT, &min     },
      { "max",     OPT_INT, &max     },
      { "trigger", OPT_INT, &trigger },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( min == 0 && max == 0 )
    {
      min = max = util_t::ability_rank( player -> level,  4300,80,  2400,68,  1800,0 );
    }

    if ( min > max ) std::swap( min, max );

    if ( max == 0 ) max = trigger;
    if ( trigger == 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s uses Mana potion", player -> name() );
    double gain = sim -> rng -> range( min, max );
    player -> resource_gain( RESOURCE_MANA, gain, player -> gains.mana_potion );
    player -> potion_used = 1;
  }

  virtual bool ready()
  {
    if ( player -> potion_used )
      return false;

    if ( ( player -> resource_max    [ RESOURCE_MANA ] -
           player -> resource_current[ RESOURCE_MANA ] ) < trigger )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// Health Stone
// ==========================================================================

struct health_stone_t : public action_t
{
  int trigger;
  int health;

  health_stone_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "health_stone", p ), trigger( 0 ), health( 0 )
  {
    option_t options[] =
    {
      { "health",  OPT_INT, &health  },
      { "trigger", OPT_INT, &trigger },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( health  == 0 ) health = trigger;
    if ( trigger == 0 ) trigger = health;
    assert( health > 0 && trigger > 0 );

    cooldown = 15 * 60;
    cooldown_group = "rune";
    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s uses Health Stone", player -> name() );
    player -> resource_gain( RESOURCE_HEALTH, health );
  }

  virtual bool ready()
  {
    if ( ( player -> resource_max    [ RESOURCE_HEALTH ] -
           player -> resource_current[ RESOURCE_HEALTH ] ) < trigger )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// Dark Rune
// ==========================================================================

struct dark_rune_t : public action_t
{
  int trigger;
  int health;
  int mana;

  dark_rune_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "dark_rune", p ), trigger( 0 ), health( 0 ), mana( 0 )
  {
    option_t options[] =
    {
      { "trigger", OPT_INT,  &trigger },
      { "mana",    OPT_INT,  &mana    },
      { "health",  OPT_INT,  &health  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( mana    == 0 ) mana = trigger;
    if ( trigger == 0 ) trigger = mana;
    assert( mana > 0 && trigger > 0 );

    cooldown = 15 * 60;
    cooldown_group = "rune";
    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s uses Dark Rune", player -> name() );
    player -> resource_gain( RESOURCE_MANA,   mana, player -> gains.dark_rune );
    player -> resource_loss( RESOURCE_HEALTH, health );
  }

  virtual bool ready()
  {
    if ( player -> resource_current[ RESOURCE_HEALTH ] <= health )
      return false;

    if ( ( player -> resource_max    [ RESOURCE_MANA ] -
           player -> resource_current[ RESOURCE_MANA ] ) < trigger )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// consumable_t::init_flask
// ==========================================================================

void consumable_t::init_flask( player_t* p )
{
  // Eventually, flask will be taken off the actions= directive.
}

// ==========================================================================
// consumable_t::init_elixirs
// ==========================================================================

void consumable_t::init_elixirs( player_t* p )
{
  // Eventually, elixirs will be taken off the actions= directive.
}

// ==========================================================================
// consumable_t::init_food
// ==========================================================================

void consumable_t::init_food( player_t* p )
{
  // Eventually, food will be taken off the actions= directive.
}

// ==========================================================================
// consumable_t::create_action
// ==========================================================================

action_t* consumable_t::create_action( player_t*          p,
                                       const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "dark_rune"          ) return new          dark_rune_t( p, options_str );
  if ( name == "destruction_potion" ) return new destruction_potion_t( p, options_str );
  if ( name == "flask"              ) return new              flask_t( p, options_str );
  if ( name == "food"               ) return new               food_t( p, options_str );
  if ( name == "health_stone"       ) return new       health_stone_t( p, options_str );
  if ( name == "mana_potion"        ) return new        mana_potion_t( p, options_str );
  if ( name == "speed_potion"       ) return new       speed_potion_t( p, options_str );
  if ( name == "wild_magic_potion"  ) return new  wild_magic_potion_t( p, options_str );

  return 0;
}

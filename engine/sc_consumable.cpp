// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Consumable
// ==========================================================================

// ==========================================================================
// Flask
// ==========================================================================
namespace ft {
struct flask_data_t
{
  flask_type_e ft;
  stat_type_e st;
  double stat_amount;
  double mixology_stat_amount;
};

static const flask_data_t flask_data[] =
{
  // cataclysm
  { FLASK_DRACONIC_MIND,    STAT_INTELLECT,  300,  380 },
  { FLASK_FLOWING_WATER,    STAT_SPIRIT,     300,  380 },
  { FLASK_STEELSKIN,        STAT_STAMINA,    300,  380 },
  { FLASK_TITANIC_STRENGTH, STAT_STRENGTH,   300,  380 },
  { FLASK_WINDS,            STAT_AGILITY,    300,  380 },
  // mop
  // FIXME: add correct mixology values
  { FLASK_WARM_SUN,         STAT_INTELLECT, 1000, 1000 },
  { FLASK_FALLING_LEAVES,   STAT_SPIRIT,    1000, 1000 },
  { FLASK_EARTH,            STAT_STAMINA,   1500, 1500 },
  { FLASK_WINTERS_BITE,     STAT_STRENGTH,  1000, 1000 },
  { FLASK_SPRING_BLOSSOMS,  STAT_AGILITY,   1000, 1000 }
};
}

struct flask_t : public action_t
{
  gain_t* gain;
  flask_type_e type;

  flask_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "flask", p ),
    gain( p -> get_gain( "flask" ) )
  {
    std::string type_str;

    option_t options[] =
    {
      { "type", OPT_STRING, &type_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    type = util_t::parse_flask_type( type_str );
    if ( type == FLASK_NONE )
    {
      sim -> errorf( "Player %s attempting to use flask of type '%s', which is not supported.\n",
                     player -> name(), type_str.c_str() );
      sim -> cancel();
    }
  }

  virtual void execute()
  {
    player_t* p = player;
    if ( sim -> log ) log_t::output( sim, "%s uses Flask %s", p -> name(), util_t::flask_type_string( type ) );
    p -> flask = type;

    for ( size_t i = 0; i < sizeof_array( ft::flask_data ); ++i )
    {
      ft::flask_data_t d = ft::flask_data[ i ];
      if ( type == d.ft )
      {
        double amount = ( p -> profession[ PROF_ALCHEMY ] > 50 ) ? d.mixology_stat_amount : d.stat_amount;
        p -> stat_gain( d.st, amount, gain, this );

        if ( d.st == STAT_STAMINA )
        {
          // Cap Health for stamina flasks if they are used outside of combat
          if ( ! player -> in_combat )
          {
            if ( amount > 0 )
              player -> resource_gain( RESOURCE_HEALTH, player -> resources.max[ RESOURCE_HEALTH ] - player -> resources.current[ RESOURCE_HEALTH ] );
          }
        }
      }
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
// Food
// ==========================================================================

struct food_t : public action_t
{
  gain_t* gain;
  food_type_e type;

  food_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "food", p ), gain( p -> get_gain( "food" ) )
  {
    std::string type_str;

    option_t options[] =
    {
      { "type", OPT_STRING, &type_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    type = util_t::parse_food_type( type_str );
    if ( type == FOOD_NONE )
    {
      sim -> errorf( "Invalid food type '%s'\n", type_str.c_str() );
      sim -> cancel();
    }
  }

  virtual void execute()
  {
    player_t* p = player;
    if ( sim -> log ) log_t::output( sim, "%s uses Food %s", p -> name(), util_t::food_type_string( type ) );
    p -> food = type;
    double intellect = 0, stamina = 0;

    double food_stat_multiplier = 1.0;
    if ( p -> race == RACE_PANDAREN )
      food_stat_multiplier = 2.0;

    switch ( type )
    {
    case FOOD_BAKED_ROCKFISH:
      p -> stat_gain( STAT_CRIT_RATING, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_BASILISK_LIVERDOG:
      p -> stat_gain( STAT_HASTE_RATING, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_BEER_BASTED_CROCOLISK:
      p -> stat_gain( STAT_STRENGTH, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_BLACKBELLY_SUSHI:
      p -> stat_gain( STAT_PARRY_RATING, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_BLACKENED_BASILISK:
      p -> stat_gain( STAT_SPELL_POWER, 23 * food_stat_multiplier );
      p -> stat_gain( STAT_SPIRIT, 20 * food_stat_multiplier );
      break;
    case FOOD_BLACKENED_DRAGONFIN:
      p -> stat_gain( STAT_AGILITY, 40 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_CROCOLISK_AU_GRATIN:
      p -> stat_gain( STAT_EXPERTISE_RATING, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_CRUNCHY_SERPENT:
      p -> stat_gain( STAT_SPELL_POWER, 23 * food_stat_multiplier );
      p -> stat_gain( STAT_SPIRIT, 20 * food_stat_multiplier );
      break;
    case FOOD_DELICIOUS_SAGEFISH_TAIL:
      p -> stat_gain( STAT_SPIRIT, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_DRAGONFIN_FILET:
      p -> stat_gain( STAT_STRENGTH, 40 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_FISH_FEAST:
      p -> stat_gain( STAT_ATTACK_POWER, 80 * food_stat_multiplier );
      p -> stat_gain( STAT_SPELL_POWER,  46 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_FORTUNE_COOKIE:
      if ( p -> stats.dodge_rating > 0 )
      {
        p -> stat_gain( STAT_DODGE_RATING, 90 * food_stat_multiplier );
      }
      else if ( p -> stats.attribute[ ATTR_STRENGTH ] >= p -> stats.attribute[ ATTR_INTELLECT ] )
      {
        if ( p -> stats.attribute[ ATTR_STRENGTH ] >= p -> stats.attribute[ ATTR_AGILITY ] )
        {
          p -> stat_gain( STAT_STRENGTH, 90 * food_stat_multiplier );
        }
        else
        {
          p -> stat_gain( STAT_AGILITY, 90 * food_stat_multiplier );
        }
      }
      else if ( p -> stats.attribute[ ATTR_INTELLECT ] >= p -> stats.attribute[ ATTR_AGILITY ] )
      {
        intellect = 90 * food_stat_multiplier; p -> stat_gain( STAT_INTELLECT, intellect, gain, this );
      }
      else
      {
        p -> stat_gain( STAT_AGILITY, 90 * food_stat_multiplier );
      }
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_GOLDEN_FISHSTICKS:
      p -> stat_gain( STAT_SPELL_POWER, 23 * food_stat_multiplier );
      p -> stat_gain( STAT_SPIRIT, 20 * food_stat_multiplier );
      break;
    case FOOD_GREAT_FEAST:
      p -> stat_gain( STAT_ATTACK_POWER, 60 * food_stat_multiplier );
      p -> stat_gain( STAT_SPELL_POWER,  35 * food_stat_multiplier );
      stamina = 30 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_GRILLED_DRAGON:
      p -> stat_gain( STAT_HIT_RATING, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_HEARTY_RHINO:
      p -> stat_gain( STAT_CRIT_RATING, 40 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_IMPERIAL_MANTA_STEAK:
    case FOOD_VERY_BURNT_WORG:
      p -> stat_gain( STAT_HASTE_RATING, 40 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_LAVASCALE_FILLET:
      p -> stat_gain( STAT_MASTERY_RATING, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_MEGA_MAMMOTH_MEAL:
    case FOOD_POACHED_NORTHERN_SCULPIN:
      p -> stat_gain( STAT_ATTACK_POWER, 80 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_MUSHROOM_SAUCE_MUDFISH:
      p -> stat_gain( STAT_DODGE_RATING, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_POACHED_BLUEFISH:
      p -> stat_gain( STAT_SPELL_POWER, 23 * food_stat_multiplier );
      p -> stat_gain( STAT_SPIRIT, 20 * food_stat_multiplier );
      break;
    case FOOD_RHINOLICIOUS_WORMSTEAK:
      p -> stat_gain( STAT_EXPERTISE_RATING, 40 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_SEAFOOD_MAGNIFIQUE_FEAST:
      if ( p -> stats.dodge_rating > 0 )
      {
        p -> stat_gain( STAT_DODGE_RATING, 90 * food_stat_multiplier );
      }
      else if ( p -> stats.attribute[ ATTR_STRENGTH ] >= p -> stats.attribute[ ATTR_INTELLECT ] )
      {
        if ( p -> stats.attribute[ ATTR_STRENGTH ] >= p -> stats.attribute[ ATTR_AGILITY ] )
        {
          p -> stat_gain( STAT_STRENGTH, 90 * food_stat_multiplier );
        }
        else
        {
          p -> stat_gain( STAT_AGILITY, 90 * food_stat_multiplier );
        }
      }
      else if ( p -> stats.attribute[ ATTR_INTELLECT ] >= p -> stats.attribute[ ATTR_AGILITY ] )
      {
        intellect = 90 * food_stat_multiplier; p -> stat_gain( STAT_INTELLECT, intellect, gain, this );
      }
      else
      {
        p -> stat_gain( STAT_AGILITY, 90 * food_stat_multiplier );
      }
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_SEVERED_SAGEFISH_HEAD:
      intellect = 90 * food_stat_multiplier; p -> stat_gain( STAT_INTELLECT, intellect, gain, this );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_SKEWERED_EEL:
      p -> stat_gain( STAT_AGILITY, 90 * food_stat_multiplier );
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_SMOKED_SALMON:
      p -> stat_gain( STAT_SPELL_POWER, 35 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_SNAPPER_EXTREME:
      p -> stat_gain( STAT_HIT_RATING, 40 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_TENDER_SHOVELTUSK_STEAK:
      p -> stat_gain( STAT_SPELL_POWER, 46 * food_stat_multiplier );
      stamina = 40 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    default: assert( 0 ); break;
    }

    // Cap Health / Mana for food if they are used outside of combat
    if ( ! player -> in_combat )
    {
      if ( intellect > 0 )
        player -> resource_gain( RESOURCE_MANA, player -> resources.max[ RESOURCE_MANA ] - player -> resources.current[ RESOURCE_MANA ], gain, this );

      if ( stamina > 0 )
        player -> resource_gain( RESOURCE_HEALTH, player -> resources.max[ RESOURCE_HEALTH ] - player -> resources.current[ RESOURCE_HEALTH ] );
    }
  }

  virtual bool ready()
  {
    return( player -> food == FOOD_NONE );
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
      min = max = util_t::ability_rank( player -> level,  10000,85, 4300,80,  2400,68,  1800,0 );
    }

    if ( min > max ) std::swap( min, max );

    if ( max == 0 ) max = trigger;
    if ( trigger == 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    trigger_gcd = timespan_t::zero();
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

    if ( ( player -> resources.max    [ RESOURCE_MANA ] -
           player -> resources.current[ RESOURCE_MANA ] ) < trigger )
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

    cooldown = p -> get_cooldown( "rune" );
    cooldown -> duration = timespan_t::from_minutes( 15 );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s uses Health Stone", player -> name() );
    player -> resource_gain( RESOURCE_HEALTH, health );
    update_ready();
  }

  virtual bool ready()
  {
    if ( ( player -> resources.max    [ RESOURCE_HEALTH ] -
           player -> resources.current[ RESOURCE_HEALTH ] ) < trigger )
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

    cooldown = p -> get_cooldown( "rune" );
    cooldown -> duration = timespan_t::from_minutes( 15 );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s uses Dark Rune", player -> name() );
    player -> resource_gain( RESOURCE_MANA,   mana, player -> gains.dark_rune );
    player -> resource_loss( RESOURCE_HEALTH, health );
    update_ready();
  }

  virtual bool ready()
  {
    if ( player -> resources.current[ RESOURCE_HEALTH ] <= health )
      return false;

    if ( ( player -> resources.max    [ RESOURCE_MANA ] -
           player -> resources.current[ RESOURCE_MANA ] ) < trigger )
      return false;

    return action_t::ready();
  }
};
// ==========================================================================
//  Potion Base
// ==========================================================================

struct potion_base_t : public action_t
{
  buff_t* potion_buff;
  const timespan_t cd;

  potion_base_t( player_t* p, const std::string& n, buff_t* pb, const std::string& options_str ) :
    action_t( ACTION_USE, n, p ),
    potion_buff( pb ),
    cd( timespan_t::from_seconds( 60.0 ) )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    cooldown = p -> get_cooldown( "potion" );
    cooldown -> duration = cd;
  }

  virtual void execute()
  {
    if ( player -> in_combat )
    {
      potion_buff -> trigger();
    }
    else
    {
      cooldown -> duration -= timespan_t::from_seconds( 5.0 );
      potion_buff -> trigger( 1, -1.0, potion_buff -> default_chance, ( potion_buff->buff_duration - timespan_t::from_seconds( 5.0 ) ) );
    }

    if ( sim -> log ) log_t::output( sim, "%s uses %s", player -> name(), name() );
    if ( player -> in_combat ) player -> potion_used = 1;
    update_ready();
    cooldown -> duration = cd;
  }

  virtual bool ready()
  {
    if ( ! player -> in_combat && player -> use_pre_potion <= 0 )
      return false;

    if ( player -> potion_used )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// consumable_t::create_action
// ==========================================================================

action_t* consumable_t::create_action( player_t*          p,
                                       const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "dark_rune"             ) return new             dark_rune_t( p, options_str );
  if ( name == "flask"                 ) return new                 flask_t( p, options_str );
  if ( name == "food"                  ) return new                  food_t( p, options_str );
  if ( name == "health_stone"          ) return new          health_stone_t( p, options_str );
  if ( name == "mana_potion"           ) return new           mana_potion_t( p, options_str );
  if ( name == "mythical_mana_potion"  ) return new           mana_potion_t( p, options_str );
  if ( name == "speed_potion"          ) return new  potion_base_t( p, name, p -> potion_buffs.speed, options_str );
  if ( name == "volcanic_potion"       ) return new  potion_base_t( p, name, p -> potion_buffs.volcanic, options_str );
  if ( name == "earthen_potion"        ) return new  potion_base_t( p, name, p -> potion_buffs.earthen, options_str );
  if ( name == "golemblood_potion"     ) return new  potion_base_t( p, name, p -> potion_buffs.golemblood, options_str );
  if ( name == "tolvir_potion"         ) return new  potion_base_t( p, name, p -> potion_buffs.tolvir, options_str );
  // new mop potions
  if ( name == "jinyu_potion"          ) return new  potion_base_t( p, name, p -> potion_buffs.jinyu, options_str );
  if ( name == "mountains_potion"      ) return new  potion_base_t( p, name, p -> potion_buffs.mountains, options_str );
  if ( name == "mogu_power_potion"     ) return new  potion_base_t( p, name, p -> potion_buffs.mogu_power, options_str );
  if ( name == "virmens_bite_potion"   ) return new  potion_base_t( p, name, p -> potion_buffs.virmens_bite, options_str );

  return 0;
}

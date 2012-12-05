// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct flask_data_t
{
  flask_e ft;
  stat_e st;
  int stat_amount;
  int mixology_stat_amount;
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
  { FLASK_WARM_SUN,         STAT_INTELLECT, 1000, 1320 },
  { FLASK_FALLING_LEAVES,   STAT_SPIRIT,    1000, 1480 },
  { FLASK_EARTH,            STAT_STAMINA,   1500, 1980 },
  { FLASK_WINTERS_BITE,     STAT_STRENGTH,  1000, 1320 },
  { FLASK_SPRING_BLOSSOMS,  STAT_AGILITY,   1000, 1320 }
};

struct food_data_t
{
  food_e ft;
  stat_e st;
  int stat_amount;
};

static const food_data_t food_data[] =
{
  // cataclysm
  { FOOD_BAKED_ROCKFISH,               STAT_CRIT_RATING,       90 },
  { FOOD_BAKED_ROCKFISH,               STAT_STAMINA,           90 },

  { FOOD_BASILISK_LIVERDOG,            STAT_HASTE_RATING,      90 },
  { FOOD_BASILISK_LIVERDOG,            STAT_STAMINA,           90 },

  { FOOD_BEER_BASTED_CROCOLISK,        STAT_STRENGTH,          90 },
  { FOOD_BEER_BASTED_CROCOLISK,        STAT_STAMINA,           90 },

  { FOOD_BLACK_PEPPER_RIBS_AND_SHRIMP, STAT_STRENGTH,         300 },

  { FOOD_BLACKBELLY_SUSHI,             STAT_PARRY_RATING,      90 },
  { FOOD_BLACKBELLY_SUSHI,             STAT_STAMINA,           90 },

  { FOOD_BLANCHED_NEEDLE_MUSHROOMS,    STAT_DODGE_RATING,     200 },

  { FOOD_BOILED_SILKWORM_PUPA,         STAT_HIT_RATING,       100 },

  { FOOD_BRAISED_TURTLE,               STAT_INTELLECT,        275 },

  { FOOD_CHARBROILED_TIGER_STEAK,      STAT_STRENGTH,         250 },

  { FOOD_CHUN_TIAN_SPRING_ROLLS,       STAT_STAMINA,          450 },

  { FOOD_CROCOLISK_AU_GRATIN,          STAT_EXPERTISE_RATING,  90 },
  { FOOD_CROCOLISK_AU_GRATIN,          STAT_STAMINA,           90 },

  { FOOD_DELICIOUS_SAGEFISH_TAIL,      STAT_SPIRIT,            90 },
  { FOOD_DELICIOUS_SAGEFISH_TAIL,      STAT_STAMINA,           90 },

  { FOOD_DRIED_NEEDLE_MUSHROOMS,       STAT_DODGE_RATING,     100 },

  { FOOD_DRIED_PEACHES,                STAT_PARRY_RATING,     100 },

  { FOOD_ETERNAL_BLOSSOM_FISH,         STAT_STRENGTH,         275 },

  { FOOD_FIRE_SPIRIT_SALMON,           STAT_SPIRIT,           275 },

  { FOOD_FISH_FEAST,                   STAT_ATTACK_POWER,      80 },
  { FOOD_FISH_FEAST,                   STAT_SPELL_POWER,       46 },
  { FOOD_FISH_FEAST,                   STAT_STAMINA,           40 },

  { FOOD_GREEN_CURRY_FISH,             STAT_CRIT_RATING,      200 },

  { FOOD_GRILLED_DRAGON,               STAT_HIT_RATING,        90 },
  { FOOD_GRILLED_DRAGON,               STAT_STAMINA,           90 },

  { FOOD_LAVASCALE_FILLET,             STAT_MASTERY_RATING,    90 },
  { FOOD_LAVASCALE_FILLET,             STAT_STAMINA,           90 },

  { FOOD_MOGU_FISH_STEW,               STAT_INTELLECT,        300 },

  { FOOD_MUSHROOM_SAUCE_MUDFISH,       STAT_DODGE_RATING,      90 },
  { FOOD_MUSHROOM_SAUCE_MUDFISH,       STAT_STAMINA,           90 },

  { FOOD_PEACH_PIE,                    STAT_PARRY_RATING,     200 },

  { FOOD_PEARL_MILK_TEA,               STAT_MASTERY_RATING,   200 },

  { FOOD_POUNDED_RICE_CAKE,            STAT_EXPERTISE_RATING, 100 },

  { FOOD_RED_BEAN_BUN,                 STAT_EXPERTISE_RATING, 200 },

  { FOOD_RICE_PUDDING,                 STAT_EXPERTISE_RATING, 275 },

  { FOOD_ROASTED_BARLEY_TEA,           STAT_MASTERY_RATING,   100 },

  { FOOD_SAUTEED_CARROTS,              STAT_AGILITY,          250 },

  { FOOD_SEA_MIST_RICE_NOODLES,        STAT_AGILITY,          300 },

  { FOOD_SEVERED_SAGEFISH_HEAD,        STAT_INTELLECT,         90 },
  { FOOD_SEVERED_SAGEFISH_HEAD,        STAT_STAMINA,           90 },

  { FOOD_SKEWERED_EEL,                 STAT_AGILITY,           90 },
  { FOOD_SKEWERED_EEL,                 STAT_STAMINA,           90 },

  { FOOD_SHRIMP_DUMPLINGS,             STAT_SPIRIT,           250 },

  { FOOD_SKEWERED_PEANUT_CHICKEN,      STAT_HIT_RATING,       200 },

  { FOOD_SPICY_SALMON,                 STAT_HIT_RATING,       300 },

  { FOOD_SPICY_VEGETABLE_BOWL,         STAT_EXPERTISE_RATING, 300 },

  { FOOD_STEAMED_CRAB_SURPRISE,        STAT_SPIRIT,           300 },

  { FOOD_SWIRLING_MIST_SOUP,           STAT_INTELLECT,        250 },

  { FOOD_TANGY_YOGURT,                 STAT_HASTE_RATING,     200 },

  { FOOD_TOASTED_FISH_JERKY,           STAT_CRIT_RATING,      100 },

  { FOOD_TWIN_FISH_PLATTER,            STAT_STAMINA,          415 },

  { FOOD_VALLEY_STIR_FRY,              STAT_AGILITY,          275 },

  { FOOD_WILDFOWL_GINSENG_SOUP,        STAT_HIT_RATING,       275 },

  { FOOD_WILDFOWL_ROAST,               STAT_STAMINA,          375 },

  { FOOD_YAK_CHEESE_CURDS,             STAT_HASTE_RATING,     100 },
};

struct flask_t : public action_t
{
  gain_t* gain;
  flask_e type;

  flask_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "flask", p ),
    gain( p -> get_gain( "flask" ) ), type( FLASK_NONE )
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

    type = util::parse_flask_type( type_str );
    if ( type == FLASK_NONE )
    {
      sim -> errorf( "Player %s attempting to use unsupported flask '%s'.\n",
                     player -> name(), type_str.c_str() );
      sim -> cancel();
    }
    else if ( type == FLASK_ALCHEMISTS && player -> profession[ PROF_ALCHEMY ] < 300 )
    {
      sim -> errorf( "Player %s attempting to use Alchemist's Flask without appropriate alchemy skill.\n",
                     player -> name() );
      sim -> cancel();
    }
  }

  virtual void execute()
  {
    player_t& p = *player;

    if ( type == FLASK_ALCHEMISTS )
    {
      stat_e boost_stat = STAT_STRENGTH;
      double stat_value = p.strength();

      if ( p.agility() > stat_value )
      {
        stat_value = p.agility();
        boost_stat = STAT_AGILITY;
      }
      if ( p.intellect() > stat_value )
      {
        stat_value = p.intellect();
        boost_stat = STAT_INTELLECT;
      }

      double amount = util::ability_rank( p.level, 320,86, 80,81, 40,71,  24,61,  0,0 );

      p.stat_gain( boost_stat, amount, gain, this );
    }
    else
    {
      for ( size_t i = 0; i < sizeof_array( flask_data ); ++i )
      {
        const flask_data_t& d = flask_data[ i ];
        if ( type == d.ft )
        {
          double amount = ( p.profession[ PROF_ALCHEMY ] > 50 ) ? d.mixology_stat_amount : d.stat_amount;
          p.stat_gain( d.st, amount, gain, this );

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
    if ( sim -> log ) sim -> output( "%s uses Flask %s", p.name(), util::flask_type_string( type ) );
    p.flask = type;
  }

  virtual bool ready()
  {
    return ( player -> sim -> allow_flasks            &&
             player -> flask           ==  FLASK_NONE &&
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
  food_e type;

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

    type = util::parse_food_type( type_str );
    if ( type == FOOD_NONE )
    {
      sim -> errorf( "Player %s: invalid food type '%s'\n", p -> name(), type_str.c_str() );
      sim -> cancel();
    }
  }

  virtual void execute()
  {
    player_t* p = player;
    if ( sim -> log ) sim -> output( "%s uses Food %s", p -> name(), util::food_type_string( type ) );
    p -> food = type;

    double food_stat_multiplier = 1.0;
    if ( p -> race == RACE_PANDAREN )
      food_stat_multiplier = 2.0;

    for ( size_t i = 0; i < sizeof_array( food_data ); ++i )
    {
      const food_data_t& d = food_data[ i ];
      if ( type == d.ft )
      {
        p -> stat_gain( d.st, d.stat_amount * food_stat_multiplier, gain, this );

        if ( d.st == STAT_STAMINA )
        {
          // Cap Health for stamina flasks if they are used outside of combat
          if ( ! player -> in_combat )
          {
            if ( d.stat_amount > 0 )
              player -> resource_gain( RESOURCE_HEALTH, player -> resources.max[ RESOURCE_HEALTH ] - player -> resources.current[ RESOURCE_HEALTH ] );
          }
        }
      }
    }


    double stamina = 0;
    double gain_amount = 0;

    switch ( type )
    {
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
        p -> stat_gain( STAT_INTELLECT, 90 * food_stat_multiplier, gain, this );
      }
      else
      {
        p -> stat_gain( STAT_AGILITY, 90 * food_stat_multiplier );
      }
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
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
        p -> stat_gain( STAT_INTELLECT, 90 * food_stat_multiplier, gain, this );
      }
      else
      {
        p -> stat_gain( STAT_AGILITY, 90 * food_stat_multiplier );
      }
      stamina = 90 * food_stat_multiplier; p -> stat_gain( STAT_STAMINA, stamina );
      break;
    case FOOD_BANQUET_OF_THE_BREW:
    case FOOD_BANQUET_OF_THE_GRILL:
    case FOOD_BANQUET_OF_THE_OVEN:
    case FOOD_BANQUET_OF_THE_POT:
    case FOOD_BANQUET_OF_THE_STEAMER:
    case FOOD_BANQUET_OF_THE_WOK:
    case FOOD_GREAT_BANQUET_OF_THE_BREW:
    case FOOD_GREAT_BANQUET_OF_THE_GRILL:
    case FOOD_GREAT_BANQUET_OF_THE_OVEN:
    case FOOD_GREAT_BANQUET_OF_THE_POT:
    case FOOD_GREAT_BANQUET_OF_THE_STEAMER:
    case FOOD_GREAT_BANQUET_OF_THE_WOK:
      if ( gain_amount <= 0.0 ) gain_amount = 250;
    case FOOD_PANDAREN_BANQUET:
    case FOOD_GREAT_PANDAREN_BANQUET:
      if ( gain_amount <= 0.0 ) gain_amount = 275;

      if ( p -> stats.dodge_rating > 0 )
      {
        p -> stat_gain( STAT_DODGE_RATING, gain_amount * food_stat_multiplier );
      }
      else if ( p -> stats.attribute[ ATTR_STRENGTH ] >= p -> stats.attribute[ ATTR_INTELLECT ] )
      {
        if ( p -> stats.attribute[ ATTR_STRENGTH ] >= p -> stats.attribute[ ATTR_AGILITY ] )
        {
          p -> stat_gain( STAT_STRENGTH, gain_amount * food_stat_multiplier );
        }
        else
        {
          p -> stat_gain( STAT_AGILITY, gain_amount * food_stat_multiplier );
        }
      }
      else if ( p -> stats.attribute[ ATTR_INTELLECT ] >= p -> stats.attribute[ ATTR_AGILITY ] )
      {
        p -> stat_gain( STAT_INTELLECT, gain_amount * food_stat_multiplier, gain, this );
      }
      else
      {
        p -> stat_gain( STAT_AGILITY, gain_amount * food_stat_multiplier );
      }
      break;

    default: break;
    }
    // Cap Health for food if used outside of combat
    if ( ! player -> in_combat )
    {
      if ( stamina > 0 )
        player -> resource_gain( RESOURCE_HEALTH, player -> resources.max[ RESOURCE_HEALTH ] - player -> resources.current[ RESOURCE_HEALTH ] );
    }


  }

  virtual bool ready()
  {
    return( player -> sim -> allow_food  &&
            player -> food == FOOD_NONE );
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
      { "min",     OPT_INT,     &min     },
      { "max",     OPT_INT,     &max     },
      { "trigger", OPT_INT,     &trigger },
      { NULL,      OPT_UNKNOWN, NULL     }
    };
    parse_options( options, options_str );

    if ( max <= 0 ) max = trigger;

    if ( min < 0 ) min = 0;
    if ( max < 0 ) max = 0;
    if ( min > max ) std::swap( min, max );

    if ( max <= 0 )
      min = max = util::ability_rank( player -> level,  30001,86, 10000,85, 4300,80,  2400,68,  1800,0 );

    assert( max > 0 );

    if ( trigger <= 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> output( "%s uses Mana potion", player -> name() );
    double gain = sim -> range( min, max );
    player -> resource_gain( RESOURCE_MANA, gain, player -> gains.mana_potion );
    player -> potion_used = true;
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

struct health_stone_t : public heal_t
{
  int charges;

  static double heal_percent() { return 0.2; }

  health_stone_t( player_t* p, const std::string& options_str ) :
    heal_t( "health_stone", p ), charges( 3 )
  {
    parse_options( NULL, options_str );

    cooldown -> duration = timespan_t::from_minutes( 2 );
    trigger_gcd = timespan_t::zero();

    harmful = false;
    may_crit = true;

    target = player;
  }

  virtual void reset()
  { charges = 3; }

  virtual void execute()
  {
    --charges;
    base_dd_min = base_dd_max = heal_percent() * player -> resources.current[ RESOURCE_HEALTH ];
    heal_t::execute();
  }

  virtual bool ready()
  {
    if ( charges <= 0 )
      return false;

    if ( ! if_expr )
    {
      if ( player -> resources.current[ RESOURCE_HEALTH ] > ( 1 - heal_percent() ) * player -> resources.max[ RESOURCE_HEALTH ] )
      return false;
    }

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
    if ( sim -> log ) sim -> output( "%s uses Dark Rune", player -> name() );
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
  timespan_t pre_pot_time;
  buff_t*    potion_buff;

  potion_base_t( player_t* p, const std::string& n, buff_t* pb, const std::string& options_str ) :
    action_t( ACTION_USE, n, p ),
    pre_pot_time( timespan_t::from_seconds( 5.0 ) ),
    potion_buff( pb )
  {
    assert( pb );

    option_t options[] =
    {
      { "pre_pot_time", OPT_TIMESPAN, &pre_pot_time },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    pre_pot_time = std::max( timespan_t::zero(), std::min( pre_pot_time, potion_buff -> buff_duration ) );

    trigger_gcd = timespan_t::zero();
    harmful = false;
    cooldown = p -> get_cooldown( "potion" );
    cooldown -> duration = potion_buff -> cooldown -> duration;
  }

  virtual void execute()
  {
    if ( player -> in_combat )
    {
      potion_buff -> trigger();
      player -> potion_used = true;
    }
    else
    {
      cooldown -> duration -= pre_pot_time;
      potion_buff -> trigger( 1, buff_t::DEFAULT_VALUE(), potion_buff -> default_chance,
                              potion_buff ->  buff_duration - pre_pot_time );
    }

    if ( sim -> log ) sim -> output( "%s uses %s", player -> name(), name() );
    update_ready();
    cooldown -> duration = potion_buff -> cooldown -> duration;
  }

  virtual bool ready()
  {
    if ( player -> potion_used )
      return false;

    return action_t::ready();
  }
};

} // END UNNAMED NAMESPACE

// ==========================================================================
// consumable_t::create_action
// ==========================================================================

action_t* consumable::create_action( player_t*          p,
                                     const std::string& name,
                                     const std::string& options_str )
{
  if ( name == "dark_rune"            ) return new    dark_rune_t( p, options_str );
  if ( name == "flask"                ) return new        flask_t( p, options_str );
  if ( name == "food"                 ) return new         food_t( p, options_str );
  if ( name == "health_stone"         ) return new health_stone_t( p, options_str );
  if ( name == "mana_potion"          ) return new  mana_potion_t( p, options_str );
  if ( name == "mythical_mana_potion" ) return new  mana_potion_t( p, options_str );
  if ( name == "speed_potion"         ) return new  potion_base_t( p, name, p -> potion_buffs.speed, options_str );
  if ( name == "volcanic_potion"      ) return new  potion_base_t( p, name, p -> potion_buffs.volcanic, options_str );
  if ( name == "earthen_potion"       ) return new  potion_base_t( p, name, p -> potion_buffs.earthen, options_str );
  if ( name == "golemblood_potion"    ) return new  potion_base_t( p, name, p -> potion_buffs.golemblood, options_str );
  if ( name == "tolvir_potion"        ) return new  potion_base_t( p, name, p -> potion_buffs.tolvir, options_str );
  // new mop potions
  if ( name == "jade_serpent_potion"  ) return new  potion_base_t( p, name, p -> potion_buffs.jade_serpent, options_str );
  if ( name == "mountains_potion"     ) return new  potion_base_t( p, name, p -> potion_buffs.mountains, options_str );
  if ( name == "mogu_power_potion"    ) return new  potion_base_t( p, name, p -> potion_buffs.mogu_power, options_str );
  if ( name == "virmens_bite_potion"  ) return new  potion_base_t( p, name, p -> potion_buffs.virmens_bite, options_str );

  return 0;
}

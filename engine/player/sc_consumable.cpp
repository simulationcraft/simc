// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

enum elixir_type_e
{
  ELIXIR_GUARDIAN,
  ELIXIR_BATTLE
};

struct elixir_data_t
{
  std::string name;
  elixir_type_e type;
  stat_e st;
  int stat_amount;
};

const elixir_data_t elixir_data[] =
{
  // mop
  { "mantid", ELIXIR_GUARDIAN, STAT_BONUS_ARMOR, 256 },
  { "mad_hozen", ELIXIR_BATTLE, STAT_CRIT_RATING, 85 },
};

struct food_data_t
{
  food_e ft;
  stat_e st;
  int stat_amount;
};

const food_data_t food_data[] =
{
  // Wrath
  { FOOD_FISH_FEAST,                   STAT_ATTACK_POWER,        19 },
  { FOOD_FISH_FEAST,                   STAT_SPELL_POWER,         22 },
  { FOOD_FISH_FEAST,                   STAT_STAMINA,             19 },

  // Cata
  { FOOD_BAKED_ROCKFISH,               STAT_CRIT_RATING,         36 },
  { FOOD_BAKED_ROCKFISH,               STAT_STAMINA,             36 },

  { FOOD_BASILISK_LIVERDOG,            STAT_HASTE_RATING,        36 },
  { FOOD_BASILISK_LIVERDOG,            STAT_STAMINA,             36 },

  { FOOD_BEER_BASTED_CROCOLISK,        STAT_STRENGTH,            36 },
  { FOOD_BEER_BASTED_CROCOLISK,        STAT_STAMINA,             36 },

  { FOOD_BLACKBELLY_SUSHI,             STAT_PARRY_RATING,        36 },
  { FOOD_BLACKBELLY_SUSHI,             STAT_STAMINA,             36 },

  { FOOD_CROCOLISK_AU_GRATIN,          STAT_HASTE_RATING,        36 },
  { FOOD_CROCOLISK_AU_GRATIN,          STAT_STAMINA,             36 },

  { FOOD_DELICIOUS_SAGEFISH_TAIL,      STAT_SPIRIT,              36 },
  { FOOD_DELICIOUS_SAGEFISH_TAIL,      STAT_STAMINA,             36 },

  { FOOD_GRILLED_DRAGON,               STAT_HIT_RATING,          36 },
  { FOOD_GRILLED_DRAGON,               STAT_STAMINA,             36 },

  { FOOD_LAVASCALE_FILLET,             STAT_MASTERY_RATING,      24 },
  { FOOD_LAVASCALE_FILLET,             STAT_STAMINA,             24 },

  { FOOD_MUSHROOM_SAUCE_MUDFISH,       STAT_DODGE_RATING,        36 },
  { FOOD_MUSHROOM_SAUCE_MUDFISH,       STAT_STAMINA,             36 },

  { FOOD_SEVERED_SAGEFISH_HEAD,        STAT_INTELLECT,           36 },
  { FOOD_SEVERED_SAGEFISH_HEAD,        STAT_STAMINA,             36 },

  { FOOD_SKEWERED_EEL,                 STAT_AGILITY,             36 },
  { FOOD_SKEWERED_EEL,                 STAT_STAMINA,             36 },

  // MoP
  { FOOD_BLACK_PEPPER_RIBS_AND_SHRIMP, STAT_STRENGTH,            34 },

  { FOOD_BLANCHED_NEEDLE_MUSHROOMS,    STAT_DODGE_RATING,        23 },

  { FOOD_BOILED_SILKWORM_PUPA,         STAT_HIT_RATING,          11 },

  { FOOD_BRAISED_TURTLE,               STAT_INTELLECT,           31 },

  { FOOD_CHARBROILED_TIGER_STEAK,      STAT_STRENGTH,            28 },

  { FOOD_CHUN_TIAN_SPRING_ROLLS,       STAT_STAMINA,             51 },

  { FOOD_CRAZY_SNAKE_NOODLES,          STAT_DODGE_RATING,        23 },

  { FOOD_DRIED_NEEDLE_MUSHROOMS,       STAT_DODGE_RATING,        11 },

  { FOOD_DRIED_PEACHES,                STAT_PARRY_RATING,        11 },

  { FOOD_ETERNAL_BLOSSOM_FISH,         STAT_STRENGTH,            31 },

  { FOOD_FIRE_SPIRIT_SALMON,           STAT_SPIRIT,              31 },

  { FOOD_GREEN_CURRY_FISH,             STAT_CRIT_RATING,         23 },

  { FOOD_GOLDEN_DRAGON_NOODLES,        STAT_HASTE_RATING,        23 },

  { FOOD_HARMONIOUS_RIVER_NOODLES,     STAT_PARRY_RATING,        23 },

  { FOOD_LUCKY_MUSHROOM_NOODLES,       STAT_CRIT_RATING,         23 },

  { FOOD_MANGO_ICE,                    STAT_MASTERY_RATING,      34 },

  { FOOD_MOGU_FISH_STEW,               STAT_INTELLECT,           34 },

  { FOOD_PEACH_PIE,                    STAT_PARRY_RATING,        23 },

  { FOOD_PEARL_MILK_TEA,               STAT_MASTERY_RATING,      23 },

  { FOOD_POUNDED_RICE_CAKE,            STAT_HASTE_RATING,        11 },

  { FOOD_RED_BEAN_BUN,                 STAT_HASTE_RATING,        23 },

  { FOOD_RICE_PUDDING,                 STAT_HASTE_RATING,        31 },

  { FOOD_ROASTED_BARLEY_TEA,           STAT_MASTERY_RATING,      11 },

  { FOOD_SAUTEED_CARROTS,              STAT_AGILITY,             28 },

  { FOOD_SEA_MIST_RICE_NOODLES,        STAT_AGILITY,             34 },

  { FOOD_SHRIMP_DUMPLINGS,             STAT_SPIRIT,              28 },

  { FOOD_SKEWERED_PEANUT_CHICKEN,      STAT_HIT_RATING,          23 },

  { FOOD_SPICY_SALMON,                 STAT_HIT_RATING,          34 },

  { FOOD_SPICY_MUSHAN_NOODLES,         STAT_HIT_RATING,          23 },

  { FOOD_SPICY_VEGETABLE_CHIPS,        STAT_HASTE_RATING,        34 },

  { FOOD_STEAMED_CRAB_SURPRISE,        STAT_SPIRIT,              34 },

  { FOOD_STEAMING_GOAT_NOODLES,        STAT_HASTE_RATING,        23 },

  { FOOD_SWIRLING_MIST_SOUP,           STAT_INTELLECT,           28 },

  { FOOD_TANGY_YOGURT,                 STAT_HASTE_RATING,        23 },

  { FOOD_TOASTED_FISH_JERKY,           STAT_CRIT_RATING,         11 },

  { FOOD_TWIN_FISH_PLATTER,            STAT_STAMINA,             47 },

  { FOOD_VALLEY_STIR_FRY,              STAT_AGILITY,             31 },

  { FOOD_WILDFOWL_GINSENG_SOUP,        STAT_HIT_RATING,          31 },

  { FOOD_WILDFOWL_ROAST,               STAT_STAMINA,             43 },

  { FOOD_YAK_CHEESE_CURDS,             STAT_HASTE_RATING,        11 },

  // WoD
  { FOOD_BLACKROCK_BARBECUE,           STAT_CRIT_RATING,        100 },

  { FOOD_BLACKROCK_HAM,                STAT_CRIT_RATING,        75 },

  { FOOD_BRAISED_BASILISK,             STAT_MASTERY_RATING,     75 },

  { FOOD_BUTTERED_STURGEON,            STAT_HASTE_RATING,       125 },

  { FOOD_CALAMARI_CREPES,              STAT_MULTISTRIKE_RATING, 100 },

  { FOOD_CLEFTHOOF_SAUSAGES,           STAT_VERSATILITY_RATING, 75 },

  { FOOD_FAT_SLEEPER_CAKES,            STAT_MASTERY_RATING,     75 },

  { FOOD_FIERY_CALAMARI,               STAT_MULTISTRIKE_RATING, 75 },

  { FOOD_FROSTY_STEW,                  STAT_HASTE_RATING,       100 },

  { FOOD_GORGROND_CHOWDER,             STAT_VERSATILITY_RATING, 100 },

  { FOOD_GRILLED_GULPER,               STAT_CRIT_RATING,        75 },

  { FOOD_HEARTY_ELEKK_STEAK,           STAT_STAMINA,            75 },

  { FOOD_JUMBO_SEA_DOG,                STAT_VERSATILITY_RATING, 125 },

  { FOOD_PAN_SEARED_TALBUK,            STAT_HASTE_RATING,       75 },

  { FOOD_PICKLED_EEL,                  STAT_CRIT_RATING,        125 },

  { FOOD_RYLAK_CREPES,                 STAT_MULTISTRIKE_RATING, 75 },

  { FOOD_SALTY_SQUID_ROLL,             STAT_MULTISTRIKE_RATING, 125 },

  { FOOD_SLEEPER_SURPRISE,             STAT_MASTERY_RATING,     100 },

  { FOOD_SLEEPER_SUSHI,                STAT_MASTERY_RATING,     125 },

  { FOOD_STEAMED_SCORPION,             STAT_STAMINA,            75 },

  { FOOD_STURGEON_STEW,                STAT_HASTE_RATING,       75 },

  { FOOD_TALADOR_SURF_AND_TURF,        STAT_STAMINA,            112 },

  { FOOD_WHIPTAIL_FILLET,              STAT_STAMINA,            125 },
};

struct flask_base_t : public action_t
{
  gain_t* gain;

  flask_base_t( player_t* p, const std::string& action_name_str ) :
    action_t( ACTION_USE, action_name_str, p ),
    gain( p -> get_gain( action_name_str ) )
  {
    harmful = callbacks = false;

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), player -> consumables.flask -> name() );

    player -> consumables.flask -> trigger();
  }

  virtual bool ready()
  {
    if ( ! player -> sim -> allow_flasks )
      return false;

    if ( ! player -> consumables.flask )
      return false;

    if ( player -> consumables.flask && player -> consumables.flask -> check() )
      return false;

    if ( player -> consumables.battle_elixir && player -> consumables.battle_elixir -> check() )
      return false;

    if( player -> consumables.guardian_elixir && player -> consumables.guardian_elixir -> check() )
      return false;

    return action_t::ready();
  }
};

struct flask_t : public flask_base_t
{
  std::string flask_name;
  bool alchemist;

  flask_t( player_t* p, const std::string& options_str ) :
    flask_base_t( p, "flask" ),
    alchemist( false )
  {
    std::string type_str;

    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );

    const item_data_t* item;
    for ( item = dbc::items( maybe_ptr( p -> dbc.ptr ) ); item -> id != 0; item++ )
    {
      if ( item -> item_class != 0 )
        continue;

      if ( item -> item_subclass != ITEM_SUBCLASS_FLASK )
        continue;

      flask_name = item -> name;

      util::tokenize( flask_name );

      if ( util::str_compare_ci( flask_name, type_str ) )
        break;

      std::string::size_type offset = flask_name.find( "flask_of_the_" );
      if ( offset != std::string::npos )
      {
        offset += 13;
        if ( util::str_compare_ci( flask_name.substr( offset ), type_str ) )
          break;
      }
      else
      {
        offset = flask_name.find( "flask_of_" );
        if ( offset != std::string::npos )
        {
          offset += 9;
          if ( util::str_compare_ci( flask_name.substr( offset ), type_str ) )
            break;
        }
      }
    }

    const spell_data_t* spell = 0;
    for ( size_t i = 0, end = sizeof_array( item -> id_spell ); i < end; i++ )
    {
      if ( item -> trigger_spell[ i ] == ITEM_SPELLTRIGGER_ON_USE )
      {
        spell = p -> find_spell( item -> id_spell[ i ] );
        break;
      }
    }

    if ( ! util::str_in_str_ci( type_str, "alchemist" ) )
    {
      if ( ! item || item -> id == 0 || ! spell || spell -> id() == 0 )
      {
        sim -> errorf( "Player %s attempting to use unsupported flask '%s'.\n",
                      player -> name(), type_str.c_str() );
      }
      else
      {
        std::string buff_name = spell -> name_cstr();
        util::tokenize( buff_name );
        p -> consumables.flask = stat_buff_creator_t( p, buff_name, spell );
      }
    }
    // Alch flask, special handle
    else
    {
      if ( p -> profession[ PROF_ALCHEMY ] < 300 )
      {
        sim -> errorf( "Player %s attempting to use 'alchemist flask' with %d Alchemy.\n",
                      player -> name(), p -> profession[ PROF_ALCHEMY ] );
      }
      else
      {
        alchemist = true;
        p -> consumables.flask = stat_buff_creator_t( p, "alchemists_flask", p -> find_spell( 105617 ) )
                          .add_stat( STAT_AGILITY, 0 )
                          .add_stat( STAT_STRENGTH, 0 )
                          .add_stat( STAT_INTELLECT, 0 );
      }
    }
  }

  virtual void execute()
  {
    if ( alchemist )
    {
      double v = player -> consumables.flask -> data().effectN( 1 ).average( player );
      stat_e stat = STAT_NONE;
      if ( player -> cache.agility() >= player -> cache.strength() )
      {
        if ( player -> cache.agility() >= player -> cache.intellect() )
          stat = STAT_AGILITY;
        else
          stat = STAT_INTELLECT;
      }
      else
      {
        if ( player -> cache.strength() >= player -> cache.intellect() )
          stat = STAT_STRENGTH;
        else
          stat = STAT_INTELLECT;
      }

      if ( stat == STAT_AGILITY )
        player -> consumables.flask -> stats[ 0 ].amount = v;
      else if ( stat == STAT_STRENGTH )
        player -> consumables.flask -> stats[ 1 ].amount = v;
      else if ( stat == STAT_INTELLECT )
        player -> consumables.flask -> stats[ 2 ].amount = v;
    }

    flask_base_t::execute();
  }
};

struct elixir_t : public action_t
{
  gain_t* gain;
  const elixir_data_t* data;
  stat_buff_t* buff;

  elixir_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "elixir", p ),
    gain( p -> get_gain( "elixir" ) ),
    data( nullptr ),
    buff( nullptr )
  {
    std::string type_str;

    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    for ( size_t i = 0; i < sizeof_array( elixir_data ); ++i )
    {
      const elixir_data_t& d = elixir_data[ i ];
      if ( d.name == type_str )
      {
        data = &d;
        break;
      }
    }
    if ( ! data )
    {
      sim -> errorf( "Player %s attempting to use unsupported elixir '%s'.\n",
                     player -> name(), type_str.c_str() );
      background = true;
    }
    else
    {
      double amount = data -> stat_amount;
      buff = stat_buff_creator_t( player, data -> name + "_elixir" )
             .duration( timespan_t::from_seconds( 60 * 60 ) ) // 1hr
             .add_stat( data -> st, amount );
      if ( data -> type == ELIXIR_BATTLE )
      {
        player -> consumables.battle_elixir = buff;
      }
      else if ( data -> type == ELIXIR_GUARDIAN )
      {
        player -> consumables.guardian_elixir = buff;
      }
    }
  }

  virtual void execute()
  {
    player_t& p = *player;

    assert( buff );

    buff -> trigger();

    if ( data -> st == STAT_STAMINA )
    {
      // Cap Health for stamina elixir if used outside of combat
      if ( ! p.in_combat )
      {
        p.resource_gain( RESOURCE_HEALTH,
                         p.resources.max[ RESOURCE_HEALTH ] - p.resources.current[ RESOURCE_HEALTH ] );
      }
    }

    if ( sim -> log ) sim -> out_log.printf( "%s uses elixir %s", p.name(), data -> name.c_str() );

  }
  virtual bool ready()
  {
    if ( ! player -> sim -> allow_flasks )
      return false;

    if ( player -> consumables.flask &&  player -> consumables.flask -> check() )
      return false;

    assert( data );

    if ( data -> type == ELIXIR_BATTLE && player -> consumables.battle_elixir && player -> consumables.battle_elixir -> check() )
      return false;

    if ( data -> type == ELIXIR_GUARDIAN && player -> consumables.guardian_elixir && player -> consumables.guardian_elixir -> check() )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// Food
// ==========================================================================

struct food_t : public action_t
{
  food_e type;
  std::string type_str;

  food_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "food", p ),
    type( FOOD_NONE )
  {

    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    type = util::parse_food_type( type_str );
    if ( type == FOOD_NONE )
    {
      sim -> errorf( "Player %s: invalid food type '%s'\n", p -> name(), type_str.c_str() );
      background = true;
    }
    std::string food_name = util::food_type_string( type );
    food_name += "_food";

    player -> consumables.food = stat_buff_creator_t( player, food_name )
      .duration( timespan_t::from_seconds( 60 * 60 ) ); // 1hr
  }

  /**
   * Find food data in manual food database
   */
  const food_data_t* find_food( food_e type )
  {
    for( size_t i = 0; i < sizeof_array( food_data ); ++i )
    {
      const food_data_t& food = food_data[ i ];
      if ( type == food.ft )
      {
        return &food;
      }
    }

    return nullptr;
  }
  stat_e get_highest_secondary()
  {
    player_t* p = player;

    stat_e secondaries[] = { STAT_CRIT_RATING,
                             STAT_HASTE_RATING,
                             STAT_MASTERY_RATING,
                             STAT_MULTISTRIKE_RATING,
                             STAT_VERSATILITY_RATING };

    stat_e highest_secondary = secondaries[0];
    for ( size_t i = 1; i < sizeof_array( secondaries ); i++ )
    {
      if ( p -> current.stats.get_stat( highest_secondary ) <
           p -> current.stats.get_stat( secondaries[ i ] ) )
      {
        highest_secondary = secondaries[ i ];
      }
    }

    return highest_secondary;
  }

  /**
   * New WoD feasts
   */
  stat_e get_wod_feast_stat()
  {
    if ( player -> current.stats.dodge_rating > 0 )
    {
      return STAT_DODGE_RATING;
    }
    return get_highest_secondary();
  }

  /**
   * For old feasts ( MoP )
   */
  stat_e get_mop_feast_stat()
  {
    player_t* p = player;
    stat_e stat = STAT_NONE;
    if ( p -> current.stats.dodge_rating > 0 )
    {
      stat = STAT_DODGE_RATING;
    }
    else if ( p -> current.stats.attribute[ ATTR_STRENGTH ] >= p -> current.stats.attribute[ ATTR_INTELLECT ] )
    {
      if ( p -> current.stats.attribute[ ATTR_STRENGTH ] >= p -> current.stats.attribute[ ATTR_AGILITY ] )
      {
        stat = STAT_STRENGTH;
      }
      else
      {
        stat = STAT_AGILITY;
      }
    }
    else if ( p -> current.stats.attribute[ ATTR_INTELLECT ] >= p -> current.stats.attribute[ ATTR_AGILITY ] )
    {
      stat = STAT_INTELLECT;
    }
    else
    {
      stat = STAT_AGILITY;
    }

    return stat;
  }

  virtual void execute()
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s uses Food %s", player -> name(), util::food_type_string( type ) );

    player -> consumables.food->stats.clear(); // Clear old stat pairs
    std::vector<stat_buff_t::buff_stat_t> stats; // vector of new stat pairs to be added

    double gain_amount = 0.0; // tmp value for stat gain amount

    double food_stat_multiplier = 1.0;
    if ( is_pandaren( player -> race ) )
      food_stat_multiplier = 2.00;

    switch ( type )
    {
    // *************************
    // MoP Feasts
    // *************************
      case FOOD_FORTUNE_COOKIE:
      case FOOD_SEAFOOD_MAGNIFIQUE_FEAST:
        if ( gain_amount <= 0.0 )
          gain_amount = 36;
        stats.push_back( stat_buff_t::buff_stat_t( STAT_STAMINA, gain_amount * food_stat_multiplier ) ); // Stamina buff is extra on top of the normal stats
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
      case FOOD_NOODLE_SOUP:
        if ( gain_amount <= 0.0 ) gain_amount = 28;
      case FOOD_PANDAREN_BANQUET:
      case FOOD_GREAT_PANDAREN_BANQUET:
      case FOOD_DELUX_NOODLE_SOUP:
        if ( gain_amount <= 0.0 ) gain_amount = 31;
      case FOOD_PANDAREN_TREASURE_NOODLE_SOUP:
        if ( gain_amount <= 0.0 ) gain_amount = 34;

        // Now we add the actual stats
        gain_amount *= food_stat_multiplier;
        stats.push_back( stat_buff_t::buff_stat_t( get_mop_feast_stat(), gain_amount ) );
        break;

      // *************************
      // WoD Feasts
      // *************************
      case FOOD_FEAST_OF_BLOOD:
      case FOOD_FEAST_OF_THE_WATERS:
        if ( gain_amount <= 0.0 ) gain_amount = 50;
      case FOOD_SAVAGE_FEAST:
        if ( gain_amount <= 0.0 ) gain_amount = 100;

        gain_amount *= food_stat_multiplier;
        stats.push_back( stat_buff_t::buff_stat_t( get_wod_feast_stat(), gain_amount ) );
        break;

      default:
        // *************************
        // Manual Food Database
        // *************************
        const food_data_t* fooddata = find_food( type );
        if ( fooddata )
        {
          stats.push_back( stat_buff_t::buff_stat_t( fooddata -> st, fooddata -> stat_amount * food_stat_multiplier ) );
          if ( sim -> log )
            sim -> out_log.printf( "Player %s found food data for '%s': stat_type=%s amount=%i",
                                   player -> name(), type_str.c_str(),
                                   util::stat_type_string( fooddata -> st), fooddata -> stat_amount);
        }
        else
        {
          sim -> errorf( "Player %s attempting to use unsupported food '%s'.\n",
                         player -> name(), type_str.c_str() );
        }
        break;
    }

    player -> consumables.food -> stats = stats;

    player -> consumables.food -> trigger();
  }

  virtual bool ready()
  {
    if ( ! player -> sim -> allow_food )
      return false;

    if ( ! player -> consumables.food )
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
    add_option( opt_int( "min", min ) );
    add_option( opt_int( "max", max ) );
    add_option( opt_int( "trigger", trigger ) );
    parse_options( options_str );

    if ( max <= 0 ) max = trigger;

    if ( min < 0 ) min = 0;
    if ( max < 0 ) max = 0;
    if ( min > max ) std::swap( min, max );

    if ( max <= 0 )
      min = max = util::ability_rank( player -> level,  30001, 86, 10000, 85, 4300, 80,  2400, 68,  1800, 0 );

    assert( max > 0 );

    if ( trigger <= 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s uses Mana potion", player -> name() );
    double gain = rng().range( min, max );
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

  health_stone_t( player_t* p, const std::string& options_str ) :
    heal_t( "health_stone", p ), charges( 3 )
  {
    parse_options( options_str );

    cooldown -> duration = timespan_t::from_minutes( 2 );
    trigger_gcd = timespan_t::zero();

    harmful = false;
    may_crit = true;

    pct_heal = 0.2;

    target = player;
  }

  virtual void reset()
  { charges = 3; }

  virtual void execute()
  {
    assert( charges > 0 );
    --charges;
    heal_t::execute();
  }

  virtual bool ready()
  {
    if ( charges <= 0 )
      return false;

    if ( ! if_expr )
    {
      if ( player -> resources.pct( RESOURCE_HEALTH ) > ( 1 - pct_heal ) )
        return false;
    }

    return action_t::ready();
  }
};

// ==========================================================================
//  Potion Base
// ==========================================================================

struct dbc_potion_t : public action_t
{
  timespan_t pre_pot_time;
  stat_buff_t* stat_buff;
  std::string consumable_name_str;

  dbc_potion_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "potion", p ),
    pre_pot_time( timespan_t::from_seconds( 2.0 ) ),
    stat_buff( 0 )
  {
    harmful = callbacks = may_miss = may_crit = may_block = may_glance = may_dodge = may_parry = false;
    proc = true;
    trigger_gcd = timespan_t::zero();

    add_option( opt_timespan( "pre_pot_time", pre_pot_time ) );
    add_option( opt_string( "name", consumable_name_str ) );
    parse_options( options_str );


    const item_data_t* item = unique_gear::find_consumable( p -> dbc, consumable_name_str, ITEM_SUBCLASS_POTION );
    // Cannot find potion by the name given; background the action
    if ( ! item )
    {
      sim -> errorf( "%s: Could not find a potion item with the name '%s'.", player -> name(), consumable_name_str.c_str() );
      background = true;
      return;
    }

    // Setup the stat buff, use the first available spell in the potion item
    for ( size_t i = 0, end = sizeof_array( item -> id_spell ); i < end; i++ )
    {
      if ( item -> id_spell[ i ] > 0 )
      {
        const spell_data_t* spell = p -> find_spell( item -> id_spell[ i ] );
        // Safe cast, as we check for positive above
        if ( spell -> id() != static_cast< unsigned >( item -> id_spell[ i ] ) )
          continue;

        std::string spell_name = spell -> name_cstr();
        util::tokenize( spell_name );
        buff_t* existing_buff = buff_t::find( p -> buff_list, spell_name );
        if ( ! existing_buff )
          stat_buff = stat_buff_creator_t( p, spell_name, spell );
        else
        {
          assert( dynamic_cast< stat_buff_t* >( existing_buff ) && "Potion stat buff is not stat_buff_t" );
          stat_buff = static_cast< stat_buff_t* >( existing_buff );
        }

        id = spell -> id();
        s_data = spell;

        stats = player -> get_stats( spell_name, this );
        break;
      }
    }

    if ( ! stat_buff )
    {
      sim -> errorf( "%s: No buff found in potion '%s'.", player -> name(), item -> name );
      background = true;
      return;
    }

    // Setup cooldown
    cooldown = p -> get_cooldown( "potion" );
    for ( size_t i = 0, end = sizeof_array( item -> cooldown_category ); i < end; i++ )
    {
      if ( item -> cooldown_group[ i ] > 0 && item -> cooldown_group_duration[ i ] > 0 )
      {
        cooldown -> duration = timespan_t::from_millis( item -> cooldown_group_duration[ i ] );
        break;
      }
    }

    if ( cooldown -> duration == timespan_t::zero() )
    {
      sim -> errorf( "%s: No cooldown found for potion '%s'.", player -> name(), item -> name );
      background = true;
      return;
    }

    // Sanity check pre-pot time at this time so that it's not longer than the duration of the buff
    pre_pot_time = std::max( timespan_t::zero(), std::min( pre_pot_time, stat_buff -> data().duration() ) );
  }

  void update_ready( timespan_t cd_duration )
  {
    // If the player is in combat, just make a very long CD
    if ( player -> in_combat )
      cd_duration = sim -> max_time * 3;
    else
      cd_duration = cooldown -> duration - pre_pot_time;

    action_t::update_ready( cd_duration );
  }

  // Needed to satisfy normal execute conditions
  result_e calculate_result( action_state_t* )
  { return RESULT_HIT; }

  void execute()
  {
    action_t::execute();

    timespan_t duration = stat_buff -> data().duration();

    if ( ! player -> in_combat )
      duration -= pre_pot_time;

    stat_buff -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }
};


struct augmentation_t : public action_t
{
  augmentation_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_USE, "augmentation", p )
  {
    std::string type_str;

    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );
    if ( util::str_compare_ci( type_str, "focus" ) )
    {
      player -> consumables.augmentation = stat_buff_creator_t( player, "focus_augmentation" )
          .spell( player -> find_spell( 175457 ) );
    }
    else if  ( util::str_compare_ci( type_str, "hyper" ) )
    {
      player -> consumables.augmentation = stat_buff_creator_t( player, "hyper_augmentation" )
          .spell( player -> find_spell( 175456 ) );
    }
    else if  ( util::str_compare_ci( type_str, "stout" ) )
    {
      player -> consumables.augmentation = stat_buff_creator_t( player, "stout_augmentation" )
          .spell( player -> find_spell( 175439 ) );
    }
    else
    {
      sim -> errorf( "%s unknown augmentation type: '%s'", player -> name(), type_str.c_str() );
      background = true;
    }

    trigger_gcd = timespan_t::zero();
    harmful = false;

  }

  virtual void execute()
  {
    assert( player -> consumables.augmentation );

    player -> consumables.augmentation -> trigger();

    if ( sim -> log )
      sim -> out_log.printf( "%s uses augmentation.", player -> name() );

  }
  virtual bool ready()
  {
    if ( ! player -> consumables.augmentation )
      return false;

    if ( player -> consumables.augmentation && player -> consumables.augmentation -> check() )
      return false;

    return action_t::ready();
  }
};

// Misc consumables

struct oralius_whispering_crystal_t : public flask_base_t
{
  oralius_whispering_crystal_t( player_t* p, const std::string& options_str ) :
    flask_base_t( p, "oralius_whispering_crystal" )
  {
    parse_options( options_str );

    const spell_data_t* spell = p -> find_spell( 176151 );

    std::string buff_name = spell -> name_cstr();
    util::tokenize( buff_name );
    p -> consumables.flask = stat_buff_creator_t( p, buff_name, spell );
  }
};

struct crystal_of_insanity_t : public flask_base_t
{
  crystal_of_insanity_t( player_t* p, const std::string& options_str ) :
    flask_base_t( p, "crystal_of_insanity" )
  {
    parse_options( options_str );

    const spell_data_t* spell = p -> find_spell( 127230 );

    std::string buff_name = spell -> name_cstr();
    util::tokenize( buff_name );
    p -> consumables.flask = stat_buff_creator_t( p, buff_name, spell );
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
  if ( name == "potion"               ) return new   dbc_potion_t( p, options_str );
  if ( name == "flask"                ) return new        flask_t( p, options_str );
  if ( name == "elixir"               ) return new       elixir_t( p, options_str );
  if ( name == "food"                 ) return new         food_t( p, options_str );
  if ( name == "health_stone"         ) return new health_stone_t( p, options_str );
  if ( name == "mana_potion"          ) return new  mana_potion_t( p, options_str );
  if ( name == "augmentation"         ) return new augmentation_t( p, options_str );

  // Misc consumables
  if ( name == "oralius_whispering_crystal" ) return new oralius_whispering_crystal_t( p, options_str );
  if ( name == "crystal_of_insanity" ) return new crystal_of_insanity_t( p, options_str );

  return 0;
}

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

  { FOOD_CALAMARI_CREPES,              STAT_CRIT_RATING,        100 },

  { FOOD_CLEFTHOOF_SAUSAGES,           STAT_VERSATILITY_RATING, 75 },

  { FOOD_FAT_SLEEPER_CAKES,            STAT_MASTERY_RATING,     75 },

  { FOOD_FIERY_CALAMARI,               STAT_CRIT_RATING,        75 },

  { FOOD_FROSTY_STEW,                  STAT_HASTE_RATING,       100 },

  { FOOD_GORGROND_CHOWDER,             STAT_VERSATILITY_RATING, 100 },

  { FOOD_GRILLED_GULPER,               STAT_CRIT_RATING,        75 },

  { FOOD_HEARTY_ELEKK_STEAK,           STAT_STAMINA,            75 },

  { FOOD_JUMBO_SEA_DOG,                STAT_VERSATILITY_RATING, 125 },

  { FOOD_PAN_SEARED_TALBUK,            STAT_HASTE_RATING,       75 },

  { FOOD_PICKLED_EEL,                  STAT_CRIT_RATING,        125 },

  { FOOD_RYLAK_CREPES,                 STAT_CRIT_RATING,        75 },

  { FOOD_SALTY_SQUID_ROLL,             STAT_CRIT_RATING,        125 },

  { FOOD_SLEEPER_SURPRISE,             STAT_MASTERY_RATING,     100 },

  { FOOD_SLEEPER_SUSHI,                STAT_MASTERY_RATING,     125 },

  { FOOD_STEAMED_SCORPION,             STAT_STAMINA,            75 },

  { FOOD_STURGEON_STEW,                STAT_HASTE_RATING,       75 },

  { FOOD_TALADOR_SURF_AND_TURF,        STAT_STAMINA,            112 },

  { FOOD_WHIPTAIL_FILLET,              STAT_STAMINA,            125 },

  // Legion
  { FOOD_AZSHARI_SALAD,                STAT_HASTE_RATING,       375 },

  { FOOD_BARRACUDDA_MRGLGAGH,          STAT_MASTERY_RATING,     300 },

  { FOOD_KOISCENTED_STORMRAY,          STAT_VERSATILITY_RATING, 300 },

  { FOOD_LEYBEQUE_RIBS,                STAT_CRIT_RATING,        300 },

  { FOOD_NIGHTBORNE_DELICACY_PLATTER,  STAT_MASTERY_RATING,     375 },

  { FOOD_SEEDBATTERED_FISH_PLATE,      STAT_VERSATILITY_RATING, 375 },

  { FOOD_SURAMAR_SURF_AND_TURF,        STAT_HASTE_RATING,       300 },

  { FOOD_THE_HUNGRY_MAGISTER,          STAT_CRIT_RATING,        375 },
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

  virtual void execute() override
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
  virtual bool ready() override
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
    
    // Hardcode this for now
    if ( type == FOOD_FELMOUTH_FRENZY )
    {
      special_effect_t effect( player );
      unique_gear::initialize_special_effect( effect, 188534 );
      player -> special_effects.push_back( new special_effect_t( effect ) );
      // This needs to be done manually, because player_t::init_special_effects() is invoked before
      // init_actions.
      unique_gear::initialize_special_effect_2( player -> special_effects.back() );
    }
    else
    {
      std::string food_name = util::food_type_string( type );
      food_name += "_food";

      player -> consumables.food = stat_buff_creator_t( player, food_name )
        .duration( timespan_t::from_seconds( 60 * 60 ) ); // 1hr
    }
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
   * WoD feasts
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

  virtual void execute() override
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

      // *************************
      // Legion Feasts
      // *************************
      case FOOD_HEARTY_FEAST:
        if ( gain_amount <= 0.0 ) gain_amount = 150;
      case FOOD_LAVISH_SURAMAR_FEAST:
        if ( gain_amount <= 0.0 ) gain_amount = 200;

        gain_amount *= food_stat_multiplier;
        stats.push_back( stat_buff_t::buff_stat_t( get_mop_feast_stat(), gain_amount ) );
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

  virtual bool ready() override
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
      min = max = util::ability_rank( player -> level(),  30001, 86, 10000, 85, 4300, 80,  2400, 68,  1800, 0 );

    assert( max > 0 );

    if ( trigger <= 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute() override
  {
    if ( sim -> log ) sim -> out_log.printf( "%s uses Mana potion", player -> name() );
    double gain = rng().range( min, max );
    player -> resource_gain( RESOURCE_MANA, gain, player -> gains.mana_potion );
    player -> potion_used = true;
  }

  virtual bool ready() override
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

    base_pct_heal = 0.2;

    target = player;
  }

  virtual void reset() override
  { charges = 3; }

  virtual void execute() override
  {
    assert( charges > 0 );
    --charges;
    heal_t::execute();
  }

  virtual bool ready() override
  {
    if ( charges <= 0 )
      return false;

    if ( ! if_expr )
    {
      if ( player -> resources.pct( RESOURCE_HEALTH ) > ( 1 - base_pct_heal ) )
        return false;
    }

    return action_t::ready();
  }
};

// ==========================================================================
// DBC-backed consumable base class
// ==========================================================================

struct dbc_consumable_base_t : public action_t
{
  std::string              consumable_name;
  const item_data_t*       item_data;
  item_subclass_consumable type;

  action_t*                consumable_action;
  buff_t*                  consumable_buff;

  dbc_consumable_base_t( player_t* p, const std::string& name_str ) :
    action_t( ACTION_USE, name_str, p ), item_data( nullptr ), type( ITEM_SUBCLASS_CONSUMABLE )
  {
    add_option( opt_string( "name", consumable_name ) );

    harmful = callbacks = may_crit = may_miss = false;

    trigger_gcd = timespan_t::zero();

    // Consumables always target the owner
    target = player;
  }

  void init() override
  {
    item_data = unique_gear::find_consumable( player -> dbc, consumable_name, type );

    if ( ! initialize_consumable() )
    {
      sim -> errorf( "%s: Unable to initialize consumable %s for %s", player -> name(),
          consumable_name.c_str(), signature_str.c_str() );
      background = true;
    }

    action_t::init();
  }

  // Needed to satisfy normal execute conditions
  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  void execute() override
  {
    action_t::execute();

    if ( consumable_action )
    {
      consumable_action -> execute();
    }

    if ( consumable_buff )
    {
      consumable_buff -> trigger();
    }
  }

  // Find a suitable DBC spell for the consumable. This method is overridable where needed (for
  // example custom flasks)
  virtual const spell_data_t* driver() const
  {
    if ( ! item_data )
    {
      return spell_data_t::not_found();
    }

    for ( const auto& spell_id : item_data -> id_spell )
    {
      auto ptr = player -> find_spell( spell_id );
      if ( ptr -> id() == as<unsigned>( spell_id ) )
      {
        return ptr;
      }
    }

    return spell_data_t::not_found();
  }

  // Overridable method to customize the special effect that is used to drive the buff creation for
  // the consumable
  virtual special_effect_t* create_special_effect()
  {
    auto effect = new special_effect_t( player );
    effect = new special_effect_t( player );
    effect -> type = SPECIAL_EFFECT_USE;
    effect -> source = SPECIAL_EFFECT_SOURCE_ITEM;

    return effect;
  }

  // Attempts to initialize the consumable. Jumps through quite a few hoops to manage to create
  // special effects only once, if the user input contains multiple consumable lines (as is possible
  // with potions for example).
  virtual bool initialize_consumable()
  {
    if ( driver() -> id() == 0 )
    {
      sim -> errorf( "%s: Unable to find consumable %s for %s", player -> name(),
          consumable_name.c_str(), signature_str.c_str() );
      return false;
    }

    auto effect = unique_gear::find_special_effect( player, driver() -> id(), SPECIAL_EFFECT_USE );
    // No special effect for this consumable found, so create one
    if ( ! effect )
    {
      effect = create_special_effect();
      auto ret = unique_gear::initialize_special_effect( *effect, driver() -> id() );

      // Something went wrong with the special effect init, so return false (will disable this
      // consumable)
      if ( ret == false )
      {
        delete effect;
        return ret;
      }

      // First special effect initialization phase could not decude a proper consumable to create
      if ( effect -> type == SPECIAL_EFFECT_NONE )
      {
        delete effect;
        return false;
      }

      // Note, this needs to be added before initializing the (potentially) custom special effect,
      // since find_special_effect for this same driver needs to find this newly created special
      // effect, not anything the custom init might create.
      player -> special_effects.push_back( effect );

      // Finally, initialize the special effect. If it's a plain old stat buff this does nothing,
      // but some consumables require custom initialization.
      unique_gear::initialize_special_effect_2( effect );
    }

    // And then, grab the action and buff from the special effect, if they are enabled
    consumable_action = effect -> create_action();
    consumable_buff = effect -> create_buff();

    return true;
  }
};

// ==========================================================================
// Flasks (DBC-backed)
// ==========================================================================

struct flask_base_t : public dbc_consumable_base_t
{
  flask_base_t( player_t* p, const std::string& name, const std::string& options_str ) :
    dbc_consumable_base_t( p, name )
  {
    // Backwards compatibility reasons
    add_option( opt_string( "type", consumable_name ) );
    parse_options( options_str );

    type = ITEM_SUBCLASS_FLASK;
  }

  // Flasks (starting from legion it seems) have a reverse mapping to the spell that creates them in
  // the effect's trigger spell. This confuses the dbc-backed special effect system into thinking
  // that's the triggering spell, so let's override trigger spell here, presuming that the flasks
  // are simple stat buffs.
  special_effect_t* create_special_effect() override
  {
    auto e = dbc_consumable_base_t::create_special_effect();
    e -> trigger_spell_id = driver() -> id();
    return e;
  }

  void init() override
  {
    dbc_consumable_base_t::init();

    if ( ! background )
    {
      player -> consumables.flask = consumable_buff;
    }
  }

  bool ready() override
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

    return dbc_consumable_base_t::ready();
  }
};

struct flask_t : public flask_base_t
{
  flask_t( player_t* p, const std::string& options_str ) :
    flask_base_t( p, "flask", options_str )
  { }
};

struct oralius_whispering_crystal_t : public flask_base_t
{
  oralius_whispering_crystal_t( player_t* p, const std::string& options_str ) :
    flask_base_t( p, "oralius_whispering_crystal", options_str )
  { }

  const spell_data_t* driver() const override
  { return player -> find_spell( 176151 ); }
};

struct crystal_of_insanity_t : public flask_base_t
{
  crystal_of_insanity_t( player_t* p, const std::string& options_str ) :
    flask_base_t( p, "crystal_of_insanity", options_str )
  { }

  const spell_data_t* driver() const override
  { return player -> find_spell( 127230 ); }
};

// ==========================================================================
// Potions (DBC-backed)
// ==========================================================================

struct potion_t : public dbc_consumable_base_t
{
  timespan_t pre_pot_time;

  potion_t( player_t* p, const std::string& options_str ) :
    dbc_consumable_base_t( p, "potion" ),
    pre_pot_time( timespan_t::from_seconds( 2.0 ) )
  {
    add_option( opt_timespan( "pre_pot_time", pre_pot_time ) );
    parse_options( options_str );

    type = ITEM_SUBCLASS_POTION;
    cooldown = player -> get_cooldown( "potion" );
  }

  bool initialize_consumable() override
  {
    if ( ! dbc_consumable_base_t::initialize_consumable() )
    {
      return false;
    }

    // Setup a cooldown duration for the potion
    for ( size_t i = 0; i < sizeof_array( item_data -> cooldown_group ); i++ )
    {
      if ( item_data -> cooldown_group[ i ] > 0 && item_data -> cooldown_group_duration[ i ] > 0 )
      {
        cooldown -> duration = timespan_t::from_millis( item_data -> cooldown_group_duration[ i ] );
        break;
      }
    }

    if ( cooldown -> duration == timespan_t::zero() )
    {
      sim -> errorf( "%s: No cooldown found for potion '%s'",
          player -> name(), item_data -> name );
      return false;
    }

    // Sanity check pre-pot time at this time so that it's not longer than the duration of the buff
    if ( consumable_buff )
    {
      pre_pot_time = std::max( timespan_t::zero(),
                               std::min( pre_pot_time, consumable_buff -> data().duration() ) );
    }

    return true;
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    // If the player is in combat, just make a very long CD
    if ( player -> in_combat )
      cd_duration = sim -> max_time * 3;
    else
      cd_duration = cooldown -> duration - pre_pot_time;

    action_t::update_ready( cd_duration );
  }

  // Overwrite with fanciful execution due to prepotting
  void execute() override
  {
    action_t::execute();

    if ( consumable_action )
    {
      consumable_action -> execute();
    }

    if ( consumable_buff )
    {
      timespan_t duration = consumable_buff -> data().duration();
      if ( ! player -> in_combat )
      {
        duration -= pre_pot_time;
      }

      consumable_buff -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
    }
  }

  bool ready() override
  {
    if ( ! player -> sim -> allow_potions )
      return false;

    return dbc_consumable_base_t::ready();
  }
};

// ==========================================================================
// Augmentations (WoD raiding consumable) (DBC-backed)
// ==========================================================================

struct augmentation_t : public dbc_consumable_base_t
{
  augmentation_t( player_t* p, const std::string& options_str ) :
    dbc_consumable_base_t( p, options_str )
  {
    add_option( opt_string( "type", consumable_name ) );
    parse_options( options_str );
  }

  // Custom driver for now, we don't really want to include the item data for now
  const spell_data_t* driver() const override
  {
    if      ( util::str_in_str_ci( consumable_name, "defiled" ) ) return player -> find_spell( 224001 );
    else if ( util::str_in_str_ci( consumable_name, "focus"   ) ) return player -> find_spell( 175457 );
    else if ( util::str_in_str_ci( consumable_name, "hyper"   ) ) return player -> find_spell( 175456 );
    else if ( util::str_in_str_ci( consumable_name, "stout"   ) ) return player -> find_spell( 175439 );
    else return spell_data_t::not_found();
  }

  void init() override
  {
    dbc_consumable_base_t::init();

    if ( consumable_buff )
    {
      player -> consumables.augmentation = consumable_buff;
    }
  }

  bool ready() override
  {
    if ( ! player -> consumables.augmentation )
      return false;

    if ( player -> consumables.augmentation && player -> consumables.augmentation -> check() )
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
  if ( name == "potion"               ) return new       potion_t( p, options_str );
  if ( name == "flask"                ) return new        flask_t( p, options_str );
  if ( name == "elixir"               ) return new       elixir_t( p, options_str );
  if ( name == "food"                 ) return new         food_t( p, options_str );
  if ( name == "health_stone"         ) return new health_stone_t( p, options_str );
  if ( name == "mana_potion"          ) return new  mana_potion_t( p, options_str );
  if ( name == "augmentation"         ) return new augmentation_t( p, options_str );

  // Misc consumables
  if ( name == "oralius_whispering_crystal" ) return new oralius_whispering_crystal_t( p, options_str );
  if ( name == "crystal_of_insanity" ) return new crystal_of_insanity_t( p, options_str );

  return nullptr;
}

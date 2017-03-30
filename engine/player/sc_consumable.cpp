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
  bool                     opt_disabled; // Disabled through a consumable-specific "disabled" keyword

  dbc_consumable_base_t( player_t* p, const std::string& name_str ) :
    action_t( ACTION_USE, name_str, p ), item_data( nullptr ), type( ITEM_SUBCLASS_CONSUMABLE ),
    consumable_action( nullptr ), consumable_buff( nullptr ), opt_disabled( false )
  {
    add_option( opt_string( "name", consumable_name ) );
    add_option( opt_string( "type", consumable_name ) );

    harmful = callbacks = may_crit = may_miss = false;

    trigger_gcd = timespan_t::zero();

    // Consumables always target the owner
    target = player;
  }

  expr_t* create_expression( const std::string& name_str ) override
  {
    auto split = util::string_split( name_str, "." );
    if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "consumable" ) )
    {
      auto match = util::str_compare_ci( consumable_name, split[ 1 ] );
      return expr_t::create_constant( name_str, match );
    }

    return action_t::create_expression( name_str );
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

  // Figure out the default consumable for a given type
  virtual std::string consumable_default() const
  { return std::string(); }

  // Consumable type is fully disabled; base class just returns the option state (for the consumable
  // type). Consumable type specialized classes take into account other options (i.e., the allow_X
  // sim-wide options)
  virtual bool disabled_consumable() const
  { return opt_disabled; }

  void init() override
  {
    // Figure out the default consumable name if nothing is given in the name/type parameters
    if ( consumable_name.empty() )
    {
      consumable_name = consumable_default();
      // Check for disabled string in the consumable player scope option
      opt_disabled = util::str_compare_ci( consumable_name, "disabled" );
    }
    // If a specific name is given, we'll still need to parse the potentially user given "disabled"
    // option to disable the consumable type entirely.
    else
    {
      opt_disabled = util::str_compare_ci( consumable_default(), "disabled" );
    }

    if ( ! disabled_consumable() )
    {
      item_data = unique_gear::find_consumable( player -> dbc, consumable_name, type );

      if ( ! initialize_consumable() )
      {
        sim -> errorf( "%s: Unable to initialize consumable %s for %s", player -> name(),
            consumable_name.empty() ? "none" : consumable_name.c_str(), signature_str.c_str() );
        background = true;
      }
    }

    action_t::init();
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
          consumable_name.empty() ? "none" : consumable_name.c_str(), signature_str.c_str() );
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

  bool ready() override
  {
    if ( disabled_consumable() )
    {
      return false;
    }

    return action_t::ready();
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
    parse_options( options_str );

    type = ITEM_SUBCLASS_FLASK;
  }

  bool disabled_consumable() const override
  { return dbc_consumable_base_t::disabled_consumable() || sim -> allow_flasks == false; }

  // Figure out the default consumable for flasks
  std::string consumable_default() const override
  {
    if ( ! player -> flask_str.empty() )
    {
      return player -> flask_str;
    }
    // Class module default, if defined
    else if ( ! player -> default_flask().empty() )
    {
      return player -> default_flask();
    }

    return std::string();
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

    // Note, potions pretty much have to be targeted to an enemy so expressions still work.
    target = p -> target;
  }

  bool disabled_consumable() const override
  { return dbc_consumable_base_t::disabled_consumable() || sim -> allow_potions == false; }

  std::string consumable_default() const override
  {
    if ( ! player -> potion_str.empty() )
    {
      return player -> potion_str;
    }
    else if ( ! player -> default_potion().empty() )
    {
      return player -> default_potion();
    }

    return std::string();
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
      timespan_t duration = consumable_buff -> buff_duration;
      if ( ! player -> in_combat )
      {
        duration -= pre_pot_time;
      }

      consumable_buff -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
    }
  }
};

// ==========================================================================
// Augmentation runes (Raiding consumable) (DBC-backed)
// ==========================================================================

struct augmentation_t : public dbc_consumable_base_t
{
  augmentation_t( player_t* p, const std::string& options_str ) :
    dbc_consumable_base_t( p, "augmentation" )
  {
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

// ==========================================================================
// Food (DBC-backed)
// ==========================================================================

struct food_t : public dbc_consumable_base_t
{
  // Some foods (like Felmouth Frenzy) cannot be inferred from item data directly, so have a manual
  // map to bind consumable names to driver spells. Initialized below.
  static const std::map<std::string, unsigned> __map;

  food_t( player_t* p, const std::string& options_str ) :
    dbc_consumable_base_t( p, "food" )
  {
    parse_options( options_str );

    type = ITEM_SUBCLASS_FOOD;
  }

  bool disabled_consumable() const override
  { return dbc_consumable_base_t::disabled_consumable() || sim -> allow_food == false; }

  std::string consumable_default() const override
  {
    if ( ! player -> food_str.empty() )
    {
      return player -> food_str;
    }
    else if ( ! player -> default_food().empty() )
    {
      return player -> default_food();
    }

    return std::string();
  }

  void init() override
  {
    dbc_consumable_base_t::init();

    if ( consumable_buff )
    {
      player -> consumables.food = consumable_buff;
    }
  }

  // For claritys sake, name the food buff with the food name. In HTML reports, it'll actually be
  // something like "Well Fed (buttered_sturgeon)".
  special_effect_t* create_special_effect() override
  {
    auto effect = dbc_consumable_base_t::create_special_effect();

    if ( item_data )
    {
      effect -> name_str = item_data -> name;
      util::tokenize( effect -> name_str );
    }

    return effect;
  }

  // Generic food buffs need some special handling. Blizzard constructs food spell data such that
  // the primary spell applies the food/drink buffs, and starts a 10 second periodic effect for the
  // "Well Fed" buff. We find well fed buff here, and return it as the driver. This will in turn let
  // the special_effect_t automatically infer stats from that spell data.
  const spell_data_t* driver() const override
  {
    // First, check the manual map for food buffs that are not sourced from actual consumable food
    // (Felmouth Frenzy for example)
    auto it = __map.find( consumable_name );
    if ( it != __map.end() )
    {
      return player -> find_spell( it -> second );
    }

    // Figure out base food buff (the spell you cast from the food item)
    const spell_data_t* driver = dbc_consumable_base_t::driver();
    if ( driver -> id() == 0 || ! driver -> _effects )
    {
      return driver;
    }

    // Find the "Well Fed" buff from the base food
    for ( const auto& effect : *driver -> _effects )
    {
      if ( ! effect )
      {
        continue;
      }

      // Return the "Well Fed" effect of the food (might not always be named well fed)
      if ( effect -> type() == E_APPLY_AURA &&
           effect -> subtype() == A_PERIODIC_TRIGGER_SPELL &&
           effect -> period() == timespan_t::from_millis( 10000 ) )
      {
        return effect -> trigger();
      }
    }

    return driver;
  }

  // Initialize the food buff, and if it's a stat buff, apply epicurean on it for Pandaren. On
  // action-based or non-stat-buff food, Epicurean must be applied manually. Note that even if the
  // food uses a custom initialization, if the resulting buff is a stat buff, Epicurean will be
  // automatically applied.
  bool initialize_consumable() override
  {
    auto ret = dbc_consumable_base_t::initialize_consumable();
    if ( ! ret )
    {
      return false;
    }

    // No buff, or non-Pandaren so we are finished here
    if ( ! consumable_buff || ! is_pandaren( player -> race ) )
    {
      return ret;
    }

    // Apply epicurean on the buff stats. Note, presumes that there's a single food APL line in the
    // profile or bad things will happen, but that's a relatively good presumption.
    if ( auto buff = dynamic_cast<stat_buff_t*>( consumable_buff ) )
    {
      range::for_each( buff -> stats, []( stat_buff_t::buff_stat_t& s ) { s.amount *= 2.0; } );
    }

    return ret;
  }

  bool ready() override
  {
    if ( ! player -> consumables.food )
      return false;

    if ( player -> consumables.food && player -> consumables.food -> check() )
      return false;

    return dbc_consumable_base_t::ready();
  }
};

const std::map<std::string, unsigned> food_t::__map = {
  { "felmouth_frenzy", 188534 },
  { "lemon_herb_filet", 185736 },
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

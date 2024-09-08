// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "consumable.hpp"

#include "action/heal.hpp"
#include "buff/buff.hpp"
#include "dbc/dbc.hpp"
#include "item/item.hpp"
#include "item/special_effect.hpp"
#include "player.hpp"
#include "player/unique_gear.hpp"
#include "sc_enums.hpp"
#include "sim/cooldown.hpp"
#include "sim/expressions.hpp"
#include "sim/sim.hpp"
#include "util/rng.hpp"
#include "util/static_map.hpp"
#include "util/string_view.hpp"

#include <string>

namespace
{  // UNNAMED NAMESPACE

// Find a consumable of a given subtype, see data_enum.hh for type values.
// Returns 0 if not found.
const dbc_item_data_t* find_consumable( const dbc_t& dbc, std::string_view name_str, item_subclass_consumable type )
{
  if ( name_str.empty() )
  {
    return nullptr;
  }

  std::string_view name;
  int quality = 0;

  // parse string for profession revamp rank suffix. name without a suffix will default to rank 1.
  auto r_pos = name_str.find_last_of( '_' );
  if ( r_pos != std::string::npos && r_pos == name_str.size() - 2 &&
       util::is_number( name_str.substr( r_pos + 1, 1 ) ) )
  {
    name = name_str.substr( 0, r_pos );
    quality = util::to_int( name_str.substr( r_pos + 1, 1 ) );
  }
  else
  {
    name = name_str;
  }

  // Poor man's longest matching prefix!
  const auto& item = dbc::find_consumable( type, dbc.ptr, [ name, quality ]( const dbc_item_data_t* i ) {
    auto n = util::tokenize_fn( i->name ? i->name : "unknown" );
    if ( util::str_in_str_ci( n, name ) )
    {
      auto q = i->crafting_quality;
      return q == quality || ( !quality && q == 1 );
    }

    return false;
  } );

  if ( item.id != 0 )
    return &item;

  return nullptr;
}

enum class elixir
{
  GUARDIAN,
  BATTLE
};

struct elixir_data_t
{
  util::string_view name;
  elixir type;
  stat_e st;
  int stat_amount;
};

static constexpr std::array<elixir_data_t, 2> elixir_data { {
  // mop
  { "mantid", elixir::GUARDIAN, STAT_BONUS_ARMOR, 256 },
  { "mad_hozen", elixir::BATTLE, STAT_CRIT_RATING, 85 },
} };

struct elixir_t : public action_t
{
  gain_t* gain;
  const elixir_data_t* data;
  stat_buff_t* buff;

  elixir_t( player_t* p, util::string_view options_str )
    : action_t( ACTION_USE, "elixir", p ), gain( p->get_gain( "elixir" ) ), data( nullptr ), buff( nullptr )
  {
    std::string type_str;

    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;

    for ( auto& elixir : elixir_data )
    {
      if ( elixir.name == type_str )
      {
        data = &elixir;
        break;
      }
    }
    if ( !data )
    {
      sim->error( "Player {} attempting to use unsupported elixir '{}'.\n", player->name(), type_str );
      background = true;
    }
    else
    {
      double amount = data->stat_amount;
      buff = make_buff<stat_buff_t>( player, fmt::format( "{}_elixir", data->name ) )->add_stat( data->st, amount );
      buff->set_duration( timespan_t::from_minutes( 60 ) );
      if ( data->type == elixir::BATTLE )
      {
        player->consumables.battle_elixir = buff;
      }
      else if ( data->type == elixir::GUARDIAN )
      {
        player->consumables.guardian_elixir = buff;
      }
    }
  }

  void execute() override
  {
    player_t& p = *player;

    assert( buff );

    buff->trigger();

    if ( data->st == STAT_STAMINA )
    {
      // Cap Health for stamina elixir if used outside of combat
      if ( !p.in_combat )
      {
        p.resource_gain( RESOURCE_HEALTH, p.resources.max[ RESOURCE_HEALTH ] - p.resources.current[ RESOURCE_HEALTH ] );
      }
    }

    sim->print_log( "{} uses elixir {}.", p.name(), data->name );
  }
  bool ready() override
  {
    if ( !player->sim->allow_flasks )
      return false;

    if ( player->consumables.flask && player->consumables.flask->check() )
      return false;

    assert( data );

    if ( data->type == elixir::BATTLE && player->consumables.battle_elixir &&
         player->consumables.battle_elixir->check() )
    {
      return false;
    }

    if ( data->type == elixir::GUARDIAN && player->consumables.guardian_elixir &&
         player->consumables.guardian_elixir->check() )
    {
      return false;
    }

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

  mana_potion_t( player_t* p, util::string_view options_str )
    : action_t( ACTION_USE, "mana_potion", p ), trigger( 0 ), min( 0 ), max( 0 )
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
    {
      struct potion_entry_t { int level, value; };
      potion_entry_t entries[] = { { 0, 1800 },
                                   { 68, 2400 },
                                   { 80, 4300 },
                                   { 85, 10000 },
                                   { 86, 30001 },};
      for ( const auto& entry : entries )
      {
        min = max = entry.value;
        if ( player->level() > entry.level )
        {
          break;
        }
      }
    }

    assert( max > 0 );

    if ( trigger <= 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  void execute() override
  {
    sim->print_log( "{} uses Mana potion", player->name() );
    double gain_mana = rng().range( min, max );
    player->resource_gain( RESOURCE_MANA, gain_mana, gain );
    player->potion_used = true;
  }

  bool ready() override
  {
    if ( player->potion_used )
      return false;

    if ( ( player->resources.max[ RESOURCE_MANA ] - player->resources.current[ RESOURCE_MANA ] ) < trigger )
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

  health_stone_t( player_t* p, util::string_view options_str ) : heal_t( "health_stone", p ), charges( 3 )
  {
    add_option( opt_float( "pct_heal", base_pct_heal ) );
    parse_options( options_str );

    cooldown->duration = 2_min;
    trigger_gcd = 0_ms;

    harmful = false;
    may_crit = true;

    base_pct_heal = 0.2;

    target = player;
  }

  void reset() override
  {
    charges = 3;
  }

  void execute() override
  {
    assert( charges > 0 );
    --charges;
    heal_t::execute();
  }

  bool ready() override
  {
    if ( charges <= 0 )
      return false;

    if ( !if_expr )
    {
      if ( player->resources.pct( RESOURCE_HEALTH ) > ( 1 - base_pct_heal ) )
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
  flask_base_t( player_t* p, util::string_view name, util::string_view options_str ) : dbc_consumable_base_t( p, name )
  {
    // Backwards compatibility reasons
    parse_options( options_str );

    type = ITEM_SUBCLASS_FLASK;
  }

  bool disabled_consumable() const override
  {
    return dbc_consumable_base_t::disabled_consumable() || !sim->allow_flasks;
  }

  // Figure out the default consumable for flasks
  std::string consumable_default() const override
  {
    if ( !player->flask_str.empty() )
    {
      return player->flask_str;
    }
    // Class module default, if defined
    else if ( !player->default_flask().empty() )
    {
      return player->default_flask();
    }

    return {};
  }

  // Flasks (starting from legion it seems) have a reverse mapping to the spell that creates them in
  // the effect's trigger spell. This confuses the dbc-backed special effect system into thinking
  // that's the triggering spell, so let's override trigger spell here, presuming that the flasks
  // are simple stat buffs.
  special_effect_t* create_special_effect() override
  {
    auto e = dbc_consumable_base_t::create_special_effect();
    e->trigger_spell_id = driver()->id();
    return e;
  }

  void init() override
  {
    dbc_consumable_base_t::init();

    if ( !background )
    {
      player->consumables.flask = consumable_buff;
    }
  }

  void initialize_consumable() override
  {
    dbc_consumable_base_t::initialize_consumable();

    if ( !consumable_buff )
      return;

    // Apply modifiers on the buff stats. Note, presumes that there's a single flask APL line in the profile or bad
    // things will happen, but that's a relatively good presumption.
    if ( auto buff = dynamic_cast<stat_buff_t*>( consumable_buff ) )
    {
      double mul = 1.0;

      auto ep = player->find_soulbind_spell( "Exacting Preparation" );
      if ( ep->ok() )
        mul *= 1.0 + ep->effectN( 1 ).percent();  // While all effects have the same value, effect#1 applies to flasks

      range::for_each( buff->stats, [ mul ]( stat_buff_t::buff_stat_t& s ) {
        s.amount *= mul;
      } );
    }
  }

  bool ready() override
  {
    if ( !player->consumables.flask )
      return false;

    if ( player->consumables.flask && player->consumables.flask->check() )
      return false;

    if ( player->consumables.battle_elixir && player->consumables.battle_elixir->check() )
      return false;

    if ( player->consumables.guardian_elixir && player->consumables.guardian_elixir->check() )
      return false;

    return dbc_consumable_base_t::ready();
  }
};

struct flask_t : public flask_base_t
{
  flask_t( player_t* p, util::string_view options_str ) : flask_base_t( p, "flask", options_str ) {}
};

struct oralius_whispering_crystal_t : public flask_base_t
{
  oralius_whispering_crystal_t( player_t* p, util::string_view options_str )
    : flask_base_t( p, "oralius_whispering_crystal", options_str )
  {}

  const spell_data_t* driver() const override
  {
    return player->find_spell( 176151 );
  }
};

struct crystal_of_insanity_t : public flask_base_t
{
  crystal_of_insanity_t( player_t* p, util::string_view options_str )
    : flask_base_t( p, "crystal_of_insanity", options_str )
  {}

  const spell_data_t* driver() const override
  {
    return player->find_spell( 127230 );
  }
};

// ==========================================================================
// Potions (DBC-backed)
// ==========================================================================

struct potion_t : public dbc_consumable_base_t
{
  timespan_t pre_pot_time;
  bool dynamic_prepot;
  double dur_mod_low, dur_mod_high;  // multiplier for Refined Palate soulbind

  potion_t( player_t* p, util::string_view options_str )
    : dbc_consumable_base_t( p, "potion" ),
      pre_pot_time( 2_s ),
      dynamic_prepot( false ),
      dur_mod_low( 0.0 ),
      dur_mod_high( 0.0 )
  {
    add_option( opt_timespan( "pre_pot_time", pre_pot_time ) );
    add_option( opt_bool( "dynamic_prepot", dynamic_prepot ) );
    parse_options( options_str );

    auto refined = p->find_soulbind_spell( "Refined Palate" );
    if ( refined->ok() )
    {
      dur_mod_low = refined->effectN( 1 ).percent();
      dur_mod_high = refined->effectN( 2 ).percent();
    }

    type = ITEM_SUBCLASS_POTION;
    cooldown = player->get_cooldown( "potion" );

    // Note, potions pretty much have to be targeted to an enemy so expressions still work.
    target = p->target;
  }

  bool disabled_consumable() const override
  {
    return dbc_consumable_base_t::disabled_consumable() || !static_cast<bool>( sim->allow_potions );
  }

  std::string consumable_default() const override
  {
    if ( !player->potion_str.empty() )
    {
      return player->potion_str;
    }
    else if ( !player->default_potion().empty() )
    {
      return player->default_potion();
    }

    return {};
  }

  void initialize_consumable() override
  {
    dbc_consumable_base_t::initialize_consumable();

    // Setup a cooldown duration for the potion
    for ( const item_effect_t& effect : player->dbc->item_effects( item_data->id ) )
    {
      auto cd_grp = effect.cooldown_group;
      auto cd_dur = effect.cooldown_group_duration;

      // fallback to spell cooldown group & duration if item effect data doesn't contain it
      if ( cd_grp <= 0 && cd_dur <= 0 )
      {
        auto s_data = player->find_spell( effect.spell_id );
        if ( s_data->ok() )
        {
          cd_grp = s_data->_category;
          cd_dur = s_data->_category_cooldown;
        }
      }

      if ( cd_grp > 0 && cd_dur > 0 )
      {
        cooldown->duration = timespan_t::from_millis( cd_dur );
        break;
      }
    }

    if ( cooldown->duration == 0_ms )
    {
      throw std::invalid_argument( fmt::format( "No cooldown found for potion '{}'.", item_data->name ) );
    }

    // Sanity check pre-pot time at this time so that it's not longer than the duration of the buff
    if ( consumable_buff )
    {
      pre_pot_time = std::max( 0_ms, std::min( pre_pot_time, consumable_buff->data().duration() ) );
    }
  }

  void adjust_dynamic_prepot_time()
  {
    const auto& apl = player->precombat_action_list;

    auto it = range::find( apl, this );
    if ( it == apl.end() )
      return;

    pre_pot_time = 0_ms;

    std::for_each( it + 1, apl.end(), [ this ]( action_t* a ) {
      if ( a->action_ready() )
      {
        timespan_t delta =
            std::max( std::max( a->base_execute_time.value(), a->trigger_gcd ) * a->composite_haste(), a->min_gcd );
        sim->print_debug( "PRECOMBAT: {} pre-pot timing pushed by {} for {}", consumable_name, delta, a->name() );
        pre_pot_time += delta;

        return a->harmful;
      }
      return false;
    } );

    sim->print_debug( "PRECOMBAT: {} quaffed {}s before combat.", consumable_name, pre_pot_time );
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    // dynamic pre-pot timing adjustment
    if ( dynamic_prepot && !player->in_combat )
      adjust_dynamic_prepot_time();

    // adjust for prepot
    if ( !player->in_combat )
      cd_duration = cooldown->duration - pre_pot_time;

    dbc_consumable_base_t::update_ready( cd_duration );
  }

  // Overwrite with fanciful execution due to prepotting
  void execute() override
  {
    action_t::execute();

    if ( consumable_action )
    {
      consumable_action->execute();
    }

    if ( consumable_buff )
    {
      timespan_t duration = consumable_buff->buff_duration();

      // Adjust for Refined Palate soulbind
      if ( dur_mod_low )
      {
        // TODO: is this really a simple uniform distribution?
        duration *= 1.0 + rng().range( dur_mod_low, dur_mod_high );
      }

      if ( !player->in_combat )
      {
        duration -= pre_pot_time;
      }

      consumable_buff->trigger( duration );
    }
  }
};

// ==========================================================================
// Augmentation runes (Raiding consumable) (DBC-backed)
// ==========================================================================

struct augmentation_t : public dbc_consumable_base_t
{
  augmentation_t( player_t* p, util::string_view options_str ) : dbc_consumable_base_t( p, "augmentation" )
  {
    parse_options( options_str );

    type = ITEM_SUBCLASS_CONSUMABLE_OTHER;
  }

  bool disabled_consumable() const override
  {
    return dbc_consumable_base_t::disabled_consumable() || !static_cast<bool>( sim->allow_augmentations );
  }

  std::string consumable_default() const override
  {
    if ( !player->rune_str.empty() )
    {
      return player->rune_str;
    }
    else if ( !player->default_rune().empty() )
    {
      return player->default_rune();
    }

    return {};
  }

  // Custom driver for now, we don't really want to include the item data for now
  const spell_data_t* driver() const override
  {
    if      ( util::str_in_str_ci( consumable_name, "defiled"        ) ) return player->find_spell( 224001 );
    else if ( util::str_in_str_ci( consumable_name, "focus"          ) ) return player->find_spell( 175457 );
    else if ( util::str_in_str_ci( consumable_name, "hyper"          ) ) return player->find_spell( 175456 );
    else if ( util::str_in_str_ci( consumable_name, "stout"          ) ) return player->find_spell( 175439 );
    else if ( util::str_in_str_ci( consumable_name, "battle_scarred" ) ) return player->find_spell( 270058 );
    else if ( util::str_in_str_ci( consumable_name, "veiled"         ) ) return player->find_spell( 347901 );
    else if ( util::str_in_str_ci( consumable_name, "draconic"       ) ) return player->find_spell( 393438 );
    // NOTE: still using hte old spell for Draconic Augment Rune in spelldata
    else if ( util::str_in_str_ci( consumable_name, "dreambound"     ) ) return player->find_spell( 393438 );
    else if ( util::str_in_str_ci( consumable_name, "crystallized"   ) ) return player->find_spell( 453250 );
    else return spell_data_t::not_found();
  }

  void init() override
  {
    dbc_consumable_base_t::init();

    if ( consumable_buff )
    {
      player->consumables.augmentation = consumable_buff;
    }
  }

  bool ready() override
  {
    if ( !player->consumables.augmentation )
      return false;

    if ( player->consumables.augmentation && player->consumables.augmentation->check() )
      return false;

    return action_t::ready();
  }
};

// ==========================================================================
// Food (DBC-backed)
// ==========================================================================

// Some foods (like Felmouth Frenzy) cannot be inferred from item data directly, so have a manual
// map to bind consumable names to driver spells.
static constexpr auto __manual_food_map = util::make_static_map<util::string_view, unsigned>( {
  { "seafood_magnifique_feast",  87545 },
  { "felmouth_frenzy",          188534 },
  { "lemon_herb_filet",         185736 },
  { "sugarcrusted_fish_feast",  185736 },
  { "fancy_darkmoon_feast",     185786 },
} );

struct food_t : public dbc_consumable_base_t
{
  food_t( player_t* p, util::string_view options_str ) : dbc_consumable_base_t( p, "food" )
  {
    parse_options( options_str );

    type = ITEM_SUBCLASS_FOOD;
  }

  bool disabled_consumable() const override
  {
    if ( player->race == RACE_EARTHEN_HORDE || player->race == RACE_EARTHEN_ALLIANCE )
      return true;

    return dbc_consumable_base_t::disabled_consumable() || !static_cast<bool>( sim->allow_food );
  }

  std::string consumable_default() const override
  {
    if ( !player->food_str.empty() )
    {
      return player->food_str;
    }
    else if ( !player->default_food().empty() )
    {
      return player->default_food();
    }

    return {};
  }

  void init() override
  {
    dbc_consumable_base_t::init();

    if ( consumable_buff )
    {
      player->consumables.food = consumable_buff;
    }
  }

  // For claritys sake, name the food buff with the food name. In HTML reports, it'll actually be
  // something like "Well Fed (buttered_sturgeon)".
  special_effect_t* create_special_effect() override
  {
    auto effect = dbc_consumable_base_t::create_special_effect();

    if ( item_data )
    {
      effect->name_str = util::tokenize_fn( item_data->name );
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
    auto it = __manual_food_map.find( consumable_name );
    if ( it != __manual_food_map.end() )
    {
      return player->find_spell( it->second );
    }

    // Figure out base food buff (the spell you cast from the food item)
    const spell_data_t* driver = dbc_consumable_base_t::driver();
    if ( driver->id() == 0 )
    {
      return driver;
    }

    // Check if the driver is directly registered as a special effect
    if ( auto db_item = unique_gear::find_special_effect_db_item( driver->id() ); db_item.size() )
    {
      return driver;
    }

    // Find the "Well Fed" buff from the base food
    for ( const spelleffect_data_t& effect : driver->effects() )
    {
      // Return the "Well Fed" effect of the food (might not always be named well fed)
      if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_TRIGGER_SPELL &&
           effect.period() == timespan_t::from_millis( 10000 ) )
      {
        return effect.trigger();
      }
    }

    return driver;
  }

  // Initialize the food buff, and if it's a stat buff, apply epicurean on it for Pandaren and exacting preparation
  // soulbind. On action-based or non-stat-buff food, these must be applied manually. Note that even if the food uses a
  // custom initialization, if the resulting buff is a stat buff, modifiers will be automatically applied.
  void initialize_consumable() override
  {
    dbc_consumable_base_t::initialize_consumable();

    if ( !consumable_buff )
      return;

    // Apply modifiers on the buff stats. Note, presumes that there's a single food APL line in the profile or bad
    // things will happen, but that's a relatively good presumption.
    if ( auto buff = dynamic_cast<stat_buff_t*>( consumable_buff ) )
    {
      double mul = 1.0;

      if ( is_pandaren( player->race ) )
        mul *= 2.0;

      // TODO: confirm if these two modifiers are multiplicative or additive
      auto ep = player->find_soulbind_spell( "Exacting Preparation" );
      if ( ep->ok() )
        mul *= 1.0 + ep->effectN( 2 ).percent();  // While all effects have the same value, effect#2 applies to well fed

      range::for_each( buff->stats, [ mul ]( stat_buff_t::buff_stat_t& s ) {
        s.amount *= mul;
      } );
    }
  }

  bool ready() override
  {
    if ( !player->consumables.food )
      return false;

    if ( player->consumables.food && player->consumables.food->check() )
      return false;

    return dbc_consumable_base_t::ready();
  }
};

}  // END UNNAMED NAMESPACE

// ==========================================================================
// DBC-backed consumable base class
// ==========================================================================

dbc_consumable_base_t::dbc_consumable_base_t( player_t* p, std::string_view name_str )
  : action_t( ACTION_USE, name_str, p ),
    item_data( nullptr ),
    type( ITEM_SUBCLASS_CONSUMABLE ),
    consumable_action( nullptr ),
    consumable_buff( nullptr ),
    opt_disabled( false )
{
  add_option( opt_string( "name", consumable_name ) );
  add_option( opt_string( "type", consumable_name ) );

  harmful = callbacks = may_crit = may_miss = false;

  trigger_gcd = timespan_t::zero();

  // Consumables always target the owner
  target = player;
}

std::unique_ptr<expr_t> dbc_consumable_base_t::create_expression( std::string_view name_str )
{
  auto split = util::string_split<util::string_view>( name_str, "." );
  if ( split.size() == 2 && util::str_compare_ci( split[ 0 ], "consumable" ) )
  {
    auto match = util::str_compare_ci( consumable_name, split[ 1 ] );
    return expr_t::create_constant( name_str, match );
  }

  return action_t::create_expression( name_str );
}

void dbc_consumable_base_t::execute()
{
  action_t::execute();

  if ( consumable_action )
  {
    consumable_action->execute();
  }

  if ( consumable_buff )
  {
    consumable_buff->trigger();
  }
}

void dbc_consumable_base_t::init()
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

  if ( !disabled_consumable() )
  {
    item_data = find_consumable( *player->dbc, consumable_name, type );

    try
    {
      initialize_consumable();
    }
    catch ( const std::exception& )
    {
      std::throw_with_nested( std::invalid_argument(
        fmt::format( "Unable to initialize consumable '{}' from '{}'", signature_str, consumable_name ) ) );
    }
  }

  if ( auto con_data = dynamic_cast<consumable_buff_item_data_t*>( consumable_buff ) )
  {
    con_data->item_data = item_data;
  }

  action_t::init();
}

// Find a suitable DBC spell for the consumable. This method is overridable where needed (for
// example custom flasks)
const spell_data_t* dbc_consumable_base_t::driver() const
{
  if ( !item_data )
  {
    return spell_data_t::not_found();
  }

  for ( const item_effect_t& effect : player->dbc->item_effects( item_data->id ) )
  {
    // Note, bypasses level check from the spell itself, since it seems some consumable spells are
    // flagged higher level than the actual food they are in.
    auto ptr = dbc::find_spell( player, effect.spell_id );
    if ( ptr && ptr->id() == effect.spell_id )
    {
      return ptr;
    }
  }

  return spell_data_t::not_found();
}

  // Overridable method to customize the special effect that is used to drive the buff creation for
  // the consumable
special_effect_t* dbc_consumable_base_t::create_special_effect()
{
  auto effect = new special_effect_t( player );
  effect->type = SPECIAL_EFFECT_USE;
  effect->source = SPECIAL_EFFECT_SOURCE_ITEM;

  if ( item_data && item_data->crafting_quality )
  {
    // Dragonflight consumables with crafting quality use the items ilevel for action/buff effect values, so if the
    // item_data has a crafting quality, create an item for the effect to use
    consumable_item = std::make_unique<item_t>( player, "" );
    consumable_item->parsed.data.name = item_data->name;
    consumable_item->parsed.data.id = item_data->id;
    consumable_item->parsed.data.level = item_data->level;
    consumable_item->parsed.data.inventory_type = INVTYPE_TRINKET;  // DF consumables use trinket CR multipliers
    consumable_item->parsed.data.crafting_quality = item_data->crafting_quality;

    effect->item = consumable_item.get();
  }

  return effect;
}

  // Attempts to initialize the consumable. Jumps through quite a few hoops to manage to create
  // special effects only once, if the user input contains multiple consumable lines (as is possible
  // with potions for example).
void dbc_consumable_base_t::initialize_consumable()
{
  if ( driver()->id() == 0 )
  {
    throw std::invalid_argument( "Unable to find consumable." );
  }

  // populate ID and spell data for better reporting
  id = driver()->id();
  s_data_reporting = driver();
  name_str_reporting = s_data_reporting->name_cstr();
  util::tokenize( name_str_reporting );

  auto effect = unique_gear::find_special_effect( player, driver()->id(), SPECIAL_EFFECT_USE );
  // No special effect for this consumable found, so create one
  if ( !effect )
  {
    effect = create_special_effect();
    unique_gear::initialize_special_effect( *effect, driver()->id() );

    // First special effect initialization phase could not deduce a proper consumable to create
    if ( effect->type == SPECIAL_EFFECT_NONE )
    {
      throw std::invalid_argument(
        "First special effect initialization phase could not deduce a proper consumable to create." );
    }

    // Note, this needs to be added before initializing the (potentially) custom special effect,
    // since find_special_effect for this same driver needs to find this newly created special
    // effect, not anything the custom init might create.
    player->special_effects.push_back( effect );

    // Finally, initialize the special effect. If it's a plain old stat buff this does nothing,
    // but some consumables require custom initialization.
    unique_gear::initialize_special_effect_2( effect );
  }

  // And then, grab the action and buff from the special effect, if they are enabled
  // Cooldowns are handled via potion_t::cooldown so we 0 out any on the action/buff
  consumable_action = effect->create_action();
  if ( consumable_action )
  {
    consumable_action->cooldown->duration = 0_ms;
  }

  consumable_buff = effect->create_buff();
  if ( consumable_buff )
  {
    consumable_buff->set_cooldown( 0_ms );
    consumable_buff->s_data_reporting = s_data_reporting;
  }
}

bool dbc_consumable_base_t::ready()
{
  if ( disabled_consumable() )
  {
    return false;
  }

  return action_t::ready();
}

// ==========================================================================
// consumable_t::create_action
// ==========================================================================

action_t* consumable::create_action( player_t* p, std::string_view name, std::string_view options_str )
{
  if ( name == "potion"                   ) return new       potion_t( p, options_str );
  if ( name == "flask" || name == "phial" ) return new        flask_t( p, options_str );
  if ( name == "elixir"                   ) return new       elixir_t( p, options_str );
  if ( name == "food"                     ) return new         food_t( p, options_str );
  if ( name == "health_stone"             ) return new health_stone_t( p, options_str );
  if ( name == "mana_potion"              ) return new  mana_potion_t( p, options_str );
  if ( name == "augmentation"             ) return new augmentation_t( p, options_str );

  // Misc consumables
  if ( name == "oralius_whispering_crystal" ) return new oralius_whispering_crystal_t( p, options_str );
  if ( name == "crystal_of_insanity"        ) return new        crystal_of_insanity_t( p, options_str );

  return nullptr;
}

// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear.hpp"

#include "dbc/racial_spells.hpp"
#include "player/scaling_metric_data.hpp"
#include "sc_enums.hpp"
#include "sim/expressions.hpp"
#include "util/util.hpp"

#include <cctype>
#include <memory>
#include <regex>

#include "simulationcraft.hpp"

using namespace unique_gear;

#define maintenance_check( ilvl ) static_assert( (ilvl) >= 90, "unique item below min level, should be deprecated." )

namespace { // UNNAMED NAMESPACE

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace enchants
{
}

namespace profession
{
}

namespace item
{
}

namespace set_bonus
{
  // Generic passive stat aura adder for set bonuses
  void passive_stat_aura( special_effect_t& );
}

namespace racial
{
  void touch_of_the_grave( special_effect_t& );
  void entropic_embrace( special_effect_t& );
  void brush_it_off( special_effect_t& );
  void zandalari_loa( special_effect_t& );
  void combat_analysis( special_effect_t& );
}

namespace generic
{
  void enable_all_item_effects( special_effect_t& );
}

/**
 * Select attribute operator for buffs. Selects the attribute based on the
 * comparator given (std::greater for example), based on all defined attributes
 * that the stat buff is using. Note that this is for _ATTRIBUTES_ only. Using
 * it for any kind of stats will not work (for now).
 *
 * TODO: Generic way to get "composite" stat_e, so we can extend this class to
 * work on all Blizzard stats.
 */
template<template<typename> class CMP>
struct select_attr
{
  CMP<double> comparator;

  bool operator()( const stat_buff_t& buff ) const
  {
    // Comparing to 0 isn't exactly "correct", however the odds of an actor
    // having zero for all primary attributes is slim to none. If for some
    // reason all checked attributes are 0, the last checked attribute will be
    // the one selected. The order of stats checked is determined by the
    // stat_buff_creator add_stats() calls.
    double compare_to = 0;
    stat_e compare_stat = STAT_NONE;
    stat_e my_stat = STAT_NONE;

    for ( size_t i = 0, end = buff.stats.size(); i < end; i++ )
    {
      if ( this == buff.stats[ i ].check_func.target<select_attr<CMP> >() )
        my_stat = buff.stats[ i ].stat;

      attribute_e stat = static_cast<attribute_e>( buff.stats[ i ].stat );
      double val = buff.player -> get_attribute( stat );
      if ( ! compare_to || comparator( val, compare_to ) )
      {
        compare_to = val;
        compare_stat = buff.stats[ i ].stat;
      }
    }

    return compare_stat == my_stat;
  }
};

std::string suffix( const item_t* item )
{
  assert( item );
  if ( item -> slot == SLOT_OFF_HAND )
    return "_oh";
  return "";
}

std::string tokenized_name( const spell_data_t* data )
{
  return util::tokenize_fn( data -> name_cstr() );
}

// Enchants ================================================================

// Profession perks =========================================================

// TODO: Ratings
[[maybe_unused]] void set_bonus::passive_stat_aura( special_effect_t& effect )
{
  const spell_data_t* spell = effect.player -> find_spell( effect.spell_id );
  stat_e stat = STAT_NONE;
  // Sanity check for stat-giving aura, either stats or aura type 465 ("bonus armor")
  if ( spell -> effectN( 1 ).subtype() != A_MOD_STAT || spell -> effectN( 1 ).subtype() == A_MOD_BONUS_ARMOR )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  if ( spell -> effectN( 1 ).subtype() == A_MOD_STAT )
  {
    if ( spell -> effectN( 1 ).misc_value1() >= 0 )
    {
      stat = static_cast< stat_e >( spell -> effectN( 1 ).misc_value1() + 1 );
    }
    else if ( spell -> effectN( 1 ).misc_value1() == -1 )
    {
      stat = STAT_ALL;
    }
  }
  else
  {
    stat = STAT_BONUS_ARMOR;
  }

  double amount = util::round( spell -> effectN( 1 ).average( effect.player, std::min( MAX_LEVEL, effect.player -> level() ) ) );

  effect.player -> passive.add_stat( stat, amount );
}

// Items ====================================================================

// Blazefury Medallion
// 243988 Driver
// 243991 Damage spell
void item::blazefury_medallion( special_effect_t& effect )
{
  struct blazefury_medallion_t : public generic_proc_t
  {
    blazefury_medallion_t( const special_effect_t& effect )
      : generic_proc_t( effect, "blazefury_medallion", effect.driver()->effectN( 1 ).trigger() )
    {
    }
  };

  struct blazefury_medallion_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    double base_damage;

    blazefury_medallion_cb_t( const special_effect_t& e, action_t* a ) :
      dbc_proc_callback_t( e.player, e ),
      damage( a ),
      base_damage( e.driver()->effectN( 1 ).trigger()->effectN( 1 ).average( e.item ) )
    {
    }

    void execute( const spell_data_t*, player_t* t, action_state_t* s ) override
    {
      if ( s->action->result_is_hit( s->result ) )
      {
        // Currently only uses MH weapon speed, not the speed of the triggering weapon
        // Leaving this in a CB handler just in case this changes at some point via hotfix
        // TOCHECK -- Unclear if this uses equipped weapon speed for Feral or not
        double speed_mod = ( listener->main_hand_weapon.type == WEAPON_NONE ? 2.0 :
                             listener->main_hand_weapon.swing_time.total_seconds() );
        damage->execute_on_target( t, base_damage * speed_mod );
      }
    }
  };

  // if your autoattacks happen to be aoe, this will apply to all targets hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  auto damage = create_proc_action<blazefury_medallion_t>( "blazefury_medallion", effect );
  new blazefury_medallion_cb_t( effect, damage );
}

// Racial
struct touch_of_the_grave_t : public spell_t
{
  touch_of_the_grave_t( player_t* p, const spell_data_t* spell ) :
    spell_t( "touch_of_the_grave", p, spell )
  {
    background = may_crit = true;
    base_dd_min = base_dd_max = 0;
    ap_type = attack_power_type::NO_WEAPON;
    // these are sadly hardcoded in the tooltip
    attack_power_mod.direct = 1.25 * .25;
    spell_power_mod.direct = 1.0 * .25;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap <= sp )
      return 0;
    return spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap > sp )
      return 0;
    return spell_t::spell_direct_power_coefficient( s );
  }
};

void racial::touch_of_the_grave( special_effect_t& effect )
{
  effect.execute_action = new touch_of_the_grave_t( effect.player, effect.trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

void racial::entropic_embrace( special_effect_t& effect )
{
  buff_t* base_buff = buff_t::find( effect.player, "entropic_embrace" );
  if ( base_buff == nullptr )
  {
    base_buff = make_buff( effect.player, "entropic_embrace", effect.trigger() )
      ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
      ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
    effect.player->buffs.entropic_embrace = base_buff;
  }

  effect.custom_buff = base_buff;
  new dbc_proc_callback_t( effect.player, effect );
}

void racial::brush_it_off( special_effect_t& effect )
{
  struct brush_it_off_cb_t : public dbc_proc_callback_t
  {
    action_t* regen;
    double heal_pct;

    brush_it_off_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), heal_pct( e.driver()->effectN( 2 ).percent() )
    {
      regen = new residual_action::residual_periodic_action_t<proc_heal_t>( "brush_it_off", e.player,
                                                                            e.player->find_spell( 291843 ) );
    }

    void execute( const spell_data_t*, player_t*, action_state_t* s ) override
    {
      residual_action::trigger( regen, listener, s->result_amount * heal_pct );
    }
  };

  new brush_it_off_cb_t( effect );
}

struct embrace_of_bwonsamdi_t : public spell_t
{
  embrace_of_bwonsamdi_t(player_t* p, const spell_data_t* sd) :
    spell_t("embrace_of_bwonsamdi", p, sd)
  {
    background = true;
    ap_type = attack_power_type::NO_WEAPON; //TOCHECK: Is this true? Based off of Touch of the Grave right now.
    base_dd_min = base_dd_max = 0;
    //Hardcoded tooltip values
    attack_power_mod.direct = 0.22;
    spell_power_mod.direct = 0.22;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap <= sp )
      return 0;
    return spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap > sp )
      return 0;
    return spell_t::spell_direct_power_coefficient( s );
  }
};

struct embrace_of_kimbul_t : public spell_t
{
  embrace_of_kimbul_t(player_t* p, const spell_data_t* sd) :
    spell_t("embrace_of_kimbul", p, sd)
  {
    tick_may_crit = false; //TOCHECK: Longer test needed with max level character for these values
    background = true;
    hasted_ticks = false;
    dot_max_stack = sd->max_stacks();
    dot_behavior = DOT_REFRESH_DURATION;
    ap_type = attack_power_type::NO_WEAPON;
    attack_power_mod.tick = 0.075; //Hardcoded in tooltip
    spell_power_mod.tick = 0.075;
  }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.tick * s -> composite_attack_power();
    const double sp = spell_power_mod.tick * s -> composite_spell_power();

    if ( ap <= sp )
      return 0;
    return spell_t::attack_tick_power_coefficient( s );
  }

  double spell_tick_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.tick * s -> composite_attack_power();
    const double sp = spell_power_mod.tick * s -> composite_spell_power();

    if ( ap > sp )
      return 0;
    return spell_t::spell_tick_power_coefficient( s );
  }
};

void racial::zandalari_loa( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "embrace_of_paku" } ) )
    return;

  if ( effect.player->zandalari_loa == player_t::AKUNDA )
  {
    // Akunda - Healing Proc (not implemented)
  }
  else if ( effect.player->zandalari_loa == player_t::GONK )
  {
    effect.player->base.stacking_movement_speed_modifier += effect.player->find_spell( 292362 )->effectN( 1 ).percent();
  }
  else if ( effect.player->zandalari_loa == player_t::BWONSAMDI )
  {
    // Bwonsamdi - 100% of damage done is returned as healing (healing not implemented)
    special_effect_t* driver = new special_effect_t( effect.player );
    driver->source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( *driver, 292360 );
    driver->execute_action = new embrace_of_bwonsamdi_t( effect.player, effect.player->find_spell( 292380 ) );

    effect.player->special_effects.push_back( driver );

    new dbc_proc_callback_t( effect.player, *driver );
  }
  else if ( effect.player->zandalari_loa == player_t::KIMBUL )
  {
    // Kimbul - Chance to apply bleed dot, max stack of 3
    special_effect_t* driver = new special_effect_t( effect.player );
    driver->source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( *driver, 292363 );
    driver->execute_action = new embrace_of_kimbul_t( effect.player, effect.player->find_spell( 292473 ) );

    effect.player->special_effects.push_back( driver );

    new dbc_proc_callback_t( effect.player, *driver );
  }
  else if ( effect.player->zandalari_loa == player_t::KRAGWA )
  {
    // Kragwa - Grants health and armor (not implemented)
  }
  else if ( effect.player->zandalari_loa == player_t::PAKU )
  {
    special_effect_t* driver = new special_effect_t( effect.player );
    driver->source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( *driver, 292361 );  // Permanent buff spell id, contains proc data

    // Paku - Grants crit chance
    buff_t* paku = buff_t::find( effect.player, "embrace_of_paku" );
    if ( paku == nullptr )
    {
      // Buff spell data contains duration and amount
      paku = make_buff( effect.player, "embrace_of_paku", effect.player->find_spell( 292463 ) )
        ->set_pct_buff_type_from_data( true );
    }

    driver->custom_buff = paku;

    effect.player->special_effects.push_back( driver );

    new dbc_proc_callback_t( driver->player, *driver );
  }
}

void racial::combat_analysis( special_effect_t& effect )
{
  const spell_data_t* buff_spell = effect.player->find_spell( 312923 );
  effect.stat                    = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );

  buff_t* buff = buff_t::find( effect.player, "combat_analysis" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "combat_analysis", buff_spell )
               ->add_stat( effect.stat, buff_spell->effectN( 1 ).average( effect.player ) );
    buff->set_max_stack( as<int>( buff_spell->effectN( 3 ).base_value() ) );
  }

  effect.player->register_combat_begin( [buff, buff_spell]( player_t* ) {
    make_repeating_event( *buff->sim, buff_spell->effectN( 1 ).period(), [buff]() { buff->trigger(); } );
  } );
}

void generic::enable_all_item_effects( special_effect_t& effect )
{
  if ( !effect.player->sim->enable_all_item_effects )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  struct enable_all_item_effects_t : public action_t
  {
    std::vector<special_effect_t*> action_effects;
    std::vector<special_effect_t*> buff_effects;

    enable_all_item_effects_t( const special_effect_t& e )
      : action_t( action_e::ACTION_USE, "enable_all_item_effects", e.player )
    {
      callbacks = false;
      cooldown->duration = 20_s;
    }

    void init() override
    {
      action_t::init();

      for ( auto id : midnight::__mid_special_effect_ids )
      {
        if ( auto eff = find_special_effect( player, id, SPECIAL_EFFECT_USE ) )
        {
          if ( eff->custom_buff )
            buff_effects.push_back( eff );

          if ( eff->execute_action )
            action_effects.push_back( eff );
        }
      }
    }

    result_e calculate_result( action_state_t* ) const override
    {
      return result_e::RESULT_NONE;
    }

    void execute() override
    {
      action_t::execute();

      for ( auto eff : buff_effects )
        eff->custom_buff->trigger();

      for ( auto eff : action_effects )
        if ( eff->execute_action->action_ready() )
          eff->execute_action->execute();
    }
  };

  effect.execute_action = new enable_all_item_effects_t( effect );
}

bool stat_fits_criteria( stat_e stat, stat_e criteria )
{
  if ( !stat )
    return false;

  if ( criteria == STAT_ALL )
    return true;

  if ( criteria == STAT_ANY_DPS )
    return stat != STAT_LEECH_RATING && stat != STAT_SPEED_RATING && stat != STAT_AVOIDANCE_RATING;

  return stat == criteria;
}

// Figure out if a given generic buff (associated with a trinket/item) is a
// stat buff of the correct type
bool buff_has_stat( const buff_t* buff, stat_e stat )
{
  if ( ! buff )
    return false;

  // Not a stat buff
  const stat_buff_t* stat_buff = dynamic_cast< const stat_buff_t* >( buff );
  if ( ! stat_buff )
    return false;

  // At this point, if "any" was specificed, we're satisfied
  if ( stat == STAT_ALL )
    return true;

  // TODO: Probably needs more cases or potentially a more elegant solution here
  //       Just filter tertiaries for now since they are present on some DPS trinket use effects
  if ( stat == STAT_ANY_DPS )
  {
    return range::any_of( stat_buff->stats, []( auto elem ) {
      return elem.stat != STAT_LEECH_RATING && elem.stat != STAT_SPEED_RATING && elem.stat != STAT_AVOIDANCE_RATING;
    } );
  }

  for ( auto & elem : stat_buff->stats )
  {
    if ( elem.stat == stat )
      return true;
  }

  return false;
}

bool action_has_damage( const action_t* action )
{
  if ( !action )
    return false;

  // check direct damage
  if ( action->does_direct_damage() )
    return true;

  // check periodic damage
  if ( action->does_periodic_damage() )
    return true;

  // check impact action
  if ( action->impact_action && action_has_damage( action->impact_action ) )
    return true;

  // check tick action
  if ( action->tick_action )
  {
    if ( action_has_damage( action->tick_action ) )
      return true;

    // check tick action's impact action
    if ( action->tick_action->impact_action && action_has_damage( action->tick_action->impact_action ) )
      return true;
  }

  return false;
}
} // UNNAMED NAMESPACE

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned iid, util::span<const slot_e> s )
  : targetdata_initializer_t(), item_id( iid ), spell_id( 0 ), slots_( s.begin(), s.end() )
{
  active_fn = []( const special_effect_t* e ) { return e != nullptr; };
  debuff_fn = []( player_t*, const special_effect_t* e ) { return e->trigger(); };
}

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned sid, unsigned did )
  : targetdata_initializer_t(), item_id( 0 ), spell_id( sid )
{
  active_fn = []( const special_effect_t* e ) { return e != nullptr; };

  if ( did )
    debuff_fn = [ did ]( player_t* p, const special_effect_t* ) { return p->find_spell( did ); };
  else
    debuff_fn = []( player_t*, const special_effect_t* e ) { return e->trigger(); };
}

const special_effect_t* item_targetdata_initializer_t::find( player_t* p ) const
{
  if ( spell_id )
  {
    return unique_gear::find_special_effect( p, spell_id );
  }
  else
  {
    for ( slot_e slot : slots_ )
    {
      if ( p->items[ slot ].parsed.data.id == item_id )
      {
        return p->items[ slot ].parsed.special_effects[ 0 ];
      }
    }
  }

  return nullptr;
}

bool item_targetdata_initializer_t::init( player_t* p ) const
{
  // No need to check on pets/enemies
  if ( p->is_pet() || p->is_enemy() || p->type == HEALING_ENEMY )
    return false;

  return targetdata_initializer_t::init( p );
}

const special_effect_t* item_targetdata_initializer_t::effect( actor_target_data_t* td ) const
{
  return data[ td->source ];
}

/**
 * Initialize a special effect, based on a spell id. Returns true if the first
 * phase initialization succeeded, false otherwise. If the spell id points to a
 * spell that our system cannot support, also sets the special effect type to
 * SPECIAL_EFFECT_NONE.
 *
 * Note that the first phase initialization simply fills up special_effect_t
 * with relevant information for non-custom special effects. Second phase of
 * the initialization (performed by unique_gear::init) will instantiate the
 * proc callback, and relevant actions/buffs, or call a custom function to
 * perform the initialization.
 */
void unique_gear::initialize_special_effect( special_effect_t& effect, unsigned spell_id )
{
  player_t* p = effect.player;

  // Perform max level checking on the driver before anything
  const spell_data_t* spell = p->find_spell( spell_id );
  if ( spell->max_aura_level() > 0 && as<unsigned>( p->level() ) > spell->max_aura_level() )
  {
    if ( p->sim->debug )
    {
      p->sim->out_debug.printf( "%s disabled effect %s, player level %d higher than maximum effect level %u", p->name(),
                                spell->name_cstr(), p->level(), spell->max_aura_level() );
    }
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // Try to find the special effect from the custom effect database
  for ( const auto dbitem: find_special_effect_db_item( spell_id ) )
  {
    // Parse auxilary effect options before doing spell data based parsing
    if ( ! dbitem -> encoded_options.empty() )
    {
      std::string encoded_options = dbitem -> encoded_options;
      util::tolower( encoded_options );
      // Note, if the encoding parse fails (this should never ever happen),
      // we don't parse game client data either.
      special_effect::parse_special_effect_encoding( effect, encoded_options );
    }
    else if ( dbitem -> cb_obj )
    {
      // Check that the custom special effect initializer is valid. Invalid special effect
      // validators could be for example spec-specific initializers (see scoped_action_callback_t
      // and child classes derived off of it).
      if ( ! dbitem -> cb_obj -> valid( effect ) )
      {
        continue;
      }

      // Custom special effect initialization is deferred, and no parsing from spell data is done
      // automatically.
      if ( dbitem -> cb_obj )
      {
        effect.custom_init_object.push_back( dbitem -> cb_obj );
      }
    }
  }

  // Setup the driver always, though honoring any parsing performed in the first phase options.
  if ( effect.spell_id == 0 )
    effect.spell_id = spell_id;

  // Check the passive effects database. These are initialized in the first phase since they may affect player base
  // stats
  for ( const auto dbitem : find_passive_effect_db_item( spell_id ) )
  {
    // Check that a custom special effect initializer exists and is valid
    if ( !dbitem->cb_obj || !dbitem->cb_obj->valid( effect ) )
      continue;

    dbitem->cb_obj->initialize( effect );
    // Set as passive so second phase initialization doesn't happen
    effect.type = SPECIAL_EFFECT_PASSIVE;
  }

  // No further processing is necessary for passive effects.
  if ( effect.type == SPECIAL_EFFECT_PASSIVE )
    return;

  // Custom init found a valid initializer callback, this special effect will be initialized with it
  // later on
  if ( !effect.custom_init_object.empty() )
  {
    return;
  }

  // If the item is legendary, and it has an item effect, mandate that the item effect is actually
  // created through custom means. This is to prevent the automatic inference below to create
  // completely nonsensical special effects, when there is not enough client data to fully implement
  // the effect properly.
  //
  // This is mostly relevant for "simple looking" legendary effects such as Recurrent Ritual that
  // gets automatically inferred to affect all (warlock) spells globally.
  if ( effect.custom_init_object.empty() && effect.item &&
       effect.source == SPECIAL_EFFECT_SOURCE_ITEM &&
       effect.item->parsed.data.quality == ITEM_QUALITY_LEGENDARY )
  {
    if ( p -> sim -> debug )
    {
      p -> sim -> out_debug.printf( "Player %s no custom special effect initializer for item %s, "
                                    "spell %s (id=%u), disabling effect",
        p -> name(), effect.item -> name(), p -> find_spell( spell_id ) -> name_cstr(), spell_id );
    }
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // No custom effect found, so ensure that we have spell data for the driver
  if ( p -> find_spell( effect.spell_id ) -> id() != effect.spell_id )
  {
    if ( p -> sim -> debug )
      p -> sim -> out_debug.printf( "Player %s unable to initialize special effect in item %s, spell identifier %u not found.",
          p -> name(), effect.item ? effect.item -> name() : "unknown", effect.spell_id );
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // For generic procs, make sure we have a PPM, RPPM or Proc Chance available,
  // otherwise there's no point in trying to proc anything
  if ( effect.type == SPECIAL_EFFECT_EQUIP )
  {
    if (!special_effect::usable_proc( effect ))
    {
      effect.type = SPECIAL_EFFECT_NONE;
    }
  }

  // For generic use stuff, we need to have a proper buff or action that we can generate
  else if ( effect.type == SPECIAL_EFFECT_USE &&
            effect.buff_type() == SPECIAL_EFFECT_BUFF_NONE &&
            effect.action_type() == SPECIAL_EFFECT_ACTION_NONE )
  {
    effect.type = SPECIAL_EFFECT_NONE;
  }
}

// Second phase initialization, creates the proc callback object for generic on-equip special
// effects, or calls the custom initialization function given in the first phase initialization.
void unique_gear::initialize_special_effect_2( special_effect_t* effect )
{
  if ( effect->type == SPECIAL_EFFECT_PASSIVE )
    return;

  if ( effect -> custom_init || !effect -> custom_init_object.empty() )
  {
    if ( effect -> custom_init )
    {
      effect -> custom_init( *effect );
    }
    else
    {
      range::for_each( effect -> custom_init_object, [ effect ]( scoped_callback_t* cb ) {
        cb -> initialize( *effect );
      } );
    }

    // Allow class modules to adjust the special_effect_t object generated by the core
    // special effect registry, if they want.
    effect->player->init_special_effect( *effect );
  }
  else if ( effect -> type == SPECIAL_EFFECT_EQUIP )
  {
    // Ensure we are not accidentally initializing a generic special effect multiple times
    bool exists = effect -> player -> callbacks.has_callback( [ effect ]( const action_callback_t* cb ) {
      auto dbc_cb = dynamic_cast<const dbc_proc_callback_t*>( cb );
      if ( dbc_cb == nullptr )
      {
        return false;
      }

      // Special effects are unique, and have an 1:1 relationship with (dbc proc) callbacks. Pointer
      // comparison here is enough to ensure that no special_effect_t object gets more than one
      // dbc_proc_callback_t object.
      return &( dbc_cb -> effect ) == effect;
    } );

    if ( exists )
    {
      return;
    }

    // Allow class modules to adjust the special_effect_t object generated by the core
    // special effect registry, if they want.
    effect->player->init_special_effect( *effect );

    if ( effect -> item )
    {
      new dbc_proc_callback_t( effect -> item, *effect );
    }
    else
    {
      new dbc_proc_callback_t( effect -> player, *effect );
    }
  }
}

void unique_gear::initialize_racial_effects( player_t* player )
{
  if ( player->race == RACE_NONE )
  {
    return;
  }

  if ( !util::race_id( player->race ) )
  {
    return;
  }

  // Iterate over all race spells for the player
  for ( const auto* entry : player->dbc->racial_spell( player->type, player->race ) )
  {
    auto spell = dbc::find_spell( player, entry->spell_id );
    if ( !spell || !spell->ok() )
    {
      continue;
    }

    special_effect_t effect( player );
    effect.source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( effect, spell->id() );
    if ( !effect.is_custom() )
    {
      continue;
    }

    player->sim->print_debug( "Player {} initialized racial spell {} (id={}, class_mask={:#08x})",
      player->name(), spell->name_cstr(), spell->id(), entry->mask_class );

    player->special_effects.push_back( new special_effect_t( effect ) );
  }
}

void unique_gear::initialize_expansion_trait_effects( player_t* player, std::string_view talents_str )
{
  if ( !player || talents_str.empty() )
    return;

  for ( auto entry : util::string_split<std::string_view>( talents_str, "/" ) )
  {
    auto split = util::string_split<std::string_view>( entry, ":" );
    auto _trait = split[ 0 ];
    // auto _rank = split.size() > 1 ? split[ 1 ] : "1"; ignored for now

    unsigned spell_id;

    if ( util::is_number( _trait ) )
      spell_id = trait_data_t::find( util::to_unsigned( _trait ), player->is_ptr() )->id_spell;
    else
      spell_id = trait_data_t::find( talent_tree::EXPANSION, _trait, 0, SPEC_NONE, player->is_ptr(), true )->id_spell;

    if ( !spell_id )
      throw sc_invalid_player_argument( fmt::format( "Unable to find expansion talent '{}'.", _trait ) );

    special_effect_t _effect( player );
    _effect.spell_id = spell_id;

    unique_gear::initialize_special_effect( _effect, spell_id );

    player->special_effects.push_back( new special_effect_t( _effect ) );
  }
}

// ==========================================================================
// unique_gear::init
// ==========================================================================

void unique_gear::init( player_t* p )
{
  if ( p->is_pet() || p->is_enemy() )
    return;

  for ( size_t i = 0; i < p->items.size(); i++ )
  {
    item_t& item = p->items[ i ];

    for ( size_t j = 0; j < item.parsed.special_effects.size(); j++ )
    {
      special_effect_t* effect = item.parsed.special_effects[ j ];

      p->sim->print_debug( "Initializing item-based special effect {}", *effect );

      initialize_special_effect_2( effect );
    }
  }

  // Generic special effects, bound to no specific item
  for ( size_t i = 0; i < p->special_effects.size(); i++ )
  {
    special_effect_t* effect = p->special_effects[ i ];

    p->sim->print_debug( "Initializing generic special effect {}", *effect );

    initialize_special_effect_2( effect );
  }
}

// Base class for item effect expressions, finds all the special effects in the
// listed slots
struct item_effect_base_expr_t : public expr_t
{
  std::vector<const special_effect_t*> effects;

  item_effect_base_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    expr_t( full_expression )
  {
    const special_effect_t* e = nullptr;

    for (auto slot : slots)
    {
      e = player.items[ slot ].special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_EQUIP );
      if ( e && e -> source != SPECIAL_EFFECT_SOURCE_NONE )
        effects.push_back( e );

      e = player.items[ slot ].special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
      if ( e && e -> source != SPECIAL_EFFECT_SOURCE_NONE )
        effects.push_back( e );
    }
  }
};

// Base class for expression-based item expressions (such as buff, or cooldown
// expressions). Implements the behavior of expression evaluation.
struct item_effect_expr_t : public item_effect_base_expr_t
{
  std::vector<std::unique_ptr<expr_t>> exprs;

  item_effect_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_base_expr_t( player, slots, full_expression )
  { }

  // Evaluates automatically to the maximum value out of all expressions, may
  // not be wanted in all situations. Best case? We should allow internal
  // operators here somehow
  double evaluate() override
  {
    double result = 0;
    for (auto&& expr : exprs)
    {
      double r = expr -> eval();
      if ( r > result )
        result = r;
    }

    return result;
  }

  bool is_constant() override
  {
    return exprs.empty();
  }
};

// Buff based item expressions, creates buff expressions for the items from user input
struct item_buff_expr_t : public item_effect_expr_t
{
  item_buff_expr_t( player_t& player, const std::vector<slot_e>& slots, stat_e s, bool stacking,
                    util::string_view expr_str )
    : item_effect_expr_t( player, slots, expr_str )
  {
    for ( auto e : effects )
    {
      auto _list = e->buff_list;  // make a copy
      if ( auto _buff = buff_t::find( &player, e->name() ); _buff && !range::contains( _list, _buff ) )
        _list.push_back( _buff );

      for ( auto b : _list )
      {
        if ( buff_has_stat( b, s ) && ( !stacking || ( stacking && b->max_stack() > 1 ) ) )
        {
          if ( auto expr_obj = buff_t::create_expression( b->name(), expr_str, *b ) )
            exprs.push_back( std::move( expr_obj ) );
        }
      }
    }
  }
};

struct item_buff_exists_expr_t : public item_effect_expr_t
{
  double v;

  item_buff_exists_expr_t( player_t& player, const std::vector<slot_e>& slots, stat_e s,
                           util::string_view full_expression )
    : item_effect_expr_t( player, slots, full_expression ), v( 0 )
  {
    for ( auto e : effects )
    {
      auto _list = e->buff_list;  // make a copy
      if ( auto _buff = buff_t::find( &player, e->name() ); _buff && !range::contains( _list, _buff ) )
        _list.push_back( _buff );

      for ( auto b : _list )
      {
        if ( buff_has_stat( b, s ) )
        {
          v = 1;
          break;
        }
      }

      if ( v == 1 )
        break;
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  { return v; }
};

// Cooldown based item expressions, creates cooldown expressions for the items
// from user input
struct item_cooldown_expr_t : public item_effect_expr_t
{
  item_cooldown_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view expr, util::string_view full_expression ) :
    item_effect_expr_t( player, slots, full_expression )
  {
    for (auto e : effects)
    {
      if ( e -> cooldown() != timespan_t::zero() )
      {
        cooldown_t* cd = player.get_cooldown( e -> cooldown_name() );
        if ( auto expr_obj = cd -> create_expression( expr ) )
          exprs.push_back( std::move(expr_obj) );
      }
    }
  }
};

struct item_cast_time_expr_t : public item_effect_expr_t
{
  double v = 0;

  item_cast_time_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_expr_t( player, slots, full_expression ), v( 0 )
  {
    for (auto e : effects)
    {
      if ( e -> execute_action )
      {
        if ( e->execute_action->channeled )
        {
          v = e->execute_action->dot_duration.total_seconds();
        }
        else
        {
          v = e->execute_action->base_execute_time.value().total_seconds();
        }
        break;
      }
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  { return v; }
};

struct item_ready_expr_t : public item_effect_base_expr_t
{
  item_ready_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_base_expr_t( player, slots, full_expression )
  {
  }

  double evaluate() override
  {
    for ( auto e : effects )
    {
      if ( e -> cooldown_group_duration() != timespan_t::zero() )
      {
        cooldown_t* cd = e->player->get_cooldown( e -> cooldown_group_name() );
        if ( !cd -> up() )
          return 0;
      }
      if ( e -> cooldown() != timespan_t::zero() )
      {
        cooldown_t* cd = e->player->get_cooldown( e -> cooldown_name() );
        if ( !cd -> up() )
          return 0;
      }
    }

    return 1;
  }

  bool is_constant() override
  {
    return effects.empty();
  }
};

struct item_is_expr_t : public expr_t
{
  double is = 0;

  item_is_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view item_name )
    : expr_t( "item_is_expr" )
  {
    for ( auto slot : slots )
    {
      if ( player.items[ slot ].name() == item_name )
        is = 1;
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  {
    return is;
  }
};

struct item_lvl_expr_t : public item_effect_expr_t
{
  double ilvl;
  item_lvl_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression )
    : item_effect_expr_t( player, slots, full_expression )
  {
    for (auto slot : slots)
    {
      ilvl = player.items[ slot ].item_level();
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  {
    return ilvl;
  }
};

struct item_cooldown_exists_expr_t : public item_effect_expr_t
{
  double v;

  item_cooldown_exists_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_expr_t( player, slots, full_expression ), v( 0 )
  {
    for (auto e : effects)
    {
      if ( e->cooldown() != timespan_t::zero() )
      {
        v = 1;
        break;
      }
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  { return v; }
};

struct item_has_use_expr_t : public item_effect_expr_t
{
  double v;
  bool has_use;
  bool has_buff;
  bool has_damage;

  item_has_use_expr_t( player_t& player, const std::vector<slot_e>& slots, std::string_view full_expression,
                       bool check_buff, bool check_damage )
    : item_effect_expr_t( player, slots, full_expression ),
      v( 0 ),
      has_use( false ),
      has_buff( false ),
      has_damage( false )
  {
    for ( auto e : effects )
    {
      if ( e->type == SPECIAL_EFFECT_USE )
      {
        has_use = true;
        break;
      }
    }

    if ( check_buff )
    {
      for ( auto e : effects )
      {
        // Check has_use_buff override
        if ( e->has_use_buff_override )
        {
          has_buff = true;
          break;
        }

        // Check if there is a stat set on the special effect
        if ( stat_fits_criteria( e->stat, STAT_ANY_DPS ) )
        {
          has_buff = true;
          break;
        }

        // Check if the special effect has a suitable buff effect
        for ( size_t i = 1, i_end = e->trigger()->effect_count(); i <= i_end; i++ )
        {
          if ( has_buff )
            break;

          const spelleffect_data_t& effect = e->trigger()->effectN( i );
          if ( effect.id() == 0 )
            continue;

          if ( stat_fits_criteria( e->stat_buff_type( effect ), STAT_ANY_DPS ) )
          {
            has_buff = true;
            break;
          }

          // Check if an effect triggers something with a suitable buff effect
          if ( effect.trigger() )
          {
            for ( size_t j = 1, j_end = effect.trigger()->effect_count(); j <= j_end; j++ )
            {
              const spelleffect_data_t& trigger_effect = effect.trigger()->effectN( j );
              if ( trigger_effect.id() == 0 )
                continue;

              if ( stat_fits_criteria( e->stat_buff_type( trigger_effect ), STAT_ANY_DPS ) )
              {
                has_buff = true;
                break;
              }
            }
          }
        }

        // Check if the special effect created a suitable buff
        buff_t* b = buff_t::find( &player, e->name() );
        if ( buff_has_stat( b, STAT_ANY_DPS ) )
        {
          has_buff = true;
          break;
        }
      }
    }

    if ( check_damage )
    {
      for ( auto e : effects )
      {
        // check has_use_damage override
        if ( e->has_use_damage_override )
        {
          has_damage = true;
          break;
        }

        // check if action name exists
        action_t* a = player.find_action( e->name() );
        if ( action_has_damage( a ) )
        {
          has_damage = true;
          break;
        }

        // check any custom execute_action
        if ( action_has_damage( e->execute_action ) )
        {
          has_damage = true;
          break;
        }

        // check any auto-parsed actions
        if ( e->is_offensive_spell_action() || e->is_attack_action() )
        {
          has_damage = true;
          break;
        }
      }
    }

    if ( has_use && ( has_buff || !check_buff ) && ( has_damage || !check_damage ) )
      v = 1;
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  {
    return v;
  }
};

struct item_cooldown_category_expr_t : public item_effect_base_expr_t
{
  item_cooldown_category_expr_t( player_t& player, const std::vector<slot_e>& slots,
                                 util::string_view full_expression )
    : item_effect_base_expr_t( player, slots, full_expression )
  {
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  {
    for ( auto effect : effects )
      if ( auto cd_group = effect->cooldown_group(); cd_group )
        return cd_group;
    return 0.0;
  }
};

/**
 * Create "trinket" expressions, or anything relating to special effects.
 *
 * Note that this method returns zero (nullptr) when it cannot create an
 * expression.  The callee (player_t::create_expression) will handle unknown
 * expression processing.
 *
 * Trinket expressions are of the form:
 * trinket[.1|2|name].(has_|)(stacking_|)proc.<stat>.<buff_expr> OR
 * trinket[.1|2|name].(has_|)cooldown.<cooldown_expr>
 */
std::unique_ptr<expr_t> unique_gear::create_expression( player_t& player, util::string_view name_str )
{
  enum proc_expr_e
  {
    PROC_EXISTS,
    PROC_ENABLED,
    PROC_READY
  };

  enum proc_type_e
  {
    PROC_STAT,
    PROC_STACKING_STAT,
    PROC_COOLDOWN,
  };

  unsigned int ptype_idx = 1;
  unsigned int stat_idx = 2;
  unsigned int expr_idx = 3;
  enum proc_expr_e pexprtype = PROC_ENABLED;
  enum proc_type_e ptype = PROC_STAT;
  stat_e stat = STAT_NONE;
  std::vector<slot_e> slots;

  auto splits = util::string_split<util::string_view>( name_str, "." );

  // Hyperthread Wristwraps
  if ( splits[ 0 ] == "hyperthread_wristwraps" )
  {
    if ( auto a = player.find_action( "hyperthread_wristwraps" ) )
    {
      return a->create_expression( name_str );
    }
  }

  if ( splits.size() < 2 )
  {
    return nullptr;
  }

  if ( util::is_number( splits[ 1 ] ) )
  {
    if ( splits[ 1 ] == "1" )
    {
      slots.push_back( SLOT_TRINKET_1 );
    }
    else if ( splits[ 1 ] == "2" )
    {
      slots.push_back( SLOT_TRINKET_2 );
    }
    else
      return nullptr;
    ptype_idx++;

    stat_idx++;
    expr_idx++;
  }
  // Try to find trinket.<trinketname>
  else if ( !splits[ 1 ].empty() )
  {
    auto item = player.find_item_by_name( splits[ 1 ] );
    if ( item && ( item->slot == SLOT_TRINKET_1 || item->slot == SLOT_TRINKET_2 ) )
    {
      slots.push_back( item->slot );
    }
    // If the item is not found, return an always false expression instead of erroring out
    else
    {
      return expr_t::create_constant( "trinket-named-expr", 0 );
    }

    ptype_idx++;

    stat_idx++;
    expr_idx++;
  }
  // No positional parameter given so check both trinkets
  else
  {
    slots.push_back( SLOT_TRINKET_1 );
    slots.push_back( SLOT_TRINKET_2 );
  }

  if ( splits.size() <= ptype_idx )
  {
    throw std::invalid_argument(
      fmt::format( "'{}' parts required, only '{}' provided.", ptype_idx + 1, splits.size() ) );
  }

  if ( util::str_compare_ci( splits[ ptype_idx ], "is" ) )
  {
    return std::make_unique<item_is_expr_t>( player, slots, splits[ expr_idx - 1 ] );
  }

  if ( util::str_compare_ci( splits[ ptype_idx ], "ilvl" ) )
  {
    return std::make_unique<item_lvl_expr_t>( player, slots, name_str );
  }

  if ( util::str_prefix_ci( splits[ ptype_idx ], "has_use" ) )
  {
    bool check_buff = false;
    bool check_damage = false;

    if ( util::str_in_str_ci( splits[ ptype_idx ], "buff" ) )
      check_buff = true;
    else if ( util::str_in_str_ci( splits[ ptype_idx ], "damage" ) )
      check_damage = true;

    return std::make_unique<item_has_use_expr_t>( player, slots, name_str, check_buff, check_damage );
  }

  if ( util::str_compare_ci( splits[ ptype_idx ], "cast_time" ) )
  {
    return std::make_unique<item_cast_time_expr_t>( player, slots, name_str );
  }

  if ( util::str_prefix_ci( splits[ ptype_idx ], "has_" ) )
    pexprtype = PROC_EXISTS;
  else if ( util::str_prefix_ci( splits[ ptype_idx ], "ready_" ) )
    pexprtype = PROC_READY;

  if ( util::str_in_str_ci( splits[ ptype_idx ], "cooldown" ) )
  {
    ptype = PROC_COOLDOWN;
    // Cooldowns dont have stat type for now
    expr_idx--;
  }

  if ( util::str_in_str_ci( splits[ ptype_idx ], "stacking_" ) )
    ptype = PROC_STACKING_STAT;

  if ( ptype != PROC_COOLDOWN )
  {
    if ( splits.size() <= stat_idx )
    {
      throw std::invalid_argument(
        fmt::format( "'{}' parts required, only '{}' provided.", stat_idx + 1, splits.size() ) );
    }
    // Use "all stat" to indicate "any" ..
    if ( util::str_compare_ci( splits[ stat_idx ], "any" ) )
      stat = STAT_ALL;
    else if ( util::str_compare_ci( splits[ stat_idx ], "any_dps" ) )
      stat = STAT_ANY_DPS;
    else
    {
      stat = util::parse_stat_type( splits[ stat_idx ] );
      if ( stat == STAT_NONE )
      {
        throw std::invalid_argument( fmt::format( "Invalid stat '{}'.", splits[ stat_idx ] ) );
      }
    }
  }

  if ( pexprtype == PROC_ENABLED && ptype != PROC_COOLDOWN && splits.size() >= 4 )
  {
    if ( splits.size() <= expr_idx )
    {
      throw std::invalid_argument(
        fmt::format( "'{}' parts required, only '{}' provided.", expr_idx + 1, splits.size() ) );
    }
    return std::make_unique<item_buff_expr_t>( player, slots, stat, ptype == PROC_STACKING_STAT, splits[ expr_idx ] );
  }
  else if ( pexprtype == PROC_ENABLED && ptype == PROC_COOLDOWN && splits.size() >= 3 )
  {
    return std::make_unique<item_cooldown_expr_t>( player, slots, splits[ expr_idx ], name_str );
  }
  else if ( pexprtype == PROC_EXISTS )
  {
    if ( ptype != PROC_COOLDOWN )
    {
      return std::make_unique<item_buff_exists_expr_t>( player, slots, stat, name_str );
    }
    else
    {
      return std::make_unique<item_cooldown_exists_expr_t>( player, slots, name_str );
    }
  }
  else if ( pexprtype == PROC_READY )
  {
    return std::make_unique<item_ready_expr_t>( player, slots, name_str );
  }

  if ( util::str_compare_ci (splits[ ptype_idx ], "cooldown_category" ) )
    return std::make_unique<item_cooldown_category_expr_t>( player, slots, name_str );

  throw std::invalid_argument( fmt::format( "Invalid unique gear expression '{}'.", splits.back() ) );
}

namespace unique_gear
{
  void proc_resource_t::__initialize()
  {
    may_miss = may_dodge = may_parry = may_block = harmful = false;
    target = player;

    for ( size_t i = 1; i <= data().effect_count(); i++ )
    {
      const spelleffect_data_t& eff = data().effectN( i );
      if ( eff.type() == E_ENERGIZE )
      {
        gain_da = eff.average( item );
        gain_resource = eff.resource_gain_type();
      }
      else if ( eff.type() == E_APPLY_AURA && eff.subtype() == A_PERIODIC_ENERGIZE )
      {
        gain_ta = eff.average( item );
        gain_resource = eff.resource_gain_type();
      }
    }

    gain = player->get_gain(name());
  }

std::vector<special_effect_db_item_t> __special_effect_db, __fallback_effect_db, __passive_effect_db;

bool class_scoped_callback_t::valid(const special_effect_t& effect) const
{
  assert(effect.player);

  if (!class_.empty() && range::find(class_, effect.player->type) == class_.end())
  {
    return false;
  }

  if (!spec_.empty() && range::find(spec_, effect.player->specialization()) == spec_.end())
  {
    return false;
  }

  return true;
}

void proc_attack_t::override_data(const special_effect_t& e)
{
  super::override_data(e);

  if ((e.override_result_es_mask & RESULT_DODGE_MASK))
  {
    this->may_dodge = e.result_es_mask & RESULT_DODGE_MASK;
  }

  if ((e.override_result_es_mask & RESULT_PARRY_MASK))
  {
    this->may_parry = e.result_es_mask & RESULT_PARRY_MASK;
  }
}

} // unique_gear

wrapper_callback_t::wrapper_callback_t( custom_cb_t cb_, wowv_t min_, wowv_t max_ )
  : scoped_callback_t(), cb( std::move( cb_ ) ), min_build( min_ ), max_build( max_ )
{}

bool wrapper_callback_t::valid( const special_effect_t& effect ) const
{
  return effect.player->dbc->wowv() >= min_build && effect.player->dbc->wowv() < max_build;
}

void wrapper_callback_t::initialize( special_effect_t& effect )
{
  cb( effect );
}

static unique_gear::special_effect_set_t do_find_special_effect_db_item(
    const std::vector<special_effect_db_item_t>& db, unsigned spell_id )
{
  special_effect_set_t entries;

  auto it = range::lower_bound( db, spell_id, {}, &special_effect_db_item_t::spell_id );

  if ( it == db.end() || it -> spell_id != spell_id )
  {
    return { };
  }

  while ( it != db.end() && it -> spell_id == spell_id )
  {
    // If there's an encoded option string, just return it straight up
    if ( ! it -> encoded_options.empty() )
    {
      return { &( *it ) };
    }

    assert( it -> cb_obj );

    // Push all callback-based initializers of the same priority into the vector
    if ( entries.empty() || it -> cb_obj -> priority == entries.front() -> cb_obj -> priority )
    {
      entries.push_back( &( *it ) );
    }
    else
    {
      break;
    }

    it++;
  }

  return entries;
}

static special_effect_set_t find_fallback_effect_db_item( unsigned spell_id )
{ return do_find_special_effect_db_item( __fallback_effect_db, spell_id ); }

special_effect_set_t unique_gear::find_special_effect_db_item( unsigned spell_id )
{ return do_find_special_effect_db_item( __special_effect_db, spell_id ); }

special_effect_set_t unique_gear::find_passive_effect_db_item( unsigned spell_id )
{ return do_find_special_effect_db_item( __passive_effect_db, spell_id ); }

void unique_gear::add_effect( const special_effect_db_item_t& dbitem )
{
  // Passive special effects are processed during first phase initialization so aren't added to __special_effect_db if
  // they have a custom initializer.
  if ( dbitem.passive && dbitem.cb_obj )
    __passive_effect_db.push_back( dbitem );
  else
    __special_effect_db.push_back( dbitem );

  if ( dbitem.fallback )
    __fallback_effect_db.push_back( dbitem );
}

void unique_gear::register_special_effect( unsigned spell_id, custom_cb_t init_callback, bool fallback, bool passive,
                                           wowv_t min_build, wowv_t max_build )
{
  special_effect_db_item_t dbitem;
  dbitem.spell_id = spell_id;
  dbitem.cb_obj = new wrapper_callback_t( std::move( init_callback ), min_build, max_build );
  dbitem.fallback = fallback;
  dbitem.passive = passive;
  add_effect( dbitem );
}

void unique_gear::register_special_effect( std::initializer_list<unsigned> spell_ids, custom_cb_t init_callback,
                                           bool fallback, bool passive, wowv_t min_build, wowv_t max_build )
{
  for ( auto id : spell_ids )
    register_special_effect( id, init_callback, fallback, passive, min_build, max_build );
}

void unique_gear::register_special_effect( unsigned spell_id, const char* encoded_str )
{
  special_effect_db_item_t dbitem;
  dbitem.spell_id = spell_id;
  dbitem.encoded_options = encoded_str;

  __special_effect_db.push_back( dbitem );
}

bool unique_gear::create_fallback_buffs( const special_effect_t& effect, const std::vector<util::string_view>& names )
{
  if ( effect.source != SPECIAL_EFFECT_SOURCE_FALLBACK )
    return false;

  for ( auto name : names )
    buff_t::make_fallback( effect.player, name, effect.player );

  return true;
}

void unique_gear::init_feast( special_effect_t& effect, std::initializer_list<std::pair<stat_e, int>> stat_map )
{
  effect.stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );

  for ( auto&& stat : stat_map )
  {
    if ( stat.first == effect.stat )
    {
      effect.trigger_spell_id = stat.second;
      break;
    }
  }
  effect.stat_amount = effect.player->find_spell( effect.trigger_spell_id )->effectN( 1 ).average( effect.player );
}

void unique_gear::DISABLED_EFFECT( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_NONE;
}

/**
 * Master list of special effects in Simulationcraft.
 *
 * This list currently contains custom procs and procs where game client data
 * is either incorrect (so we can override values), or incomplete (so we can
 * help the automatic creation process on the simc side).
 *
 * Each line in the array corresponds to a specific spell (a proc driver spell,
 * or an "on use" spell) in World of Warcraft. There are several sources for
 * special effects:
 * 1) Items (Use, Equip, Chance on hit)
 * 2) Enchants, and profession specific enchants
 * 3) Engineering special effects (tinkers, ranged enchants)
 * 4) Gems
 *
 * Blizzard does not discriminate between the different types, nor do we
 * anymore. Each spell can be mapped to a special effect in the simc client.
 * Each special effect is fed to a new proc callback object
 * (dbc_proc_callback_t) that handles the initialization of the proc, and in
 * generic proc cases, the initialization of the buffs/actions.
 *
 * Each entry contains three fields:
 * 1) The spell ID of the effect. You can find these from third party websites
 *    by clicking on the generated link in item tooltip.
 * 2) Currently a c-string of "additional options" given for a special effect.
 *    This includes the forementioned fixes of incorrect values, and "help" to
 *    drive the automatic special effect generation process. Case insensitive.
 * 3) A callback to a custom initialization function. The function is of the
 *    form: void custom_function_of_awesome( special_effect_t& effect,
 *                                           const item_t& item,
 *                                           const special_effect_db_item_t& dbitem )
 *    Where 'effect' is the effect being created, 'item' is the item that has
 *    the special effect, and 'dbitem' is the database entry itself.
 *
 * Now, special effect creation in this new system is currently a two phase
 * process. First, the special_effect_t struct for the special effect is filled
 * with enough information to initialize the proc (for generic procs, a driver
 * spell id is sufficient), and any options given in this list (through the
 * additional options). For custom special effects, the first phase simply
 * creates a stub special_effect_t object, and no game client data is processed
 * at this time.
 *
 * The second phase of the creation process is responsible for instantiating
 * the necessary action_callback_t object (simc procs), and whatever buffs, or
 * actions are required for the proc. This is also when custom callbacks get
 * called.
 *
 * Note: The special effect initialization process is now unified for all types
 * of special effects, we no longer discriminate between item, enchant, tinker,
 * or gem based special effects.
 *
 * Note2: Enchants, addons, and possibly gems will have a separate translation
 * table in sc_enchant.cpp that maps "user given" names of enchants
 * (enchant=dancing_steel), to in game data, so we can properly initialize the
 * correct spells here. Most of the enchants etc., are automatically
 * identified.  The table will only have the "non standard" user strings we
 * currently use, and whatever else we will use in the future.
 */
void unique_gear::register_special_effects()
{
  /* Legacy Effects, pre-5.0 */
  register_special_effect( 45481,  "ProcOn/hit_45479Trigger"            ); /* Shattered Sun Pendant of Acumen */
  register_special_effect( 45482,  "ProcOn/hit_45480Trigger"            ); /* Shattered Sun Pendant of Might */
  register_special_effect( 45483,  "ProcOn/hit_45431Trigger"            ); /* Shattered Sun Pendant of Resolve */
  register_special_effect( 45484,  "ProcOn/hit_45478Trigger"            ); /* Shattered Sun Pendant of Restoration */
  register_special_effect( 57345,  item::darkmoon_card_greatness        );
  register_special_effect( 71519,  item::deathbringers_will             );
  register_special_effect( 71562,  item::deathbringers_will             );
  register_special_effect( 71892,  item::heartpierce                    );
  register_special_effect( 71880,  item::heartpierce                    );
  register_special_effect( 72413,  "10%"                                ); /* ICC Melee Ring */
  register_special_effect( 96976,  item::matrix_restabilizer            ); /* Matrix Restabilizer */
  register_special_effect( 107824, "1Tick_108016Trigger_20Dur"          ); /* Kiril, Fury of Beasts */
  register_special_effect( 109862, "1Tick_109860Trigger_20Dur"          ); /* Kiril, Fury of Beasts */
  register_special_effect( 109865, "1Tick_109863Trigger_20Dur"          ); /* Kiril, Fury of Beasts */
  register_special_effect( 107995, item::vial_of_shadows                );
  register_special_effect( 109725, item::vial_of_shadows                );
  register_special_effect( 109722, item::vial_of_shadows                );
  register_special_effect( 108006, item::cunning_of_the_cruel           );
  register_special_effect( 109799, item::cunning_of_the_cruel           );
  register_special_effect( 109801, item::cunning_of_the_cruel           );
  register_special_effect( 243988, item::blazefury_medallion            ); /* Kazzak Neck */

  /* Misc effects */
  register_special_effect( 188534, item::felmouth_frenzy                );

  /* Warlords of Draenor 6.2 */
  register_special_effect( 184270, item::mirror_of_the_blademaster      );
  register_special_effect( 184291, item::soul_capacitor                 );
  register_special_effect( 183942, item::insatiable_hunger              );
  register_special_effect( 184066, item::prophecy_of_fear               );
  register_special_effect( 183951, item::unblinking_gaze_of_sethe       );
  register_special_effect( 184249, item::discordant_chorus              );
  register_special_effect( 184257, item::empty_drinking_horn            );
  register_special_effect( 184767, item::tyrants_decree                 );
  register_special_effect( 184762, item::warlords_unseeing_eye          );
  register_special_effect( 201404, item::gronntooth_war_horn            );
  register_special_effect( 201407, item::infallible_tracking_charm      );
  register_special_effect( 201409, item::orb_of_voidsight               );
  register_special_effect( 429257, item::witherbarks_branch             );

  /* Warlords of Draenor 6.0 */
  register_special_effect( 177085, item::blackiron_micro_crucible       );
  register_special_effect( 177071, item::humming_blackiron_trigger      );
  register_special_effect( 177104, item::battering_talisman_trigger     );
  register_special_effect( 177098, item::forgemasters_insignia          );
  register_special_effect( 177090, item::autorepairing_autoclave        );
  register_special_effect( 177171, item::spellbound_runic_band          );
  register_special_effect( 177163, item::spellbound_solium_band         );

  /* Mists of Pandaria: 5.4 */
  register_special_effect( 146195, item::flurry_of_xuen                 );
  register_special_effect( 146197, item::essence_of_yulon               );

  register_special_effect( 146219, "ProcOn/Hit"                         ); /* Yu'lon's Bite */
  register_special_effect( 146251, "ProcOn/Hit"                         ); /* Thok's Tail Tip (Str proc) */

  register_special_effect( 145955, item::readiness                      );
  register_special_effect( 146019, item::readiness                      );
  register_special_effect( 146025, item::readiness                      );
  register_special_effect( 146051, item::amplification, false, true     );
  register_special_effect( 146136, item::cleave                         );

  register_special_effect( 146183, item::black_blood_of_yshaarj         );
  register_special_effect( 146286, item::skeers_bloodsoaked_talisman    );
  register_special_effect( 146315, item::prismatic_prison_of_pride      );
  register_special_effect( 146047, item::purified_bindings_of_immerseus );
  register_special_effect( 146251, item::thoks_tail_tip                 );

  /* Mists of Pandaria: 5.2 */
  register_special_effect( 139116, item::rune_of_reorigination          );
  register_special_effect( 138957, item::spark_of_zandalar              );
  register_special_effect( 138964, item::unerring_vision_of_leishen     );

  register_special_effect( 138728, "Reverse"                            ); /* Steadfast Talisman of the Shado-Pan Assault */
  register_special_effect( 138701, "ProcOn/Hit"                         ); /* Brutal Talisman of the Shado-Pan Assault */
  register_special_effect( 138700, "ProcOn/Hit"                         ); /* Vicious Talisman of the Shado-Pan Assault */
  register_special_effect( 139171, "ProcOn/Crit_RPPMAttackCrit"         ); /* Gaze of the Twins */
  register_special_effect( 138757, "1Tick_138737Trigger"                ); /* Renataki's Soul Charm */
  register_special_effect( 138790, "ProcOn/Hit_1Tick_138788Trigger"     ); /* Wushoolay's Final Choice */
  register_special_effect( 138758, "1Tick_138760Trigger"                ); /* Fabled Feather of Ji-Kun */
  register_special_effect( 139134, "ProcOn/Crit_RPPMSpellCrit"          ); /* Cha-Ye's Essence of Brilliance */

  register_special_effect( 138865, "ProcOn/Dodge"                       ); /* Delicate Vial of the Sanguinaire */

  /* Mists of Pandaria: 5.0 */
  register_special_effect( 126650, "ProcOn/Hit"                         ); /* Terror in the Mists */
  register_special_effect( 126658, "ProcOn/Hit"                         ); /* Darkmist Vortex */

  /* Mists of Pandaria: Dungeon */
  register_special_effect( 126473, "ProcOn/Hit"                         ); /* Vision of the Predator */
  register_special_effect( 126516, "ProcOn/Hit"                         ); /* Carbonic Carbuncle */
  register_special_effect( 126482, "ProcOn/Hit"                         ); /* Windswept Pages */
  register_special_effect( 126490, "ProcOn/Crit"                        ); /* Searing Words */

  /* Mists of Pandaria: Player versus Player */
  register_special_effect( 126706, "ProcOn/Hit"                         ); /* Gladiator's Insignia of Dominance */

  /* Mists of Pandaria: Darkmoon Faire */
  register_special_effect( 128990, "ProcOn/Hit"                         ); /* Relic of Yu'lon */
  register_special_effect( 128445, "ProcOn/Crit"                        ); /* Relic of Xuen (agi) */

  /* Timewalking */
  register_special_effect( 96963, item::necromantic_focus               ); // Firelands Timewalking Trinket
  register_special_effect( 91003, item::sorrowsong                      ); // Lost City of Tol'vir Timewalking Trinket

  /**
   * Enchants
   */
  register_special_effect( { 44797, 55275, 55344 }, enchants::meta_gem_effect, false, true );

  /* The Burning Crusade */
  register_special_effect(  28093, "1PPM"                               ); /* Mongoose */

  /* Wrath of the Lich King */
  register_special_effect(  59620, "1PPM"                               ); /* Berserking */
  register_special_effect(  42976, enchants::executioner                );

  /* Cataclysm */
  register_special_effect(  94747, enchants::hurricane_spell            );
  register_special_effect(  74221, "1PPM"                               ); /* Hurricane Weapon */
  register_special_effect(  74245, "1PPM"                               ); /* Landslide */

  /* Mists of Pandaria */
  register_special_effect( 118333, enchants::dancing_steel              );
  register_special_effect( 142531, enchants::dancing_steel              ); /* Bloody Dancing Steel */
  register_special_effect( 120033, enchants::jade_spirit                );
  register_special_effect( 141178, enchants::jade_spirit                );
  register_special_effect( 104561, enchants::windsong                   );
  register_special_effect( 104428, "rppmhaste"                          ); /* Elemental Force */
  register_special_effect( 104441, enchants::rivers_song                );
  register_special_effect( 118314, enchants::colossus                   );

  /* Warlords of Draenor */
  register_special_effect( 159239, enchants::mark_of_the_shattered_hand );
  register_special_effect( 159243, enchants::mark_of_the_thunderlord    );
  register_special_effect( 159682, enchants::mark_of_warsong            );
  register_special_effect( 159683, enchants::mark_of_the_frostwolf      );
  register_special_effect( 159685, enchants::mark_of_blackrock          );
  register_special_effect( 156059, enchants::megawatt_filament          );
  register_special_effect( 156052, enchants::oglethorpes_missile_splitter );
  register_special_effect( 173286, enchants::hemets_heartseeker         );
  register_special_effect( 173321, enchants::mark_of_bleeding_hollow    );

  /* Engineering enchants */
  register_special_effect( 177708, "1PPM_109092Trigger"                 ); /* Mirror Scope */
  register_special_effect( 177707, "1PPM_109085Trigger"                 ); /* Lord Blastingtons Scope of Doom */
  register_special_effect(  95713, "1PPM_95712Trigger"                  ); /* Gnomish XRay */
  register_special_effect(  99622, "1PPM_99621Trigger"                  ); /* Flintlocks Woodchucker */

  /* Profession perks */
  register_special_effect( 105574, profession::zen_alchemist_stone      ); /* Zen Alchemist Stone (stat proc) */
  register_special_effect( 157136, profession::draenor_philosophers_stone ); /* Draenor Philosopher's Stone (stat proc) */
  register_special_effect(  55004, profession::nitro_boosts             );
  register_special_effect(  82626, profession::grounded_plasma_shield   );

  /**
   * Gems
   */

  // TODO: check why 20% PPM and not 100% from spell data?
  register_special_effect(  39958, "0.2PPM"                             ); /* Thundering Skyfire */
  register_special_effect(  55380, "0.2PPM"                             ); /* Thundering Skyflare */

  /* Generic special effects begin here */

  /* Racial special effects */
  register_special_effect( 5227,   racial::touch_of_the_grave );
  register_special_effect( 255669, racial::entropic_embrace );
  register_special_effect( 291628, racial::brush_it_off );
  register_special_effect( 292751, racial::zandalari_loa, true );
  register_special_effect( 312923, racial::combat_analysis );

  /* Generic "global scope" special effects */
  register_special_effect( 63604, generic::enable_all_item_effects );
}

void unique_gear::unregister_special_effects()
{
  for ( auto& dbitem : __special_effect_db )
    delete dbitem.cb_obj;

  for ( auto& dbitem : __passive_effect_db )
    delete dbitem.cb_obj;
  }

action_t* unique_gear::create_action( player_t* player, util::string_view name, util::string_view options )
{
  return nullptr;
}

void unique_gear::register_hotfixes() {}

void unique_gear::register_target_data_initializers( sim_t* sim ) {}

void unique_gear::register_actor_initializers( sim_t& sim ) {}

std::vector<special_effect_t*> unique_gear::find_special_effects( player_t* p, unsigned id, special_effect_e type )
{
  std::vector<special_effect_t*> effects;

  for ( auto e : p->special_effects )
  {
    if ( e->spell_id == id && ( type == SPECIAL_EFFECT_NONE || type == e->type ) )
    {
      effects.push_back( e );
    }
  }

  for ( const auto& item : p->items )
  {
    for ( auto e : item.parsed.special_effects )
    {
      if ( e->spell_id == id && ( type == SPECIAL_EFFECT_NONE || type == e->type ) )
      {
        effects.push_back( e );
      }
    }
  }

  return effects;
}

special_effect_t* unique_gear::find_special_effect( player_t* p, unsigned id, special_effect_e type )
{
  auto effects = unique_gear::find_special_effects( p, id, type );

  return effects.empty() ? nullptr : effects.front();
}

// Some special effects may use fallback initializers, where the fallback initializer is called if
// the special effect is not found on the actor. Typical cases include anything relating to
// class-specific special effects, where buffs for example should be unconditionally created for the
// actor. This method is called after the normal special effect initialization process finishes on
// the actor.
void unique_gear::initialize_special_effect_fallbacks( player_t* actor )
{
  special_effect_t fallback_effect( actor );

  // Generate an unique list of fallback spell ids
  std::vector<unsigned> fallback_ids;
  range::for_each( __fallback_effect_db, [ &fallback_ids ]( const special_effect_db_item_t& elem ) {
    if ( range::find( fallback_ids, elem.spell_id ) == fallback_ids.end() )
    {
      fallback_ids.push_back( elem.spell_id );
    }
  });

  // Check all fallback ids
  for ( auto fallback_id : fallback_ids )
  {
    // Actor already has a special effect with the fallback id, so don't do anything
    if ( find_special_effect( actor, fallback_id ) )
    {
      continue;
    }

    fallback_effect.reset();
    fallback_effect.spell_id = fallback_id;
    // TODO: Is this really needed?
    fallback_effect.source = SPECIAL_EFFECT_SOURCE_FALLBACK;
    fallback_effect.type = SPECIAL_EFFECT_FALLBACK;

    // Get all registered fallback effects for the spell (fallback) id
    auto dbitems = find_fallback_effect_db_item( fallback_id );
    // .. nothing found, continue
    if ( dbitems.empty() )
    {
      continue;
    }

    // For all registered fallback effects
    for ( const auto& dbitem: dbitems )
    {
      // Ensure that the fallback effect is actually valid for the special effect (actor)
      if ( ! dbitem -> cb_obj -> valid( fallback_effect ) )
      {
        continue;
      }

      fallback_effect.custom_init_object.push_back( dbitem -> cb_obj );
    }

    if ( !fallback_effect.custom_init_object.empty() )
    {
      actor -> special_effects.push_back( new special_effect_t( fallback_effect ) );
    }
  }
}

namespace
{
bool cmp_special_effect( const special_effect_db_item_t& a, const special_effect_db_item_t& b )
{
  if ( &a == &b )
    return false;

  if ( a.spell_id == b.spell_id )
  {
    if ( ! a.encoded_options.empty() && b.encoded_options.empty() )
    {
      return true;
    }
    else if ( a.encoded_options.empty() && ! b.encoded_options.empty() )
    {
      return false;
    }
    else if ( ! a.encoded_options.empty() && ! b.encoded_options.empty() )
    {
      return a.encoded_options < b.encoded_options;
    }

    assert( a.cb_obj && b.cb_obj );
    // Note, descending priority order
    return a.cb_obj -> priority > b.cb_obj -> priority;
  }

  return a.spell_id < b.spell_id;
}

} // unnamed namespace ends

void unique_gear::sort_special_effects()
{
  range::sort( __special_effect_db, cmp_special_effect );
  range::sort( __fallback_effect_db, cmp_special_effect );
  range::sort( __passive_effect_db, cmp_special_effect );
}

bool unique_gear::has_role_mult( player_t* player, const spell_data_t* s_data )
{
  // Failsafe if driver is spell_data_t::nil() or spell_data_t::not_found()
  if ( !s_data->ok() )
    return false;

  auto vars = player->dbc->spell_desc_vars( s_data->id() ).desc_vars();
  if( !vars )
    return false;

  std::cmatch m;
  std::regex get_var( R"(\$(?:healing)?rolemult=\$(.*))" );

  return std::regex_search( vars, m, get_var );
}

bool unique_gear::has_role_mult( const special_effect_t& effect )
{
  return has_role_mult( effect.player, effect.driver() );
}

double unique_gear::role_mult( player_t* player, const spell_data_t* s_data )
{
  static constexpr const char* role_mult_str =
    "$rolemult=$?a137048|a137028|a137023|a137010|a212613|a137008|a137039|a137031|a137032|a137029|a137024|a137024|"
    "a356810|a137012[${0.66}.2][${1}]";

  double mult = 1.0;
  auto vars = s_data ? player->dbc->spell_desc_vars( s_data->id() ).desc_vars() : role_mult_str;

  assert( vars && "No spell description variables found. role_mult( player_t* ) can provide a default value." );
  if ( vars )
  {
    std::cmatch m;
    std::regex get_var( R"(\$(?:healing)?rolemult=\$(.*))" );  // find the $rolemult= variable
    if ( std::regex_search( vars, m, get_var ) )
    {
      const auto var = m.str( 1 );
      std::regex get_role( R"(\??((?:a\d+\|?)*)\[\$\{([\d\.]+)\}[\d\.]*\])" );  // find each role group
      std::sregex_iterator role_it( var.begin(), var.end(), get_role );
      for ( std::sregex_iterator i = role_it; i != std::sregex_iterator(); i++ )
      {
        mult = util::to_double( i->str( 2 ) );
        const auto role = i->str( 1 );
        std::regex get_spec( R"(a(\d+))" );  // find each spec spell id
        std::sregex_iterator spec_it( role.begin(), role.end(), get_spec );
        for ( std::sregex_iterator j = spec_it; j != std::sregex_iterator(); j++ )
        {
          if ( util::to_unsigned_ignore_error( j->str( 1 ), 0u ) == player->spec_spell->id() )
          {
            player->sim->print_debug( "parsed role multiplier for spell '{}': {}",
                                      s_data ? s_data->name_cstr() : "none", mult );
            return mult;
          }
        }
      }
    }
  }

  return mult;
}

double unique_gear::role_mult( const special_effect_t& effect )
{
  return role_mult( effect.player, effect.driver() );
}

const spell_data_t* unique_gear::spell_from_spell_text( const special_effect_t& e )
{
  if ( auto desc = e.player->dbc->spell_text( e.spell_id ).desc() )
  {
    std::cmatch m;
    std::regex r( R"(\$\?a)" + std::to_string( e.player->spec_spell->id() ) + R"(\[\$@spellname([0-9]+)\]\[\])" );
    if ( std::regex_search( desc, m, r ) )
    {
      auto id = as<unsigned>( std::stoi( m.str( 1 ) ) );
      auto spell = e.player->find_spell( id );

      e.player->sim->print_debug( "parsed spell for special effect '{}': {}", e.name(), *spell );
      return spell;
    }
  }

  return spell_data_t::nil();
}

std::vector<unsigned> unique_gear::equipped_gem_list( player_t* player, util::span<const unsigned> gem_desc_id )
{
  std::vector<unsigned> gems;

  for ( const auto& item : player->items )
  {
    for ( auto gem_id : item.parsed.gem_id )
    {
      if ( gem_id )
      {
        const auto& _gem = player->dbc->item( gem_id );
        const auto& _prop = player->dbc->gem_property( _gem.gem_properties );
        for ( auto g : gem_desc_id )
        {
          if ( _prop.desc_id == g )
          {
            gems.push_back( g );
            break;
          }
        }
      }
    }
  }

  return gems;
}

std::vector<unsigned> unique_gear::unique_gem_list( player_t* player, util::span<const unsigned> gem_desc_id )
{
  auto _list = equipped_gem_list( player, gem_desc_id );
  range::sort( _list );

  auto it = range::unique( _list );
  _list.erase( it, _list.end() );

  return _list;
}

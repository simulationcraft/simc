// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_thewarwithin.hpp"

#include "action/absorb.hpp"
#include "action/action.hpp"
#include "action/dot.hpp"
#include "actor_target_data.hpp"
#include "buff/buff.hpp"
#include "darkmoon_deck.hpp"
#include "dbc/data_enums.hh"
#include "dbc/item_database.hpp"
#include "dbc/spell_data.hpp"
#include "ground_aoe.hpp"
#include "item/item.hpp"
#include "player/action_variable.hpp"
#include "player/consumable.hpp"
#include "player/pet_spawner.hpp"
#include "set_bonus.hpp"
#include "sim/cooldown.hpp"
#include "sim/proc_rng.hpp"
#include "sim/sim.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"
#include "util/string_view.hpp"

#include <regex>

namespace unique_gear::thewarwithin
{
std::vector<unsigned> __tww_special_effect_ids;

// assuming priority for highest/lowest secondary is vers > mastery > haste > crit
static constexpr std::array<stat_e, 4> secondary_ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING,
                                                             STAT_HASTE_RATING, STAT_CRIT_RATING };

// can be called via unqualified lookup
void register_special_effect( unsigned spell_id, custom_cb_t init_callback, bool fallback = false )
{
  unique_gear::register_special_effect( spell_id, init_callback, fallback );
  __tww_special_effect_ids.push_back( spell_id );
}

void register_special_effect( std::initializer_list<unsigned> spell_ids, custom_cb_t init_callback,
                              bool fallback = false )
{
  for ( auto id : spell_ids )
    register_special_effect( id, init_callback, fallback );
}

const spell_data_t* spell_from_spell_text( const special_effect_t& e )
{
  if ( auto desc = e.player->dbc->spell_text( e.spell_id ).desc() )
  {
    std::cmatch m;
    std::regex r( R"(\$\?a)" + std::to_string( e.player->spec_spell->id() ) + R"(\[\$@spellname([0-9]+)\]\[\])" );
    if ( std::regex_search( desc, m, r ) )
    {
      auto id = as<unsigned>( std::stoi( m.str( 1 ) ) );
      auto spell = e.player->find_spell( id );

      e.player->sim->print_debug( "parsed spell for special effect '{}': {} ({})", e.name(), spell->name_cstr(), id );
      return spell;
    }
  }

  return spell_data_t::nil();
}

template <typename T = stat_buff_t>
void create_all_stat_buffs( const special_effect_t& effect, const spell_data_t* buff_data, double amount,
                            std::function<void( stat_e, buff_t* )> add_fn )
{
  static_assert( std::is_base_of_v<stat_buff_t, T> );
  auto buff_name = util::tokenize_fn( buff_data->name_cstr() );

  for ( const auto& eff : buff_data->effects() )
  {
    if ( eff.type() != E_APPLY_AURA || eff.subtype() != A_MOD_RATING )
      continue;

    auto stats = util::translate_all_rating_mod( eff.misc_value1() );

    std::vector<std::string_view> stat_strs;
    range::transform( stats, std::back_inserter( stat_strs ), &util::stat_type_abbrev );

    auto name = fmt::format( "{}_{}", buff_name, util::string_join( stat_strs, "_" ) );
    auto buff = create_buff<T>( effect.player, name, buff_data )
      ->add_stat( stats.front(), amount ? amount : eff.average( effect ) )
      ->set_name_reporting( util::string_join( stat_strs ) );

    add_fn( stats.front(), buff );
  }
}

// from item_naming.inc
enum gem_color_e : unsigned
{
  GEM_RUBY     = 14110,
  GEM_AMBER    = 14111,
  GEM_EMERALD  = 14113,
  GEM_SAPPHIRE = 14114,
  GEM_ONYX     = 14115
};

static constexpr std::array<gem_color_e, 5> gem_colors = { GEM_RUBY, GEM_AMBER, GEM_EMERALD, GEM_SAPPHIRE, GEM_ONYX };

std::vector<gem_color_e> algari_gem_list( player_t* player )
{
  std::vector<gem_color_e> gems;

  for ( const auto& item : player->items )
  {
    for ( auto gem_id : item.parsed.gem_id )
    {
      if ( gem_id )
      {
        const auto& _gem  = player->dbc->item( gem_id );
        const auto& _prop = player->dbc->gem_property( _gem.gem_properties );
        for ( auto g : gem_colors )
        {
          if ( _prop.desc_id == static_cast<unsigned>( g ) )
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

std::vector<gem_color_e> algari_gem_list( const special_effect_t& effect )
{
  return algari_gem_list( effect.player );
}

std::vector<gem_color_e> unique_gem_list( player_t* player )
{
  auto _list = algari_gem_list( player );
  range::sort( _list );

  auto it = range::unique( _list );
  _list.erase( it, _list.end() );

  return _list;
}

std::vector<gem_color_e> unique_gem_list( const special_effect_t& effect )
{
  return unique_gem_list( effect.player );
}

namespace consumables
{
// Food
static constexpr unsigned food_coeff_spell_id = 456961;
using selector_fn = std::function<stat_e( const player_t*, util::span<const stat_e> )>;

struct selector_food_buff_t : public consumable_buff_t<stat_buff_t>
{

  double amount;
  bool highest;

  selector_food_buff_t( const special_effect_t& e, bool b )
    : consumable_buff_t( e.player, e.name(), e.driver() ), highest( b )
  {
    amount = e.stat_amount;
  }

  void start( int s, double v, timespan_t d ) override
  {
    auto stat = highest ? util::highest_stat( player, secondary_ratings )
                        : util::lowest_stat( player, secondary_ratings );

    if( !manual_stats_added )
      add_stat( stat, amount );

    consumable_buff_t::start( s, v, d );
  }
};

custom_cb_t selector_food( unsigned id, bool highest, bool major = true )
{
  return [ = ]( special_effect_t& effect ) {
    effect.spell_id = id;

    auto coeff = effect.player->find_spell( food_coeff_spell_id );

    effect.stat_amount = coeff->effectN( 4 ).average( effect );
    if ( !major )
      effect.stat_amount *= coeff->effectN( 1 ).base_value() * 0.1;

    effect.custom_buff = new selector_food_buff_t( effect, highest );
  };
}

custom_cb_t primary_food( unsigned id, stat_e stat, size_t primary_idx = 3, bool major = true )
{
  return [ = ]( special_effect_t& effect ) {
    effect.spell_id = id;

    auto coeff = effect.player->find_spell( food_coeff_spell_id );

    auto buff = create_buff<consumable_buff_t<stat_buff_t>>( effect.player, effect.driver() );

    if ( primary_idx )
    {
      auto _amt = coeff->effectN( primary_idx ).average( effect );
      if ( !major )
        _amt *= coeff->effectN( 1 ).base_value() * 0.1;

      buff->add_stat( effect.player->convert_hybrid_stat( stat ), _amt );
    }

    if ( primary_idx == 3 )
    {
      auto _amt = coeff->effectN( 8 ).average( effect );
      if ( !major )
        _amt *= coeff->effectN( 1 ).base_value() * 0.1;

      buff->add_stat( STAT_STAMINA, _amt );
    }

    effect.custom_buff = buff;
  };
}

custom_cb_t secondary_food( unsigned id, stat_e stat1, stat_e stat2 = STAT_NONE )
{
  return [ = ]( special_effect_t& effect ) {
    effect.spell_id = id;

    auto coeff = effect.player->find_spell( food_coeff_spell_id );

    auto buff = create_buff<consumable_buff_t<stat_buff_t>>( effect.player, effect.driver() );

    // all single secondary stat food are minor foods. note that item tooltip for hearty versions are incorrect and do
    // not apply the minor food multiplier.
    if ( stat2 == STAT_NONE )
    {
      auto _amt = coeff->effectN( 4 ).average( effect ) * coeff->effectN( 1 ).base_value() * 0.1;
      buff->add_stat( stat1, _amt );
    }
    else
    {
      auto _amt = coeff->effectN( 5 ).average( effect );
      buff->add_stat( stat1, _amt );
      buff->add_stat( stat2, _amt );
    }

    effect.custom_buff = buff;
  };
}

// Flasks
// TODO: can you randomize into the same stat? same bonus stat? same penalty stats?
void flask_of_alchemical_chaos( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "flask_of_alchemical_chaos_vers", "flask_of_alchemical_chaos_mastery",
                                        "flask_of_alchemical_chaos_haste", "flask_of_alchemical_chaos_crit" } ) )
  {
    return;
  }

  struct flask_of_alchemical_chaos_buff_t : public consumable_buff_t<buff_t>
  {
    std::vector<std::pair<buff_t*, buff_t*>> buff_list;

    flask_of_alchemical_chaos_buff_t( const special_effect_t& e ) : consumable_buff_t( e.player, e.name(), e.driver() )
    {
      auto bonus = e.driver()->effectN( 5 ).average( e );
      auto penalty = -( e.driver()->effectN( 6 ).average( e ) );

      auto add_vers = create_buff<stat_buff_t>( e.player, "flask_of_alchemical_chaos_vers", e.driver() )
        ->add_stat( STAT_VERSATILITY_RATING, bonus )->set_name_reporting( "Vers" );
      auto add_mastery = create_buff<stat_buff_t>( e.player, "flask_of_alchemical_chaos_mastery", e.driver() )
        ->add_stat( STAT_MASTERY_RATING, bonus )->set_name_reporting( "Mastery" );
      auto add_haste = create_buff<stat_buff_t>( e.player, "flask_of_alchemical_chaos_haste", e.driver() )
        ->add_stat( STAT_HASTE_RATING, bonus )->set_name_reporting( "Haste" );
      auto add_crit = create_buff<stat_buff_t>( e.player, "flask_of_alchemical_chaos_crit", e.driver() )
        ->add_stat( STAT_CRIT_RATING, bonus )->set_name_reporting( "Crit" );

      auto sub_vers = create_buff<stat_buff_t>( e.player, "alchemical_chaos_vers_penalty", e.driver() )
        ->add_stat( STAT_VERSATILITY_RATING, penalty )->set_quiet( true );
      auto sub_mastery = create_buff<stat_buff_t>( e.player, "alchemical_chaos_mastery_penalty", e.driver() )
        ->add_stat( STAT_MASTERY_RATING, penalty )->set_quiet( true );
      auto sub_haste = create_buff<stat_buff_t>( e.player, "alchemical_chaos_haste_penalty", e.driver() )
        ->add_stat( STAT_HASTE_RATING, penalty )->set_quiet( true );
      auto sub_crit = create_buff<stat_buff_t>( e.player, "alchemical_chaos_crit_penalty", e.driver() )
        ->add_stat( STAT_CRIT_RATING, penalty )->set_quiet( true );

      buff_list.emplace_back( add_vers, sub_vers );
      buff_list.emplace_back( add_mastery, sub_mastery );
      buff_list.emplace_back( add_haste, sub_haste );
      buff_list.emplace_back( add_crit, sub_crit );

      set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
        rng().shuffle( buff_list.begin(), buff_list.end() );
        buff_list[ 0 ].first->trigger();   // bonus
        buff_list[ 0 ].second->expire();
        buff_list[ 1 ].first->expire();
        buff_list[ 1 ].second->trigger();  // penalty
        buff_list[ 2 ].first->expire();
        buff_list[ 2 ].second->trigger();  // penalty
        buff_list[ 3 ].first->expire();
        buff_list[ 3 ].second->expire();
      } );
    }

    timespan_t tick_time() const override
    {
      if ( current_tick == 0 )
        return sim->rng().range( 1_ms, buff_period );
      else
        return buff_period;
    }
  };

  effect.custom_buff = new flask_of_alchemical_chaos_buff_t( effect );
}

// Potions
void tempered_potion( special_effect_t& effect )
{
  auto tempered_stat = STAT_NONE;

  if ( auto _flask = dynamic_cast<dbc_consumable_base_t*>( effect.player->find_action( "flask" ) ) )
    if ( auto _buff = _flask->consumable_buff; _buff && util::starts_with( _buff->name_str, "flask_of_tempered_" ) )
      tempered_stat = static_cast<stat_buff_t*>( _buff )->stats.front().stat;

  auto buff = create_buff<stat_buff_t>( effect.player, effect.driver() );
  auto amount = effect.driver()->effectN( 1 ).average( effect );

  for ( auto s : secondary_ratings )
    if ( s != tempered_stat )
      buff->add_stat( s, amount );

  effect.custom_buff = buff;
}

void potion_of_unwavering_focus( special_effect_t& effect )
{
  struct unwavering_focus_t : public generic_proc_t
  {
    double mul;

    unwavering_focus_t( const special_effect_t& e ) : generic_proc_t( e, e.name(), e.driver() )
    {
      target_debuff = e.driver();

      auto rank = e.item->parsed.data.crafting_quality;
      auto base = e.driver()->effectN( 2 ).base_value();

      mul = ( base + rank - 1 ) * 0.01;
    }

    buff_t* create_debuff( player_t* t ) override
    {
      auto debuff = generic_proc_t::create_debuff( t )
        ->set_default_value( mul )
        ->set_cooldown( 0_ms );

      player->get_target_data( t )->debuff.unwavering_focus = debuff;

      return debuff;
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      get_debuff( s->target )->trigger();
    }
  };

  effect.execute_action = create_proc_action<unwavering_focus_t>( "potion_of_unwavering_focus", effect );
}

// Oils
void oil_of_deep_toxins( special_effect_t& effect )
{
  effect.discharge_amount = effect.driver()->effectN( 1 ).average( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Bubbling Wax
void bubbling_wax( special_effect_t& effect )
{
  // This is Rogue Only.
  if ( effect.player->type != ROGUE )
    return;

  // TODO: See if check later if RPPM increases while wearing two. Appears currently not?
  // Currently, this only works if you use the wax on your Main Hand and the offhand does not function.
  // Remove this if they fix it being Main Hand Only, and see below comment what to do incase of RPPM increase instead
  // of damage.
  if ( effect.item->slot != SLOT_MAIN_HAND && effect.player->bugs )
    return;

  auto wax_action = effect.player->find_action( "bubbling_wax" );

  auto damage_amount = effect.driver()->effectN( 1 ).average( effect );

  if ( !wax_action )
  {
    auto damage_id      = effect.driver()->effectN( 1 ).trigger_spell_id();
    auto damage         = create_proc_action<generic_aoe_proc_t>( "bubbling_wax", effect, damage_id, true );
    damage->base_dd_min = damage->base_dd_max = damage_amount;
    effect.execute_action                     = damage;
    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    // This would increase the damage if you had two, but due to the slot main_hand check above this will never occur
    // unless bugs=0 is set.
    // If this is ever fixed and RPPM increases with two instead, remove the if() else() block and initialise two procs,
    // one for each hand using the main code from the above if.
    wax_action->base_dd_min += damage_amount;
    wax_action->base_dd_max += damage_amount;
  }
}
}  // namespace consumables

namespace enchants
{
void authority_of_radiant_power( special_effect_t& effect )
{
  auto found = effect.player->find_action( "authority_of_radiant_power" );

  auto damage = create_proc_action<generic_proc_t>( "authority_of_radiant_power", effect, 448744 );
  auto damage_val = effect.driver()->effectN( 1 ).average( effect );
  damage->base_dd_min += damage_val;
  damage->base_dd_max += damage_val;

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 448730 ) )
    ->add_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 2 ).average( effect ) );

  if ( found )
    return;

  damage->base_multiplier *= role_mult( effect.player, effect.player->find_spell( 445339 ) );

  effect.spell_id = effect.trigger()->id();  // rppm driver is the effect trigger

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ buff, damage ]( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
      damage->execute_on_target( s->target );
      buff->trigger();
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// TODO: confirm coeff is per tick and not for entire dot
void authority_of_the_depths( special_effect_t& effect )
{
  auto found = effect.player->find_action( "suffocating_darkness" );

  auto damage = create_proc_action<generic_proc_t>( "suffocating_darkness", effect, 449217 );
  damage->base_td += effect.driver()->effectN( 1 ).average( effect );

  if ( found )
    return;

  damage->base_multiplier *= role_mult( effect.player, effect.player->find_spell( 445341 ) );

  effect.spell_id = effect.trigger()->id();  // rppm driver is the effect trigger

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

void authority_of_storms( special_effect_t& effect )
{
  // has data to suggest proc is supposed to be (or once was) a 12s buff that triggers damage at 6rppm, but currently in
  // game is a simple damage proc.

  auto found = effect.player->find_action( "authority_of_storms" ) ;

  auto damage_id = as<unsigned>( effect.trigger()->effectN( 1 ).misc_value1() );
  auto damage = create_proc_action<generic_aoe_proc_t>( "authority_of_storms", effect, damage_id );
  damage->base_dd_min += effect.driver()->effectN( 1 ).average( effect );
  damage->base_dd_max += effect.driver()->effectN( 1 ).average( effect );

  if ( found )
    return;

  damage->base_multiplier *= role_mult( effect.player, effect.player->find_spell( 445336 ) );

  effect.spell_id = effect.trigger()->id();  // rppm driver is the effect trigger

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

void secondary_weapon_enchant( special_effect_t& effect )
{
  auto buff_data = effect.trigger()->effectN( 1 ).trigger();
  auto buff_name = util::tokenize_fn( buff_data->name_cstr() );

  auto found = buff_t::find( effect.player, buff_name );

  auto buff = create_buff<stat_buff_t>( effect.player, buff_name, buff_data )
    ->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

  if ( found )
    return;

  effect.spell_id = effect.trigger()->id();  // rppm driver is the effect trigger

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// 435500 driver
void culminating_blasphemite( special_effect_t& effect )
{
  auto pct = effect.driver()->effectN( 1 ).percent() * unique_gem_list( effect ).size();
  // check for prismatic null stone
  if ( auto null_stone = find_special_effect( effect.player, 435992 ) )
    pct *= 1.0 + null_stone->driver()->effectN( 1 ).percent();

  effect.player->base.crit_damage_multiplier *= 1.0 + pct;
  effect.player->base.crit_healing_multiplier *= 1.0 + pct;
}

// 435488 driver
void insightful_blasphemite( special_effect_t& effect )
{
  auto pct = effect.driver()->effectN( 1 ).percent() * unique_gem_list( effect ).size();
  // check for prismatic null stone
  if ( auto null_stone = find_special_effect( effect.player, 435992 ) )
    pct *= 1.0 + null_stone->driver()->effectN( 1 ).percent();

  effect.player->resources.base_multiplier[ RESOURCE_MANA ] *= 1.0 + pct;
}

void daybreak_spellthread( special_effect_t& effect )
{
  effect.player->resources.base_multiplier[ RESOURCE_MANA ] *= 1.0 + effect.driver()->effectN( 1 ).percent();
}

}  // namespace enchants

namespace embellishments
{
// 443743 driver, trigger buff
// 447005 buff
void blessed_weapon_grip( special_effect_t& effect )
{
  std::unordered_map<stat_e, buff_t*> buffs;

  create_all_stat_buffs( effect, effect.trigger(), effect.trigger()->effectN( 6 ).average( effect ),
    [ &buffs ]( stat_e s, buff_t* b ) { buffs[ s ] = b->set_reverse( true ); } );

  effect.player->callbacks.register_callback_execute_function( effect.spell_id,
    [ buffs ]( const dbc_proc_callback_t* cb, action_t*, const action_state_t* ) {
      auto stat = util::highest_stat( cb->listener, secondary_ratings );
      for ( auto [ s, b ] : buffs )
      {
        if ( s == stat )
          b->trigger();
        else
          b->expire();
      }
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 453503 equip, trigger driver
//  e1: damage coeff?
// 453508 driver, trigger missile
//  e1: damage coeff?
// 453510 missile, trigger damage
// 453782 damage
// TODO: determine which coeff is the correct one. assuming driver is correct.
// TODO: confirm damage doesn't increase per extra target
void pouch_of_pocket_grenades( special_effect_t& effect )
{
  auto driver = effect.trigger();
  auto missile = driver->effectN( 1 ).trigger();
  auto damage = missile->effectN( 1 ).trigger();
  // TODO: determine which coeff is the correct one. assuming driver is correct.
  auto amount = driver->effectN( 1 ).average( effect );
  auto multiplier = role_mult( effect );

  effect.spell_id = driver->id();

  auto found = effect.player->find_action( "pocket_grenade" );

  // TODO: confirm damage doesn't increase per extra target
  auto grenade = create_proc_action<generic_aoe_proc_t>( "pocket_grenade", effect, damage );
  grenade->base_dd_min += amount;
  grenade->base_dd_max += amount;
  // We cannot use `*=`, as two copies of the embellishment would doubly apply role mult.
  grenade->base_multiplier = multiplier;

  if ( found )
    return;

  grenade->travel_speed = missile->missile_speed();

  effect.execute_action = grenade;

  new dbc_proc_callback_t( effect.player, effect );
}

// 461177 equip
//  e1: coeff, trigger driver
// 461180 driver
// 461185 holy damage from amber
// 461190 nature damage from emerald
// 461191 shadow damage from onyx
// 461192 fire damage from ruby
// 461193 frost damage from sapphire
void elemental_focusing_lens( special_effect_t& effect )
{
  auto gems = unique_gem_list( effect );

  if ( !gems.size() )
    return;

  auto amount = effect.driver()->effectN( 1 ).average( effect );
  auto multiplier = role_mult( effect );

  effect.spell_id = effect.trigger()->id();

  // guard against duplicate proxy/callback from multiple embellishments
  auto proxy = effect.player->find_action( "elemental_focusing_lens" );
  if ( !proxy )
  {
    proxy = new action_t( action_e::ACTION_OTHER, "elemental_focusing_lens", effect.player, effect.driver() );
    new dbc_proc_callback_t( effect.player, effect );
  }

  std::vector<action_t*> damages;

  static constexpr std::array<std::tuple<gem_color_e, unsigned, const char*>, 5> damage_ids = { {
    { GEM_RUBY,     461192, "ruby"     },
    { GEM_AMBER,    461185, "amber"    },
    { GEM_EMERALD,  461190, "emerald"  },
    { GEM_SAPPHIRE, 461193, "sapphire" },
    { GEM_ONYX,     461191, "onyx"     }
  } };

  for ( auto [ g, id, name ] : damage_ids )
  {
    if ( !range::contains( gems, g ) )
      continue;

    auto dam = create_proc_action<generic_proc_t>( fmt::format( "elemental_focusing_lens_{}", name ), effect, id );
    dam->base_dd_min += amount;
    dam->base_dd_max += amount;
    // We cannot use `*=`, as two copies of the embellishment would doubly apply role mult.
    dam->base_multiplier = multiplier;
    dam->name_str_reporting = util::inverse_tokenize( name );
    damages.push_back( dam );
    proxy->add_child( dam );
  }

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ damages ]( const dbc_proc_callback_t* cb, action_t*, const action_state_t* s ) {
      cb->rng().range( damages )->execute_on_target( s->target );
    } );
}

// 436085 on use
// 436132 Binding of Binding Driver
// 436159 Boon of Binding Buff
void binding_of_binding( special_effect_t& effect )
{
  struct binding_of_binding_cb_t : public dbc_proc_callback_t
   {
     target_specific_t<buff_t> buffs;
     double binding_of_binding_ally_trigger_chance;
     binding_of_binding_cb_t( const special_effect_t& e )
       : dbc_proc_callback_t( e.player, e ),
         buffs{ false },
         binding_of_binding_ally_trigger_chance( effect.player->thewarwithin_opts.binding_of_binding_ally_trigger_chance )
     {
       get_buff( effect.player );
     }

     buff_t* get_buff( player_t* buff_player )
     {
       if ( buffs[ buff_player ] )
         return buffs[ buff_player ];

       auto gems = unique_gem_list( buff_player );

       auto buff_spell = effect.player->find_spell( 436159 );

       auto buff = make_buff<stat_buff_t>( actor_pair_t{ buff_player, effect.player }, "boon_of_binding", buff_spell );

       for ( gem_color_e gem_color : gems )
       {
         switch ( gem_color )
         {
           case GEM_RUBY:
             buff->add_stat_from_effect( 2, effect.driver()->effectN( 1 ).average( effect.player ) / 4 );
             break;
           case GEM_EMERALD:
             buff->add_stat_from_effect( 3, effect.driver()->effectN( 1 ).average( effect.player ) / 4 );
             break;
           case GEM_SAPPHIRE:
             buff->add_stat_from_effect( 4, effect.driver()->effectN( 1 ).average( effect.player ) / 4 );
             break;
           case GEM_ONYX:
             buff->add_stat_from_effect( 5, effect.driver()->effectN( 1 ).average( effect.player ) / 4 );
             break;
           default:
             break;
         }
       }

       buffs[ buff_player ] = buff;

       return buff;
     }

     void execute( action_t*, action_state_t* ) override
     {
       if ( effect.player->thewarwithin_opts.binding_of_binding_on_you > 0 )
       {
         buffs[ effect.player ]->trigger();
       }
       else
       {
         if ( !effect.player->sim->single_actor_batch && effect.player->sim->player_non_sleeping_list.size() > 1 )
         {
           std::vector<player_t*> helper_vector = effect.player->sim->player_non_sleeping_list.data();
           rng().shuffle( helper_vector.begin(), helper_vector.end() );

           for ( auto p : helper_vector )
           {
             if ( p == effect.player || p->is_pet() )
               continue;

             if ( rng().roll( binding_of_binding_ally_trigger_chance ) )
               get_buff( p )->trigger();

             break;
           }
         }
       }
     }
   };

   new binding_of_binding_cb_t( effect );
}

// 457665 dawnthread driver
// 457666 dawnthread buff
// 457677 duskthread driver
// 457674 duskthread buff
void dawn_dusk_thread_lining( special_effect_t& effect )
{
  struct dawn_dusk_thread_lining_t : public stat_buff_t
  {
    rng::truncated_gauss_t interval;

    dawn_dusk_thread_lining_t( player_t* p, std::string_view n, const spell_data_t* s )
      : stat_buff_t( p, n, s ),
        interval( p->thewarwithin_opts.dawn_dusk_thread_lining_update_interval,
                  p->thewarwithin_opts.dawn_dusk_thread_lining_update_interval_stddev )
    {
    }
  };

  const spell_data_t* buff_spell = effect.player->find_spell( 457674 );

  if( effect.spell_id == 457665 )
    buff_spell = effect.player->find_spell( 457666 );

  auto buff = create_buff<dawn_dusk_thread_lining_t>( effect.player, buff_spell );
  buff->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  bool first = !buff->manual_stats_added;
  // In some cases, the buff values from separate items don't stack. This seems to fix itself
  // when the player loses and regains the buff, so we just assume they stack properly.
  buff->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

  // In case the player has two copies of this embellishment, set up the buff events only once.
  if ( first && effect.player->thewarwithin_opts.dawn_dusk_thread_lining_uptime > 0.0 )
  {
    auto up = effect.player->get_uptime( buff_spell->_name )
                  ->collect_duration( *effect.player->sim )
                  ->collect_uptime( *effect.player->sim );

    buff->player->register_precombat_begin( [ buff, up ]( player_t* p ) {
      buff->trigger();
      up->update( true, p->sim->current_time() );

      auto pct = p->thewarwithin_opts.dawn_dusk_thread_lining_uptime;

      make_repeating_event(
          *p->sim, [ p, buff ] { return p->rng().gauss( buff->interval ); },
          [ buff, p, up, pct ] {
            if ( p->rng().roll( pct ) )
            {
              buff->trigger();
              up->update( true, p->sim->current_time() );
            }
            else
            {
              buff->expire();
              up->update( false, p->sim->current_time() );
            }
          } );
    } );
  }
}

// Deepening Darkness
// 443760 Driver
// 446753 Damage
// 446743 Buff
// 446836 movement speed debuff
// TODO: implement movement speed debuff
void deepening_darkness( special_effect_t& effect )
{
  auto damage =
    create_proc_action<generic_aoe_proc_t>( "deepening_darkness", effect, effect.player->find_spell( 446753 ), true );

  damage->base_dd_min = damage->base_dd_max =
    effect.driver()->effectN( 2 ).average( effect );
  damage->base_multiplier *= role_mult( effect );
  damage->base_multiplier *= writhing_mul( effect.player );

  auto buff = create_buff<buff_t>( effect.player, effect.player->find_spell( 446743 ) )
                  ->set_expire_callback( [ damage ]( buff_t*, int, timespan_t d ) {
                    if ( d > 0_ms )
                    {
                      damage->execute();
                    }
                  } )
                  ->set_expire_at_max_stack( true );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Spark of Beledar
// 443736 Driver
// 446224 Damage
// 446402 Damage with Debuff
// 446234 Debuff
void spark_of_beledar( special_effect_t& effect )
{
  struct spark_of_beledar_damage_t : public generic_proc_t
  {
    action_t* bonus_damage;
    const spell_data_t* debuff_spell;

    spark_of_beledar_damage_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "spark_of_beledar", data ),
        bonus_damage( nullptr ),
        debuff_spell( e.player->find_spell( 446234 ) )
    {
      bonus_damage =
          create_proc_action<generic_proc_t>( "blazing_spark_of_beledar", e, e.player->find_spell( 446402 ) );
      bonus_damage->base_dd_min = bonus_damage->base_dd_max = e.driver()->effectN( 3 ).average( e );

      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );

      add_child( bonus_damage );
    }

    buff_t* create_debuff( player_t* t ) override
    {
      return make_buff( actor_pair_t( t, player ), "spark_of_beledar_debuff", debuff_spell );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      auto debuff = get_debuff( s->target );
      if ( debuff->check() )
      {
        debuff->expire();
        bonus_damage->execute_on_target( s->target );
      }
      else
      {
        debuff->trigger();
      }
    }
  };

  effect.execute_action =
      create_proc_action<spark_of_beledar_damage_t>( "spark_of_beledar", effect, effect.player->find_spell( 446224 ) );

  new dbc_proc_callback_t( effect.player, effect );
}

// Adrenal Surge Clasp
// 443762 Driver
// 446108 Buff
void adrenal_surge( special_effect_t& effect )
{
  // create primary stat buff
  auto primary_stat_trigger = effect.driver()->effectN( 1 ).trigger();
  auto primary_value = primary_stat_trigger->effectN( 1 ).average( effect ) * writhing_mul( effect.player );
  auto primary_stat_buff = create_buff<stat_buff_t>( effect.player, "adrenal_surge", primary_stat_trigger )
    ->set_stat_from_effect_type( A_MOD_STAT, primary_value );


  // create mastery loss debuff
  // TODO: currently this does not scale with item level
  auto mastery_loss_trigger = primary_stat_trigger->effectN( 3 ).trigger();
  auto mastery_value = mastery_loss_trigger->effectN( 1 ).average( effect ) * writhing_mul( effect.player );
  auto mastery_loss_buff = create_buff<stat_buff_t>( effect.player, "adrenal_surge_debuff", mastery_loss_trigger )
    ->set_stat_from_effect_type( A_MOD_RATING, mastery_value )
    ->set_name_reporting( "Debuff" );

  effect.player->callbacks.register_callback_execute_function(
      effect.spell_id,
      [ primary_stat_buff, mastery_loss_buff ]( const dbc_proc_callback_t*, action_t*, action_state_t* ) {
        primary_stat_buff->trigger();
        mastery_loss_buff->trigger();
      } );

  new dbc_proc_callback_t( effect.player, effect );
}

// Siphoning Stilleto
// 453573 Driver
// Effect 1: Self Damage
// Effect 2: Damage
// Effect 3: Duration
// Effect 4: Range
// 458630 Self Damage
// 458624 Damage
void siphoning_stilleto( special_effect_t& effect )
{
  struct siphoning_stilleto_cb_t : public dbc_proc_callback_t
  {
    action_t* self;
    action_t* damage;
    timespan_t duration;
    siphoning_stilleto_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        self( nullptr ),
        damage( nullptr ),
        duration( timespan_t::from_seconds( e.driver()->effectN( 3 ).base_value() ) )
    {
      self              = create_proc_action<generic_proc_t>( "siphoning_stilleto_self", e, 458630 );
      // TODO: Check if self damage is affected by the role multiplier
      self->base_dd_min = self->base_dd_max = e.driver()->effectN( 1 ).average( e ) * writhing_mul( e.player );
      self->stats->type = STATS_NEUTRAL;
      // TODO: Check if self damage can trigger other effects
      self->callbacks   = false;
      self->target      = e.player;

      damage              = create_proc_action<generic_proc_t>( "siphoning_stilleto", e, 458624 );
      damage->base_dd_min = damage->base_dd_max =
          e.driver()->effectN( 2 ).average( e );
      damage->base_multiplier *= role_mult( e );
      damage->base_multiplier *= writhing_mul( e.player );
    }

    void execute( action_t*, action_state_t* ) override
    {
      self->execute_on_target( listener );
      // TODO: implement range check if it ever matters for specilizations that can use this.
      make_event( *listener->sim, duration, [ this ] {
        if ( !listener->sim->target_non_sleeping_list.empty() )
          damage->execute_on_target( rng().range( listener->sim->target_non_sleeping_list ) );
      } );
    }
  };

  new siphoning_stilleto_cb_t( effect );
}
}  // namespace embellishments

namespace items
{
struct stat_buff_with_multiplier_t : public stat_buff_t
{
  double stat_mul = 1.0;

  stat_buff_with_multiplier_t( player_t* p, std::string_view n, const spell_data_t* s ) : stat_buff_t( p, n, s ) {}

  double buff_stat_stack_amount( const buff_stat_t& stat, int s ) const override
  {
    return stat_buff_t::buff_stat_stack_amount( stat, s ) * stat_mul;
  }

  void expire_override( int s, timespan_t d ) override
  {
    stat_buff_t::expire_override( s, d );
    stat_mul = 1.0;
  }
};

// Trinkets
// 444958 equip driver
//  e1: stacking value
//  e2: on-use value
// 444959 on-use driver & buff
// 451199 proc projectile & buff
void spymasters_web( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "spymasters_web", "spymasters_report" } ) )
    return;

  unsigned equip_id = 444958;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Spymaster's Web missing equip effect" );

  auto equip_data = equip->driver();
  auto buff_data = equip->trigger();

  auto stacking_buff = create_buff<stat_buff_t>( effect.player, buff_data )
    ->set_stat_from_effect_type( A_MOD_STAT, equip_data->effectN( 1 ).average( effect ) )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  equip->custom_buff = stacking_buff;

  new dbc_proc_callback_t( effect.player, *equip );

  auto use_buff = create_buff<stat_buff_t>( effect.player, effect.driver() )
    ->set_stat_from_effect_type( A_MOD_STAT, equip_data->effectN( 2 ).average( effect ) )
    ->set_max_stack( stacking_buff->max_stack() )
    ->set_cooldown( 0_ms );

  struct spymasters_web_t : public generic_proc_t
  {
    buff_t* stacking_buff;
    buff_t* use_buff;

    spymasters_web_t( const special_effect_t& e, buff_t* stack, buff_t* use )
      : generic_proc_t( e, "spymasters_web", e.driver() ), stacking_buff( stack ), use_buff( use )
    {}

    void execute() override
    {
      generic_proc_t::execute();

      if ( !stacking_buff->check() )
        return;

      use_buff->expire();
      use_buff->trigger( stacking_buff->check() );
      stacking_buff->expire();
    }
  };

  effect.disable_buff();
  effect.has_use_buff_override = true;
  effect.execute_action = create_proc_action<spymasters_web_t>( "spymasters_web", effect, stacking_buff, use_buff );
}

// 445593 equip
//  e1: damage value
//  e2: haste value
//  e3: mult per use stack
//  e4: unknown, damage cap? self damage?
//  e5: post-combat duration?
//  e6: unknown, chance to be silenced?
// 451895 is empowered buff
// 445619 on-use
// 452350 silence
// 451845 haste buff
// 451866 damage
// 452279 unknown, possibly unrelated?
// per-spec drivers?
//   452030, 452037, 452057, 452059, 452060, 452061,
//   452062, 452063, 452064, 452065, 452066, 452067,
//   452068, 452069, 452070, 452071, 452072, 452073
// TODO: confirm cycle doesn't rest on combat start
// TODO: replace with spec-specific driver id if possible
// TODO: confirm damage procs off procs
// TODO: determine when silence applies
void aberrant_spellforge( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "aberrant_empowerment", "aberrant_spellforge" } ) )
    return;

  // sanity check equip effect exists
  unsigned equip_id = 445593;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Aberrant Spellforge missing equip effect" );

  auto empowered = spell_from_spell_text( effect );
  if ( !empowered->ok() )
    return;

  // cache data
  auto data = equip->driver();
  auto period = effect.player->find_spell( 452030 )->effectN( 2 ).period();
  [[maybe_unused]] auto silence_dur = effect.player->find_spell( 452350 )->duration();

  // create buffs
  auto empowerment = create_buff<buff_t>( effect.player, effect.player->find_spell( 451895 ) )
                         ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  auto stack = create_buff<buff_t>( effect.player, effect.driver() )
                   ->set_cooldown( 0_ms )
                   ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  auto haste = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 451845 ) )
    ->set_stat_from_effect_type( A_MOD_RATING, data->effectN( 2 ).average( effect ) );

  // proc damage action
  struct aberrant_shadows_t : public generic_proc_t
  {
    buff_t* stack;
    double mult;

    aberrant_shadows_t( const special_effect_t& e, const spell_data_t* data, buff_t* b )
      : generic_proc_t( e, "aberrant_shadows", 451866 ), stack( b ), mult( 1.0 + data->effectN( 3 ).percent() )
    {
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e.player, e.player->find_spell( 445593 ) );

      for ( auto a : player->action_list )
      {
          if ( a->action_list && a->action_list->name_str == "precombat" &&
               a->name_str == "use_item_" + item->name_str )
          {
              a->harmful = false;
              break;
          }
      };
    }

    double action_multiplier() const override
    {
      // 125% per stack is applied exponentially
      return generic_proc_t::action_multiplier() * std::pow( mult, stack->check() );
    }
  };

  auto damage = create_proc_action<aberrant_shadows_t>( "aberrant_shadows", effect, data, stack );

  // setup equip effect
  // TODO: confirm cycle doesn't rest on combat start
  effect.player->register_precombat_begin( [ empowerment, period ]( player_t* p ) {
    empowerment->trigger();
    make_event( *p->sim, p->rng().range( 1_ms, period ), [ p, empowerment, period ] {
      empowerment->trigger();
      make_repeating_event( *p->sim, period, [ empowerment ] { empowerment->trigger(); } );
    } );
  } );

  // replace equip effect with per-spec driver
  // TODO: replace with spec-specific driver id if possible
  equip->spell_id = 452030;

  // TODO: confirm damage procs off procs
  // NOTE: if multiple abilities can proc, re-register the trigger function for 452030 in the class module and check for
  // all proccing abilities.
  effect.player->callbacks.register_callback_trigger_function( equip->spell_id,
      dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ id = empowered->id() ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* s ) {
        return s->result_amount && a->data().id() == id;
      } );

  effect.player->callbacks.register_callback_execute_function( equip->spell_id,
      [ damage, empowerment ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* s ) {
        if ( empowerment->check() )
        {
          damage->execute_on_target( s->target );
          empowerment->expire( a );
        }
      } );

  auto cb = new dbc_proc_callback_t( effect.player, *equip );
  cb->deactivate();

  // setup on-use effect
  /* TODO: determine when silence applies
  auto silence = [ dur = effect.player->find_spell( 452350 )->duration() ]( player_t* p ) {
    p->buffs.stunned->increment();
    p->stun();

    make_event( *p->sim, dur, [ p ] { p->buffs.stunned->decrement(); } );
  };
  */

  empowerment->set_stack_change_callback( [ cb, haste, stack ]( buff_t*, int old_, int new_ ) {
    if ( !old_ )
    {
      cb->activate();
    }
    if ( !new_ )
    {
      cb->deactivate();

      if ( stack->at_max_stacks() )
        haste->trigger();
    }
  } );

  effect.custom_buff = stack;

  // TODO: unknown if this is hardcoded to 15s or based off the dummy data
  auto expire_delay = timespan_t::from_seconds( data->effectN( 5 ).base_value() ) * 2;
  effect.player->register_on_combat_state_callback( [ stack, expire_delay ]( player_t* p, bool c ) {
    if ( !c )
    {
      make_event( *p->sim, expire_delay, [ p, stack ] {
        if ( !p->in_combat )
          stack->expire();
      } );
    }
  } );
}

// 445203 data
//  e1: flourish damage
//  e2: flourish parry
//  e3: flourish avoidance
//  e4: decimation damage
//  e5: decimation damage to shields on crit
//  e6: barrage damage
//  e7: barrage speed
// 447970 on-use
// 445434 flourish damage
// 447962 flourish stance
// 448090 decimation damage
// 448519 decimation damage to shield on crit
// 447978 decimation stance
// 445475 barrage damage
// 448036 barrage stance
// 448433 barrage stacking buff
// 448436 barrage speed buff
// TODO: confirm order is flourish->decimation->barrage
// TODO: confirm stance can be changed pre-combat and does not reset on encounter start
// TODO: confirm flourish damage value is for entire dot and not per tick
// TODO: confirm decimation damage on crit to shield only counts the absorbed amount, instead of the full damage
void sikrans_endless_arsenal( special_effect_t& effect )
{
  unsigned equip_id = 445203;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Sikran's Shadow Arsenal missing equip effect" );

  auto data = equip->driver();

  struct sikrans_endless_arsenal_t : public generic_proc_t
  {
    std::vector<std::pair<action_t*, buff_t*>> stance;

    sikrans_endless_arsenal_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "sikrans_endless_arsenal", e.driver() )
    {
      // stances are populated in order: flourish->decimation->barrage
      // TODO: confirm order is flourish->decimation->barrage

      // setup flourish
      auto f_dam = create_proc_action<generic_proc_t>( "surekian_flourish", e, 445434 );
      f_dam->base_td = data->effectN( 1 ).average( e ) * f_dam->base_tick_time / f_dam->dot_duration;
      f_dam->base_multiplier *= role_mult( e.player, e.player->find_spell( 445434 ) );
      add_child( f_dam );

      auto f_stance = create_buff<stat_buff_t>( e.player, e.player->find_spell( 447962 ) )
        ->add_stat_from_effect( 1, data->effectN( 2 ).base_value() )
        ->add_stat_from_effect( 2, data->effectN( 3 ).base_value() );

      stance.emplace_back( f_dam, f_stance );

      // setup decimation
      auto d_dam = create_proc_action<generic_aoe_proc_t>( "surekian_decimation", e, 448090, true );
      d_dam->base_dd_min = d_dam->base_dd_max = data->effectN( 4 ).average( e );
      d_dam->base_multiplier *= role_mult( e.player, e.player->find_spell( 448090 ) );
      add_child( d_dam );

      auto d_shield = create_proc_action<generic_proc_t>( "surekian_brutality", e, 448519 );
      d_shield->base_dd_min = d_shield->base_dd_max = 1.0;  // for snapshot flag parsing
      add_child( d_shield );

      auto d_stance = create_buff<buff_t>( e.player, e.player->find_spell( 447978 ) );

      auto d_driver = new special_effect_t( e.player );
      d_driver->name_str = d_stance->name();
      d_driver->spell_id = d_stance->data().id();
      d_driver->proc_flags2_ = PF2_CRIT;
      e.player->special_effects.push_back( d_driver );

      // assume that absorbed amount equals result_mitigated - result_absorbed. this assumes everything in
      // player_t::assess_damage_imminent_pre_absorb are absorbs, but as for enemies this is currently unused and highly
      // unlikely to be used in the future, we should be fine with this assumption.
      // TODO: confirm only the absorbed amount counts, instead of the full damage
      e.player->callbacks.register_callback_execute_function( d_driver->spell_id,
          [ d_shield, mul = data->effectN( 5 ).percent() ]
          ( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
            auto absorbed = s->result_mitigated - s->result_absorbed;
            if ( absorbed > 0 )
            {
              // TODO: determine if this is affected by role mult
              d_shield->base_dd_min = d_shield->base_dd_max = mul * absorbed;
              d_shield->execute_on_target( s->target );
            }
          } );

      e.player->callbacks.register_callback_trigger_function( d_driver->spell_id,
        dbc_proc_callback_t::trigger_fn_type::CONDITION,
        []( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) { return s->target->is_enemy(); } );

      auto d_cb = new dbc_proc_callback_t( e.player, *d_driver );
      d_cb->activate_with_buff( d_stance );

      stance.emplace_back( d_dam, d_stance );

      // setup barrage
      auto b_dam = create_proc_action<generic_aoe_proc_t>( "surekian_barrage", e, 445475 );
      b_dam->split_aoe_damage = false;
      b_dam->base_dd_min = b_dam->base_dd_max = data->effectN( 6 ).average( e );
      b_dam->base_multiplier *= role_mult( e.player, e.player->find_spell( 445475 ) );
      add_child( b_dam );

      auto b_speed = create_buff<buff_t>( e.player, e.player->find_spell( 448436 ) )
        ->add_invalidate( CACHE_RUN_SPEED );

      e.player->buffs.surekian_grace = b_speed;

      auto b_stack = create_buff<buff_t>( e.player, e.player->find_spell( 448433 ) );
      b_stack->set_default_value( data->effectN( 7 ).percent() / b_stack->max_stack() )
        ->set_expire_callback( [ b_speed ]( buff_t* b, int s, timespan_t ) {
          b_speed->trigger( 1, b->default_value * s );
        } );

      e.player->register_movement_callback( [ b_stack ]( bool start ) {
        if ( start )
          b_stack->expire();
      } );

      auto b_stance = create_buff<buff_t>( e.player, e.player->find_spell( 448036 ) )
        ->set_tick_callback( [ b_stack ]( buff_t*, int, timespan_t ) {
          b_stack->trigger();
        } );

      stance.emplace_back( b_dam, b_stance );

      // adjust for thewarwithin.sikrans.shadow_arsenal_stance= option
      const auto& option = e.player->thewarwithin_opts.sikrans_endless_arsenal_stance;
      if ( !option.is_default() )
      {
        if ( util::str_compare_ci( option, "decimation" ) )
          std::rotate( stance.begin(), stance.begin() + 1, stance.end() );
        else if ( util::str_compare_ci( option, "barrage" ) )
          std::rotate( stance.begin(), stance.begin() + 2, stance.end() );
        else if ( !util::str_compare_ci( option, "flourish" ) )
          throw std::invalid_argument( "Valid thewarwithin.sikrans.shadow_arsenal_stance: flourish, decimation, barrage" );
      }

      e.player->register_precombat_begin( [ this ]( player_t* ) {
        cycle_stance( false );
      } );
    }

    void cycle_stance( bool action = true )
    {
      stance.back().second->expire();

      if ( action && target )
        stance.front().first->execute_on_target( target );

      stance.front().second->trigger();

      std::rotate( stance.begin(), stance.begin() + 1, stance.end() );
    }

    void execute() override
    {
      generic_proc_t::execute();
      cycle_stance();
    }
  };

  effect.has_use_damage_override = true;
  effect.execute_action = create_proc_action<sikrans_endless_arsenal_t>( "sikrans_endless_arsenal", effect, data );
}

// 444292 equip
//  e1: damage
//  e2: shield cap
//  e3: repeating period
// 444301 on-use
// 447093 scarab damage
// 447097 'retrieval'?
// 447134 shield
// TODO: determine absorb priority
// TODO: determine how scarabs select target
// TODO: create proxy action if separate equip/on-use reporting is needed
void swarmlords_authority( special_effect_t& effect )
{
  unsigned equip_id = 444292;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Swarmlord's Authority missing equip effect" );

  auto data = equip->driver();

  struct ravenous_scarab_t : public generic_proc_t
  {
    struct ravenous_scarab_buff_t : public absorb_buff_t
    {
      double absorb_pct;

      ravenous_scarab_buff_t( const special_effect_t& e, const spell_data_t* s, const spell_data_t* data )
        : absorb_buff_t( e.player, "ravenous_scarab", s ), absorb_pct( s->effectN( 2 ).percent() )
      {
        cumulative = true;

        // TODO: set high priority & add to player absorb priority if necessary
        set_default_value( data->effectN( 2 ).average( e ) );
      }

      double consume( double a, action_state_t* s ) override
      {
        return absorb_buff_t::consume( a * absorb_pct, s );
      }
    };

    buff_t* shield;
    double return_speed;

    ravenous_scarab_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "ravenous_scarab", e.trigger() )
    {
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e );

      auto return_spell = e.player->find_spell( 447097 );
      shield = make_buff<ravenous_scarab_buff_t>( e, return_spell->effectN( 2 ).trigger(), data );
      return_speed = return_spell->missile_speed();
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      // TODO: determine how shield behaves during on-use
      make_event( *sim, timespan_t::from_seconds( player->get_player_distance( *s->target ) / return_speed ), [ this ] {
        shield->trigger();
      } );
    }
  };

  auto scarab = create_proc_action<ravenous_scarab_t>( "ravenous_scarab", effect, data );

  // setup equip
  equip->execute_action = scarab;
  new dbc_proc_callback_t( effect.player, *equip );

  // setup on-use
  effect.custom_buff = create_buff<buff_t>( effect.player, effect.driver() )
    ->set_tick_callback( [ scarab ]( buff_t*, int, timespan_t ) { scarab->execute(); } );
}

// 444264 on-use
// 444258 data + heal driver
// 446805 heal
// 446886 hp buff
// 446887 shield, unknown use
// TODO: determine out of combat decay
void foul_behemoths_chelicera( special_effect_t& effect )
{
  struct digestive_venom_t : public generic_proc_t
  {
    struct tasty_juices_t : public generic_heal_t
    {
      buff_t* buff;

      tasty_juices_t( const special_effect_t& e, const spell_data_t* data )
        : generic_heal_t( e, "tasty_juices", 446805 )
      {
        base_dd_min = base_dd_max = data->effectN( 2 ).average( e );

        // TODO: determine out of combat decay
        buff = create_buff<buff_t>( e.player, e.player->find_spell( 446886 ) )
          ->set_expire_callback( [ this ]( buff_t* b, int, timespan_t ) {
            player->resources.temporary[ RESOURCE_HEALTH ] -= b->current_value;
            player->recalculate_resource_max( RESOURCE_HEALTH );
          } );
      }

      void impact( action_state_t* s ) override
      {
        generic_heal_t::impact( s );

        if ( !buff->check() )
          buff->trigger();

        // overheal is result_total - result_amount
        auto overheal = s->result_total - s->result_amount;
        if ( overheal > 0 )
        {
          buff->current_value += overheal;
          player->resources.temporary[ RESOURCE_HEALTH ] += overheal;
          player->recalculate_resource_max( RESOURCE_HEALTH );
        }
      }
    };

    dbc_proc_callback_t* cb;

    digestive_venom_t( const special_effect_t& e ) : generic_proc_t( e, "digestive_venom", e.driver() )
    {
      auto data = e.player->find_spell( 444258 );
      base_td = data->effectN( 1 ).average( e ) * ( base_tick_time / dot_duration );

      auto driver = new special_effect_t( e.player );
      driver->name_str = data->name_cstr();
      driver->spell_id = data->id();
      driver->execute_action = create_proc_action<tasty_juices_t>( "tasty_juices", e, data );
      e.player->special_effects.push_back( driver );

      cb = new dbc_proc_callback_t( e.player, *driver );
      cb->deactivate();
    }

    void trigger_dot( action_state_t* s ) override
    {
      generic_proc_t::trigger_dot( s );

      cb->activate();
    }

    void last_tick( dot_t* d ) override
    {
      generic_proc_t::last_tick( d );

      cb->deactivate();
    }
  };

  effect.execute_action = create_proc_action<digestive_venom_t>( "digestive_venom", effect );
}

// 445066 equip + data
//  e1: primary
//  e2: secondary
//  e3: max stack
//  e4: reduction
//  e5: unreduced cap
//  e6: period
// 445560 on-use
// 449578 primary buff
// 449581 haste buff
// 449593 crit buff
// 449594 mastery buff
// 449595 vers buff
// TODO: confirm secondary precedence in case of tie is vers > mastery > haste > crit
void ovinaxs_mercurial_egg( special_effect_t& effect )
{
  unsigned equip_id = 445066;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Ovinax's Mercurial Egg missing equip effect" );

  auto data = equip->driver();

  struct ovinax_stat_buff_t : public stat_buff_t
  {
    double cap_mul;
    int cap;

    ovinax_stat_buff_t( player_t* p, std::string_view n, const spell_data_t* s, const spell_data_t* data )
      : stat_buff_t( p, n, s ),
        cap_mul( 1.0 - data->effectN( 4 ).percent() ),
        cap( as<int>( data->effectN( 5 ).base_value() ) )
    {}

    // values can be off by a +/-2 due to unknown rounding being performed by the in-game script
    double buff_stat_stack_amount( const buff_stat_t& stat, int s ) const override
    {
      double val = std::max( 1.0, std::fabs( stat.amount ) );
      double stack = s <= cap ? s : cap + ( s - cap ) * cap_mul;
      // TODO: confirm truncation happens on final amount, and not per stack amount
      return std::copysign( std::trunc( stack * val + 1e-3 ), stat.amount );
    }
  };

  // setup stat buffs
  auto primary = create_buff<ovinax_stat_buff_t>( effect.player, effect.player->find_spell( 449578 ), data );
  primary->set_stat_from_effect_type( A_MOD_STAT, data->effectN( 1 ).average( effect ) )
         ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  static constexpr std::array<unsigned, 4> buff_ids = { 449595, 449594, 449581, 449593 };

  std::unordered_map<stat_e, ovinax_stat_buff_t*> secondaries;

  for ( size_t i = 0; i < secondary_ratings.size(); i++ )
  {
    auto stat_str = util::stat_type_abbrev( secondary_ratings[ i ] );
    auto spell = effect.player->find_spell( buff_ids[ i ] );
    auto name = fmt::format( "{}_{}", util::tokenize_fn( spell->name_cstr() ), stat_str );

    auto buff = create_buff<ovinax_stat_buff_t>( effect.player, name, spell, data );
    buff->set_stat_from_effect_type( A_MOD_RATING, data->effectN( 2 ).average( effect ) )
        ->set_name_reporting( stat_str )
        ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

    secondaries[ secondary_ratings[ i ] ] = buff;
  }

  // proxy buff for on-use
  auto halt = create_buff<buff_t>( effect.player, effect.driver() )->set_cooldown( 0_ms );

  // proxy buff for equip ticks
  auto ticks = create_buff<buff_t>( effect.player, equip->driver() )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_tick_callback( [ primary, secondaries, halt, p = effect.player ]( buff_t*, int, timespan_t ) {
      if ( halt->check() )
        return;

      // if player is moving decrement stack. if player is above desired primary stack and not casting, assume player
      // will sidestep to try to decrement.
      auto desired = p->thewarwithin_opts.ovinaxs_mercurial_egg_desired_primary_stacks;
      // randomly add leeway to desired to simulate player reaction
      auto leeway = p->thewarwithin_opts.ovinaxs_mercurial_egg_desired_primary_stacks_leeway;
      desired += p->rng().range( -leeway, leeway );

      if ( ( primary->check() > desired && ( !p->executing || p->executing->usable_moving() ) &&
             ( !p->channeling || p->channeling->usable_moving() ) ) ||
           p->is_moving() )
      {
        primary->decrement();

        // stat selection only updates on increment
        auto buff = secondaries.at( util::highest_stat( p, secondary_ratings ) );
        int stack = 1;

        if ( !buff->check() )  // new stat, expire all stats first
        {
          range::for_each( secondaries, []( const auto& b ) { b.second->expire(); } );

          stack = primary->max_stack() - primary->check();
        }

        if ( !buff->at_max_stacks() )
          buff->trigger( stack );
      }
      else
      {
        range::for_each( secondaries, []( const auto& b ) { b.second->decrement(); } );

        if ( !primary->at_max_stacks() )
          primary->trigger();
      }
    } );

  effect.player->register_precombat_begin(
      [ ticks, primary, secondaries ]( player_t* p ) {
        auto p_stacks = p->thewarwithin_opts.ovinaxs_mercurial_egg_initial_primary_stacks;
        auto s_stacks = primary->max_stack() - p_stacks;

        if ( p_stacks )
        {
          primary->trigger( p_stacks );
        }

        if ( s_stacks )
        {
          auto buff = secondaries.at( util::highest_stat( p, secondary_ratings ) );
          buff->trigger( s_stacks );
        }

        make_event( *p->sim, p->rng().range( 1_ms, ticks->buff_period ), [ ticks ] { ticks->trigger(); } );
      } );

  effect.custom_buff = halt;
}

// 449946 on use
// 446209 stat buff value container
// 449947 jump task
// 449952 collect orb
// 449948 stand here task
// 450025 stand here task AoE DR/stacking trigger
// 449954 primary buff
// TODO: Implement precombat use with timing option.
struct do_treacherous_transmitter_task_t : public action_t
{
  buff_t* task = nullptr;

  do_treacherous_transmitter_task_t( player_t* p, std::string_view opt )
    : action_t( ACTION_OTHER, "do_treacherous_transmitter_task", p, spell_data_t::nil() )
  {
    parse_options( opt );

    s_data_reporting   = p->find_spell( 446209 );
    name_str_reporting = "Complete Task";

    callbacks = harmful = false;
    trigger_gcd         = 0_ms;
  }

  bool ready() override
  {
    if ( task != nullptr )
    {
      // Set the minimum time it would reasonably take to complete the task
      // Prevents using the trinket and instantly completing the tasks, which is unreasonable.
      // Starting at a 1.5s minimum for all tasks
      if ( task->remains() > task->buff_duration() - 1.5_s )
        return false;

      switch ( task->data().id() )
      {
        case 449947:
          // Jumping takes almost no time, 1.5s minimum is fine.
          return task->check();
          break;
        case 449952:
          // Collecting the orb should take a bit longer, due to its movement, 2s minimum should be fine.
          if ( task->remains() <= task->buff_duration() - 2_s )
            return task->check();
          else
            return false;
          break;
        case 449948:
          // If the task is stand here, account for the movement time, and 2s for the stacking buff to occur.
          if ( task->remains() <= task->buff_duration() - 3_s )
            return task->check();
          else
            return false;
          break;
        default:
          return false;
          break;
      }
    }
    else
      return false;
  }

  void execute() override
  {
    task->expire();
  }
};

void treacherous_transmitter( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs(
           effect, { "cryptic_instructions", "errant_manaforge_emission", "realigning_nexus_convergence_divergence",
                     "ethereal_powerlink" } ) )
    return;

  struct cryptic_instructions_t : public generic_proc_t
  {
    std::vector<buff_t*> tasks;
    buff_t* stat_buff;
    std::vector<action_t*> apl_actions;
    action_t* use_action;
    const special_effect_t& effect;
    timespan_t task_dur;

    cryptic_instructions_t( const special_effect_t& e )
      : generic_proc_t( e, "cryptic_instructions", e.driver() ),
        effect( e ),
        task_dur( 0_ms )
    {
      harmful            = false;
      cooldown->duration = 0_ms;  // Handled by the item

      stat_buff =
          create_buff<stat_buff_t>( e.player, e.player->find_spell( 449954 ) )
              ->set_stat_from_effect_type( A_MOD_STAT, e.player->find_spell( 446209 )->effectN( 1 ).average( e ) );

      for ( auto& a : e.player->action_list )
      {
        if ( a->name_str == "do_treacherous_transmitter_task" )
        {
          apl_actions.push_back( a );
        }
        if ( a->action_list && a->action_list->name_str == "precombat" && a->name_str == "use_item_" + item->name_str )
        {
          a->harmful = harmful;
          use_action = a;
        }
      }

      if ( apl_actions.size() > 0 || use_action != nullptr )
      {
        buff_t* jump_task   = create_buff<buff_t>( e.player, e.player->find_spell( 449947 ) );
        buff_t* collect_orb = create_buff<buff_t>( e.player, e.player->find_spell( 449952 ) );
        buff_t* stand_here  = create_buff<buff_t>( e.player, e.player->find_spell( 449948 ) );

        tasks.push_back( jump_task );
        tasks.push_back( collect_orb );
        tasks.push_back( stand_here );

        for ( auto& t : tasks )
        {
          t->set_expire_callback( [ & ]( buff_t*, int, timespan_t d ) {
            if ( d > 0_ms )
            {
              stat_buff->trigger();
            }
          } );
        }

        // Set a default task for the actions ready() function, will be overwritten later
        for ( auto& a : apl_actions )
        {
          debug_cast<do_treacherous_transmitter_task_t*>( a )->task = tasks[ 0 ];
        }
      }

      task_dur = e.player->find_spell( 449947 )->duration();
    }

    void precombat_buff()
    {
      // shared cd (other trinkets & on-use items)
      auto cdgrp = player->get_cooldown( effect.cooldown_group_name() );

      timespan_t time = 0_ms;

      if ( time == 0_ms )  // No global override, check for an override from an APL variable
      {
        for ( auto v : player->variables )
        {
          if ( v->name_ == "treacherous_transmitter_precombat_cast" )
          {
            time = timespan_t::from_seconds( v->value() );
            break;
          }
        }
      }

      const auto& apl = player->precombat_action_list;

      auto it = range::find( apl, use_action );
      if ( it == apl.end() )
      {
        sim->print_debug(
            "WARNING: Precombat /use_item for Treacherous Transmitter exists but not found in precombat APL!" );
        return;
      }

      cdgrp->start( 1_ms );  // tap the shared group cd so we can get accurate action_ready() checks

      // total duration of the buff
      auto total = tasks[ 0 ]->buff_duration();
      // actual duration of the buff you'll get in combat
      auto actual = total - time;
      // cooldown on effect/trinket at start of combat
      // auto cd_dur = cooldown->duration - time;
      // shared cooldown at start of combat
      // auto cdgrp_dur = std::max( 0_ms, effect.cooldown_group_duration() - time );

      sim->print_debug( "PRECOMBAT: Treacherous Transmitter started {}s before combat via {}, {}s in-combat buff", time,
                        "APL", actual );

      tasks[ 0 ]->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, actual );

      if ( use_action )  // from the apl, so cooldowns will be started by use_item_t. adjust. we are still in precombat.
      {
        make_event( *sim, [ this, time, cdgrp ] {  // make an event so we adjust after cooldowns are started
          cooldown->adjust( -time );

          if ( use_action )
            use_action->cooldown->adjust( -time );

          cdgrp->adjust( -time );
        } );
      }
      if ( apl_actions.size() == 0 )
      {
        make_event( *sim, rng().range( 0_ms, actual - 1_ms ), [ this ] {
          tasks[ 0 ]->expire();
          stat_buff->trigger();
        } );
      }
    }

    void execute() override
    {
      if ( !player->in_combat && use_action )
      {
        precombat_buff();
        return;
      }

      generic_proc_t::execute();

      if ( apl_actions.size() > 0 )
      {
        rng().shuffle( tasks.begin(), tasks.end() );
        for ( auto& a : apl_actions )
        {
          debug_cast<do_treacherous_transmitter_task_t*>( a )->task = tasks[ 0 ];
        }
        tasks[ 0 ]->trigger();
      }
      else
      {
        make_event( *sim, rng().gauss_ab( 6_s, 3_s, 3_s, task_dur ), [ this ] { stat_buff->trigger(); } );
      }
    }
  };

  effect.disable_buff();
  effect.stat           = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );
  effect.execute_action = create_proc_action<cryptic_instructions_t>( "cryptic_instructions", effect );
}

// 443124 on-use
//  e1: damage
//  e2: trigger heal return
// 443128 coeffs
//  e1: damage
//  e2: heal
//  e3: unknown
//  e4: cdr on kill
// 446067 heal
// 455162 heal return
//  e1: dummy
//  e2: trigger heal
// TODO: confirm cast time is hasted
void mad_queens_mandate( special_effect_t& effect )
{
  unsigned coeff_id = 443128;
  auto coeff = find_special_effect( effect.player, coeff_id );
  assert( coeff && "Mad Queen's Mandate missing coefficient effect" );

  struct abyssal_gluttony_t : public generic_proc_t
  {
    cooldown_t* item_cd;
    action_t* heal;
    timespan_t cdr;
    double heal_speed;
    double hp_mul;

    abyssal_gluttony_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "abyssal_gluttony", e.driver() ),
        item_cd( e.player->get_cooldown( e.cooldown_name() ) ),
        cdr( timespan_t::from_seconds( data->effectN( 4 ).base_value() ) ),
        heal_speed( e.trigger()->missile_speed() ),
        hp_mul( 0.5 ) // not present in spell data
    {
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );

      heal = create_proc_action<generic_heal_t>( "abyssal_gluttony_heal", e, "abyssal_gluttony_heal",
                                                 e.trigger()->effectN( 2 ).trigger() );
      heal->name_str_reporting = "abyssal_gluttony";
      heal->base_td = data->effectN( 2 ).average( e ) * ( heal->base_tick_time / heal->dot_duration );
    }

    bool usable_moving() const override { return true; }

    void execute() override
    {
      bool was_sleeping = target->is_sleeping();

      generic_proc_t::execute();

      auto delay = timespan_t::from_seconds( player->get_player_distance( *target ) / heal_speed );
      make_event( *sim, delay, [ this ] { heal->execute(); } );

      // Assume if target wasn't sleeping but is now, it was killed by the damage
      if ( !was_sleeping && target->is_sleeping() )
      {
        cooldown->adjust( -cdr );
        item_cd->adjust( -cdr );
      }
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      auto mul = 1.0 + hp_mul * ( 100 - t->health_percentage() ) * 0.01;

      heal->base_td_multiplier = mul;

      return generic_proc_t::composite_target_multiplier( t ) * mul;
    }
  };

  effect.execute_action = create_proc_action<abyssal_gluttony_t>( "abyssal_gluttony", effect, coeff->driver() );
}

// Sigil of Algari Concordance
// Might have variations based on the players Specilization.
// Testing on DPS Death Knight showed only a single spawnable pet.
// 443378 Driver
// Effect 2: Thunder Bolt Damage
// Effect 3: Bolt Rain Damage
// Effect 4: Thundering Bolt Damage
// Effect 5: Unknown
// Effect 6: Unknown
// Effect 7: Unknown
// Effect 8: Unknown
// Effect 9: Mighty Smash Damage
// Effect 10: Earthen Ire Damage
// 452310 Summon Spell (DPS)
// 452335 Thunder Bolt - Damage Spell DPS Single Target
// 452334 Bolt Rain - Damage Spell DPS AoE
// 452445 Thundering Bolt - One Time Spell DPS
// 452496 Summon Spell (Tank)
// 452545 Mighty Smash - Damage Spell Tank
// 452514 Earthen Ire - One Time Spell Tank
// 452890 Earthen Ire - Ticking Damage
// 452325, 452457, and 452498 Periodic Triggers for pet actions
void sigil_of_algari_concordance( special_effect_t& e )
{
  struct algari_pet_cast_event_t : public event_t
  {
    action_t* st_action;
    action_t* aoe_action;
    action_t* one_time_action;
    unsigned tick;
    pet_t* pet;
    timespan_t period;

    algari_pet_cast_event_t( pet_t* p, timespan_t time_to_execute, unsigned tick, action_t* st_action,
                             action_t* aoe_action, action_t* one_time_action )
      : event_t( *p, time_to_execute ),
        st_action( st_action ),
        aoe_action( aoe_action ),
        one_time_action( one_time_action ),
        tick( tick ),
        pet( p ), 
        period( 0_ms )
    {
      period = pet->find_spell( 452325 )->effectN( 1 ).period();
    }

    const char* name() const override
    {
      return "algari_pet_cast";
    }

    void execute() override
    {
      if ( pet->is_active() )
      {
        tick++;
        // Emulates the odd behavior where sometimes it will execute the one time action the first tick
        // while other times it will execute it later. While still guarenteeing it will cast sometime in its duration.
        if ( one_time_action != nullptr && rng().roll( 0.33 * tick ) )
        {
          one_time_action->execute();
          one_time_action = nullptr;
        }
        else if ( aoe_action != nullptr && pet->sim->target_non_sleeping_list.size() > 2 )
        {
          aoe_action->execute();
        }
        else
        {
          st_action->execute();
        }
        make_event<algari_pet_cast_event_t>( sim(), pet, period, tick,
                                             st_action, aoe_action, one_time_action );
      }
    }
  };

  struct sigil_of_algari_concordance_pet_t : public unique_gear_pet_t
  {
    action_t* st_action;
    action_t* one_time_action;
    action_t* aoe_action;

    sigil_of_algari_concordance_pet_t( std::string_view name, const special_effect_t& e,
                                       const spell_data_t* summon_spell )
      : unique_gear_pet_t( name, e, summon_spell ),
        st_action( nullptr ),
        one_time_action( nullptr ),
        aoe_action( nullptr )
    {
      npc_id = summon_spell->effectN( 1 ).misc_value1();
    }

    void arise() override
    {
      unique_gear_pet_t::arise();
      make_event<algari_pet_cast_event_t>( *sim, this, 0_ms, 0, st_action, aoe_action, one_time_action );
    }
  };

  struct algari_concodance_pet_spell_t : public spell_t
  {
    unsigned max_scaling_targets;
    bool scale_aoe_damage;
    algari_concodance_pet_spell_t( std::string_view n, pet_t* p, const spell_data_t* s, action_t* a )
      : spell_t( n, p, s ), max_scaling_targets( 5 ), scale_aoe_damage( false )
    {
      background = true;
      auto proxy = a;
      auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );

      // TODO: determine if these are affected by role mult
    }

    player_t* p() const
    {
      return debug_cast<player_t*>( debug_cast<pet_t*>( this->player )->owner );
    }

    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double am = spell_t::composite_aoe_multiplier( state );

      if( scale_aoe_damage )
        am *= 1.0 + 0.15 * clamp( state->n_targets - 1u, 0u, max_scaling_targets );

      return am;
    }
  };

  struct thunder_bolt_silvervein_t : public algari_concodance_pet_spell_t
  {

    thunder_bolt_silvervein_t( std::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : algari_concodance_pet_spell_t( name, p, p->find_spell( 452335 ), a )
    {
      name_str_reporting = "thunder_bolt";
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );
    }
  };

  struct bolt_rain_t : public algari_concodance_pet_spell_t
  {
    bolt_rain_t( std::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : algari_concodance_pet_spell_t( name, p, p->find_spell( 452334 ), a )
    {
      aoe = -1;
      split_aoe_damage = true;
      scale_aoe_damage = true;
      base_dd_min = base_dd_max = e.driver()->effectN( 3 ).average( e );
    }
  };

  struct thundering_bolt_t : public algari_concodance_pet_spell_t
  {
    thundering_bolt_t( std::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : algari_concodance_pet_spell_t( name, p, p->find_spell( 452445 ), a )
    {
      aoe = -1;
      split_aoe_damage = true;
      scale_aoe_damage = true;
      base_dd_min = base_dd_max = e.driver()->effectN( 4 ).average( e );
    }
  };

  struct mighty_smash_t : public algari_concodance_pet_spell_t
  {
    mighty_smash_t( std::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : algari_concodance_pet_spell_t( name, p, p->find_spell( 452545 ), a )
    {
      aoe = -1;
      split_aoe_damage = true;
      scale_aoe_damage = true;
      base_dd_min = base_dd_max = e.driver()->effectN( 9 ).average( e );
    }
  };

  struct earthen_ire_buff_t : public algari_concodance_pet_spell_t
  {
    earthen_ire_buff_t( std::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : algari_concodance_pet_spell_t( name, p, e.player->find_spell( 452518 ), a )
    {
      background = true;
    }

    void execute() override
    {
      algari_concodance_pet_spell_t::execute();
      p()->buffs.earthen_ire->trigger();
    }
  };

  struct silvervein_pet_t : public sigil_of_algari_concordance_pet_t
  {
    action_t* action;
    const special_effect_t& effect;
    silvervein_pet_t( const special_effect_t& e, action_t* a = nullptr, const spell_data_t* summon_spell = nullptr )
      : sigil_of_algari_concordance_pet_t( "silvervein", e, summon_spell ), action( a ), effect( e )
    {
    }

    void create_actions() override
    {
      sigil_of_algari_concordance_pet_t::create_actions();
      st_action       = new thunder_bolt_silvervein_t( "thunder_bolt_silvervein", this, effect, action );
      aoe_action      = new bolt_rain_t( "bolt_rain", this, effect, action );
      one_time_action = new thundering_bolt_t( "thundering_bolt", this, effect, action );
    }
  };

  struct boulderbane_pet_t : public sigil_of_algari_concordance_pet_t
  {
    action_t* action;
    const special_effect_t& effect;
    boulderbane_pet_t( const special_effect_t& e, action_t* a = nullptr, const spell_data_t* summon_spell = nullptr )
      : sigil_of_algari_concordance_pet_t( "boulderbane", e, summon_spell ), action( a ), effect( e )
    {
    }

    void create_actions() override
    {
      sigil_of_algari_concordance_pet_t::create_actions();
      st_action       = new mighty_smash_t( "mighty_smash", this, effect, action );
      one_time_action = new earthen_ire_buff_t( "earthen_ire_buff", this, effect, action );
    }
  };

  struct sigil_of_algari_concordance_t : public generic_proc_t
  {
    const spell_data_t* summon_spell;
    spawner::pet_spawner_t<silvervein_pet_t> silvervein_spawner;
    spawner::pet_spawner_t<boulderbane_pet_t> boulderbane_spawner;
    bool silvervein;
    bool boulderbane;

    sigil_of_algari_concordance_t( const special_effect_t& e, action_t* earthen_ire_damage )
      : generic_proc_t( e, "sigil_of_algari_concordance", e.driver() ),
        summon_spell( nullptr ),
        silvervein_spawner( "silvervein", e.player ),
        boulderbane_spawner( "boulderbane", e.player ),
        silvervein( false ),
        boulderbane( false )
    {
      switch ( e.player->_spec )
      {
        case HUNTER_BEAST_MASTERY:
        case HUNTER_MARKSMANSHIP:
        case PRIEST_SHADOW:
        case SHAMAN_ELEMENTAL:
        case MAGE_ARCANE:
        case MAGE_FIRE:
        case MAGE_FROST:
        case WARLOCK_AFFLICTION:
        case WARLOCK_DEMONOLOGY:
        case WARLOCK_DESTRUCTION:
        case DRUID_BALANCE:
        case EVOKER_DEVASTATION:
        case EVOKER_AUGMENTATION:
          silvervein = true;
          break;
        case WARRIOR_PROTECTION:
        case PALADIN_PROTECTION:
        case DEATH_KNIGHT_BLOOD:
        case MONK_BREWMASTER:
        case DRUID_GUARDIAN:
        case DEMON_HUNTER_VENGEANCE:
          boulderbane = true;
          break;
        case PALADIN_HOLY:
        case PRIEST_DISCIPLINE:
        case PRIEST_HOLY:
        case SHAMAN_RESTORATION:
        case MONK_MISTWEAVER:
        case DRUID_RESTORATION:
        case EVOKER_PRESERVATION:
          break;
        case WARRIOR_ARMS:
        case WARRIOR_FURY:
        case PALADIN_RETRIBUTION:
        case HUNTER_SURVIVAL:
        case ROGUE_ASSASSINATION:
        case ROGUE_OUTLAW:
        case ROGUE_SUBTLETY:
        case DEATH_KNIGHT_FROST:
        case DEATH_KNIGHT_UNHOLY:
        case SHAMAN_ENHANCEMENT:
        case MONK_WINDWALKER:
        case DRUID_FERAL:
        case DEMON_HUNTER_HAVOC:
        default:
          silvervein = true;
          break;
      }

      if ( silvervein )
      {
        summon_spell = e.player->find_spell( 452310 );
        silvervein_spawner.set_creation_callback( [ & ]( player_t* ) { return new silvervein_pet_t( e, this, summon_spell ); } );
        silvervein_spawner.set_default_duration( summon_spell->duration() );
      }
      if ( boulderbane )
      {
        summon_spell = e.player->find_spell( 452496 );
        boulderbane_spawner.set_creation_callback( [ & ]( player_t* ) { return new boulderbane_pet_t( e, this, summon_spell ); } );
        boulderbane_spawner.set_default_duration( summon_spell->duration() );
        add_child( earthen_ire_damage );
      }

      background = true;
    }

    void execute() override
    {
      generic_proc_t::execute();
      if ( boulderbane )
      {
        boulderbane_spawner.spawn();
      }
      if ( silvervein )
      {
        silvervein_spawner.spawn();
      }
    }
  };

  auto earthen_ire_damage = create_proc_action<generic_aoe_proc_t>( "earthen_ire", e, e.player->find_spell( 452890 ), true );

  earthen_ire_damage->base_dd_min = earthen_ire_damage->base_dd_max = e.driver()->effectN( 10 ).average( e );

  auto earthen_ire_buff_spell = e.player->find_spell( 452514 );
  e.player->buffs.earthen_ire =
      create_buff<buff_t>( e.player, earthen_ire_buff_spell )
          ->set_tick_callback( [ earthen_ire_damage ]( buff_t*, int, timespan_t ) { earthen_ire_damage->execute(); } );

  e.execute_action =
      create_proc_action<sigil_of_algari_concordance_t>( "sigil_of_algari_concordance", e, earthen_ire_damage );
  new dbc_proc_callback_t( e.player, e );
}

// Skarmorak Shard
// 443407 Main Buff & Driver
// 449792 Stacking Buff
void skarmorak_shard( special_effect_t& e )
{
  auto main_buff = create_buff<stat_buff_t>( e.player, e.driver() )
                       ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 1 ).average( e ) );

  auto on_kill_buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 449792 ) )
                          ->add_stat_from_effect_type( A_MOD_RATING, e.player->find_spell( 449792 )->effectN( 1 ).average( e ) );

  e.player->register_on_kill_callback( [ e, on_kill_buff ]( player_t* ) {
    if ( !e.player->sim->event_mgr.canceled )
      on_kill_buff->trigger();
  } );

  e.custom_buff = main_buff;
}

// Void Pactstone
// 443537 Driver
// 450960 Damage
// 450962 Buff
void void_pactstone( special_effect_t& e )
{
  auto buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 450962 ) )
                  // Will Throw a warning currently, as the mod rating misc_value1 is empty. Does not work in game either.
                  ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 2 ).average( e ) );

  auto damage         = create_proc_action<generic_aoe_proc_t>( "void_pulse", e, 450960 );
  damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 1 ).average( e );
  damage->base_multiplier *= role_mult( e );
  damage->split_aoe_damage = true;

  e.custom_buff    = buff;
  e.execute_action = damage;
  new dbc_proc_callback_t( e.player, e );
}

// Ravenous Honey Buzzer
// 448904 Driver
// 448903 Trigger, forced movement
// 448909 Damage
// TODO: implement possible lost uptime from forced movement
void ravenous_honey_buzzer( special_effect_t& e )
{
  struct ravenous_honey_buzzer_t : public generic_aoe_proc_t
  {
    timespan_t movement_dur;

    ravenous_honey_buzzer_t( const special_effect_t& e )
      : generic_aoe_proc_t( e, "ravenous_honey_buzzer", e.player->find_spell( 448909 ), true ),
        movement_dur( timespan_t::from_seconds( e.trigger()->missile_speed() ) )
    {
      base_multiplier *= role_mult( e );
    }

    void execute() override
    {
      generic_aoe_proc_t::execute();

      // TODO: implement possible lost uptime from forced movement
      player->buffs.movement->trigger( movement_dur );
    }
  };

  e.execute_action = create_proc_action<ravenous_honey_buzzer_t>( "ravenous_honey_buzzer", e );
}

// Overclocked Gear-a-rang Launcher
// 443411 Use Driver
// 446764 Equip Driver
// 446811 Use Damage
// 449842 Ground Effect Trigger
// 449828 Equip Damage
// 450453 Equip Buff
// TODO: stagger travel time on targets to simulate the blade movement
void overclocked_geararang_launcher( special_effect_t& e )
{
  struct overclocked_strike_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;
    action_t* overclock_strike;

    overclocked_strike_cb_t( const special_effect_t& e, buff_t* buff, action_t* strike )
      : dbc_proc_callback_t( e.player, e ), buff( buff ), overclock_strike( strike )
    {
    }

    void execute( action_t*, action_state_t* s ) override
    {
      buff->expire();
      overclock_strike->execute_on_target( s->target );
    }
  };

  struct overclock_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;
    cooldown_t* item_cd;
    cooldown_t* shared_trinket_cd;
    const spell_data_t* equip_driver;

    overclock_cb_t( const special_effect_t& e, const special_effect_t& use, buff_t* buff )
      : dbc_proc_callback_t( e.player, e ),
        buff( buff ),
        item_cd( e.player->get_cooldown( use.cooldown_name() ) ),
        shared_trinket_cd( e.player->get_cooldown( "item_cd_" + util::to_string( use.driver()->category() ) ) ),
        equip_driver( e.driver() )
    {
    }

    void execute( action_t*, action_state_t* ) override
    {
      item_cd->adjust( timespan_t::from_seconds( -equip_driver->effectN( 1 ).base_value() ) );
      if ( listener->bugs )
      {
        shared_trinket_cd->adjust( timespan_t::from_seconds( -equip_driver->effectN( 1 ).base_value() ) );
      }
      buff->trigger();
    }
  };

  struct geararang_serration_t : public generic_proc_t
  {
    geararang_serration_t( const special_effect_t& e ) : generic_proc_t( e, "geararang_serration", 446811 )
    {
      aoe = -1;
      radius = e.driver()->effectN( 1 ).radius();
      base_multiplier *= role_mult( e );
      chain_multiplier = 0.95;  // not in spell data
      // TODO: stagger travel time on targets to simulate the blade movement
      range = 30;
      travel_speed = 20;  // guessed ~1.5s to reach max range of ~30yd
    }

    std::vector<player_t*>& target_list() const override
    {
      // to simulate mobs not always grouping up in the exact same order, re-create the target list every time.
      available_targets( target_cache.list );
      player->rng().shuffle( target_cache.list.begin(), target_cache.list.end() );

      // duplicate the list add in reverse to simulate the blade going out then returning
      std::vector<player_t*> tmp_tl = target_cache.list;  // make a copy

      while ( !tmp_tl.empty() )
      {
        target_cache.list.push_back( tmp_tl.back() );
        tmp_tl.pop_back();
      }

      return target_cache.list;
    }
  };

  unsigned equip_id = 446764;
  auto equip        = find_special_effect( e.player, equip_id );
  assert( equip && "Overclocked Gear-a-Rang missing equip effect" );

  auto equip_driver = equip->driver();

  auto damage_buff_spell = e.player->find_spell( 450453 );
  auto overclock_buff    = create_buff<buff_t>( e.player, damage_buff_spell );

  auto overclock_strike = create_proc_action<generic_proc_t>( "overclocked_strike", e, e.player->find_spell( 449828 ) );
  overclock_strike->base_dd_min = overclock_strike->base_dd_max = equip_driver->effectN( 2 ).average( e );
  overclock_strike->base_multiplier *= role_mult( e );

  auto damage          = new special_effect_t( e.player );
  damage->name_str     = "overclocked_strike_proc";
  damage->item         = e.item;
  damage->spell_id     = damage_buff_spell->id();
  damage->cooldown_    = 1_ms; // Artificial tiny ICD to prevent double proccing before the buff expires
  damage->proc_flags2_ = PF2_ALL_HIT;
  e.player->special_effects.push_back( damage );

  auto damage_cb = new overclocked_strike_cb_t( *damage, overclock_buff, overclock_strike );
  damage_cb->activate_with_buff( overclock_buff, true );

  auto overclock      = new special_effect_t( e.player );
  overclock->name_str = "overclock";
  overclock->item     = e.item;
  overclock->spell_id = equip_driver->id();
  e.player->special_effects.push_back( overclock );

  auto overclock_cb = new overclock_cb_t( *overclock, e, overclock_buff );
  overclock_cb->initialize();
  overclock_cb->activate();

  e.execute_action = create_proc_action<geararang_serration_t>( "geararang_serration", e );
  e.execute_action->add_child( overclock_strike );
}

// Remnant of Darkness
// 443530 Driver
// 451369 Buff
// 452032 Damage
// 451602 Transform Buff
void remnant_of_darkness( special_effect_t& e )
{
  auto damage         = create_proc_action<generic_aoe_proc_t>( "dark_swipe", e, 452032 );
  damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 2 ).average( e );
  damage->base_multiplier *= role_mult( e );

  auto transform_buff = create_buff<buff_t>( e.player, e.player->find_spell( 451602 ) )
                            ->set_tick_callback( [ damage ]( buff_t*, int, timespan_t ) { damage->execute(); } );

  auto stat_buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 451369 ) )
                       ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) )
                       ->set_stack_change_callback( [ transform_buff ]( buff_t* b, int, int ) {
                         if ( b->at_max_stacks() )
                         {
                           transform_buff->trigger();
                         }
                       } );

  transform_buff->set_expire_callback( [ stat_buff ]( buff_t*, int, timespan_t ) { stat_buff->expire(); } );

  e.custom_buff = stat_buff;
  new dbc_proc_callback_t( e.player, e );
}

// Opressive Orator's Larynx
// 443552 Use Driver
// 446787 Equip Driver
// 451011 Buff
// 451015 Damage
void opressive_orators_larynx( special_effect_t& e )
{
  struct dark_oration_t : public generic_aoe_proc_t
  {
    double mult;
    dark_oration_t( const special_effect_t& e, const spell_data_t* equip_driver )
      : generic_aoe_proc_t( e, "dark_oration", e.player->find_spell( 451015 ) ), mult( 0 )
    {
      background  = true;
      base_dd_min = base_dd_max = equip_driver->effectN( 2 ).average( e );
      base_multiplier *= role_mult( e );
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = generic_proc_t::composite_da_multiplier( state );

      m *= 1.0 + mult;

      return m;
    }
  };

  struct dark_oration_tick_t : public generic_proc_t
  {
    buff_t* buff;
    action_t* damage;
    double increase_per_stack;
    dark_oration_tick_t( const special_effect_t& e, buff_t* buff, action_t* damage )
      : generic_proc_t( e, "dark_oration", e.driver() ), buff( buff ), damage( damage ), increase_per_stack( 0 )
    {
      background         = true;
      tick_action        = damage;
      increase_per_stack = 0.25;  // Found through testing, doesnt appear to be in data
    }

    void execute() override
    {
      debug_cast<dark_oration_t*>( damage )->mult = buff->stack() * increase_per_stack;
      buff->expire();
      generic_proc_t::execute();
    }
  };

  unsigned equip_id = 446787;
  auto equip_effect = find_special_effect( e.player, equip_id );
  assert( equip_effect && "Opressive Orator's Larynx missing equip effect" );

  auto equip_driver = equip_effect->driver();

  auto buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 451011 ) )
                  ->add_stat_from_effect_type( A_MOD_STAT, equip_driver->effectN( 1 ).average( e ) )
                  ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

  auto equip         = new special_effect_t( e.player );
  equip->name_str    = "dark_orators_larynx";
  equip->item        = e.item;
  equip->spell_id    = equip_driver->id();
  equip->custom_buff = buff;
  e.player->special_effects.push_back( equip );

  auto equip_cb = new dbc_proc_callback_t( e.player, *equip );
  equip_cb->initialize();
  equip_cb->activate();

  auto damage        = create_proc_action<dark_oration_t>( "dark_oration", e, equip_driver );
  auto ticking_spell = create_proc_action<dark_oration_tick_t>( "dark_oration_tick", e, buff, damage );

  e.execute_action = ticking_spell;
}

// Ara-Kara Sacbrood
// 443541 Driver
// 452146 Buff
// 452226 Spiderling Buff
// 452227 Spiderfling Missile
// 452229 Damage
void arakara_sacbrood( special_effect_t& e )
{
  struct spiderfling_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    action_t* missile;
    buff_t* buff;
    spiderfling_cb_t( const special_effect_t& e, buff_t* buff )
      : dbc_proc_callback_t( e.player, e ), damage( nullptr ), missile( nullptr ), buff( buff )
    {
      damage                 = create_proc_action<generic_proc_t>( "spidersting", e, e.player->find_spell( 452229 ) );
      damage->base_td        = e.player->find_spell( 443541 )->effectN( 2 ).average( e );
      damage->base_multiplier *= role_mult( e.player, e.player->find_spell( 443541 ) );
      missile                = create_proc_action<generic_proc_t>( "spiderfling", e, e.player->find_spell( 452227 ) );
      missile->impact_action = damage;
    }

    void execute( action_t*, action_state_t* s ) override
    {
      missile->execute_on_target( s->target );
      buff->decrement();
    }
  };

  // In game this buff seems to stack infinitely, while data suggests 1 max stack.
  auto spiderling_buff = create_buff<buff_t>( e.player, e.player->find_spell( 452226 ) )
                         ->set_max_stack( 99 );

  auto spiderling      = new special_effect_t( e.player );
  spiderling->name_str = "spiderling";
  spiderling->item     = e.item;
  spiderling->spell_id = 452226;
  e.player->special_effects.push_back( spiderling );

  auto spiderling_cb = new spiderfling_cb_t( *spiderling, spiderling_buff );
  spiderling_cb->activate_with_buff( spiderling_buff, true );

  auto buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 452146 ) )
                  ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) )
                  ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                  ->set_stack_change_callback( [ spiderling_buff ]( buff_t*, int old_, int new_ ) {
                    if ( old_ > new_ )
                    {
                      spiderling_buff->trigger();
                    }
                  } );

  e.custom_buff = buff;
  new dbc_proc_callback_t( e.player, e );
}

// Skyterror's Corrosive Organ
// 444489 Use Driver
// 444488 Equip Driver
// 447471 DoT
// 447495 AoE damage
void skyterrors_corrosive_organ( special_effect_t& e )
{
  struct volatile_acid_splash_t : public generic_proc_t
  {
    action_t* dot;
    volatile_acid_splash_t( const special_effect_t& e, const spell_data_t* equip_driver, action_t* dot )
      : generic_proc_t( e, "volatile_acid_splash", e.player->find_spell( 447495 ) ), dot( dot )
    {
      background       = true;
      aoe              = data().max_targets();
      split_aoe_damage = false;
      base_dd_min = base_dd_max = equip_driver->effectN( 2 ).average( e );
      base_multiplier *= role_mult( e );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = generic_proc_t::composite_da_multiplier( s );

      m *= dot->get_dot( s->target )->current_stack();

      return m;
    }

    // Doesnt hit the target that triggered the effect
    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      generic_proc_t::available_targets( tl );

      auto it = range::find( tl, target );
      if ( it != tl.end() )
      {
        tl.erase( it );
      }

      return tl.size();
    }
  };

  unsigned equip_id = 444488;
  auto equip        = find_special_effect( e.player, equip_id );
  assert( equip && "Skyterror's Corrosive Organ missing equip effect" );

  auto equip_driver   = equip->driver();
  auto dot            = create_proc_action<generic_proc_t>( "volatile_acid", e, 447471 );
  auto aoe_damage     = create_proc_action<volatile_acid_splash_t>( "volatile_acid_splash", e, equip_driver, dot );
  dot->dot_behavior   = DOT_NONE;  // Doesnt Refresh, just stacks
  dot->base_td        = equip_driver->effectN( 1 ).average( e ) * role_mult( e );
  dot->execute_action = aoe_damage;
  dot->add_child( aoe_damage );

  auto volatile_acid            = new special_effect_t( e.player );
  volatile_acid->name_str       = "volatile_acid";
  volatile_acid->spell_id       = equip_driver->id();
  volatile_acid->execute_action = dot;
  e.player->special_effects.push_back( volatile_acid );

  auto volatile_acid_proc = new dbc_proc_callback_t( e.player, *volatile_acid );
  volatile_acid_proc->initialize();
  volatile_acid_proc->activate();

  e.player->callbacks.register_callback_trigger_function(
      volatile_acid->spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ dot ]( const dbc_proc_callback_t*, action_t*, action_state_t* s ) {
        return dot->get_dot( s->target )->is_ticking();
      } );

  e.execute_action = dot;
}

// 443415 driver
//  e1: trigger damage with delay
//  e2: damage coeff
//  e3: buff coeff
// 450921 damage
// 450922 area trigger
// 451247 buff return missile
// 451248 buff
void high_speakers_accretion( special_effect_t& effect )
{
  struct high_speakers_accretion_t : public generic_proc_t
  {
    struct high_speakers_accretion_damage_t : public generic_aoe_proc_t
    {
      std::vector<player_t*>& targets_hit;

      high_speakers_accretion_damage_t( const special_effect_t& e, std::vector<player_t*>& tl )
        : generic_aoe_proc_t( e, "high_speakers_accretion_damage", 450921, true ), targets_hit( tl )
      {
        base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );
        base_multiplier *= role_mult( e );
      }

      void impact( action_state_t* s ) override
      {
        generic_aoe_proc_t::impact( s );

        if ( as<unsigned>( targets_hit.size() ) < 5u && !range::contains( targets_hit, s->target ) )
          targets_hit.emplace_back( s->target );
      }
    };

    ground_aoe_params_t params;
    std::vector<player_t*> targets_hit;

    high_speakers_accretion_t( const special_effect_t& e ) : generic_proc_t( e, "high_speakers_accretion", e.driver() )
    {
      auto damage =
        create_proc_action<high_speakers_accretion_damage_t>( "high_speakers_accretion_damage", e, targets_hit );
      damage->stats = stats;

      auto buff = create_buff<stat_buff_with_multiplier_t>( e.player, e.player->find_spell( 451248 ) );
      buff->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 3 ).average( e ) );

      params.pulse_time( 1_s )
          .duration( e.trigger()->duration() )
          .action( damage )
          .expiration_callback(
              [ this, buff, delay = timespan_t::from_seconds( e.player->find_spell( 451247 )->missile_speed() ) ](
                  const action_state_t* ) {
                if ( targets_hit.empty() )
                  buff->stat_mul = 1.0;
                else
                  buff->stat_mul = 1.0 + 0.15 * ( as<unsigned>( targets_hit.size() ) - 1u );

                make_event( *sim, delay, [ buff ] { buff->trigger(); } );
              } );

      travel_delay = e.driver()->effectN( 1 ).misc_value1() * 0.001;
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      targets_hit.clear();
      params.start_time( timespan_t::min() ).target( target );  // reset start time
      make_event<ground_aoe_event_t>( *sim, player, params );
    }
  };

  effect.has_use_damage_override = true;
  effect.execute_action = create_proc_action<high_speakers_accretion_t>( "high_speakers_accretion", effect );
}

// 443380 driver
// 449267 fragment spawn
// 449259 fragment duration
// 449254 buff
struct pickup_entropic_skardyn_core_t : public action_t
{
  buff_t* buff = nullptr;
  buff_t* tracker = nullptr;

  pickup_entropic_skardyn_core_t( player_t* p, std::string_view opt )
    : action_t( ACTION_OTHER, "pickup_entropic_skardyn_core", p )
  {
    parse_options( opt );

    s_data_reporting = p->find_spell( 443380 );
    name_str_reporting = "Pickup";

    callbacks = harmful = false;
    trigger_gcd = 0_ms;
  }

  bool ready() override
  {
    return tracker->check();
  }

  void execute() override
  {
    buff->trigger();
    tracker->decrement();
  }
};

// 443380 driver
// 449267 crystal summon
// 449259 crystal duration
// 449254 buff
// TODO: determine reasonable default values for core pickup delay
void entropic_skardyn_core( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "entropic_skardyn_core", "entropic_reclamation" } ) )
    return;

  struct entropic_skardyn_core_cb_t : public dbc_proc_callback_t
  {
    rng::truncated_gauss_t pickup;
    buff_t* buff;
    buff_t* tracker;
    timespan_t delay;

    entropic_skardyn_core_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        pickup( e.player->thewarwithin_opts.entropic_skardyn_core_pickup_delay,
                e.player->thewarwithin_opts.entropic_skardyn_core_pickup_stddev ),
        delay( timespan_t::from_seconds( e.trigger()->missile_speed() ) )
    {
      buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 449254 ) )
        ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) );

      tracker = create_buff<buff_t>( e.player, e.trigger()->effectN( 1 ).trigger() )
        ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
        ->set_max_stack( 6 );  // TODO: 'safe' value for 2 rppm. increase if necessary.

      for ( auto a : e.player->action_list )
      {
        if ( auto pickup_action = dynamic_cast<pickup_entropic_skardyn_core_t*>( a ) )
        {
          pickup_action->buff = buff;
          pickup_action->tracker = tracker;
        }
      }
    }

    void execute( action_t*, action_state_t* ) override
    {
      make_event( *listener->sim, delay, [ this ] {                    // delay for spawn
        tracker->trigger();                                            // trigger tracker
        make_event( *listener->sim, rng().gauss( pickup ), [ this ] {  // delay for pickup
          if ( tracker->check() )                                      // check hasn't been picked up via action
          {
            buff->trigger();
            tracker->decrement();
          }
        } );
      } );
    }
  };

  new entropic_skardyn_core_cb_t( effect );
}

// 443538 driver
//  e1: trigger buff
// 449275 buff
void empowering_crystal_of_anubikkaj( special_effect_t& effect )
{
  std::vector<buff_t*> buffs;

  create_all_stat_buffs( effect, effect.trigger(), 0,
    [ &buffs ]( stat_e, buff_t* b ) { buffs.push_back( b ); } );

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ buffs ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
      auto buff = cb->listener->rng().range( buffs );
      for ( auto b : buffs )
      {
        if ( b == buff )
          b->trigger();
        else
          b->expire();
      }
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 450561 on-use
// 450641 equip
//  e1: stat coeff
//  e2: damage coeff
// 443539 debuff, damage, damage taken driver
// 450551 buff
// TODO: move vers buff to external/passive/custom buff system
// TODO: determine if attack needs to do damage to proc vers buff
void mereldars_toll( special_effect_t& effect )
{
  unsigned equip_id = 450641;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Mereldar's Toll missing equip effect" );

  auto data = equip->driver();

  struct mereldars_toll_t : public generic_proc_t
  {
    int allies;
    target_specific_t<buff_t> buffs;
    const spell_data_t* equip_data;
    const special_effect_t& driver_effect;


    mereldars_toll_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "mereldars_toll", e.driver() ),
        allies( as<int>( data->effectN( 3 ).base_value() ) ),
        buffs{ false },
        equip_data( data ),
        driver_effect( e )
    {
      target_debuff = e.trigger();

      impact_action = create_proc_action<generic_proc_t>( "mereldars_toll_damage", e, e.trigger() );
      impact_action->base_dd_min = impact_action->base_dd_max = data->effectN( 2 ).average( e );
      impact_action->base_multiplier *= role_mult( e );
      impact_action->stats = stats;

      // Pre-initialise the players own personal buff.
      get_buff( e.player );
    }

    buff_t* get_buff( player_t* buff_player )
    {
      if ( buffs[ buff_player ] )
        return buffs[ buff_player ];

      if ( auto buff = buff_t::find( buff_player, "mereldars_toll", player ) )
      {
        buffs[ buff_player ] = buff;
        return buff;
      }

      auto vers =
          make_buff<stat_buff_t>( actor_pair_t{ buff_player, player }, "mereldars_toll", player->find_spell( 450551 ) )
              ->add_stat_from_effect_type( A_MOD_RATING, equip_data->effectN( 1 ).average( driver_effect ) );

      buffs[ buff_player ] = vers;

      return vers;
    }

    buff_t* create_debuff( player_t* t ) override
    {
      auto debuff = generic_proc_t::create_debuff( t )
        ->set_max_stack( allies )
        ->set_reverse( true );

      auto toll = new special_effect_t( t );
      toll->name_str = "mereldars_toll_debuff";
      toll->spell_id = target_debuff->id();
      toll->disable_action();
      t->special_effects.push_back( toll );

      // TODO: determine if attack needs to do damage to proc vers buff
      t->callbacks.register_callback_execute_function(
        toll->spell_id, [ this, debuff ]( const dbc_proc_callback_t* cb, action_t* a, action_state_t* ) {
          auto vers = get_buff( a->player );
          if ( vers && !vers->check() )
          {
            debuff->trigger();
            if ( cb->listener->rng().roll( cb->listener->thewarwithin_opts.mereldars_toll_ally_trigger_chance ) )
              vers->trigger();
          }
        } );

      auto cb = new dbc_proc_callback_t( t, *toll );
      cb->activate_with_buff( debuff, true );  // init = true as this is during runtime

      return debuff;
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      get_debuff( s->target )->trigger();
    }
  };

  effect.has_use_damage_override = true;
  effect.execute_action = create_proc_action<mereldars_toll_t>( "mereldars_toll", effect, data );
}

// 443527 driver
//  e1: buff trigger + coeff
//  e2: in light coeff
// 451367 buff
// 451368 in light buff
// TODO: determine reasonable default for delay in entering light
void carved_blazikon_wax( special_effect_t& effect )
{
  struct carved_blazikon_wax_cb_t : public dbc_proc_callback_t
  {
    rng::truncated_gauss_t delay;
    rng::truncated_gauss_t remain;
    buff_t* buff;
    buff_t* light;

    carved_blazikon_wax_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        delay( e.player->thewarwithin_opts.carved_blazikon_wax_enter_light_delay,
               e.player->thewarwithin_opts.carved_blazikon_wax_enter_light_stddev ),
        remain( e.player->thewarwithin_opts.carved_blazikon_wax_stay_in_light_duration,
                e.player->thewarwithin_opts.carved_blazikon_wax_stay_in_light_stddev )
    {
      auto light_spell = e.player->find_spell( 451368 );
      auto buff_spell = e.player->find_spell( 451367 );
      light =
        create_buff<stat_buff_t>( e.player, util::tokenize_fn( light_spell->name_cstr() ) + "_light", light_spell )
          ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 2 ).average( e ) )
          ->set_name_reporting( "In Light" );

      buff = create_buff<stat_buff_t>( e.player, buff_spell )
        ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 1 ).average( e ) );
    }

    void execute( action_t*, action_state_t* ) override
    {
      buff->trigger();

      auto wait = rng().gauss( delay );
      auto dur = light->buff_duration() - wait;

      if ( remain.mean > 0_ms )
        dur = std::min( dur, rng().gauss( remain ) );

      make_event( *light->sim, wait, [ this, dur ] { light->trigger( dur ); } );
    }
  };

  new carved_blazikon_wax_cb_t( effect );
}

// 443531 on-use, buff
// 450877 equip
//  e1: party coeff
//  e2: buff coeff
// 450882 party buff
// TODO: determine reasonable default for party buff options
// TODO: confirm you can have multiple party buffs at the same time
void signet_of_the_priory( special_effect_t& effect )
{
  unsigned equip_id = 450877;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Signet of the Priory missing equip effect" );

  auto data = equip->driver();

  struct signet_of_the_priory_t : public generic_proc_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;
    std::unordered_map<stat_e, buff_t*> party_buffs;

    rng::truncated_gauss_t party_first_use;
    rng::truncated_gauss_t party_use;

    signet_of_the_priory_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "signet_of_the_priory", e.driver() ),
        party_first_use( 0_ms, e.player->thewarwithin_opts.signet_of_the_priory_party_use_stddev ),
        party_use( e.player->thewarwithin_opts.signet_of_the_priory_party_use_cooldown,
                   e.player->thewarwithin_opts.signet_of_the_priory_party_use_stddev, e.driver()->cooldown() )
    {
      create_all_stat_buffs( e, e.driver(), data->effectN( 2 ).average( e ),
        [ this ]( stat_e s, buff_t* b ) { buffs[ s ] = b; } );

      create_all_stat_buffs( e, e.player->find_spell( 450882 ), data->effectN( 1 ).average( e ),
        [ this ]( stat_e s, buff_t* b ) { party_buffs[ s ] = b; } );
    }

    void execute() override
    {
      generic_proc_t::execute();

      buffs.at( util::highest_stat( player, secondary_ratings ) )->trigger();
    }
  };

  effect.disable_buff();
  auto signet = debug_cast<signet_of_the_priory_t*>(
    create_proc_action<signet_of_the_priory_t>( "signet_of_the_priory", effect, data ) );
  effect.execute_action = signet;
  effect.stat           = STAT_ANY_DPS;

  // TODO: determine reasonable default for party buff options
  // TODO: confirm you can have multiple party buffs at the same time
  std::vector<buff_t*> party_buffs;
  auto option = effect.player->thewarwithin_opts.signet_of_the_priory_party_stats;

  for ( auto s : util::string_split<std::string_view>( option, "/" ) )
    if ( auto stat = util::parse_stat_type( s ); stat != STAT_NONE )
      party_buffs.push_back( signet->party_buffs.at( stat ) );

  for ( auto b : party_buffs )
  {
    effect.player->register_combat_begin( [ b, signet ]( player_t* ) {
      make_event( *b->sim, b->rng().gauss( signet->party_first_use ), [ b, signet ] {
        b->trigger();
        make_repeating_event( *b->sim,
          [ b, signet ] { return b->rng().gauss( signet->party_use ); },
          [ b ] { b->trigger(); } );
      } );
    } );
  }
}

// 451055 driver
//  e1: damage coeff
//  e2: buff coeff
// 451292 damage
// 451303 buff
// 443549 summon from back right?
// 451991 summon from back left?
// TODO: determine travel speed to hit target, assuming 5yd/s based on 443549 range/duration
// TODO: determine reasonable delay to intercept
void harvesters_edict( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "volatile_blood_blast", effect, effect.driver(), true );
  damage->base_dd_min = damage->base_dd_max =
    effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );
  // TODO: determine travel speed to hit target, assuming 5yd/s based on 443549 range/duration
  damage->travel_speed = 5.0;

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 451303 ) )
    ->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 2 ).average( effect ) );

  // TODO: determine reasonable delay to intercept
  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ damage, buff ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
      if ( cb->rng().roll( cb->listener->thewarwithin_opts.harvesters_edict_intercept_chance ) )
        buff->trigger();
      else
        damage->execute();
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 443525 driver
//  e1: damage coeff
// 450429 damage
// 450416 unknown, cart travel path?
// 450458 unknown, cart travel path?
// 450459 unknown, cart travel path?
// 450460 unknown, cart travel path?
// TODO: determine travel speed/delay, assuming 7.5yd/s based on summed cart path(?) radius/duration
void conductors_wax_whistle( special_effect_t& effect )
{
  // TODO: confirm damage does not increase per extra target
  auto damage = create_proc_action<generic_aoe_proc_t>( "collision", effect, 450429, true );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );
  // TODO: determine travel speed/delay, assuming 7.5yd/s based on summed cart path(?) radius/duration
  damage->travel_speed = 7.5;

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

// 460469 on use
// 443337 cast spell
//  e1: damage coeff
//  e2: jump speed?
//  e3: unknown
// 448892 damage
// TODO: confirm lightning ball jump speed
void charged_stormrook_plume( special_effect_t& effect )
{
  // TODO: confirm damage does not increase per extra target
  auto damage = create_proc_action<generic_aoe_proc_t>( "charged_stormrook_plume", effect, 448892, true );
  auto cast_spell = effect.player->find_spell( 443337 );
  damage->base_dd_min = damage->base_dd_max = cast_spell->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect.player, cast_spell );
  damage->base_execute_time = cast_spell->cast_time();
  // TODO: confirm lightning ball jump speed
  damage->travel_speed = cast_spell->effectN( 2 ).base_value();

  effect.execute_action = damage;
}

// 443556 on use
// 450044 equip
//  e1: damage coeff
//  e2: unknown coeff, dual strike?
// 450157 use window
// 450119 melee damage
// 450151 can ranged strike (2nd)
// 450158 ranged damage
// 450340 ranged speed
// 450162 can dual strike (3rd)
// 450204 unknown
// TODO: add APL action for 2nd & 3rd use if needed. for now assumed to be used asap.
void twin_fang_instruments( special_effect_t& effect )
{
  unsigned equip_id = 450044;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Twin Fang Instruments missing equip effect" );

  auto data = equip->driver();

  struct twin_fang_instruments_t : public generic_proc_t
  {
    action_t* melee;
    action_t* range;
    timespan_t delay = 2_s;  // not in spell data

    twin_fang_instruments_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "twin_fang_instruments", e.driver() )
    {
      melee = create_proc_action<generic_aoe_proc_t>( "nxs_shadow_strike", e, e.player->find_spell( 450119 ) );
      melee->base_dd_min = melee->base_dd_max = data->effectN( 1 ).average( e ) * 0.5;
      melee->base_multiplier *= role_mult( e );
      melee->split_aoe_damage = false;
      add_child( melee );

      range = create_proc_action<generic_aoe_proc_t>( "vxs_frost_slash", e, e.player->find_spell( 450158 ), true );
      range->base_dd_min = range->base_dd_max = data->effectN( 1 ).average( e );
      range->base_multiplier *= role_mult( e );
      range->travel_speed = range->data().effectN( 3 ).trigger()->missile_speed();
      add_child( range );
    }

    void execute() override
    {
      generic_proc_t::execute();

      // TODO: add APL action for 2nd & 3rd use if needed. for now assumed to be used asap.
      melee->execute_on_target( target );

      make_event( *sim, delay, [ this ] {
        range->execute_on_target( target );
      } );

      make_event( *sim, delay * 2, [ this ] {
        melee->execute_on_target( target );
        range->execute_on_target( target );
      } );
    }
  };

  effect.has_use_damage_override = true;
  effect.execute_action = create_proc_action<twin_fang_instruments_t>( "twin_fang_instruments", effect, data );
}

// 463610 trinket equip
// 463232 embellishment equip
//  e1: trigger cycle
//  e2: buff coeff
// 455535 cycle
// 455536 buff
// 455537 self damage
// TODO: determine if self damage procs anything
void darkmoon_deck_symbiosis( special_effect_t& effect )
{
  bool is_embellishment = effect.driver()->id() == 463232;

  struct symbiosis_buff_t : public stat_buff_t
  {
    event_t* ev = nullptr;
    action_t* self_damage;
    timespan_t period;
    double self_damage_pct;

    symbiosis_buff_t( const special_effect_t& e )
      : stat_buff_t( e.player, "symbiosis", e.player->find_spell( 455536 ) ),
        period( e.player->find_spell( 455535 )->effectN( 1 ).period() )
    {
      self_damage = create_proc_action<generic_proc_t>( "symbiosis_self", e, 455537 );

      // We don't want this counted towards our dps
      self_damage->stats->type = stats_e::STATS_NEUTRAL;

      // TODO: determine if self damage procs anything
      self_damage->callbacks = false;
      self_damage->target    = player;
      self_damage_pct        = self_damage->data().effectN( 1 ).percent();
    }

    void start_symbiosis()
    {
      ev = make_event( *player->sim, period, [ this ] { tick_symbiosis(); } );
    }

    void tick_symbiosis()
    {
      self_damage->execute_on_target( player, player->resources.max[ RESOURCE_HEALTH ] * self_damage_pct );
      trigger();
      ev = make_event( *player->sim, period, [ this ] { tick_symbiosis(); } );
    }

    void cancel_symbiosis()
    {
      if ( ev )
        event_t::cancel( ev );
    }
  };

  auto buff = buff_t::find( effect.player, "symbiosis" );
  symbiosis_buff_t* symbiosis = nullptr;

  if ( !buff )
  {
    buff      = make_buff<symbiosis_buff_t>( effect );
    symbiosis = debug_cast<symbiosis_buff_t*>( buff );
    effect.player->register_on_combat_state_callback( [ symbiosis ]( player_t*, bool c ) {
      if ( c )
        symbiosis->start_symbiosis();
      else
        symbiosis->cancel_symbiosis();
    } );
  }
  else
  {
    symbiosis = debug_cast<symbiosis_buff_t*>( buff );
  }

  double value = effect.driver()->effectN( 2 ).average( effect );
  if ( is_embellishment )
    value *= writhing_mul( effect.player );

  symbiosis->add_stat_from_effect_type( A_MOD_RATING, value );
}

// 454859 rppm data
// 454857 card driver/values
// 463611 embellish values (11.0.5 only)
// 454862 fire
// 454975 shadow
// 454976 nature
// 454977 frost
// 454978 holy
// 454979 arcane
// 454980 Physical
// 454982 Magical Multischool
void darkmoon_deck_vivacity( special_effect_t& effect )
{
  struct vivacity_cb_t : public dbc_proc_callback_t
  {
    buff_t* impact;
    buff_t* shadow;
    buff_t* nature;
    buff_t* frost;
    buff_t* holy;
    buff_t* arcane;
    buff_t* force;
    buff_t* magical_multi;

    vivacity_cb_t( const special_effect_t& e, const spell_data_t* values )
      : dbc_proc_callback_t( e.player, e ),
        impact( nullptr ),
        shadow( nullptr ),
        nature( nullptr ),
        frost( nullptr ),
        holy( nullptr ),
        arcane( nullptr ),
        force( nullptr ),
        magical_multi( nullptr )
    {
      impact = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454862 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );

      shadow = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454975 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );

      nature = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454976 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );

      frost = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454977 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );

      holy = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454978 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );

      arcane = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454979 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );

      force = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454980 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );

      magical_multi = create_buff<stat_buff_t>( e.player, e.player->find_spell( 454982 ) )
        ->add_stat_from_effect( 1, values->effectN( 1 ).average( e ) )
        ->add_stat_from_effect( 2, values->effectN( 2 ).average( e ) );
    }

    void execute( action_t*, action_state_t* s ) override
    {
      switch ( s->action->get_school() )
      {
        case SCHOOL_FIRE:
          impact->trigger();
          break;
        case SCHOOL_SHADOW:
          shadow->trigger();
          break;
        case SCHOOL_NATURE:
          nature->trigger();
          break;
        case SCHOOL_FROST:
          frost->trigger();
          break;
        case SCHOOL_HOLY:
          holy->trigger();
          break;
        case SCHOOL_ARCANE:
          arcane->trigger();
          break;
        case SCHOOL_PHYSICAL:
          force->trigger();
          break;
        default:
          magical_multi->trigger();
          break;
      }
    }
  };

  auto data = effect.driver();
  effect.spell_id = 454859;

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;

  new vivacity_cb_t( effect, data );
}

void algari_alchemist_stone( special_effect_t& e )
{
  auto stat = e.player->convert_hybrid_stat( STAT_STR_AGI_INT );
  const spell_data_t* buff_spell;
  switch ( stat )
  {
    case STAT_STRENGTH:
      buff_spell = e.player->find_spell( 299788 );
      break;
    case STAT_AGILITY:
      buff_spell = e.player->find_spell( 299789 );
      break;
    default:
      buff_spell = e.player->find_spell( 299790 );
      break;
  }

  auto buff = create_buff<stat_buff_t>( e.player, buff_spell )
    ->add_stat_from_effect( 1, e.driver()->effectN( 1 ).average( e ) );

  e.custom_buff = buff;
  new dbc_proc_callback_t( e.player, e );
}

// Darkmoon Deck Ascension
// 463095 Trinket Driver
// 458573 Embelishment Driver
// 457594 Periodic Trigger Buff
// 458502 Crit Buff
// 458503 Haste Buff
// 458525 Mastery Buff
// 458524 Vers Buff
void darkmoon_deck_ascension( special_effect_t& effect )
{
  bool is_embellishment = effect.spell_id == 458573;

  struct ascension_tick_t : public buff_t
  {
    std::vector<buff_t*> buff_list;
    buff_t* last_buff;
    unsigned stack;
    bool in_combat;

    ascension_tick_t( const special_effect_t& e, std::string_view n, const spell_data_t* s, bool embellish )
      : buff_t( e.player, n, s ), buff_list(), last_buff( nullptr ), stack( 0 ), in_combat( false )
    {
      add_stats( e, embellish );
      last_buff = buff_list[ 0 ];
      // refreshes if in combat on tick, otherwise buff expires naturally
      set_tick_callback( [ & ]( buff_t*, int, timespan_t ) {
        if ( in_combat )
          trigger_ascension();
        // Ticking stops if out of combat
        else
          make_event( *player->sim, 0_ms, [ & ] { expire(); } );
      } );
    }

    void add_stats( const special_effect_t& e, bool embellish )
    {
      static constexpr std::pair<unsigned, const char*> buff_entries[] = {
        { 458502, "Crit"    },
        { 458503, "Haste"   },
        { 458525, "Mastery" },
        { 458524, "Vers"    }
      };

      for ( const auto& [ id, stat ] : buff_entries )
      {
        auto s_data = e.player->find_spell( id );
        double value = 0;
        if ( embellish )
          value = e.driver()->effectN( 2 ).average( e ) * writhing_mul( e.player );
        else
          value = e.player->find_spell( 463059 )->effectN( 1 ).average( e );

        auto buff = create_buff<stat_buff_t>( e.player, fmt::format( "{}_{}", s_data->name_cstr(), stat ), s_data )
          ->add_stat_from_effect_type( A_MOD_RATING, value )
          ->set_name_reporting( stat );

        buff_list.push_back( buff );
      }
    }

    void trigger_ascension()
    {
      if ( stack < as<unsigned>( data().effectN( 1 ).base_value() ) )
      {
        if ( last_buff->check() && stack > 0 )
        {
          stack = last_buff->check() + 1;
        }
        else
        {
          ++stack;
        }
      }

      rng().shuffle( buff_list.begin(), buff_list.end() );

      if ( buff_list[ 0 ] == last_buff )
      {
        buff_list[ 0 ]->trigger();
      }
      else
      {
        if ( last_buff->check() )
        {
          last_buff->expire();
        }
        buff_list[ 0 ]->trigger( stack );
        last_buff = buff_list[ 0 ];
      }
    }

    void reset() override
    {
      buff_t::reset();
      stack = 0;
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      buff_t::start( stacks, value, duration );
      stack = 0;
    };

    void expire_override( int stacks, timespan_t duration ) override
    {
      buff_t::expire_override( stacks, duration );
      stack = 0;
    }
  };

  auto buff_name = "ascendance_darkmoon";
  auto buff = buff_t::find( effect.player, buff_name );
  if ( buff )
  {
    debug_cast<ascension_tick_t*>( buff )->add_stats( effect, is_embellishment );
    return;
  }

  buff = make_buff<ascension_tick_t>( effect, buff_name, effect.player->find_spell( 457594 ), is_embellishment );
  effect.name_str = "ascendance_darkmoon";
  effect.player->register_on_combat_state_callback( [ buff ]( player_t*, bool c ) {
    if ( c )
    {
      if ( !buff->check() )
      {
        buff->trigger();
      }

      debug_cast<ascension_tick_t*>( buff )->in_combat = true;
    }
    else
    {
      debug_cast<ascension_tick_t*>( buff )->in_combat = false;
    }
  } );
}

// Darkmoon Deck Radiance
// 463108 Trinket Driver
// 454558 Embelishment Driver
// 454560 Debuff/Accumulator
// 454785 Buff
// 454559 RPPM data
void darkmoon_deck_radiance( special_effect_t& effect )
{
  struct radiant_focus_debuff_t : public buff_t
  {
    double accumulated_damage;
    double max_damage;
    buff_t* buff;

    radiant_focus_debuff_t( actor_pair_t td, const special_effect_t& e, std::string_view n, const spell_data_t* s,
                            buff_t* b )
      : buff_t( td, n, s ), accumulated_damage( 0 ), max_damage( data().effectN( 1 ).average( e ) ), buff( b )
    {
      set_default_value( max_damage );
    }

    void reset() override
    {
      buff_t::reset();
      accumulated_damage = 0;
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      buff_t::start( stacks, value, duration );
      accumulated_damage = 0;
    };

    void expire_override( int stacks, timespan_t duration ) override
    {
      buff_t::expire_override( stacks, duration );
      double buff_value = std::min( 1.0, std::max( 0.5, accumulated_damage / max_damage ) ) * buff->default_value;

      auto stat_buff = debug_cast<stat_buff_t*>( buff );

      for ( auto& s : stat_buff->stats )
      {
        if ( s.stat == util::highest_stat( source, secondary_ratings ) )
        {
          s.amount = buff_value;
        }
        else
        {
          s.amount = 0;
        }
      }

      buff->trigger();

      accumulated_damage = 0;
    }
  };

  struct radiant_focus_cb_t : public dbc_proc_callback_t
  {
    const spell_data_t* debuff_spell;
    const special_effect_t& effect;
    buff_t* buff;

    radiant_focus_cb_t( const special_effect_t& e, buff_t* b )
      : dbc_proc_callback_t( e.player, e ), debuff_spell( e.player->find_spell( 454560 ) ), effect( e ), buff( b )
    {
      effect.player->callbacks.register_callback_execute_function(
          454560, [ & ]( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
            auto target_debuff = get_debuff( s->target );
            if ( s->result_amount > 0 && target_debuff->check() )
            {
              auto debuff = debug_cast<radiant_focus_debuff_t*>( target_debuff );
              debuff->accumulated_damage += s->result_amount;
              if ( debuff->accumulated_damage >= debuff->max_damage )
              {
                target_debuff->expire();
              }
            }
          } );

      effect.player->callbacks.register_callback_trigger_function(
          454560, dbc_proc_callback_t::trigger_fn_type::CONDITION,
          [ & ]( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
            return get_debuff( s->target )->check();
          } );
    }

    buff_t* create_debuff( player_t* t ) override
    {
      auto debuff = make_buff<radiant_focus_debuff_t>( actor_pair_t( t, listener ), effect, "radiant_focus_debuff",
                                                       debuff_spell, buff );

      return debuff;
    }

    void execute( action_t*, action_state_t* s ) override
    {
      auto debuff = get_debuff( s->target );
      // as of 8/17/2024, does not refresh or expire the debuff if it triggers again while active
      if ( !debuff->check() )
      {
        debuff->trigger();
      }
    }
  };

  auto radiant_focus      = new special_effect_t( effect.player );
  radiant_focus->name_str = "radiant_focus_darkmoon";
  radiant_focus->spell_id = 454560;
  effect.player->special_effects.push_back( radiant_focus );

  auto radiant_focus_proc = new dbc_proc_callback_t( effect.player, *radiant_focus );
  radiant_focus_proc->initialize();
  radiant_focus_proc->activate();

  bool embelish = effect.driver()->id() == 454558;

  auto buff = buff_t::find( effect.player, "radiance" );
  if ( !buff )
  {
    auto buff_spell = effect.player->find_spell( 454785 );

    // Very oddly setup buff, scripted to provide the highest secondary stat of the player
    // Data doesnt contain the other stats like usual, manually setting things up here.
    buff = create_buff<stat_buff_t>( effect.player, buff_spell )
               ->add_stat( STAT_CRIT_RATING, 0 )
               ->add_stat( STAT_MASTERY_RATING, 0 )
               ->add_stat( STAT_HASTE_RATING, 0 )
               ->add_stat( STAT_VERSATILITY_RATING, 0 );

    if ( embelish )
      buff->set_default_value( effect.driver()->effectN( 2 ).average( effect ) );
    else
      buff->set_default_value( effect.driver()->effectN( 1 ).average( effect ) );
  }
  else if ( buff )
  {
    if ( embelish )
      buff->modify_default_value( effect.driver()->effectN( 2 ).average( effect ) );
    else
      buff->modify_default_value( effect.driver()->effectN( 1 ).average( effect ) );
  }

  effect.spell_id = 454559;
  effect.name_str = "radiant_focus";

  new radiant_focus_cb_t( effect, buff );
}

// Nerubian Pheromone Secreter
// 441023 Driver
// 441428 Buff
// 441508, 441507, 441430 Area Triggers for Phearomone
// Gems:
// 227445 - Mastery, Bonus id 11314
// 227447 - Haste, Bonus id 11315
// 227448 - Crit, Bonus id 11316
// 227449 - Versatility, Bonus id 11317
struct pickup_nerubian_pheromone_t : public action_t
{
  buff_t* orb = nullptr;

  pickup_nerubian_pheromone_t( player_t* p, std::string_view opt )
    : action_t( ACTION_OTHER, "pickup_nerubian_pheromone", p, spell_data_t::nil() )
  {
    parse_options( opt );

    s_data_reporting   = p->find_spell( 441023 );
    name_str_reporting = "Picked up Nerubian Pheromone";

    callbacks = harmful = false;
    trigger_gcd         = 0_ms;
  }

  bool ready() override
  {
    return orb->check();
  }

  void execute() override
  {
    orb->decrement();
  }
};

void nerubian_pheromone_secreter( special_effect_t& effect )
{
  struct nerubian_pheromones_cb_t : public dbc_proc_callback_t
  {
    buff_t* stat_buff;
    buff_t* orb;
    std::vector<action_t*> apl_actions;

    nerubian_pheromones_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), stat_buff( nullptr ), orb( nullptr ), apl_actions()
    {
      stat_buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 441428 ) )
        ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) );

      for ( auto& a : e.player->action_list )
      {
        if ( a->name_str == "pickup_nerubian_pheromone" )
        {
          apl_actions.push_back( a );
        }
      }

      if ( apl_actions.size() > 0 )
      {
        orb = create_buff<buff_t>( e.player, e.player->find_spell( 441430 ) )
                  ->set_max_stack( e.player->thewarwithin_opts.nerubian_pheromone_secreter_pheromones )
                  ->set_duration( e.player->find_spell( 441430 )->duration() )
                  ->set_initial_stack( e.player->thewarwithin_opts.nerubian_pheromone_secreter_pheromones )
                  ->set_quiet( true )
                  ->set_expire_callback( [ & ]( buff_t*, int, timespan_t d ) {
                    if ( d > 0_ms )
                    {
                      stat_buff->trigger();
                    }
                  } );
      }

      // Set a default task for the actions ready() function, will be overwritten later
      for ( auto& a : apl_actions )
      {
        debug_cast<pickup_nerubian_pheromone_t*>( a )->orb = orb;
      }
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( apl_actions.size() > 0 )
      {
        orb->trigger();
      }
      else
      {
        for ( int i = 0; i < listener->thewarwithin_opts.nerubian_pheromone_secreter_pheromones; i++ )
        {
          make_event( *listener->sim, rng().range( 200_ms, listener->find_spell( 441430 )->duration() ),
                      [ & ] { stat_buff->trigger(); } );
        }
      }
    }
  };

  new nerubian_pheromones_cb_t( effect );
}

// Shadowed Essence
// 455640 Driver
// 455643 Periodic Trigger Buff
// 455653 Damage missile
// 455654 Damage
// 455656 Crit Buff
// 455966 Primary Stat Buff
void shadowed_essence( special_effect_t& effect )
{
  struct shadowed_essence_buff_t : public buff_t
  {
    unsigned tick_n = 0;

    shadowed_essence_buff_t( const special_effect_t& e )
      : buff_t( e.player, "shadowed_essence_periodic", e.trigger() ), tick_n( 0 )
    {
      auto shadowed_essence = create_buff<stat_buff_t>( e.player, e.player->find_spell( 455966 ) )
                                  ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 2 ).average( e ) );

      auto dark_embrace = create_buff<stat_buff_t>( e.player, e.player->find_spell( 455656 ) )
                              ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 3 ).average( e ) );

      auto damage         = create_proc_action<generic_proc_t>( "shadowed_essence_damage", e, 455654 );
      damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 1 ).average( e );
      damage->base_multiplier *= role_mult( e.player );

      auto missile           = create_proc_action<generic_proc_t>( "shadowed_essence", e, 455653 );
      missile->add_child( damage );
      missile->impact_action = damage;

      set_quiet( true );
      set_tick_on_application( true );
      set_tick_callback( [ &, dark_embrace, shadowed_essence, missile ]( buff_t*, int, timespan_t ) {
        if ( tick_n == 0 )
        {
          dark_embrace->trigger();
        }
        else if ( tick_n % 3 == 0 )
        {
          if ( dark_embrace->check() )
            shadowed_essence->trigger();
          else
            dark_embrace->trigger();
        }
        else
        {
          missile->execute_on_target( e.player->target );
        }
        tick_n++;
      } );
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      buff_t::start( stacks, value, duration );
      tick_n = 0;
    }

    void expire_override( int stacks, timespan_t duration ) override
    {
      buff_t::expire_override( stacks, duration );
      tick_n = 0;
    }

    void reset() override
    {
      buff_t::reset();
      tick_n = 0;
    }
  };

  auto buff = buff_t::find( effect.player, "shadowed_essence_periodic" );
  if ( !buff )
  {
    buff = make_buff<shadowed_essence_buff_t>( effect );
  }

  effect.player->register_on_combat_state_callback( [ buff ]( player_t*, bool c ) {
    if ( c )
      buff->trigger();
    else
      buff->expire();
  } );
}

// Spelunker's Waning Candle
// 445419 driver
// 445420 buff
// 8-31-24 ally sharing is not functioning in game
void spelunkers_waning_candle( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
                           ->add_stat_from_effect( 1, effect.driver()->effectN( 2 ).average( effect.item ) );

  new dbc_proc_callback_t( effect.player, effect );
}

// 455436 driver
// 455441 mastery
// 455454 crit
// 455455 haste
// 455456 vers
void unstable_power_core( special_effect_t& effect )
{
  static constexpr std::array<std::pair<unsigned, const char*>, 4> buff_ids = {
      { { 455441, "mastery" }, { 455454, "crit" }, { 455455, "haste" }, { 455456, "vers" } } };

  std::vector<buff_t*> buffs = {};

  auto buff_name = util::tokenize_fn( effect.driver()->name_cstr() );

  for ( const auto& [ id, stat_name ] : buff_ids )
  {
    auto buff  = make_buff<stat_buff_t>( effect.player, fmt::format( "{}_{}", buff_name, stat_name ),
                                        effect.player->find_spell( id ) );

    buff->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

    buffs.push_back( buff );
  }

  for ( auto& buff : buffs )
  {
    buff->set_expire_callback( [ effect, buffs ]( buff_t*, int, timespan_t remains ) {
      if ( remains > 0_ms )
        return;

      effect.player->rng().range( buffs )->trigger( effect.player->rng().range( 10_s, 30_s ) );
    } );
  }

  effect.player->register_precombat_begin( [ buffs ]( player_t* p ) {
    p->rng().range( buffs )->trigger( p->rng().range( 10_s, 30_s ) );
  } );
}

// 451742 driver
// 451750 buff scaling driver?
// 451748 buff
void stormrider_flight_badge( special_effect_t& effect )
{
  auto buff =
      create_buff<stat_buff_t>( effect.player, "stormrider_flight_badge", effect.driver()->effectN( 2 ).trigger() );
  buff->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).trigger()->effectN( 1 ).average( effect.item ) );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// 435502 driver
// 440389 debuff
void shadowbinding_ritual_knife( special_effect_t& effect )
{
  auto primary_buff = create_buff<stat_buff_t>( effect.player, "shadowbinding_ritual_knife_primary", effect.driver() )
                          ->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect ) )
                          ->set_chance( 1.0 )
                          ->set_rppm( RPPM_DISABLE )
                          ->set_duration( 0_s )
                          ->set_constant_behavior( buff_constant_behavior::ALWAYS_CONSTANT );

  effect.player->register_combat_begin( [ primary_buff ]( player_t* ) { primary_buff->trigger(); } );

  std::vector<buff_t*> negative_buffs;

  auto buff_spell = effect.driver()->effectN( 2 ).trigger();

  create_all_stat_buffs( effect, buff_spell, effect.driver()->effectN( 2 ).average( effect ),
    [ &negative_buffs ]( stat_e, buff_t* b ) { negative_buffs.push_back( b ); } );

  if ( negative_buffs.size() > 0 )
  {
    effect.player->callbacks.register_callback_execute_function(
      effect.driver()->id(), [ negative_buffs ]( const dbc_proc_callback_t* cb, action_t*, const action_state_t* ) {
        cb->listener->rng().range( negative_buffs )->trigger();
      } );
  }

  effect.buff_disabled = true;

  new dbc_proc_callback_t( effect.player, effect );
}

// 455432 Driver
// 455433 Damage Spell
// 455434 Healing Spell
void shining_arathor_insignia( special_effect_t& effect )
{
  // TODO: make it heal players as well
  // TODO: determine if this is affected by role mult
  auto damage_proc         = create_proc_action<generic_proc_t>( "shining_arathor_insignia_damage", effect, 455433 );
  damage_proc->base_dd_min = damage_proc->base_dd_max = effect.driver()->effectN( 1 ).average( effect );

  effect.execute_action = damage_proc;

  new dbc_proc_callback_t( effect.player, effect );
}

// 455451 buff
//  e1: speed
//  e2: haste
void quickwick_candlestick( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.driver(), effect.item )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED )
    ->add_invalidate( CACHE_RUN_SPEED );

  effect.player->buffs.quickwicks_quick_trick_wick_walk = buff;
  effect.custom_buff = buff;
}

// 455435 driver
//  e1: weak light up (455443)
//  e2: weak not so gentle flame (455447)
//  e3: strong light up (455480)
//  e4: strong not so gentle flame (455479)
// 455445 summon waxx (weak light up)
// 455448 summon wayne (weak not so gentle flame)
// 455453 summon take (strong light up, strong not so gentle flame)
// 455443 weak light up
// 455447 weak not so gentle flame
// 455479 strong not so gentle flame
// 455480 strong light up
// TODO: Figure out Takes odd behavior
// Figure out the pets AP/SP coefficients for auto attacking pets
void candle_confidant( special_effect_t& effect )
{
  struct candle_confidant_pet_t : public unique_gear_pet_t
  {
  protected:
    using base_t = candle_confidant_pet_t;

    candle_confidant_pet_t( std::string_view name, const special_effect_t& e, const spell_data_t* summon_spell )
      : unique_gear_pet_t( name, e, summon_spell )
    {
      npc_id = summon_spell->effectN( 1 ).misc_value1();
      use_auto_attack = false;
    }

    void update_stats() override
    {
      unique_gear_pet_t::update_stats();
      // Currently doesnt seem to scale with haste
      if ( owner->bugs )
      {
        current_pet_stats.composite_melee_haste             = 1;
        current_pet_stats.composite_spell_haste             = 1;
        current_pet_stats.composite_melee_auto_attack_speed = 1;
        current_pet_stats.composite_spell_cast_speed        = 1;
      }
    }
  };

  struct auto_attack_melee_t : public melee_attack_t
  {
    auto_attack_melee_t( pet_t* p, std::string_view name = "main_hand", action_t* a = nullptr )
      : melee_attack_t( name, p )
    {
      this->background = this->repeating = true;
      this->not_a_proc = this->may_crit = true;
      this->special                     = false;
      this->weapon_multiplier           = 1.0;
      this->trigger_gcd                 = 0_ms;
      this->school                      = SCHOOL_PHYSICAL;
      this->stats->school               = SCHOOL_PHYSICAL;
      this->base_dd_min = this->base_dd_max = p->dbc->expected_stat( p->true_level ).creature_auto_attack_dps;
      this->base_multiplier                 = p->main_hand_weapon.swing_time.total_seconds();

      auto proxy = a;
      auto it    = range::find( proxy->child_action, name, &action_t::name );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }

    double composite_crit_chance() const override
    {
      // Currently their auto attacks dont seem to scale with player crit chance.
      return this->player->base.attack_crit_chance;
    }

    // Pet melee attacks seem to still scale with aura 380 and 531
    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = melee_attack_t::composite_da_multiplier( s );
      if ( player->is_ptr() )
        m *= this->player->cast_pet()->owner->composite_player_pet_damage_multiplier( s, type == PLAYER_GUARDIAN );
      return m;
    }

    double composite_target_multiplier( player_t* p ) const override
    {
      double m = melee_attack_t::composite_target_multiplier( p );
      if ( player->is_ptr() )
        m *= this->player->cast_pet()->owner->composite_player_target_pet_damage_multiplier( p, type == PLAYER_GUARDIAN );
      return m;
    }

    void execute() override
    {
      if ( this->player->executing )
        this->schedule_execute();
      else
        melee_attack_t::execute();
    }
  };

  struct candle_confidant_pet_spell_t : public spell_t
  {
    candle_confidant_pet_spell_t( std::string_view n, pet_t* p, const spell_data_t* s, std::string_view options_str,
                                  action_t* a )
      : spell_t( n, p, s )
    {
      auto proxy = a;
      auto it = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );

      parse_options( options_str );
    }
  };

  struct weak_light_up_t : public candle_confidant_pet_spell_t
  {
    weak_light_up_t( const special_effect_t& e, candle_confidant_pet_t* p, std::string_view options_str, action_t* a )
      : candle_confidant_pet_spell_t( "light_up_waxx", p, p->find_spell( 455443 ), options_str, a )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e );
      name_str_reporting = "light_up";
    }

    void execute() override
    {
      // Has the odd spell que delay where itll wait a bit before starting the next cast, about 2.75s on average.
      trigger_gcd = base_execute_time + rng().range( 500_ms, 1000_ms );
      candle_confidant_pet_spell_t::execute();
    }
  };

  struct weak_not_so_gentle_flame_t : public candle_confidant_pet_spell_t
  {
    weak_not_so_gentle_flame_t( const special_effect_t& e, candle_confidant_pet_t* p, std::string_view options_str,
                                action_t* a )
      : candle_confidant_pet_spell_t( "notsogentle_flame_wayne", p, p->find_spell( 455447 ), options_str, a )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );
      name_str_reporting = "notsogentle_flame";
    }

    void execute() override
    {
      // Has the odd spell que delay where itll wait a bit before starting the next cast, about 3.2s on average.
      cooldown->duration = 3_s + rng().range( 100_ms, 300_ms );
      candle_confidant_pet_spell_t::execute();
    }
  };

  struct strong_light_up_t : public candle_confidant_pet_spell_t
  {
    strong_light_up_t( const special_effect_t& e, candle_confidant_pet_t* p, std::string_view options_str, action_t* a )
      : candle_confidant_pet_spell_t( "light_up_take", p, p->find_spell( 455480 ), options_str, a )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 3 ).average( e );
      name_str_reporting = "light_up";
    }

    void execute() override
    {
      // TODO: Take is weird, lots of odd spell queing events, missed casts, and long delays between casts.
      // Figure out exactly whats going on here.
      // https://www.warcraftlogs.com/reports/PNwkxKmn1cgtMYh3#fight=68&type=damage-done&source=289&view=events
      cooldown->duration = 3_s + rng().range( 100_ms, 300_ms );
      candle_confidant_pet_spell_t::execute();
    }
  };

  struct strong_not_so_gentle_flame_t : public candle_confidant_pet_spell_t
  {
    strong_not_so_gentle_flame_t( const special_effect_t& e, candle_confidant_pet_t* p, std::string_view options_str,
                                  action_t* a )
      : candle_confidant_pet_spell_t( "notsogentle_flame_take", p, p->find_spell( 455479 ), options_str, a )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 4 ).average( e );
      name_str_reporting = "notsogentle_flame";
    }

    void execute() override
    {
      // TODO: Take is weird, lots of odd spell queing events, missed casts, and long delays between casts.
      // Figure out exactly whats going on here.
      // https://www.warcraftlogs.com/reports/PNwkxKmn1cgtMYh3#fight=68&type=damage-done&source=289&view=events
      cooldown->duration = 5_s + rng().range( 100_ms, 300_ms );
      candle_confidant_pet_spell_t::execute();
    }
  };

  struct waxx_pet_t : public candle_confidant_pet_t
  {
    waxx_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : base_t( "Waxx", e, e.player->find_spell( 455445 ) )
    {
      parent_action = parent;
    }

    action_t* create_action( std::string_view name, std::string_view options_str ) override
    {
      if ( name == "light_up" )
        return new weak_light_up_t( effect, this, options_str, parent_action );

      return base_t::create_action( name, options_str );
    }

    void init_action_list() override
    {
      base_t::init_action_list();
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "light_up" );
    }
  };

  struct wayne_pet_t : public candle_confidant_pet_t
  {
    wayne_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : base_t( "Wayne", e, e.player->find_spell( 455448 ) )
    {
      parent_action = parent;
      use_auto_attack = true;
      main_hand_weapon.type = WEAPON_BEAST;
      main_hand_weapon.swing_time = 2_s;
    }

    attack_t* create_auto_attack() override
    {
      auto a = new auto_attack_melee_t( this, "main_hand_wayne", parent_action );
      a->name_str_reporting = "Melee";
      return a;
    }

    action_t* create_action( std::string_view name, std::string_view options_str ) override
    {
      if ( name == "not_so_gentle_flame" )
        return new weak_not_so_gentle_flame_t( effect, this, options_str, parent_action );

      return base_t::create_action( name, options_str );
    }

    void init_action_list() override
    {
      base_t::init_action_list();
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "not_so_gentle_flame" );
    }
  };

  // TODO: Take is weird, lots of odd spell queing events, missed casts, and long delays between casts.
  // Figure out exactly whats going on here.
  // https://www.warcraftlogs.com/reports/PNwkxKmn1cgtMYh3#fight=68&type=damage-done&source=289&view=events
  struct take_pet_t : public candle_confidant_pet_t
  {
    take_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : base_t( "Take", e, e.player->find_spell( 455453 ) )
    {
      parent_action = parent;
      use_auto_attack = true;
      main_hand_weapon.type = WEAPON_BEAST;
      main_hand_weapon.swing_time = 2_s;
    }

    attack_t* create_auto_attack() override
    {
      auto a = new auto_attack_melee_t( this, "main_hand_take", parent_action );
      a->name_str_reporting = "Melee";
      return a;
    }

    action_t* create_action( std::string_view name, std::string_view options_str ) override
    {
      if ( name == "not_so_gentle_flame" )
        return new strong_not_so_gentle_flame_t( effect, this, options_str, parent_action );
      if ( name == "light_up" )
        return new strong_light_up_t( effect, this, options_str, parent_action );

      return base_t::create_action( name, options_str );
    }

    void init_action_list() override
    {
      base_t::init_action_list();
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "not_so_gentle_flame" );
      def->add_action( "light_up" );
    }
  };

  struct candle_confidant_t : public generic_proc_t
  {
    spawner::pet_spawner_t<waxx_pet_t> waxx_spawner;
    spawner::pet_spawner_t<wayne_pet_t> wayne_spawner;
    spawner::pet_spawner_t<take_pet_t> take_spawner;

    candle_confidant_t( const special_effect_t& e )
      : generic_proc_t( e, "candle_confidant", e.driver() ),
        waxx_spawner( "waxx", e.player ),
        wayne_spawner( "wayne", e.player ),
        take_spawner( "take", e.player )
    {
      auto waxx_summon_spell = e.player->find_spell( 455445 );
      auto waxx = new action_t( action_e::ACTION_OTHER, "waxx", e.player, waxx_summon_spell );
      waxx->name_str_reporting = "Waxx";
      waxx_spawner.set_creation_callback( [ &e, waxx ]( player_t* ) { return new waxx_pet_t( e, waxx ); } );
      waxx_spawner.set_default_duration( waxx_summon_spell->duration() );
      add_child( waxx );

      auto wayne_summon_spell = e.player->find_spell( 455448 );
      auto wayne = new action_t( action_e::ACTION_OTHER, "wayne", e.player, wayne_summon_spell );
      wayne->name_str_reporting = "Wayne";
      wayne_spawner.set_creation_callback( [ &e, wayne ]( player_t* ) { return new wayne_pet_t( e, wayne ); } );
      wayne_spawner.set_default_duration( wayne_summon_spell->duration() );
      add_child( wayne );

      auto take_summon_spell = e.player->find_spell( 455453 );
      auto take = new action_t( action_e::ACTION_OTHER, "take", e.player, take_summon_spell );
      take->name_str_reporting = "Take";
      take_spawner.set_creation_callback( [ &e, take ]( player_t* ) { return new take_pet_t( e, take ); } );
      take_spawner.set_default_duration( take_summon_spell->duration() );
      add_child( take );
    }

    void execute() override
    {
      generic_proc_t::execute();
      int pet_id = rng().range( 0, 3 );
      switch ( pet_id )
      {
        case 0:  waxx_spawner.spawn(); break;
        case 1:  wayne_spawner.spawn(); break;
        case 2:  take_spawner.spawn(); break;
        default: break;
      }
    }
  };

  effect.execute_action = create_proc_action<candle_confidant_t>( "candle_confidant", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// 435493 driver, buff
// 453425 hidden, unused?
// 440235 stun, NYI
void concoction_kiss_of_death( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "concoction_kiss_of_death" } ) )
    return;

  struct concoction_kiss_of_death_buff_t : public stat_buff_t
  {
    concoction_kiss_of_death_buff_t( player_t* p, std::string_view n, const spell_data_t* s, const item_t* i )
      : stat_buff_t( p, n, s, i )
    {
      if ( p->thewarwithin_opts.concoction_kiss_of_death_buff_remaining_max <
           p->thewarwithin_opts.concoction_kiss_of_death_buff_remaining_min )
      {
        sim->error( "thewarwithin.concoction_kiss_of_death_buff_remaining_max must be greater than or equal to "
                    "thewarwithin.concoction_kiss_of_death_buff_remaining_min." );

        p->thewarwithin_opts.concoction_kiss_of_death_buff_remaining_max =
          p->thewarwithin_opts.concoction_kiss_of_death_buff_remaining_min;
      }
    }

    void start( int s, double v, timespan_t d ) override
    {
      if ( d < 0_ms )
      {
        d = ( buff_duration() * get_time_duration_multiplier() ) -
            rng().range( player->thewarwithin_opts.concoction_kiss_of_death_buff_remaining_min,
                         player->thewarwithin_opts.concoction_kiss_of_death_buff_remaining_max );
      }

      stat_buff_t::start( s, v, d );
    }
  };

  // TODO: the driver has two cooldown categories, 1141 for the on-use and 2338 for the charge. currently the generation
  // script prioritizes the charge category so we manually set it here until the script can be adjusted.
  effect.cooldown_category_ = 1141;
  effect.custom_buff = create_buff<concoction_kiss_of_death_buff_t>( effect.player, effect.driver(), effect.item );
}

// 435473 driver
//  e1: coeff
// 440635 stacking counter
// 440645 damage driver
// 440646 damage
void everburning_lantern( special_effect_t& effect )
{
  // setup primary proc driver & counter buff
  auto counter = create_buff<buff_t>( effect.player, effect.player->find_spell( 440635 ) )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  effect.custom_buff = counter;
  auto cb = new dbc_proc_callback_t( effect.player, effect );

  // setup damage proc callback driver & buff
  auto fireflies = create_buff<buff_t>( effect.player, effect.player->find_spell( 440645 ) )
    ->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
      if ( new_ )
        cb->deactivate();
      else
        cb->activate();
    } );

  counter->set_expire_callback( [ fireflies ]( buff_t*, int s, timespan_t ) {
    fireflies->trigger( -1, as<double>( s ) );
  } );

  auto on_next = new special_effect_t( effect.player );
  on_next->name_str = fireflies->name();
  on_next->spell_id = fireflies->data().id();
  effect.player->special_effects.push_back( on_next );

  auto damage_cb = new dbc_proc_callback_t( effect.player, *on_next );
  damage_cb->activate_with_buff( fireflies );

  // setup damage
  auto damage = create_proc_action<generic_proc_t>( "fire_flies", effect, 440646 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.player->callbacks.register_callback_execute_function( on_next->spell_id,
    [ fireflies, damage ]( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
      if ( !fireflies->check() )  // prevent extra procs from multi-part hits
        return;

      auto stacks = as<unsigned>( fireflies->check_value() );
      fireflies->expire();

      while ( stacks > 0 )
      {
        damage->execute_on_target( s->target );
        stacks--;
      }
    } );
}

// 455484 driver
//  e1: coeff
//  e2: unknown
// 455487 damage
void detachable_fang( special_effect_t& effect )
{
  struct gnash_t : public generic_proc_t
  {
    player_t* gnash_target = nullptr;
    unsigned count = 0;

    gnash_t( const special_effect_t& e ) : generic_proc_t( e, "gnash", 455487 )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );
    }

    void reset() override
    {
      gnash_target = nullptr;
      count = 1;
    }

    void execute() override
    {
      generic_proc_t::execute();

      if ( count == 3 )
        count = 1;
      else
        count++;
    }

    void set_target( player_t* t ) override
    {
      generic_proc_t::set_target( t );

      if ( !gnash_target || gnash_target != t )
      {
        gnash_target = t;
        count = 1;
      }
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      auto ctm = generic_proc_t::composite_target_multiplier( t );

      if ( count == 3 )
      {
        // These values are not in spell data and may be hard scripted or based on a curve id
        auto hp = t->health_percentage();
        if ( hp < 30 )
          ctm *= 1.5;
        else if ( hp < 60 )
          ctm *= 1.25;
        else if ( hp < 90 )
          ctm *= 1.1;
      }

      return ctm;
    }
  };

  effect.execute_action = create_proc_action<gnash_t>( "gnash", effect );

  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_execute_function( effect.driver()->id(),
    []( const dbc_proc_callback_t* cb, action_t*, const action_state_t* s ) {
      if ( cb->listener->get_player_distance( *s->target ) <= cb->proc_action->range )
        cb->proc_action->execute_on_target( s->target );
    } );
}

// 459222 driver
// 459224 stacking buff
// 459228 max buff + driver
// 459231 damage
void scroll_of_momentum( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "full_momentum" } ) )
    return;

  // setup stacking buff + driver
  auto counter = create_buff<buff_t>( effect.player, effect.player->find_spell( 459224 ) )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED )
    ->add_invalidate( CACHE_RUN_SPEED )
    ->set_expire_at_max_stack( true );

  effect.player->buffs.building_momentum = counter;
  effect.custom_buff = counter;

  auto cb = new dbc_proc_callback_t( effect.player, effect );

  // setup max buff
  auto max = create_buff<buff_t>( effect.player, effect.player->find_spell( 459228 ) )
    ->set_default_value_from_effect_type( A_MOD_INCREASE_SPEED )
    ->add_invalidate( CACHE_RUN_SPEED )
    ->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
      if ( new_ )
        cb->deactivate();
      else
        cb->activate();
    } );

  effect.player->buffs.full_momentum = max;

  counter->set_expire_callback( [ max ]( buff_t*, int, timespan_t ) {
    max->trigger();
  } );

  // setup damage
  struct high_velocity_impact_t : public generic_proc_t
  {
    high_velocity_impact_t( const special_effect_t& e ) : generic_proc_t( e, "highvelocity_impact", 459231 )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );
    }

    double action_multiplier() const override
    {
      // hard scripted values, not in spell data
      return generic_proc_t::action_multiplier() * ( player->composite_movement_speed() + 17 ) * 0.04;
    }
  };

  auto full_momentum = new special_effect_t( effect.player );
  full_momentum->name_str = max->name();
  full_momentum->spell_id = max->data().id();
  full_momentum->execute_action = create_proc_action<high_velocity_impact_t>( "highvelocity_impact", effect );
  effect.player->special_effects.push_back( full_momentum );

  auto damage_cb = new dbc_proc_callback_t( effect.player, *full_momentum );
  damage_cb->activate_with_buff( max );
}

// 442429 driver
// 442801 damage
void wildfire_wick( special_effect_t& effect )
{
  auto dot = create_proc_action<generic_proc_t>( "wildfire_wick", effect, effect.trigger() );
  dot->base_td = effect.driver()->effectN( 1 ).average( effect );
  dot->base_multiplier *= role_mult( effect.player );  // not found on spell desc

  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_execute_function( effect.driver()->id(),
    [ p = effect.player ]( const dbc_proc_callback_t* cb, action_t*, const action_state_t* s ) {
      if ( cb->proc_action->get_dot( s->target )->is_ticking() )
      {
        if ( auto tnsl = p->sim->target_non_sleeping_list.data(); tnsl.size() > 1 )  // make a copy
        {
          range::erase_remove( tnsl, s->target );
          cb->proc_action->execute_on_target( p->rng().range( tnsl ) );
        }
      }

      cb->proc_action->execute_on_target( s->target );
    } );
}

// 455452 equip
// 455464 counter buff
// 455467 on-use
void kaheti_shadeweavers_emblem( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "kaheti_shadeweavers_dark_ritual" } ) )
    return;

  struct kaheti_shadeweavers_emblem_t : public generic_proc_t
  {
    buff_t* counter;

    kaheti_shadeweavers_emblem_t( const special_effect_t& e )
      : generic_proc_t( e, "kaheti_shadeweavers_emblem", 455467 )
    {
      base_multiplier *= role_mult( e );

      unsigned equip_id = 455452;
      auto equip = find_special_effect( e.player, equip_id );
      assert( equip && "Kaheti Shadeweaver's Emblem missing equip effect" );

      counter = create_buff<buff_t>( e.player, equip->trigger() );
      equip->custom_buff = counter;

      new dbc_proc_callback_t( e.player, *equip );
    }

    double action_multiplier() const override
    {
      return generic_proc_t::action_multiplier() * counter->check();
    }

    void execute() override
    {
      generic_proc_t::execute();
      counter->expire();
    }
  };

  effect.execute_action = create_proc_action<kaheti_shadeweavers_emblem_t>( "kaheti_shadeweavers_emblem", effect );
}

// 469927 driver
// 469928 damage
// TODO: confirm if rolemult gets implemented in-game
void hand_of_justice( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_proc_t>( "quick_strike", effect, 469928 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

// 469915 driver
//  e1: counter threshold
//  e2: damage coeff
// 469917 counter
// 469918 unknown
// 469919 missile, trigger damage
// 469920 damage
//  e1: damage coeff (unused?)
void golem_gearbox( special_effect_t& effect )
{
  auto counter = create_buff<buff_t>( effect.player, effect.player->find_spell( 469917 ) )
    ->set_max_stack( as<int>( effect.driver()->effectN( 1 ).base_value() ) );

  auto missile = create_proc_action<generic_proc_t>( "torrent_of_flames", effect, 469919 );
  auto damage =
    create_proc_action<generic_proc_t>( "torrent_of_flames_damage", effect, missile->data().effectN( 1 ).trigger() );
  damage->dual = damage->background = true;
  // TODO: confirm driver coeff is used and not damage spell coeff
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 2 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  missile->add_child( damage );
  missile->impact_action = damage;

  effect.proc_flags2_ = PF2_CRIT;
  effect.custom_buff = counter;
  effect.execute_action = missile;

  new dbc_proc_callback_t( effect.player, effect );
}

// 469922 driver, trigger damage
// 469924 damage
void doperels_calling_rune( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_proc_t>( "ghostly_ambush", effect, effect.trigger() );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// 469925 on-use, buff, driver, trigger int buff
// 469925 int buff
// TODO: determine what can and cannot proc the int buff. mana cost does not seem to be a determining factor, as it will
// proc from harmful actions with no mana or resource cost, and buffs that reduce resource cost to 0 still allow the
// ability to proc the int buff. on the other hand, some mana cost non-harmful spell will not proc.
void burst_of_knowledge( special_effect_t& effect )
{
  auto int_buff = create_buff<stat_buff_t>( effect.player, effect.trigger(), effect.item );

  auto buff = create_buff<buff_t>( effect.player, effect.driver() )
                  ->set_cooldown( 0_ms )
                  ->set_expire_callback( [ int_buff ]( buff_t*, int, timespan_t ) { int_buff->expire(); } );

  effect.has_use_buff_override = true;
  effect.custom_buff           = buff;

  auto on_use_cb         = new special_effect_t( effect.player );
  on_use_cb->name_str    = effect.name() + "_cb";
  on_use_cb->spell_id    = effect.driver()->id();
  on_use_cb->cooldown_   = effect.driver()->internal_cooldown();
  on_use_cb->custom_buff = int_buff;
  effect.player->special_effects.push_back( on_use_cb );

  auto cb = new dbc_proc_callback_t( effect.player, *on_use_cb );
  cb->activate_with_buff( buff );
}

void heart_of_roccor( special_effect_t& effect )
{
  // Currently missing the misc value for the buff type, manually setting it for now.
  // Implementation will probably be redundant once its fixed.
  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger(), effect.item )
                  ->add_stat( STAT_STRENGTH, effect.trigger()->effectN( 1 ).average( effect ) );

  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// Wayward Vrykul's Lantern
// 467767 Driver
// 472360 Buff
void wayward_vrykuls_lantern( special_effect_t& effect )
{
  struct wayward_vrykuls_lantern_cb_t : public dbc_proc_callback_t
  {
    std::set<unsigned> proc_spell_id;
    buff_t* buff;
    double duration_multiplier;
    timespan_t last_activation_time;
    timespan_t max_dur;

    wayward_vrykuls_lantern_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        proc_spell_id(),
        buff( nullptr ),
        duration_multiplier( 0 ),
        last_activation_time( timespan_t::zero() ),
        max_dur( timespan_t::zero() )
    {
      for ( auto& s : e.player->dbc->spells_by_label( 690 ) )
        if ( s->is_class( e.player->type ) )
        {
          proc_spell_id.insert( s->id() );
          e.player->sim->print_debug( "Wayward Vrykul's can Proc off of Spell: {}: {}\n", s->name_cstr(), s->id() );
        }

      buff = create_buff<stat_buff_t>( e.player, e.trigger(), e.item )
                 ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 1 ).average( e ) );

      // Odd formula, but, appears to be 0.222222~ seconds added to the buff duration per 1 second of time between
      // activations.
      duration_multiplier = buff->data().duration().total_seconds() / 36;
      max_dur             = buff->data().duration() * 5;
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( range::contains( proc_spell_id, a->data().id() ) )
        return dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t*, action_state_t* ) override
    {
      // Assume for the first proc its been at least 3 minutes since the last proc, triggering it at the maximum
      // duration.
      if ( last_activation_time == timespan_t::zero() )
      {
        buff->trigger( max_dur );
        last_activation_time = listener->sim->current_time();
        return;
      }

      // If there was a valid last activation time, calculate the duration based on the time since the last activation.
      timespan_t trigger_dur = std::max(
          buff->buff_duration(),
          std::min( max_dur, ( listener->sim->current_time() - last_activation_time ) * duration_multiplier ) );

      // Only trigger the buff if the new duration would be greater than the current remaining duration.
      if ( buff->remains() < trigger_dur )
      {
        buff->trigger( trigger_dur );
        last_activation_time = listener->sim->current_time();
      }
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      last_activation_time = timespan_t::zero();
    }
  };

  effect.proc_flags2_ = PF2_ALL_CAST;

  new wayward_vrykuls_lantern_cb_t( effect );
}

// Cursed Pirate Skull
// 468035 Driver
// 472228 Buff
// 472232 Damage
void cursed_pirate_skull( special_effect_t& effect )
{
  auto damage_spell   = effect.trigger()->effectN( 1 ).trigger();
  auto damage         = create_proc_action<generic_proc_t>( "cursed_pirate_skull", effect, damage_spell );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->aoe         = damage_spell->max_targets();
  // No Role Mult currently, likely to change in the future.
  // damage->base_multiplier *= role_mult( effect );

  auto buff = create_buff<buff_t>( effect.player, effect.trigger(), effect.item )
                  ->set_tick_callback( [ damage ]( buff_t*, int, timespan_t ) { damage->execute(); } );

  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// Runecaster's Stormbound Rune
// 468033 Driver
// 472636 Periodic Trigger Buff
// 472637 Damage
void runecasters_stormbound_rune( special_effect_t& effect )
{
  auto damage_spell   = effect.player->find_spell( 472637 );
  auto damage         = create_proc_action<generic_proc_t>( "runecasters_stormbound_rune", effect, damage_spell );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  // No Role Mult currently, likely to change in the future.
  // damage->base_multiplier *= role_mult( effect );

  auto buff_spell = effect.player->find_spell( 472636 );
  auto buff       = create_buff<buff_t>( effect.player, buff_spell )
                  ->set_tick_on_application( true )
                  ->set_tick_callback( [ &, damage ]( buff_t*, int, timespan_t ) {
                    if ( effect.player->sim->target_non_sleeping_list.size() > 0 )
                    {
                      auto target = effect.player->rng().range( effect.player->sim->target_non_sleeping_list );
                      damage->execute_on_target( target );
                    }
                  } );

  // TODO: Test if initial hit is truly on hit, or only enter combat. Proc flags on the driver are combat start.
  effect.player->register_on_combat_state_callback( [ buff ]( player_t*, bool c ) {
    if ( c )
      buff->trigger();
    else
      buff->expire();
  } );
}


// Darktide Wavebender's Orb
// 468034 Driver
// 472336 Missile
// 472337 Damage
void darktide_wavebenders_orb( special_effect_t& effect )
{
  auto damage_spell   = effect.player->find_spell( 472337 );
  auto damage         = create_proc_action<generic_proc_t>( "darktide_wavebenders_orb", effect, damage_spell );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->aoe         = damage_spell->max_targets();
  // No Role Mult currently, likely to change in the future.
  // damage->base_multiplier *= role_mult( effect );

  auto missile_spell = effect.player->find_spell( 472336 );
  auto missile       = create_proc_action<generic_proc_t>( "darktide_wavebenders_orb_missile", effect, missile_spell );
  missile->impact_action = damage;

  effect.execute_action = missile;
  new dbc_proc_callback_t( effect.player, effect );
}

// Torq's Big Red Button
// 470042 Values
// 470286 Driver & Stat Buff
// 472787 Stacking Buff
// 472784 Damage
void torqs_big_red_button( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct spiteful_zapbolt_t : public generic_proc_t
  {
    buff_t* stack_buff;
    const spell_data_t* value_spell;

    spiteful_zapbolt_t( const special_effect_t& e, buff_t* b, const spell_data_t* value )
      : generic_proc_t( e, "spiteful_zapbolt", 472784 ), stack_buff( b ), value_spell( value )
    {
      base_dd_min = base_dd_max = value_spell->effectN( 2 ).average( e );
      base_multiplier *= role_mult( e );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double v = generic_proc_t::composite_da_multiplier( s );

      // TODO: Double Check this formula is correct once this is testable. Might carry over to the next use?
      v *= 1.0 + ( ( stack_buff->max_stack() - stack_buff->stack() ) * value_spell->effectN( 3 ).percent() );

      return v;
    }

    void execute() override
    {
      generic_proc_t::execute();
      stack_buff->decrement();
    }
  };

  struct torqs_big_red_button_t : public generic_proc_t
  {
    buff_t* stat_buff;
    buff_t* stack_buff;

    torqs_big_red_button_t( const special_effect_t& e, std::string_view name, const spell_data_t* spell )
      : generic_proc_t( e.player, name, spell ), stat_buff( nullptr ), stack_buff( nullptr )
    {
      auto value_spell = e.player->find_spell( 470042 );
      assert( value_spell && "Torq's Big Red Button missing value spell" );

      stat_buff        = create_buff<stat_buff_t>( e.player, e.driver(), e.item )
                      ->add_stat_from_effect( 1, value_spell->effectN( 1 ).average( e ) );

      stack_buff = create_buff<buff_t>( e.player, e.player->find_spell( 472787 ) )->set_reverse( true );

      auto damage = create_proc_action<spiteful_zapbolt_t>( "spiteful_zapbolt", e, stack_buff, value_spell );

      add_child( damage );

      auto on_next            = new special_effect_t( e.player );
      on_next->name_str       = stack_buff->name();
      on_next->spell_id       = stack_buff->data().id();
      on_next->execute_action = damage;
      e.player->special_effects.push_back( on_next );

      auto cb = new dbc_proc_callback_t( e.player, *on_next );
      cb->activate_with_buff( stack_buff );
    }

    void execute() override
    {
      generic_proc_t::execute();
      stat_buff->trigger();
      stack_buff->trigger();
    }
  };

  effect.execute_action = create_proc_action<torqs_big_red_button_t>( "torqs_big_red_button", effect,
                                                                      "torqs_big_red_button", effect.driver() );
}

// House of Cards
// 466681 Driver
// 466680 Values
// 1219158 Stacking Mastery
void house_of_cards( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto value_spell = effect.player->find_spell( 466680 );
  assert( value_spell && "House of Cards missing value spell" );

  auto stacking_buff = create_buff<buff_t>( effect.player, effect.player->find_spell( 1219158 ) );

  effect.player->register_on_combat_state_callback( [ stacking_buff ]( player_t* p, bool c ) {
    if ( !c && !p->sim->event_mgr.canceled )
      stacking_buff->expire();
  } );

  struct house_of_cards_buff_t : public stat_buff_t
  {
    double range_min;
    double range_max;
    double stack_buff_mod;
    buff_t* stack_buff;

    house_of_cards_buff_t( player_t* p, std::string_view n, const special_effect_t& e, const spell_data_t* s,
                           buff_t* stack )
      : stat_buff_t( p, n, e.driver(), e.item ),
        range_min( 0 ),
        range_max( 0 ),
        stack_buff_mod( 0 ),
        stack_buff( stack )
    {
      add_stat_from_effect_type( A_MOD_RATING, s->effectN( 1 ).average( e ) );
      default_value  = s->effectN( 1 ).average( e );
      range_min      = 1.0 - ( s->effectN( 2 ).base_value() / 100 );
      range_max      = 1.0 + ( s->effectN( 2 ).base_value() / 100 );
      stack_buff_mod = default_value * ( s->effectN( 2 ).base_value() / 100 / 3 );
    }

    double randomize_stat_value()
    {
      double v = default_value * rng().range( range_min, range_max );
      if ( stack_buff->check() )
        v += stack_buff_mod * stack_buff->check();

      return v;
    }

    void start( int s, double, timespan_t d ) override
    {
      stat_buff_t::start( s, randomize_stat_value(), d );
      stack_buff->trigger();
    }
  };

  effect.custom_buff =
      create_buff<house_of_cards_buff_t>( effect.player, "house_of_cards", effect, value_spell, stacking_buff );
}

struct external_action_state_t : public action_state_t
{
  player_t* faked_player;

  external_action_state_t( action_t* a, player_t* t ) : action_state_t( a, t ), faked_player( nullptr )
  {
  }

  void initialize() override
  {
    action_state_t::initialize();
    faked_player = nullptr;
  }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    faked_player = static_cast<const external_action_state_t*>( o )->faked_player;
  }
};

template <typename ab>
struct external_scaling_proc_t : public ab
{
public:
  player_t* faked_player;

  external_scaling_proc_t( const special_effect_t& effect, ::util::string_view name, const spell_data_t* s )
    : ab( effect, name, s ), faked_player( effect.player )
  {
  }

  external_scaling_proc_t( player_t* p, ::util::string_view name, const spell_data_t* s, const item_t* i = nullptr )
    : ab( p, name, s, i ), faked_player( p )
  {
  }

  action_state_t* new_state() override
  {
    return new external_action_state_t( this, ab::target );
  }

  external_action_state_t* cast_state( action_state_t* s )
  {
    return static_cast<external_action_state_t*>( s );
  }

  const external_action_state_t* cast_state( const action_state_t* s ) const
  {
    return static_cast<const external_action_state_t*>( s );
  }

  player_t* p( const action_state_t* state )
  {
    return cast_state( state )->faked_player;
  }

  player_t* p( const action_state_t* state ) const
  {
    return cast_state( state )->faked_player;
  }

private:
  using ab::composite_crit_chance;
  using ab::composite_crit_chance_multiplier;
  using ab::composite_haste;
  using ab::composite_target_armor;
  using ab::composite_target_crit_chance;
  using ab::composite_target_da_multiplier;
  using ab::composite_target_multiplier;
  using ab::composite_target_ta_multiplier;

public:
  double composite_attack_power( const action_state_t* s ) const
  {
    return p( s )->composite_total_attack_power_by_type( ab::get_attack_power_type() );
  }

  double composite_spell_power( const action_state_t* s ) const
  {
    double spell_power = 0;
    double tmp;

    auto _player = p( s );

    for ( auto base_school : ab::base_schools )
    {
      tmp = _player->composite_total_spell_power( base_school );
      if ( tmp > spell_power )
        spell_power = tmp;
    }

    return spell_power;
  }

  double composite_versatility( const action_state_t* s ) const override
  {
    return action_t::composite_versatility( s ) + p( s )->cache.damage_versatility();
  }

  virtual double composite_crit_chance( const action_state_t* s ) const
  {
    return action_t::composite_crit_chance() + p( s )->cache.spell_crit_chance();
  }

  virtual double composite_haste( const action_state_t* s ) const
  {
    return action_t::composite_haste() * p( s )->cache.spell_cast_speed();
  }

  double composite_crit_chance_multiplier( const action_state_t* s ) const
  {
    return action_t::composite_crit_chance_multiplier() * p( s )->composite_spell_crit_chance_multiplier();
  }

  double composite_player_critical_multiplier( const action_state_t* s ) const override
  {
    return p( s )->composite_player_critical_damage_multiplier( s );
  }

  double composite_target_crit_chance( const action_state_t* s ) const
  {
    return p( s )->composite_player_target_crit_chance( s->target );
  }

  double composite_target_multiplier( const action_state_t* s ) const
  {
    return p( s )->composite_player_target_multiplier( s->target, ab::get_school() );
  }

  double composite_player_multiplier( const action_state_t* s ) const override
  {
    double player_school_multiplier = 0.0;
    double tmp;

    for ( auto base_school : ab::base_schools )
    {
      tmp = p( s )->cache.player_multiplier( base_school );
      if ( tmp > player_school_multiplier )
        player_school_multiplier = tmp;
    }

    return player_school_multiplier;
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    return p( s )->composite_persistent_multiplier( ab::get_school() );
  }

  virtual double composite_target_da_multiplier( const action_state_t* s ) const
  {
    return composite_target_multiplier( s );
  }

  virtual double composite_target_ta_multiplier( const action_state_t* s ) const
  {
    return composite_target_multiplier( s );
  }

  virtual double composite_target_armor( const action_state_t* s ) const
  {
    return p( s )->composite_player_target_armor( s->target );
  }

  void snapshot_internal( action_state_t* state, unsigned flags, result_amount_type rt ) override
  {
    assert( state );

    cast_state( state )->faked_player = faked_player;

    ab::snapshot_internal( state, flags, rt );

    if ( flags & STATE_CRIT )
      state->crit_chance = composite_crit_chance( state ) * composite_crit_chance_multiplier( state );

    if ( flags & STATE_HASTE )
      state->haste = composite_haste( state );

    if ( flags & STATE_AP )
      state->attack_power = composite_attack_power( state );

    if ( flags & STATE_SP )
      state->spell_power = composite_spell_power( state );

    if ( flags & STATE_TGT_MUL_DA )
      state->target_da_multiplier = composite_target_da_multiplier( state );

    if ( flags & STATE_TGT_MUL_TA )
      state->target_ta_multiplier = composite_target_ta_multiplier( state );

    if ( flags & STATE_TGT_CRIT )
      state->target_crit_chance = composite_target_crit_chance( state ) * composite_crit_chance_multiplier( state );

    // if ( flags & STATE_TGT_ARMOR )
    //   state->target_armor = composite_target_armor( state );
  }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    ab::snapshot_state( s, rt );
    cast_state( s )->faked_player = faked_player;
  }
};

template <typename T>
struct external_proc_cb_t : public dbc_proc_callback_t
{
  player_t* source;
  external_proc_cb_t( const special_effect_t& e, player_t* source )
    : dbc_proc_callback_t( e.player, e ), source( source )
  {
  }

  void execute( action_t* action, action_state_t* state ) override
  {
    debug_cast<external_scaling_proc_t<T>*>( proc_action )->faked_player = source;
    dbc_proc_callback_t::execute( action, state );
  }
};

// 443559 Driver
//  e1 - Primary
//  e2 - Secondary
//  e3 - Tertiary
//  e4 - Damage?
//  e5 - Healing?
//  e6 - Mana?
// 452365 DPS Proc Driver
// 452800 DPS Proc Action
// 452366 Healer Proc Driver - Healing
// 452801 Healer Proc Action
// 452361 Tank Proc Driver - Damage Taken - Overwrite with damage flags.
// 452804 Tank Proc Action
// 452337 Secondary Stat Proc - All Stats in effect, Grants Highest.
// 452288 Primary Stat Proc
void cirral_concoctory( special_effect_t& effect )
{
  struct cirral_concoctory_cb_t : public dbc_proc_callback_t
  {
    enum cirral_outcomes_e
    {
      LORD_PROC,
      ASCENDED_PROC,
      SUNDEERED_PROC,
      QUEEN_PROC,
      SAGE_PROC
    };

    const std::vector<std::pair<cirral_outcomes_e, double>> dps_cirral_weights = {
        { LORD_PROC, 0.4 },
        { ASCENDED_PROC, 0.4 },
        { SUNDEERED_PROC, 0.25 },
        { QUEEN_PROC, 0.02 },
    };

    const std::vector<std::pair<cirral_outcomes_e, double>> healer_cirral_weights = {
        { LORD_PROC, 0.4 }, { ASCENDED_PROC, 0.4 }, { SUNDEERED_PROC, 0.25 }, { QUEEN_PROC, 0.02 }, { SAGE_PROC, 0.08 },
    };

    const std::vector<std::pair<cirral_outcomes_e, double>> normalise_weights(
        const std::vector<std::pair<cirral_outcomes_e, double>>& to_norm )
    {
      std::vector<std::pair<cirral_outcomes_e, double>> normalised_vector;
      auto total_weight = 0.0;

      for ( auto [ _, weight ] : to_norm )
      {
        assert( weight >= 0 && "Weights must be positive" );
        total_weight += weight;
      }

      auto running_weight = 0.0;
      for ( auto [ outcome, weight ] : to_norm )
      {
        if ( weight > 0 )
        {
          running_weight += weight;
          normalised_vector.push_back( { outcome, running_weight / total_weight } );
        }
      }

      return normalised_vector;
    }

    const std::vector<std::pair<cirral_outcomes_e, double>> dps_cirral_normalised_weights;
    const std::vector<std::pair<cirral_outcomes_e, double>> healer_cirral_normalised_weights;

    target_specific_t<buff_t> primary_stat_buffs;
    target_specific_t<std::unordered_map<stat_e, buff_t*>> secondary_stat_buffs;
    target_specific_t<buff_t> proc_driver_buffs;
    std::vector<player_t*> potential_targets;

    external_scaling_proc_t<generic_heal_t>* healing_action;
    external_scaling_proc_t<generic_proc_t>* dps_action;
    external_scaling_proc_t<generic_aoe_proc_t>* tank_action;

    cirral_concoctory_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        dps_cirral_normalised_weights( normalise_weights( ( dps_cirral_weights ) ) ),
        healer_cirral_normalised_weights( normalise_weights( ( healer_cirral_weights ) ) ),
        primary_stat_buffs{ false },
        secondary_stat_buffs{ true },
        proc_driver_buffs{ false },
        potential_targets{},
        healing_action( nullptr ),
        dps_action( nullptr ),
        tank_action( nullptr )
    {
      create_actions();
    }

    void create_actions()
    {
      auto healing_action_spell = listener->find_spell( 452801 );
      healing_action = new external_scaling_proc_t<generic_heal_t>( listener, "strand_of_the_sundered_healer",
                                                                    healing_action_spell );
      healing_action->base_dd_min = healing_action->base_dd_max = effect.driver()->effectN( 5 ).average( effect );

      auto dps_action_spell = listener->find_spell( 452800 );
      dps_action =
          new external_scaling_proc_t<generic_proc_t>( listener, "strand_of_the_sundered_dps", dps_action_spell );
      dps_action->base_dd_min = dps_action->base_dd_max = effect.driver()->effectN( 4 ).average( effect );

      auto tank_action_spell = listener->find_spell( 452804 );
      tank_action =
          new external_scaling_proc_t<generic_aoe_proc_t>( listener, "strand_of_the_sundered_tank", tank_action_spell );
      tank_action->base_dd_min = tank_action->base_dd_max = effect.driver()->effectN( 4 ).average( effect );
    }

    buff_t* get_primary_buff( player_t* buff_player )
    {
      if ( primary_stat_buffs[ buff_player ] )
        return primary_stat_buffs[ buff_player ];

      auto buff_spell = effect.player->find_spell( 452288 );

      auto buff = make_buff<stat_buff_t>( actor_pair_t{ buff_player, effect.player }, "strand_of_the_lord", buff_spell )
                      ->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect ) );

      primary_stat_buffs[ buff_player ] = buff;

      return buff;
    }

    buff_t* get_secondary_buff( player_t* buff_player )
    {
      if ( secondary_stat_buffs[ buff_player ] )
        return secondary_stat_buffs[ buff_player ]->at( util::highest_stat( buff_player, secondary_ratings ) );

      auto buff_spell = effect.player->find_spell( 452337 );
      auto buff_name  = util::tokenize_fn( buff_spell->name_cstr() );
      auto amount     = effect.driver()->effectN( 2 ).average( effect );

      std::unordered_map<stat_e, buff_t*>* map_for_player = new std::unordered_map<stat_e, buff_t*>();

      for ( const auto& eff : buff_spell->effects() )
      {
        if ( eff.type() != E_APPLY_AURA || eff.subtype() != A_MOD_RATING )
          continue;

        auto stats = util::translate_all_rating_mod( eff.misc_value1() );
        auto stat  = stats.front();

        std::vector<std::string_view> stat_strs;
        range::transform( stats, std::back_inserter( stat_strs ), &util::stat_type_abbrev );

        auto name = fmt::format( "{}_{}", buff_name, util::string_join( stat_strs, "_" ) );
        auto buff = make_buff<stat_buff_t>( actor_pair_t{ buff_player, effect.player }, name, buff_spell )
                        ->add_stat( stat, amount ? amount : eff.average( effect ) )
                        ->set_name_reporting( util::string_join( stat_strs ) );

        ( *map_for_player )[ stat ] = buff;
      }

      secondary_stat_buffs[ buff_player ] = std::move( map_for_player );
      return secondary_stat_buffs[ buff_player ]->at( util::highest_stat( buff_player, secondary_ratings ) );
    }

    buff_t* get_proc_buff( player_t* buff_player )
    {
      if ( proc_driver_buffs[ buff_player ] )
        return proc_driver_buffs[ buff_player ];

      switch ( buff_player->primary_role() )
      {
        case ROLE_HEAL:
        case ROLE_HYBRID:
        {
          auto driver_spell = effect.player->find_spell( 452366 );

          auto buff_name = util::tokenize_fn( driver_spell->name_cstr() );

          auto driver_effect            = new special_effect_t( buff_player );
          driver_effect->name_str       = buff_name;
          driver_effect->spell_id       = driver_spell->id();
          driver_effect->execute_action = healing_action;
          buff_player->special_effects.push_back( driver_effect );

          auto buff = make_buff( actor_pair_t{ buff_player, effect.player }, buff_name, driver_spell );
          buff->set_chance( 1.0 )->set_rppm( RPPM_NONE );
          proc_driver_buffs[ buff_player ] = buff;

          auto healer_cb = new external_proc_cb_t<generic_heal_t>( *driver_effect, listener );
          healer_cb->activate_with_buff( buff, true );
          break;
        }
        case ROLE_TANK:
        {
          auto driver_spell     = effect.player->find_spell( 452361 );
          auto dps_driver_spell = effect.player->find_spell( 452365 );

          auto buff_name = util::tokenize_fn( driver_spell->name_cstr() );

          auto driver_effect            = new special_effect_t( buff_player );
          driver_effect->name_str       = buff_name;
          driver_effect->spell_id       = driver_spell->id();
          driver_effect->execute_action = tank_action;
          driver_effect->proc_flags_    = dps_driver_spell->proc_flags();
          buff_player->special_effects.push_back( driver_effect );

          auto buff = make_buff( actor_pair_t{ buff_player, effect.player }, buff_name, driver_spell );
          buff->set_chance( 1.0 )->set_rppm( RPPM_NONE );
          proc_driver_buffs[ buff_player ] = buff;

          auto tank_cb = new external_proc_cb_t<generic_aoe_proc_t>( *driver_effect, listener );
          tank_cb->activate_with_buff( buff, true );
          break;
        }
        default:
        {
          auto driver_spell = effect.player->find_spell( 452365 );
          auto buff_name    = util::tokenize_fn( driver_spell->name_cstr() );

          auto driver_effect            = new special_effect_t( buff_player );
          driver_effect->name_str       = buff_name;
          driver_effect->spell_id       = driver_spell->id();
          driver_effect->execute_action = dps_action;
          buff_player->special_effects.push_back( driver_effect );

          auto buff = make_buff( actor_pair_t{ buff_player, effect.player }, buff_name, driver_spell );
          buff->set_chance( 1.0 )->set_rppm( RPPM_NONE );
          proc_driver_buffs[ buff_player ] = buff;

          auto dps_cb = new external_proc_cb_t<generic_proc_t>( *driver_effect, listener );
          dps_cb->activate_with_buff( buff, true );
          break;
        }
      }

      return proc_driver_buffs[ buff_player ];
    }

    cirral_outcomes_e roll_for_outcome( const std::vector<std::pair<cirral_outcomes_e, double>> normalised_weights )
    {
      auto roll = rng().real();

      for ( auto [ outcome, weight ] : normalised_weights )
      {
        if ( weight > roll )
          return outcome;
      }

      assert( false && "This should be unreachable. Invalid Weights setup found." );
      return LORD_PROC;
    }

    void trigger_for_player( player_t* buff_player, cirral_outcomes_e outcome )
    {
      switch ( outcome )
      {
        case LORD_PROC:
          get_primary_buff( buff_player )->trigger();
          break;
        case ASCENDED_PROC:
          get_secondary_buff( buff_player )->trigger();
          break;
        case SUNDEERED_PROC:
          get_proc_buff( buff_player )->trigger();
          break;
        case QUEEN_PROC:
        case SAGE_PROC:
        default:
          break;
      }
    }

    void trigger_for_player( player_t* buff_player )
    {
      if ( buff_player->primary_role() == ROLE_HEAL || buff_player->primary_role() == ROLE_HYBRID )
      {
        trigger_for_player( buff_player, roll_for_outcome( healer_cirral_normalised_weights ) );
      }
      else
      {
        trigger_for_player( buff_player, roll_for_outcome( dps_cirral_normalised_weights ) );
      }
    }

    void execute( action_t*, action_state_t* ) override
    {
      potential_targets.clear();

      for ( auto& potential_target : listener->sim->player_no_pet_list )
      {
        if ( potential_target != listener && !potential_target->is_sleeping() && potential_target->is_player() )
        {
          potential_targets.push_back( potential_target );
        }
      }

      if ( potential_targets.size() == 0 )
        return;

      auto potential_target = rng().range( potential_targets );
      // Healers are about one third as likely, probably. For now this is a good approximation.
      if ( ( potential_target->primary_role() == ROLE_HEAL || potential_target->primary_role() == ROLE_HYBRID ) &&
           rng().roll( 0.75 ) )
        potential_target = rng().range( potential_targets );

      trigger_for_player( potential_target );
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE | PF2_PERIODIC_HEAL;

  if ( !effect.player->sim->single_actor_batch )
    new cirral_concoctory_cb_t( effect );
}

// Eye of Kezan
// 469888 Driver
// 469889 Buff
// 1216593 Damage
// 1216594 Heal
void eye_of_kezan( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct eye_of_kezan_cb_t : public dbc_proc_callback_t
  {
    buff_t* stat_buff;
    action_t* damage;
    action_t* heal;

    eye_of_kezan_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), stat_buff( nullptr ), damage( nullptr ), heal( nullptr )
    {
      stat_buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 469889 ) )
                      ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) );

      auto out_of_combat_buff =
          create_buff<stat_buff_t>( e.player, "eye_of_kezan_out_of_combat", e.player->find_spell( 469889 ) )
              ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) )
              ->set_tick_callback(
                  []( buff_t* b, int, timespan_t ) { make_event( *b->source->sim, 0_ms, [ b ] { b->decrement(); } ); } )
              ->set_duration( 20_s );  // Doesnt appear to be in data

      e.player->register_on_combat_state_callback( [ &, out_of_combat_buff ]( player_t* p, bool c ) {
        if ( c && !stat_buff->check() && out_of_combat_buff->check() )
        {
          stat_buff->trigger( out_of_combat_buff->check() );
          out_of_combat_buff->expire();
        }
        if ( !c && !p->sim->event_mgr.canceled && stat_buff->check() && !out_of_combat_buff->check() )
        {
          out_of_combat_buff->trigger( stat_buff->check() );
          stat_buff->expire();
        }
      } );

      damage              = create_proc_action<generic_proc_t>( "wrath_of_kezan", e, e.player->find_spell( 1216593 ) );
      damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 2 ).average( e );
      damage->base_multiplier *= role_mult( e );

      heal              = new heal_t( "vigor_of_kezan", e.player, e.player->find_spell( 1216594 ) );
      heal->base_dd_min = heal->base_dd_max = e.driver()->effectN( 3 ).average( e );
    }

    void execute( action_t*, action_state_t* s ) override
    {
      if ( !stat_buff->at_max_stacks() )
      {
        stat_buff->trigger();
        return;
      }

      switch ( s->target->is_enemy() )
      {
        case true:
          damage->execute();
          break;
        case false:
          heal->execute_on_target( s->target );
          break;
      }
    }
  };

  new eye_of_kezan_cb_t( effect );
}

// Geargrinder's Remote
// 471059 Driver
// 471058 Value
// 472030 Damage
void geargrinders_remote( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto value_spell = effect.player->find_spell( 471058 );
  assert( value_spell && "Geargrinder's Remote missing Value spell" );
  auto damage_spell = effect.player->find_spell( 472030 );
  assert( damage_spell && "Geargrinder's Remote missing Damage spell" );

  auto damage         = create_proc_action<generic_aoe_proc_t>( "blaze_of_glory", effect, damage_spell, true );
  damage->base_dd_min = damage->base_dd_max = value_spell->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;
}

// Improvised Seaforium Pacemaker
// 1218714 Driver
// 1218713 Buff
void improvised_seaforium_pacemaker( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto buff_spell = effect.player->find_spell( 1218713 );
  auto buff       = create_buff<stat_buff_t>( effect.player, buff_spell )
                  ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

  struct explosive_adrenaline_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;
    int max_extensions;
    int extensions;
    explosive_adrenaline_cb_t( const special_effect_t& e, buff_t* b )
      : dbc_proc_callback_t( e.player, e ), buff( b ), max_extensions( 0 ), extensions( 0 )
    {
      // Max extensions doesnt appear to be in spell data, basing it off tooltip for now.
      max_extensions = 15;
      buff->set_expire_callback( [ & ]( buff_t*, int, timespan_t ) { extensions = 0; } );
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( extensions++ < max_extensions )
      {
        // Duration extension doesnt appear to be in spell data. Manually setting for now.
        buff->extend_duration( listener, 1_s );
      }
    }

    void reset() override
    {
      extensions = 0;
    }
  };

  auto buff_extension          = new special_effect_t( effect.player );
  buff_extension->name_str     = buff_spell->name_cstr();
  buff_extension->spell_id     = buff_spell->id();
  buff_extension->proc_flags_  = PF_ALL_DAMAGE | PF_ALL_HEAL;
  buff_extension->proc_flags2_ = PF2_CRIT;
  effect.player->special_effects.push_back( buff_extension );

  auto cb = new explosive_adrenaline_cb_t( *buff_extension, buff );
  cb->activate_with_buff( buff );

  effect.cooldown_ = timespan_t::from_seconds( effect.driver()->effectN( 2 ).base_value() );
  new dbc_proc_callback_t( effect.player, effect );
}

// Reverb Radio
// 471567 Driver
// 1216212 Buff
void reverb_radio( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct hyped_buff_t : public stat_buff_t
  {
    timespan_t amped_duration;
    double amped_value;
    bool amped;
    hyped_buff_t( player_t* p, util::string_view name, const spell_data_t* s, const special_effect_t& e,
                  const item_t* item = nullptr )
      : stat_buff_t( p, name, s, item ), amped_duration( 0_ms ), amped_value( 0 ), amped( false )
    {
      amped_duration = timespan_t::from_seconds( e.driver()->effectN( 3 ).base_value() );
      double value   = e.driver()->effectN( 1 ).average( e );
      set_stat_from_effect_type( A_MOD_RATING, value );
      amped_value = 1.0 + e.driver()->effectN( 2 ).percent();
    }

    void expire_override( int s, timespan_t d ) override
    {
      if ( amped )
      {
        for ( auto& buff_stat : stats )
        {
          player->stat_loss( buff_stat.stat, buff_stat.current_value, stat_gain, nullptr,
                             buff_duration() > timespan_t::zero() );

          buff_stat.current_value = 0;
        }
        amped = false;
        set_refresh_behavior( buff_refresh_behavior::DURATION );
        buff_t::expire_override( s, d );
      }
      else
        stat_buff_t::expire_override( s, d );
    }

    void reset() override
    {
      stat_buff_t::reset();
      amped = false;
    }

    void amp_it_up()
    {
      trigger( max_stack(), amped_duration );
      amped = true;
      set_refresh_behavior( buff_refresh_behavior::DISABLED );

      for ( auto& buff_stat : stats )
      {
        if ( buff_stat.check_func && !buff_stat.check_func( *this ) )
          continue;

        player->stat_gain( buff_stat.stat, buff_stat_stack_amount( buff_stat, current_stack ), stat_gain, nullptr,
                           buff_duration() > timespan_t::zero() );

        buff_stat.current_value *= amped_value;
      }
    }

    void bump( int stacks, double ) override
    {
      if ( at_max_stacks() && !amped )
      {
        make_event( *sim, 0_ms, [ & ] { expire(); } );
        make_event( *sim, 0_ms, [ & ] { amp_it_up(); } );
        return;
      }

      if ( !at_max_stacks() && !amped )
        stat_buff_t::bump( stacks );
    }
  };

  auto buff = create_buff<hyped_buff_t>( effect.player, "hyped", effect.player->find_spell( 1216212 ), effect );
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Mechano-core Amplifer
// 1214787 Driver
// e1: Highest stat value
// e2: Lowest stat value
// 1214806 Crit Buff
// 1214807 Haste Buff
// 1214808 Mastery Buff
// 1214810 Vers Buff
void mechanocore_amplifier( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct mechanocore_amplifier_buff_t : public stat_buff_t
  {
    double high_value;
    double low_value;
    mechanocore_amplifier_buff_t( player_t* p, util::string_view name, const spell_data_t* s, const special_effect_t& e,
                                  const item_t* item = nullptr )
      : stat_buff_t( p, name, s, item ), high_value( 0 ), low_value( 0 )
    {
      high_value = e.driver()->effectN( 1 ).average( e );
      low_value  = e.driver()->effectN( 2 ).average( e );
      set_default_value( 0 );
    }

    void set_given_stat_value( double val )
    {
      for ( auto& buffed_stat : stats )
      {
        buffed_stat.amount = val;
      }
    }

    void trigger_high()
    {
      set_given_stat_value( high_value );
      trigger();
    }

    void trigger_low()
    {
      set_given_stat_value( low_value );
      trigger();
    }
  };

  struct mechanocore_amplifier_cb_t : public dbc_proc_callback_t
  {
    std::unordered_map<stat_e, mechanocore_amplifier_buff_t*> buffs;

    mechanocore_amplifier_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), buffs()
    {
      static constexpr std::array<std::pair<stat_e, int>, 4> buff_spells = {
          std::make_pair( stat_e::STAT_CRIT_RATING, 1214806 ), std::make_pair( stat_e::STAT_HASTE_RATING, 1214807 ),
          std::make_pair( stat_e::STAT_MASTERY_RATING, 1214808 ),
          std::make_pair( stat_e::STAT_VERSATILITY_RATING, 1214810 ) };

      for ( auto& spell : buff_spells )
      {
        auto buff_spell     = e.player->find_spell( spell.second );
        auto buff_stat_name = util::stat_type_abbrev( spell.first );
        auto name           = fmt::format( "{}_{}", buff_spell->name_cstr(), buff_stat_name );
        auto buff           = create_buff<mechanocore_amplifier_buff_t>( e.player, name, buff_spell, e );
        buff->add_stat_from_effect_type( A_MOD_RATING, 0 );
        buff->set_name_reporting( buff_stat_name );
        buffs.insert( { spell.first, buff } );
      }
    }

    void execute( action_t*, action_state_t* ) override
    {
      bool highest = rng().roll( 0.5 );
      switch ( highest )
      {
        case true:
          buffs.at( util::highest_stat( listener, secondary_ratings ) )->trigger_high();
          break;
        case false:
          buffs.at( util::lowest_stat( listener, secondary_ratings ) )->trigger_low();
          break;
      }
    }
  };

  new mechanocore_amplifier_cb_t( effect );
}

// Papa's Prized Putter
// 1215238 Driver
// 1215321 Damage
void papas_prized_putter( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto damage         = create_proc_action<generic_proc_t>( "papas_prized_putter", effect, 1215321 );
  damage->base_dd_min = damage->base_dd_max = effect.player->find_spell( 1215321 )->effectN( 1 ).average( effect );
  damage->base_multiplier                   = role_mult( effect );

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

// Turbo-Drain 5000
// 472125 Driver
// 472127 DoT
void turbodrain_5000( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto dot_spell    = effect.player->find_spell( 472127 );
  auto dot          = create_proc_action<generic_proc_t>( "turbodrain_5000", effect, dot_spell );
  auto total_damage = effect.driver()->effectN( 1 ).average( effect );
  auto ticks        = dot_spell->duration() / dot_spell->effectN( 1 ).period();
  dot->base_td      = total_damage / ticks;
  // currently has no role mult
  // dot->base_td_multiplier = role_mult( effect );
  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );
}

// Noggenfogger Ultimate Deluxe
// 470675 Driver
// 471404 Summon Pet spell
// 233767 Pet NPC ID
// Pet currently only melee attacks.
void noggenfogger_ultimate_deluxe( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct auto_attack_melee_t : public melee_attack_t
  {
    auto_attack_melee_t( pet_t* p, std::string_view name = "main_hand", action_t* a = nullptr )
      : melee_attack_t( name, p )
    {
      this->background = this->repeating = true;
      this->not_a_proc = this->may_crit = true;
      this->special                     = false;
      this->weapon_multiplier           = 1.0;
      this->trigger_gcd                 = 0_ms;
      this->school                      = SCHOOL_PHYSICAL;
      this->stats->school               = SCHOOL_PHYSICAL;
      this->base_dd_min = this->base_dd_max = p->dbc->expected_stat( p->true_level ).creature_auto_attack_dps;
      this->base_multiplier                 = p->main_hand_weapon.swing_time.total_seconds();

      auto proxy = a;
      auto it    = range::find( proxy->child_action, name, &action_t::name );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }

    // Pet melee attacks seem to still scale with aura 380 and 531
    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = melee_attack_t::composite_da_multiplier( s );
      m *= this->player->cast_pet()->owner->composite_player_pet_damage_multiplier( s, type == PLAYER_GUARDIAN );
      return m;
    }

    double composite_target_multiplier( player_t* p ) const override
    {
      double m = melee_attack_t::composite_target_multiplier( p );
      m *= this->player->cast_pet()->owner->composite_player_target_pet_damage_multiplier( p, type == PLAYER_GUARDIAN );
      return m;
    }

    void execute() override
    {
      if ( this->player->executing )
        this->schedule_execute();
      else
        melee_attack_t::execute();
    }
  };

  struct blackwater_pirate_pet_t : public unique_gear_pet_t
  {
    blackwater_pirate_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : unique_gear_pet_t( "blackwater_pirate", e, e.player->find_spell( 471404 ) )
    {
      parent_action               = parent;
      main_hand_weapon.type       = WEAPON_BEAST;
      main_hand_weapon.swing_time = 2_s;
      use_auto_attack             = true;
    }

    attack_t* create_auto_attack() override
    {
      auto a                = new auto_attack_melee_t( this, "main_hand", parent_action );
      a->name_str_reporting = "Melee";
      return a;
    }
  };

  struct noggenfogger_ultimate_deluxe_t : public generic_proc_t
  {
    spawner::pet_spawner_t<blackwater_pirate_pet_t> pirate_spawner;

    noggenfogger_ultimate_deluxe_t( const special_effect_t& e )
      : generic_proc_t( e, "noggenfogger_utimate_deluxe", e.driver() ), pirate_spawner( "blackwater_pirate", e.player )
    {
      auto summon_spell          = e.player->find_spell( 471404 );
      auto pirate                = new action_t( action_e::ACTION_OTHER, "blackwater_pirate", e.player, summon_spell );
      pirate->name_str_reporting = "blackwater_pirate";
      pirate_spawner.set_creation_callback(
          [ &e, pirate ]( player_t* ) { return new blackwater_pirate_pet_t( e, pirate ); } );
      pirate_spawner.set_default_duration( summon_spell->duration() );
      add_child( pirate );
    }

    void execute() override
    {
      generic_proc_t::execute();
      pirate_spawner.spawn();
    }
  };

  // Name is currently typod in spell data, might need fixed if the data name changes.
  effect.execute_action = create_proc_action<noggenfogger_ultimate_deluxe_t>( "noggenfogger_utimate_deluxe", effect );
}

// Ratfang Toxin
// 1216605 Use Driver
// 1216603 Equip Driver & Values
// 1216604 Debuff
// 1216606 DoT
void ratfang_toxin( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto equip_driver = effect.player->find_spell( 1216603 );
  assert( equip_driver && "Ratfang Toxing missing Equip Driver" );

  struct ratfang_toxin_cb_t : public dbc_proc_callback_t
  {
    const spell_data_t* debuff_spell;

    ratfang_toxin_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), debuff_spell( e.player->find_spell( 1216604 ) )
    {
    }

    buff_t* create_debuff( player_t* t ) override
    {
      if ( t->is_enemy() )
      {
        return make_buff( actor_pair_t( t, listener ), "ratfang_toxin_debuff", debuff_spell );
      }
      else
        return nullptr;
    }

    void execute( action_t*, action_state_t* s ) override
    {
      get_debuff( s->target )->trigger();
    }
  };

  auto equip      = new special_effect_t( effect.player );
  equip->name_str = "ratfang_toxin_equip";
  equip->item     = effect.item;
  equip->spell_id = equip_driver->id();
  effect.player->special_effects.push_back( equip );

  auto equip_cb = new ratfang_toxin_cb_t( *equip );
  equip_cb->initialize();
  equip_cb->activate();

  struct ratfang_toxin_t : public generic_proc_t
  {
    buff_t* debuff;
    const spell_data_t* value_spell;
    double base_tick_damage;
    double debuff_increase;
    ratfang_toxin_cb_t& equip_callback;

    ratfang_toxin_t( const special_effect_t& e, std::string_view name, const spell_data_t* spell,
                     ratfang_toxin_cb_t& equip )
      : generic_proc_t( e, name, spell ),
        debuff( nullptr ),
        value_spell( e.player->find_spell( 1216603 ) ),
        base_tick_damage( 0 ),
        debuff_increase( 0 ),
        equip_callback( equip )
    {
      auto ticks = spell->duration() / spell->effectN( 1 ).period();
      base_tick_damage = value_spell->effectN( 2 ).average( e ) / ticks;
      base_td         = base_tick_damage;
      debuff_increase = value_spell->effectN( 3 ).average( e ) / ticks;
    }

    void execute() override
    {
      debuff = equip_callback.get_debuff( target );
      if ( debuff->up() )
      {
        base_td += debuff_increase * debuff->check();
        debuff->expire();
      }
      generic_proc_t::execute();
    }

    void last_tick( dot_t* d ) override
    {
      generic_proc_t::last_tick( d );
      base_td = base_tick_damage;
    }

    void reset() override
    {
      generic_proc_t::reset();
      base_td = base_tick_damage;
    }
  };

  auto use = create_proc_action<ratfang_toxin_t>( "ratfang_toxin_dot", effect, "ratfang_toxin_dot", effect.player->find_spell( 1216606 ), *equip_cb );

  effect.execute_action = use;
}

// Garbagemancer's Last Resort
// 1219294 Driver
// 1219314 Ground Trigger
// 1219296 Value spell
// 1219299 Damage
void garbagemancers_last_resort( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct garbagemancers_last_resort_t : public generic_proc_t
  {
    ground_aoe_params_t params;
    garbagemancers_last_resort_t( const special_effect_t& e, std::string_view n, const spell_data_t* s )
      : generic_proc_t( e, n, s )
    {
      auto value_spell    = e.player->find_spell( 1219296 );
      auto damage         = create_proc_action<generic_aoe_proc_t>( "garbocalypse", e, 1219299, true );
      damage->base_dd_min = damage->base_dd_max = value_spell->effectN( 1 ).average( e );

      auto ground_aoe = create_proc_action<generic_aoe_proc_t>( "garbagemancers_last_resort_ground", e, 1219314 );

      // TODO: probably better ground targeting emulation for the damage, unlikely targets in a place will still be
      // there after 10s
      params.pulse_time( ground_aoe->data().duration() )
          .duration( timespan_t::from_seconds( value_spell->effectN( 2 ).base_value() ) )
          .action( ground_aoe )
          .expiration_callback( [ damage ]( const action_state_t* ) { damage->execute(); } );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      params.start_time( timespan_t::min() ).target( target );  // reset start time
      make_event<ground_aoe_event_t>( *sim, player, params );
    }
  };

  auto use = create_proc_action<garbagemancers_last_resort_t>( "garbagemancers_last_resort", effect,
                                                               "garbagemancers_last_resort", effect.driver() );

  // Setting a random cooldown for now, its got none in data leading to sims spamming the hell out of it and getting
  // stuck. Also has no cooldown in game...
  // TODO - Remove once theres a cooldown in data.
  effect.cooldown_      = 60_s;
  effect.execute_action = use;
}

// Funhouse Lens
// 1213432 Driver
// 1214603 Value Spell
// 1213433 Crit Buff
// 1213434 Haste Buff
void funhouse_lens( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct funhouse_lens_t : public spell_t
  {
    buff_t* crit_buff;
    buff_t* haste_buff;
    funhouse_lens_t( const special_effect_t& e, std::string_view n, const spell_data_t* s )
      : spell_t( n, e.player, s ), crit_buff( nullptr ), haste_buff( nullptr )
    {
      crit_buff = create_buff<stat_buff_t>( e.player, "funhouse_lens_crit", e.player->find_spell( 1213433 ) )
                      ->add_stat_from_effect_type( A_MOD_RATING, s->effectN( 1 ).average( e ) )
                      ->set_name_reporting( "Crit" );

      haste_buff = create_buff<stat_buff_t>( e.player, "funhouse_lens_haste", e.player->find_spell( 1213434 ) )
                       ->add_stat_from_effect_type( A_MOD_RATING, s->effectN( 2 ).average( e ) )
                       ->set_name_reporting( "Haste" );
    }

    void execute() override
    {
      spell_t::execute();
      bool crit = rng().roll( 0.5 );
      switch ( crit )
      {
        case true:
          crit_buff->trigger();
          break;
        case false:
          haste_buff->trigger();
          break;
      }
    }
  };

  effect.execute_action = create_proc_action<funhouse_lens_t>( "funhouse_lens", effect, "funhouse_lens",
                                                               effect.player->find_spell( 1214603 ) );
}

// Suspicious Energy Drink
// 1216625 Driver
// 1216650 Buff
void suspicious_energy_drink( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct suspicious_energy_drink_buff_t : public stat_buff_t
  {
    double bonus_value;
    double hp_limit;
    double base_buff_value;
    suspicious_energy_drink_buff_t( player_t* p, util::string_view n, const special_effect_t& e, const spell_data_t* s )
      : stat_buff_t( e.player, n, s ), bonus_value( 0 ), hp_limit( 0 )
    {
      base_buff_value = e.driver()->effectN( 1 ).average( e );
      set_stat_from_effect_type( A_MOD_RATING, base_buff_value );
      bonus_value = e.driver()->effectN( 2 ).average( e );
      hp_limit    = e.driver()->effectN( 3 ).base_value();
    }

    void start( int s, double, timespan_t d ) override
    {
      double value = base_buff_value;
      if ( player->health_percentage() < hp_limit )
      {
        value += bonus_value;
      }

      stat_buff_t::start( s, value, d );
    }
  };

  auto buff = create_buff<suspicious_energy_drink_buff_t>( effect.player, "suspicious_energy_drink", effect,
                                                           effect.player->find_spell( 1216650 ) );

  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// Mug's Moxie Jug
// 471548 Driver
// 474376 Hidden Buff Driver
// 474285 Buff
void mugs_moxie_jug( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto crit_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 474285 ) )
                       ->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect ) )
                       ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  auto buff_driver      = new special_effect_t( effect.player );
  buff_driver->name_str = util::tokenize_fn( effect.driver()->effectN( 1 ).trigger()->name_cstr() );
  buff_driver->spell_id = effect.driver()->effectN( 1 ).trigger_spell_id();
  buff_driver->proc_flags2_ = PF2_ALL_HIT;
  buff_driver->custom_buff  = crit_buff;
  effect.player->special_effects.push_back( buff_driver );

  auto second_proc = new dbc_proc_callback_t( effect.player, *buff_driver );
  second_proc->activate_with_buff( crit_buff, true );
  
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.custom_buff   = crit_buff;
  auto main_proc = new dbc_proc_callback_t( effect.player, effect );
}
// Amorphous Relic
// 472120 Driver
// 472195 Periodic Trigger
// 472184 Haste Buff
// 472185 Primary Buff
void amorphous_relic( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct amorphous_relic_event_t : public event_t
  {
    player_t* player;
    buff_t* haste_buff;
    buff_t* crit_buff;
    timespan_t period;
    event_t* next_event;

    amorphous_relic_event_t( player_t* p, timespan_t time_to_execute, buff_t* haste, buff_t* crit )
      : event_t( *p, time_to_execute ),
        player( p ),
        haste_buff( haste ),
        crit_buff( crit ),
        period( 0_ms ),
        next_event( nullptr )
    {
      period = player->find_spell( 472195 )->effectN( 1 ).period();
    }

    const char* name() const override
    {
      return "amorphous_relic_event";
    }

    void execute() override
    {
      if ( next_event == nullptr )
        next_event = make_event<amorphous_relic_event_t>( sim(), player, period, haste_buff, crit_buff );
      else
      {
        // Reset the timer if combat was started again and the event triggered before the next secheduled time.
        event_t::cancel( next_event );
        next_event = make_event<amorphous_relic_event_t>( sim(), player, period, haste_buff, crit_buff );
      }

      if ( player->in_combat )
      {
        bool haste = player->rng().roll( 0.5 );
        switch ( haste )
        {
          case true:
            haste_buff->trigger();
            break;
          case false:
            crit_buff->trigger();
            break;
        }
      }
    }
  };

  auto haste_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 472184 ) )
                        ->add_stat_from_effect( 2, effect.driver()->effectN( 4 ).average( effect ) )
                        ->add_stat_from_effect( 3, effect.driver()->effectN( 3 ).average( effect ) );

  auto crit_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 472185 ) )
                       ->add_stat_from_effect( 2, effect.driver()->effectN( 1 ).average( effect ) )
                       ->add_stat_from_effect( 3, effect.driver()->effectN( 2 ).average( effect ) );

  effect.player->register_on_combat_state_callback( [ haste_buff, crit_buff ]( player_t* p, bool c ) {
    if ( c )
    {
      make_event<amorphous_relic_event_t>( *p->sim, p, 0_ms, haste_buff, crit_buff );
    }
  } );
}

// Zee's Thug Hotline
// 1217356 Driver
// Pocket Ace:
// 1217431 Summon
// 1217675 Gutstab - Only uses in ST
// 1217676 Fan of Stabs - Only uses in AoE
// Uses a basic auto attack as well
// Snake Eyes:
// 1217432 Summon
// 1217719 Snipe - Only Ability
// Does not auto attack
// Thwack Jack:
// 1217427 Summon
// 1217638 Thwack! - Only uses in ST
// 1217665 Thwack Thwack Thwack! - Only uses in AoE
// Uses a basic auto attack as well
void zees_thug_hotline( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct auto_attack_melee_t : public melee_attack_t
  {
    auto_attack_melee_t( pet_t* p, std::string_view name = "main_hand", action_t* a = nullptr,
                         const special_effect_t* e = nullptr )
      : melee_attack_t( name, p )
    {
      this->background = this->repeating = true;
      this->not_a_proc = this->may_crit = true;
      this->special                     = false;
      this->weapon_multiplier           = 1.0;
      this->trigger_gcd                 = 0_ms;
      this->school                      = SCHOOL_PHYSICAL;
      this->stats->school               = SCHOOL_PHYSICAL;
      this->base_multiplier = p->main_hand_weapon.swing_time.total_seconds();
      this->base_dd_min = this->base_dd_max = e->driver()->effectN( 5 ).average( *e ) * 0.375;

      auto proxy = a;
      auto it    = range::find( proxy->child_action, name, &action_t::name );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }

    // Pet melee attacks seem to still scale with aura 380 and 531
    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = melee_attack_t::composite_da_multiplier( s );
      m *= this->player->cast_pet()->owner->composite_player_pet_damage_multiplier( s, type == PLAYER_GUARDIAN );
      return m;
    }

    double composite_target_multiplier( player_t* p ) const override
    {
      double m = melee_attack_t::composite_target_multiplier( p );
      m *= this->player->cast_pet()->owner->composite_player_target_pet_damage_multiplier( p, type == PLAYER_GUARDIAN );
      return m;
    }

    void execute() override
    {
      if ( this->player->executing )
        this->schedule_execute();
      else
        melee_attack_t::execute();
    }
  };

  struct zees_thug_hotline_pet_spell_t : public spell_t
  {
    zees_thug_hotline_pet_spell_t( std::string_view n, pet_t* p, const spell_data_t* s, std::string_view options_str,
                                   action_t* a )
      : spell_t( n, p, s )
    {
      auto proxy = a;
      auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );

      parse_options( options_str );

      // Not 100% confident this is correct. Just what it appeared to be at a glance.
      cooldown->duration = 4_s;
      cooldown->hasted = true;
    }
  };

  struct gutstab_t : public zees_thug_hotline_pet_spell_t
  {
    gutstab_t( const special_effect_t& e, unique_gear_pet_t* p, std::string_view options_str, action_t* a )
      : zees_thug_hotline_pet_spell_t( "gutstab", p, p->find_spell( 1217675 ), options_str, a )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e );
    }
  };

  struct fan_of_stabs_t : public zees_thug_hotline_pet_spell_t
  {
    fan_of_stabs_t( const special_effect_t& e, unique_gear_pet_t* p, std::string_view options_str, action_t* a )
      : zees_thug_hotline_pet_spell_t( "fan_of_stabs", p, p->find_spell( 1217676 ), options_str, a )
    {
      aoe         = -1;
      base_dd_min = base_dd_max = e.driver()->effectN( 4 ).average( e );
    }
  };

  struct snipe_t : public zees_thug_hotline_pet_spell_t
  {
    snipe_t( const special_effect_t& e, unique_gear_pet_t* p, std::string_view options_str, action_t* a )
      : zees_thug_hotline_pet_spell_t( "snipe", p, p->find_spell( 1217719 ), options_str, a )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 3 ).average( e );
    }
  };

  struct thwack_t : public zees_thug_hotline_pet_spell_t
  {
    thwack_t( const special_effect_t& e, unique_gear_pet_t* p, std::string_view options_str, action_t* a )
      : zees_thug_hotline_pet_spell_t( "thwack", p, p->find_spell( 1217638 ), options_str, a )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );
    }
  };

  struct thwack_thwack_thwack_t : public zees_thug_hotline_pet_spell_t
  {
    thwack_thwack_thwack_t( const special_effect_t& e, unique_gear_pet_t* p, std::string_view options_str,
                            action_t* a )
      : zees_thug_hotline_pet_spell_t( "thwack_thwack_thwack", p, p->find_spell( 1217665 ), options_str, a )
    {
      aoe         = -1;
      base_dd_min = base_dd_max = e.driver()->effectN( 4 ).average( e );
    }
  };

  struct pocket_ace_pet_t : public unique_gear_pet_t
  {
    pocket_ace_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : base_t( "pocket_ace", e, e.player->find_spell( 1217431 ) )
    {
      parent_action               = parent;
      use_auto_attack             = true;
      main_hand_weapon.type       = WEAPON_BEAST;
      main_hand_weapon.swing_time = 1.6_s;
    }

    attack_t* create_auto_attack() override
    {
      auto a                = new auto_attack_melee_t( this, "main_hand_pocket_ace", parent_action, &effect );
      a->name_str_reporting = "Melee";
      return a;
    }

    action_t* create_action( std::string_view name, std::string_view options_str ) override
    {
      if ( name == "gutstab" )
        return new gutstab_t( effect, this, options_str, parent_action );
      if ( name == "fan_of_stabs" )
        return new fan_of_stabs_t( effect, this, options_str, parent_action );

      return base_t::create_action( name, options_str );
    }

    void init_action_list() override
    {
      base_t::init_action_list();
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "gutstab,if=active_enemies=1" );
      def->add_action( "fan_of_stabs,if=active_enemies>1" );
    }
  };

  struct snake_eyes_pet_t : public unique_gear_pet_t
  {
    snake_eyes_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : base_t( "snake_eyes", e, e.player->find_spell( 1217432 ) )
    {
      parent_action = parent;
    }

    action_t* create_action( std::string_view name, std::string_view options_str ) override
    {
      if ( name == "snipe" )
        return new snipe_t( effect, this, options_str, parent_action );

      return base_t::create_action( name, options_str );
    }

    void init_action_list() override
    {
      base_t::init_action_list();
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "snipe" );
    }
  };

  struct thwack_jack_pet_t : public unique_gear_pet_t
  {
    thwack_jack_pet_t( const special_effect_t& e, action_t* parent = nullptr )
      : base_t( "thwack_jack", e, e.player->find_spell( 1217427 ) )
    {
      parent_action               = parent;
      use_auto_attack             = true;
      main_hand_weapon.type       = WEAPON_BEAST;
      main_hand_weapon.swing_time = 2.6_s;
    }

    attack_t* create_auto_attack() override
    {
      auto a                = new auto_attack_melee_t( this, "main_hand_thwack_jack", parent_action, &effect );
      a->name_str_reporting = "Melee";
      return a;
    }

    action_t* create_action( std::string_view name, std::string_view options_str ) override
    {
      if ( name == "thwack" )
        return new thwack_t( effect, this, options_str, parent_action );
      if ( name == "thwack_thwack_thwack" )
        return new thwack_thwack_thwack_t( effect, this, options_str, parent_action );

      return base_t::create_action( name, options_str );
    }

    void init_action_list() override
    {
      base_t::init_action_list();
      action_priority_list_t* def = get_action_priority_list( "default" );
      def->add_action( "thwack,if=active_enemies=1" );
      def->add_action( "thwack_thwack_thwack,if=active_enemies>=2" );
    }
  };

  struct zees_thug_hotline_t : public generic_proc_t
  {
    spawner::pet_spawner_t<pocket_ace_pet_t> pocket_ace_spawner;
    spawner::pet_spawner_t<snake_eyes_pet_t> snake_eyes_spawner;
    spawner::pet_spawner_t<thwack_jack_pet_t> thwack_jack_spawner;

    zees_thug_hotline_t( const special_effect_t& e )
      : generic_proc_t( e, "zees_thug_hotline", e.driver() ),
        pocket_ace_spawner( "pocket_ace", e.player ),
        snake_eyes_spawner( "snake_eyes", e.player ),
        thwack_jack_spawner( "thwack_jack", e.player )
    {
      auto pocket_ace_summon_spell = e.player->find_spell( 1217431 );
      auto pocket_ace = new action_t( action_e::ACTION_OTHER, "call_pocket_ace", e.player, pocket_ace_summon_spell );
      pocket_ace_spawner.set_creation_callback(
          [ &e, pocket_ace ]( player_t* ) { return new pocket_ace_pet_t( e, pocket_ace ); } );
      pocket_ace_spawner.set_default_duration( pocket_ace_summon_spell->duration() );
      pocket_ace_spawner.set_max_pets( 1 );
      pocket_ace_spawner.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
      add_child( pocket_ace );

      auto snake_eyes_summon_spell = e.player->find_spell( 1217432 );
      auto snake_eyes = new action_t( action_e::ACTION_OTHER, "call_snake_eyes", e.player, snake_eyes_summon_spell );
      snake_eyes_spawner.set_creation_callback(
          [ &e, snake_eyes ]( player_t* ) { return new snake_eyes_pet_t( e, snake_eyes ); } );
      snake_eyes_spawner.set_default_duration( snake_eyes_summon_spell->duration() );
      snake_eyes_spawner.set_max_pets( 1 );
      snake_eyes_spawner.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
      add_child( snake_eyes );

      auto thwack_jack_summon_spell = e.player->find_spell( 1217427 );
      auto thwack_jack = new action_t( action_e::ACTION_OTHER, "call_thwack_jack", e.player, thwack_jack_summon_spell );
      thwack_jack_spawner.set_creation_callback(
          [ &e, thwack_jack ]( player_t* ) { return new thwack_jack_pet_t( e, thwack_jack ); } );
      thwack_jack_spawner.set_default_duration( thwack_jack_summon_spell->duration() );
      thwack_jack_spawner.set_max_pets( 1 );
      thwack_jack_spawner.set_replacement_strategy( spawner::pet_replacement_strategy::REPLACE_OLDEST );
      add_child( thwack_jack );

      e.player->buffs.bloodlust->add_stack_change_callback( [ & ]( buff_t*, int, int new_ ) {
          if ( new_ )
          {
            pocket_ace_spawner.spawn();
            snake_eyes_spawner.spawn();
            thwack_jack_spawner.spawn();
          }
        } );
    }

    void execute() override
    {
      generic_proc_t::execute();
      int pet_id = rng().range( 0, 3 );
      switch ( pet_id )
      {
        case 0:
          pocket_ace_spawner.spawn();
          break;
        case 1:
          snake_eyes_spawner.spawn();
          break;
        case 2:
          thwack_jack_spawner.spawn();
          break;
        default:
          break;
      }
    }
  };

  effect.execute_action = create_proc_action<zees_thug_hotline_t>( "zees_thug_hotline", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Mister Lock-n-Stalk
// 467469 Equip Driver
// 467485 Use Driver
// 1215690 Precision Targeting damage
// 1215733 Mass Destruction damage
void mister_locknstalk( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct mister_locknstalk_cb_t : public dbc_proc_callback_t
  {
    action_t* st_damage;
    action_t* aoe_damage;
    action_t* proxy;
    enum mister_locknstalk_modes_t
    {
      MODE_DYNAMIC,
      MODE_SINGLE_TARGET,
      MODE_AOE
    };
    mister_locknstalk_modes_t mode;
    mister_locknstalk_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), st_damage( nullptr ), aoe_damage( nullptr ), proxy( nullptr ), mode( MODE_DYNAMIC )
    {
      proxy = new action_t( action_e::ACTION_OTHER, "mister_locknstalk", e.player, e.driver() );
      st_damage              = create_proc_action<generic_proc_t>( "precision_targeting", e, 1215690 );
      st_damage->base_dd_min = st_damage->base_dd_max = e.driver()->effectN( 1 ).average( e );
      // st_damage->base_multiplier                      = role_mult( e );
      proxy->add_child( st_damage );

      aoe_damage              = create_proc_action<generic_aoe_proc_t>( "mass_destruction", e, 1215733, true );
      aoe_damage->base_dd_min = aoe_damage->base_dd_max = e.driver()->effectN( 2 ).average( e );
      // aoe_damage->base_multiplier                       = role_mult( e );
      proxy->add_child( aoe_damage );

      const auto& option = e.player->thewarwithin_opts.mister_locknstalk_mode;
      if ( !option.is_default() )
      {
        if ( util::str_compare_ci( option, "dynamic" ) )
          mode = MODE_DYNAMIC;
        else if ( util::str_compare_ci( option, "single_target" ) )
          mode = MODE_SINGLE_TARGET;
        else if ( util::str_compare_ci( option, "aoe" ) )
          mode = MODE_AOE;
        else
          throw std::invalid_argument( "Valid thewarwithin.mister_locknstalk_mode: dynamic, single_target, aoe" );
      }
    }

    void execute( action_t*, action_state_t* s )
    {
      switch ( mode )
      {
        case MODE_DYNAMIC:
          if ( listener->sim->target_non_sleeping_list.size() > 1 )
            aoe_damage->execute_on_target( s->target );
          else
            st_damage->execute_on_target( s->target );
          break;
        case MODE_SINGLE_TARGET:
          st_damage->execute_on_target( s->target );
          break;
        case MODE_AOE:
          aoe_damage->execute_on_target( s->target );
          break;
        default:
          break;
      }

      proxy->stats->add_execute( 0_ms, listener );
    }
  };

  new mister_locknstalk_cb_t( effect );
}

// Weapons

// 443384 driver
// 443585 damage
// 443515 unknown buff
// 443519 debuff
// 443590 stat buff
// TODO: damage spell data is shadowfrost and not cosmic
// TODO: confirm buff/debuff is based on target type and not triggering spell type
// TODO: confirm cannot proc on pets
void fateweaved_needle( special_effect_t& effect )
{
  struct fateweaved_needle_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    const spell_data_t* buff_spell;
    const spell_data_t* debuff_spell;
    double stat_amt;

    fateweaved_needle_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
      buff_spell( e.player->find_spell( 443590 ) ),
      debuff_spell( e.player->find_spell( 443519 ) ),
      stat_amt( e.driver()->effectN( 2 ).average( e ) )
    {
      // TODO: damage spell data is shadowfrost and not cosmic
      damage = create_proc_action<generic_proc_t>( "fated_pain", e, 443585 );
      damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 1 ).average( e );
      damage->base_multiplier *= role_mult( e );
    }

    buff_t* create_debuff( player_t* t ) override
    {
      // TODO: confirm buff/debuff is based on target type and not triggering spell type
      if ( t->is_enemy() )
      {
        return make_buff( actor_pair_t( t, listener ), "thread_of_fate_debuff", debuff_spell )
          ->set_expire_callback( [ this ]( buff_t* b, int, timespan_t ) {
            damage->execute_on_target( b->player );
          } );
      }
      else
      {
        return make_buff<stat_buff_t>( actor_pair_t( t, listener ), "thread_of_fate_buff", buff_spell )
          ->set_stat_from_effect_type( A_MOD_STAT, stat_amt )
          ->set_name_reporting( "thread_of_fate" );
      }
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      // TODO: confirm cannot proc on pets
      if ( s->target->is_enemy() || ( s->target != listener && s->target->is_player() ) )
        return dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t*, action_state_t* s ) override
    {
      get_debuff( s->target )->trigger();
      // From logs whenever it triggers on an enemy/ally it also triggers on yourself
      get_debuff( listener )->trigger();
    }
  };

  new fateweaved_needle_cb_t( effect );
}

// 442205 driver
// 442268 dot
// 442267 on-next buff
// 442280 on-next damage
// TODO: confirm on-next buff/damage stacks per dot expired
// TODO: determine if on-next buff ICD affects how often it can be triggered
// TODO: determine if on-next buff ICD affects how often it can be consumed
void befoulers_syringe( special_effect_t& effect )
{
  struct befouled_blood_t : public generic_proc_t
  {
    buff_t* bloodlust;  // on-next buff

    befouled_blood_t( const special_effect_t& e, buff_t* b )
      : generic_proc_t( e, "befouled_blood", e.trigger() ), bloodlust( b )
    {
      base_td = e.driver()->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );
      dot_max_stack = 1;
      dot_behavior = dot_behavior_e::DOT_REFRESH_DURATION;
      target_debuff = e.trigger();
    }

    buff_t* create_debuff( player_t* t ) override
    {
      return generic_proc_t::create_debuff( t )
        ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
        // trigger on-next buff here as buffs are cleared before dots in demise()
        // TODO: confirm on-next buff/damage stacks per dot expired
        // TODO: determine if on-next buff ICD affects how often it can be triggered
        ->set_expire_callback( [ this ]( buff_t*, int stack, timespan_t remains ) {
          if ( remains > 0_ms && stack )
            bloodlust->trigger( stack );
        } );
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      auto ta = generic_proc_t::composite_target_multiplier( t );

      if ( auto debuff = find_debuff( t ) )
        ta *= debuff->check();

      return ta;
    }

    void trigger_dot( action_state_t* s ) override
    {
      generic_proc_t::trigger_dot( s );

      // get dot refresh duration, as it accounts for the ongoing tick
      auto duration = calculate_dot_refresh_duration( get_dot( s->target ), dot_duration );

      // execute() instead of trigger() to avoid proc delay
      get_debuff( s->target )->execute( 1, buff_t::DEFAULT_VALUE(), duration );
    }
  };

  // create on-next melee damage
  auto strike = create_proc_action<generic_proc_t>( "befouling_strike", effect, 442280 );
  strike->base_dd_min = strike->base_dd_max = effect.driver()->effectN( 2 ).average( effect );
  strike->base_multiplier *= role_mult( effect );

  // create on-next melee buff
  auto bloodlust = create_buff<buff_t>( effect.player, effect.player->find_spell( 442267 ) );

  auto on_next = new special_effect_t( effect.player );
  on_next->name_str = bloodlust->name();
  on_next->spell_id = bloodlust->data().id();
  effect.player->special_effects.push_back( on_next );

  // TODO: determine if on-next buff ICD affects how often it can be consumed
  effect.player->callbacks.register_callback_execute_function( on_next->spell_id,
      [ bloodlust, strike ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* s ) {
        strike->execute_on_target( s->target );
        bloodlust->expire( a );
      } );

  auto cb = new dbc_proc_callback_t( effect.player, *on_next );
  cb->activate_with_buff( bloodlust );

  effect.execute_action = create_proc_action<befouled_blood_t>( "befouled_blood", effect, bloodlust );

  new dbc_proc_callback_t( effect.player, effect );
}

// 455887 driver
//  e1: damage coeff
//  e2: stat coeff
// 455888 vfx?
// 455910 damage
// 456652 buff
void voltaic_stormcaller( special_effect_t& effect )
{
  struct voltaic_stormstrike_t : public generic_aoe_proc_t
  {
    stat_buff_with_multiplier_t* buff;

    voltaic_stormstrike_t( const special_effect_t& e ) : generic_aoe_proc_t( e, "voltaic_stormstrike", 455910 )
    {
      travel_delay = 0.95;  // not in spell data, estimated from logs

      split_aoe_damage = false;  // TODO: confirm this remains true in 11.1
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );

      buff = create_buff<stat_buff_with_multiplier_t>( e.player, e.player->find_spell( 456652 ) );
      buff->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 2 ).average( e ) );
    }

    void impact( action_state_t* s ) override
    {
      generic_aoe_proc_t::impact( s );

      buff->stat_mul = generic_aoe_proc_t::composite_aoe_multiplier( execute_state );
      // buff is delayed by ~250ms, not in spell data, estimated from logs
      make_event( *sim, 250_ms, [ this ] { buff->trigger(); } );
    }
  };

  effect.execute_action = create_proc_action<voltaic_stormstrike_t>( "voltaic_stormstrike", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// 455819 driver
// 455821 damage
void harvesters_interdiction( special_effect_t& effect )
{
  auto dot = create_proc_action<generic_proc_t>( "interdictive_injection", effect, 455821 );
  dot->base_td = effect.driver()->effectN( 1 ).average( effect );
  dot->base_multiplier *= role_mult( effect );

  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );
}

// 469936 driver
// 469937 crit buff
// 469938 haste buff
// 469941 mastery buff
// 469942 versatility buff
// TODO: confirm buff cycle doesn't reset during middle of dungeon
void guiding_stave_of_wisdom( special_effect_t& effect )
{
  struct guiding_stave_of_wisdom_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;

    guiding_stave_of_wisdom_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      for ( auto id : { 469937, 469938, 469941, 469942 } )
      {
        auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( id ) )
          ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect ) );

        buffs.push_back( buff );
      }
    }

    void reset() override
    {
      std::rotate( buffs.begin(), buffs.begin() + effect.player->rng().range( 4 ), buffs.end() );
    }

    void execute( action_t*, action_state_t* ) override
    {
      buffs.front()->trigger();
      std::rotate( buffs.begin(), buffs.begin() + 1, buffs.end() );
    }
  };

  new guiding_stave_of_wisdom_cb_t( effect );
}

// 470641 driver, trigger damage
// 470642 damage
// 470643 reflect, NYI
void flame_wrath( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "flame_wrath", effect, effect.trigger(), true );
  damage->base_dd_min = damage->base_dd_max =
    effect.driver()->effectN( 1 ).average( effect ) + effect.trigger()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;

  // TODO: damage reflect shield NYI
  new dbc_proc_callback_t( effect.player, effect );
}

// 470647 Driver
// 470648 Damage
void force_of_magma( special_effect_t& effect )
{
  auto damage         = create_proc_action<generic_proc_t>( "force_of_magma", effect, effect.trigger() );
  damage->base_dd_min = damage->base_dd_max =
      effect.driver()->effectN( 1 ).average( effect ) + effect.trigger()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );

  effect.execute_action = damage;
  new dbc_proc_callback_t( effect.player, effect );
}

// Vile Contamination
// 471316 Driver
// 473602 DoT
void vile_contamination( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto dot     = create_proc_action<generic_proc_t>( "vile_contamination", effect, effect.trigger() );
  dot->base_td = effect.driver()->effectN( 1 ).average( effect );
  // Setting a reasonably high non 0 duration so the DoT works as expected. Data contains no duration resulting in it
  // never applying.
  dot->dot_duration = 300_s;

  effect.execute_action = dot;
  new dbc_proc_callback_t( effect.player, effect );
}

// Best-in-Slots
// 473402 Use Driver & On Use Stat buff
// 471063 Equip Driver & Values
// 473492 Proc Stat Buff
void best_in_slots( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  auto equip_driver = effect.player->find_spell( 471063 );
  assert( equip_driver && "Best-in-Slots missing equip driver" );

  struct best_in_slots_stat_buff_t : stat_buff_t
  {
    double range_min;
    double range_max;

    best_in_slots_stat_buff_t( const special_effect_t& e, util::string_view name, const spell_data_t* s,
                               const item_t* item = nullptr )
      : stat_buff_t( e.player, name, s, item ), range_min( 0 ), range_max( 0 )
    {
      auto equip_driver = e.player->find_spell( 471063 );
      auto mod          = equip_driver->effectN( 2 ).percent();
      range_min         = 1 - mod;
      range_max         = 1 + mod;
    }

    double randomize_stat_value()
    {
      double val = default_value * rng().range( range_min, range_max );
      for ( auto& buff_stat : stats )
      {
        double delta            = val - buff_stat.current_value;
        buff_stat.current_value = val;
        buff_stat.amount        = val;
        if ( delta > 0 )
        {
          player->stat_gain( buff_stat.stat, delta, stat_gain, nullptr, buff_duration() > timespan_t::zero() );
        }
        else if ( delta < 0 )
        {
          player->stat_loss( buff_stat.stat, std::fabs( delta ), stat_gain, nullptr,
                             buff_duration() > timespan_t::zero() );
        }
      }
      return val;
    }

    void bump( int stacks, double ) override
    {
      buff_t::bump( stacks, randomize_stat_value() );
    }

    void expire_override( int s, timespan_t d ) override
    {
      for ( auto& buff_stat : stats )
      {
        player->stat_loss( buff_stat.stat, buff_stat.current_value, stat_gain, nullptr,
                           buff_duration() > timespan_t::zero() );

        buff_stat.current_value = 0;
      }

      // Purposely skip over stat_buff_t::expire_override() as we do the lost stat calculations manually
      buff_t::expire_override( s, d );
    }
  };

  struct best_in_slots_cb_t : public dbc_proc_callback_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;

    best_in_slots_cb_t( const special_effect_t& equip, const special_effect_t& use )
      : dbc_proc_callback_t( equip.player, equip ), buffs()
    {
      auto buff_value      = equip.driver()->effectN( 1 ).average( use );
      auto proc_buff_spell = equip.player->find_spell( 473492 );

      create_all_stat_buffs<best_in_slots_stat_buff_t>( equip, proc_buff_spell, buff_value,
                                                        [ &, buff_value ]( stat_e s, buff_t* b ) {
                                                          b->default_value = buff_value;
                                                          buffs[ s ]       = b;
                                                        } );
    }

    void execute( action_t*, action_state_t* ) override
    {
      auto buff = buffs.at( rng().range( secondary_ratings ) );
      if ( !buff->check() )
      {
        range::for_each( buffs, []( auto& b ) { b.second->expire(); } );
      }
      buff->trigger();
    }
  };

  auto equip      = new special_effect_t( effect.player );
  equip->name_str = equip_driver->name_cstr();
  equip->spell_id = equip_driver->id();
  effect.player->special_effects.push_back( equip );

  auto cb = new best_in_slots_cb_t( *equip, effect );
  cb->initialize();
  cb->activate();

  struct cheating_t : public spell_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;

    cheating_t( const special_effect_t& e, const spell_data_t* equip_driver )
      : spell_t( "cheating", e.player, e.driver() ), buffs()
    {
      auto value_mod  = 1 + equip_driver->effectN( 2 ).percent();
      auto buff_value = equip_driver->effectN( 1 ).average( e ) * value_mod;

      create_all_stat_buffs( e, e.driver(), buff_value, [ &, buff_value ]( stat_e s, buff_t* b ) {
        b->default_value = buff_value;
        buffs[ s ]       = b;
        b->cooldown->duration = 0_ms; // Handled by the action
      } );
    }

    void execute() override
    {
      spell_t::execute();
      buffs.at( util::highest_stat( player, secondary_ratings ) )->trigger();
    }
  };

  effect.disable_buff();
  effect.spell_id       = 473402;
  effect.execute_action = create_proc_action<cheating_t>( "cheating", effect, equip_driver );
}

// Machine Gob's Iron Grin
// 1218442 Driver
// 1218471 Big Damage
// 1218469 Medium Damage
// 1218463 Small Damage
void machine_gobs_iron_grin( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct machine_gobs_iron_grin_cb_t : public dbc_proc_callback_t
  {
    action_t* big_damage;
    action_t* medium_damage;
    action_t* small_damage;
    action_t* proxy;

    machine_gobs_iron_grin_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), big_damage( nullptr ), medium_damage( nullptr ), small_damage( nullptr ), proxy( nullptr )
    {
      proxy = new action_t( action_e::ACTION_OTHER, "machine_gobs_iron_grin", e.player, e.driver() );
      big_damage = create_proc_action<generic_aoe_proc_t>( "machine_gobs_bellowing_laugh", e, 1218471, true );
      big_damage->base_dd_min = big_damage->base_dd_max = e.driver()->effectN( 3 ).average( e );
      proxy->add_child( big_damage );

      medium_damage              = create_proc_action<generic_aoe_proc_t>( "machine_gobs_big_grin", e, 1218469, true );
      medium_damage->base_dd_min = medium_damage->base_dd_max = e.driver()->effectN( 2 ).average( e );
      proxy->add_child( medium_damage );

      small_damage              = create_proc_action<generic_aoe_proc_t>( "machine_gobs_hiccup", e, 1218463, true );
      small_damage->base_dd_min = small_damage->base_dd_max = e.driver()->effectN( 1 ).average( e );
      proxy->add_child( small_damage );
    }

    void execute( action_t*, action_state_t* s ) override
    {
      auto rand = rng().range( 0, 3 );
      switch ( rand )
      {
        case 0:
          big_damage->execute_on_target( s->target );
          break;
        case 1:
          medium_damage->execute_on_target( s->target );
          break;
        case 2:
          small_damage->execute_on_target( s->target );
          break;
      }
      proxy->stats->add_execute( 0_ms, listener );
    }
  };

  new machine_gobs_iron_grin_cb_t( effect );
}

// Armor
// 457815 driver
// 457918 nature damage driver
// 457925 counter
// 457928 dot
void seal_of_the_poisoned_pact( special_effect_t& effect )
{
  unsigned nature_id = 457918;
  auto nature = find_special_effect( effect.player, nature_id );
  assert( nature && "Seal of the Poisoned Pact missing nature damage driver" );

  auto dot = create_proc_action<generic_proc_t>( "venom_shock", effect, 457928 );
  dot->base_td = effect.driver()->effectN( 1 ).average( effect ) * dot->base_tick_time / dot->dot_duration;
  dot->base_multiplier *= role_mult( effect );

  auto counter = create_buff<buff_t>( effect.player, effect.player->find_spell( 457925 ) )
    ->set_expire_at_max_stack( true )
    ->set_expire_callback( [ dot ]( buff_t* b, int, timespan_t ) {
      dot->execute_on_target( b->player->target );
    } );

  effect.custom_buff = counter;

  new dbc_proc_callback_t( effect.player, effect );

  // TODO: confirm nature damage driver is entirely independent
  nature->name_str = nature->name() + "_nature";
  nature->custom_buff = counter;

  effect.player->callbacks.register_callback_trigger_function( nature_id,
    dbc_proc_callback_t::trigger_fn_type::CONDITION,
    []( const dbc_proc_callback_t*, action_t* a, const action_state_t* ) {
      return dbc::has_common_school( a->get_school(), SCHOOL_NATURE );
    } );

  new dbc_proc_callback_t( effect.player, *nature );
}

void imperfect_ascendancy_serum( special_effect_t& effect )
{
  struct ascension_channel_t : public proc_spell_t
  {
    buff_t* buff;
    action_t* use_action;  // if this exists, then we're prechanneling via the APL

    ascension_channel_t( const special_effect_t& e, buff_t* ascension )
      : proc_spell_t( "ascension_channel", e.player, e.driver(), e.item )
    {
      channeled = hasted_ticks = hasted_dot_duration = true;
      harmful                                        = false;
      dot_duration = base_tick_time = base_execute_time;
      base_execute_time             = 0_s;
      buff                          = ascension;
      effect                        = &e;
      interrupt_auto_attack         = false;

      for ( auto a : player->action_list )
      {
        if ( a->action_list && a->action_list->name_str == "precombat" && a->name_str == "use_item_" + item->name_str )
        {
          a->harmful = harmful;  // pass down harmful to allow action_t::init() precombat check bypass
          use_action = a;
          use_action->base_execute_time = base_tick_time;
          break;
        }
      }
    }

    void execute() override
    {
      if ( !player->in_combat )  // if precombat...
      {
        if ( use_action )  // ...and use_item exists in the precombat apl
        {
          precombat_buff();
        }
      }
      else
      {
        proc_spell_t::execute();
        event_t::cancel( player->readying );
        player->delay_ranged_auto_attacks( composite_dot_duration( execute_state ) );
      }
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;

      cooldown->adjust( d->duration() );

      auto cdgrp = player->get_cooldown( effect->cooldown_group_name() );
      cdgrp->adjust( d->duration() );

      proc_spell_t::last_tick( d );
      buff->trigger();

      if ( was_channeling && !player->readying )
        player->schedule_ready();
    }

    void precombat_buff()
    {
      timespan_t time = 0_ms;

      if ( time == 0_ms )  // No global override, check for an override from an APL variable
      {
        for ( auto v : player->variables )
        {
          if ( v->name_ == "imperfect_ascendancy_precombat_cast" )
          {
            time = timespan_t::from_seconds( v->value() );
            break;
          }
        }
      }

      // shared cd (other trinkets & on-use items)
      auto cdgrp = player->get_cooldown( effect->cooldown_group_name() );

      if ( time == 0_ms )  // No hardcoded override, so dynamically calculate timing via the precombat APL
      {
        time            = data().cast_time();
        const auto& apl = player->precombat_action_list;

        auto it = range::find( apl, use_action );
        if ( it == apl.end() )
        {
          sim->print_debug(
              "WARNING: Precombat /use_item for Imperfect Ascendancy Serum exists but not found in precombat APL!" );
          return;
        }

        cdgrp->start( 1_ms );  // tap the shared group cd so we can get accurate action_ready() checks

        // add cast time or gcd for any following precombat action
        std::for_each( it + 1, apl.end(), [ &time, this ]( action_t* a ) {
          if ( a->action_ready() )
          {
            timespan_t delta =
                std::max( std::max( a->base_execute_time.value(), a->trigger_gcd ) * a->composite_haste(), a->min_gcd );
            sim->print_debug( "PRECOMBAT: Imperfect Ascendancy Serum precast timing pushed by {} for {}", delta, a->name() );
            time += delta;

            return a->harmful;  // stop processing after first valid harmful spell
          }
          return false;
        } );
      }
      else if ( time < base_tick_time )  // If APL variable can't set to less than cast time
      {
        time = base_tick_time;
      }

      // how long you cast for
      auto cast = base_tick_time;
      // total duration of the buff
      auto total = buff->buff_duration();
      // actual duration of the buff you'll get in combat
      auto actual = total + cast - time;
      // cooldown on effect/trinket at start of combat
      auto cd_dur = cooldown->duration + cast - time;
      // shared cooldown at start of combat
      auto cdgrp_dur = std::max( 0_ms, effect->cooldown_group_duration() + cast - time );

      sim->print_debug( "PRECOMBAT: Imperfect Ascendency Serum started {}s before combat via {}, {}s in-combat buff", time,
                        use_action ? "APL" : "TWW_OPT", actual );

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, actual );

      if ( use_action )  // from the apl, so cooldowns will be started by use_item_t. adjust. we are still in precombat.
      {
        make_event( *sim, [ this, cast, time, cdgrp ] {  // make an event so we adjust after cooldowns are started
          cooldown->adjust( cast - time );

          if ( use_action )
            use_action->cooldown->adjust( cast - time );

          cdgrp->adjust( cast - time );
        } );
      }
      else  // via bfa. option override, start cooldowns. we are in-combat.
      {
        cooldown->start( cd_dur );

        if ( use_action )
          use_action->cooldown->start( cd_dur );

        if ( cdgrp_dur > 0_ms )
          cdgrp->start( cdgrp_dur );
      }
    }
  };

  auto buff_spell = effect.driver();
  buff_t* buff    = create_buff<stat_buff_t>( effect.player, buff_spell )
                     ->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect ) )
                     ->add_stat_from_effect( 2, effect.driver()->effectN( 2 ).average( effect ) )
                     ->add_stat_from_effect( 4, effect.driver()->effectN( 4 ).average( effect ) )
                     ->add_stat_from_effect( 5, effect.driver()->effectN( 5 ).average( effect ) )
                     ->set_cooldown( 0_ms );

  auto action           = new ascension_channel_t( effect, buff );
  effect.execute_action = action;
  effect.disable_buff();
}

// 455799 Driver
// 455820 First Dig
// 455826 Second Dig
// 455827 Third Dig
void excavation( special_effect_t& effect )
{
  // Currently does nothing if multiple of these are equipped at the same time
  if ( buff_t::find( effect.player, "Uncommon Treasure" ) )
    return;

  struct excavation_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buff_list;
    unsigned dig_count;

    excavation_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), buff_list(), dig_count( 0 )
    {
      auto uncommon = create_buff<stat_buff_t>( e.player, e.player->find_spell( 455820 ) )
                          ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) * 0.9 );

      buff_list.push_back( uncommon );

      auto rare = create_buff<stat_buff_t>( e.player, e.player->find_spell( 455826 ) )
                      ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) );

      buff_list.push_back( rare );

      auto epic = create_buff<stat_buff_t>( e.player, e.player->find_spell( 455827 ) )
                      ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e ) * 1.1 );

      buff_list.push_back( epic );
    }

    void execute( action_t*, action_state_t* ) override
    {
      buff_list[ dig_count ]->trigger();
      if ( dig_count < buff_list.size() - 1 )
        dig_count++;
      else
        dig_count = 0;
    }
  };

  new excavation_cb_t( effect );
}

void sureki_zealots_insignia( special_effect_t& e )
{
  auto buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 457684 ) )
                  ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 1 ).average( e ) );

  e.custom_buff = buff;

  // The Drivers flags are for Healing Taken. Swap flags to damage flags to at least get personal valuation.
  e.proc_flags_  = PF_ALL_DAMAGE | PF_PERIODIC;
  e.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;

  e.rppm_modifier_ = e.player->thewarwithin_opts.sureki_zealots_insignia_rppm_multiplier;

  new dbc_proc_callback_t( e.player, e );
}

// The Jastors Diamond
// 1214161 Value Spell
// 1214822 Driver
// 1214823 Self Buff
// 1214826 Ally Buff
void the_jastor_diamond( special_effect_t& effect )
{
  if ( !effect.player->is_ptr() )
    return;

  struct jastor_diamond_buff_base_t : public stat_buff_t
  {
    jastor_diamond_buff_base_t( player_t* p, std::string_view n, const spell_data_t* s ) : stat_buff_t( p, n, s )
    {
      set_default_value( 0 );
      add_stat_from_effect( 1, 0 );
      add_stat_from_effect( 2, 0 );
      add_stat_from_effect( 3, 0 );
      add_stat_from_effect( 4, 0 );
    }

    virtual void add_ally_stat( stat_e s, double val )
    {
      auto& buff_stat = get_stat( s );
      buff_stat.current_value += val;
      buff_stat.amount = buff_stat.current_value;
      // Only handle stat gain here, stat loss will be handled in i_did_that_buff_t::expire_override()
      player->stat_gain( buff_stat.stat, val, stat_gain, nullptr, buff_duration() > timespan_t::zero() );
    }

    void expire_override( int s, timespan_t d ) override
    {
      for ( auto& buff_stat : stats )
      {
        player->stat_loss( buff_stat.stat, buff_stat.current_value, stat_gain, nullptr,
                           buff_duration() > timespan_t::zero() );

        buff_stat.current_value = 0;
      }

      // Purposely skip over stat_buff_t::expire_override() as we do the lost stat calculations manually
      buff_t::expire_override( s, d );
    }

    buff_stat_t& get_stat( stat_e s )
    {
      for ( auto& buff_stat : stats )
      {
        if ( buff_stat.stat == s )
          return buff_stat;
      }
      // Fallback if the above fails for any reason.
      return stats[ 0 ];
    }

    void bump( int stacks, double /* value */ ) override
    {
      // Purposely skip over stat_buff_t::bump() as we do the added stat calculations manually
      buff_t::bump( stacks );
    }

    double buff_stat_stack_amount( const buff_stat_t&, int ) const override
    {
      return 0;
    }
  };

  struct i_did_that_buff_t : public jastor_diamond_buff_base_t
  {
    int fake_stacks;
    int max_fake_stacks;

    i_did_that_buff_t( player_t* p, std::string_view n, const spell_data_t* s )
      : jastor_diamond_buff_base_t( p, n, s ), fake_stacks( 0 ), max_fake_stacks( 0 )
    {
      max_fake_stacks = as<int>( p->find_spell( 1214161 )->effectN( 2 ).base_value() );
      set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
    }

    void add_ally_stat( stat_e s, double val ) override
    {
      if ( fake_stacks++ < max_fake_stacks )
      {
        jastor_diamond_buff_base_t::add_ally_stat( s, val );
      }
    }

    double get_current_stat_value( stat_e s )
    {
      auto stat = get_stat( s );
      return stat.current_value;
    }

    void reset() override
    {
      jastor_diamond_buff_base_t::reset();
      fake_stacks = 0;
    }

    void expire_override( int s, timespan_t d ) override
    {
      fake_stacks = 0;
      jastor_diamond_buff_base_t::expire_override( s, d );
    }
  };

  struct no_i_did_that_buff_t : public jastor_diamond_buff_base_t
  {
    no_i_did_that_buff_t( player_t* p, std::string_view n, const spell_data_t* s )
      : jastor_diamond_buff_base_t( p, n, s )
    {
    }
  };

  struct the_jastor_diamond_cb_t : public dbc_proc_callback_t
  {
    i_did_that_buff_t* self_buff;
    const spell_data_t* value_spell;
    const spell_data_t* ally_buff_spell;
    double buff_value;
    std::unordered_map<player_t*, no_i_did_that_buff_t*> ally_buffs;
    vector_with_callback<player_t*> ally_list;

    the_jastor_diamond_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        self_buff( nullptr ),
        value_spell( e.player->find_spell( 1214161 ) ),
        ally_buff_spell( e.player->find_spell( 1214826 ) ),
        buff_value( 0 ),
        ally_buffs(),
        ally_list()
    {
      assert( value_spell && "The Jastor Diamond missing value spell." );
      buff_value = value_spell->effectN( 1 ).average( e );

      self_buff = create_buff<i_did_that_buff_t>( e.player, "i_did_that", e.player->find_spell( 1214823 ) );

      e.player->register_precombat_begin( [ & ]( player_t* p ) {
        if ( p->sim->player_no_pet_list.size() > 1 && !p->sim->single_actor_batch )
        {
          ally_list = p->sim->player_no_pet_list;
          ally_list.find_and_erase( p );
          for ( auto& ally : ally_list )
          {
            auto ally_buff = create_buff<no_i_did_that_buff_t>( ally, "no_i_did_that", ally_buff_spell );
            ally_buffs.insert( { { ally }, { ally_buff } } );
          }
        }
      } );
    }

    player_t* pick_random_target()
    {
      std::vector<player_t*> eligible;
      for ( auto p : ally_list )
        if ( !p->is_sleeping() ) eligible.push_back( p );
      assert( !eligible.empty() );
      return rng().range( eligible );
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( ally_list.size() > 0 )
      {
        player_t* random_target = pick_random_target();
        if ( self_buff->check() && rng().roll( value_spell->effectN( 3 ).percent() ) )
        {
          no_i_did_that_buff_t* buff = ally_buffs.at( random_target );
          for ( auto stat : secondary_ratings )
          {
            double val = self_buff->get_current_stat_value( stat );
            buff->add_ally_stat( stat, val );
          }
          buff->trigger();
          self_buff->expire();
        }
        else
        {
          auto stat = util::highest_stat( random_target, secondary_ratings );
          self_buff->add_ally_stat( stat, buff_value );
          self_buff->trigger();
        }
      }
      else
      {
        if ( self_buff->check() && rng().roll( value_spell->effectN( 3 ).percent() ) )
        {
          self_buff->expire();
        }
        else
        {
          auto stat_roll = rng().range( secondary_ratings );
          self_buff->add_ally_stat( stat_roll, buff_value );
          self_buff->trigger();
        }
      }
    }
  };

  effect.cooldown_ = 12.5_s; // Oddly got removed from spell data? manually overriding for now
  effect.spell_id = 1214822;
  new the_jastor_diamond_cb_t( effect );
}

}  // namespace items

namespace sets
{
// 444067 driver
// 448643 unknown (0.2s duration, 6yd radius, outgoing aoe hit?)
// 448621 unknown (0.125s duration)
// 448669 damage
void void_reapers_contract( special_effect_t& effect )
{
  struct void_reapers_contract_cb_t : public dbc_proc_callback_t
  {
    action_t* major;
    action_t* minor;
    action_t* queensbane = nullptr;
    double hp_pct;

    void_reapers_contract_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), hp_pct( e.driver()->effectN( 2 ).base_value() )
    {
      auto damage_spell = effect.player->find_spell( 448669 );
      auto damage_name = std::string( damage_spell->name_cstr() );
      auto damage_amount = effect.driver()->effectN( 1 ).average( effect );

      major = create_proc_action<generic_aoe_proc_t>( damage_name, effect, damage_spell );
      major->base_dd_min = major->base_dd_max = damage_amount;
      major->split_aoe_damage = false;
      major->base_multiplier *= role_mult( effect );
      major->base_aoe_multiplier = effect.driver()->effectN( 5 ).percent();

      minor = create_proc_action<generic_aoe_proc_t>( damage_name + "_echo", effect, damage_spell );
      minor->base_dd_min = minor->base_dd_max = damage_amount * effect.driver()->effectN( 3 ).percent();
      minor->split_aoe_damage = false;
      minor->base_multiplier *= role_mult( effect );
      minor->base_aoe_multiplier = effect.driver()->effectN( 5 ).percent();
      minor->name_str_reporting = "Echo";
      major->add_child( minor );
    }

    void initialize() override
    {
      dbc_proc_callback_t::initialize();

      if ( listener->sets->has_set_bonus( listener->specialization(), TWW_KCI, B2 ) )
        if ( auto claw = find_special_effect( listener, 444135 ) )
          queensbane = claw->execute_action;
    }

    void execute( action_t*, action_state_t* s ) override
    {
      major->execute_on_target( s->target );

      bool echo = false;

      if ( queensbane )
      {
        auto dot = queensbane->find_dot( s->target );
        if ( dot && dot->is_ticking() )
          echo = true;
      }

      if ( !echo && s->target->health_percentage() < hp_pct )
        echo = true;

      if ( echo )
      {
        // each echo delayed by 300ms
        make_repeating_event( *listener->sim, 300_ms, [ this, s ] {
          if ( !s->target->is_sleeping() )
            minor->execute_on_target( s->target );
        }, 2 );
      }
    }
  };

  new void_reapers_contract_cb_t( effect );
}

// 444135 driver
// 448862 dot (trigger)
void void_reapers_warp_blade( special_effect_t& effect )
{
  auto dot_data = effect.trigger();
  auto dot = create_proc_action<generic_proc_t>( "queensbane", effect, dot_data );
  dot->base_td =
    effect.driver()->effectN( 1 ).average( effect ) * dot_data->effectN( 1 ).period() / dot_data->duration();
  dot->base_multiplier *= role_mult( effect );

  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );
}

// Woven Dusk
// 457655 Driver
// 457630 Buff
void woven_dusk( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "woven_dusk" } ) )
    return;

  // Need to force player sccaling for equipment set
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 457630 ) )
                  ->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 2 ).average( effect ) );

  // Something *VERY* weird is going on here. It appears to work properly for Discipline Priest, but not Shadow.
  if ( effect.player->specialization() == PRIEST_DISCIPLINE )
  {
    buff->set_chance( 1.0 )->set_rppm( RPPM_DISABLE );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Woven Dawn
// 455521 Driver
// 457627 Buff
void woven_dawn( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "woven_dawn" } ) )
    return;

  // Need to force player sccaling for equipment set
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 457627 ) )
                  ->add_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 2 ).average( effect ) );

  buff->set_chance( 1.0 )->set_rppm( RPPM_DISABLE );

  effect.custom_buff = buff;

  // Driver procs off Druid hostile abilities (Probably) as well as Priest hostile abilities (confirmed) as they always historically do
  if ( effect.player->type == player_e::DRUID || effect.player->type == player_e::PRIEST )
    effect.proc_flags_ |= PF_MAGIC_SPELL | PF_MELEE_ABILITY;

  new dbc_proc_callback_t( effect.player, effect );
}


// Embrace of the Cinderbee
// 443764 Driver
// 451698 Orb Available Buff
// 451699 Player Stat Buff
// 451980 Ally stat buff ( NYI )
struct pickup_cinderbee_orb_t : public action_t
{
  buff_t* orb = nullptr;

  pickup_cinderbee_orb_t( player_t* p, std::string_view opt )
    : action_t( ACTION_OTHER, "pickup_cinderbee_orb", p, spell_data_t::nil() )
  {
    parse_options( opt );

    s_data_reporting   = p->find_spell( 451698 );
    name_str_reporting = "Cinderbee Orb Collected";

    callbacks = harmful = false;
    trigger_gcd         = 0_ms;
  }

  bool ready() override
  {
    return orb->check();
  }

  void execute() override
  {
    if ( !rng().roll( player->thewarwithin_opts.embrace_of_the_cinderbee_miss_chance ) )
    {
      orb->decrement();
    }
  }
};

void embrace_of_the_cinderbee( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "embrace_of_the_cinderbee_orb" } ) )
    return;

  struct embrace_of_the_cinderbee_t : public dbc_proc_callback_t
  {
    buff_t* orb;
    std::unordered_map<stat_e, buff_t*> buffs;
    std::vector<action_t*> apl_actions;
    player_t* player;

    embrace_of_the_cinderbee_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), player( e.player )
    {
      auto value = e.player->find_spell( 451699 )->effectN( 5 ).average( e );
      create_all_stat_buffs( e, e.player->find_spell( 451699 ), value,
                             [ this ]( stat_e s, buff_t* b ) { buffs[ s ] = b; } );

      orb = create_buff<buff_t>( e.player, "embrace_of_the_cinderbee_orb", e.player->find_spell( 451698 ) )
                ->set_max_stack( 10 )
                ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                ->set_stack_change_callback( [ & ]( buff_t*, int old_, int new_ ) {
                  if ( old_ > new_ )
                  {
                    buffs.at( util::highest_stat( e.player, secondary_ratings ) )->trigger();
                  }
                } );

      for ( auto& a : e.player->action_list )
      {
        if ( a->name_str == "pickup_cinderbee_orb" )
        {
          apl_actions.push_back( a );
        }
      }

      if ( apl_actions.size() > 0 )
      {
        for ( auto& a : apl_actions )
        {
          debug_cast<pickup_cinderbee_orb_t*>( a )->orb = orb;
        }
      }
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( apl_actions.size() > 0 )
      {
        orb->trigger();
      }
      else
      {
        timespan_t timing  = player->thewarwithin_opts.embrace_of_the_cinderbee_timing;
        double miss_chance = player->thewarwithin_opts.embrace_of_the_cinderbee_miss_chance;
        make_event( *player->sim, timing > 0_ms ? timing : rng().range( 100_ms, orb->data().duration() ),
                    [ this, miss_chance ] {
                      if ( !rng().roll( miss_chance ) )
                      {
                        buffs.at( util::highest_stat( player, secondary_ratings ) )->trigger();
                      }
                    } );
      }
    }
  };

  new embrace_of_the_cinderbee_t( effect );
}

// 443773 driver
// 449440 buff
// 449441 orb
void fury_of_the_stormrook( special_effect_t& effect )
{
  struct fury_of_the_stormrook_cb_t : public dbc_proc_callback_t
  {
    rng::truncated_gauss_t pickup;
    buff_t* buff;

    fury_of_the_stormrook_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        pickup( e.player->thewarwithin_opts.fury_of_the_stormrook_pickup_delay,
                e.player->thewarwithin_opts.fury_of_the_stormrook_pickup_stddev )
    {
      buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 449440 ) );
    }

    void execute( action_t*, action_state_t* ) override
    {
      make_event( *listener->sim, rng().gauss( pickup ), [ this ] { buff->trigger(); } );
    }
  };

  new fury_of_the_stormrook_cb_t( effect );
}
}  // namespace sets

namespace singing_citrines
{
enum singing_citrines_drivers_e
{
  CITRINE_DRIVER_NONE = 0,
  // Thunder
  STORM_SEWERS_CITRINE           = 462532,  // Absorb
  ROARING_WARQUEENS_CITRINE      = 462526,  // Trigger 4 Nearby Ally Singing Thunder Citrines
  STORMBRINGERS_RUNED_CITRINE    = 462536,  // Equal Secondary Stat Split
  THUNDERLORDS_CRACKLING_CITRINE = 462540,  // ST Damage Proc

  // Sea
  MARINERS_HALLOWED_CITRINE    = 462530,  // NYI - Heal ally, jumps to 2 nearby.
  SEABED_LEVIATHANS_CITRINE    = 462527,  // Stamina & Size increase. At >80% HP attacks take dmg.
  FATHOMDWELLERS_RUNED_CITRINE = 462535,  // Mastery stat. All (?) other effects increased based on mastery.
  UNDERSEA_OVERSEERS_CITRINE   = 462538,  // Damage, jumps to 2 nearby.

  // Wind
  OLD_SALTS_BARDIC_CITRINE   = 462531,  // NYI - Heal 5t AOE
  LEGENDARY_SKIPPERS_CITRINE = 462528,  // Random other gem, 150% effect.
  WINDSINGERS_RUNED_CITRINE  = 462534,  // Highest Secondary Stat
  SQUALL_SAILORS_CITRINE     = 462539,  // Damage 5t AOE

  // Ring Driver
  CYRCES_CIRCLET =
      462342  // The ring itself. Never implemented and not a stone. Easier to reference for setting up stones.
};

enum singing_citrine_type_e
{
  CITRINE_TYPE_NONE = 0,
  CITRINE_TYPE_DAMAGE,
  CITRINE_TYPE_HEAL,
  CITRINE_TYPE_ABSORB,
  CITRINE_TYPE_STAT,
  CITRINE_TYPE_OTHER
};

singing_citrine_type_e get_singing_citrine_type( unsigned driver )
{
  switch ( driver )
  {
    case THUNDERLORDS_CRACKLING_CITRINE:
    case UNDERSEA_OVERSEERS_CITRINE:
    case SQUALL_SAILORS_CITRINE:
      return CITRINE_TYPE_DAMAGE;
    case MARINERS_HALLOWED_CITRINE:
    case OLD_SALTS_BARDIC_CITRINE:
      return CITRINE_TYPE_HEAL;
    case STORM_SEWERS_CITRINE:
      return CITRINE_TYPE_ABSORB;
    case STORMBRINGERS_RUNED_CITRINE:
    case FATHOMDWELLERS_RUNED_CITRINE:
    case SEABED_LEVIATHANS_CITRINE:
      return CITRINE_TYPE_STAT;
    case WINDSINGERS_RUNED_CITRINE:
    case LEGENDARY_SKIPPERS_CITRINE:
    case ROARING_WARQUEENS_CITRINE:
      return CITRINE_TYPE_OTHER;
    default:
      break;
  }

  return CITRINE_TYPE_NONE;
}

action_t* find_citrine_action( player_t* player, unsigned driver )
{
  switch ( driver )
  {
    // damage stones
    case THUNDERLORDS_CRACKLING_CITRINE:
      return player->find_action( "thunderlords_crackling_citrine" );
    case UNDERSEA_OVERSEERS_CITRINE:
      return player->find_action( "undersea_overseers_citrine" );
    case SQUALL_SAILORS_CITRINE:
      return player->find_action( "squall_sailors_citrine" );

    // healing stones
    case MARINERS_HALLOWED_CITRINE:
      return player->find_action( "mariners_hallowed_citrine" );
    case OLD_SALTS_BARDIC_CITRINE:
      return player->find_action( "old_salts_bardic_citrine" );

    // absorb stones
    case STORM_SEWERS_CITRINE:
      return player->find_action( "storm_sewers_citrine" );

    // Stat Stones
    case STORMBRINGERS_RUNED_CITRINE:
      return nullptr;
    case FATHOMDWELLERS_RUNED_CITRINE:
      return nullptr;
    case WINDSINGERS_RUNED_CITRINE:
      return player->find_action( "windsingers_runed_citrine_proc" );

    // other
    case ROARING_WARQUEENS_CITRINE:
      return player->find_action( "roaring_warqueens_citrine" );
    case LEGENDARY_SKIPPERS_CITRINE:
      return nullptr;
    case SEABED_LEVIATHANS_CITRINE:
      return player->find_action( "seabed_leviathans_citrine" );

    default:
      break;
  }

  return nullptr;
}

buff_t* find_citrine_proc_buff( player_t* player, unsigned driver )
{
  if ( get_singing_citrine_type( driver ) != CITRINE_TYPE_STAT )
    return nullptr;

  switch ( driver )
  {
    case STORMBRINGERS_RUNED_CITRINE:
      return buff_t::find( player, "stormbringers_runed_citrine_proc", player );
    case FATHOMDWELLERS_RUNED_CITRINE:
      return buff_t::find( player, "fathomdwellers_runed_citrine_proc", player );
    case SEABED_LEVIATHANS_CITRINE:
      return buff_t::find( player, "seabed_leviathans_citrine_proc", player );

    default:
      break;
  }

  return nullptr;
}

// TODO: Convert this to a better way I was tired when I did this.
struct stat_buff_current_value_t : stat_buff_t
{
  bool has_fathomdwellers;
  bool skipper_proc;
  double skipper_mult;
  bool queen_proc;
  bool is_proc;
  double proc_mult;

  stat_buff_current_value_t( actor_pair_t q, util::string_view name, const spell_data_t* s,
                             const item_t* item = nullptr )
    : stat_buff_t( q, name, s, item ),
      has_fathomdwellers( find_special_effect(
          player, FATHOMDWELLERS_RUNED_CITRINE ) ),  // By default, initialise this to the status. If it does not apply
                                                     // to current buff then after creation flag false.
      skipper_proc( false ),
      skipper_mult( 0 ),
      queen_proc( false ),
      is_proc( false ),
      proc_mult( 1.3 )
  {
    skipper_mult = player->find_spell( LEGENDARY_SKIPPERS_CITRINE )->effectN( 3 ).percent();
    // Stat procs dont update their value unless bumped, so only recalculate passive buffs.
    // Seems to have a heartbeat update, likely the same 5.25s heartbeat as usual. Tested with Ovinax movement buff
    // changing stack values. Last tested 19-12-2024.
    if ( data().duration() == timespan_t::zero() )
      player->sim->register_heartbeat_event_callback( [ & ]( sim_t* ) { force_recalculate(); } );
  }

  double get_buff_size()
  {
    double amount = default_value;

    if ( has_fathomdwellers )
      amount *= 1.0 + ( source->apply_combat_rating_dr(
      RATING_MASTERY, source->composite_mastery_rating() / source->current.rating.mastery ) ) /
      120;
    if ( skipper_proc )
      amount *= skipper_mult;
    // Windsingers Mastery proc doesnt seem to be affected by this... for reasons?
    if ( is_proc && ( data().id() == 465963 && util::highest_stat( source, secondary_ratings ) != STAT_MASTERY_RATING || data().id() != 465963 ) )
      amount *= proc_mult;

    return amount;
  }

  void force_recalculate()
  {
    double value = get_buff_size();
    for ( auto& buff_stat : stats )
    {
      if ( buff_stat.check_func && !buff_stat.check_func( *this ) )
        continue;

      buff_stat.amount = value;

      double delta = buff_stat_stack_amount( buff_stat, current_stack ) - buff_stat.current_value;
      if ( delta > 0 )
      {
        player->stat_gain( buff_stat.stat, delta, stat_gain, nullptr, buff_duration() > timespan_t::zero() );
      }
      else if ( delta < 0 )
      {
        player->stat_loss( buff_stat.stat, std::fabs( delta ), stat_gain, nullptr,
                           buff_duration() > timespan_t::zero() );
      }

      buff_stat.current_value += delta;
    }
  }

  void start( int stacks, double, timespan_t duration ) override
  {
    if ( skipper_proc || queen_proc )
      is_proc = true;
    else
      is_proc = false;
    buff_t::start( stacks, get_buff_size(), duration );
    if ( skipper_proc || queen_proc )
      skipper_proc = queen_proc = false;
  }

  void reset() override
  {
    buff_t::reset();
    skipper_proc = false;
  }

  void bump( int stacks, double value ) override
  {
    buff_t::bump( stacks, value );
    force_recalculate();
  }
};

template <typename BASE>
struct citrine_base_t : public BASE
{
  bool has_fathomdwellers;
  const spell_data_t* cyrce_driver;

  template <typename... ARGS>
  citrine_base_t( const special_effect_t& effect, ARGS&&... args )
    : BASE( std::forward<ARGS>( args )... ),
      has_fathomdwellers( find_special_effect( effect.player, FATHOMDWELLERS_RUNED_CITRINE ) ),
      cyrce_driver( effect.player->find_spell( CYRCES_CIRCLET ) )
  {
    BASE::background = true;
  }

  double action_multiplier() const override
  {
    double m = BASE::action_multiplier();

    if ( has_fathomdwellers )
    {
      // Seems to only use gear rating
      m *= 1.0 + ( BASE::player->apply_combat_rating_dr(
        RATING_MASTERY, BASE::player->composite_mastery_rating() / BASE::player->current.rating.mastery ) ) /
        120;
    }

    return m;
  }
};

struct damage_citrine_t : citrine_base_t<generic_proc_t>
{
  const spell_data_t* driver_spell;

  damage_citrine_t( const special_effect_t& e, std::string_view name, unsigned spell, singing_citrines_drivers_e scd )
    : citrine_base_t( e, e, name, spell ), driver_spell( e.player->find_spell( scd ) )
  {
    // TODO: Confirm bug behaviour
    // Currently the role multiplier of damaging citrines does not appear to be applying. Tested Discipline Priest and
    // Holy Priest 13/12/2024.
    if ( has_role_mult( e.player, driver_spell ) )
      this->base_multiplier *= role_mult( e.player, driver_spell );
  }

  void execute() override
  {
    if ( !target->is_enemy() )
    {
      target_cache.is_valid = false;
      target                = rng().range( target_list() );
    }

    citrine_base_t::execute();
  }
};

struct heal_citrine_t : citrine_base_t<generic_heal_t>
{
  const spell_data_t* driver_spell;
  heal_citrine_t( const special_effect_t& e, std::string_view name, unsigned spell, singing_citrines_drivers_e scd )
    : citrine_base_t( e, e, name, spell ), driver_spell( e.player->find_spell( scd ) )
  {
    if ( has_role_mult( e.player, driver_spell ) )
      this->base_multiplier *= role_mult( e.player, driver_spell );
  }

  void execute() override
  {
    if ( target->is_enemy() )
    {
      target                = player;
      target_cache.is_valid = false;
      target = rng().range( target_list() );
    }

    citrine_base_t::execute();
  }
};

struct absorb_citrine_t : citrine_base_t<absorb_t>
{
  const spell_data_t* driver_spell;
  absorb_citrine_t( const special_effect_t& e, std::string_view name, unsigned spell, singing_citrines_drivers_e scd )
    : citrine_base_t( e, name, e.player, e.player->find_spell( spell ) ), driver_spell( e.player->find_spell( scd ) )
  {
    if ( has_role_mult( e.player, driver_spell ) )
      this->base_multiplier *= role_mult( e.player, driver_spell );
  }
  void execute() override
  {
    if ( target->is_enemy() )
    {
      target                = player;
      target_cache.is_valid = false;
      target                = rng().range( target_list() );
    }

    citrine_base_t::execute();
  }

  absorb_buff_t* create_buff( const action_state_t* s ) override
  {
    absorb_buff_t* b = absorb_t::create_buff( s );

    if ( s->target->is_pet() )
      b->set_quiet( true );

    return b;
  }
};

struct thunderlords_crackling_citrine_t : public damage_citrine_t
{
  thunderlords_crackling_citrine_t( const special_effect_t& e )
    : damage_citrine_t( e, "thunderlords_crackling_citrine", 462951, THUNDERLORDS_CRACKLING_CITRINE )
  {
    base_dd_min = base_dd_max = cyrce_driver->effectN( 1 ).average( e ) * driver_spell->effectN( 2 ).percent();
  }
};

struct undersea_overseers_citrine_t : public damage_citrine_t
{
  undersea_overseers_citrine_t( const special_effect_t& e )
    : damage_citrine_t( e, "undersea_overseers_citrine", 462953, UNDERSEA_OVERSEERS_CITRINE )
  {
    base_dd_min = base_dd_max = cyrce_driver->effectN( 1 ).average( e ) * driver_spell->effectN( 2 ).percent();
  }
};

struct squall_sailors_citrine_t : public damage_citrine_t
{
  squall_sailors_citrine_t( const special_effect_t& e )
    : damage_citrine_t( e, "squall_sailors_citrine", 462952, SQUALL_SAILORS_CITRINE )
  {
    base_dd_min = base_dd_max = cyrce_driver->effectN( 1 ).average( e ) * driver_spell->effectN( 2 ).percent();
  }
};

struct seabed_leviathans_citrine_t : public damage_citrine_t
{
  seabed_leviathans_citrine_t( const special_effect_t& e )
    : damage_citrine_t( e, "seabed_leviathans_citrine", 468990, SEABED_LEVIATHANS_CITRINE )
  {
    base_dd_min = base_dd_max = cyrce_driver->effectN( 1 ).average( e ) * driver_spell->effectN( 5 ).percent();
  }
};

struct mariners_hallowed_citrine_t : public heal_citrine_t
{
  mariners_hallowed_citrine_t( const special_effect_t& e )
    : heal_citrine_t( e, "mariners_hallowed_citrine", 462960, MARINERS_HALLOWED_CITRINE )
  {
    base_dd_min = base_dd_max = cyrce_driver->effectN( 1 ).average( e ) * driver_spell->effectN( 2 ).percent();
  }
};

struct old_salts_bardic_citrine_t : public heal_citrine_t
{
  old_salts_bardic_citrine_t( const special_effect_t& e )
    : heal_citrine_t( e, "old_salts_bardic_citrine", 462959, OLD_SALTS_BARDIC_CITRINE )
  {
    base_td = cyrce_driver->effectN( 1 ).average( e ) * driver_spell->effectN( 2 ).percent() / 6;
  }
};

struct storm_sewers_citrine_t : public absorb_citrine_t
{
  struct storm_sewers_citrine_damage_t : public spell_t
  {
    storm_sewers_citrine_damage_t( player_t* p ) : spell_t( "storm_sewers_citrine_damage", p, p->find_spell( 468422 ) )
    {
    }
  };

  action_t* damage;
  storm_sewers_citrine_t( const special_effect_t& e )
    : absorb_citrine_t( e, "storm_sewers_citrine", 462958, STORM_SEWERS_CITRINE )
  {
    base_dd_min = base_dd_max = cyrce_driver->effectN( 1 ).average( e ) * driver_spell->effectN( 2 ).percent();
    damage                    = new storm_sewers_citrine_damage_t( e.player );
    add_child( damage );
  }

  void impact( action_state_t* s )
  {
    absorb_citrine_t::impact( s );

    // Always execute the damage portion because normal sims do not have damage events to break the absorb shield.
    if ( result_is_hit( s->result ) )
    {
      player_t* potential_target = s->action->player->target;
      if ( !potential_target || !potential_target->is_enemy() || potential_target->is_sleeping() )
      {
        for ( auto* t : s->action->player->sim->target_non_sleeping_list )
        {
          if ( !t->is_enemy() || t->is_sleeping() )
            continue;

          potential_target = t;
          break;
        }
      }
      if ( potential_target && potential_target->is_enemy() && !potential_target->is_sleeping() && !potential_target->is_pet() )
      {
        damage->execute_on_target( potential_target, s->result_amount * driver_spell->effectN( 3 ).percent() );
      }
    }
  }
};

action_t* create_citrine_action( const special_effect_t& effect, singing_citrines_drivers_e driver );

struct roaring_warqueen_citrine_t : public spell_t
{
  static constexpr std::array<singing_citrines_drivers_e, 3> CITRINE_DRIVERS = {
      STORM_SEWERS_CITRINE, STORMBRINGERS_RUNED_CITRINE, THUNDERLORDS_CRACKLING_CITRINE };

  enum gem_type_t
  {
    NONE = 0,
    ACTION,
    BUFF
  };

  struct citrine_data_t
  {
    gem_type_t gem_type             = gem_type_t::NONE;
    action_t* action                = nullptr;
    stat_buff_current_value_t* buff = nullptr;

    citrine_data_t()
    {
    }
  };

  target_specific_t<citrine_data_t> citrine_data;
  bool estimate_group_value;
  action_t* thunder_gem;

  roaring_warqueen_citrine_t( const special_effect_t& e )
    : spell_t( "roaring_warqueens_citrine", e.player, e.player->find_spell( 462964 ) ),
      citrine_data{ true },
      estimate_group_value( false ),
      thunder_gem( create_citrine_action( e, THUNDERLORDS_CRACKLING_CITRINE ) )
  {
    background = true;
    aoe        = as<int>( e.player->find_spell( ROARING_WARQUEENS_CITRINE )->effectN( 2 ).base_value() );

    setup_ally_gem( player );
  }

  void setup_ally_gem( player_t* t )
  {
    if ( citrine_data[ t ] )
      return;

    if ( t->is_pet() || t->is_enemy() )
      return;

    citrine_data[ t ] = new citrine_data_t();

    special_effect_t* circlet = unique_gear::find_special_effect( t, CYRCES_CIRCLET );
    if ( !circlet )
      return;

    singing_citrines_drivers_e citrine = CITRINE_DRIVER_NONE;
    for ( auto& d : CITRINE_DRIVERS )
    {
      if ( auto eff = unique_gear::find_special_effect( t, d ) )
      {
        citrine = d;
        break;
      }
    }

    if ( citrine == CITRINE_DRIVER_NONE || citrine == ROARING_WARQUEENS_CITRINE )
      return;

    if ( auto proc_buff = find_citrine_proc_buff( t, citrine ) )
    {
      auto buff                   = debug_cast<stat_buff_current_value_t*>( proc_buff );
      citrine_data[ t ]->gem_type = gem_type_t::BUFF;
      citrine_data[ t ]->buff     = buff;
      return;
    }

    if ( auto proc_action = find_citrine_action( t, citrine ) )
    {
      citrine_data[ t ]->gem_type = gem_type_t::ACTION;
      citrine_data[ t ]->action   = proc_action;
    }
  }

  void activate() override
  {
    if ( !estimate_group_value && !sim->single_actor_batch )
    {
      sim->player_no_pet_list.register_callback( [ this ]( player_t* t ) { setup_ally_gem( t ); } );

      for ( const auto& t : sim->player_no_pet_list )
      {
        setup_ally_gem( t );
      }
    }
  }

  void trigger_ally_gem( player_t* t, bool trigger_own = false )
  {
    if ( t->is_sleeping() || ( target == player && !trigger_own ) )
      return;

    if ( !citrine_data[ t ] )
      setup_ally_gem( t );

    switch ( citrine_data[ t ]->gem_type )
    {
      case gem_type_t::ACTION:
        assert( citrine_data[ t ]->action && "There must be a valid action pointer if the gem type is set to ACTION" );
        citrine_data[ t ]->action->execute();
        break;
      case gem_type_t::BUFF:
        assert( citrine_data[ t ]->buff && "There must be a valid buff pointer if the gem type is set to BUFF" );
        citrine_data[ t ]->buff->queen_proc = true;
        citrine_data[ t ]->buff->trigger();
        break;
      default:
        break;
    }
  }

  size_t available_targets( std::vector<player_t*>& target_list ) const override
  {
    target_list.clear();

    if ( estimate_group_value )
    {
      int _n_targets = aoe > 0 ? aoe : 4;
      for ( int i = 0; i < _n_targets; i++ )
      {
        target_list.push_back( player );
      }
    }
    else
    {
      for ( const auto& t : sim->player_no_pet_list )
      {
        if ( t->is_sleeping() )
          continue;

        if ( !citrine_data[ t ] || citrine_data[ t ]->gem_type == gem_type_t::NONE || t == player )
        {
          continue;
        }

        target_list.push_back( t );
      }
    }

    return target_list.size();
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( s->target != player || s->chain_target == 0 )
    {
      trigger_ally_gem( s->target, estimate_group_value );
    }

    if ( s->target == player && ( s->chain_target > 0 || citrine_data[ player ]->gem_type == gem_type_t::NONE ) )
    {
      thunder_gem->execute();
    }
  }

  void execute() override
  {
    bool skippers_estimate = player->thewarwithin_opts.estimate_skippers_group_benefit &&
                             ( player->sim->single_actor_batch || player->sim->player_no_pet_list.size() == 1 );
    // Base of 1 + small epsilon to avoid rounding errors.
    if ( base_multiplier > 1.001 && player->thewarwithin_opts.personal_estimate_skippers_group_benefit &&
         ( player->thewarwithin_opts.force_estimate_skippers_group_benefit || skippers_estimate ) )
    {
      bool old_group_value = estimate_group_value;
      // Can only happen with Legendary Skippers
      estimate_group_value = true;
      if ( estimate_group_value != old_group_value )
        target_cache.is_valid = false;
    }

    if ( target_list().size() == 0 )
    {
      // We have no targets at all. Free state(s) and abandon ship.
      if ( pre_execute_state )
      {
        action_state_t::release( pre_execute_state );
      }
      if ( execute_state )
      {
        action_state_t::release( execute_state );
      }
      return;
    }

    rng().shuffle( target_list().begin(), target_list().end() );
    target = *target_list().begin();

    spell_t::execute();
  }
};

// Proxy action just to trigger the highest stat buff. Mostly for reporting purposes.
// Might be a better way to do this, but this will do for now.
struct windsingers_runed_citrine_proc_t : public generic_proc_t
{
  std::unordered_map<stat_e, buff_t*> buffs;
  windsingers_runed_citrine_proc_t( const special_effect_t& e )
    : generic_proc_t( e, "windsingers_runed_citrine_proc", WINDSINGERS_RUNED_CITRINE ), buffs()
  {
    background = true;
    harmful = false;
    const spell_data_t* buff_driver = e.player->find_spell( WINDSINGERS_RUNED_CITRINE );
    const spell_data_t* buff_spell = e.player->find_spell( 465963 );
    const spell_data_t* cyrce_driver = e.player->find_spell( CYRCES_CIRCLET );
    double stat_value = cyrce_driver->effectN( 2 ).average( e ) * buff_driver->effectN( 2 ).percent() /
      cyrce_driver->effectN( 3 ).base_value() * cyrce_driver->effectN( 5 ).base_value() / 3;

    create_all_stat_buffs<stat_buff_current_value_t>( e, buff_spell, stat_value,
                                                      [ &, stat_value ]( stat_e s, buff_t* b )
                                                      {
                                                        b->default_value = stat_value;
                                                        b->name_str_reporting = fmt::format( "{}_{}", b->name_str, "proc");
                                                        buffs[ s ] = b;
                                                      } );
  }

  void execute() override
  {
    generic_proc_t::execute();
    // Ensure only one instance of Windsingers proc buff can be up at any time.
    for ( auto& b : buffs )
    {
      if ( b.first == util::highest_stat( player, secondary_ratings ) )
      {
        debug_cast<stat_buff_current_value_t*>( b.second )->skipper_proc = true;
        b.second->trigger();
      }
      else
        b.second->expire();
    }
  }
};

action_t* create_citrine_action( const special_effect_t& effect, singing_citrines_drivers_e driver )
{
  action_t* action = find_citrine_action( effect.player, driver );
  if ( action )
  {
    return action;
  }

  switch ( driver )
  {
    // damage stones
    case THUNDERLORDS_CRACKLING_CITRINE:
      return new thunderlords_crackling_citrine_t( effect );
    case UNDERSEA_OVERSEERS_CITRINE:
      return new undersea_overseers_citrine_t( effect );
    case SQUALL_SAILORS_CITRINE:
      return new squall_sailors_citrine_t( effect );

    // healing stones
    case MARINERS_HALLOWED_CITRINE:
      return new mariners_hallowed_citrine_t( effect );
    case OLD_SALTS_BARDIC_CITRINE:
      return new old_salts_bardic_citrine_t( effect );

    // absorb stones
    case STORM_SEWERS_CITRINE:
      return new storm_sewers_citrine_t( effect );

    // Stat Stones
    case STORMBRINGERS_RUNED_CITRINE:
      return nullptr;
    case FATHOMDWELLERS_RUNED_CITRINE:
      return nullptr;
    case WINDSINGERS_RUNED_CITRINE:
      return new windsingers_runed_citrine_proc_t( effect );

    // other
    case ROARING_WARQUEENS_CITRINE:
      return new roaring_warqueen_citrine_t( effect );
    case LEGENDARY_SKIPPERS_CITRINE:
      return nullptr;
    case SEABED_LEVIATHANS_CITRINE:
      return new seabed_leviathans_citrine_t( effect );

    default:
      break;
  }

  return nullptr;
}

struct seabed_leviathans_citrine_proc_buff_t : stat_buff_current_value_t
{
  seabed_leviathans_citrine_proc_buff_t( player_t* p, const special_effect_t& effect )
    : stat_buff_current_value_t( p, "seabed_leviathans_citrine_proc", p->find_spell( 462963 ), effect.item )
  {
    const spell_data_t* cyrce_driver = effect.player->find_spell( CYRCES_CIRCLET );
    auto buff_driver                 = effect.player->find_spell( SEABED_LEVIATHANS_CITRINE );
    double stat_value = cyrce_driver->effectN( 2 ).average( effect ) * buff_driver->effectN( 2 ).percent() /
                        cyrce_driver->effectN( 3 ).base_value() * cyrce_driver->effectN( 5 ).base_value() / 3;

    default_value = stat_value;

    add_stat_from_effect_type( A_MOD_STAT, stat_value );

    auto damage_action     = create_citrine_action( effect, SEABED_LEVIATHANS_CITRINE );
    auto hp_threshhold     = buff_driver->effectN( 6 ).base_value();
    auto proc_spell        = effect.player->find_spell( 462963 );
    auto damage            = new special_effect_t( p );
    damage->name_str       = "seabed_leviathans_citrine_proc";
    damage->item           = effect.item;
    damage->spell_id       = proc_spell->id();
    damage->proc_flags_    = PF_DAMAGE_TAKEN;
    damage->proc_flags2_   = PF2_ALL_HIT;
    damage->proc_chance_   = 1.0;
    damage->execute_action = damage_action;
    effect.player->special_effects.push_back( damage );

    effect.player->callbacks.register_callback_trigger_function(
        proc_spell->id(), dbc_proc_callback_t::trigger_fn_type::CONDITION,
        [ &, hp_threshhold ]( const dbc_proc_callback_t*, action_t*, const action_state_t* ) {
          return effect.player->health_percentage() > hp_threshhold;
        } );

    auto damage_cb = new dbc_proc_callback_t( p, *damage );
    damage_cb->activate_with_buff( this, true );
  }
};

buff_t* create_citrine_proc_buff( const special_effect_t& effect, singing_citrines_drivers_e driver )
{
  if ( get_singing_citrine_type( driver ) != CITRINE_TYPE_STAT )
    return nullptr;

  buff_t* action = find_citrine_proc_buff( effect.player, driver );
  if ( action )
  {
    return action;
  }

  switch ( driver )
  {
    case STORMBRINGERS_RUNED_CITRINE:
    {
      const spell_data_t* buff_driver  = effect.player->find_spell( driver );
      const spell_data_t* buff_spell   = effect.player->find_spell( 465961 );
      const spell_data_t* cyrce_driver = effect.player->find_spell( CYRCES_CIRCLET );
      double stat_value = cyrce_driver->effectN( 2 ).average( effect ) * buff_driver->effectN( 2 ).percent() /
                          cyrce_driver->effectN( 3 ).base_value() * cyrce_driver->effectN( 5 ).base_value() / 3;

      buff_t* buff =
          create_buff<stat_buff_current_value_t>( effect.player, "stormbringers_runed_citrine_proc", buff_spell )
              ->add_stat_from_effect( 1, stat_value )
              ->set_default_value( stat_value );
      return buff;
    }
    case FATHOMDWELLERS_RUNED_CITRINE:
    {
      const spell_data_t* buff_driver  = effect.player->find_spell( driver );
      const spell_data_t* buff_spell   = effect.player->find_spell( 465962 );
      const spell_data_t* cyrce_driver = effect.player->find_spell( CYRCES_CIRCLET );
      double stat_value = cyrce_driver->effectN( 2 ).average( effect ) * buff_driver->effectN( 2 ).percent() /
                          cyrce_driver->effectN( 3 ).base_value() * cyrce_driver->effectN( 5 ).base_value() / 3;

      buff_t* buff =
          create_buff<stat_buff_current_value_t>( effect.player, "fathomdwellers_runed_citrine_proc", buff_spell )
              ->add_stat_from_effect( 1, stat_value )
              ->set_default_value( stat_value );

      return buff;
    }
    case SEABED_LEVIATHANS_CITRINE:
      return new seabed_leviathans_citrine_proc_buff_t( effect.player, effect );
    default:
      break;
  }

  return nullptr;
}

/** Thunderlords Crackling Citrine
 * id=462342 Ring Driver
 * id=462540 Driver
 * id=462951 Damage
 */
void thunderlords_crackling_citrine( special_effect_t& effect )
{
  effect.execute_action = create_citrine_action( effect, THUNDERLORDS_CRACKLING_CITRINE );

  new dbc_proc_callback_t( effect.player, effect );
}

/** Squall Sailors Citrine
 * id=462342 Ring Driver
 * id=462539 Driver
 * id=462952 Damage
 */
void squall_sailors_citrine( special_effect_t& effect )
{
  effect.execute_action = create_citrine_action( effect, SQUALL_SAILORS_CITRINE );

  new dbc_proc_callback_t( effect.player, effect );
}

/** Undersea Overseers Citrine
 * id=462342 Ring Driver
 * id=462538 Driver
 * id=462953 Damage
 */
void undersea_overseers_citrine( special_effect_t& effect )
{
  effect.execute_action = create_citrine_action( effect, UNDERSEA_OVERSEERS_CITRINE );

  new dbc_proc_callback_t( effect.player, effect );
}
/** Storm Sewers Citrine
 * id=462342 Ring Driver
 * id=462530 Driver
 * id=462960 Heal
 */
void mariners_hallowed_citrine( special_effect_t& effect )
{
  effect.execute_action = create_citrine_action( effect, MARINERS_HALLOWED_CITRINE );

  new dbc_proc_callback_t( effect.player, effect );
}

/**  Old Salts Bardic Citrine
 * id=462342 Ring Driver
 * id=462531 Driver
 * id=462959 Heal
 */
void old_salts_bardic_citrine( special_effect_t& effect )
{
  effect.execute_action = create_citrine_action( effect, OLD_SALTS_BARDIC_CITRINE );

  new dbc_proc_callback_t( effect.player, effect );
}

/** Storm Sewers Citrine
 * id=462342 Ring Driver
 * id=462532 Driver
 * id=462958 Absorb
 * id=468422 Damage
 */
void storm_sewers_citrine( special_effect_t& effect )
{
  effect.execute_action = create_citrine_action( effect, STORM_SEWERS_CITRINE );

  new dbc_proc_callback_t( effect.player, effect );
}

/** Windsingers Runed Citrine
 * id=462342 Ring Driver
 * id=462534 Driver
 * id=465963 Proc Buff
 */
void windsingers_runed_citrine( special_effect_t& effect )
{
  auto cyrce_driver = effect.player->find_spell( CYRCES_CIRCLET );
  auto stat_value   = ( cyrce_driver->effectN( 2 ).average( effect ) / cyrce_driver->effectN( 3 ).base_value() ) *
                    effect.driver()->effectN( 2 ).percent() * ( cyrce_driver->effectN( 5 ).base_value() / 3 );

  std::unordered_map<stat_e, buff_t*> buffs;

  create_all_stat_buffs<stat_buff_current_value_t>( effect, effect.driver(), stat_value,
                                                    [ &buffs, stat_value ]( stat_e s, buff_t* b ) {
                                                      b->default_value = stat_value;
                                                      buffs[ s ]       = b;
                                                    } );

  // Windsinger does not update its given passive stat if the highest stat changes as of 19-12-2024.
  const auto& desired_stat = effect.player->thewarwithin_opts.windsingers_passive_stat;
  stat_e stat              = STAT_NONE;
  if ( util::str_compare_ci( desired_stat, "haste" ) )
    stat = STAT_HASTE_RATING;

  if ( util::str_compare_ci( desired_stat, "mastery" ) )
    stat = STAT_MASTERY_RATING;

  if ( util::str_compare_ci( desired_stat, "critical_strike" ) || util::str_compare_ci( desired_stat, "crit" ) )
    stat = STAT_CRIT_RATING;

  if ( util::str_compare_ci( desired_stat, "versatility" ) || util::str_compare_ci( desired_stat, "vers" ) )
    stat = STAT_VERSATILITY_RATING;

  if ( stat != STAT_NONE )
    effect.player->register_on_arise_callback( effect.player, [ buffs, stat ] { buffs.at( stat )->trigger(); } );
  else
    effect.player->register_on_arise_callback( effect.player, [ &, buffs ] {
      buffs.at( util::highest_stat( effect.player, secondary_ratings ) )->trigger();
    } );
}

/** Fathomdwellers Runed Citrine
 * id=462342 Ring Driver
 * id=462535 Driver
 * id=465962 Proc Buff
 */
void fathomdwellers_runed_citrine( special_effect_t& effect )
{
  auto cyrce_driver = effect.player->find_spell( CYRCES_CIRCLET );
  auto stat_value   = cyrce_driver->effectN( 2 ).average( effect ) * effect.driver()->effectN( 2 ).percent() /
                    cyrce_driver->effectN( 3 ).base_value() * cyrce_driver->effectN( 5 ).base_value() / 3;

  auto buff = create_buff<stat_buff_t>( effect.player, "fathomdwellers_runed_citrine", effect.driver() )
                  ->add_stat_from_effect( 1, stat_value );

  effect.player->register_on_arise_callback( effect.player, [ buff ] { buff->trigger(); } );
}

void legendary_skippers_citrine( special_effect_t& effect )
{
  static constexpr std::array<singing_citrines_drivers_e, 11> possible_stones = {
      // DAMAGE
      THUNDERLORDS_CRACKLING_CITRINE,
      UNDERSEA_OVERSEERS_CITRINE,
      SQUALL_SAILORS_CITRINE,

      // Stats
      STORMBRINGERS_RUNED_CITRINE,
      FATHOMDWELLERS_RUNED_CITRINE,
      WINDSINGERS_RUNED_CITRINE,
      SEABED_LEVIATHANS_CITRINE,

      // Absorb
      STORM_SEWERS_CITRINE,

      // Heal
      MARINERS_HALLOWED_CITRINE,
      OLD_SALTS_BARDIC_CITRINE,

      // Ally Trigger
      ROARING_WARQUEENS_CITRINE

      // Itself
      // LEGENDARY_SKIPPERS_CITRINE,
  };

  struct legendary_skippers_citrine_t : public spell_t
  {
    std::vector<action_t*> citrine_actions;
    std::vector<buff_t*> citrine_buffs;
    double multiplier;
    int wq_targets;

    legendary_skippers_citrine_t( const special_effect_t& effect )
      : spell_t( "legendary_skippers_citrine", effect.player, effect.player->find_spell( 462962 ) ),
        citrine_actions(),
        citrine_buffs(),
        multiplier(),
        wq_targets( as<int>( effect.player->find_spell( ROARING_WARQUEENS_CITRINE )->effectN( 2 ).base_value() ) )
    {
      background = true;

      auto driver = effect.player->find_spell( LEGENDARY_SKIPPERS_CITRINE );
      multiplier  = driver->effectN( 3 ).percent();

      for ( auto driver : possible_stones )
      {
        switch ( get_singing_citrine_type( driver ) )
        {
          case CITRINE_TYPE_STAT:
            citrine_buffs.push_back( create_citrine_proc_buff( effect, driver ) );
            break;
          default:
            citrine_actions.push_back( create_citrine_action( effect, driver ) );
            break;
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        auto ix = rng().range( citrine_actions.size() + citrine_buffs.size() );
        if ( ix < citrine_actions.size() )
        {
          if ( auto action = citrine_actions[ ix ] )
          {
              auto old_base = action->base_multiplier;
              action->base_multiplier *= multiplier;
              action->execute_on_target( s->target );
              action->base_multiplier = old_base;
          }
        }
        else
        {
          if ( auto buff = citrine_buffs[ ix - citrine_actions.size() ] )
          {
            debug_cast<stat_buff_current_value_t*>( buff )->skipper_proc = true;
            buff->trigger();
          }
        }
      }
    }
  };

  effect.execute_action = create_proc_action<legendary_skippers_citrine_t>( "legendary_skippers_citrine", effect );

  new dbc_proc_callback_t( effect.player, effect );

  struct skippers_roaring_warqueens_estimate_cb_t : public dbc_proc_callback_t
  {
    roaring_warqueen_citrine_t::citrine_data_t citrine_data;

    skippers_roaring_warqueens_estimate_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), citrine_data()
    {
      singing_citrines_drivers_e citrine = CITRINE_DRIVER_NONE;
      for ( auto& d : roaring_warqueen_citrine_t::CITRINE_DRIVERS )
      {
        if ( unique_gear::find_special_effect( effect.player, d ) )
        {
          citrine = d;
          break;
        }
      }

      if ( citrine == CITRINE_DRIVER_NONE )
        return;

      if ( auto proc_buff = find_citrine_proc_buff( effect.player, citrine ) )
      {
        auto buff             = debug_cast<stat_buff_current_value_t*>( proc_buff );
        citrine_data.gem_type = roaring_warqueen_citrine_t::gem_type_t::BUFF;
        citrine_data.buff     = buff;
        return;
      }

      if ( auto proc_action = find_citrine_action( effect.player, citrine ) )
      {
        citrine_data.gem_type = roaring_warqueen_citrine_t::gem_type_t::ACTION;
        citrine_data.action   = proc_action;
      }
    }

    void execute( action_t*, action_state_t* s ) override
    {
      switch ( citrine_data.gem_type )
      {
        case roaring_warqueen_citrine_t::gem_type_t::ACTION:
          assert( citrine_data.action && "There must be a valid action pointer if the gem type is set to ACTION" );
          citrine_data.action->execute();
          break;
        case roaring_warqueen_citrine_t::gem_type_t::BUFF:
          assert( citrine_data.buff && "There must be a valid buff pointer if the gem type is set to BUFF" );
          citrine_data.buff->queen_proc = true;
          citrine_data.buff->trigger();
          break;
        default:
          break;
      }
    }

    void trigger( action_t* a, action_state_t* state ) override
    {
      auto _rppm_modifier = rppm->get_modifier();
      rppm->set_modifier( _rppm_modifier * ( 1 + a->player->buffs.bloodlust->check_stack_value() ) );

      dbc_proc_callback_t::trigger( a, state );

      rppm->set_modifier( _rppm_modifier );
    }
  };

  if ( effect.player->thewarwithin_opts.estimate_skippers_group_benefit &&
       effect.player->thewarwithin_opts.estimate_skippers_group_members > 0 )
  {
    auto skipper_rwq_estimate      = new special_effect_t( effect.player );
    skipper_rwq_estimate->name_str = "legendary_skippers_party_estimate";
    skipper_rwq_estimate->type     = SPECIAL_EFFECT_EQUIP;
    skipper_rwq_estimate->source   = SPECIAL_EFFECT_SOURCE_ITEM;
    skipper_rwq_estimate->spell_id = effect.spell_id;
    // 25% Raid average haste assumption (slight underestimate)
    skipper_rwq_estimate->rppm_modifier_ =
        effect.player->thewarwithin_opts.estimate_skippers_group_members / possible_stones.size() * 1.25;
    effect.player->special_effects.push_back( skipper_rwq_estimate );

    // Make the RPPM Object early so it can be set to RPPM_NONE
    effect.player->get_rppm( skipper_rwq_estimate->name(), skipper_rwq_estimate->rppm(),
                             skipper_rwq_estimate->rppm_modifier(), rppm_scale_e::RPPM_NONE );

    auto cb = new skippers_roaring_warqueens_estimate_cb_t( *skipper_rwq_estimate );

    effect.player->register_combat_begin( [ cb ]( player_t* p ) {
      if ( ( !p->sim->single_actor_batch && p->sim->player_no_pet_list.size() > 1 ) &&
               !p->thewarwithin_opts.force_estimate_skippers_group_benefit ||
           p->thewarwithin_opts.personal_estimate_skippers_group_benefit )
        cb->deactivate();
    } );
  }
}


/** Stormbringers Runed Citrine
 * id=462342 Ring Driver
 * id=462536 Driver
 * id=465961 Proc Buff
 */
void stormbringers_runed_citrine( special_effect_t& effect )
{
  auto cyrce_driver = effect.player->find_spell( CYRCES_CIRCLET );
  auto stat_value   = ( cyrce_driver->effectN( 2 ).average( effect ) / cyrce_driver->effectN( 3 ).base_value() ) *
                    effect.driver()->effectN( 2 ).percent() * ( cyrce_driver->effectN( 5 ).base_value() / 3 );

  auto buff = create_buff<stat_buff_current_value_t>( effect.player, "stormbringers_runed_citrine", effect.driver() )
                  ->add_stat_from_effect( 1, stat_value )
                  ->set_default_value( stat_value );

  // Always create stormbringers proc buff in case another player has Roaring Warqueen's Citrine
  create_citrine_proc_buff( effect, STORMBRINGERS_RUNED_CITRINE );
  effect.player->register_on_arise_callback( effect.player, [ buff ] { buff->trigger(); } );
}

/** Seabed Leviathan's Citrine
 * id=462342 Ring Driver
 * id=462527 Driver
 * id=462963 Proc Buff
 * id=468990 Damage
 */
void seabed_leviathans_citrine( special_effect_t& effect )
{
  auto damage = create_citrine_action( effect, SEABED_LEVIATHANS_CITRINE );
  // Manually setting the proc flags, Driver appears to use a 0 value absorb buff
  // to check for incoming damage, rather than traditional proc flags.
  effect.proc_flags_    = PF_DAMAGE_TAKEN;
  effect.proc_flags2_   = PF2_ALL_HIT;
  effect.proc_chance_   = 1.0;
  effect.execute_action = damage;

  effect.player->callbacks.register_callback_trigger_function(
      effect.driver()->id(), dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ & ]( const dbc_proc_callback_t*, action_t*, const action_state_t* ) {
        return effect.player->health_percentage() > effect.driver()->effectN( 6 ).base_value();
      } );

  new dbc_proc_callback_t( effect.player, effect );

  auto cyrce_driver = effect.player->find_spell( CYRCES_CIRCLET );
  auto stat_value   = cyrce_driver->effectN( 2 ).average( effect ) * effect.driver()->effectN( 2 ).percent() /
                    cyrce_driver->effectN( 3 ).base_value() * cyrce_driver->effectN( 5 ).base_value() / 3;

  auto buff = create_buff<stat_buff_current_value_t>( effect.player, "seabed_leviathans_citrine", effect.driver() )
                  ->set_stat_from_effect_type( A_MOD_STAT, stat_value )
                  ->set_default_value( stat_value );

  effect.player->register_on_arise_callback( effect.player, [ buff ] { buff->trigger(); } );
}

void roaring_warqueens_citrine( special_effect_t& effect )
{
  effect.execute_action = create_citrine_action( effect, ROARING_WARQUEENS_CITRINE );

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace singing_citrines

void register_special_effects()
{
  // NOTE: use unique_gear:: namespace for static consumables so we don't activate them with enable_all_item_effects
  // Food
  unique_gear::register_special_effect( 457302, consumables::selector_food( 461957, true ) );  // the sushi special
  unique_gear::register_special_effect( 455960, consumables::selector_food( 454137 /*Maybe Wrong Spell id, had to guess*/, false ) );  // everything stew
  unique_gear::register_special_effect( 454149, consumables::selector_food( 461957, true ) );  // beledar's bounty, empress' farewell, jester's board, outsider's provisions
  unique_gear::register_special_effect( 457282, consumables::selector_food( 461939, false, false ) );  // pan seared mycobloom, hallowfall chili, coreway kabob, flash fire fillet
  unique_gear::register_special_effect( 454087, consumables::selector_food( 461943, false, false ) );  // unseasoned field steak, roasted mycobloom, spongey scramble, skewered fillet, simple stew
  unique_gear::register_special_effect( 457283, consumables::primary_food( 457284, STAT_STR_AGI_INT, 2 ) );  // feast of the divine day
  unique_gear::register_special_effect( 457285, consumables::primary_food( 457284, STAT_STR_AGI_INT, 2 ) );  // feast of the midnight masquerade
  unique_gear::register_special_effect( 457294, consumables::primary_food( 461925, STAT_STRENGTH ) );  // sizzling honey roast
  unique_gear::register_special_effect( 457136, consumables::primary_food( 461924, STAT_AGILITY ) );  // mycobloom risotto
  unique_gear::register_special_effect( 457295, consumables::primary_food( 461922, STAT_INTELLECT ) );  // stuffed cave peppers
  unique_gear::register_special_effect( 457296, consumables::primary_food( 461927, STAT_STAMINA, 7 ) );  // angler's delight
  unique_gear::register_special_effect( 457298, consumables::primary_food( 461947, STAT_STRENGTH, 3, false ) );  // meat and potatoes
  unique_gear::register_special_effect( 457297, consumables::primary_food( 461948, STAT_AGILITY, 3, false ) );  // rib stickers
  unique_gear::register_special_effect( 457299, consumables::primary_food( 461949, STAT_INTELLECT, 3, false ) );  // sweet and sour meatballs
  unique_gear::register_special_effect( 457300, consumables::primary_food( 461946, STAT_STAMINA, 7, false ) );  // tender twilight jerky
  unique_gear::register_special_effect( 457286, consumables::secondary_food( 461861, STAT_HASTE_RATING ) );  // zesty nibblers
  unique_gear::register_special_effect( 456968, consumables::secondary_food( 461860, STAT_CRIT_RATING ) );  // fiery fish sticks
  unique_gear::register_special_effect( 457287, consumables::secondary_food( 461859, STAT_VERSATILITY_RATING ) );  // ginger glazed fillet
  unique_gear::register_special_effect( 457288, consumables::secondary_food( 461858, STAT_MASTERY_RATING ) );  // salty dog
  unique_gear::register_special_effect( 457289, consumables::secondary_food( 461857, STAT_HASTE_RATING, STAT_CRIT_RATING ) );  // deepfin patty
  unique_gear::register_special_effect( 457290, consumables::secondary_food( 461856, STAT_HASTE_RATING, STAT_VERSATILITY_RATING ) );  // sweet and spicy soup
  unique_gear::register_special_effect( 457291, consumables::secondary_food( 461854, STAT_CRIT_RATING, STAT_VERSATILITY_RATING ) );  // fish and chips
  unique_gear::register_special_effect( 457292, consumables::secondary_food( 461853, STAT_MASTERY_RATING, STAT_CRIT_RATING ) );  // salt baked seafood
  unique_gear::register_special_effect( 457293, consumables::secondary_food( 461845, STAT_MASTERY_RATING, STAT_VERSATILITY_RATING ) );  // marinated tenderloins
  unique_gear::register_special_effect( 457301, consumables::secondary_food( 461855, STAT_MASTERY_RATING, STAT_HASTE_RATING ) );  // chippy tea

  // Flasks
  unique_gear::register_special_effect( 432021, consumables::flask_of_alchemical_chaos, true );

  // Potions
  unique_gear::register_special_effect( 431932, consumables::tempered_potion );
  unique_gear::register_special_effect( 431914, consumables::potion_of_unwavering_focus );

  // Oils
  register_special_effect( { 451904, 451909, 451912 }, consumables::oil_of_deep_toxins );
  register_special_effect( 448000, consumables::bubbling_wax );

  // Enchants & gems
  register_special_effect( { 448710, 448714, 448716 }, enchants::authority_of_radiant_power );
  register_special_effect( { 449221, 449223, 449222 }, enchants::authority_of_the_depths );
  register_special_effect( { 449018, 449019, 449020 }, enchants::authority_of_storms );
  register_special_effect( { 449055, 449056, 449059,                                          // council's guile (crit)
                             449095, 449096, 449097,                                          // stormrider's fury (haste)
                             449112, 449113, 449114,                                          // stonebound artistry (mastery)
                             449120, 449118, 449117 }, enchants::secondary_weapon_enchant );  // oathsworn tenacity (vers)
  register_special_effect( 435500, enchants::culminating_blasphemite );
  register_special_effect( 435488, enchants::insightful_blasphemite );
  register_special_effect( { 457615, 457616, 457617 }, enchants::daybreak_spellthread );


  // Embellishments & Tinkers
  register_special_effect( 443743, embellishments::blessed_weapon_grip );
  register_special_effect( 453503, embellishments::pouch_of_pocket_grenades );
  register_special_effect( 435992, DISABLED_EFFECT );  // prismatic null stone
  register_special_effect( 461177, embellishments::elemental_focusing_lens );
  register_special_effect( { 457665, 457677 }, embellishments::dawn_dusk_thread_lining );
  register_special_effect( 443760, embellishments::deepening_darkness );
  register_special_effect( 443736, embellishments::spark_of_beledar );
  register_special_effect( 443762, embellishments::adrenal_surge );
  register_special_effect( 443902, DISABLED_EFFECT );  // writhing armor banding
  register_special_effect( 436132, embellishments::binding_of_binding );
  register_special_effect( 436085, DISABLED_EFFECT ); // Binding of Binding on use
  register_special_effect( 453573, embellishments::siphoning_stilleto );

  // Trinkets
  register_special_effect( 444959, items::spymasters_web, true );
  register_special_effect( 444958, DISABLED_EFFECT );  // spymaster's web
  register_special_effect( 445619, items::aberrant_spellforge, true );
  register_special_effect( 445593, DISABLED_EFFECT );  // aberrant spellforge
  register_special_effect( 447970, items::sikrans_endless_arsenal );
  register_special_effect( 445203, DISABLED_EFFECT );  // sikran's shadow arsenal
  register_special_effect( 444301, items::swarmlords_authority );
  register_special_effect( 444292, DISABLED_EFFECT );  // swarmlord's authority
  register_special_effect( 444264, items::foul_behemoths_chelicera );
  register_special_effect( 445560, items::ovinaxs_mercurial_egg );
  register_special_effect( 445066, DISABLED_EFFECT );  // ovinax's mercurial egg
  register_special_effect( 449946, items::treacherous_transmitter, true );
  register_special_effect( 446209, DISABLED_EFFECT );  // treacherous transmitter
  register_special_effect( 443124, items::mad_queens_mandate );
  register_special_effect( 443128, DISABLED_EFFECT );  // mad queen's mandate
  register_special_effect( 443378, items::sigil_of_algari_concordance );
  register_special_effect( 443407, items::skarmorak_shard );
  register_special_effect( 443409, DISABLED_EFFECT );  // skarmorak's shard
  register_special_effect( 443537, items::void_pactstone );
  register_special_effect( 448904, items::ravenous_honey_buzzer );
  register_special_effect( 443411, items::overclocked_geararang_launcher );
  register_special_effect( 446764, DISABLED_EFFECT ); // overclocked gear-a-rang launcher
  register_special_effect( 443530, items::remnant_of_darkness );
  register_special_effect( 443552, items::opressive_orators_larynx );
  register_special_effect( 446787, DISABLED_EFFECT ); // opressive orator's larynx
  register_special_effect( 443541, items::arakara_sacbrood );
  register_special_effect( 444489, items::skyterrors_corrosive_organ );
  register_special_effect( 444488, DISABLED_EFFECT );  // skyterror's corrosive organ
  register_special_effect( 443415, items::high_speakers_accretion );
  register_special_effect( 443380, items::entropic_skardyn_core, true );
  register_special_effect( 443538, items::empowering_crystal_of_anubikkaj );
  register_special_effect( 450561, items::mereldars_toll );
  register_special_effect( 450641, DISABLED_EFFECT );  // mereldar's toll
  register_special_effect( 443527, items::carved_blazikon_wax );
  register_special_effect( 443531, items::signet_of_the_priory );
  register_special_effect( 450877, DISABLED_EFFECT );  // signet of the priory
  register_special_effect( 451055, items::harvesters_edict );
  register_special_effect( 443525, items::conductors_wax_whistle );
  register_special_effect( 460469, items::charged_stormrook_plume );
  register_special_effect( 443556, items::twin_fang_instruments );
  register_special_effect( 450044, DISABLED_EFFECT );  // twin fang instruments
  register_special_effect( { 463610, 463232 }, items::darkmoon_deck_symbiosis );
  register_special_effect( 455482, items::imperfect_ascendancy_serum );
  register_special_effect( { 454857, 463611 }, items::darkmoon_deck_vivacity );
  register_special_effect( 432421, items::algari_alchemist_stone );
  register_special_effect( { 458573, 463095 }, items::darkmoon_deck_ascension );
  register_special_effect( { 454558, 463108 }, items::darkmoon_deck_radiance );
  register_special_effect( 441023, items::nerubian_pheromone_secreter );
  register_special_effect( 455640, items::shadowed_essence );
  register_special_effect( 455419, items::spelunkers_waning_candle );
  register_special_effect( 455436, items::unstable_power_core );
  register_special_effect( 451742, items::stormrider_flight_badge );
  register_special_effect( 451750, DISABLED_EFFECT );  // stormrider flight badge special effect
  register_special_effect( 435502, items::shadowbinding_ritual_knife );
  register_special_effect( 455432, items::shining_arathor_insignia );
  register_special_effect( 455451, items::quickwick_candlestick );
  register_special_effect( 455435, items::candle_confidant );
  register_special_effect( 435493, items::concoction_kiss_of_death, true );
  register_special_effect( 435473, items::everburning_lantern );
  register_special_effect( 455484, items::detachable_fang );
  register_special_effect( 459222, items::scroll_of_momentum, true );
  register_special_effect( 442429, items::wildfire_wick );
  register_special_effect( 455467, items::kaheti_shadeweavers_emblem, true );
  register_special_effect( 455452, DISABLED_EFFECT );  // kaheti shadeweaver's emblem
  register_special_effect( 469927, items::hand_of_justice );
  register_special_effect( 469915, items::golem_gearbox );
  register_special_effect( 469922, items::doperels_calling_rune );
  register_special_effect( 469925, items::burst_of_knowledge );
  register_special_effect( 469768, items::heart_of_roccor );
  register_special_effect( 467767, items::wayward_vrykuls_lantern );
  register_special_effect( 468035, items::cursed_pirate_skull );
  register_special_effect( 468033, items::runecasters_stormbound_rune );
  register_special_effect( 468034, items::darktide_wavebenders_orb );
  register_special_effect( 470286, items::torqs_big_red_button );
  register_special_effect( 466681, items::house_of_cards );
  register_special_effect( 443559, items::cirral_concoctory );
  register_special_effect( 469888, items::eye_of_kezan );
  register_special_effect( 471059, items::geargrinders_remote );
  register_special_effect( 1218714, items::improvised_seaforium_pacemaker );
  register_special_effect( 471567, items::reverb_radio );
  register_special_effect( 1214787, items::mechanocore_amplifier );
  register_special_effect( 1215238, items::papas_prized_putter );
  register_special_effect( 472125, items::turbodrain_5000 );
  register_special_effect( 470675, items::noggenfogger_ultimate_deluxe );
  register_special_effect( 1216605, items::ratfang_toxin );
  register_special_effect( 1216603, DISABLED_EFFECT ); // Ratfang toxin equip driver
  register_special_effect( 1219294, items::garbagemancers_last_resort );
  register_special_effect( 1213432, items::funhouse_lens );
  register_special_effect( 1214603, DISABLED_EFFECT ); // Funhouse Lens value spell
  register_special_effect( 1216625, items::suspicious_energy_drink );
  register_special_effect( 472120, items::amorphous_relic );
  register_special_effect( 1217356, items::zees_thug_hotline );
  register_special_effect( 467469, items::mister_locknstalk );
  register_special_effect( 467485, DISABLED_EFFECT );
  register_special_effect( 471548, items::mugs_moxie_jug );

  // Weapons
  register_special_effect( 443384, items::fateweaved_needle );
  register_special_effect( 442205, items::befoulers_syringe );
  register_special_effect( 455887, items::voltaic_stormcaller );
  register_special_effect( 455819, items::harvesters_interdiction );
  register_special_effect( 469936, items::guiding_stave_of_wisdom );
  register_special_effect( 470641, items::flame_wrath );
  register_special_effect( 470647, items::force_of_magma );
  register_special_effect( 471316, items::vile_contamination );
  register_special_effect( { 473400, 473401 }, items::best_in_slots );
  register_special_effect( 471063, DISABLED_EFFECT );  // best in slots equip driver
  register_special_effect( 1218442, items::machine_gobs_iron_grin );

  // Armor
  register_special_effect( 457815, items::seal_of_the_poisoned_pact );
  register_special_effect( 457918, DISABLED_EFFECT );  // seal of the poisoned pact
  register_special_effect( 455799, items::excavation );
  register_special_effect( 457683, items::sureki_zealots_insignia );
  register_special_effect( 1214161, items::the_jastor_diamond );

  // Sets
  register_special_effect( 444067, sets::void_reapers_contract );    // kye'veza's cruel implements trinket
  register_special_effect( 444135, sets::void_reapers_warp_blade );  // kye'veza's cruel implements weapon
  register_special_effect( 444166, DISABLED_EFFECT );                // kye'veza's cruel implements
  register_special_effect( 457655, sets::woven_dusk, true );
  register_special_effect( 455521, sets::woven_dawn, true );
  register_special_effect( 443764, sets::embrace_of_the_cinderbee, true );
  register_special_effect( 443773, sets::fury_of_the_stormrook );
  
  // Singing Citrines
  register_special_effect( singing_citrines::CYRCES_CIRCLET,                    DISABLED_EFFECT );  // Disable ring driver.
  register_special_effect( singing_citrines::THUNDERLORDS_CRACKLING_CITRINE,    singing_citrines::thunderlords_crackling_citrine );
  register_special_effect( singing_citrines::UNDERSEA_OVERSEERS_CITRINE,        singing_citrines::undersea_overseers_citrine );
  register_special_effect( singing_citrines::SQUALL_SAILORS_CITRINE,            singing_citrines::squall_sailors_citrine );
  register_special_effect( singing_citrines::WINDSINGERS_RUNED_CITRINE,         singing_citrines::windsingers_runed_citrine );
  register_special_effect( singing_citrines::FATHOMDWELLERS_RUNED_CITRINE,      singing_citrines::fathomdwellers_runed_citrine );
  register_special_effect( singing_citrines::STORMBRINGERS_RUNED_CITRINE,       singing_citrines::stormbringers_runed_citrine );
  register_special_effect( singing_citrines::ROARING_WARQUEENS_CITRINE,         singing_citrines::roaring_warqueens_citrine );
  register_special_effect( singing_citrines::LEGENDARY_SKIPPERS_CITRINE,        singing_citrines::legendary_skippers_citrine );
  register_special_effect( singing_citrines::OLD_SALTS_BARDIC_CITRINE,          singing_citrines::old_salts_bardic_citrine );
  register_special_effect( singing_citrines::MARINERS_HALLOWED_CITRINE,         singing_citrines::mariners_hallowed_citrine );
  register_special_effect( singing_citrines::STORM_SEWERS_CITRINE,              singing_citrines::storm_sewers_citrine );
  register_special_effect( singing_citrines::SEABED_LEVIATHANS_CITRINE,         singing_citrines::seabed_leviathans_citrine );
}

void register_target_data_initializers( sim_t& )
{
}

void register_hotfixes()
{
}

action_t* create_action( player_t* p, std::string_view n, std::string_view options )
{
  // Trinket Actions
  if ( n == "pickup_entropic_skardyn_core" ) return new items::pickup_entropic_skardyn_core_t( p, options );
  if ( n == "do_treacherous_transmitter_task" ) return new items::do_treacherous_transmitter_task_t( p, options );
  if ( n == "pickup_nerubian_pheromone" ) return new items::pickup_nerubian_pheromone_t( p, options );

  // Set Actions
  if ( n == "pickup_cinderbee_orb" ) return new sets::pickup_cinderbee_orb_t( p, options );

  return nullptr;
}

// writhing armor banding embellishment, doubles nerubian embellishment values
double writhing_mul( player_t* p )
{
  if ( unique_gear::find_special_effect( p, 443902 ) )
    return 2.0;  // hardcoded
  else
    return 1.0;
}
}  // namespace unique_gear::thewarwithin

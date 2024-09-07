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
#include "player/action_priority_list.hpp"
#include "player/action_variable.hpp"
#include "player/consumable.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "set_bonus.hpp"
#include "sim/cooldown.hpp"
#include "sim/proc_rng.hpp"
#include "sim/sim.hpp"
#include "stats.hpp"
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

void create_all_stat_buffs( const special_effect_t& effect, const spell_data_t* buff_data, double amount,
                            std::function<void( stat_e, buff_t* )> add_fn )
{
  auto buff_name = util::tokenize_fn( buff_data->name_cstr() );

  for ( const auto& eff : buff_data->effects() )
  {
    if ( eff.type() != E_APPLY_AURA || eff.subtype() != A_MOD_RATING )
      continue;

    auto stats = util::translate_all_rating_mod( eff.misc_value1() );
    if ( stats.size() != 1 )
    {
      effect.player->sim->error( "buff data {} effect {} has multiple stats", buff_data->id(), eff.index() );
      continue;
    }

    auto stat_str = util::stat_type_abbrev( stats.front() );

    auto buff = create_buff<stat_buff_t>( effect.player, fmt::format( "{}_{}", buff_name, stat_str ), buff_data )
      ->add_stat( stats.front(), amount ? amount : eff.average( effect ) )
      ->set_name_reporting( stat_str );

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

    if ( stat2 == STAT_NONE )
    {
      auto _amt = coeff->effectN( 4 ).average( effect );
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

  effect.spell_id = driver->id();

  // TODO: confirm damage doesn't increase per extra target
  auto grenade = create_proc_action<generic_aoe_proc_t>( "pocket_grenade", effect, damage );
  grenade->base_dd_min += amount;
  grenade->base_dd_max += amount;
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
// TODO: determine if damage selection is per gem, or per unique gem type
void elemental_focusing_lens( special_effect_t& effect )
{
  // TODO: determine if damage selection is per gem, or per unique gem type
  auto gems = algari_gem_list( effect );

  if ( !gems.size() )
    return;

  auto amount = effect.driver()->effectN( 1 ).average( effect );

  effect.spell_id = effect.trigger()->id();

  // guard against duplicate proxy/callback from multiple embellishments
  auto proxy = effect.player->find_action( "elemental_focusing_lens" );
  if ( !proxy )
  {
    proxy = new action_t( action_e::ACTION_OTHER, "elemental_focusing_lens", effect.player, effect.driver() );
    new dbc_proc_callback_t( effect.player, effect );
  }

  std::unordered_map<gem_color_e, action_t*> damages;

  static constexpr std::array<std::tuple<gem_color_e, unsigned, const char*>, 5> damage_ids = { {
    { GEM_RUBY,     461192, "ruby"     },
    { GEM_AMBER,    461185, "amber"    },
    { GEM_EMERALD,  461190, "emerald"  },
    { GEM_SAPPHIRE, 461193, "sapphire" },
    { GEM_ONYX,     461191, "onyx"     }
  } };

  for ( auto [ g, id, name ] : damage_ids )
  {
    auto dam = create_proc_action<generic_proc_t>( fmt::format( "elemental_focusing_lens_{}", name ), effect, id );
    dam->base_dd_min += amount;
    dam->base_dd_max += amount;
    dam->name_str_reporting = util::inverse_tokenize( name );
    damages[ g ] = dam;
    proxy->add_child( dam );
  }

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ gems, damages ]( const dbc_proc_callback_t* cb, action_t*, const action_state_t* s ) {
      damages.at( gems.at( cb->rng().range( gems.size() ) ) )->execute_on_target( s->target );
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
     double binding_of_binding_ally_skip_chance;
     binding_of_binding_cb_t( const special_effect_t& e )
       : dbc_proc_callback_t( e.player, e ),
         buffs{ false },
         binding_of_binding_ally_skip_chance( effect.player->thewarwithin_opts.binding_of_binding_ally_skip_chance )
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

             if ( rng().roll( binding_of_binding_ally_skip_chance ) )
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
    effect.driver()->effectN( 2 ).average( effect ) * role_mult( effect ) * writhing_mul( effect.player );

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
// TODO: confirm there is an animation delay for the spiders to return before buff is applied
// TODO: confirm you cannot use without stacking buff present
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
    ->set_stat_from_effect_type( A_MOD_STAT, equip_data->effectN( 1 ).average( effect ) );

  // TODO: confirm there is an animation delay for the spiders to return before buff is applied
  effect.player->callbacks.register_callback_execute_function( equip_id,
      [ stacking_buff,
        p = effect.player,
        vel = buff_data->missile_speed() ]
      ( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
        auto delay = timespan_t::from_seconds( p->get_player_distance( *s->target ) / vel );

        make_event( *p->sim, delay, [ stacking_buff ] { stacking_buff->trigger(); } );
      } );

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

    // TODO: confirm you cannot use without stacking buff present
    bool ready() override
    {
      return stacking_buff->check();
    }

    void execute() override
    {
      generic_proc_t::execute();
      
      use_buff->expire();
      use_buff->trigger( stacking_buff->check() );
      stacking_buff->expire();
    }
  };

  effect.disable_buff();
  effect.execute_action = create_proc_action<spymasters_web_t>( "spymasters_web", effect, stacking_buff, use_buff );
}

// 444067 driver
// 448643 unknown (0.2s duration, 6yd radius, outgoing aoe hit?)
// 448621 unknown (0.125s duration, echo delay?)
// 448669 damage
// TODO: confirm if additional echoes are immediate, delayed, or staggered
void void_reapers_chime( special_effect_t& effect )
{
  struct void_reapers_chime_cb_t : public dbc_proc_callback_t
  {
    action_t* major;
    action_t* minor;
    action_t* queensbane = nullptr;
    double hp_pct;

    void_reapers_chime_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), hp_pct( e.driver()->effectN( 2 ).base_value() )
    {
      auto damage_spell = effect.player->find_spell( 448669 );
      auto damage_name = std::string( damage_spell->name_cstr() );
      auto damage_amount = effect.driver()->effectN( 1 ).average( effect );

      major = create_proc_action<generic_aoe_proc_t>( damage_name, effect, damage_spell );
      major->base_dd_min = major->base_dd_max = damage_amount;
      major->base_multiplier *= role_mult( effect );
      major->base_aoe_multiplier = effect.driver()->effectN( 5 ).percent();

      minor = create_proc_action<generic_aoe_proc_t>( damage_name + "_echo", effect, damage_spell );
      minor->base_dd_min = minor->base_dd_max = damage_amount * effect.driver()->effectN( 3 ).percent();
      minor->base_multiplier *= role_mult( effect );
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
          // TODO: are these immediate, delayed, or staggered?
          minor->execute_on_target( s->target );
          minor->execute_on_target( s->target );
      }
    }
  };

  new void_reapers_chime_cb_t( effect );
}

// 445593 equip
//  e1: damage value
//  e2: haste value
//  e3: mult per use stack
//  e4: unknown, damage cap? self damage?
//  e5: post-combat duration
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
// TODO: confirm empowerment is not consumed by procs
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
                   ->set_default_value( data->effectN( 3 ).percent() )
                   ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  auto haste = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 451845 ) )
    ->set_stat_from_effect_type( A_MOD_RATING, data->effectN( 2 ).average( effect ) );

  // proc damage action
  struct aberrant_shadows_t : public generic_proc_t
  {
    buff_t* stack;

    aberrant_shadows_t( const special_effect_t& e, const spell_data_t* data, buff_t* b )
      : generic_proc_t( e, "aberrant_shadows", 451866 ), stack( b )
    {
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e );
      base_multiplier *= role_mult( e );

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
      return generic_proc_t::action_multiplier() * ( 1.0 + stack->check_stack_value() );
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
      [ id = empowered->id() ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* ) {
        return a->data().id() == id;
      } );

  // TODO: confirm empowerment is not consumed by procs
  effect.player->callbacks.register_callback_execute_function( equip->spell_id,
      [ damage, empowerment ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* s ) {
        damage->execute_on_target( s->target );
        empowerment->expire( a );
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

  effect.player->register_on_combat_state_callback(
      [ stack, dur = timespan_t::from_seconds( data->effectN( 5 ).base_value() ) ]( player_t* p, bool c ) {
        if ( !c )
        {
          make_event( *p->sim, dur, [ p, stack ] {
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
// TODO: confirm decimation damage does not have standard +15% per target up to five
// TODO: confirm decimation damage on crit to shield only counts the absorbed amount, instead of the full damage
// TODO: confirm barrage damage isn't split and has no diminishing returns
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
      f_dam->base_multiplier *= role_mult( e );
      add_child( f_dam );

      auto f_stance = create_buff<stat_buff_t>( e.player, e.player->find_spell( 447962 ) )
        ->add_stat_from_effect( 1, data->effectN( 2 ).base_value() )
        ->add_stat_from_effect( 2, data->effectN( 3 ).base_value() );

      stance.emplace_back( f_dam, f_stance );

      // setup decimation
      auto d_dam = create_proc_action<generic_aoe_proc_t>( "surekian_decimation", e, 448090 );
      // TODO: confirm there is no standard +15% per target up to five
      d_dam->base_dd_min = d_dam->base_dd_max = data->effectN( 4 ).average( e ) * role_mult( e );
      d_dam->base_multiplier *= role_mult( e );
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
              d_shield->base_dd_min = d_shield->base_dd_max = mul * absorbed;
              d_shield->execute_on_target( s->target );
            }
          } );

      auto d_cb = new dbc_proc_callback_t( e.player, *d_driver );
      d_cb->activate_with_buff( d_stance );

      stance.emplace_back( d_dam, d_stance );

      // setup barrage
      auto b_dam = create_proc_action<generic_aoe_proc_t>( "surekian_barrage", e, 445475 );
      // TODO: confirm damage isn't split and has no diminishing returns
      b_dam->split_aoe_damage = false;
      b_dam->base_dd_min = b_dam->base_dd_max = data->effectN( 6 ).average( e ) * role_mult( e );
      b_dam->base_multiplier *= role_mult( e );
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
// TODO: determine how shield behaves during on-use
// TODO: confirm equip cycle restarts on combat
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
// TODO: confirm effect value is for the entire dot and not per tick
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
      // TODO: confirm effect value is for the entire dot and not per tick
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
// TODO: confirm equip cycle continues ticking while on-use is active
// TODO: confirm that stats swap at one stack per tick
// TODO: determine what happens when your highest secondary changes
// TODO: add options to control balancing stacks
// TODO: add option to set starting stacks
// TODO: confirm stat value truncation happens on final amount, and not per stack amount
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
        cap_mul( data->effectN( 4 ).percent() ),
        cap( as<int>( data->effectN( 5 ).base_value() ) )
    {}

    double buff_stat_stack_amount( const buff_stat_t& stat, int s ) const override
    {
      double val = std::max( 1.0, std::fabs( stat.amount ) );
      double stack = s <= cap ? s : cap + ( s - cap ) * cap_mul;
      // TODO: confirm truncation happens on final amount, and not per stack amount
      return std::copysign( std::trunc( stack * val + 1e-3 ), stat.amount );
    }
  };

  // setup stat buffs
  auto primary = create_buff<ovinax_stat_buff_t>( effect.player, effect.player->find_spell( 449578 ), data )
    ->set_stat_from_effect_type( A_MOD_STAT, data->effectN( 1 ).average( effect ) )
    ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

  static constexpr std::array<unsigned, 4> buff_ids = { 449595, 449594, 449581, 449593 };

  std::unordered_map<stat_e, buff_t*> secondaries;

  for ( size_t i = 0; i < secondary_ratings.size(); i++ )
  {
    auto stat_str = util::stat_type_abbrev( secondary_ratings[ i ] );
    auto spell = effect.player->find_spell( buff_ids[ i ] );
    auto name = fmt::format( "{}_{}", spell->name_cstr(), stat_str );

    auto buff = create_buff<ovinax_stat_buff_t>( effect.player, name, spell, data )
      ->set_stat_from_effect_type( A_MOD_RATING, data->effectN( 2 ).average( effect ) )
      ->set_name_reporting( stat_str )
      ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

    secondaries[ secondary_ratings[ i ] ] = buff;
  }

  // proxy buff for on-use
  // TODO: confirm equip cycle continues ticking while on-use is active
  auto halt = create_buff<buff_t>( effect.player, effect.driver() )->set_cooldown( 0_ms );

  // proxy buff for equip ticks
  // TODO: confirm that stats swap at one stack per tick
  // TODO: determine what happens when your highest secondary changes
  // TODO: add options to control balancing stacks
  auto ticks = create_buff<buff_t>( effect.player, equip->driver() )
    ->set_quiet( true )
    ->set_tick_zero( true )
    ->set_tick_callback( [ primary, secondaries, halt, p = effect.player ]( buff_t*, int, timespan_t ) {
      if ( halt->check() )
        return;

      if ( p->is_moving() )
      {
        primary->decrement();
        secondaries.at( util::highest_stat( p, secondary_ratings ) )->trigger();
      }
      else
      {
        range::for_each( secondaries, []( const auto& b ) { b.second->decrement(); } );
        primary->trigger();
      }
    } );

  int initial_primary_stacks   = effect.player->thewarwithin_opts.ovinaxs_mercurial_egg_initial_primary_stacks;
  int initial_secondary_stacks = effect.player->thewarwithin_opts.ovinaxs_mercurial_egg_initial_secondary_stacks;
  if ( initial_primary_stacks + initial_secondary_stacks > primary->max_stack() )
  {
    initial_secondary_stacks = primary->max_stack() - initial_primary_stacks;

    effect.player->sim->error(
      "Ovinax's Mercurial Egg initial stacks can not exceed '{}' combined. Primary stacks set to '{}', secondary "
      "stacks set to '{}'.",
      primary->max_stack(), initial_primary_stacks, initial_secondary_stacks );
  }

  effect.player->register_precombat_begin(
      [ ticks, primary, secondaries, initial_primary_stacks, initial_secondary_stacks ]( player_t* p ) {
        if ( initial_primary_stacks )
          primary->trigger( initial_primary_stacks );

        if ( initial_secondary_stacks )
          secondaries.at( util::highest_stat( p, secondary_ratings ) )->trigger( initial_secondary_stacks );

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

    cryptic_instructions_t( const special_effect_t& e ) : generic_proc_t( e, "cryptic_instructions", e.driver() )
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
      }

      if ( apl_actions.size() > 0 )
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
    }

    void execute() override
    {
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
        make_event( *sim, rng().range( 3_s, player->find_spell( 449947 )->duration() - 250_ms ),
                    [ this ] { stat_buff->trigger(); } );
      }
    }
  };
  
  effect.disable_buff();
  effect.stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );
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
// TODO: confirm heal coeff is for entire hot
// TODO: determine magnitude of increase for missing health. currently assumed 100%
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
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e ) * role_mult( e );
      base_multiplier *= role_mult( e );

      heal = create_proc_action<generic_heal_t>( "abyssal_gluttony_heal", e, "abyssal_gluttony_heal",
                                                 e.trigger()->effectN( 2 ).trigger() );
      heal->name_str_reporting = "abyssal_gluttony";
      // TODO: confirm heal coeff is for entire hot
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

    algari_pet_cast_event_t( pet_t* p, timespan_t time_to_execute, unsigned tick, action_t* st_action,
                             action_t* aoe_action, action_t* one_time_action )
      : event_t( *p, time_to_execute ),
        st_action( st_action ),
        aoe_action( aoe_action ),
        one_time_action( one_time_action ),
        tick( tick ),
        pet( p )
    {
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
        make_event<algari_pet_cast_event_t>( sim(), pet, pet->find_spell( 452325 )->effectN( 1 ).period(), tick,
                                             st_action, aoe_action, one_time_action );
      }
    }
  };

  struct sigil_of_algari_concordance_pet_t : public pet_t
  {
    action_t* st_action;
    action_t* one_time_action;
    action_t* aoe_action;

    sigil_of_algari_concordance_pet_t( util::string_view name, const special_effect_t& e, const spell_data_t* summon_spell )
      : pet_t( e.player->sim, e.player, name, true, true ),
        st_action( nullptr ),
        one_time_action( nullptr ),
        aoe_action( nullptr )
    {
      npc_id = summon_spell->effectN( 1 ).misc_value1();
    }

    resource_e primary_resource() const override
    {
      return RESOURCE_NONE;
    }

    void arise() override
    {
      pet_t::arise();
      make_event<algari_pet_cast_event_t>( *sim, this, 0_ms, 0, st_action, aoe_action, one_time_action );
    }
  };

  struct algari_concodance_pet_spell_t : public spell_t
  {
    unsigned max_scaling_targets;
    bool scale_aoe_damage;
    algari_concodance_pet_spell_t( util::string_view n, pet_t* p, const spell_data_t* s, action_t* a )
      : spell_t( n, p, s ), max_scaling_targets( 5 ), scale_aoe_damage( false )
    {
      background = true;
      auto proxy = a;
      auto it    = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
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

    thunder_bolt_silvervein_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : algari_concodance_pet_spell_t( name, p, p->find_spell( 452335 ), a )
    {
      name_str_reporting = "thunder_bolt";
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );
    }
  };

  struct bolt_rain_t : public algari_concodance_pet_spell_t
  {
    bolt_rain_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
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
    thundering_bolt_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
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
    mighty_smash_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
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
    earthen_ire_buff_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
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

      // TODO: doesn't split damage at all
      if ( e.player->bugs )
      {
        split_aoe_damage = false;
        aoe_damage_increase = false;
      }
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

  auto damage      = new special_effect_t( e.player );
  damage->name_str = "overclocked_strike_proc";
  damage->item     = e.item;
  damage->spell_id = damage_buff_spell->id();
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
      base_multiplier = role_mult( e );
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
      damage->base_multiplier = role_mult( e );
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
      base_dd_min = base_dd_max = equip_driver->effectN( 2 ).average( e ) * role_mult( e );
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
//  e1: trigger damage with delay?
//  e2: damage coeff
//  e3: buff coeff
// 450921 damage
// 450922 area trigger
// 451247 buff return missile
// 451248 buff
// TODO: confirm damage increases by standard 15% per extra target
// TODO: confirm buff increases by 15% per extra taget hit, up to 5.
// TODO: confirm ticks once a second, not hasted
// TODO: confirm buff return has 500ms delay
// TODO: confirm 500ms delay before damage starts ticking (not including 1s tick time)
// TODO: confirm buff procs even if no targets are hit
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
      // TODO: confirm damage increases by standard 15% per extra target
      auto damage =
        create_proc_action<high_speakers_accretion_damage_t>( "high_speakers_accretion_damage", e, targets_hit );
      damage->stats = stats;

      // TODO: confirm buff increases by 15% per extra target hit, up to 5.
      auto buff = create_buff<stat_buff_with_multiplier_t>( e.player, e.player->find_spell( 451248 ) );
      buff->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 3 ).average( e ) );

      // TODO: confirm ticks once a second, not hasted
      // TODO: confirm buff return has 500ms delay
      params.pulse_time( 1_s )
          .duration( e.trigger()->duration() )
          .action( damage )
          .expiration_callback(
              [ this, buff, delay = timespan_t::from_seconds( e.player->find_spell( 451247 )->missile_speed() ) ](
                  const action_state_t* ) {
                // TODO: confirm buff procs even if no targets  are hit
                if ( !targets_hit.empty() )
                  buff->stat_mul = 1.0 + 0.15 * ( as<unsigned>( targets_hit.size() ) - 1u );

                make_event( *sim, delay, [ buff ] { buff->trigger(); } );
              } );

      // TODO: confirm 500ms delay before damage starts ticking (not including 1s tick time)
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
        if ( auto pickup = dynamic_cast<pickup_entropic_skardyn_core_t*>( a ) )
        {
          pickup->buff = buff;
          pickup->tracker = tracker;
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
// TODO: confirm refreshing proc can change stat
// TODO: confirm refreshing proc can pick same stat
// TODO: confirm refreshing proc doesn't stack stats (cannot be confirmed via tooltip)
void empowering_crystal_of_anubikkaj( special_effect_t& effect )
{
  std::vector<buff_t*> buffs;

  create_all_stat_buffs( effect, effect.trigger(), 0,
    [ &buffs ]( stat_e, buff_t* b ) { buffs.push_back( b ); } );

  // TODO: confirm refreshing proc can change stat
  // TODO: confirm refreshing proc can pick same stat
  // TODO: confirm refreshing proc doesn't stack stats (cannot be confirmed via tooltip)
  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ buffs ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
      auto buff = buffs[ cb->listener->rng().range( 0U, as<unsigned>( buffs.size() ) ) ];
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
// TODO: confirm 950ms delay in damage
// TODO: determine if attack needs to do damage to proc vers buff
void mereldars_toll( special_effect_t& effect )
{
  unsigned equip_id = 450641;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Mereldar's Toll missing equip effect" );

  auto data = equip->driver();

  struct mereldars_toll_t : public generic_proc_t
  {
    buff_t* vers;
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
      impact_action->base_dd_min = impact_action->base_dd_max = data->effectN( 2 ).average( e ) * role_mult( e );
      // TODO: confirm 950ms delay in damage
      impact_action->travel_delay = e.driver()->effectN( 2 ).misc_value1() * 0.001;
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
        toll->spell_id, [ this, debuff ]( const dbc_proc_callback_t*, action_t* a, action_state_t* ) {
          auto vers = get_buff( a->player );
          if ( vers && !vers->check() )
          {
            debuff->trigger();
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
// TODO: confirm damage doesn't increase per extra target
// TODO: determine travel speed to hit target, assuming 5yd/s based on 443549 range/duration
// TODO: determine reasonable delay to intercept
void harvesters_edict( special_effect_t& effect )
{
  // TODO: confirm damage doesn't increase per extra target
  auto damage = create_proc_action<generic_aoe_proc_t>( "volatile_blood_blast", effect, effect.driver() );
  damage->base_dd_min = damage->base_dd_max =
    effect.driver()->effectN( 1 ).average( effect ) * role_mult( effect );
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
// TODO: confirm damage does not increase per extra target
// TODO: determine travel speed/delay, assuming 7.5yd/s based on summed cart path(?) radius/duration
void conductors_wax_whistle( special_effect_t& effect )
{
  // TODO: confirm damage does not increase per extra target
  auto damage = create_proc_action<generic_aoe_proc_t>( "collision", effect, 450429 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );
  // TODO: determine travel speed/delay, assuming 7.5yd/s based on summed cart path(?) radius/duration
  damage->travel_speed = 7.5;

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

// 443337 driver
//  e1: damage coeff
//  e2: cast time?
//  e3: unknown
// 448892 damage
// TODO: determine if cast time changes
// TODO: confirm damage does not increase per extra target
void charged_stormrook_plume( special_effect_t& effect )
{
  // TODO: confirm damage does not increase per extra target
  auto damage = create_proc_action<generic_aoe_proc_t>( "charged_stormrook_plume", effect, 448892 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect );
  damage->base_multiplier *= role_mult( effect );
  // TODO: determine if cast time changes
  damage->base_execute_time = effect.driver()->cast_time();

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
// TODO: confirm can be reused instantly
// TODO: confirm melee damage does not split
// TODO: confirm range damage does not increase per extra target
// TODO: fully implement triple-use if it goes live like that. for now we just cast all 3 at once
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

    twin_fang_instruments_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "twin_fang_instruments", e.driver() )
    {
      melee = create_proc_action<generic_aoe_proc_t>( "nxs_shadow_strike", e, e.player->find_spell( 450119 ) );
      melee->base_dd_min = melee->base_dd_max = data->effectN( 1 ).average( e ) * 0.5;
      melee->base_multiplier *= role_mult( e );
      // TODO: confirm melee damage does not split
      melee->split_aoe_damage = false;
      add_child( melee );

      // TODO: confirm range damage does not increase per extra target
      range = create_proc_action<generic_aoe_proc_t>( "vxs_frost_slash", e, e.player->find_spell( 450158 ) );
      range->base_dd_min = range->base_dd_max = data->effectN( 1 ).average( e );
      range->base_multiplier *= role_mult( e );
      range->travel_speed = range->data().effectN( 3 ).trigger()->missile_speed();
      add_child( range );
    }

    void execute() override
    {
      generic_proc_t::execute();

      // TODO: fully implement triple-use if it goes live like that. for now we just cast all 3 at once
      melee->execute_on_target( target );
      range->execute_on_target( target );

      melee->execute_on_target( target );
      range->execute_on_target( target );
    }
  };

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
// TODO: confirm buff value once scaling is fixed
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
// 454857 driver/values
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

    vivacity_cb_t( const special_effect_t& e )
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
      auto values = e.player->find_spell( 454857 );
      
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

  effect.spell_id = 454859;

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;  

  new vivacity_cb_t( effect );
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

    ascension_tick_t( const special_effect_t& e, util::string_view n, const spell_data_t* s, bool embellish )
      : buff_t( e.player, n, s ), buff_list(), last_buff( nullptr ), stack( 0 ), in_combat( false )
    {
      add_stats( e, embellish );
      last_buff = buff_list[ 0 ];
      set_tick_callback( [ & ]( buff_t*, int, timespan_t ) {
        // Checks on tick if player is in combat, if not, expire the buff
        if ( !in_combat )
          make_event( *e.player->sim, 0_ms, [ & ] { expire(); } );
        else
          trigger_ascension();
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
        // TODO: confirm that embellishment counts as a nerubian embellishement, but the card does not.
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

    radiant_focus_debuff_t( actor_pair_t td, const special_effect_t& e, util::string_view n, const spell_data_t* s,
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

      auto buff_idx = effect.player->sim->rng().range( buffs.size() );
      buffs[ buff_idx ]->trigger( effect.player->rng().range( 10_s, 30_s ) );
    } );
  }

  effect.player->register_combat_begin( [ buffs ]( player_t* p ) {
    auto buff_idx = p->sim->rng().range( buffs.size() );
    buffs[ buff_idx ]->trigger( p->rng().range( 10_s, 30_s ) );
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

  std::vector<buff_t*> negative_buffs = {};

  auto buff_spell = effect.driver()->effectN( 2 ).trigger();

  for ( const auto& e : buff_spell->effects() )
  {
    if ( e.type() == E_APPLY_AURA && ( e.subtype() == A_MOD_RATING ) )
    {
      auto mods = util::translate_all_rating_mod( e.misc_value1() );

      std::vector<std::string> str_list;

      for ( auto stat : mods )
        str_list.emplace_back( util::stat_type_string( stat ) );

      auto negative_buff = make_buff<stat_buff_t>(
          effect.player, fmt::format( "shadowbinding_ritual_knife_{}", util::string_join( str_list ) ), buff_spell );

      if ( !mods.empty() )
      {
        for ( const auto& s : mods )
        {
          negative_buff->add_stat( s, effect.driver()->effectN( 2 ).average( effect.item ) );
        }
        negative_buffs.push_back( negative_buff );
      }

    }
  }

  if ( negative_buffs.size() > 0 )
  {
    effect.player->callbacks.register_callback_execute_function(
        effect.driver()->id(), [ negative_buffs ]( const dbc_proc_callback_t* cb, action_t*, const action_state_t* ) {
          negative_buffs[ cb->listener->rng().range( negative_buffs.size() ) ]->trigger();
        } );
  }

  effect.buff_disabled = true;

  new dbc_proc_callback_t( effect.player, effect );
}

// Weapons
// 444135 driver
// 448862 dot (trigger)
// TODO: confirm effect value is for the entire dot and not per tick
void void_reapers_claw( special_effect_t& effect )
{
  effect.duration_ = effect.trigger()->duration();
  effect.tick = effect.trigger()->effectN( 1 ).period();
  // TODO: confirm effect value is for the entire dot and not per tick
  effect.discharge_amount =
    effect.driver()->effectN( 1 ).average( effect ) * effect.tick / effect.duration_ * role_mult( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

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
    }
  };

  new fateweaved_needle_cb_t( effect );
}

// 442205 driver
// 442268 dot
// 442267 on-next buff
// 442280 on-next damage
// TODO: confirm it uses psuedo-async behavior found in other similarly worded dots
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
      // TODO: confirm it uses psuedo-async behavior found in other similarly worded dots
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

      // execute() instead of trigger() to avoid proc delay
      get_debuff( s->target )->execute( 1, buff_t::DEFAULT_VALUE(), dot_duration );
    }
  };

  struct befouling_strike_t : public generic_proc_t
  {
    befouling_strike_t( const special_effect_t& e )
      : generic_proc_t( e, "befouling_strike", e.player->find_spell( 442280 ) )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e );
      base_multiplier *= role_mult( e );
    }
  };

  // create on-next melee damage
  auto strike = create_proc_action<generic_proc_t>( "befouling_strike", effect, 442280 );
  strike->base_dd_min = strike->base_dd_max = effect.driver()->effectN( 2 ).average( effect );

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
// TODO: determine any delay in damage and if travel_delay needs to be implemented
void voltaic_stormcaller( special_effect_t& effect )
{
  struct voltaic_stormstrike_t : public generic_aoe_proc_t
  {
    stat_buff_with_multiplier_t* buff;

    voltaic_stormstrike_t( const special_effect_t& e ) : generic_aoe_proc_t( e, "voltaic_stormstrike", 455910, true )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e ) * role_mult( e );

      buff = create_buff<stat_buff_with_multiplier_t>( e.player, e.player->find_spell( 456652 ) );
      buff->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 2 ).average( e ) );
    }

    void execute() override
    {
      generic_aoe_proc_t::execute();

      buff->stat_mul = generic_aoe_proc_t::composite_aoe_multiplier( execute_state );
      buff->trigger();
    }
  };

  // TODO: determine any delay in damage and if travel_delay needs to be implemented
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
          e.driver()->effectN( 2 ).average( e ) * role_mult( e ) * writhing_mul( e.player );
    }

    void execute( action_t*, action_state_t* ) override
    {
      self->execute_on_target( listener );
      // TODO: implement range check if it ever matters for specilizations that can use this.
      make_event( *listener->sim, duration, [ & ] {
        auto target = listener->sim
          ->target_non_sleeping_list[ rng().range( listener->sim->target_non_sleeping_list.size() ) ];
        damage->execute_on_target( target );
      } );
    }
  };

  new siphoning_stilleto_cb_t( effect );
}

// Armor
// 457815 driver
// 457918 nature damage driver
// 457925 counter
// 457928 dot
// TODO: confirm coeff is for entire dot and not per tick
void seal_of_the_poisoned_pact( special_effect_t& effect )
{
  unsigned nature_id = 457918;
  auto nature = find_special_effect( effect.player, nature_id );
  assert( nature && "Seal of the Poisoned Pact missing nature damage driver" );

  auto dot = create_proc_action<generic_proc_t>( "venom_shock", effect, 457928 );
  // TODO: confirm coeff is for entire dot and not per tick
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
  struct excavation_cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buff_list;
    unsigned dig_count;

    excavation_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), buff_list(), dig_count( 0 )
    {
      static constexpr std::pair<unsigned, unsigned> buff_entries[] = { { 455820, 0 }, { 455826, 1 }, { 455827, 2 } };

      for ( const auto& [ id, n ] : buff_entries )
      {
        auto s_data  = e.player->find_spell( id );
        double value =  e.driver()->effectN( 1 ).average( e ) * ( 1.0 + ( 0.05 * n ) );

        auto buff = create_buff<stat_buff_t>( e.player, s_data )->add_stat_from_effect_type( A_MOD_STAT, value );

        buff_list.push_back( buff );
      }
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

}  // namespace items

namespace sets
{
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
}  // namespace sets

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

  // Enchants & gems
  register_special_effect( { 448710, 448714, 448716 }, enchants::authority_of_radiant_power );
  register_special_effect( { 449221, 449223, 449222 }, enchants::authority_of_the_depths );
  register_special_effect( { 449018, 449019, 449020 }, enchants::authority_of_storms );
  register_special_effect( { 449055, 449056, 449059,                                          // council's guile (crit)
                             449095, 449096, 449097,                                          // stormrider's fury (haste)
                             449112, 449113, 449114,                                          // stonebound artistry (mastery)
                             449120, 449118, 449117 }, enchants::secondary_weapon_enchant );  // oathsworn tenacity (vers)
  register_special_effect( 435500, enchants::culminating_blasphemite );


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

  // Trinkets
  register_special_effect( 444959, items::spymasters_web, true );
  register_special_effect( 444958, DISABLED_EFFECT );  // spymaster's web
  register_special_effect( 444067, items::void_reapers_chime );
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
  register_special_effect( 443337, items::charged_stormrook_plume );
  register_special_effect( 443556, items::twin_fang_instruments );
  register_special_effect( 450044, DISABLED_EFFECT );  // twin fang instruments
  register_special_effect( { 463610, 463232 }, items::darkmoon_deck_symbiosis );
  register_special_effect( 455482, items::imperfect_ascendancy_serum );
  register_special_effect( 454857, items::darkmoon_deck_vivacity );
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

  // Weapons
  register_special_effect( 444135, items::void_reapers_claw );
  register_special_effect( 443384, items::fateweaved_needle );
  register_special_effect( 442205, items::befoulers_syringe );
  register_special_effect( 455887, items::voltaic_stormcaller );
  register_special_effect( 455819, items::harvesters_interdiction );
  register_special_effect( 453573, items::siphoning_stilleto );

  // Armor
  register_special_effect( 457815, items::seal_of_the_poisoned_pact );
  register_special_effect( 457918, DISABLED_EFFECT );  // seal of the poisoned pact
  register_special_effect( 455799, items::excavation );

  // Sets
  register_special_effect( 444166, DISABLED_EFFECT );  // kye'veza's cruel implements
  register_special_effect( 457655, sets::woven_dusk, true );
  register_special_effect( 455521, sets::woven_dawn, true );
  register_special_effect( 443764, sets::embrace_of_the_cinderbee, true );
}

void register_target_data_initializers( sim_t& )
{
}

void register_hotfixes()
{
}

action_t* create_action( player_t* p, util::string_view n, util::string_view options )
{
  // Trinket Actions
  if ( n == "pickup_entropic_skardyn_core" ) return new items::pickup_entropic_skardyn_core_t( p, options );
  if ( n == "do_treacherous_transmitter_task" ) return new items::do_treacherous_transmitter_task_t( p, options );
  if ( n == "pickup_nerubian_pheromone" ) return new items::pickup_nerubian_pheromone_t( p, options );

  // Set Actions
  if ( n == "pickup_cinderbee_orb" ) return new sets::pickup_cinderbee_orb_t( p, options );

  return nullptr;
}

double role_mult( player_t* player, const spell_data_t* s_data )
{
  double mult = 1.0;

  if ( auto vars = player->dbc->spell_desc_vars( s_data->id() ).desc_vars() )
  {
    std::cmatch m;
    std::regex get_var( R"(\$rolemult=\$(.*))" );  // find the $rolemult= variable
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
            player->sim->print_debug( "parsed role multiplier for spell '{}': {}", s_data->name_cstr(), mult );
            return mult;
          }
        }
      }
    }
  }

  return mult;
}

double role_mult( const special_effect_t& effect )
{
  return role_mult( effect.player, effect.driver() );
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

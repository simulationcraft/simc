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
#include "player/consumable.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "set_bonus.hpp"
#include "sim/cooldown.hpp"
#include "sim/real_ppm.hpp"
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

const spell_data_t* spell_from_spell_text( const special_effect_t& e, unsigned match_id )
{
  if ( auto desc = e.player->dbc->spell_text( e.spell_id ).desc() )
  {
    std::cmatch m;
    std::regex r( R"(\$\?a)" + std::to_string( match_id ) + R"(\[\$@spellname([0-9]+)\]\[\])" );
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

    add_stat( stat, amount );

    consumable_buff_t::start( s, v, d );
  }
};

custom_cb_t selector_food( unsigned id, bool highest, bool major = true )
{
  return [ = ]( special_effect_t& effect ) {
    effect.spell_id = id;

    auto coeff = effect.player->find_spell( food_coeff_spell_id );

    effect.stat_amount = coeff->effectN( 4 ).average( effect.player );
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
      auto _amt = coeff->effectN( primary_idx ).average( effect.player );
      if ( !major )
        _amt *= coeff->effectN( 1 ).base_value() * 0.1;

      buff->add_stat( effect.player->convert_hybrid_stat( stat ), _amt );
    }

    if ( primary_idx == 3 )
    {
      auto _amt = coeff->effectN( 8 ).average( effect.player );
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
      auto _amt = coeff->effectN( 4 ).average( effect.player );
      buff->add_stat( stat1, _amt );
    }
    else
    {
      auto _amt = coeff->effectN( 5 ).average( effect.player );
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
  if ( create_fallback_buffs( effect, { "flask_of_alchemical_chaos_vers", "flask_of_alchemical_chaos_mastery",
                                        "flask_of_alchemical_chaos_haste", "flask_of_alchemical_chaos_crit" } ) )
  {
    return;
  }

  struct flask_of_alchemical_chaos_buff_t : public consumable_buff_t<buff_t>
  {
    std::vector<std::pair<buff_t*, buff_t*>> buff_list;

    flask_of_alchemical_chaos_buff_t( const special_effect_t& e ) : consumable_buff_t( e.player, e.name(), e.driver() )
    {
      auto bonus = e.driver()->effectN( 5 ).average( e.item );
      auto penalty = -( e.driver()->effectN( 6 ).average( e.item ) );

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
  auto amount = effect.driver()->effectN( 1 ).average( effect.item );

  for ( auto s : secondary_ratings )
    if ( s != tempered_stat )
      buff->add_stat( s, amount );

  effect.custom_buff = buff;
}

// TODO: confirm debuff is 3%-5% (1% per rank), not 2%-5% as tooltip says
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

      // TODO: confirm debuff is 3%-5% (1% per rank), not 2%-5% as tooltip says
      mul = ( rank + base ) * 0.01;
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
  effect.discharge_amount = effect.driver()->effectN( 1 ).average( effect.player );

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace consumables

namespace enchants
{
void authority_of_radiant_power( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 448730 ) )
    ->set_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 2 ).average( effect.player ) );

  auto damage = create_proc_action<generic_proc_t>( effect.name(), effect, 448744 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.player );

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
  auto damage = create_proc_action<generic_proc_t>( "suffocating_darkness", effect, 449217 );
  damage->base_td = effect.driver()->effectN( 1 ).average( effect.player );

  effect.spell_id = effect.trigger()->id();  // rppm driver is the effect trigger

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

void secondary_weapon_enchant( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger()->effectN( 1 ).trigger() )
    ->set_stat_from_effect_type( A_MOD_RATING, effect.driver()->effectN( 1 ).average( effect.player ) );

  effect.spell_id = effect.trigger()->id();  // rppm driver is the effect trigger

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace enchants

namespace embellishments
{
}  // namespace embellishments

namespace items
{
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
  if ( create_fallback_buffs( effect, { "spymasters_web", "spymasters_report" } ) )
    return;

  unsigned equip_id = 444958;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Spymaster's Web missing equip effect" );

  auto equip_data = equip->driver();
  auto buff_data = equip->trigger();

  auto stacking_buff = create_buff<stat_buff_t>( effect.player, buff_data )
    ->set_stat_from_effect_type( A_MOD_STAT, equip_data->effectN( 1 ).average( effect.item ) );

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
    ->set_stat_from_effect_type( A_MOD_STAT, equip_data->effectN( 2 ).average( effect.item ) )
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
      auto damage_amount = effect.driver()->effectN( 1 ).average( effect.item );

      major = create_proc_action<generic_aoe_proc_t>( damage_name, effect, damage_spell );
      major->base_dd_min = major->base_dd_max = damage_amount;
      major->base_aoe_multiplier = effect.driver()->effectN( 5 ).percent();

      minor = create_proc_action<generic_aoe_proc_t>( damage_name + "_echo", effect, damage_spell );
      minor->base_dd_min = minor->base_dd_max = damage_amount * effect.driver()->effectN( 3 ).percent();
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
  if ( create_fallback_buffs( effect, { "aberrant_empowerment", "aberrant_spellforge" } ) )
    return;

  // sanity check equip effect exists
  unsigned equip_id = 445593;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Aberrant Spellforge missing equip effect" );

  auto empowered = spell_from_spell_text( effect, effect.player->spec_spell->id() );
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
    ->set_stat_from_effect_type( A_MOD_RATING, data->effectN( 2 ).average( effect.item ) );

  // proc damage action
  struct aberrant_shadows_t : public generic_proc_t
  {
    buff_t* stack;

    aberrant_shadows_t( const special_effect_t& e, const spell_data_t* data, buff_t* b )
      : generic_proc_t( e, "aberrant_shadows", 451866 ), stack( b )
    {
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e.item );

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
// 448519 decimation damage
// 448090 decimation damage to shield on crit
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
void sikrans_shadow_arsenal( special_effect_t& effect )
{
  unsigned equip_id = 445203;
  auto equip = find_special_effect( effect.player, equip_id );
  assert( equip && "Sikran's Shadow Arsenal missing equip effect" );

  auto data = equip->driver();

  struct sikrans_shadow_arsenal_t : public generic_proc_t
  {
    std::vector<std::pair<action_t*, buff_t*>> stance;

    sikrans_shadow_arsenal_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "sikrans_shadow_arsenal", e.driver() )
    {
      // stances are populated in order: flourish->decimation->barrage
      // TODO: confirm order is flourish->decimation->barrage

      // setup flourish
      auto f_dam = create_proc_action<generic_proc_t>( "surekian_flourish", e, 445434 );
      // TODO: confirm damage value is for entire dot and not per tick
      f_dam->base_td = data->effectN( 1 ).base_value() * ( f_dam->dot_duration / f_dam->base_tick_time );
      add_child( f_dam );

      auto f_stance = create_buff<stat_buff_t>( e.player, e.player->find_spell( 447962 ) )
        ->add_stat_from_effect( 1, data->effectN( 2 ).base_value() )
        ->add_stat_from_effect( 2, data->effectN( 3 ).base_value() );

      stance.emplace_back( f_dam, f_stance );

      // setup decimation
      auto d_dam = create_proc_action<generic_aoe_proc_t>( "surekian_brutality", e, 448519 );
      // TODO: confirm there is no standard +15% per target up to five
      d_dam->base_dd_min = d_dam->base_dd_max = data->effectN( 4 ).average( e.item );
      add_child( d_dam );

      auto d_shield = create_proc_action<generic_proc_t>( "surekian_decimation", e, 448090 );
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
      b_dam->base_dd_min = b_dam->base_dd_max = data->effectN( 6 ).average( e.item );
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
      const auto& option = e.player->thewarwithin_opts.sikrans_shadow_arsenal_stance;
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

  effect.execute_action = create_proc_action<sikrans_shadow_arsenal_t>( "sikrans_shadow_arsenal", effect, data );
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
        set_default_value( data->effectN( 2 ).average( e.item ) );
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
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e.item );

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
        base_dd_min = base_dd_max = data->effectN( 2 ).average( e.item );

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
      base_td = data->effectN( 1 ).average( e.item ) * ( base_tick_time / dot_duration );

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

    double buff_stat_stack_amount( const buff_stat_t& stat ) const
    {
      double val = std::max( 1.0, std::fabs( stat.amount ) );
      double stack = current_stack <= cap ? current_stack : cap + ( current_stack - cap ) * cap_mul;
      // TODO: confirm truncation happens on final amount, and not per stack amount
      return std::copysign( std::trunc( stack * val + 1e-3 ), stat.amount );
    }

    // bypass stat_buff_t::bump entirely and call buff_t::bump directly
    void bump( int stacks, double ) override
    {
      buff_t::bump( stacks );

      for ( auto& buff_stat : stats )
      {
        if ( buff_stat.check_func && !buff_stat.check_func( *this ) )
          continue;

        double delta = buff_stat_stack_amount( buff_stat ) - buff_stat.current_value;
        if ( delta > 0 )
          player->stat_gain( buff_stat.stat, delta, stat_gain, nullptr, buff_duration() > 0_ms );
        else if ( delta < 0 )
          player->stat_loss( buff_stat.stat, std::fabs( delta ), stat_gain, nullptr, buff_duration() > 0_ms );

        buff_stat.current_value += delta;
      }
    }
  };

  // setup stat buffs
  auto primary = create_buff<ovinax_stat_buff_t>( effect.player, effect.player->find_spell( 449578 ), data )
    ->set_stat_from_effect_type( A_MOD_STAT, data->effectN( 1 ).average( effect.item ) );

  static constexpr std::array<unsigned, 4> buff_ids = { 449595, 449594, 449581, 449593 };

  std::unordered_map<stat_e, buff_t*> secondaries;

  for ( size_t i = 0; i < secondary_ratings.size(); i++ )
  {
    auto stat_str = util::stat_type_abbrev( secondary_ratings[ i ] );
    auto spell = effect.player->find_spell( buff_ids[ i ] );
    auto name = fmt::format( "{}_{}", spell->name_cstr(), stat_str );

    auto buff = create_buff<ovinax_stat_buff_t>( effect.player, name, spell, data )
      ->set_stat_from_effect_type( A_MOD_RATING, data->effectN( 2 ).average( effect.item ) )
      ->set_name_reporting( stat_str );

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

  // TODO: add option to set starting stacks
  effect.player->register_precombat_begin( [ ticks, primary /*, secondaries*/ ]( player_t* p ) {
    primary->trigger( primary->max_stack() );
    make_event( *p->sim, p->rng().range( 1_ms, ticks->buff_period ), [ ticks ] { ticks->trigger(); } );
  } );

  effect.custom_buff = halt;
}

// 446209 driver
// 449946 counter
// 449947 jump task
// 449948 unknown, counter related?
// 449952 unknown, task?
// 449954 buff
// 449966 unknown, counter related?
// 450025 unknown, task?
// TODO: retest/redo everything
// TODO: add options to control task completion, placeholder 4-8s delay
void malfunctioning_ethereum_module( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "cryptic_instructions" } ) )
    return;

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 449954 ) )
    ->set_stat_from_effect_type( A_MOD_STAT, effect.driver()->effectN( 1 ).average( effect.item ) );

  auto counter = create_buff<buff_t>( effect.player, effect.trigger() )
    ->set_expire_at_max_stack( true )
    // TODO: add options to control task completion, placeholder 4-8s delay
    ->set_expire_callback( [ buff ]( buff_t*, int, timespan_t ) {
      make_event( *buff->sim, buff->rng().range( 4_s, 8_s ), [ buff ] { buff->trigger(); } );
    } );

  effect.custom_buff = counter;

  new dbc_proc_callback_t( effect.player, effect );
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

void abyssal_effigy( special_effect_t& effect )
{
  unsigned coeff_id = 443128;
  auto coeff = find_special_effect( effect.player, coeff_id );
  assert( coeff && "Abyssal Effigy missing coefficient effect" );

  struct abyssal_gluttony_t : public generic_proc_t
  {
    cooldown_t* item_cd;
    action_t* heal;
    timespan_t cdr;
    double heal_speed;

    abyssal_gluttony_t( const special_effect_t& e, const spell_data_t* data )
      : generic_proc_t( e, "abyssal_gluttony", e.driver() ),
        item_cd( e.player->get_cooldown( e.cooldown_name() ) ),
        cdr( timespan_t::from_seconds( data->effectN( 4 ).base_value() ) ),
        heal_speed( e.trigger()->missile_speed() )
    {
      base_dd_min = base_dd_max = data->effectN( 1 ).average( e.item );

      heal = create_proc_action<generic_heal_t>( "abyssal_gluttony_heal", e, "abyssal_glutton_heal",
                                                 e.trigger()->effectN( 2 ).trigger() );
      heal->name_str_reporting = "abyssal_gluttony";
      heal->base_dd_min = heal->base_dd_max = data->effectN( 2 ).average( e.item );
    }

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
  };

  effect.execute_action = create_proc_action<abyssal_gluttony_t>( "abyssal_gluttony", effect, coeff->driver() );
}

// Sigil of Algari Concordance
// Might have variations based on the players Specilization.
// Testing on DPS Death Knight showed only a single spawnable pet.
// 443378 Driver
// 452310 Summon Spell (DPS)
// 452335 Thunder Bolt - Damage Spell DPS
// 452445 Thundering Bolt - One Time Spell DPS
// 452496 Summon Spell (Tank)
// 452545 Mighty Smash - Damage Spell Tank
// 452514 Earthen Ire - One Time Spell Tank
// 452890 Earthen Ire - Ticking Damage
// 452325, 452457, and 452498 Periodic Triggers for pet actions
void sigil_of_algari_concordance( special_effect_t& e )
{
  struct sigil_of_algari_concordance_pet_t : public pet_t
  {
    action_t* repeating_action;
    action_t* one_time_action;
    timespan_t period;
    double ticks;

    sigil_of_algari_concordance_pet_t( util::string_view name, const special_effect_t& e )
      : pet_t( e.player->sim, e.player, name, true, true ),
        repeating_action( nullptr ),
        one_time_action( nullptr ),
        period( e.player->find_spell( 452325 )->effectN( 1 ).period() ),
        ticks( ( e.player->find_spell( 452310 )->duration() - period * 2 ) / period )
    {
    }

    resource_e primary_resource() const override
    {
      return RESOURCE_NONE;
    }

    void arise() override
    {
      pet_t::arise();
      // TODO: Sometimes the one time action is delayed until the 2nd or 3rd tick, seems pretty random when it happens
      // Need to explore deeper to see whats happening
      one_time_action->execute();
      make_event( *sim, period, [ this ]() {
        repeating_action->execute();
        make_repeating_event(
            *sim, period, [ this ]() { repeating_action->execute(); }, as<int>( ticks ) );
      } );
    }
  };

  struct thunder_bolt_silvervein_t : public spell_t
  {
    thunder_bolt_silvervein_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : spell_t( name, p, p->find_spell( 452335 ) )
    {
      background         = true;
      name_str_reporting = "thunder_bolt";
      // TODO: Double check the effect mapping is correct for the abilities. Nothing is referenced in tooltips making it
      // a guessing game.
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e.item );
      auto proxy                = a;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }
  };

  struct thundering_bolt_t : public spell_t
  {
    thundering_bolt_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : spell_t( name, p, p->find_spell( 452445 ) )
    {
      aoe        = -1;
      background = true;
      // TODO: Double check the effect mapping is correct for the abilities. Nothing is referenced in tooltips making it
      // a guessing game.
      base_dd_min = base_dd_max = e.driver()->effectN( 4 ).average( e.item );
      auto proxy                = a;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }
  };

  struct silvervein_pet_t : public sigil_of_algari_concordance_pet_t
  {
    action_t* action;
    const special_effect_t& effect;
    silvervein_pet_t( const special_effect_t& e, action_t* a )
      : sigil_of_algari_concordance_pet_t( "silvervein", e ), action( a ), effect( e )
    {
    }

    void create_actions() override
    {
      sigil_of_algari_concordance_pet_t::create_actions();
      repeating_action = new thunder_bolt_silvervein_t( "thunder_bolt_silvervein", this, effect, action );
      one_time_action  = new thundering_bolt_t( "thundering_bolt", this, effect, action );
    }
  };

  struct mighty_smash_t : public spell_t
  {
    mighty_smash_t( util::string_view name, pet_t* p, const special_effect_t& e, action_t* a )
      : spell_t( name, p, p->find_spell( 452545 ) )
    {
      aoe        = -1;
      background = true;
      // TODO: Double check the effect mapping is correct for the abilities. Nothing is referenced in tooltips making it
      // a guessing game.
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e.item );
      auto proxy                = a;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }
  };

  struct earthen_ire_damage_t : public spell_t
  {
    earthen_ire_damage_t( const special_effect_t& e )
      : spell_t( "earthen_ire", e.player, e.player->find_spell( 452890 ) )
    {
      aoe        = -1;
      background = true;
      // TODO: Double check the effect mapping is correct for the abilities. Nothing is referenced in tooltips making it
      // a guessing game.
      base_dd_min = base_dd_max = e.driver()->effectN( 4 ).average( e.item );
    }
  };

  struct earthen_ire_buff_t : public spell_t
  {
    buff_t* buff;
    earthen_ire_buff_t( const special_effect_t& e, buff_t* buff )
      : spell_t( "earthen_ire", e.player, e.player->find_spell( 452518 ) ), buff( buff )
    {
      background = true;
    }

    void execute() override
    {
      buff->trigger();
    }
  };

  struct boulderbane_pet_t : public sigil_of_algari_concordance_pet_t
  {
    action_t* action;
    const special_effect_t& effect;
    boulderbane_pet_t( const special_effect_t& e, action_t* a, action_t* ire_buff )
      : sigil_of_algari_concordance_pet_t( "boulderbane", e ), action( a ), effect( e )
    {
      one_time_action = ire_buff;
    }

    void create_actions() override
    {
      sigil_of_algari_concordance_pet_t::create_actions();
      repeating_action = new mighty_smash_t( "mighty_smash", this, effect, action );
    }
  };

  struct sigil_of_algari_concordance_t : public spell_t
  {
    spawner::pet_spawner_t<silvervein_pet_t> silvervein_spawner;
    spawner::pet_spawner_t<boulderbane_pet_t> boulderbane_spawner;
    bool silvervein;
    bool boulderbane;

    sigil_of_algari_concordance_t( const special_effect_t& e, action_t* ire_buff )
      : spell_t( "sigil_of_algari_concordance", e.player, e.driver() ),
        silvervein_spawner( "silvervein", e.player,
                            [ &e, this ]( player_t* ) { return new silvervein_pet_t( e, this ); } ),
        boulderbane_spawner(
            "boulderbane", e.player,
            [ &e, this, ire_buff ]( player_t* ) { return new boulderbane_pet_t( e, this, ire_buff ); } ),
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
          silvervein_spawner.set_default_duration( e.player->find_spell( 452310 )->duration() );
          break;
        case WARRIOR_PROTECTION:
        case PALADIN_PROTECTION:
        case DEATH_KNIGHT_BLOOD:
        case MONK_BREWMASTER:
        case DRUID_GUARDIAN:
        case DEMON_HUNTER_VENGEANCE:
          boulderbane = true;
          boulderbane_spawner.set_default_duration( e.player->find_spell( 452496 )->duration() );
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
          silvervein_spawner.set_default_duration( e.player->find_spell( 452310 )->duration() );
          break;
      }

      background = true;
    }

    void execute() override
    {
      spell_t::execute();
      if ( boulderbane )
      {
        boulderbane_spawner.spawn();
      }
      else if ( silvervein )
      {
        silvervein_spawner.spawn();
      }
    }
  };

  auto earthen_ire_damage = create_proc_action<earthen_ire_damage_t>( "earthen_ire", e );
  // TODO: Figure out why the buff isnt being applied to the player? still executes earthen_ire_damage once though...?
  auto earthen_ire_buff =
      create_buff<buff_t>( e.player, e.player->find_spell( 452514 ) )
          ->set_tick_callback( [ earthen_ire_damage ]( buff_t*, int, timespan_t ) { earthen_ire_damage->execute(); } );
  auto earthen_ire_buff_action = create_proc_action<earthen_ire_buff_t>( "earthen_ire", e, earthen_ire_buff );

  e.execute_action =
      create_proc_action<sigil_of_algari_concordance_t>( "sigil_of_algari_concordance", e, earthen_ire_buff_action );
  new dbc_proc_callback_t( e.player, e );
}

// Skarmorak Shard
// 443407 Main Buff & Driver
// 449792 Stacking Buff
void skarmorak_shard( special_effect_t& e )
{
  auto main_buff = create_buff<stat_buff_t>( e.player, e.driver() )
                       ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 1 ).average( e.item ) );

  auto on_kill_buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 449792 ) )
                          ->add_stat_from_effect_type( A_MOD_RATING, e.player->find_spell( 449792 )->effectN( 1 ).average( e.item ) );

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
                  ->add_stat_from_effect_type( A_MOD_RATING, e.driver()->effectN( 2 ).average( e.item ) );

  auto damage         = create_proc_action<generic_aoe_proc_t>( "void_pulse", e, 450960 );
  damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 1 ).average( e.item );
  damage->split_aoe_damage                  = true;

  e.custom_buff    = buff;
  e.execute_action = damage;
  new dbc_proc_callback_t( e.player, e );
}

// Ravenous Honey Buzzer
// 448904 Driver
// 448909 Damage
void ravenous_honey_buzzer( special_effect_t& e )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "ravenous_honey_buzzer", e, 448909 );
  damage -> split_aoe_damage = true;

  e.execute_action = damage;
}

// Overlocked Gear-a-rang Launcher
// 443411 Use Driver
// 446764 Equip Driver
// 446811 Use Damage
// 449828 Equip Damage
// 450453 Equip Buff
void overclocked_geararang_launcher( special_effect_t& e )
{
  struct overclocked_strike_t : public generic_proc_t
  {
    buff_t* buff;
    overclocked_strike_t( const special_effect_t& e, buff_t* buff )
      : generic_proc_t( e, "overclocked_strike", e.player->find_spell( 449828 ) ), buff( buff )
    {
      background  = true;
      base_dd_min = base_dd_max = e.player->find_spell( 446764 )->effectN( 2 ).average( e.item );
    }

    void execute() override
    {
      // Appears to consume the buff before executing the action
      buff->expire();
      generic_proc_t::execute();
    }
  };

  auto damage_buff_spell     = e.player->find_spell( 450453 );
  auto overclock_buff        = create_buff<buff_t>( e.player, damage_buff_spell );
  auto damage_action         = create_proc_action<overclocked_strike_t>( "overclocked_strike", e, overclock_buff );

  auto damage            = new special_effect_t( e.player );
  damage->name_str       = "overclocked_strike_proc";
  damage->item           = e.item;
  damage->spell_id       = damage_buff_spell->id();
  damage->execute_action = damage_action;
  e.player->special_effects.push_back( damage );

  auto damage_cb = new dbc_proc_callback_t( e.player, *damage );
  damage_cb->initialize();
  damage_cb->deactivate();

  overclock_buff->set_stack_change_callback( [ damage_cb ]( buff_t*, int, int new_ ) {
    if ( new_ )
    {
      damage_cb->activate();
    }
    else
    {
      damage_cb->deactivate();
    }
  } );

  auto equip_driver = e.player->find_spell( 446764 );

  auto overclock         = new special_effect_t( e.player );
  overclock->name_str    = "overclock";
  overclock->item        = e.item;
  overclock->spell_id    = equip_driver->id();
  overclock->custom_buff = overclock_buff;
  e.player->special_effects.push_back( overclock );

  auto overclock_cb = new dbc_proc_callback_t( e.player, *overclock );
  overclock_cb->initialize();
  overclock_cb->activate();

  // Might be worth converting to a poper ground effect event later. For now, this is close enough.
  auto use_damage  = create_proc_action<generic_aoe_proc_t>( "geararang_serration", e, 446811 );
  e.execute_action = use_damage;
}

// Remnant of Darkness
// 443530 Driver
// 451369 Buff
// 452032 Damage
// 451602 Transform Buff
void remnant_of_darkness( special_effect_t& e )
{
  auto damage         = create_proc_action<generic_aoe_proc_t>( "dark_swipe", e, 452032 );
  damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 2 ).average( e.item );

  auto transform_buff = create_buff<buff_t>( e.player, e.player->find_spell( 451602 ) )
                            ->set_tick_callback( [ damage ]( buff_t*, int, timespan_t ) { damage->execute(); } );

  auto stat_buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 451369 ) )
                       ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e.item ) )
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
    dark_oration_t( const special_effect_t& e )
      : generic_aoe_proc_t( e, "dark_oration", e.player->find_spell( 451015 ) ), mult( 0 )
    {
      background  = true;
      base_dd_min = base_dd_max = e.player->find_spell( 446787 )->effectN( 2 ).average( e.item );
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
    dark_oration_tick_t( const special_effect_t& e, buff_t* buff, action_t* damage )
      : generic_proc_t( e, "dark_oration", e.driver() ), buff( buff ), damage( damage )
    {
      background  = true;
      tick_action = damage;
    }

    void execute() override
    {
      debug_cast<dark_oration_t*>( damage )->mult = buff->stack() * 0.25;
      buff->expire();
      generic_proc_t::execute();
    }
  };

  auto equip_driver = e.player->find_spell( 446787 );
  auto buff         = create_buff<stat_buff_t>( e.player, e.player->find_spell( 451011 ) )
                  ->add_stat_from_effect_type( A_MOD_STAT, equip_driver->effectN( 1 ).average( e.item ) )
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

  auto damage        = create_proc_action<dark_oration_t>( "dark_oration", e );
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
      damage->base_td        = e.player->find_spell( 443541 )->effectN( 2 ).average( e.item );
      missile                = create_proc_action<generic_proc_t>( "spiderfling", e, e.player->find_spell( 452227 ) );
      missile->impact_action = damage;
    }

    void execute( action_t*, action_state_t* s ) override
    {
      missile->execute_on_target( s->target );
      buff->decrement();
    }
  };

  auto spiderling_buff = create_buff<buff_t>( e.player, e.player->find_spell( 452226 ) );

  auto spiderling      = new special_effect_t( e.player );
  spiderling->name_str = "spiderling";
  spiderling->item     = e.item;
  spiderling->spell_id = 452226;
  e.player->special_effects.push_back( spiderling );

  auto spiderling_cb = new spiderfling_cb_t( *spiderling, spiderling_buff );
  spiderling_cb->initialize();
  spiderling_cb->deactivate();

  spiderling_buff->set_stack_change_callback( [ spiderling_cb ]( buff_t*, int, int new_ ) {
    if ( new_ == 1 )
    {
      spiderling_cb->activate();
    }
    if ( new_ == 0 )
    {
      spiderling_cb->deactivate();
    }
  } );

  auto buff = create_buff<stat_buff_t>( e.player, e.player->find_spell( 452146 ) )
                  ->add_stat_from_effect_type( A_MOD_STAT, e.driver()->effectN( 1 ).average( e.item ) )
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

// Weapons
// 444135 driver
// 448862 dot (trigger)
// TODO: confirm effect value is for the entire dot and not per tick
void void_reapers_claw( special_effect_t& effect )
{
  effect.duration_ = effect.trigger()->duration();
  effect.tick = effect.trigger()->effectN( 1 ).period();
  // TODO: confirm effect value is for the entire dot and not per tick
  effect.discharge_amount = effect.driver()->effectN( 1 ).average( effect.item ) * effect.tick / effect.duration_;

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
      stat_amt( e.driver()->effectN( 2 ).average( e.item ) )
    {
      // TODO: damage spell data is shadowfrost and not cosmic
      damage = create_proc_action<generic_proc_t>( "fated_pain", e, 443585 );
      damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 1 ).average( e.item );
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
      base_td = e.driver()->effectN( 1 ).average( e.item );
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
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e.item );
    }
  };

  // create on-next melee damage
  auto strike = create_proc_action<generic_proc_t>( "befouling_strike", effect, 442280 );
  strike->base_dd_min = strike->base_dd_max = effect.driver()->effectN( 2 ).average( effect.item );

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
  dot->base_td = effect.driver()->effectN( 1 ).average( effect.item ) * dot->base_tick_time / dot->dot_duration;

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
}  // namespace items

namespace sets
{
}  // namespace sets

void register_special_effects()
{
  // NOTE: use unique_gear:: namespace for static consumables so we don't activate them with enable_all_item_effects
  // Food
  unique_gear::register_special_effect( 457302, consumables::selector_food( 457172, true ) );  // the sushi special
  unique_gear::register_special_effect( 455960, consumables::selector_food( 457172, false ) );  // everything stew
  unique_gear::register_special_effect( 454149, consumables::selector_food( 457169, true ) );  // beledar's bounty, empress' farewell, jester's board, outsider's provisions
  unique_gear::register_special_effect( 457282, consumables::selector_food( 457170, false, false ) );  // pan seared mycobloom, hallowfall chili, coreway kabob, flash fire fillet
  unique_gear::register_special_effect( 454087, consumables::selector_food( 457171, false, false ) );  // unseasoned field steak, roasted mycobloom, spongey scramble, skewered fillet, simple stew
  unique_gear::register_special_effect( 457283, consumables::primary_food( 457172, STAT_STR_AGI_INT, 2 ) );  // feast of the divine day
  unique_gear::register_special_effect( 457285, consumables::primary_food( 457172, STAT_STR_AGI_INT, 2 ) );  // feast of the midnight masquerade
  unique_gear::register_special_effect( 457294, consumables::primary_food( 457124, STAT_STRENGTH ) );  // sizzling honey roast
  unique_gear::register_special_effect( 457136, consumables::primary_food( 457124, STAT_AGILITY ) );  // mycobloom risotto
  unique_gear::register_special_effect( 457295, consumables::primary_food( 457124, STAT_INTELLECT ) );  // stuffed cave peppers
  unique_gear::register_special_effect( 457296, consumables::primary_food( 457124, STAT_STAMINA, 7 ) );  // angler's delight
  unique_gear::register_special_effect( 457298, consumables::primary_food( 457124, STAT_STRENGTH, 3, false ) );  // meat and potatoes
  unique_gear::register_special_effect( 457297, consumables::primary_food( 457124, STAT_AGILITY, 3, false ) );  // rib stickers
  unique_gear::register_special_effect( 457299, consumables::primary_food( 457124, STAT_INTELLECT, 3, false ) );  // sweet and sour meatballs
  unique_gear::register_special_effect( 457300, consumables::primary_food( 457124, STAT_STAMINA, 7, false ) );  // tender twilight jerky
  unique_gear::register_special_effect( 457286, consumables::secondary_food( 457049, STAT_HASTE_RATING ) );  // zesty nibblers
  unique_gear::register_special_effect( 456968, consumables::secondary_food( 457049, STAT_CRIT_RATING ) );  // fiery fish sticks
  unique_gear::register_special_effect( 457287, consumables::secondary_food( 457049, STAT_VERSATILITY_RATING ) );  // ginger glazed fillet
  unique_gear::register_special_effect( 457288, consumables::secondary_food( 457049, STAT_MASTERY_RATING ) );  // salty dog
  unique_gear::register_special_effect( 457289, consumables::secondary_food( 457049, STAT_HASTE_RATING, STAT_CRIT_RATING ) );  // deepfin patty
  unique_gear::register_special_effect( 457290, consumables::secondary_food( 457049, STAT_HASTE_RATING, STAT_VERSATILITY_RATING ) );  // sweet and spicy soup
  unique_gear::register_special_effect( 457291, consumables::secondary_food( 457049, STAT_CRIT_RATING, STAT_VERSATILITY_RATING ) );  // fish and chips
  unique_gear::register_special_effect( 457292, consumables::secondary_food( 457049, STAT_MASTERY_RATING, STAT_CRIT_RATING ) );  // salt baked seafood
  unique_gear::register_special_effect( 457293, consumables::secondary_food( 457049, STAT_MASTERY_RATING, STAT_VERSATILITY_RATING ) );  // marinated tenderloins
  unique_gear::register_special_effect( 457301, consumables::secondary_food( 457049, STAT_MASTERY_RATING, STAT_HASTE_RATING ) );  // chippy tea

  // Flasks
  unique_gear::register_special_effect( 432021, consumables::flask_of_alchemical_chaos, true );

  // Potions
  unique_gear::register_special_effect( 431932, consumables::tempered_potion );
  unique_gear::register_special_effect( 431914, consumables::potion_of_unwavering_focus );

  // Oils
  register_special_effect( { 451904, 451909, 451912 }, consumables::oil_of_deep_toxins );

  // Enchants
  register_special_effect( { 448710, 448714, 448716 }, enchants::authority_of_radiant_power );
  register_special_effect( { 449221, 449223, 449222 }, enchants::authority_of_the_depths );
  register_special_effect( { 449055, 449056, 449059,                                          // council's guile (crit)
                             449095, 449096, 449097,                                          // stormrider's fury (haste)
                             449112, 449113, 449114,                                          // stonebound artistry (mastery)
                             449120, 449118, 449117 }, enchants::secondary_weapon_enchant );  // oathsworn tenacity (vers)


  // Embellishments

  // Trinkets
  register_special_effect( 444959, items::spymasters_web, true );
  register_special_effect( 444958, DISABLED_EFFECT );  // spymaster's web
  register_special_effect( 444067, items::void_reapers_chime );
  register_special_effect( 445619, items::aberrant_spellforge, true );
  register_special_effect( 445593, DISABLED_EFFECT );  // aberrant spellforge
  register_special_effect( 447970, items::sikrans_shadow_arsenal );
  register_special_effect( 445203, DISABLED_EFFECT );  // sikran's shadow arsenal
  register_special_effect( 444301, items::swarmlords_authority );
  register_special_effect( 444292, DISABLED_EFFECT );  // swarmlord's authority
  register_special_effect( 444264, items::foul_behemoths_chelicera );
  register_special_effect( 445560, items::ovinaxs_mercurial_egg );
  register_special_effect( 445066, DISABLED_EFFECT );  // ovinax's mercurial egg
  register_special_effect( 446209, items::malfunctioning_ethereum_module, true );
  register_special_effect( 443124, items::abyssal_effigy );
  register_special_effect( 443128, DISABLED_EFFECT );  // abyssal effigy
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
  // Weapons
  register_special_effect( 444135, items::void_reapers_claw );
  register_special_effect( 443384, items::fateweaved_needle );
  register_special_effect( 442205, items::befoulers_syringe );
  // Armor
  register_special_effect( 457815, items::seal_of_the_poisoned_pact );
  register_special_effect( 457918, DISABLED_EFFECT );  // seal of the poisoned pact
  // Sets
  register_special_effect( 444166, DISABLED_EFFECT );  // kye'veza's cruel implements
}

void register_target_data_initializers( sim_t& )
{
}

void register_hotfixes()
{
}
}  // namespace unique_gear::thewarwithin

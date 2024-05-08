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
    std::regex r( "\\$\\?a" + std::to_string( match_id ) + "\\[\\$@spellname([0-9]+)\\]\\[\\]" );
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
}  // namespace consumables

namespace enchants
{
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

    void execute( action_t* a, action_state_t* s ) override
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
          // TODO: are these immediate, delayes, or staggered?
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
  auto silence_dur = effect.player->find_spell( 452350 )->duration();

  // create buffs
  auto empowerment = create_buff<buff_t>( effect.player, effect.player->find_spell( 451895 ) );

  auto stack = create_buff<buff_t>( effect.player, effect.driver() )->set_cooldown( 0_ms )
    ->set_default_value( data->effectN( 3 ).percent() );

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
  effect.player->callbacks.register_callback_trigger_function( equip->spell_id,
      dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ empowered, empowerment ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* ) {
        return a->data().id() == empowered->id() && empowerment->check();
      } );

  // TODO: confirm empowerment is not consumed by procs
  effect.player->callbacks.register_callback_execute_function( equip->spell_id,
      [ damage, empowerment ]( const dbc_proc_callback_t*, action_t*, const action_state_t* s ) {
        damage->execute_on_target( s->target );
        empowerment->expire( damage );
      } );

  new dbc_proc_callback_t( effect.player, *equip );

  // setup on-use effect
  /* TODO: determine when silence applies
  auto silence = [ dur = effect.player->find_spell( 452350 )->duration() ]( player_t* p ) {
    p->buffs.stunned->increment();
    p->stun();

    make_event( *p->sim, dur, [ p ] { p->buffs.stunned->decrement(); } );
  };
  */
  stack->set_stack_change_callback( [ haste ]( buff_t* b, int, int ) {
    if ( b->at_max_stacks() )
      haste->trigger();
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

// Weapons
// 444135 driver
// 448862 dot (trigger)
void void_reapers_claw( special_effect_t& effect )
{
  effect.duration_ = effect.trigger()->duration();
  effect.tick = effect.trigger()->effectN( 1 ).period();
  // TODO: confirm effect value is for the entire dot and not per tick
  effect.discharge_amount = effect.driver()->effectN( 1 ).average( effect.item ) * effect.tick / effect.duration_;

  new dbc_proc_callback_t( effect.player, effect );
}
// Armor
}  // namespace items

namespace sets
{
}  // namespace sets

void register_special_effects()
{
  // Food
  // Phials
  // Potions

  // Enchants

  // Embellishments

  // Trinkets
  register_special_effect( 444959, items::spymasters_web, true );
  register_special_effect( 444958, DISABLED_EFFECT );  // spymaster's web
  register_special_effect( 444067, items::void_reapers_chime );
  register_special_effect( 445619, items::aberrant_spellforge );
  register_special_effect( 445593, DISABLED_EFFECT );  // aberrant spellforge
  // Weapons
  register_special_effect( 444135, items::void_reapers_claw );
  // Armor

  // Sets
}

void register_target_data_initializers( sim_t& sim )
{

}

void register_hotfixes()
{

}
}  // namespace unique_gear::thewarwithin

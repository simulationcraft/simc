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
      [ id = empowered->id(), empowerment ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* ) {
        return a->data().id() == id && empowerment->check();
      } );

  // TODO: confirm empowerment is not consumed by procs
  effect.player->callbacks.register_callback_execute_function( equip->spell_id,
      [ damage, empowerment ]( const dbc_proc_callback_t*, action_t* a, const action_state_t* s ) {
        damage->execute_on_target( s->target );
        empowerment->expire( a );
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
      auto f_dam = create_proc_action<generic_proc_t>( "surekian_flourish", e, e.player->find_spell( 445434 ) );
      // TODO: confirm damage value is for entire dot and not per tick
      f_dam->base_td = data->effectN( 1 ).base_value() * ( f_dam->dot_duration / f_dam->base_tick_time );
      add_child( f_dam );

      auto f_stance = create_buff<stat_buff_t>( e.player, e.player->find_spell( 447962 ) )
        ->add_stat_from_effect( 1, data->effectN( 2 ).base_value() )
        ->add_stat_from_effect( 2, data->effectN( 3 ).base_value() );

      stance.emplace_back( f_dam, f_stance );

      // setup decimation
      auto d_dam = create_proc_action<generic_aoe_proc_t>( "surekian_brutality", e, e.player->find_spell( 448519 ) );
      // TODO: confirm there is no standard +15% per target up to five
      d_dam->base_dd_min = d_dam->base_dd_max = data->effectN( 4 ).average( e.item );
      add_child( d_dam );

      auto d_shield = create_proc_action<generic_proc_t>( "surekian_decimation", e, e.player->find_spell( 448090 ) );
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
      auto b_dam = create_proc_action<generic_aoe_proc_t>( "surekian_barrage", e, e.player->find_spell( 445475 ) );
      // TODO: confirm damage isn't split and has no diminishing returns
      b_dam->split_aoe_damage = false;
      b_dam->base_dd_min = b_dam->base_dd_max = data->effectN( 6 ).average( e.item );
      add_child( b_dam );

      auto b_speed = create_buff<buff_t>( e.player, e.player->find_spell( 448436 ) );
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

      e.player->register_precombat_begin( [ this ]( player_t* p ) {
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
  // TODO: confirm equip cycle restarts on combat
  // TODO: determine how scarabs select target
  effect.player->register_combat_begin( [ scarab, period = data->effectN( 3 ).period() ]( player_t* p ) {
    make_repeating_event( *p->sim, period, [ scarab ] { scarab->execute(); } );
  } );

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
    struct tasty_juices_t : public proc_heal_t
    {
      buff_t* buff;

      tasty_juices_t( const special_effect_t& e, const spell_data_t* data )
        : proc_heal_t( "tasty_juices", e.player, e.player->find_spell( 446805 ) )
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
        proc_heal_t::impact( s );

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
      damage = create_proc_action<generic_proc_t>( "fated_pain", e, e.player->find_spell( 443585 ) );
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
  auto strike = create_proc_action<generic_proc_t>( "befouling_strike", effect, effect.player->find_spell( 442280 ) );
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
  register_special_effect( 447970, items::sikrans_shadow_arsenal );
  register_special_effect( 445203, DISABLED_EFFECT );  // sikran's shadow arsenal
  register_special_effect( 444301, items::swarmlords_authority );
  register_special_effect( 444292, DISABLED_EFFECT );  // swarmlord's authority
  register_special_effect( 444264, items::foul_behemoths_chelicera );
  // Weapons
  register_special_effect( 444135, items::void_reapers_claw );
  register_special_effect( 443384, items::fateweaved_needle );
  register_special_effect( 442205, items::befoulers_syringe );
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

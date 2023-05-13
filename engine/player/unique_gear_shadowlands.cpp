// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_shadowlands.hpp"

#include "sim/sim.hpp"

#include "sim/cooldown.hpp"
#include "player/action_priority_list.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "buff/buff.hpp"
#include "action/dot.hpp"
#include "item/item.hpp"
#include "class_modules/monk/sc_monk.hpp"

#include "actor_target_data.hpp"
#include "darkmoon_deck.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"

#include "report/decorators.hpp"
#include <dbc/covenant_data.hpp>

namespace unique_gear::shadowlands
{
struct shadowlands_aoe_proc_t : public generic_aoe_proc_t
{
  shadowlands_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, const spell_data_t* s,
                  bool aoe_damage_increase_ = false ) :
    generic_aoe_proc_t( effect, name, s, aoe_damage_increase_ )
  {
    // Default still seems to be 6, however shadowlands seems to indicate the maximum
    // number (slightly confusingly) in the tooltip data, which is referencable directly
    // from spell data
    max_scaling_targets = 5;
  }

  shadowlands_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, unsigned spell_id,
                       bool aoe_damage_increase_ = false ) :
    generic_aoe_proc_t( effect, name, spell_id, aoe_damage_increase_ )
  {
    max_scaling_targets = 5;
  }
};

// Enchants & Consumable buffs affected by 'Exacting Preparations' soulbind
struct SL_buff_t : public buff_t
{
  SL_buff_t( actor_pair_t q, util::string_view n, const spell_data_t* s ) : buff_t( q, n, s ) {}

  buff_t* set_default_value_from_effect_type( effect_subtype_t a, property_type_t p = P_MAX, double m = 0.0,
                                              effect_type_t e = E_APPLY_AURA ) override
  {
    buff_t::set_default_value_from_effect_type( a, p, m, e );

    auto cov_player = player->is_enemy() ? source : player;
    auto ep  = cov_player->find_soulbind_spell( "Exacting Preparation" );
    if ( ep->ok() )
      apply_affecting_aura( ep );

    return this;
  }
};

// Enchants & Consumable procs affected by 'Exacting Preparations' soulbind
struct SL_proc_spell_t : public proc_spell_t
{
  SL_proc_spell_t( const special_effect_t& e ) : proc_spell_t( e ) {}

  SL_proc_spell_t( util::string_view n, player_t* p, const spell_data_t* s ) : proc_spell_t( n, p, s ) {}

  void init() override
  {
    proc_spell_t::init();

    auto ep = player->find_soulbind_spell( "Exacting Preparation" );
    for ( const auto& eff : ep->effects() )
    {
      if ( data().affected_by_label( eff ) )
      {
        base_multiplier *= 1.0 + eff.percent();
      }
    }
  }
};

namespace consumables
{
void smothered_shank( special_effect_t& effect )
{
  struct smothered_shank_buff_t : public buff_t
  {
    action_t* belch;

    smothered_shank_buff_t( const special_effect_t& e ) : buff_t( e.player, "smothered_shank", e.driver() )
    {
      belch = create_proc_action<SL_proc_spell_t>( "pungent_belch", e );

      set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        belch->set_target( player->target );
        belch->schedule_execute();
      } );
    }
  };

  effect.name_str = "pungent_belch";
  effect.custom_buff = make_buff<smothered_shank_buff_t>( effect );
  effect.disable_action();
}

void surprisingly_palatable_feast( special_effect_t& effect )
{
  init_feast( effect,
              {{STAT_STRENGTH, 327701}, {STAT_STAMINA, 327702}, {STAT_INTELLECT, 327704}, {STAT_AGILITY, 327705}} );
}

void feast_of_gluttonous_hedonism( special_effect_t& effect )
{
  init_feast( effect,
              {{STAT_STRENGTH, 327706}, {STAT_STAMINA, 327707}, {STAT_INTELLECT, 327708}, {STAT_AGILITY, 327709}} );
}

// For things that require check for oils, the effect must be passed as first parameter of the constructor, but this
// parameter is not passed onto the base constructor
template <typename Base>
struct SL_potion_proc_t : public Base
{
  using base_t = SL_potion_proc_t<Base>;

  bool shadowcore;
  bool embalmers;

  template <typename... Args>
  SL_potion_proc_t( const special_effect_t& e, Args&&... args ) : Base( std::forward<Args>( args )... )
  {
    shadowcore = unique_gear::find_special_effect( e.player, 321382 );
    embalmers  = unique_gear::find_special_effect( e.player, 321393 );
  }
};

// proc potion initialization helper
template <typename T>
void init_proc_potion( special_effect_t& effect, util::string_view name )
{
  auto potion            = new special_effect_t( effect.player );
  potion->type           = SPECIAL_EFFECT_EQUIP;
  potion->spell_id       = effect.spell_id;
  potion->cooldown_      = 0_ms;
  potion->execute_action = create_proc_action<T>( name, effect );
  effect.player->special_effects.push_back( potion );

  auto proc = new dbc_proc_callback_t( effect.player, *potion );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
    ->set_cooldown( 0_ms )
    ->set_chance( 1.0 )
    ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
      if ( new_ )
        proc->activate();
      else
        proc->deactivate();
    } );
}

void potion_of_deathly_fixation( special_effect_t& effect )
{
  struct deathly_eruption_t : public SL_potion_proc_t<proc_spell_t>
  {
    deathly_eruption_t( const special_effect_t& e )
      : base_t( e, "deathly_eruption", e.player, e.player->find_spell( 322256 ) )
    {
      if ( shadowcore )
        base_multiplier *= 1.0 + e.driver()->effectN( 2 ).percent();
    }
  };

  struct deathly_fixation_t : public proc_spell_t
  {
    action_t* eruption;

    deathly_fixation_t( const special_effect_t& e )
      : proc_spell_t( "deathly_fixation", e.player, e.player->find_spell( 322253 ) ),
        eruption( create_proc_action<deathly_eruption_t>( "deathly_eruption", e ) )
    {
      add_child( eruption );
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      auto d = get_dot( s->target );

      if ( d->at_max_stacks() )
      {
        eruption->set_target( s->target );
        eruption->schedule_execute();
        d->cancel();
      }
    }
  };

  init_proc_potion<deathly_fixation_t>( effect, "potion_of_deathly_fixation" );
}

void potion_of_empowered_exorcisms( special_effect_t& effect )
{
  struct empowered_exorcisms_t : public SL_potion_proc_t<proc_spell_t>
  {
    empowered_exorcisms_t( const special_effect_t& e )
      : base_t( e, "empowered_exorcisms", e.player, e.player->find_spell( 322015 ) )
    {
      if ( shadowcore )
      {
        base_multiplier *= 1.0 + e.driver()->effectN( 2 ).percent();
        radius *= 1.0 + e.driver()->effectN( 2 ).percent();
      }

      reduced_aoe_targets = 1.0;
    }
  };

  init_proc_potion<empowered_exorcisms_t>( effect, "potion_of_empowered_exorcisms" );
}

void potion_of_phantom_fire( special_effect_t& effect )
{
  struct phantom_fire_t : public SL_potion_proc_t<proc_spell_t>
  {
    phantom_fire_t( const special_effect_t& e )
      : base_t( e, "phantom_fire", e.player, e.player->find_spell( 321937 ) )
    {
      if ( shadowcore )
        base_multiplier *= 1.0 + e.driver()->effectN( 2 ).percent();
    }
  };

  init_proc_potion<phantom_fire_t>( effect, "potion_of_phantom_fire" );
}
}  // namespace consumables

namespace enchants
{
void celestial_guidance( special_effect_t& effect )
{
  effect.custom_buff = buff_t::find( effect.player, "celestial_guidance" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<SL_buff_t>( effect.player, "celestial_guidance", effect.player->find_spell( 324748 ) )
      ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
      ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
      ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
      ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
  }

  new dbc_proc_callback_t( effect.player, effect );
}

void lightless_force( special_effect_t& effect )
{
  struct lightless_force_proc_t : public SL_proc_spell_t
  {
    lightless_force_proc_t( const special_effect_t& e ) : SL_proc_spell_t( e )
    {
      // Spell projectile travels along a narrow beam and does not spread out with distance. Although data has a 40yd
      // range, the 1.5s duration seems to prevent the projectile from surviving beyond ~30yd.
      // TODO: confirm maximum effective range & radius
      aoe    = 5;
      radius = 5.0;
      range  = 30.0;

      // hardcoded as a dummy effect
      spell_power_mod.direct = data().effectN( 2 ).percent();
    }

    double composite_spell_power() const override
    {
      return std::max( proc_spell_t::composite_spell_power(), proc_spell_t::composite_attack_power() );
    }
  };

  effect.trigger_spell_id = 324184;
  effect.execute_action   = create_proc_action<lightless_force_proc_t>( "lightless_force", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void sinful_revelation( special_effect_t& effect )
{
  struct sinful_revelation_cb_t : public dbc_proc_callback_t
  {
    sinful_revelation_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto td = a->player->get_target_data( s->target );
      td->debuff.sinful_revelation->trigger();
    }
  };

  new sinful_revelation_cb_t( effect );
}
}  // namespace enchants

namespace items
{
// Trinkets
void darkmoon_deck_putrescence( special_effect_t& effect )
{
  struct putrid_burst_t : public darkmoon_deck_proc_t<>
  {
    putrid_burst_t( const special_effect_t& e )
      : darkmoon_deck_proc_t( e, "putrid_burst", 333885,
                              { 311464, 311465, 311466, 311467, 311468, 311469, 311470, 311471 } )
    {
      split_aoe_damage = true;
    }

    void impact( action_state_t* s ) override
    {
      darkmoon_deck_proc_t::impact( s );

      auto td = player->get_target_data( s->target );
      // Crit debuff value is hard coded into each card
      td->debuff.putrid_burst->trigger( 1, deck->top_card->effectN( 1 ).base_value() * 0.0001 );
    }
  };

  effect.spell_id         = 347047;
  effect.trigger_spell_id = 334058;
  effect.execute_action   = new putrid_burst_t( effect );
}

void darkmoon_deck_voracity( special_effect_t& effect )
{
  struct voracious_hunger_t : public darkmoon_deck_proc_t<>
  {
    stat_buff_t* buff;

    voracious_hunger_t( const special_effect_t& e )
      : darkmoon_deck_proc_t( e, "voracious_hunger", 329446,
                              { 311483, 311484, 311485, 311486, 311487, 311488, 311489, 311490 } )
    {
      may_crit = false;

      buff = debug_cast<stat_buff_t*>( buff_t::find( player, "voracious_haste" ) );
      if ( !buff )
      {
        buff = make_buff<stat_buff_t>( player, "voracious_haste", e.driver()->effectN( 2 ).trigger(), item )
          ->add_stat( STAT_HASTE_RATING, 0 );
      }
    }

    void execute() override
    {
      darkmoon_deck_proc_t::execute();

      buff->stats[ 0 ].amount = deck->top_card->effectN( 1 ).average( item );
      buff->trigger();
    }
  };

  effect.trigger_spell_id = effect.spell_id;
  effect.execute_action   = new voracious_hunger_t( effect );
}

void stone_legion_heraldry( special_effect_t& effect )
{
  double amount   = effect.driver()->effectN( 1 ).average( effect.item );
  unsigned allies = effect.player->sim->shadowlands_opts.stone_legionnaires_in_party;
  double mul      = 1.0 + effect.driver()->effectN( 2 ).percent() * allies;

  effect.player->passive.versatility_rating += amount * mul;

  // Disable further effect handling
  effect.type = SPECIAL_EFFECT_NONE;
}

void cabalists_hymnal( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "crimson_chorus" );
  if ( !buff )
  {
    auto mul =
        1.0 + effect.driver()->effectN( 2 ).percent() * effect.player->sim->shadowlands_opts.crimson_choir_in_party;
    timespan_t duration = effect.trigger()->duration() * effect.trigger()->max_stacks();

    buff = make_buff<stat_buff_t>( effect.player, "crimson_chorus", effect.trigger(), effect.item )
      ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 1 ).average( effect.item ) * mul )
      ->set_period( effect.trigger()->duration() )
      ->set_duration( duration )
      ->set_cooldown( effect.player->find_spell( 344820 )->duration() + duration );
  }

  effect.custom_buff = buff;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

void macabre_sheet_music( special_effect_t& effect )
{
  auto data_spell = effect.player->find_spell( 345431 );
  auto stat_buff  = make_buff<stat_buff_t>( effect.player, "blood_waltz", effect.player->find_spell( 345439 ) )
    ->add_stat( STAT_HASTE_RATING, data_spell->effectN( 1 ).average( effect.item ) )
    ->add_stat( STAT_SPEED_RATING, data_spell->effectN( 2 ).average( effect.item ) );

  effect.custom_buff = make_buff( effect.player, "blood_waltz_driver", effect.driver() )
    ->set_quiet( true )
    ->set_cooldown( 0_ms )
    ->set_tick_callback( [ stat_buff ]( buff_t*, int, timespan_t ) {
      // TODO: handle potential delay/movement required to hit the 'dance partners'
      stat_buff->trigger();
    } );
}

void glyph_of_assimilation( special_effect_t& effect )
{
  auto p    = effect.player;
  auto buff = make_buff<stat_buff_t>( p, "glyph_of_assimilation", p->find_spell( 345500 ) );
  buff->add_stat( STAT_MASTERY_RATING, buff->data().effectN( 1 ).average( effect.item ) );

  p->register_on_kill_callback( [ p, buff ]( player_t* t ) {
    if ( p->sim->event_mgr.canceled )
      return;

    auto d = t->find_dot( "glyph_of_assimilation", p );
    if ( d && d->remains() > 0_ms )
      buff->trigger( d->remains() * 2.0 );
  } );
}

/**Soul Igniter
 * id=345251 driver with 500 ms cooldown
 * id=345211 buff and max scaling targets
 * id=345215 damage effect and category cooldown of second action
 * id=345214 damage coefficients and multipliers
 *           effect #1: base damage (effect #2) + buff time bonus (effect #3)
 *                      incorrectly listed as the self damage in the tooltip
 *           effect #2: base damage
 *           effect #3: max bonus damage (40%) from buff time
 *           effect #4: multiplier based on buff elapsed duration (maybe only used for the tooltip)
 *           effect #5: self damage per 1-second buff tick (NYI)
 */
void soul_igniter( special_effect_t& effect )
{
  struct blazing_surge_t : public shadowlands_aoe_proc_t
  {
    double buff_fraction_elapsed;
    double max_time_multiplier;

    blazing_surge_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "blazing_surge", 345215, true )
    {
      split_aoe_damage = true;
      base_dd_min = e.player->find_spell( 345214 )->effectN( 2 ).min( e.item );
      base_dd_max = e.player->find_spell( 345214 )->effectN( 2 ).max( e.item );
      max_time_multiplier = e.player->find_spell( 345214 )->effectN( 4 ).percent();
      max_scaling_targets = as<unsigned>( e.player->find_spell( 345211 )->effectN( 2 ).base_value() );
    }

    double action_multiplier() const override
    {
      double m = proc_spell_t::action_multiplier();

      // This may actually be added as flat damage using the coefficients,
      // but for a trinket like this it is difficult to verify this.
      m *= 1.0 + buff_fraction_elapsed * max_time_multiplier;

      return m;
    }
  };

  struct soul_ignition_buff_t : public buff_t
  {
    special_effect_t& effect;
    blazing_surge_t* damage_action;
    cooldown_t* shared_cd;
    bool is_precombat;

    soul_ignition_buff_t( special_effect_t& e, action_t* d, cooldown_t* cd ) :
      buff_t( e.player, "soul_ignition", e.player->find_spell( 345211 ) ),
      effect( e ),
      damage_action( debug_cast<blazing_surge_t*>( d ) ),
      shared_cd( cd ),
      is_precombat()
    {}

    void expire_override( int stacks, timespan_t remaining_duration ) override
    {
      // If the trinket was used in precombat, assume that it was timed so
      // that it will expire to deal full damage when it first expires.
      if ( is_precombat )
      {
        shared_cd->adjust( -remaining_duration );
        remaining_duration = 0_ms;
      }

      buff_t::expire_override( stacks, remaining_duration );

      damage_action->buff_fraction_elapsed = ( buff_duration() - remaining_duration ) / buff_duration();
      damage_action->set_target( source->target );
      damage_action->execute();
      // the 60 second cooldown associated with the damage effect trigger
      // does not appear in spell data anywhere and is just in the tooltip.
      effect.execute_action->cooldown->start( effect.execute_action, 60_s );
    }
  };

  struct soul_ignition_t : public proc_spell_t
  {
    soul_ignition_buff_t* buff;
    cooldown_t* shared_cd;
    bool has_precombat_action;

    soul_ignition_t( const special_effect_t& e, cooldown_t* cd ) :
      proc_spell_t( "soul_ignition", e.player, e.driver() ),
      shared_cd( cd ),
      has_precombat_action()
    {
      harmful = false;

      for ( auto a : player->action_list )
      {
        if ( a->action_list && a->action_list->name_str == "precombat" && a->name_str == "use_item_" + e.item->name_str )
        {
          a->harmful = harmful;  // pass down harmful to allow action_t::init() precombat check bypass
          has_precombat_action = true;
          break;
        }
      }
    }

    void init_finished() override
    {
      buff = debug_cast<soul_ignition_buff_t*>( buff_t::find( player, "soul_ignition" ) );
      proc_spell_t::init_finished();
    }

    bool ready() override
    {
      if ( buff->check() && sim->shadowlands_opts.disable_soul_igniter_second_use )
        return false;

      return proc_spell_t::ready();
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( buff->check() )
      {
        buff->expire();
      }
      else
      {
        buff->is_precombat = !player->in_combat && has_precombat_action;
        buff->trigger();
        shared_cd->start( 30_s );
      }
    }
  };

  auto category_cd = effect.player->get_cooldown( "item_cd_" + util::to_string( effect.player->find_spell( 345211 )->category() ) );
  auto damage_action = create_proc_action<blazing_surge_t>( "blazing_surge", effect );
  effect.execute_action = create_proc_action<soul_ignition_t>( "soul_ignition", effect, category_cd );
  auto buff = buff_t::find( effect.player, "soul_ignition" );
  if ( !buff )
    make_buff<soul_ignition_buff_t>( effect, damage_action, category_cd );
}

/** Skulker's Wing
 * id=345019 driver speed buff, effect 1: 8y area trigger create
 * id=345113 dummy damage container spell
 * id=345020 actual triggered leap+damage spell
 */
void skulkers_wing( special_effect_t& effect )
{
  struct skulking_predator_t : public proc_spell_t
  {
    skulking_predator_t( const special_effect_t& e ) :
      proc_spell_t( "skulking_predator", e.player, e.player->find_spell( 345020 ) )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 345113 )->effectN( 1 ).average( e.item );
    }
  };

  // Speed buff is only present until the damage trigger happens, this within 100ms if you are already in range
  // For now assume we are nearest to the primary target and just trigger the damage immediately
  // TODO: Speed buff + range-based target selection?

  effect.execute_action = create_proc_action<skulking_predator_t>( "skulking_predator", effect );
}

/** Memory of Past Sins
 * id=344662 driver buff proc stacks, effect 1: damage amp %
 * id=344663 target debuff damage amp stacks
 * id=344664 proc damage, effect 1: shadow damage
 */
void memory_of_past_sins( special_effect_t& effect )
{
  struct shattered_psyche_damage_t : public generic_proc_t
  {
    shattered_psyche_damage_t( const special_effect_t& e)
      : generic_proc_t( e, "shattered_psyche", 344664 )
    {
      callbacks = false;
    }

    double composite_target_da_multiplier( player_t* t ) const override
    {
      double m = proc_spell_t::composite_target_da_multiplier( t );
      auto td = player->get_target_data( t );

      // Assume we get about half of the value from each application of the extra ally stacks (4 allies = 2 stacks on player's first hit, 22 stacks on player's last hit etc.)
      int base_dmg_stacks = td->debuff.shattered_psyche->check() * (player->sim->shadowlands_opts.shattered_psyche_allies + 1);
      int bonus_dmg_stacks = player->sim->shadowlands_opts.shattered_psyche_allies / 2;
      m *= 1.0 + ( base_dmg_stacks + bonus_dmg_stacks ) * td->debuff.shattered_psyche->check_value();
      return m;
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      auto td = player->get_target_data( s->target );
      td->debuff.shattered_psyche->trigger();
    }
  };

  struct shattered_psyche_cb_t : public dbc_proc_callback_t
  {
    shattered_psyche_damage_t* damage;
    buff_t* buff;

    shattered_psyche_cb_t( const special_effect_t& effect, action_t* d, buff_t* b )
      : dbc_proc_callback_t( effect.player, effect ), damage( debug_cast<shattered_psyche_damage_t*>( d ) ), buff( b )
    {
    }

    void execute( action_t*, action_state_t* trigger_state ) override
    {
      if ( buff->check() )
      {
        damage->set_target( trigger_state->target );
        damage->execute();
        buff->decrement();
      }
    }
  };

  auto buff = buff_t::find( effect.player, "shattered_psyche" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "shattered_psyche", effect.driver() );
    buff->set_initial_stack( buff->max_stack() );
    buff->set_cooldown( 0_s );
  }

  action_t* damage = create_proc_action<shattered_psyche_damage_t>( "shattered_psyche", effect );

  effect.custom_buff = buff;
  effect.disable_action();

  auto cb_driver = new special_effect_t( effect.player );
  cb_driver->name_str = "shattered_psyche_driver";
  cb_driver->spell_id = 344662;
  cb_driver->cooldown_ = 0_s;
  cb_driver->proc_flags_ = effect.driver()->proc_flags();
  cb_driver->proc_flags2_ = PF2_CAST_DAMAGE; // Only triggers from damaging casts
  effect.player->special_effects.push_back( cb_driver );

  auto callback = new shattered_psyche_cb_t( *cb_driver, damage, buff );
  callback->initialize();
  callback->deactivate();

  buff->set_stack_change_callback( [ callback ]( buff_t*, int old, int new_ ) {
    if ( old == 0 )
      callback->activate();
    else if ( new_ == 0 )
      callback->deactivate();
  } );

  timespan_t precast = effect.player->sim->shadowlands_opts.memory_of_past_sins_precast;
  if ( precast > 0_s ) {
    effect.player->register_combat_begin( [&effect, buff, precast]( player_t* ) {
      buff->trigger( buff->buff_duration() - precast );

      cooldown_t* cd = effect.player->get_cooldown( effect.cooldown_name() );
      cd->start( effect.cooldown() - precast );

      cooldown_t* group_cd = effect.player->get_cooldown( effect.cooldown_group_name());
      group_cd->start(effect.cooldown_group_duration() - precast);
    } );
  }
}


/** Gluttonous spike
 * id=344153 single target proc, proccing aura 344154
 * id=344154 15s aura, with 5 ticks of 344155
 * id=344155 aoe proc, increases damage up to 5 targets
 */
void gluttonous_spike( special_effect_t& effect )
{
  struct gluttonous_spike_pulse_t : public shadowlands_aoe_proc_t
  {
    gluttonous_spike_pulse_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "gluttonous_spike_pulse", e.player->find_spell( 344155 ) )
    {
      background = true;
      // The pulse damage comes from the driver
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }
  };

  struct gluttonous_spike_buff_t : public buff_t
  {
    gluttonous_spike_pulse_t* pulse;
    gluttonous_spike_buff_t( const special_effect_t& e, action_t* action ) : buff_t( e.player, "gluttonous_spike", e.player->find_spell( 344154 ), e.item )
    {
      // Buff should never refresh itself since the trigger is disabled while it's up, but just in case
      set_refresh_behavior( buff_refresh_behavior::DISABLED );

      // Only applies the aura if you get overhealed, according to user configuration
      set_chance( player->sim->shadowlands_opts.gluttonous_spike_overheal_chance );

      // Triggers the given action for each tick
      set_tick_callback( [action]( buff_t* /* buff */, int /* current_tick */, timespan_t /* tick_time */ ) {
        action->execute();
      } );
    }
  };

  struct gluttonous_spike_t : public proc_spell_t
  {
    buff_t* buff;

    gluttonous_spike_t( const special_effect_t& e ) :
      proc_spell_t( "gluttonous_spike", e.player, e.trigger() )
    {
      // Create the pulse action and pass it to the buff
      action_t* pulse_action = create_proc_action<gluttonous_spike_pulse_t>( "gluttonous_spike_pulse", e );

      buff = buff_t::find(player, "gluttonous_spike");
      // Creates the buff if absent
      if (!buff)
      {
        buff = make_buff<gluttonous_spike_buff_t>( player, pulse_action );
      }

      // Main proc damage is in its own spell 344153, and not in the tooltip of the main spell
      base_dd_min = base_dd_max = e.player->find_spell( 344153 )->effectN( 1 ).average( e.item );

      if ( action_t* pulse = e.player->find_action( "gluttonous_spike_pulse" ) )
      {
        add_child( pulse );
      }
    }

    // Being overhealed by the proc applies the 344154 aura, which disables this proc while active
    void execute() override
    {
      // If the aura isn't currently running, execute the spell and triggers the buff
      if (!buff->check())
      {
        proc_spell_t::execute();
        buff->trigger();
      }
      else if ( pre_execute_state )
      {
        action_state_t::release( pre_execute_state );
      }
    }
  };

  effect.execute_action = create_proc_action<gluttonous_spike_t>( "gluttonous_spike", effect );
  new dbc_proc_callback_t( effect.player, effect );
}


void hateful_chain( special_effect_t& effect )
{
  struct hateful_rage_t : public proc_spell_t
  {
    hateful_rage_t( const special_effect_t& e ) :
      proc_spell_t( "hateful_rage", e.player, e.player->find_spell( 345361 ) )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
    }
  };

  struct hateful_chain_callback_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      // XXX: Assume the actor always has more health than the target
      // TODO: Handle actor health < target health case?
      proc_action->target = target( state );
      proc_action->schedule_execute();
    }
  };

  effect.execute_action = create_proc_action<hateful_rage_t>( "hateful_rage", effect );

  new hateful_chain_callback_t( effect.player, effect );
}

void bottled_flayedwing_toxin( special_effect_t& effect )
{
  // Assume the player keeps the buff up on its own as the item is off-gcd and
  // the buff has a 60 minute duration which is enough for any encounter.
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct flayedwing_toxin_t : public proc_spell_t
  {
    flayedwing_toxin_t( const special_effect_t& e )
      : proc_spell_t( "flayedwing_toxin", e.player, e.trigger(), e.item )
    {
      // Tick behavior is odd, it always ticks on refresh but is not rescheduled
      tick_zero = true;
      // Tick damage value lives in a different spell for some reason
      base_td = e.player->find_spell( 345547 )->effectN( 1 ).average( e.item );
    }
  };

  auto secondary      = new special_effect_t( effect.item );
  secondary->type     = SPECIAL_EFFECT_EQUIP;
  secondary->source   = SPECIAL_EFFECT_SOURCE_ITEM;
  secondary->spell_id = effect.spell_id;
  secondary->execute_action = create_proc_action<flayedwing_toxin_t>( "flayedwing_toxin", *secondary );
  effect.player->special_effects.push_back( secondary );

  auto callback = new dbc_proc_callback_t( effect.item, *secondary );
  callback->initialize();
}

/**Empyreal Ordnance
 * id=345539 driver
 * id=345540 projectiles and DoT
 * id=345542 DoT damage
 * id=345541 buff
 * id=345543 buff value
 */
void empyreal_ordnance( special_effect_t& effect )
{
  struct empyreal_ordnance_bolt_t : public proc_spell_t
  {
    buff_t* buff;
    double buff_travel_speed;

    empyreal_ordnance_bolt_t( const special_effect_t& e, buff_t* b ) :
      proc_spell_t( "empyreal_ordnance_bolt", e.player, e.player->find_spell( 345540 ) ),
      buff( b )
    {
      base_td = e.player->find_spell( 345542 )->effectN( 1 ).average( e.item );
      buff_travel_speed = e.player->find_spell( 345544 )->missile_speed();
    }

    timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t duration ) const override
    {
      return duration;
    }

    void last_tick( dot_t* d ) override
    {
      double distance = player->get_player_distance( *d->target );
      // TODO: This travel time seems slightly wrong at long range (about 500 ms too fast).
      timespan_t buff_travel_time = timespan_t::from_seconds( buff_travel_speed ? distance / buff_travel_speed : 0 );
      int stack = d->current_stack();
      make_event( *sim, buff_travel_time, [ this, stack ] { buff->trigger( stack ); } );
      proc_spell_t::last_tick( d );
    }
  };

  struct empyreal_ordnance_t : public proc_spell_t
  {
    action_t* empyreal_ordnance_bolt;
    size_t num_bolts;

    empyreal_ordnance_t( const special_effect_t& e, buff_t* b ) :
      proc_spell_t( "empyreal_ordnance", e.player, e.driver() )
    {
      num_bolts = as<size_t>( e.driver()->effectN( 1 ).base_value() );
      aoe = as<int>( e.driver()->effectN( 2 ).base_value() );
      // Spell data for this trinket has a cast time of 500 ms even though it is instant and can be used while moving.
      base_execute_time = 0_ms;
      empyreal_ordnance_bolt = create_proc_action<empyreal_ordnance_bolt_t>( "empyreal_ordnance_bolt", e, b );
      add_child( empyreal_ordnance_bolt );
    }

    void impact( action_state_t* s ) override
    {
      size_t n = num_bolts / s->n_targets + ( as<unsigned>( s->chain_target ) < num_bolts % s->n_targets ? 1 : 0 );
      player_t* t = s->target;
      for ( size_t i = 0; i < n; i++ )
      {
        // The bolts don't seem to all hit the target
        // at exactly the same time, which creates a
        // small partial tick at the end of the DoT.
        make_event( *sim, i * 50_ms, [ this, t ]
          {
            empyreal_ordnance_bolt->set_target( t );
            empyreal_ordnance_bolt->execute();
          } );
      }
    }
  };

  auto buff = buff_t::find( effect.player, "empyreal_surge" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "empyreal_surge", effect.player->find_spell( 345541 ) )
             ->add_stat( STAT_INTELLECT, effect.player->find_spell( 345543 )->effectN( 1 ).average( effect.item ) );
  }

  effect.execute_action = create_proc_action<empyreal_ordnance_t>( "empyreal_ordnance", effect, buff );
}

// The slow debuff on the target and the heal when gaining the crit buff are not implemented.
// id=345807: crit value coefficients
// id=345806: heal amoutn coefficient
void soulletting_ruby( special_effect_t& effect )
{
  struct soulletting_ruby_t : public proc_spell_t
  {
    stat_buff_t* buff;
    double base_crit_value;
    double max_crit_bonus;

    soulletting_ruby_t( const special_effect_t& e, stat_buff_t* b ) :
      proc_spell_t( "soulletting_ruby", e.player, e.player->find_spell( 345802 ) ),
      buff( b ),
      base_crit_value( e.player->find_spell( 345807 )->effectN( 1 ).average( e.item ) ),
      max_crit_bonus( e.player->find_spell( 345807 )->effectN( 2 ).average( e.item ) )
    {}

    void execute() override
    {
      proc_spell_t::execute();

      make_event( *sim, travel_time(), [ this, t = execute_state->target ] {
        double bonus_mul = 1.0 - ( t->is_active() ? t->health_percentage() * 0.01 : 0.0 );

        buff->stats.back().amount = base_crit_value + max_crit_bonus * bonus_mul;
        buff->trigger();
      } );
    }
  };

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "soul_infusion" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "soul_infusion", effect.player->find_spell( 345805 ) )
             ->add_stat( STAT_CRIT_RATING, 0 );
  }

  effect.execute_action = create_proc_action<soulletting_ruby_t>( "soulletting_ruby", effect, buff );
}

void satchel_of_misbegotten_minions( special_effect_t& effect )
{
  struct abomiblast_t : public shadowlands_aoe_proc_t
  {
    abomiblast_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "abomiblast", 345638, true )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
      max_scaling_targets = as<unsigned>( e.driver()->effectN( 2 ).base_value() );
    }
  };

  struct misbegotten_minion_t : public proc_spell_t
  {
    misbegotten_minion_t( const special_effect_t& e ) :
      proc_spell_t( "misbegotten_minion", e.player, e.player->find_spell( 346032 ) )
    {
      quiet = true;
      impact_action = create_proc_action<abomiblast_t>( "abomiblast", e );
    }
  };

  effect.execute_action = create_proc_action<misbegotten_minion_t>( "misbegotten_minion", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

/**
 * Mistcaller Ocarina
 * id=330067 driver #1 (versatility)
 * id=332299 driver #2 (crit)
 * id=332300 driver #3 (haste)
 * id=332301 driver #4 (mastery)
 * id=330132 buff #1 (versatility)
 * id=332077 buff #2 (crit)
 * id=332078 buff #3 (haste)
 * id=332079 buff #4 (mastery)
 */
void mistcaller_ocarina( special_effect_t& effect )
{
  using id_pair_t = std::pair<unsigned, unsigned>;
  static constexpr id_pair_t spells[] = {
    { 330067, 330132 }, // versatility
    { 332299, 332077 }, // crit
    { 332300, 332078 }, // haste
    { 332301, 332079 }, // mastery
  };
  auto it = range::find( spells, effect.spell_id, &id_pair_t::first );
  if ( it == range::end( spells ) )
    return; // Unknown driver spell, "disable" the trinket

  // Assume the player keeps the buff up on its own and disable some stuff
  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.cooldown_ = 0_ms;
  effect.duration_ = 0_ms;

  const spell_data_t* buff_spell = effect.player->find_spell( it->second );
  const std::string buff_name = util::tokenize_fn( buff_spell->name_cstr() );

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, buff_name ) );
  if ( !buff )
  {
    double amount = effect.driver()->effectN( 1 ).average( effect.item );

    buff = make_buff<stat_buff_t>( effect.player, buff_name, buff_spell );
    for ( auto& s : buff->stats )
      s.amount = amount;

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

/**Unbound Changeling
 * id=330747 coefficients for stat amounts, and also the special effect on the base item
 * id=330765 driver #1 (crit, haste, and mastery)
 * id=330080 driver #2 (crit)
 * id=330733 driver #3 (haste)
 * id=330734 driver #4 (mastery)
 * id=330764 buff #1 (crit, haste, and mastery)
 * id=330730 buff #2 (crit)
 * id=330131 buff #3 (haste)
 * id=330729 buff #4 (mastery)
 */
void unbound_changeling( special_effect_t& effect )
{
  std::string_view stat_type = effect.player->sim->shadowlands_opts.unbound_changeling_stat_type;
  if ( stat_type == "all" )
    effect.spell_id = 330765;
  else if ( stat_type == "crit" )
    effect.spell_id = 330080;
  else if ( stat_type == "haste" )
    effect.spell_id = 330733;
  else if ( stat_type == "mastery" )
    effect.spell_id = 330734;
  else
  {
    if ( effect.spell_id == 330747 )
    {
      // If one of the bonus ID effects is present, bail out and let that bonus ID handle things instead.
      for ( auto& e : effect.item->parsed.special_effects )
      {
        if ( e->spell_id == 330765 || e->spell_id == 330080 || e->spell_id == 330733 || e->spell_id == 330734 )
            return;
      }
      // Fallback, profile does not specify a stat-giving item bonus, so default to haste.
      effect.spell_id = 330733;
    }
  }

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "unbound_changeling" ) );
  if ( effect.spell_id > 0 && !buff )
  {
    int buff_spell_id = effect.driver()->effectN( 1 ).trigger_spell_id();
    buff = make_buff<stat_buff_t>( effect.player, "unbound_changeling", effect.player->find_spell( buff_spell_id ) );
    double amount = effect.player->find_spell( 330747 )->effectN( buff_spell_id != 330764 ? 1 : 2 ).average( effect.item );

    for ( auto& s : buff->stats )
      s.amount = amount;

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

/**Infinitely Divisible Ooze
 * id=345490 driver and coefficient
 * id=345489 summon spell
 * id=345495 pet cast (noxious bolt)
 */
void infinitely_divisible_ooze( special_effect_t& effect )
{
  struct noxious_bolt_t : public spell_t
  {
    noxious_bolt_t( pet_t* p, const special_effect_t& e ) : spell_t( "noxious_bolt", p, p->find_spell( 345495 ) )
    {
      // Merge the stats object with other instances of the pet
      auto ta = p->owner->find_pet( "frothing_pustule" );
      if ( ta && ta->find_action( "noxious_bolt" ) )
        stats = ta->find_action( "noxious_bolt" )->stats;

      may_crit = true;
      base_dd_min = p->find_spell( 345490 )->effectN( 1 ).min( e.item );
      base_dd_max = p->find_spell( 345490 )->effectN( 1 ).max( e.item );
    }

    void execute() override
    {
      spell_t::execute();

      if ( player->resources.current[ RESOURCE_ENERGY ] <= 0 )
      {
        make_event( *sim, 0_ms, [ this ] { player->cast_pet()->dismiss(); } );
      }
    }
  };

  struct frothing_pustule_pet_t : public pet_t
  {
    const special_effect_t& effect;

    frothing_pustule_pet_t( const special_effect_t& e ) :
      pet_t( e.player->sim, e.player, "frothing_pustule", true, true ),
      effect( e )
    {
      npc_id = 175519;
    }

    void init_base_stats() override
    {
      pet_t::init_base_stats();

      resources.base[ RESOURCE_ENERGY ] = 100;
    }

    void init_assessors() override
    {
      pet_t::init_assessors();
      auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ) {
        if ( effect.player->specialization() == MONK_BREWMASTER || effect.player->specialization() == MONK_WINDWALKER ||
             effect.player->specialization() == MONK_MISTWEAVER )
        {
          monk::monk_t* monk_player = static_cast<monk::monk_t*>( owner );
          auto td                   = monk_player->find_target_data( s->target );
          if ( !owner->bugs && td && td->debuff.bonedust_brew->check() )
            monk_player->bonedust_brew_assessor( s );
        }
        return assessor::CONTINUE;
      };

      assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
    }

    resource_e primary_resource() const override
    {
      return RESOURCE_ENERGY;
    }

    action_t* create_action( util::string_view name, util::string_view options ) override
    {
      if ( name == "noxious_bolt" )
      {
        return new noxious_bolt_t( this, effect );
      }

      return pet_t::create_action( name, options );
    }

    void init_action_list() override
    {
      pet_t::init_action_list();

      if ( action_list_str.empty() )
        get_action_priority_list( "default" )->add_action( "noxious_bolt" );
    }
  };

  struct infinitely_divisible_ooze_cb_t : public dbc_proc_callback_t
  {
    spawner::pet_spawner_t<frothing_pustule_pet_t> spawner;

    infinitely_divisible_ooze_cb_t( const special_effect_t& e ) :
      dbc_proc_callback_t( e.player, e ),
      spawner( "infinitely_divisible_ooze", e.player, [ &e ]( player_t* )
        { return new frothing_pustule_pet_t( e ); } )
    {
      spawner.set_default_duration( e.player->find_spell( 345489 )->duration() );
    }

    void execute( action_t*, action_state_t* ) override
    {
      spawner.spawn();
    }
  };

  new infinitely_divisible_ooze_cb_t( effect );
}

/**Inscrutable Quantum Device
 * id=330323 driver
 * id=330363 remove CC from self (NYI)
 * id=330364 heal spell (NYI)
 * id=330372 illusion and threat drop (NYI)
 * id=347940 illusion helper spell, taunt (NYI)
 * id=347941 illusion helper aura (NYI)
 * id=330373 execute damage on target
 * id=330376 restore mana to healer (NYI)
 * id=330366 crit buff
 * id=330368 haste buff
 * id=330380 mastery buff
 * id=330367 vers buff
 * id=348098 unknown spell
 * When this trinket is used, it triggers one of the effects listed above, following the priority list below.
 * - remove CC from self: Always triggers if you are under a hard CC mechanic, does not trigger if the CC mechanic
 *                        does not prevent the player from acting (e.g., it won't trigger while rooted).
 * - heal spell: Triggers on self or a nearby target with less than 30% health remaining. This always crits.
 * - illusion: ??? (not tested yet, priority unknown)
 * - execute damage: Deal damage to the target if it is an enemy with less than 20% health remaining.
 *                   The 20% is not in spell data and this also always crits.
 * - healer mana: Triggers on a nearby healer with less than 20% mana. Higher priority than secondary
 *                stat buffs, but priority relative to other effects not tested.
 * - secondary stat buffs:
 *   - The secondary stat granted appears to be randomly selected from stat from the player's two highest
 *     secondary stats in terms of rating. When selecting the largest stats, the priority of equal secondary
 *     stats seems to be Vers > Mastery > Haste > Crit. There is a bug where the second stat selected must
 *     have a lower rating than the first stat that was selected. If this is not possible, then the second
 *     stat will be empty and the trinket will have a chance to do nothing when used.
 *   - If a Bloodlust buff is up, the stat buff will last 25 seconds instead of the default 20 seconds.
 *     Additionally, instead of randomly selecting from the player's two highest stats it will always
 *     grant the highest stat.
 *     TODO: Look for other buffs that also cause a bonus duration to occur. The spell data still lists
 *     a 30 second buff duration, so it is possible that there are other conditions that give 30 seconds.
 */
void inscrutable_quantum_device ( special_effect_t& effect )
{
  static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };
  static constexpr std::array<int, 4> buff_ids = { 330367, 330380, 330368, 330366 };

  struct inscrutable_quantum_device_execute_t : public proc_spell_t
  {
    inscrutable_quantum_device_execute_t( const special_effect_t& e ) :
      proc_spell_t( "inscrutable_quantum_device_execute", e.player, e.player->find_spell( 330373 ), e.item )
    {
      cooldown->duration = 0_ms;
    }

    double composite_crit_chance() const override
    {
      double cc = proc_spell_t::composite_crit_chance() + 1;

      return cc;
    }
  };

  struct inscrutable_quantum_device_t : public proc_spell_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;
    std::vector<buff_t*> bonus_buffs;
    action_t* execute_damage;

    inscrutable_quantum_device_t( const special_effect_t& e ) :
      proc_spell_t( "inscrutable_quantum_device", e.player, e.player->find_spell( 330323 ) )
    {
      buffs[ STAT_NONE ] = nullptr;
      for ( unsigned i = 0; i < ratings.size(); i++ )
      {
        auto name = std::string( "inscrutable_quantum_device_" ) + util::stat_type_string( ratings[ i ] );
        stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( e.player, name ) );
        if ( !buff )
        {
          buff = make_buff<stat_buff_t>( e.player, name, e.player->find_spell( buff_ids[ i ] ), e.item );
          buff->set_cooldown( 0_ms );
        }
        buffs[ ratings[ i ] ] = buff;
      }
      execute_damage = create_proc_action<inscrutable_quantum_device_execute_t>( "inscrutable_quantum_device_execute", e );
    }

    void init_finished() override
    {
      proc_spell_t::init_finished();

      bonus_buffs.clear();

      bonus_buffs.push_back( player->buffs.bloodlust );

      if ( player->type == MAGE )
      {
        if ( buff_t* b = buff_t::find( player, "temporal_warp" ) )
          bonus_buffs.push_back( b );
        if ( buff_t* b = buff_t::find( player, "time_warp" ) )
          bonus_buffs.push_back( b );
      }
    }

    bool is_buff_extended()
    {
      return range::any_of( bonus_buffs, [] ( buff_t* b ) { return b->check(); } );
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( target->health_percentage() <= 20 && !player->sim->shadowlands_opts.disable_iqd_execute )
      {
        execute_damage->set_target( target );
        execute_damage->execute();
      }
      else
      {
        stat_e s1 = STAT_NONE;
        stat_e s2 = STAT_NONE;
        buff_t* buff;
        timespan_t duration_adjustment;

        s1 = util::highest_stat( player, ratings );

        if ( is_buff_extended() )
        {
          buff = buffs[ s1 ];
          duration_adjustment = 5_s;
        }
        else
        {
          if ( rng().roll( sim->shadowlands_opts.iqd_stat_fail_chance ) )
            return;
          for ( auto s : ratings )
          {
            auto v = util::stat_value( player, s );
            if ( ( s2 == STAT_NONE || v > util::stat_value( player, s2 ) ) &&
                 ( ( player->bugs && v < util::stat_value( player, s1 ) ) || ( !player->bugs && s != s1 ) ) )
              s2 = s;
          }
          buff = rng().roll( 0.5 ) ? buffs[ s1 ] : buffs[ s2 ];
          duration_adjustment = 10_s;
        }

        if ( buff )
          buff->trigger( buff->buff_duration() - duration_adjustment );
      }
    }
  };

  effect.execute_action = create_proc_action<inscrutable_quantum_device_t>( "inscrutable_quantum_device", effect );
  effect.disable_buff();
  effect.stat = STAT_ALL;
}

/** Phial of Putrefaction
 * id=345465 driver with periodic 5s application of the player proc buff
 * id=345464 10s player buff with proc trigger
 * id=345466 Stacking DoT damage spell
 */
void phial_of_putrefaction( special_effect_t& effect )
{
  struct liquefying_ooze_t : public proc_spell_t
  {
    liquefying_ooze_t( const special_effect_t& e ) :
      proc_spell_t( "liquefying_ooze", e.player, e.player->find_spell( 345466 ), e.item )
    {
      dot_behavior = dot_behavior_e::DOT_NONE; // DoT does not refresh duration
      base_td = e.driver()->effectN( 1 ).average( e.item );
    }
  };

  struct phial_of_putrefaction_proc_t : public dbc_proc_callback_t
  {
    phial_of_putrefaction_proc_t( const special_effect_t* e ) :
      dbc_proc_callback_t( e->player, *e ) { }

    void execute( action_t*, action_state_t* s ) override
    {
      // Only allow one proc on simultaneous hits
      if ( !proc_buff->check() )
        return;

      // Targets at max stacks do not 'eat' proc attempts or consume the player buff
      auto d = proc_action->get_dot( s->target );
      if ( !d->is_ticking() || !d->at_max_stacks() )
      {
        proc_buff->expire();
        proc_action->set_target( s->target );
        proc_action->execute();
      }
    }
  };

  auto putrefaction_buff = buff_t::find( effect.player, "phial_of_putrefaction" );
  if ( !putrefaction_buff )
  {
    auto proc_spell = effect.player->find_spell( 345464 );
    putrefaction_buff = make_buff( effect.player, "phial_of_putrefaction", proc_spell );

    special_effect_t* putrefaction_proc = new special_effect_t( effect.player );
    putrefaction_proc->proc_flags_ = proc_spell->proc_flags();
    putrefaction_proc->proc_flags2_ = PF2_CAST_DAMAGE;
    putrefaction_proc->spell_id = 345464;
    putrefaction_proc->custom_buff = putrefaction_buff;
    putrefaction_proc->execute_action = create_proc_action<liquefying_ooze_t>( "liquefying_ooze", effect );

    effect.player->special_effects.push_back( putrefaction_proc );

    auto proc_object = new phial_of_putrefaction_proc_t( putrefaction_proc );
    proc_object->deactivate();

    putrefaction_buff->set_stack_change_callback( [ proc_object ]( buff_t*, int, int new_ ) {
      if ( new_ == 1 ) { proc_object->activate(); }
      else { proc_object->deactivate(); }
    } );

    effect.player->register_combat_begin( [ &effect, putrefaction_buff ]( player_t* ) {
      putrefaction_buff->trigger();
      make_repeating_event( putrefaction_buff->source->sim, effect.driver()->effectN( 1 ).period(),
                            [ putrefaction_buff ]() { putrefaction_buff->trigger(); } );
    } );
  }
}

/** Grim Codex
 * id=345739 driver and primary shadow damage hit
 * id=345877 dummy spell with damage values for both primary and secondary impacts
 * id=345963 AoE dummy spell with radius
 * id=345864 AoE damage spell
 */
void grim_codex( special_effect_t& effect )
{
  struct grim_codex_aoe_t : public proc_spell_t
  {
    grim_codex_aoe_t( const special_effect_t& e ) :
      proc_spell_t( "whisper_of_death", e.player, e.player->find_spell( 345864 ), e.item )
    {
      aoe = -1;
      radius = player->find_spell( 345963 )->effectN( 1 ).radius_max();
      base_dd_min = base_dd_max = player->find_spell( 345877 )->effectN( 2 ).average( e.item );
    }

    // AoE does not hit the primary target
    size_t available_targets( std::vector< player_t* >& tl ) const override
    {
      proc_spell_t::available_targets( tl );
      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* t ) { return t == this->target; } ), tl.end() );
      return tl.size();
    }
  };

  struct grim_codex_t : public proc_spell_t
  {
    grim_codex_t( const special_effect_t& e ) :
      proc_spell_t( "spectral_scythe", e.player, e.driver(), e.item )
    {
      base_dd_min = base_dd_max = player->find_spell( 345877 )->effectN( 1 ).average( e.item );
      impact_action = create_proc_action<grim_codex_aoe_t>( "whisper_of_death", e );
      add_child( impact_action );
    }
  };

  effect.execute_action = create_proc_action<grim_codex_t>( "grim_codex", effect );
}

void anima_field_emitter( special_effect_t& effect )
{
  struct anima_field_emitter_proc_t : public dbc_proc_callback_t
  {
    timespan_t max_duration;

    anima_field_emitter_proc_t( const special_effect_t& e ) :
      dbc_proc_callback_t( e.player, e ),
      max_duration( effect.player->find_spell( 345534 )->duration() )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      timespan_t buff_duration = max_duration;
      if ( listener->sim->shadowlands_opts.anima_field_emitter_mean != std::numeric_limits<double>::max() )
      {
        double new_duration = rng().gauss( listener->sim->shadowlands_opts.anima_field_emitter_mean,
            listener->sim->shadowlands_opts.anima_field_emitter_stddev );
        buff_duration = timespan_t::from_seconds( clamp( new_duration, 0.0, max_duration.total_seconds() ) );
      }

      if ( buff_duration > timespan_t::zero() )
      {
        proc_buff->trigger( buff_duration );
      }
    }
  };

  buff_t* buff = buff_t::find( effect.player, "anima_field" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "anima_field", effect.player->find_spell( 345535 ) )
      ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;

    new anima_field_emitter_proc_t( effect );
  }
}

void decanter_of_animacharged_winds( special_effect_t& effect )
{
  // TODO: "Damage is increased for each enemy struck, up to 5 enemies."
  struct splash_of_animacharged_wind_t : public shadowlands_aoe_proc_t
  {
    splash_of_animacharged_wind_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "splash_of_animacharged_wind", e.trigger(), true )
    {
      max_scaling_targets = as<unsigned>( e.driver()->effectN( 2 ).base_value() );
    }
  };

  effect.execute_action = create_proc_action<splash_of_animacharged_wind_t>(
      "splash_of_animacharged_wind", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void bloodspattered_scale( special_effect_t& effect )
{
  struct blood_barrier_t : public shadowlands_aoe_proc_t
  {
    buff_t* absorb;

    blood_barrier_t( const special_effect_t& e, buff_t* absorb_ ) :
      shadowlands_aoe_proc_t( e, "blood_barrier", e.trigger(), true ), absorb( absorb_ )
    {
      max_scaling_targets = as<unsigned>( e.driver()->effectN( 2 ).base_value() );
    }

    void execute() override
    {
      proc_spell_t::execute();

      absorb->trigger( 1, execute_state->result_amount * execute_state->n_targets );
    }
  };

  auto buff = buff_t::find( effect.player, "blood_barrier" );
  if ( !buff )
  {
    buff = make_buff<absorb_buff_t>( effect.player, "blood_barrier",
        effect.player->find_spell( 329849 ), effect.item )
      ->set_default_value( effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.execute_action = create_proc_action<blood_barrier_t>( "blood_barrier", effect, buff );
  }
}

/**Shadowgrasp Totem
 * id=331523 driver with totem summon
 * id=331532 periodic dummy trigger (likely handling the retargeting) controls the tick rate
 * id=329878 scaling spell containing CDR, damage, and healing dummy parameters
 *           effect #1: scaled damage
 *           effect #2: scaled healing
 *           effect #3: CDR on target kill (in seconds)
 * id=331537 damage spell and debuff with death proc trigger
 */
void shadowgrasp_totem( special_effect_t& effect )
{
  struct shadowgrasp_totem_damage_t : public spell_t
  {
    shadowgrasp_totem_damage_t( pet_t* p, const special_effect_t& effect ) :
      spell_t( "shadowgrasp_totem", p, p->owner->find_spell( 331537 ) )
    {
      dot_duration = 0_s;
      base_dd_min = base_dd_max = p->owner->find_spell( 329878 )->effectN( 1 ).average( effect.item );
    }
  };

  struct shadowgrasp_totem_pet_t : public pet_t
  {
    const special_effect_t& effect;

    shadowgrasp_totem_damage_t* damage;

    shadowgrasp_totem_pet_t( const special_effect_t& e ) :
      pet_t( e.player->sim, e.player, "shadowgrasp_totem", true, true),
      effect( e )
    {
      npc_id = 170190;
    }

    void init_spells() override
    {
      pet_t::init_spells();

      damage = new shadowgrasp_totem_damage_t ( this, effect );
    }

    void init_assessors() override
    {
      pet_t::init_assessors();
      auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ) {
        if ( effect.player->specialization() == MONK_BREWMASTER || effect.player->specialization() == MONK_WINDWALKER ||
             effect.player->specialization() == MONK_MISTWEAVER  )
        {
          monk::monk_t* monk_player = static_cast<monk::monk_t*>( owner );
          auto td                   = monk_player->find_target_data( s->target );
          if ( !owner->bugs && td && td->debuff.bonedust_brew->check() )
            monk_player->bonedust_brew_assessor( s );
        }
        return assessor::CONTINUE;
      };

      assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
    }
  };

  struct shadowgrasp_totem_buff_t : public buff_t
  {
    spawner::pet_spawner_t<shadowgrasp_totem_pet_t> spawner;
    cooldown_t* item_cd;
    timespan_t cd_adjust;
    timespan_t last_retarget;

    shadowgrasp_totem_buff_t( const special_effect_t& effect ) :
      buff_t( effect.player, "shadowgrasp_totem", effect.player->find_spell( 331537 ) ),
      spawner( "shadowgrasp_totem", effect.player, [ &effect ]( player_t* )
        { return new shadowgrasp_totem_pet_t( effect ); } )
    {
      spawner.set_default_duration( effect.player->find_spell( 331537 )->duration() );

      // Periodic trigger in spell 331532 itself is hasted, which appears to control the tick rate
      set_tick_time_behavior( buff_tick_time_behavior::HASTED );
      set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
        auto pet = spawner.pet();
        if ( !pet )
          return;

        pet->damage->set_target( player->target );
        pet->damage->execute();
      } );

      item_cd = effect.player->get_cooldown( effect.cooldown_name() );
      cd_adjust = timespan_t::from_seconds(
        -source->find_spell( 329878 )->effectN( 3 ).base_value() );

      range::for_each( effect.player->sim->actor_list, [ this ]( player_t* t ) {
        t->register_on_demise_callback( source, [ this ]( player_t* actor ) {
          trigger_target_death( actor );
        } );
      } );
    }

    void reset() override
    {
      buff_t::reset();

      last_retarget = 0_s;
    }

    bool trigger( int stacks, double value, double chance, timespan_t duration) override
    {
      spawner.spawn();
      return buff_t::trigger(stacks, value, chance, duration);
    }

    void trigger_target_death( const player_t* actor )
    {
      auto pet = spawner.pet();
      if ( !pet || !actor->is_enemy() )
      {
        return;
      }


      if ( pet->target != actor )
      {
        auto retarget_time = sim->shadowlands_opts.retarget_shadowgrasp_totem;
        if ( retarget_time == 0_s || sim->current_time() - last_retarget < retarget_time )
          return;
        //  Emulate player "retargeting" during Shadowgrasp Totem duration to grab more
        //  15 second cooldown reductions.
        //  Don't actually change the target of the ability, as it will always hit whatever the
        //  player is hitting.
        last_retarget = sim->current_time();
      }

      item_cd->adjust( cd_adjust );
    }
  };

  auto buff = buff_t::find( effect.player, "shadowgrasp_totem" );
  if ( !buff ) {
    buff = make_buff<shadowgrasp_totem_buff_t>( effect );
  }
  effect.custom_buff = buff;
}

// TODO: Implement healing?
void hymnal_of_the_path( special_effect_t& effect )
{
  struct hymnal_of_the_path_t : public proc_spell_t
  {
    hymnal_of_the_path_t( const special_effect_t& e ) :
      proc_spell_t( "hymnal_of_the_path", e.player, e.player->find_spell( 348141 ) )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
    }
  };

  struct hymnal_of_the_path_cb_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      // XXX: Assume the actor always has more health than the target
      // TODO: Handle actor health < target health case?
      proc_action->set_target( target( state ) );
      proc_action->schedule_execute();
    }
  };

  effect.execute_action = create_proc_action<hymnal_of_the_path_t>(
      "hymnal_of_the_path", effect );

  new hymnal_of_the_path_cb_t( effect.player, effect );
}

void overwhelming_power_crystal( special_effect_t& effect)
{
  // automagic handles almost everything, just need to disable the damage action
  // so the automagic doesn't incorrectly apply it to the target
  effect.disable_action();
}

void overflowing_anima_cage( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "anima_infusion" );
  if ( !buff )
  {
    auto val = effect.player->find_spell( 343387 )->effectN( 1 ).average( effect.item );
    // The actual buff is Anima Infusion (id=343386), but the spell data there is not needed.
    buff = make_buff<stat_buff_t>( effect.player, "anima_infusion", effect.player->find_spell( 343385 ) )
             ->add_stat( STAT_CRIT_RATING, val )
             ->set_cooldown( 0_ms );
  }

  effect.custom_buff = buff;
}

void sunblood_amethyst( special_effect_t& effect )
{
  struct anima_font_proc_t : public proc_spell_t
  {
    buff_t* buff;

    anima_font_proc_t( const special_effect_t& e )
      : proc_spell_t( "anima_font", e.player, e.driver()->effectN( 2 ).trigger() )
    {
      // id:344414 Projectile with travel speed data (effect->driver->eff#2->trigger)
      // id:343394 'Font of power' spell with duration data (projectile->eff#1->trigger)
      // id:343396 Actual buff is id:343396
      // id:343397 Coefficient data
      buff = make_buff<stat_buff_t>( e.player, "anima_font", e.player->find_spell( 343396 ) )
        ->add_stat( STAT_INTELLECT, e.player->find_spell( 343397 )->effectN( 1 ).average( e.item ) )
        ->set_duration( data().effectN( 1 ).trigger()->duration() );
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      // TODO: add way to handle staying within range of the anima font to gain the int buff
      buff->trigger();
    }
  };

  struct tear_anima_proc_t : public proc_spell_t
  {
    action_t* font;

    tear_anima_proc_t( const special_effect_t& e )
      : proc_spell_t( e ), font( create_proc_action<anima_font_proc_t>( "anima_font", e ) )
    {}

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      // Assumption is that the 'tear' travels back TO player FROM target, which for sim purposes is treated as a normal
      // projectile FROM player TO target
      font->set_target( s->target );
      font->schedule_execute();
    }
  };

  effect.trigger_spell_id = effect.spell_id;
  effect.execute_action   = create_proc_action<tear_anima_proc_t>( "tear_anima", effect );
}

void flame_of_battle( special_effect_t& effect )
{
  debug_cast<stat_buff_t*>( effect.create_buff() )->stats[ 0 ].amount =
      effect.player->find_spell( 346746 )->effectN( 1 ).average( effect.item );
}

// id=336182 driver, effect#2 has multiplier
// id=336183 damage spell, effect#1 value seems to be overridden by driver effect#1 value
void tablet_of_despair( special_effect_t& effect )
{
  struct burst_of_despair_t : public proc_spell_t
  {
    // each tick has a multiplier of 1.5^(tick #) with tick on application being tick #0
    double tick_factor;
    int tick_number;

    burst_of_despair_t( const special_effect_t& e )
      : proc_spell_t( "burst_of_despair", e.player, e.player->find_spell( 336183 ) ),
        tick_factor( e.driver()->effectN( 2 ).base_value() ),
        tick_number( 0 )
    {
      base_dd_min = base_dd_max = e.trigger()->effectN( 1 ).average( e.item );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = proc_spell_t::composite_da_multiplier( s );

      am *= std::pow( tick_factor, tick_number );

      return am;
    }
  };

  struct growing_despair_t : public proc_spell_t
  {
    burst_of_despair_t* burst;

    growing_despair_t( const special_effect_t& e ) : proc_spell_t( e ), burst( nullptr )
    {
      tick_action = create_proc_action<burst_of_despair_t>( "burst_of_despair", e );
      burst = debug_cast<burst_of_despair_t*>( tick_action );
    }

    void tick( dot_t* d ) override
    {
      burst->tick_number = d->current_tick;

      proc_spell_t::tick( d );
    }
  };

  effect.execute_action = create_proc_action<growing_despair_t>( "growing_despair", effect );
}

// id=329536 driver, damage value in effect#2
// id=329540 unknown use, triggered by driver
// id=329548 damage spell
void rotbriar_sprout( special_effect_t& effect )
{
  struct rotbriar_sprout_t : public shadowlands_aoe_proc_t
  {
    rotbriar_sprout_t( const special_effect_t& e ) : shadowlands_aoe_proc_t( e, "rotbriar_sprout", 329548, true )
    {
      base_dd_min = e.driver()->effectN( 2 ).min( e.item );
      base_dd_max = e.driver()->effectN( 2 ).max( e.item );
    }
  };

  effect.execute_action = create_proc_action<rotbriar_sprout_t>( "rotbriar_sprout", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// id=339343 driver, buff & debuff rating values
// id=339342 buff
// id=339341 debuff
void murmurs_in_the_dark( special_effect_t& effect )
{
  auto debuff = buff_t::find( effect.player, "end_of_night" );
  if ( !debuff )
  {
    debuff = make_buff<stat_buff_t>( effect.player, "end_of_night", effect.player->find_spell( 339341 ) )
      ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );
  }

  auto buff = buff_t::find( effect.player, "fall_of_night" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "fall_of_night", effect.player->find_spell( 339342 ) )
      ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) )
      ->set_stack_change_callback( [ debuff ]( buff_t*, int, int new_ ) {
        if ( !new_ )
          debuff->trigger();
      } );
  }

  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// id=336219 driver
// id=336236 unknown use, triggered by driver
// id=336234 damage spell
// id=336222 melee damage spell?
void dueling_form( special_effect_t& effect )
{
  struct dueling_form_t : public proc_spell_t
  {
    dueling_form_t( const special_effect_t& e )
      : proc_spell_t( "duelists_shot", e.player, e.player->find_spell( 336234 ) )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
    }
  };

  effect.execute_action = create_proc_action<dueling_form_t>( "duelists_shot", effect );
  new dbc_proc_callback_t( effect.player, effect );
}

void instructors_divine_bell( special_effect_t& effect )
{
  effect.cooldown_category_ = 1141;
}

// 9.1 Trinkets

// id=356029 buff
// id=353492 driver
void forbidden_necromantic_tome( special_effect_t& effect )  // NYI: Battle Rezz Ghoul
{

  struct forbidden_necromantic_tome_callback_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* ) override
    {
      if ( proc_buff->at_max_stacks() )
        return;

      proc_buff->trigger();
    }
  };

  auto buff = debug_cast< stat_buff_t* >( buff_t::find( effect.player, "forbidden_knowledge" ) );

  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "forbidden_knowledge", effect.player -> find_spell( 356029 ))
           ->add_stat( STAT_CRIT_RATING, effect.driver() ->effectN( 1 ).average( effect.item ) );

    
    effect.custom_buff = buff;
    new forbidden_necromantic_tome_callback_t( effect.player, effect );
  }
}

void soul_cage_fragment( special_effect_t& effect )
{
  auto buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "torturous_might" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "torturous_might", effect.driver()->effectN( 1 ).trigger() )
           ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

void decanter_of_endless_howling( special_effect_t& effect )
{
  effect.proc_flags2_ = PF2_CRIT;
  auto buff           = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "decanted_warsong" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "decanted_warsong", effect.player->find_spell( 356687 ) )
               ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI ),
                           effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

void tome_of_monstrous_constructions( special_effect_t& effect ) // TODO: Create buff that tracks the name of the mob, and only trigger on mobs with the same name.
{
  // Assume the player keeps the buff up on its own as the item is off-gcd and
  // the buff has a 60 minute duration which is enough for any encounter.
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct tome_of_monstrous_constructions_t : public proc_spell_t
  {
    tome_of_monstrous_constructions_t( const special_effect_t& e )
      : proc_spell_t( "studious_comprehension", e.player, e.player -> find_spell( 357168 ), e.item )
    {
      base_dd_min = e.player -> find_spell( 357169 ) -> effectN( 1 ).min( e.item );
      base_dd_max = e.player-> find_spell( 357169 ) -> effectN( 1 ).max( e.item );
    }
  };

  auto secondary      = new special_effect_t( effect.item );
  secondary->type     = SPECIAL_EFFECT_EQUIP;
  secondary->source   = SPECIAL_EFFECT_SOURCE_ITEM;
  secondary->spell_id = 357163;
  secondary->execute_action =
      create_proc_action<tome_of_monstrous_constructions_t>( "studious_comprehension", *secondary );
  effect.player->special_effects.push_back( secondary );

  auto callback = new dbc_proc_callback_t( effect.item, *secondary );
  callback->initialize();
}

void tormentors_rack_fragment( special_effect_t& effect )
{
  struct excruciating_twinge_t : public proc_spell_t
  {
    excruciating_twinge_t( const special_effect_t& e )
      : proc_spell_t( "excruciating_twinge", e.player, e.player->find_spell( 356181 ), e.item )
    {
      // 2021-07-19 -- Logs show that this was hotfixed to hit again on refresh, but not in spell data
      tick_zero = true;
      base_td = e.driver()->effectN( 1 ).average( e.item );
    }
  };

  struct tormentors_rack_fragment_callback_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      proc_action->target = target( state );
      proc_action->schedule_execute();
    }
  };

  auto p    = effect.player;
  auto buff = make_buff<stat_buff_t>( p, "shredded_soul", p->find_spell( 356281 ) );
  buff->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );

  p->register_on_kill_callback( [ p, buff ]( player_t* t ) {
    if ( p->sim->event_mgr.canceled )
      return;

    auto d = t->find_dot( "excruciating_twinge", p );
    if ( d && d->remains() > 0_ms )
      buff->trigger();  // Going to assume that the player picks this up automatically, might need to add delay.
  } );

  effect.execute_action = create_proc_action<excruciating_twinge_t>( "excruciating_twinge", effect );

  new tormentors_rack_fragment_callback_t( effect.player, effect );
}

void old_warriors_soul( special_effect_t& effect )
{
  auto buff   = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "undying_rage" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "undying_rage", effect.player->find_spell( 356492 ) )
               ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;
    effect.player->register_combat_begin( [ &effect, buff ]( player_t* ) {
      buff->trigger();
      make_repeating_event( buff->source->sim, effect.player->find_spell( 356490 )->effectN( 1 ).period(),
                            [ buff ]() { buff->trigger(); } );
    } );
  }
}

void salvaged_fusion_amplifier( special_effect_t& effect)
{
  struct salvaged_fusion_amplifier_damage_t : public generic_proc_t
  {
    salvaged_fusion_amplifier_damage_t( const special_effect_t& e ) :
      generic_proc_t( e, "fusion_amplification", 355605 )
    {
      base_dd_min = e.driver() -> effectN( 1 ).average( e.item );
      base_dd_max = e.driver() -> effectN( 1 ).average( e.item );
    }
  };

  struct salvaged_fusion_amplifier_cb_t : public dbc_proc_callback_t
  {
    salvaged_fusion_amplifier_damage_t* damage;
    buff_t* buff;

    salvaged_fusion_amplifier_cb_t( const special_effect_t& effect, action_t* d, buff_t* b )
      : dbc_proc_callback_t( effect.player, effect ),
        damage( debug_cast<salvaged_fusion_amplifier_damage_t*>( d ) ),
        buff( b )
    {
    }

    void execute( action_t*, action_state_t* trigger_state ) override
    {
      if ( buff->check() )
      {
        damage->set_target( trigger_state->target );
        damage->execute();
      }
    }
  };

  auto buff = buff_t::find( effect.player, "salvaged_fusion_amplifier" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "salvaged_fusion_amplifier", effect.driver() );
    // The actual buff needs to always trigger
    buff->set_chance( 1.0 );
  }

  action_t* damage = create_proc_action<salvaged_fusion_amplifier_damage_t>( "fusion_amplification", effect );

  effect.custom_buff = buff;
  effect.disable_action();

  // since the on-use effect doesn't use the rppm, set to 0 so trinket expressions correctly determine it has a cooldown
  effect.ppm_ = 0;

  auto cb_driver          = new special_effect_t( effect.player );
  cb_driver->name_str     = "salvaged_fusion_amplifier_driver";
  cb_driver->spell_id     = 355333;
  cb_driver->cooldown_    = 0_s;
  cb_driver->proc_flags_  = effect.driver()->proc_flags();
  cb_driver->proc_flags2_ = PF2_ALL_HIT; // As this triggers from white+yellow hits not just casts, use ALL_HIT
  effect.player->special_effects.push_back( cb_driver );

  [[maybe_unused]] auto callback = new salvaged_fusion_amplifier_cb_t( *cb_driver, damage, buff );

  timespan_t precast = effect.player->sim->shadowlands_opts.salvaged_fusion_amplifier_precast;
  if ( precast > 0_s )
  {
    effect.player->register_combat_begin( [ &effect, buff, precast ]( player_t* ) {
      buff->trigger( buff->buff_duration() - precast );

      cooldown_t* cd = effect.player->get_cooldown( effect.cooldown_name() );
      cd->start( effect.cooldown() - precast );

      cooldown_t* group_cd = effect.player->get_cooldown( effect.cooldown_group_name() );
      group_cd->start( effect.cooldown_group_duration() - precast );
    } );
  }
}

void miniscule_mailemental_in_an_envelope( special_effect_t& effect )
{
  struct unstable_goods_t : public proc_spell_t
  {
    unstable_goods_t( const special_effect_t& e )
      : proc_spell_t( "unstable_goods", e.player, e.player->find_spell( 352542 ) )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
    }
  };

  struct unstable_goods_callback_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      proc_action->target = target( state );
      proc_action->schedule_execute();
    }
  };

  effect.execute_action = create_proc_action<unstable_goods_t>( "unstable_goods", effect );

  new unstable_goods_callback_t( effect.player, effect );
}

/**Titanic Ocular Gland
 * id=355313 coefficients for stat amounts
 * id=355794 "Worthy" buff
 * id=355951 "Unworthy" debuff
 */
void titanic_ocular_gland( special_effect_t& effect )
{
  // When selecting the highest stat, the priority of equal secondary stats is Vers > Mastery > Haste > Crit.
  static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };

  // Use a separate buff for each rating type so that individual uptimes are reported nicely and APLs can easily reference them.
  // Store these in pointers to reduce the size of the events that use them.
  auto worthy_buffs = std::make_shared<std::map<stat_e, buff_t*>>();
  auto unworthy_buffs = std::make_shared<std::map<stat_e, buff_t*>>();
  double amount = effect.driver()->effectN( 1 ).average( effect.item );

  for ( auto stat : ratings )
  {
    auto name = std::string( "worthy_" ) + util::stat_type_string( stat );
    buff_t* buff = buff_t::find( effect.player, name );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( 355794 ), effect.item )
               ->add_stat( stat, amount )
               ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
               ->set_can_cancel( false );
    }
    ( *worthy_buffs )[ stat ] = buff;

    name = std::string( "unworthy_" ) + util::stat_type_string( stat );
    buff = buff_t::find( effect.player, name );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( 355951 ), effect.item )
               ->add_stat( stat, -amount )
               ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
               ->set_can_cancel( false );
    }
    ( *unworthy_buffs )[ stat ] = buff;
  }

  auto update_buffs = [p = effect.player, worthy_buffs, unworthy_buffs]() mutable
  {
    bool worthy = p->rng().roll( p->sim->shadowlands_opts.titanic_ocular_gland_worthy_chance );
    bool buff_active = false;
    stat_e max_stat = util::highest_stat( p, ratings );

    // Iterate over all of the buffs and expire any that should not be active. Only one buff is
    // active at a time, so this process only needs to continue until a single active buff is found.
    for ( auto stat : ratings )
    {
      if ( ( *worthy_buffs )[ stat ]->check() )
      {
        // The Worthy buff will update to a different stat if that stat becomes higher. 
        // TODO: The buff only changes if the other stat is actually higher, otherwise the previous highest stat is used.
        if ( max_stat != stat || !worthy )
        {
          ( *worthy_buffs )[ stat ]->expire();
          max_stat = util::highest_stat( p, ratings );
        }
        else
        {
          buff_active = true;
        }
        break;
      }
      if ( ( *unworthy_buffs )[ stat ]->check() )
      {
        // The Unworthy debuff will never update its stat.
        if ( worthy )
        {
          ( *unworthy_buffs )[ stat ]->expire();
          max_stat = util::highest_stat( p, ratings );
        }
        else
        {
          buff_active = true;
        }
        break;
      }
    }

    if ( !buff_active )
    {
      if ( worthy )
        ( *worthy_buffs )[ max_stat ]->execute();
      else
        ( *unworthy_buffs )[ max_stat ]->execute();
    }
  };

  // While above the health threshold, this trinket appears to check your secondary stats every 5.25 seconds.
  // Also use this as an interval for checking whether the player is above the health threshold to toggle between
  // Worthy and Unworthy. In game, the buffs will actually switch instantly as soon as the player's health changes.
  effect.player->register_combat_begin( [update_buffs]( player_t* p ) mutable
  {
    timespan_t period = 5.25_s;
    timespan_t first_update = p->rng().real() * period;
    update_buffs();
    make_event( p->sim, first_update, [period, update_buffs, p]() mutable
    {
      update_buffs();
      make_repeating_event( p->sim, period, update_buffs );
    } );
  } );

  // right-rotate to place update_buff at the front of all combat_begin callbacks, to replicate in-game behavior where
  // the worthy buff is already present on the player before entering combat.
  auto vec = &effect.player->combat_begin_functions;
  std::rotate( vec->rbegin(), vec->rbegin() + 1, vec->rend() );
}

/**Ebonsoul Vise
 * id=355327 damage debuff
 * id=357785 crit pickup buff
 */
void ebonsoul_vise( special_effect_t& effect )
{
  struct ebonsoul_vise_t : public proc_spell_t
  {
    double max_duration_bonus;

    ebonsoul_vise_t( const special_effect_t& e )
      : proc_spell_t( "ebonsoul_vise", e.player, e.driver(), e.item ),
        max_duration_bonus( e.driver()->effectN( 3 ).percent() )
    {
      base_td = e.driver()->effectN( 1 ).average( e.item );
    }

    timespan_t composite_dot_duration( const action_state_t* state ) const override
    {
      auto target      = state->target;
      double bonus_mul = 1.0 - ( target->is_active() ? target->health_percentage() * 0.01 : 0.0 );

      timespan_t new_duration = dot_duration * ( 1 + max_duration_bonus * bonus_mul );

      return new_duration;
    }
  };

  player_t* p = effect.player;
  buff_t* buff = make_buff<stat_buff_t>( p, "shredded_soul_ebonsoul_vise", p->find_spell( 357785 ) )
                     ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );

  p->register_on_kill_callback( [ p, buff ]( player_t* t ) {
    if ( p->sim->event_mgr.canceled )
      return;

    auto d = t->find_dot( "ebonsoul_vise", p );
    bool picked_up = p->rng().roll( p->sim->shadowlands_opts.shredded_soul_pickup_chance );

    // TODO: handle potential movement required to pick up the soul
    if ( d && d->remains() > 0_ms && picked_up )
      buff->trigger();
  } );

  effect.execute_action = create_proc_action<ebonsoul_vise_t>( "ebonsoul_vise", effect );
}

/**Shadowed Orb of Torment
 * id=355321 driver
 * id=356326 mastery buff
 * id=356334 mastery buff value
 */
void shadowed_orb_of_torment( special_effect_t& effect )
{
  struct tormented_insight_channel_t : public proc_spell_t
  {
    buff_t* buff;
    action_t* use_action;  // if this exists, then we're prechanneling via the APL

    tormented_insight_channel_t( const special_effect_t& e, buff_t* b )
      : proc_spell_t( "tormented_insight", e.player, e.driver(), e.item ), buff( b ), use_action( nullptr )
    {
      // Override this so it doesn't trigger self-harm portion of the trinket
      base_td      = 0;
      effect       = &e;
      channeled    = true;
      hasted_ticks = harmful = false;

      for ( auto a : player->action_list )
      {
        if ( a->action_list && a->action_list->name_str == "precombat" && a->name_str == "use_item_" + item->name_str )
        {
          a->harmful = harmful;  // pass down harmful to allow action_t::init() precombat check bypass
          use_action = a;
          use_action->base_execute_time = 2_s;
          break;
        }
      }
    }

    void precombat_buff()
    {
      timespan_t time = sim->shadowlands_opts.shadowed_orb_of_torment_precombat_channel;

      if ( time == 0_ms )  // No global override, check for an override from an APL variable
      {
        for ( auto v : player->variables )
        {
          if ( v->name_ == "shadowed_orb_of_torment_precombat_channel" )
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
        time     = 2_s;  // base 2s channel for full effect
        const auto& apl = player->precombat_action_list;

        auto it = range::find( apl, use_action );
        if ( it == apl.end() )
        {
          sim->print_debug(
              "WARNING: Precombat /use_item for Shadowed Orb of Torment exists but not found in precombat APL!" );
          return;
        }

        cdgrp->start( 1_ms );  // tap the shared group cd so we can get accurate action_ready() checks

        // add cast time or gcd for any following precombat action
        std::for_each( it + 1, apl.end(), [ &time, this ]( action_t* a ) {
          if ( a->action_ready() )
          {
            timespan_t delta =
                std::max( std::max( a->base_execute_time, a->trigger_gcd ) * a->composite_haste(), a->min_gcd );
            sim->print_debug( "PRECOMBAT: Shadowed Orb of Torment prechannel timing pushed by {} for {}", delta,
                              a->name() );
            time += delta;

            return a->harmful;  // stop processing after first valid harmful spell
          }
          return false;
        } );
      }

      // how long you channel for (rounded down to seconds)
      auto channel = std::min( 2_s, timespan_t::from_seconds( static_cast<int>( time.total_seconds() ) ) );
      // total duration of the buff from channeling. Base is 10s for each tick of the channel (2 ticks per second)
      auto total = buff->buff_duration() * ( channel.total_seconds() * 2 );
      // actual duration of the buff you'll get in combat
      auto actual = total + channel - time;
      // cooldown on effect/trinket at start of combat
      auto cd_dur = cooldown->duration - time;
      // shared cooldown at start of combat
      auto cdgrp_dur = std::max( 0_ms, effect->cooldown_group_duration() - time );

      sim->print_debug(
          "PRECOMBAT: Shadowed Orb of Torment started {}s before combat via {}, channeled for {}s, {}s in-combat buff",
          time, use_action ? "APL" : "SHADOWLANDS_OPT", channel, actual );

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, actual );

      if ( use_action )  // from the apl, so cooldowns will be started by use_item_t. adjust. we are still in precombat.
      {
        make_event( *sim, [ this, time, cdgrp ] {  // make an event so we adjust after cooldowns are started
          cooldown->adjust( -time );

          if ( use_action )
            use_action->cooldown->adjust( -time );

          cdgrp->adjust( -time );
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

    timespan_t tick_time( const action_state_t* ) const override
    {
      return base_tick_time;
    }

    void trigger_dot( action_state_t* s ) override
    {
      if ( player->in_combat )  // only trigger channel 'dot' in combat
      {
        proc_spell_t::trigger_dot( s );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( player->in_combat )  // only channel in-combat
      {
        event_t::cancel( player->readying );
        player->reset_auto_attacks( data().duration() );
      }
      else  // if precombat...
      {
        if ( use_action )  // ...and use_item exists in the precombat apl
        {
          precombat_buff();
        }
      }
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;
      timespan_t duration = d->current_tick * buff->buff_duration();

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
      proc_spell_t::last_tick( d );

      if ( was_channeling && !player->readying )
      {
        player->schedule_ready( rng().gauss( sim->channel_lag, sim->channel_lag_stddev ) );
      }
    }
  };

  buff_t* buff = buff_t::find( effect.player, "tormented_insight" );
  if ( !buff )
  {
    double val = effect.player->find_spell( 356334 )->effectN( 1 ).average( effect.item );
    buff       = make_buff<stat_buff_t>( effect.player, "tormented_insight", effect.player->find_spell( 356326 ) )
               ->add_stat( STAT_MASTERY_RATING, val );
  }

  auto action           = new tormented_insight_channel_t( effect, buff );
  effect.execute_action = action;

  // pre-combat channeling hack via shadowlands options
  if ( effect.player->sim->shadowlands_opts.shadowed_orb_of_torment_precombat_channel > 0_ms )  // option is set
  {
    effect.player->register_combat_begin( [ action ]( player_t* ) {
      if ( !action->use_action )  // no use_item in precombat apl, so we apply the buff on combat start
        action->precombat_buff();
    } );
  }
}

/**Relic of the Frozen Wastes
  (355301) Relic of the Frozen Wastes (equip):
    chance to deal damage (effect 1) and apply debuff Frozen Heart (355759)
  (355303) Frostlord's Call (use):
    (355912, damage from equip effect 2) deals physical damage to target
    (357409, damage from equip effect 3) deals frost damage to all targets between the player and target inclusive with Frozen Heart debuff
 */
void relic_of_the_frozen_wastes_use( special_effect_t& effect )
{
  struct frost_tinged_carapace_spikes_t : public generic_proc_t
  {
    frost_tinged_carapace_spikes_t( const special_effect_t& effect, const spell_data_t* equip )
      : generic_proc_t( effect, "frosttinged_carapace_spikes", effect.player->find_spell( 357409 ) )
    {
      background = dual = true;
      callbacks = false;
      aoe = -1;
      base_dd_max = base_dd_min = equip->effectN( 3 ).average( effect.item );
    }

    void execute() override
    {
      target_cache.is_valid = false;

      generic_proc_t::execute();
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      tl.clear();

      for ( auto* t : sim->target_non_sleeping_list )
      {
         if ( t->is_enemy() && player->get_target_data( t )->debuff.frozen_heart->up() )
          tl.push_back( t );
      }

      return tl.size();
    }
  };

  struct nerubian_ambush_t : public generic_proc_t
  {
    action_t* splash;

    nerubian_ambush_t( const special_effect_t& effect, const spell_data_t* equip ) :
      generic_proc_t( effect, "nerubian_ambush", effect.player->find_spell(355912) )
    {
      background = dual = true;
      callbacks = false;
      base_dd_max = base_dd_min = equip->effectN( 2 ).average( effect.item );
    }
  };

  struct frostlords_call_t : public generic_proc_t
  {
    action_t* ambush;
    action_t* spikes;

    frostlords_call_t( const special_effect_t& effect, const spell_data_t* equip )
      : generic_proc_t( effect, "frostlords_call", effect.driver() ),
        ambush( create_proc_action<nerubian_ambush_t>( "nerubian_ambush", effect, equip ) ),
        spikes( create_proc_action<frost_tinged_carapace_spikes_t>( "frosttinged_carapace_spikes", effect, equip ) )
    {
      callbacks = false;
      travel_speed = 20;

      add_child( ambush );
      add_child( spikes );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      ambush->set_target( s->target );
      ambush->execute();

      spikes->set_target( s->target );
      spikes->execute();
    }
  };

  const spell_data_t* equip = effect.player->find_spell( 355301 );
  effect.execute_action = create_proc_action<frostlords_call_t>( "frostlords_call", effect, equip );
}

void relic_of_the_frozen_wastes_equip( special_effect_t& effect )
{
  struct frozen_heart_t : generic_proc_t
  {
    frozen_heart_t( const special_effect_t& effect ) : generic_proc_t( effect, "frozen_heart", effect.trigger() )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    }

    void execute() override
    {
      generic_proc_t::execute();

      actor_target_data_t* td = player->get_target_data( target );
      td->debuff.frozen_heart->trigger();
    }
  };

  effect.execute_action = create_proc_action<frozen_heart_t>( "frozen_heart", effect );
  new dbc_proc_callback_t( effect.player, effect );
}

/**Ticking Sack of Terror
* 9.1 version
  (351679) driver, damage on effect 1
  (351682) debuff
  (351694) fire damage at 3 stacks
  9.2 version
  (367901) driver, damage on effect 1
  (367902) debuff
  (367903) fire damage at 3 stacks
 */
void ticking_sack_of_terror( special_effect_t& effect )
{
  struct volatile_detonation_t : generic_proc_t
  {
    volatile_detonation_t( const special_effect_t& effect )
      : generic_proc_t(
            effect, "volatile_detonation",
            ( effect.spell_id == 351679 ? effect.player->find_spell( 351694 ) : effect.player->find_spell( 367903 ) ) )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    }
  };

  struct volatile_satchel_cb_t : dbc_proc_callback_t
  {
    action_t* damage;

    volatile_satchel_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.player, effect ),
        damage( create_proc_action<volatile_detonation_t>( "volatile_detonation", effect ) )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      actor_target_data_t* td = a->player->get_target_data( s->target );

      // Damage is dealt when the debuff falls off *or* reaches max stacks, whichever comes first
      // This needs to be handled via a stack_change_callback, but can't be set in register_target_data_initializer
      if ( !td->debuff.volatile_satchel->stack_change_callback )
      {
        td->debuff.volatile_satchel->set_stack_change_callback( [ this ]( buff_t* b, int /* old_ */, int new_ ) {
          if ( new_ == 0 )
          {
            damage->set_target( b->player );
            damage->execute();
          }
        } );
      }

      if ( td->debuff.volatile_satchel->at_max_stacks() )
        td->debuff.volatile_satchel->expire();
      else
        td->debuff.volatile_satchel->trigger();
    }
  };

  new volatile_satchel_cb_t( effect );
}

// 9.1 version
// id=351926 driver
// id=351927 hold stat amount
// id=351952 buff
// 9.2 version
// id=368509 driver
// id=368513 hold stat amount
// id=368512 buff
// TODO: implement external buff to simulate being an ally
void soleahs_secret_technique( special_effect_t& effect )
{
  // Assuming you don't actually use this trinket during combat but rather beforehand
  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.cooldown_ = 0_s;

  std::string_view opt_str = effect.player->sim->shadowlands_opts.soleahs_secret_technique_type;
  // Override with player option if defined
  if ( !effect.player->shadowlands_opts.soleahs_secret_technique_type.is_default() )
  {
    opt_str = effect.player->shadowlands_opts.soleahs_secret_technique_type;
  }

  if ( util::str_compare_ci( opt_str, "none" ) )
    return;

  auto val = effect.spell_id == 351926 
      ? effect.player->find_spell( 351927 )->effectN( 1 ).average( effect.item ) 
      : effect.player->find_spell( 368513 )->effectN( 1 ).average( effect.item );
  auto buff_spell = effect.spell_id == 351926
                        ? effect.player->find_spell( 351952 ) 
                        : effect.player->find_spell( 368512 );

  buff_t* buff;

  if ( util::str_compare_ci( opt_str, "haste" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_haste" );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_haste", buff_spell )
              ->add_stat( STAT_HASTE_RATING, val );
    }
  }
  else if ( util::str_compare_ci( opt_str, "crit" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_crit" );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_crit", buff_spell )
              ->add_stat( STAT_CRIT_RATING, val );
    }
  }
  else if ( util::str_compare_ci( opt_str, "versatility" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_versatility" );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_versatility", buff_spell )
              ->add_stat( STAT_VERSATILITY_RATING, val );
    }
  }
  else if ( util::str_compare_ci( opt_str, "mastery" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_mastery" );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_mastery", buff_spell )
              ->add_stat( STAT_MASTERY_RATING, val );
    }
  }
  else
  {
    buff = nullptr;
  }

  if ( buff )
    effect.player->register_combat_begin( buff );
  else
    effect.player->sim->error( "Warning: Invalid type '{}' for So'leah's Secret Technique, ignoring.", opt_str );
}


/**Reactive Defense Matrix
 * id=356813 buff
 * id=355329 proc/reflect amount
 * id=356857 damage effect
 */
void reactive_defense_matrix( special_effect_t& effect )
{
  struct reactive_defense_matrix_t : generic_proc_t
  {
    reactive_defense_matrix_t( const special_effect_t& e ) :
      generic_proc_t( e, "reactive_defense_matrix", e.player->find_spell( 356857 ) )
    {
      base_dd_min = base_dd_max = 1.0; // Ensure that the correct snapshot flags are set.
    }
  };

  struct reactive_defense_matrix_absorb_buff_t : absorb_buff_t
  {
    action_t* damage_action;

    reactive_defense_matrix_absorb_buff_t( const special_effect_t& e, action_t* a ) :
      absorb_buff_t( e.player, "reactive_defense_matrix", e.player->find_spell( 356813 ) ),
      damage_action( a )
    {
      // This cooldown is only for the proc that occurs when falling below 20% HP.
      // If the cooldown is present, it will prevent the buff from proccing normally.
      set_cooldown( 0_ms );
      set_absorb_source( e.player->get_stats( "reactive_defense_matrix_absorb" ) );
    }

    void absorb_used( double amount ) override
    {
      // TODO: This should be the target that dealt the damage instead of the players target.
      // This would also need to be tested to see in which cases the damage is not done at all.
      if ( player->target )
        damage_action->execute_on_target( player->target, amount );
    }
  };

  struct reactive_defense_matrix_absorb_t : absorb_t
  {
    buff_t* buff;

    reactive_defense_matrix_absorb_t( const special_effect_t& e, buff_t* b ) :
      absorb_t( "reactive_defense_matrix_absorb", e.player, e.player->find_spell( 356813 ) ),
      buff( b )
    {
      background = true;
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }

    absorb_buff_t* create_buff( const action_state_t* ) override
    {
      return debug_cast<absorb_buff_t*>( buff );
    }
  };

  action_t* damage_action = create_proc_action<reactive_defense_matrix_t>( "reactive_defense_matrix", effect );
  buff_t* buff = buff_t::find( effect.player, "reactive_defense_matrix" );
  if ( !buff )
    buff = make_buff<reactive_defense_matrix_absorb_buff_t>( effect, damage_action );

  action_t* proc_action = new reactive_defense_matrix_absorb_t( effect, buff );
  effect.execute_action = proc_action;
  new dbc_proc_callback_t( effect.player, effect );

  auto period = effect.player->sim->shadowlands_opts.reactive_defense_matrix_interval;
  if ( period > 0_ms )
  {
    cooldown_t* cooldown = effect.player->get_cooldown( "reactive_defense_matrix" );
    cooldown->duration = timespan_t::from_seconds( effect.driver()->effectN( 3 ).base_value() );
    effect.player->register_combat_begin( [ cooldown, proc_action, period ]( player_t* p ) mutable
    {
      make_repeating_event( p->sim, period, [ cooldown, proc_action ]()
      {
        if ( cooldown->down() )
          return;

        cooldown->start();
        proc_action->execute();
      } );
    } );
  }
}

// 9.2 Trinkets

void extract_of_prodigious_sands( special_effect_t& effect )
{
  auto damage =
      create_proc_action<generic_proc_t>( "prodigious_sands_damage", effect, "prodigious_sands_damage", 367971 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
  damage->background = damage->dual = true;

  effect.execute_action = create_proc_action<proc_spell_t>( "prodigious_sands", effect );
  effect.execute_action->impact_action = damage;
  damage->stats = effect.execute_action->stats;

  new dbc_proc_callback_t( effect.player, effect );
}


void brokers_lucky_coin( special_effect_t& effect )
{
  struct lucky_flip_callback_t : public dbc_proc_callback_t
  {
    stat_buff_t* heads;
    stat_buff_t* tails;

    lucky_flip_callback_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        heads( make_buff<stat_buff_t>( effect.player, "heads", effect.player->find_spell( 367466 ) ) ),
        tails( make_buff<stat_buff_t>( effect.player, "tails", effect.player->find_spell( 367467 ) ) )
    {
      // Values in the stat buffs are set to 0 and passed down from the trigger spell aura
      heads->stats[ 0 ].amount = e.driver()->effectN( 1 ).average( effect.item );
      tails->stats[ 0 ].amount = e.driver()->effectN( 1 ).average( effect.item );
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( rng().roll( 0.5 ) )
        heads->trigger();
      else
        tails->trigger();
    }
  };

  new lucky_flip_callback_t( effect );
}

void symbol_of_the_lupine( special_effect_t& effect )
{
  effect.execute_action =
      create_proc_action<generic_proc_t>( "lupines_slash", effect, "lupines_slash", effect.trigger() );
  effect.execute_action->base_td = effect.driver()->effectN( 1 ).average( effect.item );

  new dbc_proc_callback_t( effect.player, effect );
}

void scars_of_fraternal_strife( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs(
           effect, { "scars_of_fraternal_strife_1", "scars_of_fraternal_strife_2", "scars_of_fraternal_strife_3",
                     "scars_of_fraternal_strife_4", "scars_of_fraternal_strife_5" } ) )
    return;

  struct apply_rune_t : public proc_spell_t
  {
    struct first_rune_t : public stat_buff_t
    {
      first_rune_t( const special_effect_t& e )
        : stat_buff_t( e.player, "scars_of_fraternal_strife_1", e.player->find_spell( 368635 ), e.item )
      {
        name_str_reporting = "the_first_rune";

        // timespan_t echo_delay = timespan_t::from_seconds( data().effectN( 3 ).base_value() ); NYI
        // double echo_pct = data().effectN( 2 ).percent();
        // auto echo = new proc_spell_t( "the_first_rune", e.player, e.player->find_spell( 368850 ) );
      }
    };

    struct second_rune_t : public stat_buff_t
    {
      second_rune_t( const special_effect_t& e )
        : stat_buff_t( e.player, "scars_of_fraternal_strife_2", e.player->find_spell( 368636 ), e.item )
      {
        name_str_reporting = "the_second_rune";

        // auto heal_reduction = data().effectN( 2 ).percent(); NYI
      }
    };

    struct third_rune_t : public stat_buff_t
    {
      third_rune_t( const special_effect_t& e )
        : stat_buff_t( e.player, "scars_of_fraternal_strife_3", e.player->find_spell( 368637 ), e.item )
      {
        name_str_reporting = "the_third_rune";

        /* Disabled for now
        auto bleed = new proc_spell_t( "the_third_rune_bleed", e.player, &data(), item );
        bleed->dot_duration = timespan_t::from_seconds( sim->expected_max_time() * 2.0 );
        bleed->snapshot_flags &= STATE_NO_MULTIPLIER;
        bleed->update_flags &= STATE_NO_MULTIPLIER;

        set_stack_change_callback( [ bleed ]( buff_t* b, int, int new_ ) {
          if ( new_ )
            bleed->execute_on_target( b->player );
          else
            bleed->get_dot( b->player )->cancel();
        } ); */
      }
    };

    struct fourth_rune_t : public stat_buff_t
    {
      fourth_rune_t( const special_effect_t& e )
        : stat_buff_t( e.player, "scars_of_fraternal_strife_4", e.player->find_spell( 368638 ), e.item )
      {
        name_str_reporting = "the_fourth_rune";

        // auto snare = make_buff( e.player, "the_fourth_rune", e.player->find_spell( 368639 ) ); NYI
      }
    };

    struct final_rune_t : public stat_buff_t
    {
      final_rune_t( const special_effect_t& e, apply_rune_t* a )
        : stat_buff_t( e.player, "scars_of_fraternal_strife_5", e.player->find_spell( 368641 ), e.item )
      {
        name_str_reporting = "the_final_rune";

        auto burst = create_proc_action<generic_aoe_proc_t>( "the_final_rune", e, "the_final_rune", 368642 );
        burst->split_aoe_damage = false;

        set_stack_change_callback( [ a, burst ]( buff_t* buff, int, int new_ ) {
          if ( !new_ )
          {
            burst->execute_on_target( buff->player->target );
          }
          else
          {
            range::for_each( a->buffs, [ buff ]( buff_t* b ) {
              if ( b != buff )
                b->expire();
            } );
          }
        } );
      }
    };

    std::vector<buff_t*> buffs;
    buff_t* first;
    cooldown_t* shared_item_cd;

    apply_rune_t( const special_effect_t& e )
      : proc_spell_t( e ), shared_item_cd( player->get_cooldown( "item_cd_1141" ) )
    {
      harmful = false;

      first = buffs.emplace_back( make_buff<first_rune_t>( e ) );
      buffs.push_back( make_buff<second_rune_t>( e ) );
      buffs.push_back( make_buff<third_rune_t>( e ) );
      buffs.push_back( make_buff<fourth_rune_t>( e ) );
      buffs.push_back( make_buff<final_rune_t>( e, this ) );
    }

    void add_another()
    {
      buffs.front()->trigger();
      // Using the Final Rune triggers the shared Trinket CD
      if ( buffs.front()->data().id() == 368641 )
      {
        shared_item_cd->start( player->default_item_group_cooldown );
        sim->print_debug( "{} starts shared cooldown for {} ({}). Will be ready at {}", *player, name(),
                          shared_item_cd->name(), shared_item_cd->ready );
      }

      std::rotate( buffs.begin(), buffs.begin() + 1, buffs.end() );
    }

    void execute() override
    {
      proc_spell_t::execute();

      add_another();
    }

    void reset() override
    {
      proc_spell_t::reset();

      std::rotate( buffs.begin(), range::find( buffs, first ), buffs.end() );
    }
  };

  effect.type = SPECIAL_EFFECT_USE;

  auto apply_rune = create_proc_action<apply_rune_t>( "scars_of_fraternal_strife", effect );
  effect.execute_action = apply_rune;

  // Currently the first rune does not get removed on encounter start, allowing you to pre-use the trinket 30s before
  // and immediately apply the 2nd rune once combat starts.
  // TODO: Remove this if this behavior is a bug and is fixed.
  effect.player->register_combat_begin( [ apply_rune ]( player_t* ) {
    debug_cast<apply_rune_t*>( apply_rune )->add_another();
  });
}

// pet cast: 368203
// pet spell damage coeff: 367307
// CDR buff: 368937
// TODO: what happens if another Automa dies near you?
void architects_ingenuity_core( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "architects_ingenuity" } ) )
    return;

  struct architects_ingenuity_t : public proc_spell_t
  {
    buff_t* buff;
    double recharge_multiplier;
    std::vector<action_t*> cd_actions;

    architects_ingenuity_t( const special_effect_t& e )
      : proc_spell_t( "architects_ingenuity", e.player, e.player->find_spell( 368203 ) ),
        buff( buff_t::find( e.player, "architects_ingenuity" ) ),
        recharge_multiplier( 1.0 / ( 1 + e.driver()->effectN( 2 ).trigger()->effectN( 2 ).percent() ) )
    {
      base_td = data().effectN( 2 ).trigger()->effectN( 1 ).average( e.item );

      if ( !buff )
      {
        buff = make_buff( e.player, "architects_ingenuity", e.player->find_spell( 368937 ) );
      }
    }

    void init() override
    {
      proc_spell_t::init();

      for ( auto a : player->action_list )
      {
        // TODO: On the PTR this only affected class spells and did not affect the cooldown of charged
        // spells. Is this still the case?
        if ( a->cooldown->duration != 0_ms && a->data().class_mask() != 0 && a->data().charges() == 0 )
        {
          cd_actions.push_back( a );
        }
      }

      buff->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
        for ( auto a : cd_actions )
        {
          if ( new_ == 1 )
            a->base_recharge_multiplier *= recharge_multiplier;
          else
            a->base_recharge_multiplier /= recharge_multiplier;
          a->cooldown->adjust_recharge_multiplier();
        }
      } );
    }

    void last_tick( dot_t* d ) override
    {
      proc_spell_t::last_tick( d );

      buff->trigger();
    }
  };

  effect.execute_action = create_proc_action<architects_ingenuity_t>( "architects_ingenuity", effect );
  effect.cooldown_group_name_override = "item_cd_1141_gcd";
  effect.cooldown_group_duration_override = effect.driver()->gcd();
}

// TODO: extremely annoying to do as none of these things show up on the combat log
//   1) confirm delay between repeated launches
//   2) confirm delay between missile hitting the ground, expanding the ring, and the ring hitting the target
void resonant_reservoir( special_effect_t& effect )
{
  struct disintegration_halo_t : public proc_spell_t
  {
    struct disintegration_halo_counter_t : public buff_t
    {
      disintegration_halo_counter_t( const special_effect_t& e, std::string_view n, unsigned i )
        : buff_t( e.player, n, e.player->find_spell( i ), e.item )
      {
        set_default_value_from_effect( 1, 1.0 );
      }
    };

    struct disintegration_halo_dot_t : public proc_spell_t
    {
      disintegration_halo_dot_t( const special_effect_t& e )
        : proc_spell_t( "disintegration_halo_dot", e.player, e.player->find_spell( 368231 ), e.item )
      {
        dual = true;
      }

      timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t t ) const override
      {
        // duration is reset on refresh but the current tick does not clip
        return t;
      }

      timespan_t travel_time() const override
      {
        // NOTE: Preliminary estimation of time it takes for circle to expand and hit the target. Note that logs DO NOT
        // show when the missile lands and the halo begins to expand, so we will have to confirm these estimations with
        // further reviews.
        return rng().gauss( 0.5_s, 0.25_s );
      }
    };

    struct disintegration_halo_missile_t : public proc_spell_t
    {
      disintegration_halo_missile_t( const special_effect_t& e, std::string_view n, unsigned i, action_t* a )
        : proc_spell_t( n, e.player, e.player->find_spell( i ), e.item )
      {
        aoe = -1;
        impact_action = a;
      }
    };

    std::vector<buff_t*> buffs;
    std::vector<action_t*> missiles;
    timespan_t repeat_time;

    disintegration_halo_t( const special_effect_t& e ) : proc_spell_t( e )
    {
      buffs.push_back( nullptr );
      buffs.push_back( make_buff<disintegration_halo_counter_t>( e, "disintegration_halo_2", 368223 ) );
      buffs.push_back( make_buff<disintegration_halo_counter_t>( e, "disintegration_halo_3", 368224 ) );
      buffs.push_back( make_buff<disintegration_halo_counter_t>( e, "disintegration_halo_4", 368225 ) );

      aoe = -1;
      impact_action = create_proc_action<disintegration_halo_dot_t>( "disintegration_halo_dot", e );
      impact_action->stats = stats;

      missiles.push_back( new disintegration_halo_missile_t( e, "disintegration_halo_2", 368232, impact_action ) );
      missiles.push_back( new disintegration_halo_missile_t( e, "disintegration_halo_3", 368233, impact_action ) );
      missiles.push_back( new disintegration_halo_missile_t( e, "disintegration_halo_4", 368234, impact_action ) );

      // NOTE: Preliminary estimation of time between repeated missile launches. Note that logs DO NOT show missile
      // firings so we will have to confirm these estimations with further log reviews.
      repeat_time = 0.333_s;
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( buffs.front() )
      {
        for ( int i = 0; i < as<int>( buffs.front()->check_value() ) - 1; i++ )
        {
          make_event( *sim, repeat_time * ( i + 1 ), [ this, i ]() {
            missiles[ i ]->execute_on_target( target );
          } );
        }
      }

      std::rotate( buffs.begin(), buffs.begin() + 1, buffs.end() );

      if ( buffs.back() )
        buffs.back()->expire();

      if ( buffs.front() )
        buffs.front()->trigger();
    }

    void reset() override
    {
      proc_spell_t::reset();

      std::rotate( buffs.begin(), range::find( buffs, nullptr ), buffs.end() );
    }
  };

  effect.type = SPECIAL_EFFECT_USE;
  effect.trigger_spell_id = 367236;
  effect.execute_action = create_proc_action<disintegration_halo_t>( "disintegration_halo", effect );
}

// This has a unique on-use depending on your covenant signature ability
// Necrolord: Starts channeling a free Fleshcraft
// Kyrian: Consumes a free charge of Phial of Serenity
// Night Fae: Resets the cooldown of Soulshape
// Venthyr: Resets the cooldown of Door of Shadows
// TODO: use a separate fleshcraft_t action to trigger the effect so that the channel is cancelled correctly
void the_first_sigil( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "the_first_sigil" } ) )
    return;

  auto buff = buff_t::find( effect.player, "the_first_sigil" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "the_first_sigil", effect.player->find_spell( 367241 ) )
               ->add_stat( STAT_VERSATILITY_RATING,
                           effect.player->find_spell( 367241 )->effectN( 1 ).average( effect.item ) );
  }

  struct the_first_sigil_t : generic_proc_t
  {
    action_t* covenant_action;
    cooldown_t* orig_cd;
    cooldown_t* dummy_cd;

    the_first_sigil_t( const special_effect_t& effect )
      : generic_proc_t( effect, "the_first_sigil", effect.trigger() ),
        covenant_action( nullptr ),
        orig_cd( cooldown ),
        dummy_cd( player->get_cooldown( "the_first_sigil_covenant" ) )
    {
    }

    void init() override
    {
      proc_spell_t::init();
      unsigned covenant_signature_id;

      // Find any covenant signature abilities in the action list
      for ( const auto& e : covenant_ability_entry_t::data( player->dbc->ptr ) )
      {
        if ( e.class_id == 0 && e.ability_type == 1 &&
             e.covenant_id == static_cast<unsigned>( player->covenant->type() ) )
        {
          covenant_signature_id = e.spell_id;
          break;
        }
      }

      for ( auto a : player->action_list )
      {
        if ( covenant_signature_id == a->data().id() )
        {
          covenant_action = a;
          break;
        }
      }
    }

    void execute() override
    {
      if ( covenant_action )
      {
        // Night Fae's Soulshape (NYI) and Venthyr's Door of Shadows (NYI) CDs are Reset
        if ( player->covenant->type() == covenant_e::NIGHT_FAE || player->covenant->type() == covenant_e::VENTHYR )
        {
          player->sim->print_debug( "{} resets cooldown of {} from the_first_sigil.", player->name(),
                                    covenant_action->name() );
          covenant_action->cooldown->reset( false );
        }
        // Necrolord's Fleshcraft and Kyrian's Phial of Serenity (NYI) cast a free use of the CD
        if ( player->covenant->type() == covenant_e::NECROLORD || player->covenant->type() == covenant_e::KYRIAN )
        {
          player->sim->print_debug( "{} casts a free {} from the_first_sigil.", player->name(),
                                    covenant_action->name() );
          // TODO: don't alter the cooldown of Fleshcraft if it is off cooldown when we execute it
          covenant_action->execute();
          make_event( *sim, player->sim->shadowlands_opts.the_first_sigil_fleshcraft_cancel_time,
                      [ this ] { covenant_action->cancel(); } );
        }
      }
    }
  };

  effect.custom_buff    = buff;
  effect.execute_action = create_proc_action<the_first_sigil_t>( "the_first_sigil", effect );
}

void cosmic_gladiators_resonator( special_effect_t& effect )
{
  struct gladiators_resonator_damage_t : shadowlands_aoe_proc_t
  {
    gladiators_resonator_damage_t( const special_effect_t& effect )
      : shadowlands_aoe_proc_t( effect, "gladiators_resonator", effect.driver()->effectN( 2 ).trigger(), true )
    {
      dual = split_aoe_damage = true;
      max_scaling_targets = as<unsigned>( effect.driver()->effectN( 3 ).base_value() );
    }
  };

  struct gladiators_resonator_t : generic_proc_t
  {
    gladiators_resonator_t( const special_effect_t& effect )
      : generic_proc_t( effect, "gladiators_resonator", effect.trigger() )
    {
      harmful          = false;
      callbacks        = false;
      impact_action    = create_proc_action<gladiators_resonator_damage_t>( "gladiators_resonator_damage", effect );
      s_data_reporting = effect.driver();
      travel_delay     = effect.driver()->effectN( 2 ).misc_value1() / 1000;
    }
  };

  effect.execute_action = create_proc_action<gladiators_resonator_t>( "gladiators_resonator", effect );
}

void elegy_of_the_eternals( special_effect_t& effect )
{
  // TODO: confirm stat priority when stats are equal. for now assuming same as titanic ocular gland
  static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING,
                                                     STAT_CRIT_RATING };

  auto buff_list = std::make_shared<std::map<stat_e, buff_t*>>();

  // TODO: 369544 has same data as the driver, but with the presumably correct -7 scaling effect. Confirm that the
  // driver really is 367246 and that 369544 is an unreferenced placeholder spell for the correct scaling effect.
  double amount = effect.player->find_spell( 369544 )->effectN( 1 ).average( effect.item );

  for ( auto stat : ratings )
  {
    auto name = fmt::format( "elegy_of_the_eternals_{}", util::stat_type_abbrev( stat ) );
    auto buff = buff_t::find( effect.player, name );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( 369439 ), effect.item )
                 ->add_stat( stat, amount )
                 ->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT )
                 ->set_can_cancel( false );
    }
    ( *buff_list )[ stat ] = buff;
  }

  auto update_buffs = [ p = effect.player, buff_list ]() mutable {
    auto max_stat = util::highest_stat( p, ratings );

    for ( auto stat : ratings )
    {
      if ( ( *buff_list )[ stat ]->check() )
      {
        if ( max_stat != stat )
        {
          ( *buff_list )[ stat ]->expire();
          max_stat = util::highest_stat( p, ratings );
        }

        break;
      }
    }

    // TODO: confirm that the buff spell only lasts 10s as spell data suggests. Because it is not a permanent aura, we
    // have to execute the new buff every time update_buffs() is called.
    ( *buff_list )[ max_stat ]->execute();
    // TODO: implement bonus buff to party members
  };

  // TODO: confirm that the 10s period of effect #1 in the driver is the periodicity on which your highest stat is
  // checks & the respective buff applied
  effect.player->register_combat_begin( [ effect, update_buffs ]( player_t* p ) mutable {
    auto period = effect.driver()->effectN( 1 ).period();
    auto first_update = p->rng().real() * period;

    update_buffs();
    make_event( p->sim, first_update, [ period, update_buffs, p ]() mutable {
      update_buffs();
      make_repeating_event( p->sim, period, update_buffs );
    } );
  } );

  // right-rotate to place update_buff at the front of all combat_begin callbacks, to replicate in-game behavior where
  // the buff is already present on the player before entering combat.
  auto vec = &effect.player->combat_begin_functions;
  std::rotate( vec->rbegin(), vec->rbegin() + 1, vec->rend() );
}

// id=367336 - Brood of the Endless Feast	(driver - damage on effect 1)
// id=368585 - Scent of Souls (target debuff)
// id=368587 - Rabid Devourer Chomp (physical damage at max stacks)
void bells_of_the_endless_feast( special_effect_t& effect )
{
  struct rabid_devourer_chomp_t : generic_proc_t
  {
    rabid_devourer_chomp_t( const special_effect_t& effect )
      : generic_proc_t( effect, "rabid_devourer_chomp", effect.player->find_spell( 368587 ) )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    }
  };

  struct brood_of_the_endless_feast_cb_t : dbc_proc_callback_t
  {
    action_t* damage;

    brood_of_the_endless_feast_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.player, effect ),
        damage( create_proc_action<rabid_devourer_chomp_t>( "rabid_devourer_chomp", effect ) )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      actor_target_data_t* td = a->player->get_target_data( s->target );

      // NOTE: Damage triggers on the next tick of the debuff after it reaches max stacks, but I'm not sure if it's
      // worth modeling that properly, so we instead trigger it as soon as it reaches max stacks.
      if ( td->debuff.scent_of_souls->at_max_stacks() )
      {
        td->debuff.scent_of_souls->expire();
        damage->execute_on_target( s->target );
      }
      else
      {
        td->debuff.scent_of_souls->trigger();
      }
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT;

  new brood_of_the_endless_feast_cb_t( effect );
}

// id=367924 driver
// id=368645 haste buff
// id=369287 dot? no periodic effect yet
// id=369294 ground effect to stand in for haste buff?
// id=369318 unknown, possibly damage driven by undiscovered/unimplemented periodic effect
void grim_eclipse( special_effect_t& effect )
{
  struct grim_eclipse_t : public proc_spell_t
  {
    stat_buff_t* buff;
    timespan_t base_duration;

    grim_eclipse_t( const special_effect_t& e )
      : proc_spell_t( "grim_eclipse", e.player, e.trigger() ),
        buff( make_buff<stat_buff_t>( e.player, "grim_eclipse", e.player->find_spell( 368645 ), e.item ) ),
        base_duration( 10_s )
    {
      dot_duration   = data().duration();
      // Not in spelldata
      base_tick_time = 1_s;

      if ( e.player->sim->shadowlands_opts.grim_eclipse_dot_duration_multiplier > 0.0 )
      {
        e.player->sim->print_debug(
            "Altering grim_eclipse DoT Uptime by {}. Old Duration: {}. New duration: {}",
            e.player->sim->shadowlands_opts.grim_eclipse_dot_duration_multiplier, data().duration(),
            data().duration() * e.player->sim->shadowlands_opts.grim_eclipse_dot_duration_multiplier );
        dot_duration = data().duration() * e.player->sim->shadowlands_opts.grim_eclipse_dot_duration_multiplier;
      }

      tick_action = create_proc_action<generic_proc_t>( "grim_eclipse_damage", e, "grim_eclipse_damage", 369318 );
      // Use data().duration() here so that if you alter dot_duration the tick value is not changed
      tick_action->base_dd_min = tick_action->base_dd_max =
          e.driver()->effectN( 1 ).average( e.item ) / data().duration().total_seconds();
      // damage effect has a radius so we need to override being auto-parsed as an aoe spell
      tick_action->aoe = 0;

      buff->add_stat( STAT_HASTE_RATING, e.driver()->effectN( 2 ).average( e.item ) );
      base_duration = buff->buff_duration();

      if ( player->sim->shadowlands_opts.grim_eclipse_buff_duration_multiplier > 0.0 )
      {
        buff->set_duration_multiplier( player->sim->shadowlands_opts.grim_eclipse_buff_duration_multiplier );
        e.player->sim->print_debug( "Altering grim_eclipse Haste buff duration by {}",
                                    player->sim->shadowlands_opts.grim_eclipse_buff_duration_multiplier );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      // Always give the haste buff after the Quasar expires, regardless of DoT duration overrides
      make_event( *sim, data().duration(), [ this ] {
        // TODO: implement modeling of leaving/entering the buff zone
        if ( player->sim->shadowlands_opts.grim_eclipse_buff_duration_multiplier > 0.0 )
        {
          // Delay the buff proportional to the multiplier change
          timespan_t buff_delay = base_duration - buff->buff_duration();
          make_event( *sim, buff_delay, [ this ] { buff->trigger(); } );

          if ( buff_delay > 0_s )
          {
            player->sim->print_debug( "Scheduling grim_eclipse haste buff with a {}s delay.",
                                      buff_delay.total_seconds() );
          }
        }
      } );
    }
  };

  effect.execute_action = create_proc_action<grim_eclipse_t>( "grim_eclipse", effect );
}

// id=367802 driver
// id=368747 damage
// id=368775 coeffs
// id=368810 shield
void pulsating_riftshard( special_effect_t& effect )
{
  struct pulsating_riftshard_t : public proc_spell_t
  {
    struct pulsating_riftshard_damage_t : public shadowlands_aoe_proc_t
    {
      pulsating_riftshard_damage_t( const special_effect_t& e )
        : shadowlands_aoe_proc_t( e, "pulsating_riftshard_damage", 368747, true )
      {
        auto coeff_data = e.player->find_spell( 368775 );

        max_scaling_targets = as<unsigned>( coeff_data->effectN( 3 ).base_value() );
        base_dd_min = base_dd_max = coeff_data->effectN( 1 ).average( e.item );
        background = dual = true;
      }
    };

    action_t* damage;
    timespan_t delay;

    pulsating_riftshard_t( const special_effect_t& e )
      : proc_spell_t( "pulsating_riftshard", e.player, e.driver() ),
        damage( create_proc_action<pulsating_riftshard_damage_t>( "pulsating_riftshard_damage", e ) ),
        delay( data().duration() )
    {
      damage->stats = stats;
    }

    void execute() override
    {
      proc_spell_t::execute();

      // TODO: better modeling of frontal line behavior incl. mobs moving out, etc.
      auto t = target;
      make_event( *sim, delay, [ this, t ]() {
        damage->execute_on_target( t );
      } );
    }
  };

  effect.execute_action = create_proc_action<pulsating_riftshard_t>( "pulsating_riftshard", effect );
}

// 367804 driver, periodic weapon choice rotation
// 367805 on-use
//
// 368657 sword periodic weapon choice buff
// 368649 stacking haste buff
//
// 368656 axe periodic weapon choice buff
// 368650 axe on-crit bleed buff
// 368651 axe bleed debuff
//
// 368654 wand periodic weapon choice buff
// 368653 wand damage proc
void cache_of_acquired_treasures( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "acquired_sword", "acquired_axe", "acquired_axe_driver", "acquired_wand", "acquired_sword_haste" } ) )
    return;

  struct acquire_weapon_t : public proc_spell_t
  {
    struct acquired_wand_t : public proc_spell_t
    {
      acquired_wand_t( const special_effect_t& effect )
          : proc_spell_t( "acquired_wand", effect.player, effect.player->find_spell( 368653 ), effect.item )
      {
        base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
      }
    };

    buff_t* last;
    std::vector<buff_t*> weapons;
    timespan_t cycle_period;

    action_t* wand_damage;
    buff_t* axe_buff;
    stat_buff_t* sword_buff;

    acquire_weapon_t( const special_effect_t& effect ) : proc_spell_t( effect )
    {
      wand_damage = create_proc_action<acquired_wand_t>( "acquired_wand_blast", effect );
      axe_buff = make_buff( effect.player, "acquired_axe_driver", effect.player->find_spell( 368650 ) );
      sword_buff = make_buff<stat_buff_t>( effect.player, "acquired_sword_haste", effect.player->find_spell( 368649 ), effect.item );
      sword_buff->set_refresh_behavior( buff_refresh_behavior::DISABLED );

      auto haste_driver = new special_effect_t( effect.player );
      haste_driver->name_str = "acquired_sword_driver";
      haste_driver->spell_id = 368649;
      haste_driver->proc_flags_ = effect.player->find_spell( 368649 )->proc_flags();
      haste_driver->proc_flags2_ = PF2_ALL_HIT;
      haste_driver->custom_buff = sword_buff;
      effect.player->special_effects.push_back( haste_driver );

      auto acquired_sword_cb = new dbc_proc_callback_t( effect.player, *haste_driver );
      acquired_sword_cb->initialize();
      acquired_sword_cb->deactivate();

      sword_buff->set_stack_change_callback( [ acquired_sword_cb ]( buff_t*, int old, int new_ ) {
        if ( old == 0 )
          acquired_sword_cb->activate();
        else if ( new_ == 0 )
          acquired_sword_cb->deactivate();
      } );

      auto bleed = new proc_spell_t( "vicious_wound", effect.player, effect.player->find_spell( 368651 ), effect.item );
      bleed->dual = true; // Executes added below in the callback for DPET values vs. Wand usage

      auto bleed_driver               = new special_effect_t( effect.player );
      bleed_driver->name_str          = "acquired_axe_driver";
      bleed_driver->spell_id          = 368650;
      bleed_driver->proc_flags_       = effect.player->find_spell( 368650 )->proc_flags();
      bleed_driver->proc_flags2_      = PF2_CRIT;
      bleed_driver->execute_action    = bleed;
      effect.player->special_effects.push_back( bleed_driver );

      auto vicious_wound_cb = new dbc_proc_callback_t( effect.player, *bleed_driver );
      vicious_wound_cb->initialize();
      vicious_wound_cb->deactivate();

      axe_buff->set_stack_change_callback( [ vicious_wound_cb, bleed ]( buff_t*, int old, int new_ ) {
        if ( old == 0 )
        {
          vicious_wound_cb->activate();
          bleed->stats->add_execute( timespan_t::zero(), bleed->player );
        }
        else if ( new_ == 0 )
        {
          vicious_wound_cb->deactivate();
        }
      } );

      weapons.emplace_back(
          make_buff<buff_t>( effect.player, "acquired_sword", effect.player->find_spell( 368657 ), effect.item )
              ->set_cooldown( 0_s ) );
      weapons.push_back(
          make_buff<buff_t>( effect.player, "acquired_axe", effect.player->find_spell( 368656 ), effect.item )
              ->set_cooldown( 0_s ) );
      weapons.push_back( last =
          make_buff<buff_t>( effect.player, "acquired_wand", effect.player->find_spell( 368654 ), effect.item )
              ->set_cooldown( 0_s ) );

      cycle_period = effect.player->find_spell( 367804 )->effectN( 1 ).period();

      auto cycle_weapon = [ this ]( int cycles ) {
        // As of 9.2.5 the Sword buff is triggered in the cycle prior coming off cooldown, rather than after
        if ( cooldown->remains() < cycle_period )
        {
          weapons.front()->expire();
          std::rotate( weapons.begin(), weapons.begin() + cycles, weapons.end() );
          weapons.front()->trigger();
        }
      };

      effect.player->register_combat_begin( [ this, &effect, cycle_weapon ]( player_t* p ) {
        // randomize the weapon choice and its remaining duration when combat starts
        timespan_t first_update = p->rng().real() * cycle_period;
        int first = p->rng().range( 3 );
        cycle_weapon( first );

        make_event( p->sim, first_update, [ this, &effect, cycle_weapon ]() {
          cycle_weapon( 1 );
          make_repeating_event( effect.player->sim, cycle_period, [ cycle_weapon ]() {
            cycle_weapon( 1 );
          } );
        } );
      } );
    }

    bool ready() override
    {
      if ( !weapons.front()->check() )
        return false;

      return proc_spell_t::ready();
    }

    void execute() override
    {
      proc_spell_t::execute();

      weapons.front()->expire();

      if ( weapons.front()->data().id() == 368654 ) // wand
      {
        wand_damage->execute_on_target( player->target );
      }
      else if ( weapons.front()->data().id() == 368656 ) // axe
      {
        axe_buff->trigger();
      }
      else if ( weapons.front()->data().id() == 368657 )  // sword
      {
        sword_buff->trigger();
      }
      
      // resets to sword after on-use is triggered, rotate wand in front so sword will cycle in next after the cooldown recovers
      std::rotate( weapons.begin(), range::find( weapons, last ), weapons.end() );
    }
  };

  effect.type           = SPECIAL_EFFECT_USE;
  effect.execute_action = create_proc_action<acquire_weapon_t>( "acquire_weapon", effect );
}

// driver=367733 trigger=367734
void symbol_of_the_raptora( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "raptoras_wisdom" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "raptoras_wisdom", effect.trigger() )
               ->add_stat( STAT_INTELLECT, effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// 367470: driver
// 367471: buff + 2nd driver
// 367472: damage
void protectors_diffusion_implement( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "protectors_diffusion_implement", effect,
                                                        "protectors_diffusion_implement", 367472 );
  damage->split_aoe_damage = false;
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );

  auto proc = new special_effect_t( effect.player );
  proc->name_str = "protectors_diffusion_implement_proc";
  proc->spell_id = 367471;
  proc->execute_action = damage;
  effect.player->special_effects.push_back( proc );

  auto proc_cb = new dbc_proc_callback_t( effect.player, *proc );
  proc_cb->deactivate();

  effect.custom_buff = make_buff( effect.player, "protectors_diffusion_implement", effect.player->find_spell( 367471 ) )
    ->set_stack_change_callback( [ proc_cb ]( buff_t*, int, int new_ ) {
      if ( new_ )
        proc_cb->activate();
      else
        proc_cb->deactivate();
    } );

  new dbc_proc_callback_t( effect.player, effect );
}

// 367682 driver
// 367689 projectile
// 367687 damage
void vombatas_headbutt( special_effect_t& effect )
{
  auto damage =
      create_proc_action<generic_proc_t>( "vombatas_headbutt_damage", effect, "vombatas_headbutt_damage", 367687 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
  damage->dual = true;

  auto action = create_proc_action<proc_spell_t>( "vombatas_headbutt", effect );
  action->impact_action = damage;
  damage->stats = action->stats;

  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

// id=367808 driver
//    effect #1: Periodic trigger for pulse damage spell, dummy damage value for tooltip only
//    effect #2: Weak point damage value in dummy, overwrites spell value when dealing weak point damage
//    effect #3-5: Area damage spell triggers for the 3 weak points spawned
// id=368634 AoE damage spell, reused for both damage components
void earthbreakers_impact( special_effect_t& effect )
{
  struct earthbreakers_impact_aoe_t : public shadowlands_aoe_proc_t
  {
    earthbreakers_impact_aoe_t( const special_effect_t& e, bool weak_point ) :
      shadowlands_aoe_proc_t( e, weak_point ? "earthbreakers_impact_weak_point" : "earthbreakers_impact_pulse",
                              e.driver()->effectN( 1 ).trigger(), true )
    {
      // The same AoE spell gets reused by both the normal ticks and the weak point triggers
      if ( weak_point )
      {
        base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e.item );
      }
    }
  };

  struct earthbreakers_impact_t : public proc_spell_t
  {
    action_t* weak_point;

    earthbreakers_impact_t( const special_effect_t& e ) :
      proc_spell_t( "earthbreakers_impact", e.player, e.driver() ),
      weak_point( create_proc_action<earthbreakers_impact_aoe_t>( "earthbreakers_impact_weak_point", e, true ) )
    {
      tick_action = create_proc_action<earthbreakers_impact_aoe_t>( "earthbreakers_impact_pulse", e, false );
      add_child( weak_point );
    }

    void execute() override
    {
      proc_spell_t::execute();
      
      // Assume roughly a 1s delay between stepping on the triggers, as they are relatively close
      for ( unsigned int i = 1; i <= player->sim->shadowlands_opts.earthbreakers_impact_weak_points; i++ )
      {
        make_event( *sim, 1_s * i, [this] {
          this->weak_point->set_target( this->target );
          this->weak_point->schedule_execute();
        });
      }
    }
  };

  effect.execute_action = create_proc_action<earthbreakers_impact_t>( "earthbreakers_impact", effect );
}

void prismatic_brilliance( special_effect_t& effect )
{
  auto val = effect.driver()->effectN( 1 ).average( effect.item );
  std::vector<stat_buff_t*> buffs;

  buffs.push_back( make_buff<stat_buff_t>( effect.player, "brilliantly_critical", effect.player->find_spell( 367327 ) )
                       ->add_stat( STAT_CRIT_RATING, val ) );
  buffs.push_back( make_buff<stat_buff_t>( effect.player, "brilliantly_masterful", effect.player->find_spell( 367455 ) )
                       ->add_stat( STAT_MASTERY_RATING, val ) );
  buffs.push_back( make_buff<stat_buff_t>( effect.player, "brilliantly_versatile", effect.player->find_spell( 367457 ) )
                       ->add_stat( STAT_VERSATILITY_RATING, val ) );
  buffs.push_back( make_buff<stat_buff_t>( effect.player, "brilliantly_hasty", effect.player->find_spell( 367458 ) )
                       ->add_stat( STAT_HASTE_RATING, val ) );

  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_execute_function(
      effect.driver()->id(), [ buffs ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
        buffs[ cb->rng().range( buffs.size() ) ]->trigger();
      } );
}

// id=367931 driver, physical damage, and debuff
// Proc flags on the driver are set up as damage taken flags as it is a debuff
//    effect #1: Direct physical damage on use effect
//    effect #2: Percentage of damage absorbed by the debuff proc trigger
//    effect #3: Damage absorb and AoE damage cap
//    effect #4: AoE radius
// id=368643 AoE shadow damage spell
struct chains_of_domination_cb_t : public dbc_proc_callback_t
{
  double accumulated_damage;
  double damage_cap;
  double damage_fraction;
  buff_t* debuff;
  bool auto_break;

  chains_of_domination_cb_t( const special_effect_t& e ) :
    dbc_proc_callback_t( e.player, e ),
    accumulated_damage( 0.0 ),
    damage_cap( e.driver()->effectN( 3 ).average( e.item ) ),
    damage_fraction( e.driver()->effectN( 2 ).percent() ),
    debuff( nullptr ),
    auto_break( e.player->sim->shadowlands_opts.chains_of_domination_auto_break )
  {
  }

  void execute( action_t*, action_state_t* s ) override
  {
    if ( debuff && debuff->check() && s->target == debuff->player )
    {
      accumulated_damage = std::min( damage_cap, accumulated_damage + ( s->result_amount * damage_fraction ) );
      if ( auto_break && accumulated_damage >= damage_cap )
      {
        debuff->expire();
      }
    }
  }

  void reset() override
  {
    dbc_proc_callback_t::reset();
    accumulated_damage = 0.0;
    debuff = nullptr;
  }
};

// Action to trigger the chain break for the Chains of Domination trinket
struct break_chains_of_domination_t : public action_t
{
  chains_of_domination_cb_t* cb_driver;

  break_chains_of_domination_t( player_t* p, util::string_view opt )
    : action_t( ACTION_OTHER, "break_chains_of_domination", p ),
    cb_driver( nullptr )
  {
    parse_options( opt );
    trigger_gcd = 0_ms;
    harmful = false;
    ignore_false_positive = usable_while_casting = true;
  }

  void init_finished() override
  {
    cb_driver = dynamic_cast<chains_of_domination_cb_t*>(
      *( range::find_if( player->callbacks.all_callbacks, []( action_callback_t* t ) {
      return static_cast<dbc_proc_callback_t*>( t )->effect.spell_id == 367931;
    } ) ) );

    // If this action exists in the APL, disable the auto break option
    if ( cb_driver )
    {
      cb_driver->auto_break = false;
    }

    action_t::init_finished();
  }

  void execute() override
  {
    if ( !cb_driver || !cb_driver->debuff || !cb_driver->debuff->check() )
      return;

    cb_driver->debuff->expire();
  }

  bool ready() override
  {
    if ( !cb_driver || !cb_driver->debuff || !cb_driver->debuff->check() )
      return false;

    return action_t::ready();
  }
};

void chains_of_domination( special_effect_t& effect )
{
  struct chains_of_domination_t : public proc_spell_t
  {
    // This is currently uncapped and does not split
    struct chains_of_domination_break_t : public proc_spell_t
    {
      chains_of_domination_break_t( const special_effect_t& e ) :
        proc_spell_t( "chains_of_domination_break", e.player, e.player->find_spell( 368643 ) )
      {
        dual = true;
        radius = e.driver()->effectN( 4 ).base_value();
        base_dd_min = base_dd_max = 1.0;
      }
    };

    action_t* chain_break;
    chains_of_domination_cb_t* cb_driver;

    chains_of_domination_t( const special_effect_t& e, chains_of_domination_cb_t* cb ) :
      proc_spell_t( "chains_of_domination", e.player, e.driver(), e.item ),
      chain_break( create_proc_action<chains_of_domination_break_t>( "chains_of_domination_break", e ) ),
      cb_driver( cb )
    {
      add_child( chain_break );
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      actor_target_data_t* td = player->get_target_data( s->target );
      if ( !td->debuff.chains_of_domination->stack_change_callback )
      {
        td->debuff.chains_of_domination->set_stack_change_callback( [this]( buff_t* b, int old_, int new_ ) {
          if ( old_ == 0 && new_ > 0 )
          {
            cb_driver->debuff = b;
            cb_driver->accumulated_damage = 0.0;
            cb_driver->activate();
          }
          else if ( new_ == 0 )
          {
            // If the target dies with the debuff on them or the debuff expires, it does not trigger an explosion
            // However, if shadowlands_opts.chains_of_domination_auto_break is set, allow it to deal damage anyway
            if ( cb_driver->auto_break || ( !b->player->is_sleeping() && b->elapsed( sim->current_time() ) < b->base_buff_duration ) )
            {
              chain_break->base_dd_min = chain_break->base_dd_max = cb_driver->accumulated_damage;
              chain_break->set_target( b->player );
              chain_break->schedule_execute();
            }
            else
            {
              sim->print_debug( "{} debuff {} on {} expired without dealing damage ({} accumulated)",
                                *player, *b, *b->player, cb_driver->accumulated_damage );
            }
            cb_driver->deactivate();
            cb_driver->debuff = nullptr;
            cb_driver->accumulated_damage = 0.0;
          }
        } );
      }

      td->debuff.chains_of_domination->trigger();
    }
  };

  auto cb_driver = new special_effect_t( effect.player );
  cb_driver->name_str = "chains_of_domination_break_driver";
  cb_driver->spell_id = effect.spell_id;
  cb_driver->item = effect.item;
  cb_driver->proc_chance_ = effect.driver()->proc_chance();
  cb_driver->cooldown_ = 0_s;
  cb_driver->proc_flags_ = PF_ALL_DAMAGE | PF_PERIODIC;
  cb_driver->proc_flags2_ = PF2_ALL_HIT;
  effect.player->special_effects.push_back( cb_driver );

  auto callback = new chains_of_domination_cb_t( *cb_driver );
  callback->initialize();
  callback->deactivate();

  auto proc = create_proc_action<chains_of_domination_t>( "chains_of_domination", effect, callback );
  effect.execute_action = proc; 
}

// Weapons

// id=331011 driver
// id=331016 DoT proc debuff
void poxstorm( special_effect_t& effect )
{
  // When dual-wielded, Poxstorm has one shared DoT which is duration refreshed
  // The damage is overwritten by the last proc'd version in the case of unequal item levels
  struct bubbling_pox_t : public proc_spell_t
  {
    bubbling_pox_t( const special_effect_t& effect )
      : proc_spell_t( "bubbling_pox", effect.player, effect.trigger(), effect.item )
    {
    }
  };

  // Note, no create_proc_action here, since there is the possibility of dual-wielding them and
  // special_effect_t does not have enough support for defining "shared spells" on initialization
  effect.execute_action = new bubbling_pox_t( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void init_jaithys_the_prison_blade( special_effect_t& effect, int proc_id, int spell_id, int rank )
{
  // When same item levels are dual-wielded, Jaithys currently combines damage and str buff
  // Dev confirmed this is a bug and all procs should be independent
  action_t* harsh_tutelage_damage = create_proc_action<generic_proc_t>(  
      "harsh_tutelage", effect, "harsh_tutelage" + std::to_string( rank ), proc_id );

  harsh_tutelage_damage->base_dd_min = effect.player->find_spell( spell_id )->effectN( 1 ).average( effect.item );
  harsh_tutelage_damage->base_dd_max = effect.player->find_spell( spell_id )->effectN( 1 ).average( effect.item );

  effect.execute_action = harsh_tutelage_damage;

  // rank 2+ has a str buff
  if ( rank > 2 )
  {
    buff_t* buff = buff_t::find( effect.player, "harsh_tutelage" + std::to_string( rank ) );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "harsh_tutelage" + std::to_string( rank ),
                                  effect.player->find_spell( proc_id ) )
              ->add_stat( STAT_STRENGTH, effect.player->find_spell( spell_id )->effectN( 2 ).average( effect.item ) )
              ->set_refresh_duration_callback(
                     []( const buff_t* b, timespan_t d ) { return std::min( b->remains() + d, 9_s ); } );
    }
    effect.custom_buff = buff;
  }

  new dbc_proc_callback_t( effect.player, effect );
}

void jaithys_the_prison_blade_1( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358564, 358562, 1 );
}

void jaithys_the_prison_blade_2( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358566, 358565, 2 );
}

void jaithys_the_prison_blade_3( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358568, 358567, 3 );
}
    
void jaithys_the_prison_blade_4( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358570, 358569, 4 );
}

void jaithys_the_prison_blade_5( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358572, 358571, 5 );
}

/**Yasahm the Riftbreaker
    351527 driver, proc on crit
    351531 trigger buff, damage on effect 1
    351561 damage proc on crit after 5th stack
  */
void yasahm_the_riftbreaker( special_effect_t& effect )
{
  struct preternatural_charge_t : public proc_spell_t
  {
    preternatural_charge_t( const special_effect_t& effect )
      : proc_spell_t( "preternatural_charge", effect.player, effect.player->find_spell( 351561 ), effect.item )
    {
      base_dd_min = base_dd_max = effect.trigger()->effectN( 1 ).average( effect.item );
    }
  };

  auto proc = create_proc_action<preternatural_charge_t>( "preternatural_charge", effect );
  auto buff   = buff_t::find( effect.player, "preternatural_charge" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "preternatural_charge", effect.trigger() )->set_max_stack( effect.trigger()->max_stacks() + 1 );
    buff->set_stack_change_callback( [ proc ]( buff_t* buff, [[maybe_unused]] int old, int cur ) {
      if ( cur == buff->max_stack() )
      {
        proc->set_target( buff->player->target );
        proc->execute();
        make_event( buff->sim, [ buff ] { buff->expire(); } );
      }
    } );
  }

  effect.custom_buff  = buff;
  effect.proc_flags2_ = PF2_CRIT;
  new dbc_proc_callback_t( effect.player, effect );
}

// TODO: Add proc restrictions to match the weapons or expansion options.
void cruciform_veinripper(special_effect_t& effect)
{
  struct cruciform_veinripper_cb_t : public dbc_proc_callback_t
  {
    double proc_modifier_override;
    double proc_modifier_in_front_override;

    cruciform_veinripper_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.player, effect ),
      proc_modifier_override( effect.player->sim->shadowlands_opts.cruciform_veinripper_proc_rate ),
      proc_modifier_in_front_override( effect.player->sim->shadowlands_opts.cruciform_veinripper_in_front_rate )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      assert( rppm );
      assert( s->target );
      
      double proc_modifier = 1.0;
      if ( proc_modifier_override > 0.0 )
      {
        proc_modifier = proc_modifier_override;
      }
      else if( s->target->is_boss() )
      {
        // CC'd/Snared mobs appear to take the full proc rate, which does not work on bosses
        // The "from behind" rate is roughly half the CC'd target rate in the spell data
        proc_modifier = 0.5;
        if ( a->player->position() == POSITION_FRONT )
        {
          if ( proc_modifier_in_front_override > 0.0 )
          {
            proc_modifier *= proc_modifier_in_front_override;
          }
          else
          {
            // Generalize default tank "behind boss" time as ~40% when no manual modifier is specified
            // This does not proc from the front at all, but tanks are always position front for sims
            //
            // When the role is not tank, this is explicitly set to 0 to allow DPS to model bosses where
            // they can't hit from behind.
            proc_modifier *= a->player->primary_role() == ROLE_TANK ? 0.4 : 0.0;
          }
        }
      }

      if ( proc_modifier != rppm->get_modifier() )
      {
        effect.player->sim->print_debug( "Player {} (position: {}, role: {}) adjusts {} rppm modifer: old={} new={}",
                                         *a->player, a->player->position(), a->player->primary_role(), effect, rppm->get_modifier(), proc_modifier);
        rppm->set_modifier( proc_modifier );
      }

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  struct sadistic_glee_t : public proc_spell_t
  {
    double scaled_dmg;

    sadistic_glee_t(const special_effect_t& e)
      : proc_spell_t("sadistic_glee", e.player, e.player->find_spell(353466), e.item),
        scaled_dmg(e.driver()->effectN(1).average(e.item))
    {
      base_td = scaled_dmg;
    }

    double base_ta(const action_state_t* /* s */) const override
    {
      return scaled_dmg;
    }
  };

  auto sadistic_glee = static_cast<sadistic_glee_t*>(effect.player->find_action("sadistic_glee"));

  if (!sadistic_glee)
    effect.execute_action = create_proc_action<sadistic_glee_t>("sadistic_glee", effect);
  else
    sadistic_glee->scaled_dmg += effect.driver()->effectN( 1 ).average( effect.item );

  effect.spell_id = effect.driver()->effectN( 1 ).trigger_spell_id();

  new cruciform_veinripper_cb_t( effect );
}

struct singularity_supreme_t : public stat_buff_t
{
  stat_buff_t* supremacy_buff;
  buff_t* lockout;
  stat_buff_t* swap_stat_compensation;

  singularity_supreme_t( player_t* p, const item_t* i )
    : stat_buff_t( p, "singularity_supreme_counter", p->find_spell( 368845 ), i )
  {
    lockout = buff_t::find( p, "singularity_supreme_lockout" );
    if ( !lockout )
    {
      lockout = make_buff( p, "singularity_supreme_lockout", p->find_spell( 368865 ) )
                    ->set_quiet( true )
                    ->set_can_cancel( false );
    }

    supremacy_buff = dynamic_cast<stat_buff_t*>( buff_t::find( p, "singularity_supreme" ) );
    if ( !supremacy_buff )
    {
      supremacy_buff = make_buff<stat_buff_t>( p, "singularity_supreme", p->find_spell( 368863 ), i );
      supremacy_buff->set_can_cancel( false );
    }

    swap_stat_compensation = dynamic_cast<stat_buff_t*>( buff_t::find( p, "swap_stat_compensation" ) );

    if ( !swap_stat_compensation )
    {
      swap_stat_compensation = make_buff<stat_buff_t>( p, "swap_stat_compensation" );
      swap_stat_compensation->set_name_reporting( "antumbra_swapped_with_other_weapon" );
      swap_stat_compensation->set_duration( 0_s )->set_can_cancel( false );
    }

    if ( p->antumbra.swap )
    {
      swap_stat_compensation->stats.clear();
      if ( p->antumbra.int_diff != 0.0 )
        swap_stat_compensation->add_stat( STAT_INTELLECT, p->antumbra.int_diff );
      if ( p->antumbra.crit_diff != 0.0 )
        swap_stat_compensation->add_stat( STAT_CRIT_RATING, p->antumbra.crit_diff );
      if ( p->antumbra.haste_diff != 0.0 )
        swap_stat_compensation->add_stat( STAT_HASTE_RATING, p->antumbra.haste_diff );
      if ( p->antumbra.vers_diff != 0.0 )
        swap_stat_compensation->add_stat( STAT_VERSATILITY_RATING, p->antumbra.vers_diff );
      if ( p->antumbra.mastery_diff != 0.0 )
        swap_stat_compensation->add_stat( STAT_MASTERY_RATING, p->antumbra.mastery_diff );
      if ( p->antumbra.stam_diff != 0.0 )
        swap_stat_compensation->add_stat( STAT_STAMINA, p->antumbra.stam_diff );
    }

    set_expire_at_max_stack( true );
    set_stack_change_callback( [ this ]( buff_t* b, int, int ) {
      if ( b->at_max_stacks() )
      {
        lockout->trigger();
        supremacy_buff->trigger();
      }
    } );
  }
};

void singularity_supreme( special_effect_t& effect )
{

  if ( unique_gear::create_fallback_buffs(
           effect, { "singularity_supreme_counter", "singularity_supreme_lockout", "singularity_supreme", "swap_stat_compensation" } ) )
      return;

  // logs seem to show it only procs on damage spell casts
  effect.proc_flags2_ = PF2_CAST_DAMAGE;

  auto singularity_buff = new singularity_supreme_t( effect.player, effect.item );

  effect.custom_buff = singularity_buff;

  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_trigger_function(
      effect.driver()->id(), dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ singularity_buff ]( const dbc_proc_callback_t*, action_t*, action_state_t* ) {
        return !singularity_buff->swap_stat_compensation->check() && !singularity_buff->lockout->check();
      } );
}

// Action to weapon swap Antumbra
struct antumbra_swap_t : public action_t
{
  singularity_supreme_t* singularity_buff;

  antumbra_swap_t( player_t* p, util::string_view opt ) : action_t( ACTION_OTHER, "antumbra_swap", p )
  {
    parse_options( opt );
    trigger_gcd           = 1500_ms;
    gcd_type              = gcd_haste_type::NONE;
    harmful               = false;
    ignore_false_positive = true;
  }

  void init_finished() override
  {
    action_t::init_finished();
    singularity_buff = dynamic_cast<singularity_supreme_t*>( buff_t::find( player, "singularity_supreme_counter" ) );
  }

  void execute() override
  {
    if ( !singularity_buff || !player->antumbra.swap )
      return;

    if ( singularity_buff->swap_stat_compensation->check() )
    {
      singularity_buff->swap_stat_compensation->cancel();
    }
    else
    {
      singularity_buff->swap_stat_compensation->trigger();
      singularity_buff->supremacy_buff->cancel();
      singularity_buff->cancel();
    }
  }

  bool ready() override
  {
    if ( !singularity_buff || !player->antumbra.swap )
      return false;

    return action_t::ready();
  }
};

/** Gavel of the First Arbiter
  367953 driver
  369046 on-use

  Boon of Looming Winter
  368693 driver, buff
  368698 Absorb buff and frost damage

  Boon of Divine Command
  368694 driver
  368699 arcane Damage + armor buff

  Boon of Harvested Hope
  368695 driver
  368701 Health leech dot

  Boon of Assured Victory
  368696 Victory driver
  368700 Rotting Decay Nature dot, stacks

  Boon of the End
  368697 driver
  368702 Proc/buff

  Common
  369238 - Stores all the damage values
  s1 - Divine Command arcane damage
  s2 - Assured Victory - Rotting Decay, stacking nature damage
  s3 - Looming Winter frost damage
  s4 - Looming Winter absorb amount
  s5 - Harvest Hope bleed damage
  s6 - Boon of the End Shadow Damage
  s7 - Pretty sure this is Boon of the End Strength buff amount
  s8 - Divine Command +armor

*/
void gavel_of_the_first_arbiter( special_effect_t& effect )
{
  struct twisted_judgment_t : public proc_spell_t
  {
    std::vector<buff_t*> buffs;

    buff_t* looming_winter_active_buff;
    buff_t* looming_winter_absorb_buff;

    buff_t* divine_command_active_buff;
    // buff_t* divine_command_armor_buff;  NYI

    buff_t* harvested_hope_active_buff;

    buff_t* assured_victory_active_buff;

    buff_t* boon_of_the_end_active_buff;
    buff_t* boon_of_the_end_str_buff;

    twisted_judgment_t( const special_effect_t& effect ) : proc_spell_t( effect )
    {
      // Buffs
      looming_winter_absorb_buff = buff_t::find( effect.player, "boon_of_looming_winter_absorb" );
      if ( !looming_winter_absorb_buff )
      {
        looming_winter_absorb_buff =
            make_buff<absorb_buff_t>( effect.player, "boon_of_looming_winter_absorb", effect.player->find_spell( 368698 ) )
                ->set_default_value( effect.player->find_spell( 369238 )->effectN( 4 ).average( effect.item ) );
      }

      boon_of_the_end_str_buff = buff_t::find( effect.player, "boon_of_the_end_str" );
      if ( !boon_of_the_end_str_buff )
      {
        boon_of_the_end_str_buff =
            make_buff<stat_buff_t>( effect.player, "boon_of_the_end_str", effect.player->find_spell( 368697 ) )
                ->add_stat( STAT_STRENGTH, effect.player->find_spell( 369238 )->effectN( 7 ).average( effect.item ) )
                ->set_rppm( RPPM_DISABLE );
      }

      // Create effect and callback for the damage proc
      auto looming_winter = new special_effect_t( effect.player );
      looming_winter->source = SPECIAL_EFFECT_SOURCE_ITEM;
      looming_winter->name_str = "looming_winter";
      looming_winter->spell_id = 368693;
      looming_winter->proc_flags_ = effect.player->find_spell( 368693 )->proc_flags();
      looming_winter->proc_flags2_ = PF2_ALL_HIT;
      looming_winter->execute_action =
          create_proc_action<boon_of_looming_winter_t>( "boon_of_looming_winter_damage", effect, looming_winter_absorb_buff );
      add_child( looming_winter->execute_action );
      looming_winter->disable_buff();  // disable auto creation of absorb buff
      effect.player->special_effects.push_back( looming_winter );

      auto divine_command = new special_effect_t( effect.player );
      divine_command->source = SPECIAL_EFFECT_SOURCE_ITEM;
      divine_command->name_str = "divine_command";
      divine_command->spell_id = 368694;
      divine_command->proc_flags_ = effect.player->find_spell( 368694 )->proc_flags();
      divine_command->proc_flags2_ = PF2_ALL_HIT;
      divine_command->execute_action =
          create_proc_action<boon_of_divine_command_t>( "boon_of_divine_command_damage", effect );
      add_child( divine_command->execute_action );
      divine_command->disable_buff();  // Need to disable, or it auto creates the armor buff
      effect.player->special_effects.push_back( divine_command );

      auto harvested_hope = new special_effect_t( effect.player );
      harvested_hope->source = SPECIAL_EFFECT_SOURCE_ITEM;
      harvested_hope->name_str = "harvested_hope";
      harvested_hope->spell_id = 368695;
      harvested_hope->proc_flags_ = effect.player->find_spell( 368695 )->proc_flags();
      harvested_hope->proc_flags2_ = PF2_ALL_HIT;
      harvested_hope->execute_action =
          create_proc_action<boon_of_harvested_hope_t>( "boon_of_harvested_hope_damage", effect );
      add_child( harvested_hope->execute_action );
      effect.player->special_effects.push_back( harvested_hope );

      auto assured_victory = new special_effect_t( effect.player );
      assured_victory->source = SPECIAL_EFFECT_SOURCE_ITEM;
      assured_victory->name_str = "assured_victory";
      assured_victory->spell_id = 368696;
      assured_victory->proc_flags_ = effect.player->find_spell( 368696 )->proc_flags();
      assured_victory->proc_flags2_ = PF2_ALL_HIT;
      assured_victory->execute_action =
          create_proc_action<boon_of_assured_victory_t>( "boon_of_assured_victory_damage", effect );
      add_child( assured_victory->execute_action );
      effect.player->special_effects.push_back( assured_victory );

      auto boon_of_the_end = new special_effect_t( effect.player );
      boon_of_the_end->source = SPECIAL_EFFECT_SOURCE_ITEM;
      boon_of_the_end->name_str = "boon_of_the_end";
      boon_of_the_end->spell_id = 368697;
      boon_of_the_end->proc_flags_ = effect.player->find_spell( 368697 )->proc_flags();
      boon_of_the_end->proc_flags2_ = PF2_ALL_HIT;
      boon_of_the_end->execute_action =
          create_proc_action<boon_of_the_end_t>( "boon_of_the_end_damage", effect );
      add_child( boon_of_the_end->execute_action );
      effect.player->special_effects.push_back( boon_of_the_end );

      auto looming_winter_cb = new dbc_proc_callback_t( effect.player, *looming_winter );
      looming_winter_cb->initialize();
      looming_winter_cb->deactivate();

      auto divine_command_cb = new dbc_proc_callback_t( effect.player, *divine_command );
      divine_command_cb->initialize();
      divine_command_cb->deactivate();

      auto harvested_hope_cb = new dbc_proc_callback_t( effect.player, *harvested_hope );
      harvested_hope_cb->initialize();
      harvested_hope_cb->deactivate();

      auto assured_victory_cb = new dbc_proc_callback_t( effect.player, *assured_victory );
      assured_victory_cb->initialize();
      assured_victory_cb->deactivate();

      auto boon_of_the_end_cb = new dbc_proc_callback_t( effect.player, *boon_of_the_end );
      boon_of_the_end_cb->initialize();
      boon_of_the_end_cb->deactivate();

      looming_winter_active_buff = buff_t::find( effect.player, "boon_of_looming_winter_active" );
      if ( !looming_winter_active_buff )
      {
        looming_winter_active_buff =
            make_buff( effect.player, "boon_of_looming_winter_active", effect.player->find_spell( 368693 ) )
                ->set_chance( 1.0 )
                ->set_cooldown( 0_ms )
                ->set_name_reporting( "boon_of_looming_winter" )
                ->set_stack_change_callback( [ looming_winter_cb ]( buff_t*, int old, int new_ ) {
                  if ( old == 0 )
                    looming_winter_cb->activate();
                  else if ( new_ == 0 )
                    looming_winter_cb->deactivate();
                } );
      }

      divine_command_active_buff = buff_t::find( effect.player, "boon_of_divine_command_active" );
      if ( !divine_command_active_buff )
      {
        divine_command_active_buff =
            make_buff( effect.player, "boon_of_divine_command_active", effect.player->find_spell( 368694 ) )
                ->set_chance( 1.0 )
                ->set_cooldown( 0_ms )
                ->set_name_reporting( "boon_of_divine_command" )
                ->set_stack_change_callback( [ divine_command_cb ]( buff_t*, int old, int new_ ) {
                  if ( old == 0 )
                    divine_command_cb->activate();
                  else if ( new_ == 0 )
                    divine_command_cb->deactivate();
                } );
      }

      harvested_hope_active_buff = buff_t::find( effect.player, "boon_of_harvested_hope_active" );
      if ( !harvested_hope_active_buff )
      {
        harvested_hope_active_buff =
            make_buff( effect.player, "boon_of_harvested_hope_active", effect.player->find_spell( 368695 ) )
                ->set_chance( 1.0 )
                ->set_cooldown( 0_ms )
                ->set_name_reporting( "boon_of_harvested_hope" )
                ->set_stack_change_callback( [ harvested_hope_cb ]( buff_t*, int old, int new_ ) {
                  if ( old == 0 )
                    harvested_hope_cb->activate();
                  else if ( new_ == 0 )
                    harvested_hope_cb->deactivate();
                } );
      }

      assured_victory_active_buff = buff_t::find( effect.player, "boon_of_assured_victory_active" );
      if ( !assured_victory_active_buff )
      {
        assured_victory_active_buff =
            make_buff( effect.player, "boon_of_assured_victory_active", effect.player->find_spell( 368696 ) )
                ->set_chance( 1.0 )
                ->set_cooldown( 0_ms )
                ->set_name_reporting( "boon_of_assured_victory" )
                ->set_stack_change_callback( [ assured_victory_cb ]( buff_t*, int old, int new_ ) {
                  if ( old == 0 )
                    assured_victory_cb->activate();
                  else if ( new_ == 0 )
                    assured_victory_cb->deactivate();
                } );
      }

      boon_of_the_end_active_buff = buff_t::find( effect.player, "boon_of_the_end_active" );
      if ( !boon_of_the_end_active_buff )
      {
        boon_of_the_end_active_buff =
            make_buff( effect.player, "boon_of_the_end_active", effect.player->find_spell( 368697 ) )
                ->set_chance( 1.0 )
                ->set_cooldown( 0_ms )
                ->set_name_reporting( "boon_of_the_end" )
                ->set_stack_change_callback( [ boon_of_the_end_cb ]( buff_t*, int old, int new_ ) {
                  if ( old == 0 )
                    boon_of_the_end_cb->activate();
                  else if ( new_ == 0 )
                    boon_of_the_end_cb->deactivate();
                } );
      }

      // Define buff array for random selection
      buffs =
      {
        looming_winter_active_buff,
        divine_command_active_buff,
        harvested_hope_active_buff,
        assured_victory_active_buff,
        boon_of_the_end_active_buff
      };
    }

    struct boon_of_looming_winter_t : public proc_spell_t
    {
      buff_t* absorb;
      boon_of_looming_winter_t( const special_effect_t& effect, buff_t* absorb_ )
        : proc_spell_t( "boon_of_looming_winter_damage", effect.player, effect.player->find_spell( 368698 ),
                        effect.item ),
          absorb( absorb_ )
      {
        base_dd_min = base_dd_max = effect.player->find_spell( 369238 )->effectN( 3 ).average( effect.item );
        name_str_reporting = "boon_of_looming_winter";
      }

      void execute() override
      {
        proc_spell_t::execute();
        absorb->trigger();
      }
    };

    struct boon_of_divine_command_t : public proc_spell_t
    {
      boon_of_divine_command_t( const special_effect_t& effect )
        : proc_spell_t( "boon_of_divine_command_damage", effect.player, effect.player->find_spell( 368699 ) )
      {
        base_dd_min = base_dd_max = effect.player->find_spell( 369238 )->effectN( 1 ).average( effect.item );
        aoe = -1;
        name_str_reporting = "boon_of_divine_command";
      }
    };

    struct boon_of_harvested_hope_t : public proc_spell_t
    {
      boon_of_harvested_hope_t( const special_effect_t& effect )
        : proc_spell_t( "boon_of_harvested_hope_damage", effect.player, effect.player->find_spell( 368701 ) )
      {
        base_td = effect.player->find_spell( 369238 )->effectN( 5 ).average( effect.item );
        name_str_reporting = "boon_of_harvested_hope";
      }
    };

    struct boon_of_assured_victory_t : public proc_spell_t
    {
      boon_of_assured_victory_t( const special_effect_t& effect )
        : proc_spell_t( "boon_of_assured_victory_damage", effect.player, effect.player->find_spell( 368700 ) )
      {
        base_td = effect.player->find_spell( 369238 )->effectN( 2 ).average( effect.item );
        name_str_reporting = "rotting_decay";
      }
    };

    struct boon_of_the_end_t : public proc_spell_t
    {
      boon_of_the_end_t( const special_effect_t& effect )
        : proc_spell_t( "boon_of_the_end_damage", effect.player, effect.player->find_spell( 368702 ) )
      {
        base_dd_min = base_dd_max = effect.player->find_spell( 369238 )->effectN( 6 ).average( effect.item );
        aoe = -1;
        name_str_reporting = "boon_of_the_end";
      }
    };

    void execute() override
    {
      proc_spell_t::execute();
      // Here is where we select which buff we are going to trigger via random selection
      auto selected_buff = player->sim->rng().range( buffs.size() );

      buffs[ selected_buff ]->trigger();
      if ( selected_buff == 4 )  //  Boon of the end gives STR as well while it's up
        boon_of_the_end_str_buff->trigger();
    }
  };

  effect.type           = SPECIAL_EFFECT_USE;
  effect.execute_action = create_proc_action<twisted_judgment_t>( "twisted_judgment", effect );
  effect.cooldown_group_name_override = "item_cd_1141_gcd";
  effect.cooldown_group_duration_override = effect.driver()->gcd();
}

// Armor

/**Passably-Forged Credentials
 * id=352081 driver
 * id=352091 buff
 */
void passablyforged_credentials( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "passable_credentials" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "passable_credentials", effect.trigger() )
             ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  effect.custom_buff = buff;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE | PF2_PERIODIC_HEAL;
  new dbc_proc_callback_t( effect.player, effect );
}

/**Dark Ranger's Quiver
    353513 driver, damage on effect 1
    353514 trigger buff
    353515 cleave damage at max stacks
 */
void dark_rangers_quiver( special_effect_t& effect )
{
  if ( effect.player->type != HUNTER )
    return;

  struct withering_fire_t : public proc_spell_t
  {
    withering_fire_t( const special_effect_t& effect ) : proc_spell_t( "withering_fire", effect.player, effect.player->find_spell( 353515 ), effect.item )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
      aoe = data().max_targets();
    }
  };

  auto cleave = create_proc_action<withering_fire_t>( "withering_fire", effect );
  auto buff = buff_t::find( effect.player, "withering_fire" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "withering_fire", effect.trigger() );
    buff->set_stack_change_callback( [ cleave ]( buff_t* buff, int, int cur ) {
      if ( cur == buff->max_stack() )
      {
        cleave->set_target( buff->player->target );
        cleave->execute();
        make_event( buff->sim, [ buff ] { buff->expire(); } );
      }
    } );
  }

  effect.custom_buff = buff;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE;
  new dbc_proc_callback_t( effect.player, effect );
}

void soulwarped_seal_of_wrynn( special_effect_t& effect )
{
  if ( effect.player->type != PRIEST )
  {
    return;
  }

  struct lions_hope_cb_t : public dbc_proc_callback_t
  {
    lions_hope_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.item, effect )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      assert( rppm );
      assert( s->target );

      double mod = 1;

      // Appears to be roughly 2 rppm + hasted above 30% HP
      // Below that it will just be 20 rppm + hasted
      // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/886
      if ( s->target->health_percentage() >= 30 && !effect.player->bugs )
      {
        mod = 0.1;
      }

      if ( rppm->get_modifier() != mod )
      {
        if ( effect.player->sim->debug )
        {
          effect.player->sim->out_debug.printf( "Player %s adjusts %s rppm modifer: old=%.3f new=%.3f",
                                                effect.player->name(), effect.name().c_str(), rppm->get_modifier(),
                                                mod );
        }

        rppm->set_modifier( mod );
      }

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  auto buff = buff_t::find( effect.player, "lions_hope" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "lions_hope", effect.player->find_spell( 368689 ) )
               ->add_stat( STAT_INTELLECT, effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  effect.custom_buff = buff;
  // TODO: seems to not proc at all on healing right now, not sure how to remove that
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new lions_hope_cb_t( effect );
}

void soulwarped_seal_of_menethil( special_effect_t& effect )
{
  if ( effect.player->type != DEATH_KNIGHT )
  {
    return;
  }

  struct remnants_despair_cb_t : public dbc_proc_callback_t
  {
    double debuff_value;
    remnants_despair_cb_t( const special_effect_t& effect ) : dbc_proc_callback_t( effect.item, effect ),
    debuff_value(effect.driver()->effectN(1).average(effect.item))
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      assert( rppm );
      assert( s->target );

      // Below 70% HP, proc rate appears to be 2rppm
      double mod = 0.150;
	  
      // Above 70% HP, proc rate appears to be the full 20rppm.
      if ( s -> target -> health_percentage() >= 70 )
		mod = 1;

      if ( effect.player->sim->debug )
      {
        effect.player->sim->out_debug.printf( "Player %s adjusts %s rppm modifer: old=%.3f new=%.3f",
                                              effect.player->name(), effect.name().c_str(), rppm->get_modifier(), mod );
      }

      rppm->set_modifier( mod );

      dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );
      auto td = a->player->get_target_data( s->target );
      td->debuff.remnants_despair->set_default_value( debuff_value );
      td->debuff.remnants_despair->trigger();
    }
  };

  // TODO: verify flags
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new remnants_despair_cb_t( effect );
}

// Runecarves

void echo_of_eonar( special_effect_t& effect )
{
  if ( !effect.player->buffs.echo_of_eonar )
  {
    effect.player->buffs.echo_of_eonar =
      make_buff( effect.player, "echo_of_eonar", effect.player->find_spell( 347458 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
        ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  // TODO: buff to allies? (id=338489)

  effect.custom_buff = effect.player->buffs.echo_of_eonar;

  new dbc_proc_callback_t( effect.player, effect );
}

void judgment_of_the_arbiter( special_effect_t& effect )
{
  struct judgment_of_the_arbiter_arc_t : public proc_spell_t
  {
    judgment_of_the_arbiter_arc_t( const special_effect_t& e )
      : proc_spell_t( "judgment_of_the_arbiter_arc", e.player, e.player->find_spell( 339675 ) )
    {
      aoe    = -1;
      radius = 20.0;  // hardcoded here as decription says arcing happens to ally 'within 5-20 yards of you'
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      proc_spell_t::available_targets( tl );

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* ) {
        return !rng().roll( sim->shadowlands_opts.judgment_of_the_arbiter_arc_chance );
      }), tl.end() );

      return tl.size();
    }

    void execute() override
    {
      // Calling available_targets here to forces regeneration of target_cache every execute, since we're using a random
      // roll to model targets getting hit by the arc. Also prevents executes when rolls result in no targets, so we
      // don't get inaccurate execute count reporting.
      if ( available_targets( target_cache.list ) )
        proc_spell_t::execute();
      else
      {
        // sanity check, but this shouldn't be required for normal execution as no state is passed to schedule_execute()
        if ( pre_execute_state )
          action_state_t::release( pre_execute_state );
      }
    }
  };

  struct judgment_of_the_arbiter_proc_t : public proc_spell_t
  {
    action_t* arc;

    judgment_of_the_arbiter_proc_t( const special_effect_t& e ) : proc_spell_t( e ), arc( nullptr )
    {
      if ( sim->shadowlands_opts.judgment_of_the_arbiter_arc_chance > 0.0 )
      {
        arc = create_proc_action<judgment_of_the_arbiter_arc_t>( "judgment_of_the_arbiter_arc", e );
        add_child( arc );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( arc )
      {
        arc->set_target( execute_state->target );
        arc->schedule_execute();
      }
    }
  };

  effect.execute_action = create_proc_action<judgment_of_the_arbiter_proc_t>( "judgment_of_the_arbiter", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void maw_rattle( special_effect_t& /* effect */ )
{

}

void norgannons_sagacity( special_effect_t& effect )
{
  effect.proc_flags2_ |= PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.custom_buff = effect.player->buffs.norgannons_sagacity_stacks;

  new dbc_proc_callback_t( effect.player, effect );
}

void sephuzs_proclamation( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "sephuzs_proclamation" );
  if ( !buff )
  {
    auto val = effect.trigger()->effectN( 1 ).average( effect.player );
    buff     = make_buff<stat_buff_t>( effect.player, "sephuzs_proclamation", effect.trigger() )
               ->add_stat( STAT_HASTE_RATING, val )
               ->add_stat( STAT_CRIT_RATING, val )
               ->add_stat( STAT_MASTERY_RATING, val )
               ->add_stat( STAT_VERSATILITY_RATING, val );
  }

  effect.proc_flags2_ |= PF2_CAST_INTERRUPT;
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void third_eye_of_the_jailer( special_effect_t& /* effect */ )
{

}

void vitality_sacrifice( special_effect_t& /* effect */ )
{

}

namespace shards_of_domination
{
  using id_rank_pair_t = std::pair<int, unsigned>;
  static constexpr id_rank_pair_t ranks[] = {
    // Blood
    { 187057, 1},  // Shard of Bek
    { 187284, 2},
    { 187293, 3},
    { 187302, 4},
    { 187312, 5},
    { 187059, 1},  // Shard of Jas
    { 187285, 2},
    { 187294, 3},
    { 187303, 4},
    { 187313, 5},
    { 187061, 1},  // Shard of Rev
    { 187286, 2},
    { 187295, 3},
    { 187304, 4},
    { 187314, 5},
    // Frost
    { 187063, 1},  // Shard of Cor
    { 187287, 2},
    { 187296, 3},
    { 187305, 4},
    { 187315, 5},
    { 187065, 1},  // Shard of Kyr
    { 187288, 2},
    { 187297, 3},
    { 187306, 4},
    { 187316, 5},
    { 187071, 1},  // Shard of Tel
    { 187289, 2},
    { 187298, 3},
    { 187307, 4},
    { 187317, 5},
    // Unholy
    { 187073, 1},  // Shard of Dyz
    { 187290, 2},
    { 187299, 3},
    { 187308, 4},
    { 187318, 5},
    { 187076, 1},  // Shard of Oth
    { 187291, 2},
    { 187300, 3},
    { 187309, 4},
    { 187319, 5},
    { 187079, 1},  // Shard of Zed
    { 187292, 2},
    { 187301, 3},
    { 187310, 4},
    { 187320, 5},
  };

// Helper function to determine whether a Rune Word is active. Returns rank of the lowest shard if found.
int rune_word_active( const player_t* player, const spell_data_t* driver, spell_label label )
{
  if ( !player->sim->shadowlands_opts.enable_rune_words || !player->sim->shadowlands_opts.enable_domination_gems )
  {
    player->sim->print_debug( "{}: rune word {} is inactive by global override", player->name(), driver->name_cstr() );
    return 0;
  }

  unsigned equipped_shards = 0;
  unsigned rank = 0;

  for ( const auto& item : player->items )
  {
    // We cannot just check special effects because some of the Shard of Domination effects
    // are simple stat bonuses and a special effect with a spell ID will not be created.
    for ( auto gem_id : item.parsed.gem_id )
    {
      if ( gem_id == 0 )
        continue;

      const auto& gem = item.player->dbc->item( gem_id );
      if ( gem.id == 0 )
        throw std::invalid_argument( fmt::format( "No gem data for id {}.", gem_id ) );

      const gem_property_data_t& gem_prop = item.player->dbc->gem_property( gem.gem_properties );
      if ( !gem_prop.id )
        continue;

      if ( gem_prop.color != SOCKET_COLOR_SHARD_OF_DOMINATION )
        continue;

      const item_enchantment_data_t& enchant_data = item.player->dbc->item_enchantment( gem_prop.enchant_id );
      for ( size_t i = 0; i < std::size( enchant_data.ench_prop ); i++ )
      {
        switch ( enchant_data.ench_type[ i ] )
        {
          case ITEM_ENCHANTMENT_COMBAT_SPELL:
          case ITEM_ENCHANTMENT_EQUIP_SPELL:
          case ITEM_ENCHANTMENT_USE_SPELL:
          {
            auto spell = item.player->find_spell( enchant_data.ench_prop[ i ] );
            if ( spell->affected_by_label( label ) )
            {
              equipped_shards++;

              auto it = range::find( ranks, gem_id, &id_rank_pair_t::first );
              if ( it != range::end( ranks ) )
              {
                if ( rank == 0 )
                  rank = it->second;
                else
                  rank = std::min( rank, it->second );
              }
            }
            break;
          }
          default:
            break;
        }
      }
    }
  }

  int active = equipped_shards >= 3 ? as<int>( rank ) : 0;
  player->sim->print_debug(
      "{}: rune word {} is {} (rank {}) with {}/3 shards of domination equipped", player->name(),
      util::tokenize_fn( driver->name_cstr() ), active ? "active" : "inactive", rank,
      equipped_shards );

  return active;
}

report::sc_html_stream& generate_report( const player_t& player, report::sc_html_stream& root )
{
  if ( !player.sim->shadowlands_opts.enable_domination_gems )
    return root;

  std::string report_str;
  struct shard_data
  {
    unsigned rank;
    unsigned count;
    spell_label type_label;
    const spell_data_t* spell;

    shard_data( spell_label type_label, const spell_data_t* spell ) :
      rank( 0 ), count( 0 ), type_label( type_label ), spell( spell )
    {}
  };

  shard_data shard_types[ 3 ] = { shard_data( LABEL_SHARD_OF_DOMINATION_BLOOD, player.find_spell( 357347 ) ),
                                  shard_data( LABEL_SHARD_OF_DOMINATION_FROST, player.find_spell( 357348 ) ),
                                  shard_data( LABEL_SHARD_OF_DOMINATION_UNHOLY, player.find_spell( 357349 ) ) };

  // Domination Shards
  for ( const auto& item : player.items )
  {
    for ( auto gem_id : item.parsed.gem_id )
    {
      if ( gem_id == 0 )
        continue;

      const auto& gem = item.player->dbc->item( gem_id );
      if ( gem.id == 0 )
        continue;

      const gem_property_data_t& gem_prop = item.player->dbc->gem_property( gem.gem_properties );
      if ( !gem_prop.id || gem_prop.color != SOCKET_COLOR_SHARD_OF_DOMINATION )
        continue;

      const item_enchantment_data_t& enchant_data = item.player->dbc->item_enchantment( gem_prop.enchant_id );
      for ( size_t i = 0; i < std::size( enchant_data.ench_prop ); i++ )
      {
        switch ( enchant_data.ench_type[ i ] )
        {
          case ITEM_ENCHANTMENT_COMBAT_SPELL:
          case ITEM_ENCHANTMENT_EQUIP_SPELL:
          case ITEM_ENCHANTMENT_USE_SPELL:
          {
            unsigned rank = 0;
            auto it = range::find( ranks, gem_id, &id_rank_pair_t::first );
            if ( it != range::end( ranks ) )
            {
              rank = it->second;
              auto spell = item.player->find_spell( enchant_data.ench_prop[ i ] );
              
              for ( auto& shard_type : shard_types )
              {
                if ( spell->affected_by_label( shard_type.type_label ) )
                {
                  shard_type.rank = shard_type.rank == 0 ? rank : std::min( rank, shard_type.rank );
                  shard_type.count++;
                }
              }

              report_str += fmt::format( "<li class=\"nowrap\">{} ({})</li>\n",
                                         report_decorators::decorated_spell_data( *player.sim, spell ), rank );
            }
            break;
          }
          default:
            break;
        }
      }
    }
  }

  if ( !report_str.empty() )
  {
    root << "<tr class=\"left\"><th>Shards</th><td><ul class=\"float\">\n";
    root << report_str;
    root << "</ul></td></tr>\n";
  }

  // Rune Word Set Bonuses
  if ( player.sim->shadowlands_opts.enable_rune_words )
  {
    for ( auto shard_type : shard_types )
    {
      if ( shard_type.count >= 3 && shard_type.spell->ok() )
      {
        root << "<tr class=\"left\"><th>Rune Word</th><td><ul class=\"float\">\n";
        root << fmt::format( "<li>{} ({})</li>\n", report_decorators::decorated_spell_data( *player.sim, shard_type.spell ), shard_type.rank );
        root << "</ul></td></tr>\n";
        break;
      }
    }
  }

  return root;
}

std::unique_ptr<expr_t> create_expression( const player_t& player, util::string_view name_str )
{
  auto splits = util::string_split<util::string_view>( name_str, "." );
  
  if ( splits.size() < 2 )
  {
    return nullptr;
  }

  int rank = 0;
  if ( splits[ 1 ] == "blood" || splits[ 1 ] == "blood_link" )
  {
    rank = rune_word_active( &player, player.find_spell( 357347 ), LABEL_SHARD_OF_DOMINATION_BLOOD );
  }
  else if ( splits[ 1 ] == "frost" || splits[ 1 ] == "winds_of_winter" )
  {
    rank = rune_word_active( &player, player.find_spell( 357348 ), LABEL_SHARD_OF_DOMINATION_FROST );
  }
  else if ( splits[ 1 ] == "unholy" || splits[ 1 ] == "chaos_bane" )
  {
    rank = rune_word_active( &player, player.find_spell( 357349 ), LABEL_SHARD_OF_DOMINATION_UNHOLY );
  }
  else
  {
    throw std::invalid_argument( fmt::format( "Invalid Rune Word type {}", splits[ 1 ] ) );
  }

  if ( splits.size() == 2 || splits[ 2 ] == "enabled" )
  {
    return expr_t::create_constant( name_str, rank > 0 );
  }
  else if ( splits.size() == 3 && splits[ 3 ] == "rank" )
  {
    return expr_t::create_constant( name_str, rank );
  }

  return nullptr;
}
}

/**Blood Link
 * id=357347 equip special effect
 * id=355761 rank 1 driver and coefficient
 * id=359395 rank 2 driver
 * id=359420 rank 3 driver
 * id=359421 rank 4 driver
 * id=359422 rank 5 driver
 * id=355767 damage
 * id=355768 heal self
 * id=355769 heal other
 * id=355804 debuff
 */
void blood_link( special_effect_t& effect )
{
  auto rank = shards_of_domination::rune_word_active( effect.player, effect.driver(), LABEL_SHARD_OF_DOMINATION_BLOOD );
  if ( rank == 0 )
    return;

  auto buff = buff_t::find( effect.player, "blood_link" );
  if ( buff )
    return;

  struct blood_link_damage_t : proc_spell_t
  {
    blood_link_damage_t( const special_effect_t& e ) :
      proc_spell_t( "blood_link", e.player, e.player->find_spell( 355767 ) )
    {
      base_dd_min = e.driver()->effectN( 2 ).min( e.player );
      base_dd_max = e.driver()->effectN( 2 ).max( e.player );
    }
  };

  struct blood_link_cb_t : public dbc_proc_callback_t
  {
    timespan_t applied;
    player_t*  target;

    const spell_data_t* debuff;

    blood_link_cb_t( const special_effect_t& e ) :
      dbc_proc_callback_t( e.player, e ),
      applied( timespan_t::zero() ), target( nullptr ),
      debuff( e.player->find_spell( 355804 ) )
    { }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( a->hit_any_target )
      {
        applied = listener->sim->current_time();
        target = s->target;
      }
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();

      applied = timespan_t::zero();
      target = nullptr;
    }

    bool is_applied() const
    {
      return target && !target->is_sleeping() &&
             listener->sim->current_time() - applied <= debuff->duration();
    }
  };

  switch ( rank )
  {
    default: effect.spell_id = 355761; break;
    case 2:  effect.spell_id = 359395; break;
    case 3:  effect.spell_id = 359420; break;
    case 4:  effect.spell_id = 359421; break;
    case 5:  effect.spell_id = 359422; break;
  }

  auto cb = new blood_link_cb_t( effect );
  auto tick_action = create_proc_action<blood_link_damage_t>( "blood_link", effect );
  buff = make_buff( effect.player, "blood_link", effect.driver() )
           ->set_quiet( true )
           ->set_can_cancel( false )
           ->set_tick_callback( [ cb, tick_action ] ( buff_t*, int, timespan_t )
             {
                if ( cb->is_applied() )
                {
                  tick_action->set_target( cb->target );
                  tick_action->execute();
                }
             } );

  //effect.execute_action = dot_action;
  effect.player->register_combat_begin( [ buff ]( player_t* /* p */ )
  {
    buff->trigger();
  } );
}

/**Winds of Winter
 * id=357348 equip special effect
 * id=355724 rank 1 driver and damage cap coefficient
 * id=359387 rank 2 driver
 * id=359423 rank 3 driver
 * id=359424 rank 4 driver
 * id=359425 rank 5 driver
 * id=355733 damage
 * id=355735 absorb (NYI)
 */
void winds_of_winter( special_effect_t& effect )
{
  auto rank = shards_of_domination::rune_word_active( effect.player, effect.driver(), LABEL_SHARD_OF_DOMINATION_FROST );
  if ( rank == 0 )
    return;

  auto buff = buff_t::find( effect.player, "winds_of_winter" );
  if ( buff )
    return;

  struct winds_of_winter_damage_t : proc_spell_t
  {
    winds_of_winter_damage_t( const special_effect_t& e )
      : proc_spell_t( "winds_of_winter", e.player, e.player->find_spell( 355733 ) )
    {
      base_dd_min = base_dd_max = 1.0; // Ensure that the correct snapshot flags are set.
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      // Ignore Positive Damage Taken Modifiers (321)
      return std::min( proc_spell_t::composite_target_multiplier( target ), 1.0 );
    }
  };

  struct winds_of_winter_cb_t : public dbc_proc_callback_t
  {
    double accumulated_damage;
    double damage_cap;
    double damage_fraction;
    action_t* damage;
    buff_t* buff;
    player_t* target;

    winds_of_winter_cb_t( const special_effect_t& e, action_t* a, buff_t* b ) :
      dbc_proc_callback_t( e.player, e ),
      accumulated_damage(),
      damage_cap( e.driver()->effectN( 1 ).average( e.player ) ),
      damage_fraction( e.driver()->effectN( 2 ).percent() ),
      damage( a ),
      buff( b ),
      target()
    {
      buff->set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
      {
        if ( !target || target->is_sleeping() || accumulated_damage == 0 )
        {
          // If there is no valid target on a tick, the absorb triggers and damage is lost
          accumulated_damage = 0;
          return;
        }

        damage->base_dd_min = damage->base_dd_max = accumulated_damage;
        accumulated_damage = 0;
        damage->set_target( target );
        damage->execute();
      } );
    }

    void execute( action_t*, action_state_t* s ) override
    {
      if ( s->target->is_sleeping() )
        return;

      accumulated_damage += std::min( damage_cap, s->result_amount * damage_fraction );
      target = s->target; // Winds of Winter hits the last target you accumulated damage from.
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      accumulated_damage = 0.0;
    }
  };

  switch ( rank )
  {
    default: effect.spell_id = 355724; break;
    case 2:  effect.spell_id = 359387; break;
    case 3:  effect.spell_id = 359423; break;
    case 4:  effect.spell_id = 359424; break;
    case 5:  effect.spell_id = 359425; break;
  }

  effect.proc_flags2_ = PF2_CRIT;
  auto damage = create_proc_action<winds_of_winter_damage_t>( "winds_of_winter", effect );

  buff = make_buff( effect.player, "winds_of_winter", effect.driver() )
           ->set_quiet( true )
           ->set_can_cancel( false );

  new winds_of_winter_cb_t( effect, damage, buff );
  effect.player->register_combat_begin( [ buff ]( player_t* )
  {
    buff->trigger();
  } );
}

/**Chaos Bane
 * id=357349 equip special effect
 * id=355829 rank 1 driver with RPPM and buff trigger & coefficients
 *    effect#3 is soul fragment stat coefficient
 *    effect#4 is chaos bane stat coefficient
 *    effect#5 is chaos bane damage coefficient
 * id=359396 rank 2 driver
 * id=359435 rank 3 driver
 * id=359436 rank 4 driver
 * id=359437 rank 5 driver
 * id=356042 Soul Fragment buff
 * id=356043 Chaos Bane buff
 * id=356046 damage
 */
void chaos_bane( special_effect_t& effect )
{

  if ( unique_gear::create_fallback_buffs( effect, { "chaos_bane" } ) )
	  return;

  auto rank = shards_of_domination::rune_word_active( effect.player, effect.driver(), LABEL_SHARD_OF_DOMINATION_UNHOLY );

  auto buff = buff_t::find( effect.player, "chaos_bane" );
  if ( buff )
    return;

  // If we are running with rune words disbaled, we need to still create the fallback buff for apl use
  if ( effect.player->sim->shadowlands_opts.enable_rune_words == 0 || rank == 0 )
  {
    if ( !buff )
      make_buff( effect.player, "chaos_bane" )->set_chance( 0.0 );

    return;
  }

  struct chaos_bane_t : proc_spell_t
  {
    buff_t* buff;
    chaos_bane_t( const special_effect_t& e, buff_t* b ) :
      proc_spell_t( "chaos_bane", e.player, e.player->find_spell( 356046 ) ),
      buff( b )
    {
      // coeff in driver eff#5
      base_dd_max = base_dd_min = e.driver()->effectN( 5 ).average( e.player );
      split_aoe_damage = true;
    }

    void execute() override
    {
      buff->trigger();

      proc_spell_t::execute();
    }
  };

  struct chaos_bane_cb_t : public dbc_proc_callback_t
  {
    buff_t* chaos_bane;

    chaos_bane_cb_t( const special_effect_t& e, buff_t* b ) :
      dbc_proc_callback_t( e.player, e ),
      chaos_bane( b )
    {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( chaos_bane->check() )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  switch ( rank )
  {
    default: effect.spell_id = 355829; break;
    case 2:  effect.spell_id = 359396; break;
    case 3:  effect.spell_id = 359435; break;
    case 4:  effect.spell_id = 359436; break;
    case 5:  effect.spell_id = 359437; break;
  }

  buff = make_buff<stat_buff_t>( effect.player, "chaos_bane", effect.player->find_spell( 356043 ) )
    // coeff in driver eff#4
    ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ),
                effect.driver()->effectN( 4 ).average( effect.player ) )
    ->set_can_cancel( false );

  auto proc = create_proc_action<chaos_bane_t>( "chaos_bane", effect, buff );
  effect.custom_buff = buff_t::find( effect.player, "soul_fragment" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff =
        make_buff<stat_buff_t>( effect.player, "soul_fragment", effect.player->find_spell( 356042 ) )
          // coeff in driver eff#3
          ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ),
                      effect.driver()->effectN( 3 ).average( effect.player ) )
          ->set_can_cancel( false )
          ->set_stack_change_callback( [ proc ]( buff_t* b, int, int cur ) {
            if ( cur == b->max_stack() )
            {
              proc->set_target( b->player->target );
              proc->execute();
              make_event( b->sim, [ b ] { b->expire(); } );
            }
          } );
  }

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE | PF2_PERIODIC_HEAL;
  new chaos_bane_cb_t( effect, buff );
}

/**Shard of Dyz
 * id=355755 driver Rank 1
 * id=357037 driver Rank 2 (Ominous)
 * id=357055 driver Rank 3 (Desolate)
 * id=357065 driver Rank 4 (Foreboding)
 * id=357076 driver Rank 5 (Portentous)
 * id=356329 Scouring Touch debuff
 */
void shard_of_dyz( special_effect_t& effect )
{
  if ( !effect.player->sim->shadowlands_opts.enable_domination_gems )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  struct shard_of_dyz_cb_t : public dbc_proc_callback_t
  {
    double debuff_value;

    shard_of_dyz_cb_t( const special_effect_t& e ) :
      dbc_proc_callback_t( e.player, e ),
      debuff_value( 0.0001 * e.driver()->effectN( 1 ).average( e.player) )
    {}

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto td = a->player->get_target_data( s->target );
      td->debuff.scouring_touch->set_default_value( debuff_value );
      td->debuff.scouring_touch->trigger();
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new shard_of_dyz_cb_t( effect );
}

/**Shard of Cor
 * id=355741 driver Rank 1
 * id=357034 driver Rank 2 (Ominous)
 * id=357052 driver Rank 3 (Desolate)
 * id=357062 driver Rank 4 (Foreboding)
 * id=357073 driver Rank 5 (Portentous)
 * id=356364 Coldhearted buff
 */
void shard_of_cor( special_effect_t& effect )
{
  if ( !effect.player->sim->shadowlands_opts.enable_domination_gems )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  struct shard_of_cor_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    shard_of_cor_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        target_list()
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( range::contains( target_list, s->target->actor_spawn_index ) )
        return;

      dbc_proc_callback_t::execute( a, s );
      target_list.push_back( s->target->actor_spawn_index );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      target_list.clear();
    }
  };

  buff_t* buff = buff_t::find( effect.player, "coldhearted" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "coldhearted", effect.player->find_spell( 356364 ) )
               ->set_default_value( 0.0001 * effect.driver()->effectN( 1 ).average( effect.player ), 1 )
               ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  effect.custom_buff = effect.player->buffs.coldhearted = buff;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new shard_of_cor_cb_t( effect );
}

/**Shard of Bek
 * id=355721 driver Rank 1
 * id=357031 driver Rank 2 (Ominous)
 * id=357049 driver Rank 3 (Desolate)
 * id=357058 driver Rank 4 (Foreboding)
 * id=357069 driver Rank 5 (Portentous)
 * id=356372 Exsanguinated debuff
 */
void shard_of_bek( special_effect_t& effect )
{
  if ( !effect.player->sim->shadowlands_opts.enable_domination_gems )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  struct shard_of_bek_cb_t : public dbc_proc_callback_t
  {
    double debuff_value;
    double health_pct_threshold;

    shard_of_bek_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        debuff_value( 0.0001 * e.driver()->effectN( 1 ).average( e.player ) ),
        health_pct_threshold( e.driver()->effectN( 2 ).base_value() )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      auto td = a->player->get_target_data( s->target );
      if ( td->debuff.exsanguinated->check() )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto health_diff = a->player->health_percentage() - s->target->health_percentage();
      if ( health_diff > health_pct_threshold )
      {
        auto td = a->player->get_target_data( s->target );
        td->debuff.exsanguinated->set_default_value( debuff_value );
        td->debuff.exsanguinated->trigger();
      }
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new shard_of_bek_cb_t( effect );
}

/**Shard of Zed
 * id=355766 driver Rank 1
 * id=357040 driver Rank 2 (Ominous)
 * id=357057 driver Rank 3 (Desolate)
 * id=357067 driver Rank 4 (Foreboding)
 * id=357077 driver Rank 5 (Portentous)
 * id=356321 Unholy Aura buff with periodic dummy
 * id=356320 Siphon Essence AoE leech spell
 */
void shard_of_zed( special_effect_t& effect )
{
  if ( !effect.player->sim->shadowlands_opts.enable_domination_gems )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  struct siphon_essence_t : proc_spell_t
  {
    siphon_essence_t( const special_effect_t& e ) :
      proc_spell_t( "siphon_essence", e.player, e.player->find_spell( 356320 ) )
    {
      base_dd_max = base_dd_min = e.driver()->effectN( 1 ).average( e.player );
    }
  };

  auto buff = buff_t::find( effect.player, "unholy_aura" );
  if ( !buff )
  {
    auto tick_damage = create_proc_action<siphon_essence_t>( "siphon_essence", effect );
    auto range = tick_damage->data().effectN( 1 ).radius_max();

    buff = make_buff( effect.player, "unholy_aura", effect.player->find_spell( 356321 ) )
      ->set_tick_callback( [ tick_damage, range ]( buff_t* buff, int, timespan_t )
        {
          if ( buff->player->get_player_distance( *buff->player->target ) <= range )
          {
            tick_damage->set_target( buff->player->target );
            tick_damage->execute();
          }
        } );
  }

  struct unholy_aura_event_t : public event_t
  {
    buff_t* unholy_aura;
    const spell_data_t* driver;

    unholy_aura_event_t( buff_t* aura_, const spell_data_t* driver_, timespan_t interval_ ) :
      event_t( *aura_->source, interval_ ), unholy_aura( aura_ ), driver( driver_ )
    { }

    const char* name() const override
    { return "unholy_aura_driver"; }

    void execute() override
    {
      bool has_eligible_targets = range::count_if( sim().target_non_sleeping_list,
          []( player_t* enemy ) {
        return !enemy->debuffs.invulnerable || enemy->debuffs.invulnerable->check() == 0;
      } ) != 0;

      if ( has_eligible_targets )
      {
        unholy_aura->trigger();
      }

      // If no eligible targets are found, try to look for one soon after. This is
      // intended to roughly model situations such as dungeonslice, where the iteration
      // may have periods of time where only the invulnerable base target is available.
      make_event<unholy_aura_event_t>( sim(), unholy_aura, driver,
          has_eligible_targets ? driver->internal_cooldown() : 1_s );
    }
  };

  // We don't need to create a special effect out of this
  effect.type = SPECIAL_EFFECT_NONE;

  // Trigger unholy aura early on in the fight for the first time
  effect.player->register_combat_begin( [ buff, &effect ]( player_t*  p ) {
      make_event<unholy_aura_event_t>( *p->sim, buff, effect.driver(), 1_s );
  } );

  /*
  // TODO: confirm proc flags
  // 2021-07-15 - Logs seem somewhat inconclusive on this, perhaps it is delayed in triggering
  effect.proc_flags_ = PF_ALL_HEAL | PF_PERIODIC;
  effect.proc_flags2_ = PF2_LANDED | PF2_PERIODIC_HEAL;

  new dbc_proc_callback_t( effect.player, effect );
  */
}

/**Shard of Kyr
 * id=355743 driver Rank 1
 * id=357035 driver Rank 2 (Ominous)
 * id=357053 driver Rank 3 (Desolate)
 * id=357063 driver Rank 4 (Foreboding)
 * id=357074 driver Rank 5 (Portentous)
 * id=356305 Accretion buff
 */
void shard_of_kyr( special_effect_t& effect )
{
  if ( !effect.player->sim->shadowlands_opts.enable_domination_gems )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  auto buff = buff_t::find( effect.player, "accretion" );
  if ( !buff )
  {
    auto per_five_amount = effect.driver()->effectN( 1 ).average( effect.player );
    auto max_amount      = effect.driver()->effectN( 2 ).average( effect.player );

    buff = make_buff<absorb_buff_t>( effect.player, "accretion", effect.player->find_spell( 356305 ) )
               ->set_default_value( per_five_amount );

    effect.custom_buff = buff;
    effect.player->register_combat_begin( [ &effect, buff, per_five_amount, max_amount ]( player_t* ) {
      buff->trigger();
      make_repeating_event( buff->source->sim, effect.driver()->effectN( 1 ).period(),
                            [ buff, per_five_amount, max_amount ]() {
                              if ( buff->check() )
                              {
                                auto new_value = buff->check_value() + per_five_amount;
                                buff->trigger( 1, std::min( new_value, max_amount ) );
                              }
                              else
                              {
                                buff->trigger();
                              }
                            } );
    } );
  }
}
}  // namespace items

namespace set_bonus
{
// TODO:
// 1) confirm that ilevel is averaged
// 2) confirm damage formula
// 3) if formula is correct investigate if the 10% extra damage can be found in spell data
void hack_and_gore( special_effect_t& effect )
{
  auto hack = effect.player->find_item_by_id( 184194 );
  auto gore = effect.player->find_item_by_id( 184195 );

  if ( !hack || !gore )
    return;

  int ilevel = ( hack->item_level() + gore->item_level() ) / 2;

  effect.execute_action = create_proc_action<proc_spell_t>( "gore", effect );
  effect.execute_action->base_td = effect.trigger()->effectN( 1 ).base_value() * ilevel;
  effect.execute_action->base_td_multiplier = 1.1;

  new dbc_proc_callback_t( effect.player, effect );
}

void branding_blade( special_effect_t& effect )
{
  auto ripped      = new special_effect_t( effect.player );
  ripped->type     = SPECIAL_EFFECT_EQUIP;
  ripped->source   = SPECIAL_EFFECT_SOURCE_ITEM;
  ripped->spell_id = 366757;
  effect.player->special_effects.push_back( ripped );

  auto ripped_dot =
      create_proc_action<generic_proc_t>( "ripped_secrets", *ripped, "ripped_secrets", ripped->trigger() );
  ripped_dot->base_td = ripped->driver()->effectN( 1 ).base_value();

  ripped->execute_action = ripped_dot;

  new dbc_proc_callback_t( effect.player, *ripped );

  effect.player->callbacks.register_callback_trigger_function(
      ripped->spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      []( const dbc_proc_callback_t*, action_t* a, action_state_t* ) {
        return a->weapon && a->weapon->slot == SLOT_MAIN_HAND;
      } );

  auto branding_dam =
      create_proc_action<generic_proc_t>( "branding_blade", effect, "branding_blade", effect.trigger() );
  branding_dam->base_dd_min = branding_dam->base_dd_max = effect.driver()->effectN( 1 ).base_value();

  effect.execute_action = branding_dam;

  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_trigger_function(
      effect.spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ ripped_dot ]( const dbc_proc_callback_t*, action_t* a, action_state_t* s ) {
        return a->weapon && a->weapon->slot == SLOT_OFF_HAND && s->target &&
               ripped_dot->get_dot( s->target )->is_ticking();
      } );
}
}  // namespace set_bonus

void register_hotfixes()
{
}

void register_special_effects()
{
    // Food
    unique_gear::register_special_effect( 308637, consumables::smothered_shank );
    unique_gear::register_special_effect( 308458, consumables::surprisingly_palatable_feast );
    unique_gear::register_special_effect( 308462, consumables::feast_of_gluttonous_hedonism );

    // Potion
    unique_gear::register_special_effect( 307497, consumables::potion_of_deathly_fixation );
    unique_gear::register_special_effect( 307494, consumables::potion_of_empowered_exorcisms );
    unique_gear::register_special_effect( 307495, consumables::potion_of_phantom_fire );

    // Enchant
    unique_gear::register_special_effect( 324747, enchants::celestial_guidance );
    unique_gear::register_special_effect( 323932, enchants::lightless_force );
    unique_gear::register_special_effect( 324250, enchants::sinful_revelation );

    // Scopes
    unique_gear::register_special_effect( 321532, "329666trigger" ); // Infra-green Reflex Sight
    unique_gear::register_special_effect( 321533, "330038trigger" ); // Optical Target Embiggener

    // Set Bonus
    unique_gear::register_special_effect( 337893, set_bonus::hack_and_gore );
    //as current set bonus parsing can only handle one spell per bonus, both effects are handled in branding_blade
    //unique_gear::register_special_effect( 366757, set_bonus::ripped_secrets );
    unique_gear::register_special_effect( 366876, set_bonus::branding_blade );

    // Trinkets
    unique_gear::register_special_effect( 347047, items::darkmoon_deck_putrescence );
    unique_gear::register_special_effect( 331624, items::darkmoon_deck_voracity );
    unique_gear::register_special_effect( 344686, items::stone_legion_heraldry );
    unique_gear::register_special_effect( 344806, items::cabalists_hymnal );
    unique_gear::register_special_effect( 345432, items::macabre_sheet_music );
    unique_gear::register_special_effect( 345319, items::glyph_of_assimilation );
    unique_gear::register_special_effect( 345251, items::soul_igniter );
    unique_gear::register_special_effect( 345019, items::skulkers_wing );
    unique_gear::register_special_effect( 344662, items::memory_of_past_sins );
    unique_gear::register_special_effect( 344063, items::gluttonous_spike );
    unique_gear::register_special_effect( 345357, items::hateful_chain );
    unique_gear::register_special_effect( 343385, items::overflowing_anima_cage );
    unique_gear::register_special_effect( 343393, items::sunblood_amethyst );
    unique_gear::register_special_effect( 345545, items::bottled_flayedwing_toxin );
    unique_gear::register_special_effect( 345539, items::empyreal_ordnance );
    unique_gear::register_special_effect( 345801, items::soulletting_ruby );
    unique_gear::register_special_effect( 345567, items::satchel_of_misbegotten_minions );
    unique_gear::register_special_effect( 330747, items::unbound_changeling );
    unique_gear::register_special_effect( 330765, items::unbound_changeling );
    unique_gear::register_special_effect( 330080, items::unbound_changeling );
    unique_gear::register_special_effect( 330733, items::unbound_changeling );
    unique_gear::register_special_effect( 330734, items::unbound_changeling );
    unique_gear::register_special_effect( 345490, items::infinitely_divisible_ooze );
    unique_gear::register_special_effect( 330323, items::inscrutable_quantum_device );
    unique_gear::register_special_effect( 345465, items::phial_of_putrefaction );
    unique_gear::register_special_effect( 345739, items::grim_codex );
    unique_gear::register_special_effect( 345533, items::anima_field_emitter );
    unique_gear::register_special_effect( 342427, items::decanter_of_animacharged_winds );
    unique_gear::register_special_effect( 329840, items::bloodspattered_scale );
    unique_gear::register_special_effect( 331523, items::shadowgrasp_totem );
    unique_gear::register_special_effect( 348135, items::hymnal_of_the_path );
    unique_gear::register_special_effect( 330067, items::mistcaller_ocarina );
    unique_gear::register_special_effect( 332299, items::mistcaller_ocarina );
    unique_gear::register_special_effect( 332300, items::mistcaller_ocarina );
    unique_gear::register_special_effect( 332301, items::mistcaller_ocarina );
    unique_gear::register_special_effect( 336841, items::flame_of_battle );
    unique_gear::register_special_effect( 329831, items::overwhelming_power_crystal );
    unique_gear::register_special_effect( 336182, items::tablet_of_despair );
    unique_gear::register_special_effect( 329536, items::rotbriar_sprout );
    unique_gear::register_special_effect( 339343, items::murmurs_in_the_dark );
    unique_gear::register_special_effect( 336219, items::dueling_form );
    unique_gear::register_special_effect( 348139, items::instructors_divine_bell );
    unique_gear::register_special_effect( 367896, items::instructors_divine_bell );

    // 9.1 Trinkets
    unique_gear::register_special_effect( 353492, items::forbidden_necromantic_tome );
    unique_gear::register_special_effect( 357672, items::soul_cage_fragment );
    unique_gear::register_special_effect( 353692, items::tome_of_monstrous_constructions );
    unique_gear::register_special_effect( 352429, items::miniscule_mailemental_in_an_envelope );
    unique_gear::register_special_effect( 355323, items::decanter_of_endless_howling );
    unique_gear::register_special_effect( 355324, items::tormentors_rack_fragment );
    unique_gear::register_special_effect( 355297, items::old_warriors_soul );
    unique_gear::register_special_effect( 355333, items::salvaged_fusion_amplifier );
    unique_gear::register_special_effect( 355313, items::titanic_ocular_gland );
    unique_gear::register_special_effect( 355327, items::ebonsoul_vise );
    unique_gear::register_special_effect( 355301, items::relic_of_the_frozen_wastes_equip );
    unique_gear::register_special_effect( 355303, items::relic_of_the_frozen_wastes_use );
    unique_gear::register_special_effect( 355321, items::shadowed_orb_of_torment );
    unique_gear::register_special_effect( 351679, items::ticking_sack_of_terror );
    unique_gear::register_special_effect( 367901, items::ticking_sack_of_terror );
    unique_gear::register_special_effect( 351926, items::soleahs_secret_technique );
    unique_gear::register_special_effect( 368509, items::soleahs_secret_technique );
    unique_gear::register_special_effect( 355329, items::reactive_defense_matrix );

    // 9.2 Trinkets
    unique_gear::register_special_effect( 367973, items::extract_of_prodigious_sands );
    unique_gear::register_special_effect( 367464, items::brokers_lucky_coin );
    unique_gear::register_special_effect( 367722, items::symbol_of_the_lupine );
    unique_gear::register_special_effect( 367930, items::scars_of_fraternal_strife, true );
    unique_gear::register_special_effect( 368203, items::architects_ingenuity_core, true );
    unique_gear::register_special_effect( 367236, items::resonant_reservoir );
    unique_gear::register_special_effect( 367241, items::the_first_sigil, true );
    unique_gear::register_special_effect( 363481, items::cosmic_gladiators_resonator );
    unique_gear::register_special_effect( 367246, items::elegy_of_the_eternals );
    unique_gear::register_special_effect( 367336, items::bells_of_the_endless_feast );
    unique_gear::register_special_effect( 367924, items::grim_eclipse );
    unique_gear::register_special_effect( 367802, items::pulsating_riftshard );
    unique_gear::register_special_effect( 367805, items::cache_of_acquired_treasures, true );
    unique_gear::register_special_effect( 367733, items::symbol_of_the_raptora );
    unique_gear::register_special_effect( 367470, items::protectors_diffusion_implement );
    unique_gear::register_special_effect( 367682, items::vombatas_headbutt );
    unique_gear::register_special_effect( 367808, items::earthbreakers_impact );
    unique_gear::register_special_effect( 367325, items::prismatic_brilliance );
    unique_gear::register_special_effect( 367931, items::chains_of_domination );

    // Weapons
    unique_gear::register_special_effect( 331011, items::poxstorm );
    unique_gear::register_special_effect( 358562, items::jaithys_the_prison_blade_1 );
    unique_gear::register_special_effect( 358565, items::jaithys_the_prison_blade_2 );
    unique_gear::register_special_effect( 358567, items::jaithys_the_prison_blade_3 );
    unique_gear::register_special_effect( 358569, items::jaithys_the_prison_blade_4 );
    unique_gear::register_special_effect( 358571, items::jaithys_the_prison_blade_5 );
    unique_gear::register_special_effect( 351527, items::yasahm_the_riftbreaker );
    unique_gear::register_special_effect( 359168, items::cruciform_veinripper );

    // 9.2 Weapons
    unique_gear::register_special_effect( 367952, items::singularity_supreme, true );
    unique_gear::register_special_effect( 367953, items::gavel_of_the_first_arbiter );

    // Armor
    unique_gear::register_special_effect( 352081, items::passablyforged_credentials );
    unique_gear::register_special_effect( 353513, items::dark_rangers_quiver );
    unique_gear::register_special_effect( 367950, items::soulwarped_seal_of_wrynn );
    unique_gear::register_special_effect( 367951, items::soulwarped_seal_of_menethil );

    // Runecarves
    unique_gear::register_special_effect( 338477, items::echo_of_eonar );
    unique_gear::register_special_effect( 339344, items::judgment_of_the_arbiter );
    unique_gear::register_special_effect( 340197, items::maw_rattle );
    unique_gear::register_special_effect( 339340, items::norgannons_sagacity );
    unique_gear::register_special_effect( 339348, items::sephuzs_proclamation );
    unique_gear::register_special_effect( 339058, items::third_eye_of_the_jailer );
    unique_gear::register_special_effect( 338743, items::vitality_sacrifice );

    // 9.1 Shards of Domination
    unique_gear::register_special_effect( 357347, items::blood_link ); // Rune Word: Blood
    unique_gear::register_special_effect( 357348, items::winds_of_winter ); // Rune Word: Frost
    unique_gear::register_special_effect( 357349, items::chaos_bane, true ); // Rune Word: Unholy

    unique_gear::register_special_effect( 355755, items::shard_of_dyz );
    unique_gear::register_special_effect( 357037, items::shard_of_dyz );
    unique_gear::register_special_effect( 357055, items::shard_of_dyz );
    unique_gear::register_special_effect( 357065, items::shard_of_dyz );
    unique_gear::register_special_effect( 357076, items::shard_of_dyz );

    unique_gear::register_special_effect( 355741, items::shard_of_cor );
    unique_gear::register_special_effect( 357034, items::shard_of_cor );
    unique_gear::register_special_effect( 357052, items::shard_of_cor );
    unique_gear::register_special_effect( 357062, items::shard_of_cor );
    unique_gear::register_special_effect( 357073, items::shard_of_cor );

    unique_gear::register_special_effect( 355721, items::shard_of_bek );
    unique_gear::register_special_effect( 357031, items::shard_of_bek );
    unique_gear::register_special_effect( 357049, items::shard_of_bek );
    unique_gear::register_special_effect( 357058, items::shard_of_bek );
    unique_gear::register_special_effect( 357069, items::shard_of_bek );

    unique_gear::register_special_effect( 355766, items::shard_of_zed );
    unique_gear::register_special_effect( 357040, items::shard_of_zed );
    unique_gear::register_special_effect( 357057, items::shard_of_zed );
    unique_gear::register_special_effect( 357067, items::shard_of_zed );
    unique_gear::register_special_effect( 357077, items::shard_of_zed );

    unique_gear::register_special_effect( 355743, items::shard_of_kyr );
    unique_gear::register_special_effect( 357035, items::shard_of_kyr );
    unique_gear::register_special_effect( 357053, items::shard_of_kyr );
    unique_gear::register_special_effect( 357063, items::shard_of_kyr );
    unique_gear::register_special_effect( 357074, items::shard_of_kyr );

    // Disabled effects
    unique_gear::register_special_effect( 329028, DISABLED_EFFECT ); // Light-Infused Armor shield
    unique_gear::register_special_effect( 333885, DISABLED_EFFECT ); // Darkmoon Deck: Putrescence shuffler
    unique_gear::register_special_effect( 329446, DISABLED_EFFECT ); // Darkmoon Deck: Voracity shuffler
    unique_gear::register_special_effect( 364086, DISABLED_EFFECT ); // Cypher effect Strip Advantage
    unique_gear::register_special_effect( 364087, DISABLED_EFFECT ); // Cypher effect Cosmic Boom
}

action_t* create_action( player_t* player, util::string_view name, util::string_view options )
{
  if ( util::str_compare_ci( name, "break_chains_of_domination" ) )  return new items::break_chains_of_domination_t( player, options );
  if ( util::str_compare_ci( name, "antumbra_swap" ) )               return new items::antumbra_swap_t( player, options );
  
  return nullptr;
}

void register_target_data_initializers( sim_t& sim )
{
  // Sinful Revelation
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 324250 ) )
    {
      assert( !td->debuff.sinful_revelation );

      td->debuff.sinful_revelation = make_buff<SL_buff_t>( *td, "sinful_revelation", td->source->find_spell( 324260 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );
      td->debuff.sinful_revelation->reset();
    }
    else
      td->debuff.sinful_revelation = make_buff( *td, "sinful_revelation" )->set_quiet( true );
  } );

  // Darkmoon Deck: Putrescence
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 347047 ) )
    {
      assert( !td->debuff.putrid_burst );

      td->debuff.putrid_burst = make_buff<buff_t>( *td, "putrid_burst", td->source->find_spell( 334058 ) )
        ->set_cooldown( 0_ms );
      td->debuff.putrid_burst->reset();
    }
    else
      td->debuff.putrid_burst = make_buff( *td, "putrid_burst" )->set_quiet( true );
  } );

  // Memory of Past Sins
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 344662 ) )
    {
      assert( !td->debuff.shattered_psyche );

      td->debuff.shattered_psyche =
          make_buff<buff_t>( *td, "shattered_psyche_debuff", td->source->find_spell( 344663 ) )
              ->set_default_value( td->source->find_spell( 344662 )->effectN( 1 ).percent() );
      td->debuff.shattered_psyche->reset();
    }
    else
      td->debuff.shattered_psyche = make_buff( *td, "shattered_psyche_debuff" )->set_quiet( true );
  } );

  // Relic of the Frozen Wastes
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 355301 ) )
    {
      assert( !td->debuff.frozen_heart );

      td->debuff.frozen_heart = make_buff<buff_t>( *td, "frozen_heart", td->source->find_spell( 355759 ) );
      td->debuff.frozen_heart->reset();
    }
    else
      td->debuff.frozen_heart = make_buff( *td, "frozen_heart" )->set_quiet( true );
  } );

  // Ticking Sack of Terror
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 351679 ) )
    {
      assert( !td->debuff.volatile_satchel );

      td->debuff.volatile_satchel = make_buff<buff_t>( *td, "volatile_satchel", td->source->find_spell( 351682 ) );
      td->debuff.volatile_satchel->reset();
    }
    else if ( unique_gear::find_special_effect( td->source, 367901 ) )
    {
      assert( !td->debuff.volatile_satchel );

      td->debuff.volatile_satchel = make_buff<buff_t>( *td, "volatile_satchel", td->source->find_spell( 367902 ) );
      td->debuff.volatile_satchel->reset();
    }
    else
      td->debuff.volatile_satchel = make_buff( *td, "volatile_satchel" )->set_quiet( true );
  } );

  // Bells of the Endless Feast
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 367336 ) )
    {
      assert( !td->debuff.scent_of_souls );

      td->debuff.scent_of_souls = make_buff<buff_t>( *td, "scent_of_souls", td->source->find_spell( 368585 ) )
        ->set_period( 0_ms )
        ->set_cooldown( 0_ms );  // the debuff spell id seems to also be a driver of some kind giving extra stacks
      td->debuff.scent_of_souls->reset();
    }
    else
      td->debuff.scent_of_souls = make_buff( *td, "scent_of_souls" )->set_quiet( true );
  } );

  // Chains of Domination
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 367931 ) )
    {
      assert( !td->debuff.chains_of_domination );

      td->debuff.chains_of_domination = make_buff<buff_t>( *td, "chains_of_domination", td->source->find_spell( 367931 ) )
        ->set_period( 0_ms )
        ->set_cooldown( 0_ms );
      td->debuff.chains_of_domination->reset();
    }
    else
      td->debuff.chains_of_domination = make_buff( *td, "chains_of_domination" )->set_quiet( true );
  } );

  // Shard of Dyz (Scouring Touch debuff)
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 355755 )
      || unique_gear::find_special_effect( td->source, 357037 )
      || unique_gear::find_special_effect( td->source, 357055 )
      || unique_gear::find_special_effect( td->source, 357065 )
      || unique_gear::find_special_effect( td->source, 357076 ) )
    {
      assert( !td->debuff.scouring_touch );

      td->debuff.scouring_touch = make_buff( *td, "scouring_touch", td->source->find_spell( 356329 ) );
      td->debuff.scouring_touch->reset();
    }
    else
      td->debuff.scouring_touch = make_buff( *td, "scouring_touch" )->set_quiet( true );
  } );

  // Shard of Bek (Exsanguinated debuff)
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 355721 )
      || unique_gear::find_special_effect( td->source, 357031 )
      || unique_gear::find_special_effect( td->source, 357049 )
      || unique_gear::find_special_effect( td->source, 357058 )
      || unique_gear::find_special_effect( td->source, 357069 ) )
    {
      assert( !td->debuff.exsanguinated );

      td->debuff.exsanguinated = make_buff( *td, "exsanguinated", td->source->find_spell( 356372 ) );
      td->debuff.exsanguinated->reset();
    }
    else
      td->debuff.exsanguinated = make_buff( *td, "exsanguinated" )->set_quiet( true );
  } );

  // Soulwarped Seal of Menethil
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 367951 ) )
    {
      assert( !td->debuff.remnants_despair );

      td->debuff.remnants_despair = make_buff<buff_t>( *td, "remnants_despair", td->source->find_spell( 368690 ) );
      td->debuff.remnants_despair->reset();
    }
    else
      td->debuff.remnants_despair = make_buff( *td, "remnants_despair" )->set_quiet( true );
  } );
}

}  // namespace unique_gear

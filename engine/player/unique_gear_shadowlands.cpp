// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_shadowlands.hpp"

#include "sim/sc_sim.hpp"

#include "buff/sc_buff.hpp"
#include "action/dot.hpp"

#include "actor_target_data.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"

namespace unique_gear
{
namespace shadowlands
{
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
}

void feast_of_gluttonous_hedonism( special_effect_t& effect )
{

}

void potion_of_deathly_fixation( special_effect_t& effect )
{
  struct deathly_eruption_t: public proc_spell_t
  {
    deathly_eruption_t( const special_effect_t& e )
      : proc_spell_t( "deathly_eruption", e.player, e.player->find_spell( 322256 ) )
    {
      // TODO: add interaction with shadowcore oil
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
      auto d = get_dot( s->target );

      if ( d->at_max_stacks( 1 ) )
      {
        eruption->set_target( s->target );
        eruption->schedule_execute();
        d->cancel();
      }
      else
        proc_spell_t::impact( s );
    }
  };

  auto potion            = new special_effect_t( effect.player );
  potion->type           = SPECIAL_EFFECT_EQUIP;
  potion->spell_id       = effect.spell_id;
  potion->cooldown_      = 0_ms;
  potion->execute_action = create_proc_action<deathly_fixation_t>( "potion_of_deathly_fixation", effect );
  effect.player->special_effects.push_back( potion );

  auto proc = new dbc_proc_callback_t( effect.player, *potion );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
    ->set_cooldown( 0_ms )
    ->set_chance( 1.0 )
    ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
      if ( new_ ) proc->activate();
      else proc->deactivate();
    } );
}

void potion_of_empowered_exorcisms( special_effect_t& effect )
{
  struct empowered_exorcisms : public proc_spell_t
  {
    empowered_exorcisms( const special_effect_t& e )
      : proc_spell_t( "empowered_exorcisms", e.player, e.player->find_spell( 322015 ) )
    {
      // TODO: add interaction with shadowcore oil
    }

    // manually adjust for aoe reduction here instead of via action_t::reduced_aoe_damage as all targets receive reduced
    // damage, including the primary
    double composite_aoe_multiplier( const action_state_t* s ) const override
    {
      return proc_spell_t::composite_aoe_multiplier( s ) / std::sqrt( s->n_targets );
    }
  };

  auto potion            = new special_effect_t( effect.player );
  potion->type           = SPECIAL_EFFECT_EQUIP;
  potion->spell_id       = effect.spell_id;
  potion->cooldown_      = 0_ms;
  potion->execute_action = create_proc_action<empowered_exorcisms>( "potion_of_empowered_exorcisms", effect );
  effect.player->special_effects.push_back( potion );

  auto proc = new dbc_proc_callback_t( effect.player, *potion );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
    ->set_cooldown( 0_ms )
    ->set_chance( 1.0 )
    ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
      if ( new_ ) proc->activate();
      else proc->deactivate();
    } );
}

void potion_of_phantom_fire( special_effect_t& effect )
{
  struct phantom_fire_t : public proc_spell_t
  {
    phantom_fire_t( const special_effect_t& e )
      : proc_spell_t( "phantom_fire", e.player, e.player->find_spell( 321937 ) )
    {
      // TODO: add interaction with shadowcore oil
    }
  };

  auto potion            = new special_effect_t( effect.player );
  potion->type           = SPECIAL_EFFECT_EQUIP;
  potion->spell_id       = effect.spell_id;
  potion->cooldown_      = 0_ms;
  potion->execute_action = create_proc_action<phantom_fire_t>( "potion_of_phantom_fire", effect );
  effect.player->special_effects.push_back( potion );

  auto proc = new dbc_proc_callback_t( effect.player, *potion );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
    ->set_cooldown( 0_ms )
    ->set_chance( 1.0 )
    ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
      if ( new_ ) proc->activate();
      else proc->deactivate();
    } );
}

void embalmers_oil( special_effect_t& effect )
{

}

void shadowcore_oil( special_effect_t& effect )
{

}
}  // namespace consumables

namespace enchants
{
void celestial_guidance( special_effect_t& effect )
{
  if ( !effect.player->buffs.celestial_guidance )
  {
    effect.player->buffs.celestial_guidance =
        make_buff<SL_buff_t>( effect.player, "celestial_guidance", effect.player->find_spell( 324748 ) )
            ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
            ->add_invalidate( CACHE_AGILITY )
            ->add_invalidate( CACHE_INTELLECT )
            ->add_invalidate( CACHE_STRENGTH );
  }

  effect.custom_buff = effect.player->buffs.celestial_guidance;

  new dbc_proc_callback_t( effect.player, effect );
}

void lightless_force( special_effect_t& effect )
{
  effect.trigger_spell_id = 324184;
  effect.execute_action   = create_proc_action<SL_proc_spell_t>( "lightless_force", effect );
  // Spell projectile travels along a narrow beam and does not spread out with distance. Although data has a 40yd range,
  // the 1.5s duration seems to prevent the projectile from surviving beyond ~30yd.
  // TODO: confirm maximum effective range & radius
  effect.execute_action->aoe    = -1;
  effect.execute_action->radius = 5.0;
  effect.execute_action->range  = 30.0;

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

void register_hotfixes()
{
}

void register_special_effects()
{
    // Food
    unique_gear::register_special_effect( 308637, consumables::smothered_shank );

    // Potion
    unique_gear::register_special_effect( 307497, consumables::potion_of_deathly_fixation );
    unique_gear::register_special_effect( 307494, consumables::potion_of_empowered_exorcisms );
    unique_gear::register_special_effect( 307495, consumables::potion_of_phantom_fire );

    // Enchants
    unique_gear::register_special_effect( 324747, enchants::celestial_guidance );
    unique_gear::register_special_effect( 323932, enchants::lightless_force );
    unique_gear::register_special_effect( 324250, enchants::sinful_revelation );
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
}

}  // namespace shadowlands
}  // namespace unique_gear
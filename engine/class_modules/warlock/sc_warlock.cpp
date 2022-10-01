#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

#include "sc_warlock_pets.hpp"
#include "util/util.hpp"
#include "class_modules/apl/warlock.hpp"

#include <queue>

namespace warlock
{
// Spells
namespace actions
{
struct drain_life_t : public warlock_spell_t
{
  //Note: Soul Rot (Night Fae Covenant) turns Drain Life into a multi-target channeled spell. Nothing else in simc behaves this way and
  //we currently do not have core support for it. Applying this dot to the secondary targets should cover most of the behavior, although
  //it will be unable to handle the case where primary channel target dies (in-game, this appears to force-swap primary target to another
  //target currently affected by Drain Life if possible).
  struct drain_life_dot_t : public warlock_spell_t
  {
    drain_life_dot_t( warlock_t* p, util::string_view options_str) : warlock_spell_t( "Drain Life (AoE)", p, p->warlock_base.drain_life )
    {
      parse_options( options_str );
      dual = true;
      background = true;
      may_crit = false;

      //// SL - Legendary
      //dot_duration *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
      //base_tick_time *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
    }

    double bonus_ta( const action_state_t* s ) const override
    {
      double ta = warlock_spell_t::bonus_ta( s );

      // This code is currently unneeded, unless a bonus tick amount effect comes back into existence
      //if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
      //  ta = ta / ( 1.0 + p()->buffs.inevitable_demise->check_stack_value() );

      return ta;
    }

    double cost_per_tick( resource_e ) const override
    {
      return 0.0;
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
      {
        m *= 1.0 + p()->buffs.inevitable_demise->check_stack_value();
      }

      return m;
    }

    timespan_t composite_dot_duration(const action_state_t* s) const override
    {
        return dot_duration * ( tick_time( s ) / base_tick_time);
    }
  };

  drain_life_dot_t* aoe_dot;

  drain_life_t( warlock_t* p, util::string_view options_str ) : warlock_spell_t( "Drain Life", p, p->warlock_base.drain_life )
  {
    parse_options( options_str );

    aoe_dot = new drain_life_dot_t( p , options_str );
    add_child( aoe_dot );
    
    channeled    = true;
    hasted_ticks = false;
    may_crit     = false;

    //// SL - Legendary
    //dot_duration *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
    //base_tick_time *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
  }

  void execute() override
  {
    if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() > 0 )
    {
      if ( p()->buffs.drain_life->check() )
        p()->buffs.inevitable_demise->expire();
    }

    warlock_spell_t::execute();

    p()->buffs.drain_life->trigger();

    if ( p()->talents.soul_rot->ok() && p()->buffs.soul_rot->check() )
    {
      const auto& tl = target_list();
      
      for ( auto& t : tl )
      {
        //Don't apply aoe version to primary target
        if ( t == target )
          continue;

        auto data = td( t );
        if ( data->dots_soul_rot->is_ticking() )
        {
          aoe_dot->set_target( t );
          aoe_dot->execute();
        }
      }
    }
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    double ta = warlock_spell_t::bonus_ta( s );

    // This code is currently unneeded, unless a bonus tick amount effect comes back into existence
    //if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
    //  ta = ta / ( 1.0 + p()->buffs.inevitable_demise->check_stack_value() );

    return ta;
  }

  double cost_per_tick( resource_e r ) const override
  {
    if ( r == RESOURCE_MANA && p()->buffs.soul_rot->check() )
    {
      return 0.0;
    }

    auto c = warlock_spell_t::cost_per_tick( r );

    if ( r == RESOURCE_MANA && p()->buffs.inevitable_demise->check() )
      c *= 1.0 + p()->talents.inevitable_demise_buff->effectN( 3 ).percent() * p()->buffs.inevitable_demise->check();
    
    return c;
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
    {
      m *= 1.0 + p()->buffs.inevitable_demise->check_stack_value();
    }

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    p()->buffs.drain_life->expire();
    p()->buffs.inevitable_demise->expire();

    warlock_spell_t::last_tick( d );

    if ( p()->talents.soul_rot->ok() )
    {
      const auto& tl = target_list();

      for ( auto& t : tl )
      {
        auto data = td( t );
        if ( data->dots_drain_life_aoe->is_ticking() )
          data->dots_drain_life_aoe->cancel();
      }
    }
  }
};

// This is the damage spell which can be triggered on Corruption ticks for Harvester of Souls
// NOTE: As of 2022-09-23 on DF beta, spec aura is not currently affecting this spell
struct harvester_of_souls_t : public warlock_spell_t
{
  harvester_of_souls_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "Harvester of Souls", p, p->talents.harvester_of_souls_dmg )
  {
    parse_options( options_str );

    background = dual = true;
  }
};

struct corruption_t : public warlock_spell_t
{
  struct corruption_dot_t : public warlock_spell_t
  {
    harvester_of_souls_t* harvester_proc;

    corruption_dot_t( warlock_t* p ) : warlock_spell_t( "Corruption (DoT)", p, p->warlock_base.corruption->effectN( 1 ).trigger() ),
      harvester_proc( new harvester_of_souls_t( p, "" ) )
    {
      tick_zero = false;
      background = dual = true;

      add_child( harvester_proc );

      if ( p->talents.absolute_corruption->ok() )
      {
        dot_duration = sim->expected_iteration_time > 0_ms
                           ? 2 * sim->expected_iteration_time
                           : 2 * sim->max_time * ( 1.0 + sim->vary_combat_length );  // "infinite" duration
        base_td_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent(); // 2022-09-25: Only tick damage is affected
      }

      if ( p->talents.creeping_death->ok() )
        base_tick_time *= 1.0 + p->talents.creeping_death->effectN( 1 ).percent();
    }

    void tick( dot_t* d ) override
    {
      warlock_spell_t::tick( d );

      if ( result_is_hit( d->state->result ) )
      {
        if  ( p()->talents.nightfall->ok() )
        {
          // TOCHECK regularly.
          // Blizzard did not publicly release how nightfall was changed.
          // We determined this is the probable functionality copied from Agony by first confirming the
          // DR formula was the same and then confirming that you can get procs on 1st tick.
          // The procs also have a regularity that suggest it does not use a proc chance or rppm.
          // Last checked 09-28-2020.
          double increment_max = 0.13;

          double active_corruptions = p()->get_active_dots( internal_id );
          increment_max *= std::pow( active_corruptions, -2.0 / 3.0 );

          p()->corruption_accumulator += rng().range( 0.0, increment_max );

          if ( p()->corruption_accumulator >= 1 )
          {
            p()->procs.nightfall->occur();
            p()->buffs.nightfall->trigger();
            p()->corruption_accumulator -= 1.0;

          }
        }

        if ( p()->talents.harvester_of_souls.ok() && rng().roll( p()->talents.harvester_of_souls->effectN( 1 ).percent() ) )
        {
          harvester_proc->execute_on_target( d->state->target );
          p()->procs.harvester_of_souls->occur();
        }
      }
    }

    double composite_ta_multiplier(const action_state_t* s) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      // Ripped from affliction_spell_t - See what we can do to unify this under warlock_spell_t
      if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 1 ) ) )
      {
        m *= 1.0 + p()->cache.mastery_value();
      }

      if ( p()->talents.sacrolashs_dark_strike->ok() )
        m *= 1.0 + p()->talents.sacrolashs_dark_strike->effectN( 1 ).percent();

      return m;
    }
  };

  corruption_dot_t* periodic;

  corruption_t( warlock_t* p, util::string_view options_str, bool seed_action )
    : warlock_spell_t( "Corruption", p, p->warlock_base.corruption ),
    periodic( new corruption_dot_t( p ) )
  {
    parse_options( options_str );
    tick_zero                  = false;
    affected_by_woc            = true;

    // DoT information is in trigger spell (146739)
    impact_action = periodic;

    spell_power_mod.direct = 0; // By default, Corruption does not deal instant damage

    if ( p->talents.xavian_teachings.ok() && !seed_action )
    {
      spell_power_mod.direct = data().effectN( 3 ).sp_coeff(); // Talent uses this effect in base spell for damage
      base_execute_time *= 1.0 + p->talents.xavian_teachings->effectN( 1 ).percent();
    }
  }

  void impact( action_state_t* s ) override
  {
    auto dot_data = td( s->target )->dots_corruption;

    bool pi_trigger = p()->talents.pandemic_invocation.ok() && dot_data->is_ticking()
      && dot_data->remains() < timespan_t::from_millis( p()->talents.pandemic_invocation->effectN( 1 ).base_value() );

    warlock_spell_t::impact( s );

    if ( pi_trigger )
      p()->proc_actions.pandemic_invocation_proc->execute_on_target( s->target );
  }

  // direct action multiplier
  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double pm = warlock_spell_t::composite_da_multiplier( s );

    if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 2 ) ) )
    {
      pm *= 1.0 + p()->cache.mastery_value();
    }
    return pm;
  }
};

// Needs to be here due to Corruption being a shared spell
struct seed_of_corruption_t : public warlock_spell_t
{
  struct seed_of_corruption_aoe_t : public warlock_spell_t
  {
    corruption_t* corruption;

    seed_of_corruption_aoe_t( warlock_t* p )
      : warlock_spell_t( "Seed of Corruption (AoE)", p, p->talents.seed_of_corruption_aoe ),
        corruption( new corruption_t( p, "", true ) )
    {
      aoe                              = -1;
      background                       = true;
      base_costs[ RESOURCE_MANA ]      = 0;

      corruption->background                  = true;
      corruption->dual                        = true;
      corruption->base_costs[ RESOURCE_MANA ] = 0;
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        auto tdata = this->td( s->target );
        if ( tdata->dots_seed_of_corruption->is_ticking() && tdata->soc_threshold > 0 )
        {
          tdata->soc_threshold = 0;
          tdata->dots_seed_of_corruption->cancel();
        }

        // 2022-09-24 Agonizing Corruption does not apply Agony, only increments existing ones
        if ( p()->talents.agonizing_corruption.ok() && tdata->dots_agony->is_ticking() )
        {
          tdata->dots_agony->increment( (int)p()->talents.agonizing_corruption->effectN( 1 ).base_value() );
        }

        corruption->set_target( s->target );
        corruption->execute();
      }
    }

    // Copied from affliction_spell_t since this needs to be a warlock_spell_t
    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double pm = warlock_spell_t::composite_da_multiplier( s );

      if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 2 ) ) )
      {
        pm *= 1.0 + p()->cache.mastery_value();
      }
      return pm;
    }
  };

  seed_of_corruption_aoe_t* explosion;

  seed_of_corruption_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "seed_of_corruption", p, p->talents.seed_of_corruption ),
      explosion( new seed_of_corruption_aoe_t( p ) )
  {
    parse_options( options_str );
    may_crit       = false;
    tick_zero      = false;
    base_tick_time = dot_duration;
    hasted_ticks   = false;

    add_child( explosion );

    if ( p->talents.sow_the_seeds->ok() )
      aoe = 1 + as<int>( p->talents.sow_the_seeds->effectN( 1 ).base_value() );
  }

  void init() override
  {
    warlock_spell_t::init();
    snapshot_flags |= STATE_SP;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    warlock_spell_t::available_targets( tl );

    // Targeting behavior appears to be as follows:
    // 1. If any targets have no current seed (in flight or ticking), they are valid
    // 2. With Sow the Seeds, if at least one target is valid, it will only hit valid targets
    // 3. If no targets are valid according to the above, all targets are instead valid (will refresh DoT on existing target(s) instead)
    bool valid_target = false;
    for ( auto t : tl )
    {
      if ( !( td( t )->dots_seed_of_corruption->is_ticking() || has_travel_events_for( t ) ) )
      {
        valid_target = true;
        break;
      }
    }

    if ( valid_target )
      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* target ){ return ( p()->get_target_data( target )->dots_seed_of_corruption->is_ticking() || has_travel_events_for( target ) ); } ), tl.end() );

    return tl.size();
  }

  void impact( action_state_t* s ) override
  {
    // TOCHECK: Does the threshold reset if the duration is refreshed before explosion?
    if ( result_is_hit( s->result ) )
    {
      td( s->target )->soc_threshold = s->composite_spell_power() * p()->talents.seed_of_corruption->effectN( 1 ).percent();
    }

    warlock_spell_t::impact( s );
  }

  // If Seed of Corruption is refreshed on a target, it will extend the duration
  // but still explode at the original time, wiping the "DoT". tick() should be used instead
  // of last_tick() to model this appropriately.
  void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( d->remains() > 0_ms )
      d->cancel();
  }

  void last_tick( dot_t* d ) override
  {
    explosion->set_target( d->target );
    explosion->schedule_execute();

    warlock_spell_t::last_tick( d );
  }
};

struct shadow_bolt_t : public warlock_spell_t
{
  shadow_bolt_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "Shadow Bolt", p, p->talents.drain_soul.ok() ? spell_data_t::not_found() : p->warlock_base.shadow_bolt )
  {
    parse_options( options_str );

    if ( p->specialization() == WARLOCK_DEMONOLOGY )
    {
      energize_type     = action_energize::ON_CAST;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount   = 1;
    }
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.nightfall->check() )
    {
      return 0_ms;
    }

    return warlock_spell_t::execute_time();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( time_to_execute == 0_ms )
      p()->buffs.nightfall->decrement();
    //
    //if ( p()->talents.demonic_calling->ok() )
    //  p()->buffs.demonic_calling->trigger();

    //if ( p()->legendary.balespiders_burning_core->ok() )
    //  p()->buffs.balespiders_burning_core->trigger();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->talents.shadow_embrace->ok() )
        td( s->target )->debuffs_shadow_embrace->trigger();

    //  if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B4 ) )
    //  {        
    //    auto tdata = this->td( s->target );
    //    // TOFIX - As of 2022-02-03 PTR, the bonus appears to still be only checking that *any* target has these dots. May need to implement this behavior.
    //    bool tierDotsActive = tdata->dots_agony->is_ticking()
    //                       && tdata->dots_corruption->is_ticking()
    //                       && tdata->dots_unstable_affliction->is_ticking();

    //    if ( tierDotsActive && rng().roll( p()->sets->set(WARLOCK_AFFLICTION, T28, B4 )->effectN( 1 ).percent() ) )
    //    {
    //      p()->procs.calamitous_crescendo->occur();
    //      p()->buffs.calamitous_crescendo->trigger();
    //    }
    //  }
      if ( p()->talents.tormented_crescendo.ok() )
      {
        if ( p()->crescendo_check( p() ) && rng().roll( p()->talents.tormented_crescendo->effectN( 1 ).percent() ) )
        {
          p()->procs.tormented_crescendo->occur();
          p()->buffs.tormented_crescendo->trigger();
        }
      }
    }
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( time_to_execute == 0_ms && p()->buffs.nightfall->check() )
      m *= 1.0 + p()->talents.nightfall_buff->effectN( 2 ).percent();

    //if ( p()->talents.sacrificed_souls->ok() )
    //{
    //  m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_pets;
    //}

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    if ( p()->talents.withering_bolt.ok() )
      m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( (int)p()->talents.withering_bolt->effectN( 2 ).base_value(), p()->get_target_data( t )->count_affliction_dots() );

    //auto td = this->td( t );

    //if ( td->debuffs_from_the_shadows->check() && data().affected_by( td->debuffs_from_the_shadows->data().effectN( 1 ) ) )
    //{
    //  m *= 1.0 + td->debuffs_from_the_shadows->check_value();
    //}

    return m;
  }
};

struct soul_rot_t : public warlock_spell_t
{
  soul_rot_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "soul_rot", p, p->talents.soul_rot )

  {
    parse_options( options_str );
    aoe = 1 + as<int>( p->talents.soul_rot->effectN( 3 ).base_value() );
  }

  void execute() override
  {
    warlock_spell_t::execute();

    p()->buffs.soul_rot->trigger();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    //if ( p()->legendary.decaying_soul_satchel.ok() )
    //{
    //  p()->buffs.decaying_soul_satchel_haste->trigger();
    //  p()->buffs.decaying_soul_satchel_crit->trigger();
    //}
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = warlock_spell_t::composite_ta_multiplier( s );

    if ( s->chain_target == 0 )
    {
      m *= 1.0 + p()->talents.soul_rot->effectN( 4 ).base_value() / 10; //Primary takes increased damage
    }

    return m;
  }
};

struct soul_flame_t : public warlock_spell_t
{
  soul_flame_t( warlock_t* p ) : warlock_spell_t( "soul_flame", p, p->talents.soul_flame_proc )
  {
    background = true;
    aoe = -1;

    base_dd_multiplier = 1.0 + p->talents.soul_flame->effectN( 2 ).percent();
  }
};

// TOCHECK: As of 2022-09-30, talent values are pointing to the incorrect effects
// Everything should be pointed to the correct effect data but values may be garbage until this is fixed by Blizzard
struct pandemic_invocation_t : public warlock_spell_t
{
  pandemic_invocation_t( warlock_t* p ) : warlock_spell_t( "pandemic_invocation", p, p->talents.pandemic_invocation_proc )
  {
    background = true;

    base_dd_multiplier *= 1.0 + p->talents.pandemic_invocation->effectN( 3 ).percent();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( rng().roll( p()->talents.pandemic_invocation->effectN( 2 ).percent() / 100.0 ) )
    {
      p()->resource_gain( RESOURCE_SOUL_SHARD, 1, p()->gains.pandemic_invocation );
      p()->procs.pandemic_invocation_shard->occur();
    }
  }
};

struct grimoire_of_sacrifice_t : public warlock_spell_t
{
  grimoire_of_sacrifice_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "grimoire_of_sacrifice", p, p->talents.grimoire_of_sacrifice )
  {
    parse_options( options_str );
    harmful               = false;
    ignore_false_positive = true;
  }

  bool ready() override
  {
    if ( !p()->warlock_pet_list.active )
      return false;

    return warlock_spell_t::ready();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p()->warlock_pet_list.active )
    {
      p()->warlock_pet_list.active->dismiss();
      p()->warlock_pet_list.active = nullptr;
      p()->buffs.grimoire_of_sacrifice->trigger();
    }
  }
};

//Catchall action to trigger pet interrupt abilities via main APL.
//Using this should ensure that interrupt callback effects (Sephuz etc) proc correctly for the warlock.
struct interrupt_t : public spell_t
{
  interrupt_t( util::string_view n, warlock_t* p, util::string_view options_str ) :
    spell_t( n, p )
  {
    parse_options( options_str );
    callbacks = true;
    dual = usable_while_casting = true;
    may_miss = may_block = may_crit = false;
    ignore_false_positive = is_interrupt = true;
    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    auto* w = debug_cast<warlock_t*>( player );

    auto pet = w->warlock_pet_list.active;

    switch( pet->pet_type )
    {
      case PET_FELGUARD:
      case PET_FELHUNTER:
        pet->special_action->set_target( target );
        pet->special_action->execute();
        break;
      default:
        break;
    }

    spell_t::execute();
  }

  bool ready() override
  {
    auto* w = debug_cast<warlock_t*>( player );

    if ( !w->warlock_pet_list.active || w->warlock_pet_list.active->is_sleeping() )
      return false;

    auto pet = w->warlock_pet_list.active;

    switch( pet->pet_type )
    {
      case PET_FELGUARD:
      case PET_FELHUNTER:
        if ( !pet->special_action || !pet->special_action->cooldown->up() || !pet->special_action->ready() )
          return false;

        return spell_t::ready();
      default:
        return false;
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return spell_t::target_ready( candidate_target );
  }
};
}  // namespace actions

warlock_td_t::warlock_td_t( player_t* target, warlock_t& p )
  : actor_target_data_t( target, &p ), soc_threshold( 0.0 ), warlock( p )
{
  dots_drain_life = target->get_dot( "drain_life", &p );
  dots_drain_life_aoe = target->get_dot( "drain_life_aoe", &p );
  dots_soul_rot = target->get_dot( "soul_rot", &p );

  // Aff
  dots_corruption          = target->get_dot( "corruption", &p );
  dots_agony               = target->get_dot( "agony", &p );
  dots_drain_soul          = target->get_dot( "drain_soul", &p );
  dots_phantom_singularity = target->get_dot( "phantom_singularity", &p );
  dots_siphon_life         = target->get_dot( "siphon_life", &p );
  dots_seed_of_corruption  = target->get_dot( "seed_of_corruption", &p );
  dots_unstable_affliction = target->get_dot( "unstable_affliction", &p );
  dots_vile_taint          = target->get_dot( "vile_taint", &p );

  debuffs_haunt = make_buff( *this, "haunt", p.talents.haunt )
                      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                      ->set_default_value_from_effect( 2 );

  debuffs_shadow_embrace = make_buff( *this, "shadow_embrace", p.talents.shadow_embrace_debuff )
                               ->set_default_value( p.talents.shadow_embrace->effectN( 1 ).percent() );

  debuffs_malefic_affliction = make_buff( *this, "malefic_affliction", p.talents.malefic_affliction_debuff )
                                   ->set_default_value( p.talents.malefic_affliction->effectN( 1 ).percent() );

  // Destro
  dots_immolate          = target->get_dot( "immolate", &p );

  debuffs_eradication = make_buff( *this, "eradication", source->find_spell( 196414 ) )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                            ->set_default_value_from_effect( 1 );
  debuffs_roaring_blaze = make_buff( *this, "roaring_blaze", source->find_spell( 265931 ) );
  debuffs_shadowburn    = make_buff( *this, "shadowburn", source->find_spell( 17877 ) )
                              ->set_default_value( source->find_spell( 245731 )->effectN( 1 ).base_value() );
  debuffs_havoc         = make_buff( *this, "havoc", source->find_specialization_spell( 80240 ) )
                      ->set_duration( source->find_specialization_spell( 80240 )->duration() +
                                      source->find_specialization_spell( 335174 )->effectN( 1 ).time_value() )
                      ->set_cooldown( 0_ms )
                      ->set_stack_change_callback( [ &p ]( buff_t* b, int, int cur ) {
                        if ( cur == 0 )
                        {
                          p.havoc_target = nullptr;
                        }
                        else
                        {
                          if ( p.havoc_target && p.havoc_target != b->player )
                            p.get_target_data( p.havoc_target )->debuffs_havoc->expire();
                          p.havoc_target = b->player;
                        }

                        range::for_each( p.havoc_spells, []( action_t* a ) { a->target_cache.is_valid = false; } );
                      } );

  // Demo
  dots_doom         = target->get_dot( "doom", &p );

  debuffs_from_the_shadows = make_buff( *this, "from_the_shadows", source->find_spell( 270569 ) )
                                 ->set_default_value_from_effect( 1 );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void warlock_td_t::target_demise()
{
  if ( !( target->is_enemy() ) )
  {
    return;
  }

  if ( warlock.talents.unstable_affliction->ok() )
  {
    if ( dots_unstable_affliction->is_ticking() )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} gains {} shard(s) from Unstable Affliction.", target->name(),
                              warlock.name(), warlock.talents.unstable_affliction_2->effectN( 1 ).base_value() );

      warlock.resource_gain( RESOURCE_SOUL_SHARD, warlock.talents.unstable_affliction_2->effectN( 1 ).base_value(), warlock.gains.unstable_affliction_refund );
    }
  }
  if ( dots_drain_soul->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Drain Soul.", target->name(),
                            warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul );
  }

  if ( debuffs_haunt->check() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} reset Haunt's cooldown.", target->name(), warlock.name() );

    warlock.cooldowns.haunt->reset( true );
  }

  if ( debuffs_shadowburn->check() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} refunds one charge of Shadowburn.", target->name(), warlock.name() );
      
    warlock.cooldowns.shadowburn->reset( true );
   
    warlock.sim->print_log( "Player {} demised. Warlock {} gains 1 shard from Shadowburn.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, debuffs_shadowburn->check_value() / 10,
                           warlock.gains.shadowburn_refund );
  }

  if ( dots_agony->is_ticking() && warlock.talents.wrath_of_consumption.ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Wrath of Consumption from Agony.", target->name(), warlock.name() );

    warlock.buffs.wrath_of_consumption->trigger();
  }

  if ( dots_corruption->is_ticking() && warlock.talents.wrath_of_consumption.ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Wrath of Consumption from Corruption.", target->name(), warlock.name() );

    warlock.buffs.wrath_of_consumption->trigger();
  }

  if ( warlock.talents.soul_flame.ok() && !warlock.proc_actions.soul_flame_proc->target_list().empty() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Soul Flame on all targets in range.", target->name(), warlock.name() );

    warlock.proc_actions.soul_flame_proc->execute();
  }
}

static void accumulate_seed_of_corruption( warlock_td_t* td, double amount )
{
  td->soc_threshold -= amount;

  if ( td->soc_threshold <= 0 )
  {
    td->dots_seed_of_corruption->cancel();
  }
  else
  {
    if ( td->source->sim->log )
      td->source->sim->out_log.print( "Remaining damage to explode Seed on {} is {}", td->target->name_str, td->soc_threshold );
  }
}

int warlock_td_t::count_affliction_dots()
{
  int count = 0;

  if ( dots_agony->is_ticking() )
    count++;

  if ( dots_corruption->is_ticking() )
    count++;

  if ( dots_seed_of_corruption->is_ticking() )
    count++;

  if ( dots_unstable_affliction->is_ticking() )
    count++;

  if ( dots_vile_taint->is_ticking() )
    count++;

  if ( dots_phantom_singularity->is_ticking() )
    count++;

  if ( dots_soul_rot->is_ticking() )
    count++;

  if ( dots_siphon_life->is_ticking() )
    count++;

  return count;
}


warlock_t::warlock_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    ua_target( nullptr ),
    havoc_spells(),
    agony_accumulator( 0.0 ),
    corruption_accumulator( 0.0 ),
    active_pets( 0 ),
    warlock_pet_list( this ),
    active(),
    talents(),
    legendary(),
    conduit(),
    covenant(),
    cooldowns(),
    spec(),
    buffs(),
    gains(),
    procs(),
    initial_soul_shards( 3 ),
    default_pet()
{
  cooldowns.haunt               = get_cooldown( "haunt" );
  cooldowns.phantom_singularity = get_cooldown( "phantom_singularity" );
  cooldowns.darkglare           = get_cooldown( "summon_darkglare" );
  cooldowns.demonic_tyrant      = get_cooldown( "summon_demonic_tyrant" );
  cooldowns.infernal            = get_cooldown( "summon_infernal" );
  cooldowns.shadowburn          = get_cooldown( "shadowburn" );

  resource_regeneration             = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ]       = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;
}

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( warlock_base.master_demonologist->ok() )
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;

    default:
      break;
  }
}

double warlock_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  const warlock_td_t* td = get_target_data( target );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( td->debuffs_haunt->check() )
      m *= 1.0 + td->debuffs_haunt->check_value();

    if ( talents.shadow_embrace->ok() )
      m *= 1.0 + td->debuffs_shadow_embrace->check_stack_value();

  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( td->debuffs_eradication->check() )
      m *= 1.0 + td->debuffs_eradication->check_value();
  }

  return m;
}

double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

double warlock_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    m *= 1.0 + warlock_base.destruction_warlock->effectN( 3 ).percent();
  }
  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    m *= 1.0 + warlock_base.demonology_warlock->effectN( 3 ).percent();
    m *= 1.0 + cache.mastery_value();

    if ( buffs.demonic_power->check() )
      m *= 1.0 + buffs.demonic_power->default_value;
  }
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    m *= 1.0 + warlock_base.affliction_warlock->effectN( 3 ).percent();
  }
  return m;
}

double warlock_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  const warlock_td_t* td = get_target_data( target );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( td->debuffs_haunt->check() )
    {
      m *= 1.0 + td->debuffs_haunt->data().effectN( guardian ? 4 : 3 ).percent();
    }

    if ( talents.shadow_embrace->ok() )
    {
      m *= 1.0 + td->debuffs_shadow_embrace->check_stack_value(); // Talent spell sets default value according to rank
    }
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( td->debuffs_eradication->check() )
    {
      m *= 1.0 + td->debuffs_eradication->data().effectN( guardian ? 3 : 2 ).percent();
    }
  }

  return m;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.reverse_entropy->check() )
    h *= 1.0 / ( 1.0 + buffs.reverse_entropy->check_value() );

  return h;
}

double warlock_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.reverse_entropy->check() )
    h *= 1.0 / ( 1.0 + buffs.reverse_entropy->check_value() );

  return h;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();
  return m;
}

double warlock_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );
  return m;
}

double warlock_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );
  return reg;
}

double warlock_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );
  return m;
}

//Note: Level is checked to be >=27 by the function calling this. This is technically wrong for warlocks due to
//a missing level requirement in data, but correct generally.
double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return warlock_base.nethermancy->effectN( 1 ).percent();

  return 0.0;
}

action_t* warlock_t::create_action_warlock( util::string_view action_name, util::string_view options_str )
{
  using namespace actions;

  if ( ( action_name == "summon_pet" ) && default_pet.empty() )
  {
    sim->errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return nullptr;
  }

  // Pets
  if ( action_name == "summon_felhunter" )
    return new summon_main_pet_t( "felhunter", this );
  if ( action_name == "summon_felguard" )
    return new summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_sayaad" )
    return new summon_main_pet_t( "sayaad", this, 366222 );
  if ( action_name == "summon_succubus" )
    return new summon_main_pet_t( "succubus", this, 366222 );
  if ( action_name  == "summon_incubus" )
    return new summon_main_pet_t( "incubus", this, 366222 );
  if ( action_name == "summon_voidwalker" )
    return new summon_main_pet_t( "voidwalker", this );
  if ( action_name == "summon_imp" )
    return new summon_main_pet_t( "imp", this );
  if ( action_name == "summon_pet" )
  {
    if ( default_pet == "sayaad" || default_pet == "succubus" || default_pet == "incubus" )
      return new summon_main_pet_t( default_pet, this, 366222 );

    return new summon_main_pet_t( default_pet, this );
  }

  // Base Spells
  if ( action_name == "drain_life" )
    return new drain_life_t( this, options_str );
  if ( action_name == "corruption" && specialization() != WARLOCK_DESTRUCTION )
    return new corruption_t( this, options_str, false );
  if ( action_name == "shadow_bolt" && specialization() != WARLOCK_DESTRUCTION )
    return new shadow_bolt_t( this, options_str );
  if ( action_name == "grimoire_of_sacrifice" )
    return new grimoire_of_sacrifice_t( this, options_str );  // aff and destro
  if ( action_name == "soul_rot" )
    return new soul_rot_t( this, options_str );
  if ( action_name == "interrupt" )
    return new interrupt_t( action_name, this, options_str );
  if ( action_name == "seed_of_corruption" )
    return new seed_of_corruption_t( this, options_str );

  return nullptr;
}

void warlock_t::create_actions()
{
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( talents.soul_flame.ok() )
      proc_actions.soul_flame_proc = new warlock::actions::soul_flame_t( this );

    if ( talents.pandemic_invocation.ok() )
      proc_actions.pandemic_invocation_proc = new warlock::actions::pandemic_invocation_t( this );
  }
  player_t::create_actions();
}

action_t* warlock_t::create_action( util::string_view action_name, util::string_view options_str )
{
  // create_action_[specialization] should return a more specialized action if needed (ie Corruption in Affliction)
  // If no alternate action for the given spec is found, check actions in sc_warlock

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( action_t* aff_action = create_action_affliction( action_name, options_str ) )
      return aff_action;
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    if ( action_t* demo_action = create_action_demonology( action_name, options_str ) )
      return demo_action;
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( action_t* destro_action = create_action_destruction( action_name, options_str ) )
      return destro_action;
  }

  if ( action_t* generic_action = create_action_warlock( action_name, options_str ) )
    return generic_action;

  return player_t::create_action( action_name, options_str );
}

pet_t* warlock_t::create_pet( util::string_view pet_name, util::string_view pet_type )
{
  pet_t* p = find_pet( pet_name );
  if ( p )
    return p;
  using namespace pets;

  pet_t* summon_pet = create_main_pet( pet_name, pet_type );
  if ( summon_pet )
    return summon_pet;

  return nullptr;
}

void warlock_t::create_pets()
{
  create_all_pets();

  for ( auto& pet : pet_name_list )
  {
    create_pet( pet );
  }
}

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  create_buffs_affliction();
  create_buffs_demonology();
  create_buffs_destruction();

  buffs.grimoire_of_sacrifice = make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice_buff )
                                    ->set_chance( 1.0 );

  buffs.soul_rot = make_buff(this, "soul_rot", talents.soul_rot)
                       ->set_cooldown( 0_ms );

  // Legendaries
  buffs.wrath_of_consumption = make_buff( this, "wrath_of_consumption", talents.wrath_of_consumption_buff )
                               ->set_default_value( talents.wrath_of_consumption->effectN( 2 ).percent() );

  buffs.demonic_synergy = make_buff( this, "demonic_synergy", find_spell( 337060 ) )
                              ->set_default_value( legendary.relic_of_demonic_synergy->effectN( 1 ).percent() * ( this->specialization() == WARLOCK_DEMONOLOGY ? 1.5 : 1.0 ) );

  buffs.decaying_soul_satchel_haste = make_buff( this, "decaying_soul_satchel_haste", find_spell( 356369 ) )
                                          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                          ->set_default_value( find_spell( 356369 )->effectN( 1 ).percent() );

  buffs.decaying_soul_satchel_crit = make_buff( this, "decaying_soul_satchel_crit", find_spell( 356369 ) )
                                         ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                         ->set_default_value( find_spell( 356369 )->effectN( 2 ).percent() );
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  // Automatic requirement checking and relevant .inc file (/engine/dbc/generated/):
  // find_class_spell - active_spells.inc
  // find_specialization_spell - specialization_spells.inc
  // find_mastery_spell - mastery_spells.inc
  // find_talent_spell - ??
  //
  // If there is no need to check whether a spell is known by the actor, can fall back on find_spell

  // General
  warlock_base.nethermancy = find_spell( 86091 );
  warlock_base.drain_life = find_class_spell( "Drain Life" ); // Should be ID 234153
  warlock_base.corruption = find_class_spell( "Corruption" ); // Should be ID 172, DoT info is in Effect 1's trigger (146739)
  warlock_base.shadow_bolt = find_class_spell( "Shadow Bolt" ); // Should be ID 686, same for both Affliction and Demonology

  // Affliction
  warlock_base.agony = find_class_spell( "Agony" ); // Should be ID 980
  warlock_base.agony_2 = find_spell( 231792 ); // Rank 2, +4 to max stacks
  warlock_base.potent_afflictions = find_mastery_spell( WARLOCK_AFFLICTION );
  warlock_base.affliction_warlock = find_specialization_spell( "Affliction Warlock" );

  // Demonology
  warlock_base.hand_of_guldan = find_class_spell( "Hand of Gul'dan" ); // Should be ID 105174
  warlock_base.hog_impact = find_spell( 86040 ); // Contains impact damage data
  warlock_base.wild_imp = find_spell( 104317 ); // Contains pet summoning information
  warlock_base.demonic_core = find_specialization_spell( "Demonic Core" ); // Passive. Should be ID 267102
  warlock_base.demonic_core_buff = find_spell( 264173 ); // Buff data
  warlock_base.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );
  warlock_base.demonology_warlock = find_specialization_spell( "Demonology Warlock" );

  // Destruction
  warlock_base.immolate = find_class_spell( "Immolate" ); // Should be ID 348, contains direct damage and cast data
  warlock_base.immolate_dot = find_spell( 157736 ); // DoT data
  warlock_base.incinerate = find_class_spell( "Incinerate" ); // Should be ID 29722
  warlock_base.incinerate_energize = find_spell( 244670 ); // Used for resource gain information
  warlock_base.chaotic_energies = find_mastery_spell( WARLOCK_DESTRUCTION );
  warlock_base.destruction_warlock = find_specialization_spell( "Destruction Warlock" );

  // DF - REMOVE THESE?
  warlock_t::init_spells_affliction();
  warlock_t::init_spells_demonology();
  warlock_t::init_spells_destruction();

  // Talents
  talents.seed_of_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Seed of Corruption" );
  talents.seed_of_corruption_aoe = find_spell( 27285 ); // Explosion damage

  talents.grimoire_of_sacrifice = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire of Sacrifice" ); // Aff/Destro only. Should be ID 108503
  talents.grimoire_of_sacrifice_buff = find_spell( 196099 ); // Buff data and RPPM
  talents.grimoire_of_sacrifice_proc = find_spell( 196100 ); // Damage data

  talents.soul_conduit              = find_talent_spell( "Soul Conduit" );

  // Legendaries
  legendary.claw_of_endereth                     = find_runeforge_legendary( "Claw of Endereth" );
  legendary.relic_of_demonic_synergy             = find_runeforge_legendary( "Relic of Demonic Synergy" );
  legendary.wilfreds_sigil_of_superior_summoning = find_runeforge_legendary( "Wilfred's Sigil of Superior Summoning" );
  // Sacrolash is the only spec-specific legendary that can be used by other specs.
  legendary.sacrolashs_dark_strike = find_runeforge_legendary( "Sacrolash's Dark Strike" );
  //Wrath is implemented here to catch any potential cross-spec periodic effects
  legendary.wrath_of_consumption = find_runeforge_legendary("Wrath of Consumption");

  legendary.decaying_soul_satchel = find_runeforge_legendary( "Decaying Soul Satchel" );

  // Covenant Abilities
  covenant.soul_rot              = find_covenant_spell( "Soul Rot" );               // Night Fae
}

void warlock_t::init_rng()
{
  if ( specialization() == WARLOCK_AFFLICTION )
    init_rng_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_rng_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_rng_destruction();

  player_t::init_rng();
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_gains_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_gains_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_gains_destruction();

  gains.miss_refund  = get_gain( "miss_refund" );
  gains.shadow_bolt  = get_gain( "shadow_bolt" );
  gains.soul_conduit = get_gain( "soul_conduit" );
}

void warlock_t::init_procs()
{
  player_t::init_procs();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_procs_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_procs_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_procs_destruction();

  procs.one_shard_hog   = get_proc( "one_shard_hog" );
  procs.two_shard_hog   = get_proc( "two_shard_hog" );
  procs.three_shard_hog = get_proc( "three_shard_hog" );
  procs.portal_summon   = get_proc( "portal_summon" );
  procs.demonic_calling = get_proc( "demonic_calling" );
  procs.soul_conduit    = get_proc( "soul_conduit" );
  procs.carnivorous_stalkers = get_proc( "carnivorous_stalkers" );
  procs.horned_nightmare = get_proc( "horned_nightmare" );
  procs.ritual_of_ruin       = get_proc( "ritual_of_ruin" );
  procs.avatar_of_destruction = get_proc( "avatar_of_destruction" );
}

void warlock_t::init_base_stats()
{
  if ( base.distance < 1.0 )
    base.distance = 40.0;

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_SOUL_SHARD ] = 5;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      default_pet = "imp";
    else if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else if ( specialization() == WARLOCK_DESTRUCTION )
      default_pet = "imp";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() == WARLOCK_DEMONOLOGY )
    scaling->enable( STAT_STAMINA );
}

void warlock_t::init_assessors()
{
  player_t::init_assessors();

  auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ) {
    if ( get_target_data( s->target )->dots_seed_of_corruption->is_ticking() )
    {
      accumulate_seed_of_corruption( get_target_data( s->target ), s->result_total );
    }

    return assessor::CONTINUE;
  };

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );

  for ( auto pet : pet_list )
  {
    pet->assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
  }
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
    case WARLOCK_AFFLICTION:
      warlock_apl::affliction( this );
      break;
    case WARLOCK_DEMONOLOGY:
      warlock_apl::demonology( this );
      break;
    case WARLOCK_DESTRUCTION:
      warlock_apl::destruction( this );
      break;
    default:
      break;
    }

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_SOUL_SHARD ] = initial_soul_shards;
}

void warlock_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( talents.grimoire_of_sacrifice->ok() )
  {
    auto const sac_effect = new special_effect_t( this );
    sac_effect->name_str = "grimoire_of_sacrifice_effect";
    sac_effect->spell_id = talents.grimoire_of_sacrifice_buff->id();
    sac_effect->execute_action = new warlock::actions::grimoire_of_sacrifice_damage_t( this );
    special_effects.push_back( sac_effect );

    auto cb = new dbc_proc_callback_t( this, *sac_effect );

    cb->initialize();
    cb->deactivate();

    buffs.grimoire_of_sacrifice->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ){
        if ( new_ == 1 ) cb->activate();
        else cb->deactivate();
      } );
  }

  if ( legendary.relic_of_demonic_synergy->ok() )
  {
    auto const syn_effect = new special_effect_t( this );
    syn_effect->name_str = "demonic_synergy_effect";
    syn_effect->spell_id = 337057;
    special_effects.push_back( syn_effect );

    auto cb = new warlock::actions::demonic_synergy_callback_t( this, *syn_effect );

    cb->initialize();
  }
}

void warlock_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == WARLOCK_DEMONOLOGY && buffs.inner_demons && talents.inner_demons->ok() )
    buffs.inner_demons->trigger();
}

void warlock_t::reset()
{
  player_t::reset();

  range::for_each( sim->target_list, [ this ]( const player_t* t ) {
    if ( auto td = target_data[ t ] )
    {
      td->reset();
    }

    range::for_each( t->pet_list, [ this ]( const player_t* add ) {
      if ( auto td = target_data[ add ] )
      {
        td->reset();
      }
    } );
  } );

  warlock_pet_list.active            = nullptr;
  havoc_target                       = nullptr;
  ua_target                          = nullptr;
  agony_accumulator                  = rng().range( 0.0, 0.99 );
  corruption_accumulator             = rng().range( 0.0, 0.99 ); // TOCHECK - Unsure if it procs on application
  wild_imp_spawns.clear();
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
}

// Used to determine how many Wild Imps are waiting to be spawned from Hand of Guldan
int warlock_t::get_spawning_imp_count()
{
  return as<int>( wild_imp_spawns.size() );
}

// Function for returning the time until a certain number of imps will have spawned
// In the case where count is equal to or greater than number of incoming imps, time to last imp is returned
// Otherwise, time to the Nth imp spawn will be returned
// All other cases will return 0. A negative (or zero) count value will behave as "all"
timespan_t warlock_t::time_to_imps( int count )
{
  timespan_t max = 0_ms;
  if ( count >= as<int>( wild_imp_spawns.size() ) || count <= 0 )
  {
    for ( auto ev : wild_imp_spawns )
    {
      timespan_t ex = debug_cast<actions::imp_delay_event_t*>( ev )->expected_time();
      if ( ex > max )
        max = ex;
    }
    return max;
  }
  else
  {
    std::priority_queue<timespan_t> shortest;
    for ( auto ev : wild_imp_spawns )
    {
      timespan_t ex = debug_cast<actions::imp_delay_event_t*>( ev )->expected_time();
      if ( as<int>( shortest.size() ) >= count && ex < shortest.top() )
      {
        shortest.pop();
        shortest.push( ex );
      }
      else if ( as<int>( shortest.size() ) < count )
      {
        shortest.push( ex );
      }
    }

    if ( !shortest.empty() )
    {
      return shortest.top();
    }
    else
    {
      return 0_ms;
    }
  }
}

void warlock_t::darkglare_extension_helper( warlock_t* p, timespan_t darkglare_extension )
{
  for ( const auto target : p->sim->target_non_sleeping_list )
  {
    warlock_td_t* td = p->get_target_data( target );
    if ( !td )
    {
      continue;
    }
    td->dots_agony->adjust_duration( darkglare_extension );
    td->dots_corruption->adjust_duration( darkglare_extension );
    td->dots_siphon_life->adjust_duration( darkglare_extension );
    td->dots_phantom_singularity->adjust_duration( darkglare_extension );
    td->dots_vile_taint->adjust_duration( darkglare_extension );
    td->dots_unstable_affliction->adjust_duration( darkglare_extension );
    td->dots_soul_rot->adjust_duration( darkglare_extension );
  }
}

bool warlock_t::crescendo_check( warlock_t* p )
{
  bool agony = false;
  bool corruption = false;
  for ( const auto target : p->sim->target_non_sleeping_list )
  {
    warlock_td_t* td = p->get_target_data( target );
    if ( !td )
      continue;

    agony = agony || td->dots_agony->is_ticking();
    corruption = corruption || td->dots_corruption->is_ticking();

    if ( agony && corruption )
      break;
  }

  return agony && corruption && ( p->ua_target && p->get_target_data( p->ua_target )->dots_unstable_affliction->is_ticking() );
}

// Use this as a helper function when two versions are needed simultaneously (ie a PTR cycle)
// It must be adjusted manually over time, and any use of it should be removed once a patch goes live
// Returns TRUE if actor's dbc version >= version specified
// When checking VERSION_PTR, will only return true if PTR dbc is being used, regardless of version number
bool warlock_t::min_version_check( version_check_e version ) const
{
  //If we ever get a full DBC version string checker, replace these returns with that function
  switch ( version )
  {
    case VERSION_PTR:
      return is_ptr();
    case VERSION_ANY:
      return true;
  }

  return false;
}

action_t* warlock_t::pass_corruption_action( warlock_t* p )
{
  return debug_cast<action_t*>( new actions::corruption_t( p, "", false ) );
}

// Function for returning the the number of imps that will spawn in a specified time period.
int warlock_t::imps_spawned_during( timespan_t period )
{
  int count = 0;

  for ( auto ev : wild_imp_spawns )
  {
    timespan_t ex = debug_cast<actions::imp_delay_event_t*>( ev )->expected_time();

    if ( ex < period )
      count++;
  }

  return count;
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype & SAVE_PLAYER )
  {
    if ( initial_soul_shards != 3 )
      profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )
      profile_str += "default_pet=" + default_pet + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  auto* p = debug_cast<warlock_t*>( source );

  initial_soul_shards  = p->initial_soul_shards;
  default_pet          = p->default_pet;
}

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
      // This is all a guess at how the hybrid primaries will work, since they
      // don't actually appear on cloth gear yet. TODO: confirm behavior
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
      return STAT_NONE;
    case STAT_SPIRIT:
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

pet_t* warlock_t::create_main_pet( util::string_view pet_name, util::string_view pet_type )
{
  pet_t* p = find_pet( pet_name );
  if ( p )
    return p;
  using namespace pets;

  if ( pet_name == "felhunter" )
    return new pets::base::felhunter_pet_t( this, pet_name );
  if ( pet_name == "imp" )
    return new pets::base::imp_pet_t( this, pet_name );
  if ( pet_name == "sayaad" || pet_name == "incubus" || pet_name == "succubus" )
    return new pets::base::sayaad_pet_t( this, pet_name );
  if ( pet_name == "voidwalker" )
    return new pets::base::voidwalker_pet_t( this, pet_name );
  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    return create_demo_pet( pet_name, pet_type );
  }

  return nullptr;
}

void warlock_t::create_all_pets()
{
  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    for ( auto& infernal : warlock_pet_list.infernals )
    {
      infernal = new pets::destruction::infernal_t( this );
    }
  }
}

//TODO: Are these expressions outdated?
std::unique_ptr<expr_t> warlock_t::create_pet_expression( util::string_view name_str )
{
  if ( name_str == "last_cast_imps" )
  {
    return make_fn_expr( "last_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] <= 20;
      } );
    } );
  }
  else if ( name_str == "two_cast_imps" )
  {
    return make_fn_expr( "two_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] <= 40;
      } );
    } );
  }

  return player_t::create_expression( name_str );
}

std::unique_ptr<expr_t> warlock_t::create_expression( util::string_view name_str )
{
  if ( name_str == "time_to_shard" )
  {
    auto agony_id = find_action_id( "agony" );

    return make_fn_expr( name_str, [ this, agony_id ]() {
      auto td               = get_target_data( target );
      double active_agonies = get_active_dots( agony_id );
      if ( sim->debug )
        sim->out_debug.printf( "active agonies: %f", active_agonies );
      dot_t* agony = td->dots_agony;
      if ( active_agonies == 0 || !agony->current_action )
      {
        return std::numeric_limits<double>::infinity();
      }
      action_state_t* agony_state = agony->current_action->get_state( agony->state );
      timespan_t dot_tick_time    = agony->current_action->tick_time( agony_state );

      // Seeks to return the average expected time for the player to generate a single soul shard.
      // TOCHECK regularly.

      double average =
          1.0 / ( 0.184 * std::pow( active_agonies, -2.0 / 3.0 ) ) * dot_tick_time.total_seconds() / active_agonies;

      if ( talents.creeping_death->ok() )
        average /= 1.0 + talents.creeping_death->effectN( 1 ).percent();

      if ( sim->debug )
        sim->out_debug.printf( "time to shard return: %f", average );
      action_state_t::release( agony_state );
      return average;
    } );
  }
  else if ( name_str == "pet_count" )
  {
    return make_ref_expr( name_str, active_pets );
  }
  else if ( name_str == "last_cast_imps" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "two_cast_imps" )
  {
    return create_pet_expression( name_str );
  }
  // TODO: Remove the deprecated buff expressions once the APL is adjusted for
  // havoc_active/havoc_remains.
  else if ( name_str == "havoc_active" || name_str == "buff.active_havoc.up" )
  {
    return make_fn_expr( name_str, [ this ] { return havoc_target != nullptr; } );
  }
  else if ( name_str == "havoc_remains" || name_str == "buff.active_havoc.remains" )
  {
    return make_fn_expr( name_str, [ this ] {
      return havoc_target ? get_target_data( havoc_target )->debuffs_havoc->remains().total_seconds() : 0.0;
    } );
  }
  else if ( name_str == "incoming_imps" )
  {
    return make_fn_expr( name_str, [ this ] { return this->get_spawning_imp_count(); } );
  }
  else if ( name_str == "can_seed" )
  {
    std::vector<action_t*> soc_list;
    for ( auto a : action_list )
    {
      if ( a->name_str == "seed_of_corruption" )
        soc_list.push_back( a );
    }

    return make_fn_expr( name_str, [this, soc_list] {
      std::vector<player_t*> no_dots;

      if ( soc_list.empty() ) 
        return false;

      //All the actions should have the same target list, so do this once only
      auto tl = soc_list[ 0 ]->target_list();

      for ( auto t : tl )
      {
        if ( !get_target_data( t )->dots_seed_of_corruption->is_ticking() )
          no_dots.push_back( t );
      }

      //If there are no targets without a seed already, this expression should be false
      if ( no_dots.empty() )
        return false;

      //If all of the remaining unseeded targets have a seed in flight, we should also return false
      for ( auto t : no_dots )
      {
        bool can_seed = true;

        for ( auto s : soc_list )
        {
          if ( s->has_travel_events_for( t ) )
          {
            can_seed = false;
            break;
          }
        }

        if ( can_seed )
          return true;
      }

      return false;
    });
  }

  auto splits = util::string_split<util::string_view>( name_str, "." );

  if ( splits.size() == 3 && splits[ 0 ] == "time_to_imps" && splits[ 2 ] == "remains" )
  {
    auto amt = splits[ 1 ] == "all" ? -1 : util::to_int( splits[ 1 ] );

    return make_fn_expr( name_str, [ this, amt ]() {
      return this->time_to_imps( amt );
    } );
  }
  //TODO: Is this outdated in Shadowlands?
  else if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "imps_spawned_during" ) )
  {
    auto period = util::to_double( splits[ 1 ] );

    return make_fn_expr( name_str, [ this, period ]() {
      // Add a custom split .summon_demonic_tyrant which returns its cast time.
      return this->imps_spawned_during( timespan_t::from_millis( period ) );
    } );
  }

  return player_t::create_expression( name_str );
}

void warlock_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  if ( warlock_base.demonology_warlock )
  {
    action.apply_affecting_aura( warlock_base.demonology_warlock );
  }
  if ( warlock_base.destruction_warlock )
  {
    action.apply_affecting_aura( warlock_base.destruction_warlock );
  }
  if ( warlock_base.affliction_warlock )
  {
    action.apply_affecting_aura( warlock_base.affliction_warlock );
  }
}

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    return new warlock_t( sim, name, r );
  }

  //TODO: Hotfix may not be needed any longer, if so leave this function empty instead
  void register_hotfixes() const override
  {
    hotfix::register_spell("Warlock", "2020-11-15", "Manually set secondary Malefic Rapture level requirement", 324540)
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 11.0 )
      .verification_value( 43.0 );
  }

  bool valid() const override
  {
    return true;
  }
  void init( player_t* ) const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

warlock::warlock_t::pets_t::pets_t( warlock_t* w )
  : active( nullptr ),
    last( nullptr ),
    darkglare( "darkglare", w ),
    roc_infernals( "roc_infernal", w ),
    blasphemy( "blasphemy", w ),
    dreadstalkers( "dreadstalker", w ),
    vilefiends( "vilefiend", w ),
    demonic_tyrants( "demonic_tyrant", w ),
    grimoire_felguards( "grimoire_felguard", w ),
    wild_imps( "wild_imp", w ),
    shivarra( "shivarra", w ),
    darkhounds( "darkhound", w ),
    bilescourges( "bilescourge", w ),
    urzuls( "urzul", w ),
    void_terrors( "void_terror", w ),
    wrathguards( "wrathguard", w ),
    vicious_hellhounds( "vicious_hellhound", w ),
    illidari_satyrs( "illidari_satyr", w ),
    eyes_of_guldan( "eye_of_guldan", w ),
    prince_malchezaar( "prince_malchezaar", w )
{
}
}  // namespace warlock

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}

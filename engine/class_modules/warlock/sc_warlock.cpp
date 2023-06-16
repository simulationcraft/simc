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
  // Note: Soul Rot (Affliction talent) turns Drain Life into a multi-target channeled spell. Nothing else in simc behaves this way and
  // we currently do not have core support for it. Applying this dot to the secondary targets should cover most of the behavior, although
  // it will be unable to handle the case where primary channel target dies (in-game, this appears to force-swap primary target to another
  // target currently affected by Drain Life if possible).
  struct drain_life_dot_t : public warlock_spell_t
  {
    drain_life_dot_t( warlock_t* p ) : warlock_spell_t( "Drain Life (AoE)", p, p->warlock_base.drain_life )
    {
      dual = background = true;

      dot_duration *= 1.0 + p->talents.grim_feast->effectN( 1 ).percent();
      base_tick_time *= 1.0 + p->talents.grim_feast->effectN( 2 ).percent();
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
        return dot_duration * ( tick_time( s ) / base_tick_time); // We need this to model this as "channel" behavior since we can't actually set it as a channel without breaking things
    }
  };

  drain_life_dot_t* aoe_dot;

  drain_life_t( warlock_t* p, util::string_view options_str ) : warlock_spell_t( "Drain Life", p, p->warlock_base.drain_life )
  {
    parse_options( options_str );

    aoe_dot = new drain_life_dot_t( p );
    add_child( aoe_dot );
    
    channeled = true;

    dot_duration *= 1.0 + p->talents.grim_feast->effectN( 1 ).percent();
    base_tick_time *= 1.0 + p->talents.grim_feast->effectN( 2 ).percent();
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

        if ( td( t )->dots_soul_rot->is_ticking() )
        {
          aoe_dot->execute_on_target( t );
        }
      }
    }

    p()->buffs.soulburn->expire();
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
    p()->buffs.inevitable_demise->expire( 1_ms ); // Slight delay added so that the AoE version of Drain Life picks up the benefit of Inevitable Demise

    bool early_cancel = d->remains() > 0_ms;

    warlock_spell_t::last_tick( d );

    // If this is the end of the channel, the AoE DoTs will expire correctly
    // Otherwise, we need to cancel them on the spot
    if ( p()->talents.soul_rot->ok() && early_cancel )
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

struct doom_blossom_t : public warlock_spell_t
{
  doom_blossom_t( warlock_t* p ) : warlock_spell_t( "Doom Blossom", p, p->talents.doom_blossom_proc )
  {
    background = dual = true;
    aoe = -1;
  }
};

struct corruption_t : public warlock_spell_t
{
  struct corruption_dot_t : public warlock_spell_t
  {
    doom_blossom_t* doom_blossom_proc;

    corruption_dot_t( warlock_t* p ) : warlock_spell_t( "Corruption", p, p->warlock_base.corruption->effectN( 1 ).trigger() ),
      doom_blossom_proc( new doom_blossom_t( p ) )
    {
      tick_zero = false;
      background = dual = true;

      if ( !p->min_version_check( VERSION_10_1_5 ) )
        add_child( doom_blossom_proc );

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

        if ( p()->talents.doom_blossom->ok() && !p()->min_version_check( VERSION_10_1_5 ) && td( d->state->target )->dots_unstable_affliction->is_ticking() )
        {
          if ( p()->buffs.malefic_affliction->check() && rng().roll( p()->buffs.malefic_affliction->check() * p()->talents.doom_blossom->effectN( 1 ).percent() ) )
          {
            doom_blossom_proc->execute_on_target( d->state->target );
            p()->procs.doom_blossom->occur();
          }
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
    : warlock_spell_t( "Corruption (Direct)", p, p->warlock_base.corruption )
  {
    parse_options( options_str );

    periodic = new corruption_dot_t( p );
    impact_action = periodic;
    add_child( periodic );

    spell_power_mod.direct = 0; // By default, Corruption does not deal instant damage

    if ( !seed_action && p->warlock_base.xavian_teachings->ok() )
    {
      spell_power_mod.direct = data().effectN( 3 ).sp_coeff(); // Spell uses this effect in base spell for damage
      base_execute_time *= 1.0 + p->warlock_base.xavian_teachings->effectN( 1 ).percent();
    }
  }

  void impact( action_state_t* s ) override
  {
    bool pi_trigger = p()->talents.pandemic_invocation->ok() && td( s->target )->dots_corruption->is_ticking()
      && td( s->target )->dots_corruption->remains() < p()->talents.pandemic_invocation->effectN( 1 ).time_value();

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

  dot_t* get_dot( player_t* t ) override
  {
    return periodic->get_dot( t );
  }
};

// Needs to be here due to Corruption being a shared spell
struct seed_of_corruption_t : public warlock_spell_t
{
  struct seed_of_corruption_aoe_t : public warlock_spell_t
  {
    corruption_t* corruption;
    bool cruel_epiphany;
    doom_blossom_t* doom_blossom;

    seed_of_corruption_aoe_t( warlock_t* p )
      : warlock_spell_t( "Seed of Corruption (AoE)", p, p->talents.seed_of_corruption_aoe ),
        corruption( new corruption_t( p, "", true ) ),
        cruel_epiphany( false ),
        doom_blossom( new doom_blossom_t( p ) )
    {
      aoe = -1;
      background = dual = true;

      // TODO: Figure out how to adjust DPET of SoC to pick up Corruption damage appropriately

      corruption->background = true;
      corruption->dual = true;
      corruption->base_costs[ RESOURCE_MANA ] = 0;

      if ( p->min_version_check( VERSION_10_1_5 ) )
        add_child( doom_blossom );
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        auto tdata = td( s->target );
        if ( tdata->dots_seed_of_corruption->is_ticking() && tdata->soc_threshold > 0 )
        {
          tdata->soc_threshold = 0;
          tdata->dots_seed_of_corruption->cancel();
        }

        if ( p()->min_version_check( VERSION_10_1_5 ) && p()->talents.doom_blossom->ok() && tdata->dots_unstable_affliction->is_ticking() )
        {
          doom_blossom->execute_on_target( s->target );
          p()->procs.doom_blossom->occur();
        }

        // 2022-09-24 Agonizing Corruption does not apply Agony, only increments existing ones
        if ( p()->talents.agonizing_corruption->ok() && tdata->dots_agony->is_ticking() )
        {
          tdata->dots_agony->increment( (int)p()->talents.agonizing_corruption->effectN( 1 ).base_value() );
        }

        corruption->execute_on_target( s->target );
      }
    }

    // Copied from affliction_spell_t since this needs to be a warlock_spell_t
    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 2 ) ) )
      {
        m *= 1.0 + p()->cache.mastery_value();
      }

      if ( p()->buffs.cruel_epiphany->check() && cruel_epiphany )
        m *= 1.0 + p()->buffs.cruel_epiphany->check_value();

      return m;
    }
  };

  seed_of_corruption_aoe_t* explosion;
  seed_of_corruption_aoe_t* epiphany_explosion;

  seed_of_corruption_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "Seed of Corruption", p, p->talents.seed_of_corruption ),
      explosion( new seed_of_corruption_aoe_t( p ) ),
      epiphany_explosion( new seed_of_corruption_aoe_t( p ) )
  {
    parse_options( options_str );
    may_crit = false;
    tick_zero = false;
    base_tick_time = dot_duration;
    hasted_ticks = false;

    add_child( explosion );

    epiphany_explosion->cruel_epiphany = true;
    add_child( epiphany_explosion );

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

  void execute() override
  {
    warlock_spell_t::execute();

    // 2022-10-17: Cruel Epiphany provides the damage bonus on explosion, but decrements on cast
    // TOCHECK, as this can create unfortunate situations in-game and may be considered a bug
    p()->buffs.cruel_epiphany->decrement();
  }

  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s->result ) )
    {
      td( s->target )->soc_threshold = s->composite_spell_power() * p()->talents.seed_of_corruption->effectN( 1 ).percent();
    }

    warlock_spell_t::impact( s );

    if ( s->chain_target == 0 && p()->buffs.cruel_epiphany->check() )
      td( s->target )->debuffs_cruel_epiphany->trigger();
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
    if ( td( d->target )->debuffs_cruel_epiphany->check() )
    {
      epiphany_explosion->set_target( d->target );
      epiphany_explosion->schedule_execute();
    }
    else
    {
      explosion->set_target( d->target );
      explosion->schedule_execute();
    }

    td( d->target )->debuffs_cruel_epiphany->expire();

    warlock_spell_t::last_tick( d );
  }
};

struct shadow_bolt_t : public warlock_spell_t
{
  shadow_bolt_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "Shadow Bolt", p, p->talents.drain_soul->ok() ? spell_data_t::not_found() : p->warlock_base.shadow_bolt )
  {
    parse_options( options_str );

    if ( p->specialization() == WARLOCK_DEMONOLOGY )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount = 1.0;
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
    
    if ( p()->talents.demonic_calling->ok() )
      p()->buffs.demonic_calling->trigger();

    if ( p()->talents.fel_covenant->ok() )
      p()->buffs.fel_covenant->trigger();

    p()->buffs.stolen_power_final->expire();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p()->talents.shadow_embrace->ok() )
        td( s->target )->debuffs_shadow_embrace->trigger();

      if ( p()->talents.tormented_crescendo->ok() )
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

    if ( p()->talents.sacrificed_souls->ok() )
      m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_demon_count();

    if ( p()->talents.stolen_power->ok() && p()->buffs.stolen_power_final->check() )
      m *= 1.0 + p()->talents.stolen_power_final_buff->effectN( 1 ).percent();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    if ( p()->talents.withering_bolt->ok() )
      m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( (int)p()->talents.withering_bolt->effectN( 2 ).base_value(), p()->get_target_data( t )->count_affliction_dots() );

    return m;
  }
};

struct soul_rot_t : public warlock_spell_t
{
  soul_rot_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "Soul Rot", p, p->talents.soul_rot )
  {
    parse_options( options_str );
    aoe = 1 + as<int>( p->talents.soul_rot->effectN( 3 ).base_value() );
  }

  soul_rot_t( warlock_t* p, util::string_view opt, bool soul_swap ) : soul_rot_t( p, opt )
  {
    if ( soul_swap )
    {
      aoe = 1;
    }
  }

  void execute() override
  {
    warlock_spell_t::execute();

    p()->buffs.soul_rot->trigger();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( p()->talents.dark_harvest->ok() && aoe > 1)
    {
      p()->buffs.dark_harvest_haste->trigger();
      p()->buffs.dark_harvest_crit->trigger();
    }
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = warlock_spell_t::composite_ta_multiplier( s );

    if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 1 ) ) )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    // Note: Soul Swapped Soul Rot technically retains memory of who the primary target was
    // For the moment, we will shortcut this by assuming Soul Swap copy is going on a secondary target
    // TODO: Figure out how to model this appropriately in the case where you copy a secondary and then apply to primary
    if ( s->chain_target == 0 && aoe > 1 )
    {
      m *= 1.0 + p()->talents.soul_rot->effectN( 4 ).base_value() / 10.0; // Primary target takes increased damage
    }

    return m;
  }
};

struct soul_flame_t : public warlock_spell_t
{
  soul_flame_t( warlock_t* p ) : warlock_spell_t( "Soul Flame", p, p->talents.soul_flame_proc )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = p->talents.soul_flame->effectN( 4 ).base_value();

    base_dd_multiplier = 1.0 + p->talents.soul_flame->effectN( 2 ).percent();
  }
};

struct pandemic_invocation_t : public warlock_spell_t
{
  pandemic_invocation_t( warlock_t* p ) : warlock_spell_t( "Pandemic Invocation", p, p->talents.pandemic_invocation_proc )
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
    : warlock_spell_t( "Grimoire of Sacrifice", p, p->talents.grimoire_of_sacrifice )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;
    target = player;
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

struct summon_soulkeeper_t : public warlock_spell_t
{
  struct soul_combustion_t : public warlock_spell_t
  {
    int tormented_souls;

    soul_combustion_t( warlock_t* p ) : warlock_spell_t( "Soul Combustion", p, p->talents.soul_combustion )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = 8.0; // Presumably hardcoded, mentioned in tooltip

      tormented_souls = 1;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      m *= tormented_souls;

      return m;
    }
  };

  summon_soulkeeper_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "Summon Soulkeeper", p, p->talents.summon_soulkeeper_aoe )
  {
    parse_options( options_str );
    harmful = true;
    dot_duration = 0_ms;

    if ( !p->proc_actions.soul_combustion )
    {
      p->proc_actions.soul_combustion = new soul_combustion_t( p );
      p->proc_actions.soul_combustion->stats = stats;
    }
  }

  bool usable_moving() const override
  {
    return true; // Last checked 2023-01-19
  }

  bool ready() override
  {
    if ( !p()->buffs.tormented_soul->check() )
      return false;

    return warlock_spell_t::ready();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    timespan_t dur = 0_ms;

    dur = p()->talents.summon_soulkeeper_aoe->duration() - 2_s + 1_s; // Hardcoded -2 according to tooltip, but is doing 9 ticks as of 2023-01-19
    debug_cast<soul_combustion_t*>( p()->proc_actions.soul_combustion )->tormented_souls = p()->buffs.tormented_soul->stack();

    make_event<ground_aoe_event_t>( *sim, p(),
                                ground_aoe_params_t()
                                    .target( execute_state->target )
                                    .x( execute_state->target->x_position )
                                    .y( execute_state->target->y_position )
                                    .pulse_time( base_tick_time )
                                    .duration( dur )
                                    .start_time( sim->current_time() )
                                    .action( p()->proc_actions.soul_combustion ) );

    internal_cooldown->start( dur ); // As of 10.0.7, Soulkeepr can no longer be used if one is already active

    p()->buffs.tormented_soul->expire();
  }
};

struct fel_barrage_t : public warlock_spell_t
{
  fel_barrage_t( warlock_t* p ) : warlock_spell_t( "Fel Barrage", p, p->talents.fel_barrage )
  {
    background = dual = true;
  }
};

struct soulburn_t : public warlock_spell_t
{
  soulburn_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "Soulburn", p, p->talents.soulburn )
  {
    parse_options( options_str );
    harmful = false;
    may_crit = false;
  }

  bool ready() override
  {
    if ( p()->buffs.soulburn->check() )
      return false;

    return warlock_spell_t::ready();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    p()->buffs.soulburn->trigger();
  }

  void consume_resource() override
  {
    warlock_spell_t::consume_resource();

    if ( p()->specialization() == WARLOCK_DEMONOLOGY )
    {
      if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
      {
        if ( p()->buffs.nether_portal->up() )
        {
          p()->proc_actions.summon_random_demon->execute();
          p()->procs.portal_summon->occur();

          if ( p()->talents.guldans_ambition->ok() )
            p()->buffs.nether_portal_total->trigger();

          if ( p()->talents.nerzhuls_volition->ok() && rng().roll( p()->talents.nerzhuls_volition->effectN( 1 ).percent() ) )
          {
            p()->proc_actions.summon_random_demon->execute();
            p()->procs.nerzhuls_volition->occur();

            if ( p()->talents.guldans_ambition->ok() )
              p()->buffs.nether_portal_total->trigger();
          }
        }
      }
    }
    else if ( p()->specialization() == WARLOCK_DESTRUCTION )
    {
      int shards_used = as<int>( cost() );
      if ( resource_current == RESOURCE_SOUL_SHARD && p()->buffs.rain_of_chaos->check() && shards_used > 0 )
      {
        for ( int i = 0; i < shards_used; i++ )
        {
          if ( p()->rain_of_chaos_rng->trigger() )
          {
            p()->warlock_pet_list.infernals.spawn( p()->talents.summon_infernal_roc->duration() );
            p()->procs.rain_of_chaos->occur();
          }
        }
      }
    }
  }
};

// Catchall action to trigger pet interrupt abilities via main APL.
// Using this should ensure that interrupt callback effects (Sephuz etc) proc correctly for the warlock.
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
        pet->special_action->execute_on_target( target );
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
  // Shared
  dots_drain_life = target->get_dot( "drain_life", &p );
  dots_drain_life_aoe = target->get_dot( "drain_life_aoe", &p );

  // Affliction
  dots_corruption = target->get_dot( "corruption", &p );
  dots_agony = target->get_dot( "agony", &p );
  dots_drain_soul = target->get_dot( "drain_soul", &p );
  dots_phantom_singularity = target->get_dot( "phantom_singularity", &p );
  dots_siphon_life = target->get_dot( "siphon_life", &p );
  dots_seed_of_corruption = target->get_dot( "seed_of_corruption", &p );
  dots_unstable_affliction = target->get_dot( "unstable_affliction", &p );
  dots_vile_taint = target->get_dot( "vile_taint_dot", &p );
  dots_soul_rot = target->get_dot( "soul_rot", &p );

  debuffs_haunt = make_buff( *this, "haunt", p.talents.haunt )
                      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                      ->set_default_value_from_effect( 2 )
                      ->set_cooldown( 0_ms )
                      ->set_stack_change_callback( [ &p ]( buff_t*, int prev, int cur ) {
                          if ( cur < prev )
                          {
                            p.buffs.active_haunts->decrement();
                          }
                          else if ( cur > prev )
                          {
                            p.buffs.active_haunts->trigger();
                          }
                        } );

  debuffs_shadow_embrace = make_buff( *this, "shadow_embrace", p.talents.shadow_embrace_debuff )
                               ->set_default_value( p.talents.shadow_embrace->effectN( 1 ).percent() );

  debuffs_dread_touch = make_buff( *this, "dread_touch", p.talents.dread_touch_debuff )
                            ->set_default_value( p.talents.dread_touch_debuff->effectN( 1 ).percent() );

  debuffs_cruel_epiphany = make_buff( *this, "cruel_epiphany" );

  debuffs_infirmity = make_buff( *this, "infirmity", p.tier.infirmity )
                          ->set_default_value( p.tier.infirmity->effectN( 1 ).percent() )
                          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Destruction
  dots_immolate = target->get_dot( "immolate", &p );

  debuffs_eradication = make_buff( *this, "eradication", p.talents.eradication_debuff )
                            ->set_default_value( p.talents.eradication->effectN( 2 ).percent() );

  debuffs_shadowburn = make_buff( *this, "shadowburn", p.talents.shadowburn )
                           ->set_default_value( p.talents.shadowburn_2->effectN( 1 ).base_value() / 10 );

  debuffs_pyrogenics = make_buff( *this, "pyrogenics", p.talents.pyrogenics_debuff )
                           ->set_default_value( p.talents.pyrogenics->effectN( 1 ).percent() )
                           ->set_schools_from_effect( 1 )
                           ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  debuffs_conflagrate = make_buff( *this, "conflagrate", p.talents.conflagrate_debuff )
                            ->set_default_value_from_effect( 1 );

  // Use havoc_debuff where we need the data but don't have the active talent
  debuffs_havoc = make_buff( *this, "havoc", p.talents.havoc_debuff )
                      ->set_duration( p.talents.mayhem->ok() ? p.talents.mayhem->effectN( 3 ).time_value() : p.talents.havoc->duration() + p.talents.pandemonium->effectN( 1 ).time_value() )
                      ->set_cooldown( p.talents.mayhem->ok() ? p.talents.mayhem->internal_cooldown() : 0_ms )
                      ->set_chance( p.talents.mayhem->ok() ? p.talents.mayhem->effectN( 1 ).percent() + p.talents.pandemonium->effectN( 2 ).percent() : p.talents.havoc->proc_chance() )
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

  // Demonology
  dots_doom = target->get_dot( "doom", &p );

  debuffs_the_houndmasters_stratagem = make_buff( *this, "the_houndmasters_stratagem", p.talents.the_houndmasters_stratagem_debuff )
                                           ->set_default_value_from_effect( 1 );

  debuffs_fel_sunder = make_buff( *this, "fel_sunder", p.talents.fel_sunder_debuff )
                           ->set_default_value( p.talents.fel_sunder->effectN( 1 ).percent() );

  debuffs_kazaaks_final_curse = make_buff( *this, "kazaaks_final_curse", p.talents.kazaaks_final_curse )
                                    ->set_default_value( 0 );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void warlock_td_t::target_demise()
{
  if ( !( target->is_enemy() ) )
  {
    return;
  }

  if ( dots_unstable_affliction->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Unstable Affliction.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, warlock.talents.unstable_affliction_2->effectN( 1 ).base_value(), warlock.gains.unstable_affliction_refund );
  }
  if ( dots_drain_soul->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Drain Soul.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1.0, warlock.gains.drain_soul );
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

    warlock.resource_gain( RESOURCE_SOUL_SHARD, debuffs_shadowburn->check_value(), warlock.gains.shadowburn_refund );
  }

  if ( dots_agony->is_ticking() && warlock.talents.wrath_of_consumption->ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Wrath of Consumption from Agony.", target->name(), warlock.name() );

    warlock.buffs.wrath_of_consumption->trigger();
  }

  if ( dots_corruption->is_ticking() && warlock.talents.wrath_of_consumption->ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Wrath of Consumption from Corruption.", target->name(), warlock.name() );

    warlock.buffs.wrath_of_consumption->trigger();
  }

  if ( warlock.talents.soul_flame->ok() && !warlock.proc_actions.soul_flame_proc->target_list().empty() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Soul Flame on all targets in range.", target->name(), warlock.name() );

    warlock.proc_actions.soul_flame_proc->execute();
  }

  if ( warlock.talents.summon_soulkeeper->ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock gains 1 stack of Tormented Soul.", target->name(), warlock.name() );

    warlock.buffs.tormented_soul->trigger();
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
      td->source->sim->print_log( "Remaining damage to explode Seed of Corruption on {} is {}.", td->target->name_str, td->soc_threshold );
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
    ss_source( nullptr ),
    soul_swap_state(),
    havoc_spells(),
    agony_accumulator( 0.0 ),
    corruption_accumulator( 0.0 ),
    cdf_accumulator( 0.0 ),
    incinerate_last_target_count( 0 ),
    active_pets( 0 ),
    warlock_pet_list( this ),
    talents(),
    proc_actions(),
    tier(),
    cooldowns(),
    buffs(),
    gains(),
    procs(),
    initial_soul_shards( 3 ),
    disable_auto_felstorm( false ),
    default_pet(),
    use_pet_stat_update_delay( false )
{
  cooldowns.haunt = get_cooldown( "haunt" );
  cooldowns.darkglare = get_cooldown( "summon_darkglare" );
  cooldowns.demonic_tyrant = get_cooldown( "summon_demonic_tyrant" );
  cooldowns.infernal = get_cooldown( "summon_infernal" );
  cooldowns.shadowburn = get_cooldown( "shadowburn" );
  cooldowns.soul_rot = get_cooldown( "soul_rot" );
  cooldowns.call_dreadstalkers = get_cooldown( "call_dreadstalkers" );
  cooldowns.soul_fire = get_cooldown( "soul_fire" );
  cooldowns.felstorm_icd = get_cooldown( "felstorm_icd" );
  cooldowns.grimoire_felguard = get_cooldown( "grimoire_felguard" );

  resource_regeneration = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ] = true;
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
    if ( talents.haunt->ok() )
      m *= 1.0 + td->debuffs_haunt->check_value();

    if ( talents.shadow_embrace->ok() )
      m *= 1.0 + td->debuffs_shadow_embrace->check_stack_value();

    if ( sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
      m *= 1.0 + td->debuffs_infirmity->check_stack_value();
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( talents.eradication->ok() )
      m *= 1.0 + td->debuffs_eradication->check_value();

    if ( td->debuffs_pyrogenics->check() && td->debuffs_pyrogenics->has_common_school( school ) )
      m *= 1.0 + td->debuffs_pyrogenics->check_value();
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    if ( td->debuffs_fel_sunder->check() )
      m *= 1.0 + td->debuffs_fel_sunder->check_stack_value();
  }

  return m;
}

double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( buffs.rolling_havoc->check() )
      m *= 1.0 + buffs.rolling_havoc->check_stack_value();
  }

  return m;
}

double warlock_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    m *= 1.0 + warlock_base.destruction_warlock->effectN( guardian ? 4 : 3 ).percent();

    // 2022-11-27 Rolling Havoc is missing the aura for guardians
    if ( talents.rolling_havoc->ok() && !guardian )
      m *= 1.0 + buffs.rolling_havoc->check_stack_value();
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    m *= 1.0 + warlock_base.demonology_warlock->effectN( guardian ? 5 : 3 ).percent();
    m *= 1.0 + cache.mastery_value();

    if ( talents.summon_demonic_tyrant->ok() )
      m *= 1.0 + buffs.demonic_power->check_value();

    if ( buffs.rite_of_ruvaraad->check() )
      m *= 1.0 + buffs.rite_of_ruvaraad->check_value();
  }

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    m *= 1.0 + warlock_base.affliction_warlock->effectN( guardian ? 7 : 3 ).percent();
  }

  return m;
}

double warlock_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  const warlock_td_t* td = get_target_data( target );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( talents.haunt->ok() && td->debuffs_haunt->check() )
    {
      m *= 1.0 + td->debuffs_haunt->data().effectN( guardian ? 4 : 3 ).percent();
    }

    if ( talents.shadow_embrace->ok() )
    {
      m *= 1.0 + td->debuffs_shadow_embrace->check_stack_value(); // Talent spell sets default value according to rank
    }

    if ( sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) && !guardian )
    {
      // TOCHECK: Guardian effect is missing from spell data as of 2023-04-04
      m *= 1.0 + td->debuffs_infirmity->check_stack_value();
    }
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( talents.eradication->ok() )
    {
      m *= 1.0 + td->debuffs_eradication->check_value();
    }
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    // Fel Sunder lacks guardian effect, so only main pet is benefitting. Last checked 2022-11-27
    if ( talents.fel_sunder->ok() && !guardian )
      m *= 1.0 + td->debuffs_fel_sunder->check_stack_value();
  }

  return m;
}

double warlock_t::composite_spell_crit_chance() const
{
  double m = player_t::composite_spell_crit_chance();

  if ( specialization() == WARLOCK_DESTRUCTION && talents.backlash->ok() )
    m += talents.backlash->effectN( 1 ).percent();

  return m;
}

double warlock_t::composite_melee_crit_chance() const
{
  double m = player_t::composite_melee_crit_chance();

  if ( specialization() == WARLOCK_DESTRUCTION && talents.backlash->ok() )
    m += talents.backlash->effectN( 1 ).percent();

  return m;
}

// Note: Level is checked to be >=27 by the function calling this. This is technically wrong for warlocks due to
// a missing level requirement in data, but correct generally.
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
  if ( action_name == "summon_soulkeeper" )
    return new summon_soulkeeper_t( this, options_str );
  if ( action_name == "soulburn" )
    return new soulburn_t( this, options_str );

  return nullptr;
}

void warlock_t::create_actions()
{
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( talents.soul_flame->ok() )
      proc_actions.soul_flame_proc = new warlock::actions::soul_flame_t( this );

    if ( talents.pandemic_invocation->ok() )
      proc_actions.pandemic_invocation_proc = new warlock::actions::pandemic_invocation_t( this );

    if ( talents.soul_swap->ok() )
      create_soul_swap_actions();
  }

  if ( talents.inquisitors_gaze->ok() )
    proc_actions.fel_barrage = new warlock::actions::fel_barrage_t( this );

  player_t::create_actions();
}

action_t* warlock_t::create_action( util::string_view action_name, util::string_view options_str )
{
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

  pet_t* summon_pet = create_main_pet( pet_name, pet_type );
  if ( summon_pet )
    return summon_pet;

  return nullptr;
}

void warlock_t::create_pets()
{
  for ( auto& pet : pet_name_list )
  {
    create_pet( pet );
  }
}

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  // Shared buffs
  buffs.grimoire_of_sacrifice = make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice_buff )
                                    ->set_chance( 1.0 );

  buffs.demonic_synergy = make_buff( this, "demonic_synergy", talents.demonic_synergy )
                              ->set_default_value( talents.grimoire_of_synergy->effectN( 2 ).percent() );

  buffs.tormented_soul = make_buff( this, "tormented_soul", talents.tormented_soul_buff );

  buffs.tormented_soul_generator = make_buff( this, "tormented_soul_generator" )
                                       ->set_period( talents.summon_soulkeeper->effectN( 2 ).period() )
                                       ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                                       ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                                           buffs.tormented_soul->trigger();
                                         } );
  buffs.tormented_soul_generator->quiet = true;

  buffs.inquisitors_gaze = make_buff( this, "inquisitors_gaze", talents.inquisitors_gaze_buff )
                               ->set_period( 1_s )
                               ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                                 proc_actions.fel_barrage->execute_on_target( target );
                               } );

  buffs.soulburn = make_buff( this, "soulburn", talents.soulburn_buff );

  buffs.pet_movement = make_buff( this, "pet_movement" )->set_max_stack( 100 );

  // Affliction buffs
  create_buffs_affliction();

  buffs.soul_rot = make_buff(this, "soul_rot", talents.soul_rot)
                       ->set_cooldown( 0_ms );

  buffs.wrath_of_consumption = make_buff( this, "wrath_of_consumption", talents.wrath_of_consumption_buff )
                               ->set_default_value( talents.wrath_of_consumption->effectN( 2 ).percent() );

  buffs.dark_harvest_haste = make_buff( this, "dark_harvest_haste", talents.dark_harvest_buff )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                 ->set_default_value( talents.dark_harvest_buff->effectN( 1 ).percent() );

  buffs.dark_harvest_crit = make_buff( this, "dark_harvest_crit", talents.dark_harvest_buff )
                                ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                ->set_default_value( talents.dark_harvest_buff->effectN( 2 ).percent() );

  // Demonology buffs
  create_buffs_demonology();

  // Destruction buffs
  create_buffs_destruction();

  buffs.rolling_havoc = make_buff( this, "rolling_havoc", talents.rolling_havoc_buff )
                            ->set_default_value( talents.rolling_havoc->effectN( 1 ).percent() )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  version_10_1_5_data = find_spell( 417282 ); // For 10.1.5 version checking, New Crashing Chaos Buff

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
  warlock_base.xavian_teachings = find_specialization_spell( "Xavian Teachings", WARLOCK_AFFLICTION ); // Instant cast corruption and direct damage. Direct damage is in the base corruption spell on effect 3. Should be ID 317031.
  warlock_base.potent_afflictions = find_mastery_spell( WARLOCK_AFFLICTION ); // Should be ID 77215
  warlock_base.affliction_warlock = find_specialization_spell( "Affliction Warlock", WARLOCK_AFFLICTION ); // Should be ID 137043

  // Demonology
  warlock_base.hand_of_guldan = find_class_spell( "Hand of Gul'dan" ); // Should be ID 105174
  warlock_base.hog_impact = find_spell( 86040 ); // Contains impact damage data
  warlock_base.wild_imp = find_spell( 104317 ); // Contains pet summoning information
  warlock_base.fel_firebolt_2 = find_spell( 334591 ); // 20% cost reduction for Wild Imps
  warlock_base.demonic_core = find_specialization_spell( "Demonic Core" ); // Should be ID 267102
  warlock_base.demonic_core_buff = find_spell( 264173 ); // Buff data
  warlock_base.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY ); // Should be ID 77219
  warlock_base.demonology_warlock = find_specialization_spell( "Demonology Warlock", WARLOCK_DEMONOLOGY ); // Should be ID 137044

  // Destruction
  warlock_base.immolate = find_class_spell( "Immolate" ); // Should be ID 348, contains direct damage and cast data
  warlock_base.immolate_dot = find_spell( 157736 ); // DoT data
  warlock_base.incinerate = find_class_spell( "Incinerate" ); // Should be ID 29722
  warlock_base.incinerate_energize = find_spell( 244670 ); // Used for resource gain information
  warlock_base.chaotic_energies = find_mastery_spell( WARLOCK_DESTRUCTION ); // Should be ID 77220
  warlock_base.destruction_warlock = find_specialization_spell( "Destruction Warlock", WARLOCK_DESTRUCTION ); // Should be ID 137046

  warlock_t::init_spells_affliction();
  warlock_t::init_spells_demonology();
  warlock_t::init_spells_destruction();

  // Talents
  talents.seed_of_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Seed of Corruption" ); // Should be ID 27243
  talents.seed_of_corruption_aoe = find_spell( 27285 ); // Explosion damage

  talents.grimoire_of_sacrifice = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire of Sacrifice" ); // Aff/Destro only. Should be ID 108503
  talents.grimoire_of_sacrifice_buff = find_spell( 196099 ); // Buff data and RPPM
  talents.grimoire_of_sacrifice_proc = find_spell( 196100 ); // Damage data

  talents.grand_warlocks_design = find_talent_spell( talent_tree::SPECIALIZATION, "Grand Warlock's Design" ); // All 3 specs. Should be ID 387084

  talents.havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Havoc" ); // Should be spell 80240
  talents.havoc_debuff = find_spell( 80240 );

  talents.demonic_inspiration = find_talent_spell( talent_tree::CLASS, "Demonic Inspiration" ); // Should be ID 386858

  talents.wrathful_minion = find_talent_spell( talent_tree::CLASS, "Wrathful Minion" ); // Should be ID 386864

  talents.grimoire_of_synergy = find_talent_spell( talent_tree::CLASS, "Grimoire of Synergy" ); // Should be ID 171975
  talents.demonic_synergy = find_spell( 171982 );

  talents.socrethars_guile   = find_talent_spell( talent_tree::CLASS, "Socrethar's Guile" ); // Should be ID 405936 //405955
  talents.sargerei_technique = find_talent_spell( talent_tree::CLASS, "Sargerei Technique" );  // Should be ID 405955

  talents.soul_conduit = find_talent_spell( talent_tree::CLASS, "Soul Conduit" ); // Should be ID 215941

  talents.grim_feast = find_talent_spell( talent_tree::CLASS, "Grim Feast" ); // Should be ID 386689

  talents.summon_soulkeeper = find_talent_spell( talent_tree::CLASS, "Summon Soulkeeper" ); // Should be ID 386244
  talents.summon_soulkeeper_aoe = find_spell( 386256 );
  talents.tormented_soul_buff = find_spell( 386251 );
  talents.soul_combustion = find_spell( 386265 );

  talents.inquisitors_gaze = find_talent_spell( talent_tree::CLASS, "Inquisitor's Gaze" ); // Should be ID 386344
  talents.inquisitors_gaze_buff = find_spell( 388068 );
  talents.fel_barrage = find_spell( 388070 );

  talents.soulburn = find_talent_spell( talent_tree::CLASS, "Soulburn" ); // Should be ID 385899
  talents.soulburn_buff = find_spell( 387626 );
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

  procs.one_shard_hog = get_proc( "one_shard_hog" );
  procs.two_shard_hog = get_proc( "two_shard_hog" );
  procs.three_shard_hog = get_proc( "three_shard_hog" );
  procs.portal_summon = get_proc( "portal_summon" );
  procs.demonic_calling = get_proc( "demonic_calling" );
  procs.soul_conduit = get_proc( "soul_conduit" );
  procs.carnivorous_stalkers = get_proc( "carnivorous_stalkers" );
  procs.ritual_of_ruin = get_proc( "ritual_of_ruin" );
  procs.avatar_of_destruction = get_proc( "avatar_of_destruction" );
  procs.mayhem = get_proc( "mayhem" );
  procs.conflagration_of_chaos_cf = get_proc( "conflagration_of_chaos_cf" );
  procs.conflagration_of_chaos_sb = get_proc( "conflagration_of_chaos_sb" );
  procs.demonic_inspiration = get_proc( "demonic_inspiration" );
  procs.wrathful_minion = get_proc( "wrathful_minion" );
  procs.inquisitors_gaze = get_proc( "inquisitors_gaze" );
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

  if ( talents.grimoire_of_synergy->ok() )
  {
    auto const syn_effect = new special_effect_t( this );
    syn_effect->name_str = "demonic_synergy_effect";
    syn_effect->spell_id = talents.grimoire_of_synergy->id();
    special_effects.push_back( syn_effect );

    auto cb = new warlock::actions::demonic_synergy_callback_t( this, *syn_effect );

    cb->initialize();
  }

  if ( talents.inquisitors_gaze->ok() )
  {
    auto const gaze_effect = new special_effect_t( this );
    gaze_effect->name_str = "inquisitors_gaze_effect";
    gaze_effect->spell_id = talents.inquisitors_gaze->id();
    special_effects.push_back( gaze_effect );

    auto cb = new warlock::actions::inquisitors_gaze_callback_t( this, *gaze_effect );

    cb->initialize();
  }
}

void warlock_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.summon_soulkeeper->ok() )
    buffs.tormented_soul_generator->trigger();

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
  corruption_accumulator             = rng().range( 0.0, 0.99 );
  cdf_accumulator                    = rng().range( 0.0, 0.99 );
  incinerate_last_target_count       = 0;
  wild_imp_spawns.clear();
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
  add_option( opt_bool( "disable_felstorm", disable_auto_felstorm ) );
  add_option( opt_bool( "use_pet_stat_update_delay", use_pet_stat_update_delay ) );
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
      continue;

    td->dots_agony->adjust_duration( darkglare_extension );
    td->dots_corruption->adjust_duration( darkglare_extension );
    td->dots_siphon_life->adjust_duration( darkglare_extension );
    td->dots_phantom_singularity->adjust_duration( darkglare_extension );
    td->dots_vile_taint->adjust_duration( darkglare_extension );
    td->dots_unstable_affliction->adjust_duration( darkglare_extension );
    td->dots_soul_rot->adjust_duration( darkglare_extension );
  }
}

int warlock_t::active_demon_count() const
{
  int count = 0;

  for ( auto& pet : this->pet_list )
  {
    auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );
    
    if ( lock_pet == nullptr )
      continue;
    if ( lock_pet->is_sleeping() )
      continue;
    
    count++;
  }

  return count;
}

void warlock_t::expendables_trigger_helper( warlock_pet_t* source )
{
  for ( auto& pet : this->pet_list )
  {
    auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

    if ( lock_pet == nullptr )
      continue;
    if ( lock_pet->is_sleeping() )
      continue;

    if ( lock_pet == source )
      continue;

    // Pit Lord is not affected by The Expendables
    if ( lock_pet->pet_type == PET_PIT_LORD )
      continue;

    lock_pet->buffs.the_expendables->trigger();
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
    case VERSION_10_1_5:
      return !( version_10_1_5_data == spell_data_t::not_found() );
    case VERSION_10_1_0:
    case VERSION_10_0_7:
    case VERSION_10_0_5:
    case VERSION_10_0_0:
    case VERSION_ANY:
      return true;
  }

  return false;
}

action_t* warlock_t::pass_corruption_action( warlock_t* p )
{
  return debug_cast<action_t*>( new actions::corruption_t( p, "", true ) );
}

action_t* warlock_t::pass_soul_rot_action( warlock_t* p )
{
  return debug_cast<action_t*>( new actions::soul_rot_t( p, "", true ) );
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
    if ( disable_auto_felstorm )
      profile_str += "disable_felstorm=" + util::to_string( disable_auto_felstorm );
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  auto* p = debug_cast<warlock_t*>( source );

  initial_soul_shards  = p->initial_soul_shards;
  default_pet          = p->default_pet;
  disable_auto_felstorm = p->disable_auto_felstorm;
}

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
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

std::unique_ptr<expr_t> warlock_t::create_pet_expression( util::string_view name_str )
{
  if ( name_str == "last_cast_imps" )
  {
    return make_fn_expr( "last_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] < 32;
      } );
    } );
  }
  else if ( name_str == "two_cast_imps" )
  {
    return make_fn_expr( "two_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] < 48;
      } );
    } );
  }
  else if ( name_str == "igb_ratio" )
  {
    return make_fn_expr( "igb_ratio", [ this ]() {
      auto igb_count = warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->buffs.imp_gang_boss->check();
        } );

      return igb_count / as<double>( buffs.wild_imps->stack() );
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
  else if ( name_str == "igb_ratio" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "havoc_active" )
  {
    return make_fn_expr( name_str, [ this ] { return havoc_target != nullptr; } );
  }
  else if ( name_str == "havoc_remains" )
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

  action.apply_affecting_aura( talents.socrethars_guile );
  action.apply_affecting_aura( talents.sargerei_technique );
  action.apply_affecting_aura( talents.dark_virtuosity );
  action.apply_affecting_aura( talents.kindled_malice );
  action.apply_affecting_aura( talents.xavius_gambit ); // TOCHECK: Should this just go in Unstable Affliction struct for clarity?

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

  void register_hotfixes() const override
  {
    hotfix::register_spell( "Warlock", "2023-01-08", "Manually set secondary Malefic Rapture level requirement", 324540 )
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
    infernals( "infernal", w ),
    blasphemy( "blasphemy", w ),
    darkglares( "darkglare", w ),
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
    prince_malchezaar( "prince_malchezaar", w ),
    pit_lords( "pit_lord", w )
{
}
}  // namespace warlock

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}

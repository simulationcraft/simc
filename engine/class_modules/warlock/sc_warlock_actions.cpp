#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

#include "sc_warlock_pets.hpp"
#include "util/util.hpp"
#include "class_modules/apl/warlock.hpp"

#include <queue>

namespace warlock
{
namespace actions
{
  struct warlock_spell_t : public spell_t
  {
    struct affected_by_t
    {
      // Class

      // Affliction
      bool potent_afflictions_td = false;
      bool potent_afflictions_dd = false;
      bool dread_touch = false;
      bool wrath_of_consumption = false;
      bool haunted_soul  = false;
      bool creeping_death = false;

      // Demonology
      bool master_demonologist_dd = false;
      bool houndmasters = false;

      // Destruction
      bool chaotic_energies = false;
      bool havoc = false;
      bool roaring_blaze = false;
      bool chaos_incarnate = false;
      bool umbrafire_embers_dd = false;
      bool umbrafire_embers_td = false;
    } affected_by;

    struct triggers_t
    {
      // Class

      // Affliction

      // Demonology

      // Destruction
      bool shadow_invocation_direct = false;
      bool shadow_invocation_tick = false;
    } triggers;

    warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s ),
      affected_by()
    {
      may_crit = true;
      tick_may_crit = true;
      weapon_multiplier = 0.0;

      affected_by.potent_afflictions_td = data().affected_by( p->warlock_base.potent_afflictions->effectN( 1 ) );
      affected_by.potent_afflictions_dd = data().affected_by( p->warlock_base.potent_afflictions->effectN( 2 ) );
      affected_by.dread_touch = data().affected_by( p->talents.dread_touch_debuff->effectN( 1 ) );
      affected_by.wrath_of_consumption = data().affected_by( p->talents.wrath_of_consumption_buff->effectN( 1 ) );
      affected_by.haunted_soul = data().affected_by( p->talents.haunted_soul_buff->effectN( 1 ) );
      affected_by.creeping_death = data().affected_by( p->talents.creeping_death->effectN( 1 ) );

      affected_by.master_demonologist_dd = data().affected_by( p->warlock_base.master_demonologist->effectN( 2 ) );
      affected_by.houndmasters = data().affected_by( p->talents.the_houndmasters_stratagem_debuff->effectN( 1 ) );

      affected_by.roaring_blaze = data().affected_by( p->talents.conflagrate_debuff->effectN( 1 ) );
      affected_by.umbrafire_embers_dd = data().affected_by( p->tier.umbrafire_embers->effectN( 1 ) );
      affected_by.umbrafire_embers_td = data().affected_by( p->tier.umbrafire_embers->effectN( 2 ) );
    }

    warlock_t* p()
    { return static_cast<warlock_t*>( player ); }
    
    const warlock_t* p() const
    { return static_cast<warlock_t*>( player ); }

    warlock_td_t* td( player_t* t )
    { return p()->get_target_data( t ); }

    const warlock_td_t* td( player_t* t ) const
    { return p()->get_target_data( t ); }

    void reset() override
    { spell_t::reset(); }

    void consume_resource() override
    {
      spell_t::consume_resource();

      if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
      {
        int shards_used = as<int>( cost() );
        int base_shards = as<int>( base_cost() ); // Power Overwhelming is ignoring any cost changes

        if ( p()->talents.soul_conduit->ok() )
        {
          // Soul Conduit events are delayed slightly (100 ms) in sims to avoid instantaneous reactions
          // TODO: Migrate sc_event_t to the actions.cpp file
          make_event<warlock::actions::sc_event_t>( *p()->sim, p(), as<int>( last_resource_cost ) );
        }

        if ( p()->buffs.rain_of_chaos->check() && shards_used > 0 )
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

        if ( p()->talents.ritual_of_ruin.ok() && shards_used > 0 )
        {
          int overflow = p()->buffs.impending_ruin->check() + shards_used - p()->buffs.impending_ruin->max_stack();
          p()->buffs.impending_ruin->trigger( shards_used ); // Stack change callback switches Impending Ruin to Ritual of Ruin if max stacks reached
          if ( overflow > 0 )
          {
            make_event( sim, 1_ms, [ this, overflow ] { p()->buffs.impending_ruin->trigger( overflow ); } );
          }
        }

        if ( p()->talents.power_overwhelming->ok() && base_shards > 0 )
        {
          p()->buffs.power_overwhelming->trigger( base_shards );
        }

        // 2022-10-17: Spell data is missing the % chance!
        // Need to test further, but chance appears independent of shard cost
        // Also procs even if the cast is free due to other effects
        if ( base_shards > 0 && p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T29, B2 ) && rng().roll( 0.2 ) )
        {
          p()->buffs.chaos_maelstrom->trigger();
          p()->procs.chaos_maelstrom->occur();
        }
      }
    }

    void execute() override
    {
      spell_t::execute();

      if ( p()->talents.rolling_havoc.ok() && use_havoc() )
      {
        p()->buffs.rolling_havoc->trigger();
      }
    }

    void impact( action_state_t* s ) override
    {
      spell_t::impact( s );

      if ( p()->talents.reverse_entropy.ok() )
      {
        if ( p()->buffs.reverse_entropy->trigger() )
        {
          p()->procs.reverse_entropy->occur();
        }
      }

      if ( affected_by.havoc && p()->talents.mayhem.ok() )
      {
        // Havoc debuff has an ICD, so it is safe to attempt a trigger
        auto tl = target_list();
        auto n = available_targets( tl );

        if ( n > 1u )
        {
          player_t* trigger_target = tl.at( 1u + rng().range( n - 1u ) );
          if ( td( trigger_target )->debuffs_havoc->trigger() )
          {
            p()->procs.mayhem->occur();
          }
        }
      }

      if ( p()->talents.shadow_invocation->ok() && triggers.shadow_invocation_direct && rng().roll( p()->shadow_invocation_proc_chance ) )
      {
        p()->proc_actions.bilescourge_bombers_proc->execute_on_target( s->target );
        p()->procs.shadow_invocation->occur();
      }
    }

    void tick( dot_t* d ) override
    {
      spell_t::tick( d );

      if ( p()->talents.reverse_entropy.ok() )
      {
        if ( p()->buffs.reverse_entropy->trigger() )
        {
          p()->procs.reverse_entropy->occur();
        }
      }

      if ( p()->talents.shadow_invocation.ok() && triggers.shadow_invocation_tick && rng().roll( p()->shadow_invocation_proc_chance ) )
      {
        p()->proc_actions.bilescourge_bombers_proc->execute_on_target( d->target );
        p()->procs.shadow_invocation->occur();
      }
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = spell_t::composite_target_multiplier( t );

      if ( p()->talents.dread_touch.ok() && affected_by.dread_touch && td( t )->debuffs_dread_touch->check() )
      {
        m *= 1.0 + td( t )->debuffs_dread_touch->check_stack_value();
      }

      if ( p()->talents.the_houndmasters_stratagem.ok() && affected_by.houndmasters )
      {
        m *= 1.0 + td( t )->debuffs_the_houndmasters_stratagem->check_value();
      }

      if ( p()->talents.roaring_blaze.ok() && affected_by.roaring_blaze && td( t )->debuffs_conflagrate->check() )
      {
        m *= 1.0 + td( t )->debuffs_conflagrate->check_value();
      }

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B4 )
        && ( affected_by.umbrafire_embers_dd || affected_by.umbrafire_embers_td )
        && p()->buffs.umbrafire_embers->check() )
      {
        m *= 1.0 + p()->buffs.umbrafire_embers->check_stack_value();
      }

      return m;
    }

    double action_multiplier() const override
    {
      double m = spell_t::action_multiplier();

      m *= 1.0 + p()->buffs.demonic_synergy->check_stack_value();

      if ( demonology() && affected_by.master_demonologist_dd )
      {
        m *= 1.0 + p()->cache.mastery_value();
      }

      if ( destruction() && affected_by.chaotic_energies )
      {
        double destro_mastery_value = p()->cache.mastery_value() / 2.0;
        double chaotic_energies_rng = rng().range( 0, destro_mastery_value );

        if ( affected_by.chaos_incarnate )
          chaotic_energies_rng = destro_mastery_value;

        m *= 1.0 + chaotic_energies_rng + destro_mastery_value;
      }
      
      return m;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( affliction() && affected_by.potent_afflictions_dd )
      {
        m *= 1.0 + p()->cache.mastery_value();
      }

      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_ta_multiplier( s );

      if ( affliction() && affected_by.potent_afflictions_td )
      {
        m *= 1.0 + p()->cache.mastery_value();
      }

      if ( p()->talents.wrath_of_consumption.ok() && affected_by.wrath_of_consumption && p()->buffs.wrath_of_consumption->check() )
      {
        m *= 1.0 + p()->buffs.wrath_of_consumption->check_stack_value();
      }

      if ( p()->talents.haunted_soul.ok() && affected_by.haunted_soul && p()->buffs.haunted_soul->check() )
      {
        m *= 1.0 + p()->buffs.haunted_soul->check_stack_value();
      }

      return m;
    }

    double composite_crit_damage_bonus_multiplier() const override
    {
      double m = spell_t::composite_crit_damage_bonus_multiplier();

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T29, B4 ) )
      {
        m += p()->sets->set( WARLOCK_DESTRUCTION, T29, B4 )->effectN( 1 ).percent();
      }

      return m;
    }

    void extend_dot( dot_t* dot, timespan_t extend_duration )
    {
      if ( dot->is_ticking() )
      {
        // TOCHECK: Do we always cap out at pandemic amount (+50%)?
        dot->adjust_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
      }
    }

    bool use_havoc() const
    {
      // Ensure we do not try to hit the same target twice.
      return affected_by.havoc && p()->havoc_target && p()->havoc_target != target;
    }

    // We need to ensure that the target cache is invalidated, which sometimes does not take 
    // place in select_target() due to other methods we have overridden involving Havoc
    bool select_target() override
    {
      auto saved_target = target;

      bool passed = spell_t::select_target();

      if ( passed && target != saved_target && use_havoc() )
        target_cache.is_valid = false;

      return passed;
    }

    int n_targets() const override
    {
      if ( destruction() && use_havoc() )
      {
        assert( spell_t::n_targets == 0 );
        return 2;
      }
      else
      {
        return spell_t::n_targets();
      }
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      spell_t::available_targets( tl );

      // Check target list size to prevent some silly scenarios where Havoc target
      // is the only target in the list.
      if ( destruction() && tl.size() > 1 && use_havoc())
      {
        // We need to make sure that the Havoc target ends up second in the target list,
        // so that Havoc spells can pick it up correctly.
        auto it = range::find(tl, p()->havoc_target);
        if (it != tl.end())
        {
          tl.erase(it);
          tl.insert(tl.begin() + 1, p()->havoc_target);
        }
      }

      return tl.size();
    }

    void init() override
    {
      spell_t::init();

      if ( destruction() && affected_by.havoc )
      {
        base_aoe_multiplier *= p()->talents.havoc_debuff->effectN( 1 ).percent();
        p()->havoc_spells.push_back( this );
      }

      if ( p()->talents.creeping_death.ok() && affected_by.creeping_death )
      {
        base_tick_time *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
      }
    }

    bool affliction() const
    { return p()->specialization() == WARLOCK_AFFLICTION; }

    bool demonology() const
    { return p()->specialization() == WARLOCK_DEMONOLOGY; }

    bool destruction() const
    { return p()->specialization() == WARLOCK_DESTRUCTION; }
  };

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
      { return 0.0; }

      double action_multiplier() const override
      {
        double m = warlock_spell_t::action_multiplier();

        if ( p()->talents.inevitable_demise.ok() && p()->buffs.inevitable_demise->check() )
        {
          m *= 1.0 + p()->buffs.inevitable_demise->check_stack_value();
        }

        return m;
      }

      timespan_t composite_dot_duration( const action_state_t* s ) const override
      {
        // Model this like channel behavior - we can't actually set it as a channel without breaking things
        return dot_duration * ( tick_time( s ) / base_tick_time );
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

      triggers.shadow_invocation_tick = true;
    }

    void execute() override
    {
      if ( p()->talents.inevitable_demise.ok() && p()->buffs.inevitable_demise->check() > 0 )
      {
        if ( p()->buffs.drain_life->check() )
        {
          p()->buffs.inevitable_demise->expire();
        }
      }

      warlock_spell_t::execute();

      p()->buffs.drain_life->trigger();

      if ( p()->talents.soul_rot.ok() && p()->buffs.soul_rot->check() )
      {
        const auto& tl = target_list();

        for ( auto& t : tl )
        {
          // Don't apply AoE version to primary target
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

    double cost_per_tick( resource_e r ) const override
    {
      if ( r == RESOURCE_MANA && p()->buffs.soul_rot->check() )
        return 0.0;

      auto c = warlock_spell_t::cost_per_tick( r );

      if ( r == RESOURCE_MANA && p()->buffs.inevitable_demise->check() )
      {
        c *= 1.0 + p()->talents.inevitable_demise_buff->effectN( 3 ).percent() * p()->buffs.inevitable_demise->check();
      }

      return c;
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      if ( p()->talents.inevitable_demise.ok() && p()->buffs.inevitable_demise->check() )
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

  struct corruption_t : public warlock_spell_t
  {
    struct corruption_dot_t : public warlock_spell_t
    {
      corruption_dot_t( warlock_t* p ) : warlock_spell_t( "Corruption", p, p->warlock_base.corruption->effectN( 1 ).trigger() )
      {
        tick_zero = false;
        background = dual = true;

        if ( p->talents.absolute_corruption.ok() )
        {
          dot_duration = sim->expected_iteration_time > 0_ms
            ? 2 * sim->expected_iteration_time
            : 2 * sim->max_time * ( 1.0 + sim->vary_combat_length ); // "Infinite" duration
          base_td_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent(); // 2022-09-25: Only tick damage is affected
        }

        triggers.shadow_invocation_tick = true;
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( result_is_hit( d->state->result ) )
        {
          if ( p()->talents.nightfall->ok() )
          {
            // Blizzard did not publicly release how nightfall was changed.
            // We determined this is the probable functionality copied from Agony by first confirming the
            // DR formula was the same and then confirming that you can get procs on 1st tick.
            // The procs also have a regularity that suggest it does not use a proc chance or rppm.
            // Last checked 09-28-2020.
            double increment_max = 0.13;

            double active_corruptions = p()->get_active_dots( d );
            increment_max *= std::pow( active_corruptions, -2.0 / 3.0 );

            p()->corruption_accumulator += rng().range( 0.0, increment_max );

            if ( p()->corruption_accumulator >= 1 )
            {
              p()->procs.nightfall->occur();
              p()->buffs.nightfall->trigger();
              p()->corruption_accumulator -= 1.0;
            }
          }
        }
      }

      double composite_ta_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_ta_multiplier( s );

        if ( p()->talents.sacrolashs_dark_strike.ok() )
        {
          m *= 1.0 + p()->talents.sacrolashs_dark_strike->effectN( 1 ).percent();
        }

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
        spell_power_mod.direct = data().effectN( 3 ).sp_coeff();
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

    dot_t* get_dot( player_t* t ) override
    { return periodic->get_dot( t ); }
  };

  struct shadow_bolt_t : public warlock_spell_t
  {
    shadow_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Shadow Bolt", p, p->talents.drain_soul_dot->ok() ? spell_data_t::not_found() : p->warlock_base.shadow_bolt )
    {
      parse_options( options_str );

      triggers.shadow_invocation_direct = true;

      if ( demonology() )
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
          // TODO: Migrate crescendo_check
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
      {
        m *= 1.0 + p()->talents.nightfall_buff->effectN( 2 ).percent();
      }

      if ( p()->talents.sacrificed_souls.ok() )
      {
        // TODO: Migrate active_demon_count()
        m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_demon_count();
      }

      if ( p()->talents.stolen_power.ok() && p()->buffs.stolen_power_final->check() )
      {
        m *= 1.0 + p()->talents.stolen_power_final_buff->effectN( 1 ).percent();
      }

      return m;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( t );

      if ( p()->talents.withering_bolt.ok() )
      {
        m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( (int)( p()->talents.withering_bolt->effectN( 2 ).base_value() ), p()->get_target_data( t )->count_affliction_dots() );
      }

      return m;
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
  };

  struct seed_of_corruption_t : public warlock_spell_t
  {
    struct seed_of_corruption_aoe_t : public warlock_spell_t
    {
      corruption_t* corruption;
      bool cruel_epiphany;
      bool umbrafire_kindling;
      doom_blossom_t* doom_blossom;

      seed_of_corruption_aoe_t( warlock_t* p )
        : warlock_spell_t( "Seed of Corruption (AoE)", p, p->talents.seed_of_corruption_aoe ),
        corruption( new corruption_t( p, "", true ) ),
        cruel_epiphany( false ),
        umbrafire_kindling( false ),
        doom_blossom( new doom_blossom_t( p ) )
      {
        aoe = -1;
        background = dual = true;

        corruption->background = true;
        corruption->dual = true;
        corruption->base_costs[ RESOURCE_MANA ] = 0;

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

          if ( p()->talents.doom_blossom.ok() && tdata->dots_unstable_affliction->is_ticking() )
          {
            doom_blossom->execute_on_target( s->target );
            p()->procs.doom_blossom->occur();
          }

          // 2022-09-24 Agonizing Corruption does not apply Agony, only increments existing ones
          if ( p()->talents.agonizing_corruption.ok() && tdata->dots_agony->is_ticking() )
          {
            tdata->dots_agony->increment( (int)( p()->talents.agonizing_corruption->effectN( 1 ).base_value() ) ); 
          }
          
          corruption->execute_on_target( s->target );
        }
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_da_multiplier( s );

        if ( p()->buffs.cruel_epiphany->check() && cruel_epiphany )
        {
          m *= 1.0 + p()->buffs.cruel_epiphany->check_value();
        }

        if ( umbrafire_kindling )
        {
          m *= 1.0 + p()->tier.umbrafire_kindling->effectN( 2 ).percent();
        }

        return m;
      }
    };

    seed_of_corruption_aoe_t* explosion;
    seed_of_corruption_aoe_t* epiphany_explosion;
    seed_of_corruption_aoe_t* umbrafire_explosion;

    seed_of_corruption_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Seed of Corruption", p, p->talents.seed_of_corruption ),
      explosion( new seed_of_corruption_aoe_t( p ) ),
      epiphany_explosion( new seed_of_corruption_aoe_t( p ) ),
      umbrafire_explosion( new seed_of_corruption_aoe_t( p ) )
    {
      parse_options( options_str );
      may_crit = false;
      tick_zero = false;
      base_tick_time = dot_duration;
      hasted_ticks = false;

      add_child( explosion );

      epiphany_explosion->cruel_epiphany = true;
      add_child( epiphany_explosion );

      umbrafire_explosion->umbrafire_kindling = true;
      add_child( umbrafire_explosion );

      if ( p->talents.sow_the_seeds.ok() )
      {
        aoe = 1 + as<int>( p->talents.sow_the_seeds->effectN( 1 ).base_value() );
      }
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
      {
        range::erase_remove( tl, [ this ]( player_t* t ) {
          return ( td( t )->dots_seed_of_corruption->is_ticking() || has_travel_events_for( t ) );
        } );
      }

      return tl.size();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.cruel_epiphany->decrement();

      p()->buffs.umbrafire_kindling->decrement();
    }

    void impact( action_state_t* s ) override
    {
      if ( result_is_hit( s->result ) )
      {
        td( s->target )->soc_threshold = s->composite_spell_power() * p()->talents.seed_of_corruption->effectN( 1 ).percent();
      }

      warlock_spell_t::impact( s );

      if ( s->chain_target == 0 && p()->buffs.cruel_epiphany->check() )
      {
        td( s->target )->debuffs_cruel_epiphany->trigger();
      }

      if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T31, B4 ) )
      {
        td( s->target )->debuffs_umbrafire_kindling->trigger();
      }
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
      // Note: We're PROBABLY okay to do this as an if/else if on tier sets because you can't have two separate 4pc bonuses at once
      if ( td( d->target )->debuffs_cruel_epiphany->check() )
      {
        epiphany_explosion->set_target( d->target );
        epiphany_explosion->schedule_execute();
      }
      else if ( td( d->target )->debuffs_umbrafire_kindling->check() )
      {
        make_event( *sim, 0_ms, [this, t = d->target ] { umbrafire_explosion->execute_on_target( t ); } );
      }
      else
      {
        explosion->set_target( d->target );
        explosion->schedule_execute();
      }

      td( d->target )->debuffs_cruel_epiphany->expire();
      td( d->target )->debuffs_umbrafire_kindling->expire();

      warlock_spell_t::last_tick( d );
    }
  };

  struct soul_rot_t : public warlock_spell_t
  {
    soul_rot_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Soul Rot", p, p->talents.soul_rot )
    {
      parse_options( options_str );
      aoe = 1 + as<int>( p->talents.soul_rot->effectN( 3 ).base_value() );

      if ( p->sets->has_set_bonus( WARLOCK_AFFLICTION, T31, B2 ) )
        apply_affecting_aura( p->sets->set( WARLOCK_AFFLICTION, T31, B2 ) );

      if ( p->talents.souleaters_gluttony.ok() )
      {
        cooldown->duration += p->talents.souleaters_gluttony->effectN( 1 ).time_value();
      }
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

      if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T31, B4 ) )
      {
        p()->buffs.umbrafire_kindling->trigger();
      }
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.dark_harvest.ok() && aoe > 1 )
      {
        p()->buffs.dark_harvest_haste->trigger();
        p()->buffs.dark_harvest_crit->trigger();
      }
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

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

  struct doom_blossom_t : public warlock_spell_t
  {
    doom_blossom_t( warlock_t* p ) : warlock_spell_t( "Doom Blossom", p, p->talents.doom_blossom_proc )
    {
      background = dual = true;
      aoe = -1;
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
}
}
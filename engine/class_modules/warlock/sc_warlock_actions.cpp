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
}
}
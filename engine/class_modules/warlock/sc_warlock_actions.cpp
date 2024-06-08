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
      bool dread_touch;
      bool wrath_of_consumption;
      bool haunted_soul;

      // Demonology

      // Destruction
      bool havoc = false;
    } affected_by;

    warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s ),
      affected_by()
    {
      may_crit = true;
      tick_may_crit = true;
      weapon_multiplier = 0.0;

      affected_by.dread_touch = data().affected_by( p->talents.dread_touch_debuff->effectN( 1 ) );
      affected_by.wrath_of_consumption = data().affected_by( p->talents.wrath_of_consumption_buff->effectN( 1 ) );
      affected_by.haunted_soul = data().affected_by( p->talents.haunted_soul_buff->effectN( 1 ) );
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
        if ( p()->talents.soul_conduit->ok() )
        {
          // Soul Conduit events are delayed slightly (100 ms) in sims to avoid instantaneous reactions
          // TODO: Migrate sc_event_t to the actions.cpp file
          make_event<warlock::actions::sc_event_t>( *p()->sim, p(), as<int>( last_resource_cost ) );
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
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = spell_t::composite_target_multiplier( t );

      if ( p()->talents.dread_touch.ok() && affected_by.dread_touch && td( t )->debuffs_dread_touch->check() )
      {
        m *= 1.0 + td( t )->debuffs_dread_touch->check_stack_value();
      }

      return m;
    }

    double action_multiplier() const override
    {
      double m = spell_t::action_multiplier();

      m *= 1.0 + p()->buffs.demonic_synergy->check_stack_value();
      
      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_ta_multiplier( s );

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
      if ( p()->specialization() == WARLOCK_DESTRUCTION && use_havoc() )
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
      if ( p()->specialization() == WARLOCK_DESTRUCTION && tl.size() > 1 && use_havoc())
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

      if ( p()->specialization() == WARLOCK_DESTRUCTION && affected_by.havoc )
      {
        base_aoe_multiplier *= p()->talents.havoc_debuff->effectN( 1 ).percent();
        p()->havoc_spells.push_back( this );
      }
    }
  };
}
}
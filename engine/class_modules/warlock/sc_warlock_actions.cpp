#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

#include "sc_warlock_pets.hpp"
#include "util/util.hpp"

namespace warlock
{
using namespace helpers;

  struct warlock_spell_t : public spell_t
  {
    struct affected_by_t
    {
      // Class

      // Affliction
      bool potent_afflictions_td = false;
      bool potent_afflictions_dd = false;
      bool creeping_death = false;
      bool summoners_embrace_dd = false;
      bool summoners_embrace_td = false;
      bool malediction = false;
      bool contagion = false;
      bool deaths_embrace = false;
      bool umbral_lattice_dd = false;
      bool umbral_lattice_td = false;

      // Demonology
      bool master_demonologist_dd = false;
      bool sacrificed_souls = false;
      bool wicked_maw = false;
      bool soul_conduit_base_cost = false;
      bool demonic_brutality = false;

      // Destruction
      bool chaotic_energies = false;
      bool havoc = false;
      bool backdraft = false;
      bool roaring_blaze = false;
      bool ashen_remains = false;
      bool emberstorm_dd = false;
      bool emberstorm_td = false;
      bool devastation = false;
      bool ruin = false;
      bool chaos_incarnate = false;
      bool echo_of_the_azjaqir_dd = false;
      bool echo_of_the_azjaqir_td = false;

      // Diabolist
      bool touch_of_rancora = false;
      bool flames_of_xoroth_dd = false;
      bool flames_of_xoroth_td = false;

      // Hellcaller
      bool xalans_ferocity_dd = false;
      bool xalans_ferocity_td = false;
      bool xalans_ferocity_crit = false;
      bool xalans_cruelty_dd = false;
      bool xalans_cruelty_td = false;
      bool xalans_cruelty_crit = false;
    } affected_by;

    struct triggers_t
    {
      // Class

      // Affliction
      bool ravenous_afflictions = false;

      // Demonology
      bool shadow_invocation = false;

      // Destruction
      bool decimation = false;
      bool dimension_ripper = false;

      // Diabolist
      bool diabolic_ritual = false;
      bool demonic_art = false;
    } triggers;

    warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s ),
      affected_by(),
      triggers()
    {
      may_crit = true;
      tick_may_crit = true;
      weapon_multiplier = 0.0;

      affected_by.potent_afflictions_td = data().affected_by( p->warlock_base.potent_afflictions->effectN( 1 ) );
      affected_by.potent_afflictions_dd = data().affected_by( p->warlock_base.potent_afflictions->effectN( 2 ) );
      affected_by.creeping_death = data().affected_by( p->talents.creeping_death->effectN( 1 ) );
      affected_by.summoners_embrace_dd = data().affected_by( p->talents.summoners_embrace->effectN( 1 ) );
      affected_by.summoners_embrace_td = data().affected_by( p->talents.summoners_embrace->effectN( 3 ) );
      affected_by.malediction = data().affected_by( p->talents.malediction->effectN( 1 ) );
      affected_by.contagion = data().affected_by( p->talents.contagion->effectN( 1 ) );
      affected_by.umbral_lattice_dd = data().affected_by( p->tier.umbral_lattice->effectN( 1 ) );
      affected_by.umbral_lattice_td = data().affected_by( p->tier.umbral_lattice->effectN( 2 ) );

      affected_by.master_demonologist_dd = data().affected_by( p->warlock_base.master_demonologist->effectN( 2 ) );
      // TOCHECK: 2024-07-12 Despite the value of Effect 2 being 0 for Wicked Maw's debuff, the spells listed for it gain full value as if from Effect 1
      affected_by.wicked_maw = data().affected_by( p->talents.wicked_maw_debuff->effectN( 1 ) ) || data().affected_by( p->talents.wicked_maw_debuff->effectN( 2 ) );
      affected_by.demonic_brutality = data().affected_by( p->talents.demonic_brutality->effectN( 1 ) );

      affected_by.backdraft = data().affected_by( p->talents.backdraft_buff->effectN( 1 ) );
      affected_by.roaring_blaze = p->talents.roaring_blaze.ok() && data().affected_by( p->talents.conflagrate_debuff->effectN( 1 ) );
      affected_by.emberstorm_dd = data().affected_by( p->talents.emberstorm->effectN( 1 ) );
      affected_by.emberstorm_td = data().affected_by( p->talents.emberstorm->effectN( 3 ) );
      affected_by.devastation = data().affected_by( p->talents.devastation->effectN( 1 ) );
      affected_by.ruin = data().affected_by( p->talents.ruin->effectN( 1 ) );
      affected_by.echo_of_the_azjaqir_dd = data().affected_by( p->tier.echo_of_the_azjaqir->effectN( 1 ) );
      affected_by.echo_of_the_azjaqir_td = data().affected_by( p->tier.echo_of_the_azjaqir->effectN( 2 ) );

      affected_by.flames_of_xoroth_dd = data().affected_by( p->hero.flames_of_xoroth->effectN( 1 ) );
      affected_by.flames_of_xoroth_td = data().affected_by( p->hero.flames_of_xoroth->effectN( 2 ) );

      affected_by.xalans_ferocity_dd = data().affected_by( p->hero.xalans_ferocity->effectN( 1 ) );
      affected_by.xalans_ferocity_td = data().affected_by( p->hero.xalans_ferocity->effectN( 2 ) );
      affected_by.xalans_ferocity_crit = data().affected_by( p->hero.xalans_ferocity->effectN( 4 ) );
      affected_by.xalans_cruelty_dd = data().affected_by( p->hero.xalans_cruelty->effectN( 3 ) );
      affected_by.xalans_cruelty_td = data().affected_by( p->hero.xalans_cruelty->effectN( 4 ) );
      affected_by.xalans_cruelty_crit = data().affected_by( p->hero.xalans_cruelty->effectN( 1 ) );

      triggers.decimation = p->talents.decimation.ok();
    }

    warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s, util::string_view options_str )
      : warlock_spell_t( token, p, s )
    { parse_options( options_str ); }

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
        int shards_used = as<int>( last_resource_cost );
        int base_shards = as<int>( base_cost() ); // Power Overwhelming is ignoring any cost changes

        if ( p()->talents.soul_conduit.ok() )
        {
          // Soul Conduit events are delayed slightly (100 ms) in sims to avoid instantaneous reactions
          make_event<sc_event_t>( *p()->sim, p(), as<int>( affected_by.soul_conduit_base_cost ? base_shards : shards_used ) );
        }

        if ( p()->buffs.rain_of_chaos->check() && shards_used > 0 )
        {
          for ( int i = 0; i < shards_used; i++ )
          {
            if ( p()->rain_of_chaos_rng->trigger() )
            {
              auto spawned = p()->warlock_pet_list.infernals.spawn( p()->talents.summon_infernal_roc->duration() );
              for ( pets::destruction::infernal_t* s : spawned )
              {
                s->type = pets::destruction::infernal_t::infernal_type_e::RAIN;
              }
              p()->procs.rain_of_chaos->occur();
            }
          }
        }

        if ( p()->talents.ritual_of_ruin.ok() && shards_used > 0 )
        {
          int overflow = p()->buffs.impending_ruin->check() + shards_used - p()->buffs.impending_ruin->max_stack();
          p()->buffs.impending_ruin->trigger( shards_used ); // Stack change callback switches Impending Ruin to Ritual of Ruin if max stacks reached
          if ( overflow > 0 )
            make_event( sim, 1_ms, [ this, overflow ] { p()->buffs.impending_ruin->trigger( overflow ); } );
        }

        if ( p()->talents.power_overwhelming.ok() && base_shards > 0 )
          p()->buffs.power_overwhelming->trigger( base_shards );

        if ( diabolist() && triggers.diabolic_ritual )
        {
          timespan_t adjustment = -timespan_t::from_seconds( p()->hero.diabolic_ritual->effectN( 1 ).base_value() ) * shards_used;

          if ( demonology() && p()->hero.infernal_machine.ok() && p()->warlock_pet_list.demonic_tyrants.n_active_pets() > 0 )
            adjustment += -p()->hero.infernal_machine->effectN( 1 ).time_value();

          if ( destruction() && p()->hero.infernal_machine.ok() && p()->warlock_pet_list.infernals.n_active_pets() > 0 )
            adjustment += -p()->hero.infernal_machine->effectN( 1 ).time_value();

          switch( p()->diabolic_ritual )
          {
            case 0:
              if ( p()->buffs.ritual_overlord->check() )
              {
                p()->buffs.ritual_overlord->extend_duration( p(), adjustment );
              }
              else
              {
                p()->buffs.ritual_overlord->trigger();
                make_event( sim, 1_ms, [ this, adjustment ] { p()->buffs.ritual_overlord->extend_duration( p(), adjustment ); } );
              }
              break;
            case 1:
              if ( p()->buffs.ritual_mother->check() )
              {
                p()->buffs.ritual_mother->extend_duration( p(), adjustment );
              }
              else
              {
                p()->buffs.ritual_mother->trigger();
                make_event( sim, 1_ms, [ this, adjustment ] { p()->buffs.ritual_mother->extend_duration( p(), adjustment ); } );
              }
              break;
            case 2:
              if ( p()->buffs.ritual_pit_lord->check() )
              {
                p()->buffs.ritual_pit_lord->extend_duration( p(), adjustment );
              }
              else
              {
                p()->buffs.ritual_pit_lord->trigger();
                make_event( sim, 1_ms, [ this, adjustment ] { p()->buffs.ritual_pit_lord->extend_duration( p(), adjustment ); } );
              }
              break;
            default:
              break;
          }
        }

        if ( hellcaller() && base_shards > 0 && harmful && p()->hero.blackened_soul.ok() )
        {
          helpers::trigger_blackened_soul( p(), false );
        }
      }
    }

    void execute() override
    {
      spell_t::execute();

      if ( p()->talents.rolling_havoc.ok() && use_havoc() )
        p()->buffs.rolling_havoc->trigger();

      if ( diabolist() && triggers.demonic_art )
      {
        // Force event sequencing in a manner that lets Rain of Fire pick up the persistent multiplier for Touch of Rancora
        make_event( sim, 0_ms, [ this ] { p()->buffs.art_overlord->decrement(); } );
        make_event( sim, 0_ms, [ this ] { p()->buffs.art_mother->decrement(); } );
        make_event( sim, 0_ms, [ this ] { p()->buffs.art_pit_lord->decrement(); } );
      }
    }

    void impact( action_state_t* s ) override
    {
      spell_t::impact( s );

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

      if ( p()->talents.shadow_invocation.ok() && triggers.shadow_invocation && rng().roll( p()->rng_settings.shadow_invocation.setting_value ) )
      {
        p()->proc_actions.bilescourge_bombers_proc->execute_on_target( s->target );
        p()->procs.shadow_invocation->occur();
      }

      if ( destruction() && p()->talents.reverse_entropy.ok() )
      {
        if ( p()->buffs.reverse_entropy->trigger() )
          p()->procs.reverse_entropy->occur();
      }

      if ( destruction() && triggers.decimation && s->result == RESULT_CRIT && rng().roll( p()->rng_settings.decimation.setting_value ) )
      {
        p()->buffs.decimation->trigger();
        p()->cooldowns.soul_fire->reset( true );
        p()->procs.decimation->occur();
      }

      if ( destruction() && triggers.dimension_ripper && rng().roll( p()->rng_settings.dimension_ripper.setting_value ) )
      {
        if ( p()->talents.dimensional_rift.ok() )
        {
          p()->cooldowns.dimensional_rift->reset( true, 1 );
        }
        else
        {
          int rift = rng().range( 3 );

          switch ( rift )
          {
          case 0:
            p()->warlock_pet_list.shadow_rifts.spawn( p()->talents.shadowy_tear_summon->duration() );
            break;
          case 1:
            p()->warlock_pet_list.unstable_rifts.spawn( p()->talents.unstable_tear_summon->duration() );
            break;
          case 2:
            p()->warlock_pet_list.chaos_rifts.spawn( p()->talents.chaos_tear_summon->duration() );
            break;
          default:
            break;
          }
        }

        p()->procs.dimension_ripper->occur();
      }
    }

    void tick( dot_t* d ) override
    {
      spell_t::tick( d );

      if ( affliction() && triggers.ravenous_afflictions && d->state->result == RESULT_CRIT && p()->ravenous_afflictions_rng->trigger() )
      {
        p()->buffs.nightfall->trigger();
        p()->procs.ravenous_afflictions->occur();
      }

      if ( destruction() && p()->talents.reverse_entropy.ok() )
      {
        if ( p()->buffs.reverse_entropy->trigger() )
          p()->procs.reverse_entropy->occur();
      }
    }

    double composite_crit_chance() const override
    {
      double c = spell_t::composite_crit_chance();

      if ( affliction() && affected_by.malediction )
        c += p()->talents.malediction->effectN( 1 ).percent();

      if ( destruction() && affected_by.devastation )
        c += p()->talents.devastation->effectN( 1 ).percent();

      return c;
    }

    double composite_crit_chance_multiplier() const override
    {
      double m = spell_t::composite_crit_chance_multiplier();

      if ( hellcaller() && affected_by.xalans_ferocity_crit )
        m *= 1.0 + p()->hero.xalans_ferocity->effectN( 4 ).percent();

      if ( hellcaller() && affected_by.xalans_cruelty_crit )
        m *= 1.0 + p()->hero.xalans_cruelty->effectN( 1 ).percent();

      return m;
    }

    double composite_crit_damage_bonus_multiplier() const override
    {
      double m = spell_t::composite_crit_damage_bonus_multiplier();

      if ( affliction() && affected_by.contagion )
        m *= 1.0 + p()->talents.contagion->effectN( 1 ).percent();

      if ( demonology() && affected_by.demonic_brutality )
        m *= 1.0 + p()->talents.demonic_brutality->effectN( 1 ).percent();

      if ( destruction() && affected_by.ruin )
        m *= 1.0 + p()->talents.ruin->effectN( 1 ).percent();

      return m;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = spell_t::composite_target_multiplier( t );

      if ( demonology() && affected_by.wicked_maw )
        m *= 1.0 + td( t )->debuffs_wicked_maw->check_value();

      if ( destruction() && affected_by.roaring_blaze && p()->talents.roaring_blaze.ok() )
        m *= 1.0 + td( t )->debuffs_conflagrate->check_value();

      if ( destruction() && affected_by.ashen_remains && ( td( t )->dots_immolate->is_ticking() || td( t )->dots_wither->is_ticking() ) )
        m *= 1.0 + p()->talents.ashen_remains->effectN( 1 ).percent();

      return m;
    }

    double action_multiplier() const override
    {
      double m = spell_t::action_multiplier();

      if ( demonology() && affected_by.master_demonologist_dd )
        m *= 1.0 + p()->cache.mastery_value();

      if ( destruction() && affected_by.chaotic_energies )
      {
        double min_percentage = affected_by.chaos_incarnate ? p()->talents.chaos_incarnate->effectN( 1 ).percent() : 0.5;
        double chaotic_energies_rng = rng().range( min_percentage , 1.0 );

        if ( p()->normalize_destruction_mastery )
          chaotic_energies_rng = ( 1.0 + min_percentage ) / 2.0;

        m *= 1.0 + chaotic_energies_rng * p()->cache.mastery_value();
      }
      
      return m;
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_persistent_multiplier( s );

      if ( diabolist() && affected_by.touch_of_rancora )
      {
        if ( p()->buffs.art_overlord->check() )
          m *= 1.0 + p()->hero.touch_of_rancora->effectN( 1 ).percent();

        if ( p()->buffs.art_mother->check() )
          m *= 1.0 + p()->hero.touch_of_rancora->effectN( 1 ).percent();

        if ( p()->buffs.art_pit_lord->check() )
          m *= 1.0 + p()->hero.touch_of_rancora->effectN( 1 ).percent();
      }

      return m;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_da_multiplier( s );

      if ( affliction() && affected_by.potent_afflictions_dd )
        m *= 1.0 + p()->cache.mastery_value();

      if ( ( affliction() || destruction() ) && affected_by.summoners_embrace_dd )
        m *= 1.0 + p()->talents.summoners_embrace->effectN( 1 ).percent();

      if ( affliction() && affected_by.deaths_embrace && s->target->health_percentage() < p()->talents.deaths_embrace->effectN( 4 ).base_value() )
        m *= 1.0 + p()->talents.deaths_embrace->effectN( 3 ).percent();

      if ( affliction() && affected_by.umbral_lattice_dd && p()->buffs.umbral_lattice->check() )
        m *= 1.0 + p()->tier.umbral_lattice->effectN( 2 ).percent();

      if ( demonology() && affected_by.sacrificed_souls && p()->talents.sacrificed_souls.ok() )
        m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_demon_count();

      if ( destruction() && affected_by.emberstorm_dd && p()->talents.emberstorm.ok() )
        m *= 1.0 + p()->talents.emberstorm->effectN( 1 ).percent();

      if ( destruction() && affected_by.echo_of_the_azjaqir_dd && p()->buffs.echo_of_the_azjaqir->check() )
        m *= 1.0 + p()->tier.echo_of_the_azjaqir->effectN( 1 ).percent();

      if ( diabolist() && affected_by.flames_of_xoroth_dd && p()->hero.flames_of_xoroth.ok() )
        m *= 1.0 + p()->hero.flames_of_xoroth->effectN( 1 ).percent();

      if ( hellcaller() && affected_by.xalans_ferocity_dd && p()->hero.xalans_ferocity.ok() )
        m *= 1.0 + p()->hero.xalans_ferocity->effectN( 1 ).percent();

      if ( hellcaller() && affected_by.xalans_cruelty_dd && p()->hero.xalans_cruelty.ok() )
        m *= 1.0 + p()->hero.xalans_cruelty->effectN( 3 ).percent();

      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_ta_multiplier( s );

      if ( affliction() && affected_by.potent_afflictions_td )
        m *= 1.0 + p()->cache.mastery_value();

      if ( ( affliction() || destruction() ) && affected_by.summoners_embrace_td )
        m *= 1.0 + p()->talents.summoners_embrace->effectN( 3 ).percent();

      if ( affliction() && affected_by.deaths_embrace && s->target->health_percentage() < p()->talents.deaths_embrace->effectN( 4 ).base_value() )
        m *= 1.0 + p()->talents.deaths_embrace->effectN( 3 ).percent();

      if ( affliction() && affected_by.umbral_lattice_td && p()->buffs.umbral_lattice->check() )
        m *= 1.0 + p()->tier.umbral_lattice->effectN( 1 ).percent();

      if ( destruction() && affected_by.emberstorm_td && p()->talents.emberstorm.ok() )
        m *= 1.0 + p()->talents.emberstorm->effectN( 3 ).percent();

      if ( destruction() && affected_by.echo_of_the_azjaqir_td && p()->buffs.echo_of_the_azjaqir->check() )
        m *= 1.0 + p()->tier.echo_of_the_azjaqir->effectN( 2 ).percent();

      if ( diabolist() && affected_by.flames_of_xoroth_td && p()->hero.flames_of_xoroth.ok() )
        m *= 1.0 + p()->hero.flames_of_xoroth->effectN( 2 ).percent();

      if ( hellcaller() && affected_by.xalans_ferocity_td && p()->hero.xalans_ferocity.ok() )
        m *= 1.0 + p()->hero.xalans_ferocity->effectN( 2 ).percent();

      if ( hellcaller() && affected_by.xalans_cruelty_td && p()->hero.xalans_cruelty.ok() )
        m *= 1.0 + p()->hero.xalans_cruelty->effectN( 4 ).percent();

      return m;
    }

    double execute_time_pct_multiplier() const override
    {
      double m = spell_t::execute_time_pct_multiplier();

      if ( destruction() && affected_by.backdraft && p()->buffs.backdraft->check() )
        m *= 1.0 + p()->talents.backdraft_buff->effectN( 1 ).percent();

      if ( diabolist() && affected_by.touch_of_rancora )
      {
        if ( p()->buffs.art_overlord->check() )
          m *= 1.0 + p()->hero.touch_of_rancora->effectN( 2 ).percent();

        if ( p()->buffs.art_mother->check() )
          m *= 1.0 + p()->hero.touch_of_rancora->effectN( 2 ).percent();

        if ( p()->buffs.art_pit_lord->check() )
          m *= 1.0 + p()->hero.touch_of_rancora->effectN( 2 ).percent();
      }

      return m;
    }

    timespan_t gcd() const override
    {
      timespan_t t = spell_t::gcd();

      if ( !destruction() )
        return t;

      if ( t == 0_ms )
        return t;

      if ( affected_by.backdraft && p()->buffs.backdraft->check() )
        t *= 1.0 + p()->talents.backdraft_buff->effectN( 2 ).percent();

      if ( t < min_gcd )
        t = min_gcd;

      return t;
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
        assert( spell_t::n_targets() == 0 );
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
        base_aoe_multiplier *= p()->talents.havoc_debuff->effectN( 1 ).percent() + p()->hero.gloom_of_nathreza->effectN( 2 ).percent();
        p()->havoc_spells.push_back( this );
      }

      if ( affliction() && affected_by.creeping_death )
        base_tick_time *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
    }

    bool affliction() const
    { return p()->specialization() == WARLOCK_AFFLICTION; }

    bool demonology() const
    { return p()->specialization() == WARLOCK_DEMONOLOGY; }

    bool destruction() const
    { return p()->specialization() == WARLOCK_DESTRUCTION; }

    bool diabolist() const
    { return p()->hero.diabolic_ritual.ok(); }

    bool hellcaller() const
    { return p()->hero.wither.ok(); }

    bool soul_harvester() const
    { return p()->hero.demonic_soul.ok(); }

    bool active_2pc( set_bonus_type_e tier ) const
    { return p()->sets->has_set_bonus( p()->specialization(), tier, B2 ); }

    bool active_4pc( set_bonus_type_e tier ) const
    { return p()->sets->has_set_bonus( p()->specialization(), tier, B4 ); }
  };

  // Shared Class Actions Begin

  struct summon_pet_t : public warlock_spell_t
  {
    timespan_t summoning_duration;
    std::string pet_name;
    warlock_pet_t* pet;

  private:
    void _init_summon_pet_t()
    {
      util::tokenize( pet_name );
      harmful = false;

      if ( data().ok()
        && std::find( p()->pet_name_list.begin(), p()->pet_name_list.end(), pet_name ) == p()->pet_name_list.end() )
        p()->pet_name_list.push_back( pet_name );

      target = player;
    }

  public:
    summon_pet_t( util::string_view n, warlock_t* p, const spell_data_t* sd )
      : warlock_spell_t( n, p, sd ),
      summoning_duration( 0_ms ),
      pet_name( n ),
      pet( nullptr )
    { _init_summon_pet_t(); }

    summon_pet_t( util::string_view n, warlock_t* p, int id )
      : summon_pet_t( n, p, p->find_spell( id ) )
    { }

    summon_pet_t( util::string_view n, warlock_t* p )
      : summon_pet_t( n, p, p->find_class_spell( fmt::format( "Summon {}", n ) ) )
    { }

    void init_finished() override
    {
      pet = debug_cast<warlock_pet_t*>( player->find_pet( pet_name ) );

      warlock_spell_t::init_finished();
    }

    virtual void execute() override
    {
      pet->summon( summoning_duration );

      warlock_spell_t::execute();
    }

    bool ready() override
    {
      if ( !pet )
        return false;

      return warlock_spell_t::ready();
    }
  };

  struct summon_main_pet_t : public summon_pet_t
  {
    summon_main_pet_t( util::string_view n, warlock_t* p, int id )
      : summon_pet_t( n, p, id )
    { ignore_false_positive = true; }

    summon_main_pet_t( util::string_view n, warlock_t* p )
      : summon_pet_t( n, p )
    { ignore_false_positive = true; }

    void schedule_execute( action_state_t* s = nullptr ) override
    {
      summon_pet_t::schedule_execute( s );

      if ( p()->warlock_pet_list.active )
      {
        p()->warlock_pet_list.active->dismiss();
        p()->warlock_pet_list.active = nullptr;
      }
    }

    virtual bool ready() override
    {
      if ( p()->warlock_pet_list.active == pet )
        return false;

      return summon_pet_t::ready();
    }

    virtual void execute() override
    {
      summon_pet_t::execute();

      p()->warlock_pet_list.active = pet;

      if ( p()->buffs.grimoire_of_sacrifice->check() )
        p()->buffs.grimoire_of_sacrifice->expire();
    }
  };

  struct drain_life_t : public warlock_spell_t
  {

    // Note: Soul Rot (Affliction talent) turns Drain Life into a multi-target channeled spell. Nothing else in simc behaves this way and
    // we currently do not have core support for it. Applying this dot to the secondary targets should cover most of the behavior, although
    // it will be unable to handle the case where primary channel target dies (in-game, this appears to force-swap primary target to another
    // target currently affected by Drain Life if possible).
    struct drain_life_dot_t : public warlock_spell_t
    {
      drain_life_dot_t( warlock_t* p )
        : warlock_spell_t( "Drain Life (AoE)", p, p->warlock_base.drain_life )
      { dual = background = true; }

      double cost_per_tick( resource_e ) const override
      { return 0.0; }
    };
    
    drain_life_dot_t* aoe_dot;

    drain_life_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Drain Life", p, p->warlock_base.drain_life, options_str )
    {
      aoe_dot = new drain_life_dot_t( p );
      add_child( aoe_dot );

      channeled = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.soul_rot.ok() && p()->buffs.soul_rot->check() )
      {
        const auto& tl = target_list();

        for ( auto& t : tl )
        {
          // Don't apply AoE version to primary target
          if ( t == target )
            continue;

          if ( td( t )->dots_soul_rot->is_ticking() )
            aoe_dot->execute_on_target( t );
        }
      }

      p()->buffs.soulburn->expire();
    }

    double cost_per_tick( resource_e r ) const override
    {
      if ( r == RESOURCE_MANA && p()->buffs.soul_rot->check() )
        return 0.0;

      return warlock_spell_t::cost_per_tick( r );
    }

    void last_tick( dot_t* d ) override
    {
      bool early_cancel = d->remains() > 0_ms;

      warlock_spell_t::last_tick( d );

      // If this is the end of the channel, the AoE DoTs will expire correctly
      // Otherwise, we need to cancel them on the spot
      if ( p()->talents.soul_rot.ok() && early_cancel )
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
      corruption_dot_t( warlock_t* p )
        : warlock_spell_t( "Corruption", p, p->warlock_base.corruption->effectN( 1 ).trigger() )
      {
        tick_zero = false;
        background = dual = true;

        if ( p->talents.absolute_corruption.ok() )
        {
          dot_duration = sim->expected_iteration_time > 0_ms
            ? 2 * sim->expected_iteration_time
            : 2 * sim->max_time * ( 1.0 + sim->vary_combat_length ); // "Infinite" duration

          base_td_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent(); // 2024-07-06: Only tick damage is affected
        }

        base_td_multiplier *= 1.0 + p->talents.siphon_life->effectN( 3 ).percent();
        base_td_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 3 ).percent();
        base_td_multiplier *= 1.0 + p->talents.sacrolashs_dark_strike->effectN( 1 ).percent();

        if ( soul_harvester() && p->hero.sataiels_volition.ok() )
          base_tick_time *= 1.0 + p->hero.sataiels_volition->effectN( 1 ).percent();

        triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

        affected_by.deaths_embrace = p->talents.deaths_embrace.ok();
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( result_is_hit( d->state->result ) && p()->talents.nightfall.ok() )
          helpers::nightfall_updater( p(), d );
      }
    };

    corruption_dot_t* periodic;

    corruption_t( warlock_t* p, util::string_view options_str, bool seed_action )
      : warlock_spell_t( "Corruption (Direct)", p, !p->hero.wither.ok() ? p->warlock_base.corruption : spell_data_t::not_found(), options_str )
    {
      periodic = new corruption_dot_t( p );
      impact_action = periodic;
      add_child( periodic );

      spell_power_mod.direct = 0; // By default, Corruption does not deal instant damage

      if ( !seed_action && p->warlock_base.xavian_teachings->ok() )
      {
        spell_power_mod.direct = data().effectN( 3 ).sp_coeff();
        base_execute_time *= 1.0 + p->warlock_base.xavian_teachings->effectN( 1 ).percent();
      }

      base_dd_multiplier *= 1.0 + p->talents.siphon_life->effectN( 1 ).percent();
      base_dd_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 2 ).percent();

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();
    }

    dot_t* get_dot( player_t* t ) override
    { return periodic->get_dot( t ); }
  };

  struct shadow_bolt_volley_t : public warlock_spell_t
  {
    shadow_bolt_volley_t( warlock_t* p )
      : warlock_spell_t( "Shadow Bolt Volley", p, p->talents.shadow_bolt_volley )
    {
      background = dual = true;

      base_dd_multiplier *= 1.0 + p->talents.sargerei_technique->effectN( 1 ).percent();
      base_dd_multiplier *= 1.0 + p->talents.improved_shadow_bolt->effectN( 2 ).percent();
    }
  };

  struct shadow_bolt_t : public warlock_spell_t
  {
    shadow_bolt_volley_t* volley;

    shadow_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Shadow Bolt", p, p->talents.drain_soul.ok() ? spell_data_t::not_found() : p->warlock_base.shadow_bolt, options_str )
    {
      affected_by.sacrificed_souls = true;
      triggers.shadow_invocation = true;

      base_dd_multiplier *= 1.0 + p->talents.sargerei_technique->effectN( 1 ).percent();
      base_dd_multiplier *= 1.0 + p->talents.dark_virtuosity->effectN( 1 ).percent();
      base_dd_multiplier *= 1.0 + p->talents.improved_shadow_bolt->effectN( 2 ).percent();
      base_dd_multiplier *= 1.0 + p->talents.rune_of_shadows->effectN( 3 ).percent();

      if ( demonology() )
      {
        energize_type = action_energize::ON_CAST;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = 1.0;

        if ( active_2pc( TWW1 ) )
          base_dd_multiplier *= 1.0 + p->tier.hexflame_demo_2pc->effectN( 2 ).percent();
      }

      if ( p->talents.cunning_cruelty.ok() )
        volley = new shadow_bolt_volley_t( p );
    }

    bool ready() override
    {
      if ( diabolist() && p()->executing != this && p()->buffs.infernal_bolt->check() )
        return false;

      return warlock_spell_t::ready();
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      if ( p()->buffs.nightfall->check() )
        m *= 1.0 + p()->talents.nightfall_buff->effectN( 1 ).percent();

      m *= 1.0 + p()->talents.improved_shadow_bolt->effectN( 1 ).percent();

      m *= 1.0 + p()->talents.rune_of_shadows->effectN( 2 ).percent();

      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( time_to_execute == 0_ms && soul_harvester() && p()->buffs.nightfall->check() )
      {
        if ( p()->hero.wicked_reaping.ok() )
          p()->proc_actions.wicked_reaping->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.shared_fate.ok() )
          td( target )->debuffs_shared_fate->trigger();

        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && rng().roll( p()->rng_settings.feast_of_souls.setting_value ) )
          p()->feast_of_souls_gain();;
      }

      if ( time_to_execute == 0_ms )
        p()->buffs.nightfall->decrement();

      if ( p()->talents.demonic_calling.ok() )
        p()->buffs.demonic_calling->trigger();

      if ( demonology() && active_4pc( TWW1 ) && rng().roll( p()->rng_settings.empowered_legion_strike.setting_value ) )
      {
        auto active_pet = p()->warlock_pet_list.active;

        if ( active_pet && active_pet->pet_type == PET_FELGUARD )
        {
          active_pet->buffs.empowered_legion_strike->trigger();
          p()->procs.empowered_legion_strike->occur();
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        if ( p()->talents.shadow_embrace.ok() )
          td( s->target )->debuffs_shadow_embrace->trigger();

        if ( p()->talents.tormented_crescendo.ok() )
        {
          if ( crescendo_check( p() ) && rng().roll( p()->talents.tormented_crescendo->effectN( 1 ).percent() ) )
          {
            p()->procs.tormented_crescendo->occur();
            p()->buffs.tormented_crescendo->trigger();
          }
        }

        if ( p()->talents.cunning_cruelty.ok() && rng().roll( p()->rng_settings.cunning_cruelty_sb.setting_value ) )
        {
          p()->procs.shadow_bolt_volley->occur();
          volley->execute_on_target( s->target );
        }
      }
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      if ( time_to_execute == 0_ms && p()->buffs.nightfall->check() )
        m *= 1.0 + p()->talents.nightfall_buff->effectN( 2 ).percent() + p()->hero.necrolyte_teachings->effectN( 1 ).percent();

      if ( soul_harvester() && p()->hero.necrolyte_teachings.ok() )
        m *= 1.0 + p()->hero.necrolyte_teachings->effectN( 2 ).percent();

      return m;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( t );

      if ( p()->talents.withering_bolt.ok() )
        m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( (int)( p()->talents.withering_bolt->effectN( 2 ).base_value() ), p()->get_target_data( t )->count_affliction_dots() );

      return m;
    }
  };

  struct grimoire_of_sacrifice_t : public warlock_spell_t
  {
    grimoire_of_sacrifice_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Grimoire of Sacrifice", p, p->talents.grimoire_of_sacrifice, options_str )
    {
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

  struct grimoire_of_sacrifice_damage_t : public warlock_spell_t
  {
    grimoire_of_sacrifice_damage_t( warlock_t* p )
      : warlock_spell_t( "Grimoire of Sacrifice (Proc)", p, p->talents.grimoire_of_sacrifice_proc )
    {
      background = true;
      proc = true;

      triggers.decimation = false;

      base_dd_multiplier *= 1.0 + p->talents.demonic_inspiration->effectN( 2 ).percent();
      base_dd_multiplier *= 1.0 + p->talents.wrathful_minion->effectN( 2 ).percent();
    }
  };

  struct soulburn_t : public warlock_spell_t
  {
    soulburn_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Soulburn", p, p->talents.soulburn, options_str )
    {
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

  // Catchall action to trigger pet interrupt abilities via main APL.
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

  // Shared Class Actions End
  // Hellcaller Actions Begin

  struct wither_t : public warlock_spell_t
  {
    struct wither_dot_t : public warlock_spell_t
    {
      wither_dot_t( warlock_t* p )
        : warlock_spell_t( "Wither", p, p->hero.wither_dot )
      {
        background = dual = true;
        dot_ignore_stack = true;

        affected_by.chaotic_energies = destruction();

        base_td_multiplier *= 1.0 + p->hero.hatefury_rituals->effectN( 1 ).percent();
        base_td_multiplier *= 1.0 + p->hero.bleakheart_tactics->effectN( 2 ).percent();

        if ( destruction() )
        {
          dot_duration += p->talents.scalding_flames->effectN( 3 ).time_value();

          base_td_multiplier *= 1.0 + p->talents.socrethars_guile->effectN( 5 ).percent();
          base_td_multiplier *= 1.0 + p->talents.scalding_flames->effectN( 2 ).percent();
          base_td_multiplier *= 1.0 + p->hero.mark_of_xavius->effectN( 2 ).percent();
        }

        dot_duration *= 1.0 + p->hero.hatefury_rituals->effectN( 2 ).percent();

        if ( affliction() )
        {
          if ( p->talents.absolute_corruption.ok() )
          {
            dot_duration = sim->expected_iteration_time > 0_ms
              ? 2 * sim->expected_iteration_time
              : 2 * sim->max_time * ( 1.0 + sim->vary_combat_length );

            base_td_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent();
          }

          triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

          affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

          base_td_multiplier *= 1.0 + p->talents.siphon_life->effectN( 3 ).percent();
          base_td_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 3 ).percent();
          base_td_multiplier *= 1.0 + p->talents.sacrolashs_dark_strike->effectN( 1 ).percent();
        }
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( affliction() )
        {
          if ( result_is_hit( d->state->result ) && p()->talents.nightfall.ok() )
            helpers::nightfall_updater( p(), d );
        }

        if ( destruction() )
        {
          if ( d->state->result == RESULT_CRIT && rng().roll( p()->hero.wither_direct->effectN( 2 ).percent() ) )
            p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.wither_crits );

          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.wither );

          if ( p()->talents.flashpoint.ok() && d->state->target->health_percentage() >= p()->talents.flashpoint->effectN( 2 ).base_value() )
            p()->buffs.flashpoint->trigger();
        }

        if ( d->state->result == RESULT_CRIT && p()->hero.mark_of_perotharn.ok() && rng().roll( p()->rng_settings.mark_of_perotharn.setting_value ) )
        {
          d->increment( 1 );
          p()->procs.mark_of_perotharn->occur();
        }
      }

      double composite_crit_damage_bonus_multiplier() const override
      {
        double m = warlock_spell_t::composite_crit_damage_bonus_multiplier();

        m *= 1.0 + p()->hero.mark_of_perotharn->effectN( 1 ).percent();

        return m;
      }
    };

    wither_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Wither (Direct)", p, p->hero.wither.ok() ? p->hero.wither_direct : spell_data_t::not_found(), options_str )
    {
      affected_by.chaotic_energies = destruction();
      affected_by.havoc = destruction();

      impact_action = new wither_dot_t( p );
      add_child( impact_action );

      base_dd_multiplier *= 1.0 + p->hero.bleakheart_tactics->effectN( 1 ).percent();

      if ( destruction() )
      {
        triggers.decimation = p->talents.decimation.ok() && !dual;

        base_dd_multiplier *= 1.0 + p->talents.socrethars_guile->effectN( 3 ).percent();
        base_dd_multiplier *= 1.0 + p->talents.scalding_flames->effectN( 1 ).percent();
        base_dd_multiplier *= 1.0 + p->hero.mark_of_xavius->effectN( 4 ).percent();
      }

      if ( affliction() )
      {
        affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

        base_dd_multiplier *= 1.0 + p->talents.siphon_life->effectN( 1 ).percent();
        base_dd_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 2 ).percent();
      }
    }

    dot_t* get_dot( player_t* t ) override
    { return impact_action->get_dot( t ); }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( s->result == RESULT_CRIT && p()->hero.mark_of_perotharn.ok() && rng().roll( p()->rng_settings.mark_of_perotharn.setting_value ) )
      {
        td( s->target )->dots_wither->increment( 1 );
        p()->procs.mark_of_perotharn->occur();
      }
    }

    double composite_crit_damage_bonus_multiplier() const override
    {
      double m = warlock_spell_t::composite_crit_damage_bonus_multiplier();

      m *= 1.0 + p()->hero.mark_of_perotharn->effectN( 1 ).percent();

      return m;
    }
  };

  struct blackened_soul_t : public warlock_spell_t
  {
    blackened_soul_t( warlock_t* p )
      : warlock_spell_t( "Blackened Soul", p, p->hero.blackened_soul_dmg )
    {
      background = dual = true;

      affected_by.chaotic_energies = destruction();

      triggers.decimation = false;
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( target );

      if ( p()->hero.mark_of_xavius.ok() )
        m *= 1.0 + td( target )->dots_wither->current_stack() * p()->hero.mark_of_xavius->effectN( 3 ).percent();

      return m;
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      player_t* tar = s->target;

      if ( td( tar )->dots_wither->current_stack() > 1 )
        td( tar )->dots_wither->decrement( 1 );

      if ( td( tar )->dots_wither->current_stack() <= 1 )
        make_event( *sim, 0_ms, [ this, tar ] { td( tar )->debuffs_blackened_soul->expire(); } );

      if ( affliction() && p()->hero.seeds_of_their_demise.ok() && rng().roll( p()->rng_settings.seeds_of_their_demise.setting_value ) )
      {
        p()->buffs.tormented_crescendo->trigger();
        p()->procs.seeds_of_their_demise->occur();
      }

      if ( destruction() && p()->hero.seeds_of_their_demise.ok() && rng().roll( p()->rng_settings.seeds_of_their_demise.setting_value ) )
      {
        p()->buffs.flashpoint->trigger( 2 );
        p()->procs.seeds_of_their_demise->occur();
      }
    }
  };

  struct malevolence_damage_t : public warlock_spell_t
  {
    malevolence_damage_t( warlock_t* p )
      : warlock_spell_t( "Malevolence (Proc)", p, p->hero.malevolence_dmg )
    { background = dual = true; }
  };

  struct malevolence_t : public warlock_spell_t
  {
    malevolence_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Malevolence", p, p->hero.malevolence, options_str )
    {
      harmful = may_crit = false;
      trigger_gcd = p->hero.malevolence_buff->gcd();
      resource_current = RESOURCE_MANA;
      base_costs[ RESOURCE_MANA ] = p->hero.malevolence_buff->cost( POWER_MANA );
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.malevolence->trigger();

      helpers::trigger_blackened_soul( p(), true );
    }
  };

  // Hellcaller Actions End
  // Soul Harvester Actions Begin

  struct soul_anathema_t : public warlock_spell_t
  {
    soul_anathema_t( warlock_t* p )
      : warlock_spell_t( "Soul Anathema", p, p->hero.soul_anathema_dot )
    {
      background = dual = true;

      affected_by.potent_afflictions_td = affliction(); // Note: Technically Soul Anathema is on a separate effect from the others.
      affected_by.master_demonologist_dd = demonology();

      base_td_multiplier *= 1.0 + p->hero.quietus->effectN( 1 ).percent();
      base_tick_time *= 1.0 + p->hero.quietus->effectN( 2 ).percent();
      dot_duration *= 1.0 + p->hero.quietus->effectN( 3 ).percent();
    }
  };

  struct demonic_soul_t : public warlock_spell_t
  {
    bool demoniacs_fervor;

    demonic_soul_t( warlock_t* p )
      : warlock_spell_t( "Demonic Soul", p, p->hero.demonic_soul_dmg ),
      demoniacs_fervor( false )
    {
      background = dual = true;

      affected_by.master_demonologist_dd = demonology(); // Note: Technically Demonic Soul is on a separate effect from the others.

      base_dd_multiplier *= 1.0 + p->hero.wicked_reaping->effectN( 1 ).percent();

      if ( p->hero.soul_anathema.ok() )
        impact_action = new soul_anathema_t( p );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( demoniacs_fervor )
        m *= 1.0 + p()->hero.demoniacs_fervor->effectN( 1 ).percent();

      return m;
    }
  };

  struct shared_fate_t : public warlock_spell_t
  {
    shared_fate_t( warlock_t* p )
      : warlock_spell_t( "Shared Fate", p, p->hero.shared_fate_dmg )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = p->hero.shared_fate->effectN( 1 ).base_value();
    }
  };

  struct wicked_reaping_t : public warlock_spell_t
  {
    wicked_reaping_t( warlock_t* p )
      : warlock_spell_t( "Wicked Reaping", p, p->hero.wicked_reaping_dmg )
    {
      background = dual = true;

      affected_by.master_demonologist_dd = demonology();

      base_dd_multiplier *= 1.0 + p->hero.wicked_reaping->effectN( 1 ).percent();

      if ( demonology() )
        base_dd_multiplier *= p->hero.wicked_reaping->effectN( 2 ).percent();

      if ( p->hero.soul_anathema.ok() )
        impact_action = new soul_anathema_t( p );
    }
  };

  // Soul Harvester Actions End
  // Affliction Actions Begin

  struct malefic_rapture_t : public warlock_spell_t
  {
    struct malefic_touch_t : public warlock_spell_t
    {
      malefic_touch_t( warlock_t* p )
        : warlock_spell_t( "Malefic Touch", p, p->talents.malefic_touch_proc )
      {
        background = dual = true;

        base_dd_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 1 ).percent();
        base_dd_multiplier *= 1.0 + p->talents.improved_malefic_rapture->effectN( 1 ).percent();

        if ( active_2pc( TWW1 ) )
          base_dd_multiplier *= 1.0 + p->tier.hexflame_aff_2pc->effectN( 1 ).percent();
      }

      double composite_crit_chance_multiplier() const override
      {
        double m = warlock_spell_t::composite_crit_chance_multiplier();

        if ( active_2pc( TWW1 ) )
          m *= 1.0 + p()->tier.hexflame_aff_2pc->effectN( 2 ).percent();

        return m;
      }
    };

    struct malefic_rapture_damage_t : public warlock_spell_t
    {
      int target_count;
      malefic_touch_t* touch;

      malefic_rapture_damage_t( warlock_t* p )
        : warlock_spell_t ( "Malefic Rapture (hit)", p, p->warlock_base.malefic_rapture_dmg ),
        target_count( 0 )
      {
        background = dual = true;
        callbacks = false; // Individual hits have been observed to not proc trinkets like Psyche Shredder

        base_dd_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 1 ).percent();
        base_dd_multiplier *= 1.0 + p->talents.improved_malefic_rapture->effectN( 1 ).percent();

        if ( active_2pc( TWW1 ) )
          base_dd_multiplier *= 1.0 + p->tier.hexflame_aff_2pc->effectN( 1 ).percent();

        affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

        if ( p->talents.malefic_touch.ok() )
        {
          touch = new malefic_touch_t( p );
          add_child( touch );
        }
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_da_multiplier( s );

        m *= td( s->target )->count_affliction_dots();

        if ( p()->talents.focused_malignancy.ok() && td( s->target )->dots_unstable_affliction->is_ticking() )
          m *= 1.0 + p()->talents.focused_malignancy->effectN( 1 ).percent();

        if ( p()->talents.cull_the_weak.ok() )
          m *= 1.0 + ( std::min( target_count, as<int>( p()->talents.cull_the_weak->effectN( 2 ).base_value() ) ) * p()->talents.cull_the_weak->effectN( 1 ).percent() );

        if ( p()->talents.malign_omen.ok() )
          m *= 1.0 + p()->buffs.malign_omen->check_value();

        if ( soul_harvester() && p()->buffs.succulent_soul->check() )
          m *= 1.0 + p()->hero.succulent_soul->effectN( 2 ).percent();

        return m;
      }

      void execute() override
      {
        int d = td( target )->count_affliction_dots() - 1;
        assert( d < as<int>( p()->procs.malefic_rapture.size() ) && "The procs.malefic_rapture array needs to be expanded." );

        if ( d >= 0 && d < as<int>( p()->procs.malefic_rapture.size() ) )
          p()->procs.malefic_rapture[ d ]->occur();

        warlock_spell_t::execute();
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( p()->buffs.malign_omen->check() )
        {
          warlock_td_t* tdata = td( s->target );
          timespan_t extension = timespan_t::from_seconds( p()->talents.malign_omen_buff->effectN( 2 ).base_value() );

          tdata->dots_agony->adjust_duration( extension );
          tdata->dots_corruption->adjust_duration( extension );
          tdata->dots_phantom_singularity->adjust_duration( extension );
          tdata->dots_vile_taint->adjust_duration( extension );
          tdata->dots_unstable_affliction->adjust_duration( extension );
          tdata->dots_soul_rot->adjust_duration( extension );
          tdata->debuffs_haunt->extend_duration( p(), extension );
          tdata->dots_wither->adjust_duration( extension );
        }

        if ( p()->talents.malefic_touch.ok() )
          touch->execute_on_target( s->target );

        if ( soul_harvester() && p()->buffs.succulent_soul->check() )
        {
          bool fervor = td( s->target )->dots_unstable_affliction->is_ticking();
          debug_cast<demonic_soul_t*>( p()->proc_actions.demonic_soul )->demoniacs_fervor = fervor;
          p()->proc_actions.demonic_soul->execute_on_target( s->target );
        }
      }

      double composite_crit_chance_multiplier() const override
      {
        double m = warlock_spell_t::composite_crit_chance_multiplier();

        if ( active_2pc( TWW1 ) )
          m *= 1.0 + p()->tier.hexflame_aff_2pc->effectN( 2 ).percent();

        return m;
      }
    };

    malefic_rapture_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Malefic Rapture", p, p->warlock_base.malefic_rapture, options_str )
    {
      aoe = -1;

      impact_action = new malefic_rapture_damage_t( p );
      add_child( impact_action );
    }

    double cost_pct_multiplier() const override
    {
      double c = warlock_spell_t::cost_pct_multiplier();

      if ( p()->buffs.tormented_crescendo->check() )
        c *= 1.0 + p()->talents.tormented_crescendo_buff->effectN( 3 ).percent();

      return c;
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      if ( p()->buffs.tormented_crescendo->check() )
        m *= 1.0 + p()->talents.tormented_crescendo_buff->effectN( 2 ).percent();

      m *= 1.0 + p()->talents.improved_malefic_rapture->effectN( 2 ).percent();

      return m;
    }

    bool ready() override
    {
      if ( !warlock_spell_t::ready() )
        return false;

      target_cache.is_valid = false;
      return target_list().size() > 0;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.malign_omen.ok() && p()->buffs.malign_omen->check() )
        p()->buffs.soul_rot->extend_duration( p(), timespan_t::from_seconds( p()->talents.malign_omen_buff->effectN( 2 ).base_value() ) );

      if ( active_4pc( TWW1 ) )
      {
        bool success = p()->buffs.umbral_lattice->trigger();

        if ( success )
          p()->procs.umbral_lattice->occur();
      }

      p()->buffs.tormented_crescendo->decrement();
      p()->buffs.malign_omen->decrement();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      debug_cast<malefic_rapture_damage_t*>( impact_action )->target_count = as<int>( s->n_targets );

      if ( soul_harvester() && p()->buffs.succulent_soul->check() )
      {
        bool primary = ( s->chain_target == 0 );

        if ( primary )
          make_event( *sim, 1_ms, [ this ] { p()->buffs.succulent_soul->decrement(); } );
      }
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      warlock_spell_t::available_targets( tl );

      range::erase_remove( tl, [ this ]( player_t* t ){ return td( t )->count_affliction_dots() == 0; } );

      return tl.size();
    }
  };

  struct perpetual_unstability_t : public warlock_spell_t
  {
    perpetual_unstability_t( warlock_t* p )
      : warlock_spell_t( "Perpetual Unstability", p, p->talents.perpetual_unstability_proc )
    { background = dual = true; }
  };

  struct unstable_affliction_t : public warlock_spell_t
  {
    perpetual_unstability_t* perpetual_unstability;

    unstable_affliction_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Unstable Affliction", p, p->talents.unstable_affliction, options_str )
    {
      base_dd_multiplier *= 1.0 + p->talents.xavius_gambit->effectN( 2 ).percent();
      base_td_multiplier *= 1.0 + p->talents.xavius_gambit->effectN( 1 ).percent();

      dot_duration += p->talents.unstable_affliction_3->effectN( 1 ).time_value();

      triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

      if ( p->talents.perpetual_unstability.ok() )
      {
        perpetual_unstability = new perpetual_unstability_t( p );
        add_child( perpetual_unstability );
      }
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      m *= 1.0 + p()->talents.perpetual_unstability->effectN( 2 ).percent();

      return m;
    }

    void execute() override
    {
      if ( p()->ua_target && p()->ua_target != target )
        td( p()->ua_target )->dots_unstable_affliction->cancel();

      p()->ua_target = target;

      warlock_spell_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      bool ticking = td( s->target )->dots_unstable_affliction->is_ticking();
      timespan_t remains = td( s->target )->dots_unstable_affliction->remains();

      warlock_spell_t::impact( s );

      if ( p()->talents.perpetual_unstability.ok() && ticking && remains < timespan_t::from_seconds( p()->talents.perpetual_unstability->effectN( 1 ).base_value() ) )
        perpetual_unstability->execute_on_target( s->target );
    }

    void last_tick( dot_t* d ) override
    {
      warlock_spell_t::last_tick( d );

      p()->ua_target = nullptr;
    }
  };

  struct volatile_agony_t : public warlock_spell_t
  {
    volatile_agony_t( warlock_t* p )
      : warlock_spell_t( "Volatile Agony", p, p->talents.volatile_agony_aoe )
    {
      background = dual = true;
      aoe = -1;

      reduced_aoe_targets = p->talents.volatile_agony->effectN( 2 ).base_value();
    }
  };

  struct agony_t : public warlock_spell_t
  {
    volatile_agony_t* vol_ag;

    agony_t( warlock_t* p, util::string_view options_str ) 
      : warlock_spell_t( "Agony", p, p->warlock_base.agony, options_str )
    {
      may_crit = false;

      dot_max_stack = as<int>( data().max_stacks() + p->warlock_base.agony_2->effectN( 1 ).base_value() );
      dot_max_stack += as<int>( p->talents.writhe_in_agony->effectN( 1 ).base_value() );

      base_dd_multiplier *= 1.0 + p->talents.socrethars_guile->effectN( 1 ).percent();
      base_td_multiplier *= 1.0 + p->talents.socrethars_guile->effectN( 4 ).percent();
      base_td_multiplier *= 1.0 + p->hero.mark_of_xavius->effectN( 1 ).percent();

      triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

      if ( p->talents.volatile_agony.ok() )
      {
        vol_ag = new volatile_agony_t( p );
        add_child( vol_ag );
      }
    }

    void last_tick ( dot_t* d ) override
    {
      if ( p()->get_active_dots( d ) == 1 )
        p()->agony_accumulator = rng().range( 0.0, 0.99 );

      warlock_spell_t::last_tick( d );
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.writhe_in_agony.ok() )
      {
        int delta = (int)( p()->talents.writhe_in_agony->effectN( 3 ).base_value() ) - td( execute_state->target )->dots_agony->current_stack();

        if ( delta > 0 )
          td( execute_state->target )->dots_agony->increment( delta );
      }
    }

    void impact( action_state_t* s ) override
    {
      if ( p()->talents.volatile_agony.ok() )
      {
        if( td(s->target)->dots_agony->is_ticking() && td( s->target )->dots_agony->remains() < timespan_t::from_seconds( p()->talents.volatile_agony->effectN( 1 ).base_value() ) )
          vol_ag->execute_on_target( s->target );
      }

      warlock_spell_t::impact( s );
    }

    void tick( dot_t* d ) override
    {
      // Blizzard has not publicly released the formula for Agony's chance to generate a Soul Shard.
      // This set of code is based on results from 500+ Soul Shard sample sizes, and matches in-game
      // results to within 0.1% of accuracy in all tests conducted on all targets numbers up to 8.
      // Accurate as of 08-24-2018. TOCHECK regularly. If any changes are made to this section of
      // code, please also update the Time_to_Shard expression in sc_warlock.cpp.
      double increment_max = p()->rng_settings.agony.setting_value;

      double active_agonies = p()->get_active_dots( d );
      increment_max *= std::pow( active_agonies, -2.0 / 3.0 );

      // 2023-09-01: Recent test noted that Creeping Death is once again renormalizing shard generation to be neutral with/without the talent.
      if ( p()->talents.creeping_death.ok() )
        increment_max *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();

      if ( p()->talents.relinquished.ok() )
        increment_max *= 1.0 + p()->talents.relinquished->effectN( 1 ).percent();

      p()->agony_accumulator += rng().range( 0.0, increment_max );

      if ( p()->agony_accumulator >= 1 )
      {
        p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony );
        p()->agony_accumulator -= 1.0;
      }

      warlock_spell_t::tick( d );

      td( d->state->target )->dots_agony->increment( 1 );
    }
  };

  struct seed_of_corruption_t : public warlock_spell_t
  {
    struct seed_of_corruption_aoe_t : public warlock_spell_t
    {
      action_t* applied_dot;

      seed_of_corruption_aoe_t( warlock_t* p )
        : warlock_spell_t( "Seed of Corruption (AoE)", p, p->talents.seed_of_corruption_aoe )
      {
        aoe = -1;
        background = dual = true;

        if ( p->hero.wither.ok() )
          applied_dot = new wither_t( p, "" );
        else
          applied_dot = new corruption_t( p, "", true );

        applied_dot->background = true;
        applied_dot->dual = true;
        applied_dot->base_costs[ RESOURCE_MANA ] = 0;
        applied_dot->base_dd_multiplier = 0.0;

        base_dd_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 1 ).percent(); // TOCHECK: 2024-07-05 This is still in effect on Beta
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
          
          applied_dot->execute_on_target( s->target );
        }
      }
    };

    seed_of_corruption_aoe_t* explosion;

    seed_of_corruption_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Seed of Corruption", p, p->talents.seed_of_corruption, options_str ),
      explosion( new seed_of_corruption_aoe_t( p ) )
    {
      may_crit = false;
      tick_zero = false;
      base_tick_time = dot_duration;
      hasted_ticks = false;

      add_child( explosion );
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
      // 2. If no targets are valid according to the above, all targets are instead valid (will refresh DoT on existing target(s) instead)
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

    void impact( action_state_t* s ) override
    {
      if ( result_is_hit( s->result ) )
        td( s->target )->soc_threshold = s->composite_spell_power() * p()->talents.seed_of_corruption->effectN( 1 ).percent();

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

  struct drain_soul_t : public warlock_spell_t
  {
    struct drain_soul_state_t : public action_state_t
    {
      double tick_time_multiplier;
      double td_multiplier;

      drain_soul_state_t( action_t* action, player_t* target )
        : action_state_t( action, target ),
        tick_time_multiplier( 1.0 ),
        td_multiplier( 1.0 )
      { }

      void initialize() override
      {
        action_state_t::initialize();
        tick_time_multiplier = 1.0;
        td_multiplier = 1.0;
      }

      std::ostringstream& debug_str( std::ostringstream& s ) override
      {
        action_state_t::debug_str( s ) << " tick_time_multiplier=" << tick_time_multiplier;
        action_state_t::debug_str( s ) << " td_multiplier=" << td_multiplier;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        tick_time_multiplier = debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;
        td_multiplier = debug_cast<const drain_soul_state_t*>( s )->td_multiplier;
      }
    };

    shadow_bolt_volley_t* volley;

    drain_soul_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Drain Soul", p, p->talents.drain_soul.ok() ? p->talents.drain_soul_dot : spell_data_t::not_found(), options_str )
    {
      channeled = true;

      base_td_multiplier *= 1.0 + p->talents.sargerei_technique->effectN( 3 ).percent();
      base_td_multiplier *= 1.0 + p->talents.dark_virtuosity->effectN( 2 ).percent();

      if ( p->talents.cunning_cruelty.ok() )
        volley = new shadow_bolt_volley_t( p );
    }

    action_state_t* new_state() override
    { return new drain_soul_state_t( this, target ); }

    void snapshot_state( action_state_t* s, result_amount_type rt ) override
    {
      debug_cast<drain_soul_state_t*>( s )->tick_time_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? p()->talents.nightfall_buff->effectN( 3 ).percent() : 0 );
      debug_cast<drain_soul_state_t*>( s )->td_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? p()->talents.nightfall_buff->effectN( 2 ).percent() + p()->hero.necrolyte_teachings->effectN( 1 ).percent() : 0 );
      warlock_spell_t::snapshot_state( s, rt );
    }

    double tick_time_pct_multiplier( const action_state_t* s ) const override
    {
      auto mul = warlock_spell_t::tick_time_pct_multiplier( s );

      mul *= debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;

      return mul;
    }

    double dot_duration_pct_multiplier( const action_state_t* s ) const override
    {
      auto mul = warlock_spell_t::dot_duration_pct_multiplier( s );

      if ( p()->buffs.nightfall->check() )
        mul *= 1.0 + p()->talents.nightfall_buff->effectN( 4 ).percent();

      return mul;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( soul_harvester() && p()->buffs.nightfall->check() )
      {
        if ( p()->hero.wicked_reaping.ok() )
          p()->proc_actions.wicked_reaping->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.shared_fate.ok() )
          td( target )->debuffs_shared_fate->trigger();

        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && rng().roll( p()->rng_settings.feast_of_souls.setting_value ) )
          p()->feast_of_souls_gain();
      }
      p()->buffs.nightfall->decrement();
    }

    void tick( dot_t* d ) override
    {
      warlock_spell_t::tick( d );

      if ( result_is_hit( d->state->result ) )
      {
        if ( p()->talents.shadow_embrace.ok() )
          td( d->target )->debuffs_shadow_embrace->trigger();

        if ( p()->talents.tormented_crescendo.ok() )
        {
          if ( crescendo_check( p() ) && rng().roll( p()->talents.tormented_crescendo->effectN( 2 ).percent() ) )
          {
            p()->procs.tormented_crescendo->occur();
            p()->buffs.tormented_crescendo->trigger();
          }
        }

        if ( p()->talents.cunning_cruelty.ok() && rng().roll( p()->rng_settings.cunning_cruelty_ds.setting_value ) )
        {
          p()->procs.shadow_bolt_volley->occur();
          volley->execute_on_target( d->target );
        }
      }
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( t );

      if ( t->health_percentage() < p()->talents.drain_soul_dot->effectN( 3 ).base_value() )
        m *= 1.0 + p()->talents.drain_soul_dot->effectN( 2 ).percent();

      if ( p()->talents.withering_bolt.ok() )
        m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( (int)( p()->talents.withering_bolt->effectN( 2 ).base_value() ), td( t )->count_affliction_dots() );

      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      m *= debug_cast<const drain_soul_state_t*>( s )->td_multiplier;

      if ( soul_harvester() && p()->hero.necrolyte_teachings.ok() )
        m *= 1.0 + p()->hero.necrolyte_teachings->effectN( 4 ).percent();

      return m;
    }
  };

  struct vile_taint_t : public warlock_spell_t
  {
    struct vile_taint_dot_t : public warlock_spell_t
    {
      vile_taint_dot_t( warlock_t* p )
        : warlock_spell_t( "Vile Taint (DoT)", p, p->talents.vile_taint_dot )
      {
        tick_zero = background = true;
        execute_action = new agony_t( p, "" );
        execute_action->background = true;
        execute_action->dual = true;
        execute_action->base_costs[ RESOURCE_MANA ] = 0.0;
      }
    };
    
    vile_taint_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Vile Taint", p, p->talents.vile_taint, options_str )
    {
      impact_action = new vile_taint_dot_t( p );
      add_child( impact_action );
    }

    void impact( action_state_t* s ) override
    {
      bool fresh_agony = !td( s->target )->dots_agony->is_ticking();

      warlock_spell_t::impact( s );

      if ( p()->talents.infirmity.ok() && fresh_agony )
        td( s->target )->dots_agony->increment( as<int>( p()->talents.infirmity->effectN( 1 ).base_value() ) );
    }
  };

  struct phantom_singularity_t : public warlock_spell_t
  {
    struct phantom_singularity_tick_t : public warlock_spell_t
    {
      phantom_singularity_tick_t( warlock_t* p )
        : warlock_spell_t( "Phantom Singularity (tick)", p, p->talents.phantom_singularity_tick )
      {
        background = dual = true;
        may_miss = false;
        aoe = -1;
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( p()->talents.infirmity.ok() && !td( s->target )->debuffs_infirmity->check() )
          td( s->target )->debuffs_infirmity->trigger();
      }
    };

    phantom_singularity_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Phantom Singularity", p, p->talents.phantom_singularity, options_str )
    {
      callbacks = false;
      hasted_ticks = true;
      tick_action = new phantom_singularity_tick_t( p );

      spell_power_mod.tick = 0;
    }

    void init() override
    {
      warlock_spell_t::init();

      update_flags &= ~STATE_HASTE;
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.infirmity.ok() )
        td( s->target )->debuffs_infirmity->trigger();
    }

    void last_tick( dot_t* d ) override
    {
      warlock_spell_t::last_tick( d );

      for ( auto t : p()->sim->target_non_sleeping_list )
      {
        if ( !td( t ) )
          continue;

        make_event( *sim, 0_ms, [ this, t ] { td( t )->debuffs_infirmity->expire(); } );
      }
    }
  };

  struct haunt_t : public warlock_spell_t
  {
    haunt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Haunt", p, p->talents.haunt, options_str )
    { base_dd_multiplier *= 1.0 + p->talents.improved_haunt->effectN( 1 ).percent(); }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      if ( p()->talents.improved_haunt.ok() )
        m *= 1.0 + p()->talents.improved_haunt->effectN( 2 ).percent();

      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( soul_harvester() && p()->hero.sataiels_volition.ok() )
        p()->buffs.nightfall->trigger();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        td( s->target )->debuffs_haunt->trigger();

        if ( p()->talents.improved_haunt.ok() )
          td( s->target )->debuffs_shadow_embrace->trigger();
      }
    }
  };



  struct summon_darkglare_t : public warlock_spell_t
  {
    struct malevolent_visionary_t : public warlock_spell_t
    {
      malevolent_visionary_t( warlock_t* p )
        : warlock_spell_t( "Malevolent Visionary", p, p->talents.malevolent_visionary_blast )
      { background = dual = true; }
    };

    malevolent_visionary_t* mal_vis;

    summon_darkglare_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Summon Darkglare", p, p->talents.summon_darkglare, options_str )
    {
      harmful = callbacks = true; // Set to true because of 10.1 class trinket
      may_crit = may_miss = false;

      if ( p->talents.malevolent_visionary.ok() )
      {
        mal_vis = new malevolent_visionary_t( p );
        add_child( mal_vis );
      }
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.darkglares.spawn( p()->talents.summon_darkglare->duration() );

      timespan_t darkglare_extension = timespan_t::from_seconds( p()->talents.summon_darkglare->effectN( 2 ).base_value() );

      darkglare_extension_helper( darkglare_extension );

      p()->buffs.soul_rot->extend_duration( p(), darkglare_extension ); // This dummy buff is active while Soul Rot is ticking
    }

    void darkglare_extension_helper( timespan_t darkglare_extension )
    {
      for ( const auto target : p()->sim->target_non_sleeping_list )
      {
        warlock_td_t* td = p()->get_target_data( target );
        if ( !td )
          continue;

        td->dots_agony->adjust_duration( darkglare_extension );
        td->dots_corruption->adjust_duration( darkglare_extension );
        td->dots_phantom_singularity->adjust_duration( darkglare_extension );
        td->dots_vile_taint->adjust_duration( darkglare_extension );
        td->dots_unstable_affliction->adjust_duration( darkglare_extension );
        td->dots_soul_rot->adjust_duration( darkglare_extension );
        td->dots_wither->adjust_duration( darkglare_extension );

        if ( p()->talents.malevolent_visionary.ok() && td->count_affliction_dots() > 0 )
          mal_vis->execute_on_target( target );
      }
    }
  };

  struct soul_rot_t : public warlock_spell_t
  {
    soul_rot_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Soul Rot", p, p->talents.soul_rot, options_str )
    { aoe = 1 + as<int>( p->talents.soul_rot->effectN( 3 ).base_value() ); }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.soul_rot->trigger();

      if ( p()->talents.malign_omen.ok() )
        p()->buffs.malign_omen->trigger( as<int>( p()->talents.malign_omen->effectN( 2 ).base_value() ) );

      if ( soul_harvester() && p()->hero.shadow_of_death.ok() )
      {
        p()->resource_gain( RESOURCE_SOUL_SHARD, p()->hero.shadow_of_death_energize->effectN( 1 ).base_value() / 10.0, p()->gains.shadow_of_death );
        p()->buffs.succulent_soul->trigger( as<int>( p()->hero.shadow_of_death_energize->effectN( 1 ).base_value() / 10.0 ) );
      }
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.dark_harvest.ok() )
      {
        p()->buffs.dark_harvest_haste->trigger();
        p()->buffs.dark_harvest_crit->trigger();
      }
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      if ( s->chain_target == 0 && aoe > 1 )
        m *= 1.0 + p()->talents.soul_rot->effectN( 4 ).base_value() / 10.0; // Primary target takes increased damage

      return m;
    }
  };

  struct oblivion_t : public warlock_spell_t
  {
    oblivion_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Oblivion", p, p->talents.oblivion, options_str )
    { channeled = true; }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      m *= 1.0 + std::min( td( s->target )->count_affliction_dots(), as<int>( p()->talents.oblivion->effectN( 3 ).base_value() ) ) * p()->talents.oblivion->effectN( 2 ).percent();

      return m;
    }
  };

  // Affliction Actions End
  // Demonology Actions Begin

  struct hand_of_guldan_t : public warlock_spell_t
  {
    struct umbral_blaze_dot_t : public warlock_spell_t
    {
      umbral_blaze_dot_t( warlock_t* p )
        : warlock_spell_t( "Umbral Blaze", p, p->talents.umbral_blaze_dot )
      {
        background = dual = true;
        hasted_ticks = false;
        base_td_multiplier = 1.0 + p->talents.umbral_blaze->effectN( 2 ).percent();
      }
    };

    struct hog_impact_t : public warlock_spell_t
    {
      int shards_used;
      timespan_t meteor_time;
      umbral_blaze_dot_t* blaze;

      hog_impact_t( warlock_t* p )
        : warlock_spell_t( "Hand of Gul'dan (Impact)", p, p->warlock_base.hog_impact ),
        shards_used( 0 ),
        meteor_time( 400_ms )
      {
        aoe = -1;
        dual = true;

        affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();
        
        triggers.shadow_invocation = true;
        triggers.demonic_art = p->hero.diabolic_ritual.ok();

        if ( p->talents.umbral_blaze.ok() )
        {
          blaze = new umbral_blaze_dot_t( p );
          add_child( blaze );
        }
      }

      timespan_t travel_time() const override
      { return meteor_time; }

      double action_multiplier() const override
      {
        double m = warlock_spell_t::action_multiplier();

        m *= shards_used;

        if ( p()->hero.gloom_of_nathreza.ok() )
          m *= 1.0 + shards_used * p()->hero.gloom_of_nathreza->effectN( 1 ).percent();

        if ( soul_harvester() && p()->buffs.succulent_soul->check() )
          m *= 1.0 + p()->hero.succulent_soul->effectN( 3 ).percent();

        return m;
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        // Only trigger Wild Imps once for the original target impact.
        // Still keep it in impact instead of execute because of travel delay.
        if ( result_is_hit( s->result ) && s->target == target )
        {
          // Wild Imp spawns appear to have been sped up in Shadowlands. Last tested 2021-04-16.
          // Current behavior: HoG will spawn a meteor on cast finish. Travel time in spell data is 0.7 seconds.
          // However, damage event occurs before spell effect lands, happening 0.4 seconds after cast.
          // Imps then spawn roughly every 0.18 seconds seconds after the damage event.
          for ( int i = 1; i <= shards_used; i++ )
          {
            auto ev = make_event<imp_delay_event_t>( *sim, p(), rng().gauss( 180.0 * i, 25.0 ), 180.0 * i );
            p()->wild_imp_spawns.push_back( ev );
          }

          if ( p()->talents.umbral_blaze.ok() && rng().roll( p()->talents.umbral_blaze->effectN( 1 ).percent() ) )
          {
            blaze->execute_on_target( s->target );
            p()->procs.umbral_blaze->occur();
          }
        }

        // We need Demonic Soul to proc on every target, but buff is decremented on impact. Fudge this by 1ms to ensure all targets are hit.
        if ( soul_harvester() && p()->buffs.succulent_soul->check() )
        {
          bool primary = ( s->chain_target == 0 );

          if ( primary )
            make_event( *sim, 1_ms, [ this ] { p()->buffs.succulent_soul->decrement(); } );

          debug_cast<demonic_soul_t*>(p()->proc_actions.demonic_soul)->demoniacs_fervor = primary;
          p()->proc_actions.demonic_soul->execute_on_target( s->target );
        }
      }
    };

    hog_impact_t* impact_spell;

    hand_of_guldan_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Hand of Gul'dan", p, p->warlock_base.hand_of_guldan, options_str ),
      impact_spell( new hog_impact_t( p ) )
    {
      affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();

      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();

      add_child( impact_spell );
    }

    timespan_t travel_time() const override
    { return 0_ms; }

    bool ready() override
    {
      if ( diabolist() && p()->executing != this && p()->buffs.ruination->check() )
        return false;

      if ( p()->resources.current[ RESOURCE_SOUL_SHARD ] < 1.0 )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      int shards_used = as<int>( cost() );
      impact_spell->shards_used = shards_used;

      warlock_spell_t::execute();

      if ( p()->talents.dread_calling.ok() )
        p()->buffs.dread_calling->trigger( shards_used );
    }

    void consume_resource() override
    {
      warlock_spell_t::consume_resource();

      int lrc = as<int>( last_resource_cost ) - 1;

      assert( lrc < as<int>( p()->procs.hand_of_guldan_shards.size() ) && "The procs.hand_of_guldan_shards array needs to be expanded." );

      p()->procs.hand_of_guldan_shards[ lrc ]->occur();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      impact_spell->execute_on_target( s->target );

      if ( p()->talents.pact_of_the_imp_mother.ok() && rng().roll( p()->talents.pact_of_the_imp_mother->effectN( 1 ).percent() ) )
      {
        make_event( *sim, 0_ms, [this, t = target ]{ impact_spell->execute_on_target( t ); } );
        p()->procs.pact_of_the_imp_mother->occur();
      }
    }
  };

  struct demonbolt_t : public warlock_spell_t
  {
    struct demonbolt_state_t : public action_state_t
    {
      bool core_spent;

      demonbolt_state_t( action_t* action, player_t* target )
        : action_state_t( action, target ),
        core_spent( false )
      { }

      void initialize() override
      {
        action_state_t::initialize();
        core_spent = false;
      }

      std::ostringstream& debug_str( std::ostringstream& s ) override
      {
        action_state_t::debug_str( s ) << " core_spent=" << core_spent;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        core_spent = debug_cast<const demonbolt_state_t*>( s )->core_spent;
      }
    };

    demonbolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Demonbolt", p, p->talents.demoniac.ok() ? p->talents.demonbolt_spell : spell_data_t::not_found(), options_str )
    {
      energize_type = action_energize::ON_CAST;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount = 2.0;

      affected_by.sacrificed_souls = true;
      triggers.shadow_invocation = true;
    }

    action_state_t* new_state() override
    { return new demonbolt_state_t( this, target ); }

    void snapshot_state( action_state_t* s, result_amount_type rt ) override
    {
      debug_cast<demonbolt_state_t*>( s )->core_spent = p()->buffs.demonic_core->up();
      warlock_spell_t::snapshot_state( s, rt );
    }

    double execute_time_pct_multiplier() const override
    {
      auto mul = warlock_spell_t::execute_time_pct_multiplier();

      if ( p()->buffs.demonic_core->check() )
        mul *= 1.0 + p()->talents.demonic_core_buff->effectN( 1 ).percent();

      return mul;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.demonic_core->up(); // For benefit tracking

      if ( p()->buffs.demonic_core->check() )
      {
        if ( p()->talents.spiteful_reconstitution.ok() && rng().roll( p()->rng_settings.spiteful_reconstitution.setting_value ) )
        {
          p()->warlock_pet_list.wild_imps.spawn( 1u );
          p()->procs.spiteful_reconstitution->occur();
        }

        if ( p()->talents.immutable_hatred.ok() )
        {
          auto active_pet = p()->warlock_pet_list.active;

          if ( active_pet->pet_type == PET_FELGUARD )
            debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->hatred_proc->execute_on_target( execute_state->target );
        }

        if ( p()->talents.doom.ok() )
        {
          for ( const auto t : p()->sim->target_non_sleeping_list )
          {
            if ( td( t )->debuffs_doom->check() )
              td( t )->debuffs_doom->extend_duration( p(), -p()->talents.doom->effectN( 1 ).time_value() - p()->talents.doom_eternal->effectN( 1 ).time_value() );
          }
        }
      }
      else
      {
        if ( p()->talents.immutable_hatred.ok() && p()->bugs )
        {
          auto active_pet = p()->warlock_pet_list.active;

          if ( active_pet->pet_type == PET_FELGUARD )
            debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->hatred_proc->execute_on_target( execute_state->target );
        }
      }

      if ( soul_harvester() && p()->buffs.demonic_core->check() )
      {
        if ( p()->hero.wicked_reaping.ok() )
          p()->proc_actions.wicked_reaping->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.shared_fate.ok() )
          td( target )->debuffs_shared_fate->trigger();

        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && rng().roll( p()->rng_settings.feast_of_souls.setting_value ) )
          p()->feast_of_souls_gain();
      }

      p()->buffs.demonic_core->decrement();

      p()->buffs.power_siphon->decrement();

      if ( p()->talents.demonic_calling.ok() )
        p()->buffs.demonic_calling->trigger();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.doom.ok() && debug_cast<demonbolt_state_t*>( s )->core_spent && !td( s->target )->debuffs_doom->check() )
        td( s->target )->debuffs_doom->trigger();
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();
      
      m *= 1.0 + p()->buffs.power_siphon->check_value();

      return m;
    }
  };

  struct implosion_t : public warlock_spell_t
  {
    struct implosion_aoe_t : public warlock_spell_t
    {
      double energy_remaining = 0.0;
      warlock_pet_t* next_imp;

      implosion_aoe_t( warlock_t* p )
        : warlock_spell_t( "Implosion (AoE)", p, p->talents.implosion_aoe )
      {
        aoe = -1;
        background = dual = true;
        callbacks = false;

        affected_by.wicked_maw = p->talents.shadowtouched.ok(); // 2024-08-01: Despite what is listed in spell data, Wicked Maw seems to only work with Shadowtouched now for Implosion

        base_dd_multiplier = 1.0 + p->talents.spiteful_reconstitution->effectN( 1 ).percent();
      }

      double action_multiplier() const override
      {
        double m = warlock_spell_t::action_multiplier();

        if ( debug_cast<pets::demonology::wild_imp_pet_t*>( next_imp )->buffs.imp_gang_boss->check() )
          m *= 1.0 + p()->talents.imp_gang_boss->effectN( 2 ).percent();

        return m;
      }

      double composite_target_multiplier( player_t* t ) const override
      {
        double m = warlock_spell_t::composite_target_multiplier( t );

        if ( t == target )
          m *= ( energy_remaining / 100.0 );

        return m;
      }

      void execute() override
      {
        warlock_spell_t::execute();
        next_imp->dismiss();
      }
    };

    implosion_aoe_t* explosion;

    implosion_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Implosion", p, p->talents.implosion, options_str ),
      explosion( new implosion_aoe_t( p ) )
    { add_child( explosion ); }

    bool ready() override
    { return warlock_spell_t::ready() && p()->warlock_pet_list.wild_imps.n_active_pets() > 0; }

    void execute() override
    {
      // Travel speed is not in spell data, in game test appears to be 65 yds/sec as of 2020-12-04
      timespan_t imp_travel_time = calc_imp_travel_time( 65 );

      int launch_counter = 0;
      for ( auto imp : p()->warlock_pet_list.wild_imps )
      {
        if ( !imp->is_sleeping() )
        {
          implosion_aoe_t* ex = explosion;
          player_t* tar = target;
          double dist = p()->get_player_distance( *tar );

          imp->trigger_movement( dist, movement_direction_type::TOWARDS );
          imp->interrupt();
          imp->imploded = true;

          // Imps launched with Implosion appear to be staggered and snapshot when they impact
          // 2020-12-04: Implosion may have been made quicker in Shadowlands, too fast to easily discern with combat log
          // Going to set the interval to 10 ms, which should keep all but the most extreme imp counts from bleeding into the next GCD
          // TODO: There's an awkward possibility of Implosion seeming "ready" after casting it if all the imps have not imploded yet. Find a workaround
          make_event( sim, 50_ms * launch_counter + imp_travel_time, [ ex, tar, imp ] {
            if ( imp && !imp->is_sleeping() )
            {
              ex->energy_remaining = ( imp->resources.current[ RESOURCE_ENERGY ] );
              ex->set_target( tar );
              ex->next_imp = imp;
              ex->execute();
            }
          } );

          launch_counter++;
        }
      }

      warlock_spell_t::execute();
    }

    timespan_t calc_imp_travel_time( double speed )
    {
      double t = 0;

      if ( speed > 0 )
      {
        double distance = player->get_player_distance( *target );

        if ( distance > 0 )
          t += distance / speed;
      }

      double v = sim->travel_variance;

      if ( v )
        t = rng().gauss( t, v );

      t = std::max( t, min_travel_time );

      return timespan_t::from_seconds( t );
    }
  };

  struct call_dreadstalkers_t : public warlock_spell_t
  {
    call_dreadstalkers_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Call Dreadstalkers", p, p->talents.call_dreadstalkers, options_str )
    {
      may_crit = false;
      affected_by.soul_conduit_base_cost = true;
      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();
    }

    double cost_pct_multiplier() const override
    {
      double m = warlock_spell_t::cost_pct_multiplier();

      if ( p()->buffs.demonic_calling->check() )
        m *= 1.0 + p()->talents.demonic_calling_buff->effectN( 1 ).percent();

      return m;
    }

    double execute_time_pct_multiplier() const override
    {
      auto mul = warlock_spell_t::execute_time_pct_multiplier();

      if ( p()->buffs.demonic_calling->check() )
        mul *= 1.0 + p()->talents.demonic_calling_buff->effectN( 2 ).percent();

      return mul;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      unsigned count = as<unsigned>( p()->talents.call_dreadstalkers->effectN( 1 ).base_value() );

      // Set a randomized offset on first melee attacks after travel time. Make sure it's the same value for each dog so they're synced
      timespan_t delay = rng().range( 0_s, 1_s );

      timespan_t dur_adjust = duration_adjustment( delay );

      auto dogs = p()->warlock_pet_list.dreadstalkers.spawn( p()->talents.call_dreadstalkers_2->duration() + dur_adjust, count );

      for ( auto d : dogs )
      {
        if ( d->is_active() )
        {
          d->server_action_delay = delay;

          if ( p()->talents.dread_calling.ok() && !d->buffs.dread_calling->check() )
            d->buffs.dread_calling->trigger( 1, p()->buffs.dread_calling->check_stack_value() );
        }
      }

      if ( p()->buffs.demonic_calling->up() )
        p()->buffs.demonic_calling->decrement();

      p()->buffs.dread_calling->expire();
    }

    timespan_t duration_adjustment( timespan_t delay )
    {
      // Despawn events appear to be offset from the melee attack check in a correlated manner
      // Starting with this function which mimics despawns on the "off-beats" compared to the 1s heartbeat for the melee attack
      // This may require updating if better understanding is found for the behavior, such as a fudge from Blizzard related to player distance
      return ( delay + 500_ms ) % 1_s;
    }
  };

  struct bilescourge_bombers_t : public warlock_spell_t
  {
    struct bilescourge_bombers_tick_t : public warlock_spell_t
    {
      bilescourge_bombers_tick_t( warlock_t* p )
        : warlock_spell_t( "Bilescourge Bombers (tick)", p, p->talents.bilescourge_bombers_aoe )
      {
        aoe = -1;
        background = dual = direct_tick = true;
        callbacks = false;
        radius = p->talents.bilescourge_bombers->effectN( 1 ).radius();

        base_dd_multiplier *= 1.0 + p->talents.shadow_invocation->effectN( 1 ).percent();
      }
    };

    bilescourge_bombers_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Bilescourge Bombers", p, p->talents.bilescourge_bombers, options_str )
    {
      dot_duration = 0_ms;
      may_miss = may_crit = false;
      base_tick_time = 500_ms;

      if ( !p->proc_actions.bilescourge_bombers_aoe_tick )
      {
        p->proc_actions.bilescourge_bombers_aoe_tick = new bilescourge_bombers_tick_t( p );
        p->proc_actions.bilescourge_bombers_aoe_tick->stats = stats;
      }
    }

    void execute() override
    {
      warlock_spell_t::execute();

      make_event<ground_aoe_event_t>( *sim, p(),
                                      ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time )
                                        .duration( p()->talents.bilescourge_bombers->duration() )
                                        .start_time( sim->current_time() )
                                        .action( p()->proc_actions.bilescourge_bombers_aoe_tick ) );
    }
  };

  struct bilescourge_bombers_proc_t : public warlock_spell_t
  {
    bilescourge_bombers_proc_t( warlock_t* p )
      : warlock_spell_t( "Bilescourge Bombers (proc)", p, p->talents.bilescourge_bombers_aoe )
    {
      aoe = -1;
      background = dual = direct_tick = true;
      callbacks = false;
      radius = p->find_spell( 267211 )->effectN( 1 ).radius();

      base_dd_multiplier *= 1.0 + p->talents.shadow_invocation->effectN( 1 ).percent();
    }
  };

  struct demonic_strength_t : public warlock_spell_t
  {
    demonic_strength_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Demonic Strength", p, p->talents.demonic_strength, options_str )
    { internal_cooldown = p->cooldowns.felstorm_icd; }

    bool ready() override
    {
      auto active_pet = p()->warlock_pet_list.active;

      if ( !active_pet )
        return false;

      if ( active_pet->pet_type != PET_FELGUARD )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      auto active_pet = p()->warlock_pet_list.active;
      
      warlock_spell_t::execute();

      if ( active_pet->pet_type == PET_FELGUARD )
      {
        active_pet->buffs.demonic_strength->trigger();

        debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->queue_ds_felstorm();

        internal_cooldown->start( 5_s * p()->composite_spell_haste() );
      }
    }
  };

  struct power_siphon_t : public warlock_spell_t
  {
    power_siphon_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Power Siphon", p, p->talents.power_siphon, options_str )
    {
      harmful = false;
      ignore_false_positive = true;

      target = player;
    }

    bool ready() override
    {
      if ( is_precombat && p()->talents.inner_demons.ok() )
        return warlock_spell_t::ready();

      if ( p()->warlock_pet_list.wild_imps.n_active_pets() < 1 )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( is_precombat )
      {
        p()->buffs.power_siphon->trigger( 2, p()->talents.power_siphon_buff->duration() );
        p()->buffs.demonic_core->trigger( 2, p()->talents.demonic_core_buff->duration() );
        
        return;
      }

      auto imps = p()->warlock_pet_list.wild_imps.active_pets();

      range::sort(
        imps, []( const pets::demonology::wild_imp_pet_t* imp1, const pets::demonology::wild_imp_pet_t* imp2 ) {
          double lv = imp1->resources.current[ RESOURCE_ENERGY ];
          double rv = imp2->resources.current[ RESOURCE_ENERGY ];

          // Power Siphon deprioritizes Wild Imps that are Gang Bosses or empowered by Summon Demonic Tyrant
          // Padding ensures they still sort in order at the back of the list
          lv += ( imp1->buffs.imp_gang_boss->check() || imp1->buffs.demonic_power->check() ) ? 200.0 : 0.0;
          rv += ( imp2->buffs.imp_gang_boss->check() || imp2->buffs.demonic_power->check() ) ? 200.0 : 0.0;

          if ( lv == rv )
          {
            timespan_t lr = imp1->expiration->remains();
            timespan_t rr = imp2->expiration->remains();
            if ( lr == rr )
            {
              return imp1->actor_spawn_index < imp2->actor_spawn_index;
            }

            return lr < rr;
          }

          return lv < rv;
        } );

      unsigned max_imps = as<int>( p()->talents.power_siphon->effectN( 1 ).base_value() );

      if ( imps.size() > max_imps )
        imps.resize( max_imps );

      while ( !imps.empty() )
      {
        p()->buffs.power_siphon->trigger();
        p()->buffs.demonic_core->trigger();
        pets::demonology::wild_imp_pet_t* imp = imps.front();
        imps.erase( imps.begin() );
        imp->power_siphon = true;
        imp->dismiss();
      }
    }
  };

  struct summon_demonic_tyrant_t : public warlock_spell_t
  {
    summon_demonic_tyrant_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Summon Demonic Tyrant", p, p->talents.summon_demonic_tyrant, options_str )
    {
      harmful = true; // Set to true because of 10.1 class trinket
      may_crit = false;
      resource_current = RESOURCE_SOUL_SHARD; // For Cruelty of Kerxan proccing

      triggers.diabolic_ritual = p->hero.cruelty_of_kerxan.ok();
    }

    void execute() override
    {
      warlock_spell_t::execute();
      
      // Last tested 2021-07-13
      // There is a chance for tyrant to get an extra cast off before reaching the required haste breakpoint.
      // In-game testing found this can be modelled fairly closely using a normal distribution.
      timespan_t extraTyrantTime = rng().gauss<380,220>();
      auto tyrants = p()->warlock_pet_list.demonic_tyrants.spawn( data().duration() + extraTyrantTime );

      timespan_t extension_time = p()->talents.demonic_power_buff->effectN( 3 ).time_value();

      int wild_imp_counter = 0;
      int demon_counter = 0;
      int imp_cap = as<int>( p()->talents.summon_demonic_tyrant->effectN( 3 ).base_value() + p()->talents.reign_of_tyranny->effectN( 1 ).base_value() );

      for ( auto& pet : p()->pet_list )
      {
        auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

        if ( lock_pet == nullptr )
          continue;

        if ( lock_pet->is_sleeping() )
          continue;

        pet_e pet_type = lock_pet->pet_type;

        if ( pet_type == PET_DEMONIC_TYRANT )
          continue;

        if ( pet_type == PET_WILD_IMP && wild_imp_counter < imp_cap )
        {
          if ( lock_pet->expiration )
            lock_pet->expiration->reschedule_time = lock_pet->expiration->time + extension_time;

          lock_pet->buffs.demonic_power->trigger();
          wild_imp_counter++;
          demon_counter++;
        }
        else if ( pet_type == PET_DREADSTALKER || pet_type == PET_VILEFIEND || pet_type == PET_SERVICE_FELGUARD || pet_type == PET_FELGUARD )
        {
          if ( lock_pet->expiration )
            lock_pet->expiration->reschedule_time = lock_pet->expiration->time + extension_time;

          lock_pet->buffs.demonic_power->trigger();
          demon_counter++;
        }
      }

      p()->buffs.tyrant->trigger();

      if ( p()->hero.abyssal_dominion.ok() )
        p()->buffs.abyssal_dominion->trigger();

      if ( p()->buffs.dreadstalkers->check() )
        p()->buffs.dreadstalkers->extend_duration( p(), extension_time );

      if ( p()->buffs.grimoire_felguard->check() )
        p()->buffs.grimoire_felguard->extend_duration( p(), extension_time );

      if ( p()->buffs.vilefiend->check() )
        p()->buffs.vilefiend->extend_duration( p(), extension_time );

      if ( p()->talents.reign_of_tyranny.ok() )
      {
        for ( auto t : tyrants )
        {
          if ( t->is_active() )
            t->buffs.reign_of_tyranny->trigger( demon_counter );
        }
      }

      if ( p()->hero.cruelty_of_kerxan.ok() )
      {
        timespan_t reduction = -p()->hero.cruelty_of_kerxan->effectN( 1 ).time_value();

        p()->buffs.ritual_overlord->extend_duration( p(), reduction );
        p()->buffs.ritual_mother->extend_duration( p(), reduction );
        p()->buffs.ritual_pit_lord->extend_duration( p(), reduction );
      }

      if ( soul_harvester() && p()->hero.shadow_of_death.ok() )
      {
        p()->resource_gain( RESOURCE_SOUL_SHARD, p()->hero.shadow_of_death_energize->effectN( 1 ).base_value() / 10.0, p()->gains.shadow_of_death );
        p()->buffs.succulent_soul->trigger( as<int>( p()->hero.shadow_of_death_energize->effectN( 1 ).base_value() / 10.0 ) );
      }
    }
  };

  struct grimoire_felguard_t : public warlock_spell_t
  {
    grimoire_felguard_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Grimoire: Felguard", p, p->talents.grimoire_felguard, options_str )
    {
      harmful = may_crit = false;

      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.grimoire_felguards.spawn( p()->talents.grimoire_felguard->duration() );
      p()->buffs.grimoire_felguard->trigger();
    }
  };

  struct summon_vilefiend_t : public warlock_spell_t
  {
    summon_vilefiend_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Summon Vilefiend", p, p->talents.summon_vilefiend, options_str )
    {
      harmful = may_crit = false;

      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();
    }

    void execute() override
    {
      warlock_spell_t::execute();
      
      p()->buffs.vilefiend->trigger();
      p()->warlock_pet_list.vilefiends.spawn( p()->talents.summon_vilefiend->duration() );
    }
  };

  struct guillotine_t : public warlock_spell_t
  {
    guillotine_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Guillotine", p, p->talents.guillotine, options_str )
    {
      may_crit = false;
      internal_cooldown = p->get_cooldown( "felstorm_icd" );
    }

    bool ready() override
    {
      auto active_pet = p()->warlock_pet_list.active;

      if ( !active_pet )
        return false;

      if ( active_pet->pet_type != PET_FELGUARD )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      auto active_pet = p()->warlock_pet_list.active;

      warlock_spell_t::execute();

      if ( active_pet->pet_type == PET_FELGUARD )
      {
        active_pet->buffs.fiendish_wrath->trigger();

        debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->felguard_guillotine->execute_on_target( execute_state->target );

        internal_cooldown->start( 6_s );
      }
    }
  };

  struct doom_t : public warlock_spell_t
  {
    doom_t( warlock_t* p )
      : warlock_spell_t( "Doom", p, p->talents.doom_dmg )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = p->talents.doom->effectN( 2 ).base_value();

      base_dd_multiplier *= 1.0 + p->talents.impending_doom->effectN( 1 ).percent();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.impending_doom.ok() )
        p()->warlock_pet_list.wild_imps.spawn( as<int>( p()->talents.impending_doom->effectN( 2 ).base_value() ) );
    }
  };

  // Demonology Actions End
  // Destruction Actions Begin

  struct incinerate_t : public warlock_spell_t
  {
    struct incinerate_fnb_t : public warlock_spell_t
    {
      incinerate_fnb_t( warlock_t* p )
        : warlock_spell_t( "Incinerate (Fire and Brimstone)", p, p->warlock_base.incinerate )
      {
        aoe = -1;
        background = dual = true;

        affected_by.chaotic_energies = true;
        affected_by.ashen_remains = p->talents.ashen_remains.ok();

        triggers.dimension_ripper = p->talents.dimension_ripper.ok();

        base_multiplier *= p->talents.fire_and_brimstone->effectN( 1 ).percent();

        base_dd_multiplier *= 1.0 + p->talents.sargerei_technique->effectN( 2 ).percent(); // TOCHECK: Does this apply in-game correctly?
      }

      void init() override
      {
        warlock_spell_t::init();

        p()->havoc_spells.push_back( this ); // Needed for proper target list invalidation
      }

      double cost() const override
      { return 0.0; }

      size_t available_targets( std::vector<player_t*>& tl ) const override
      {
        warlock_spell_t::available_targets( tl );

        auto it = range::find( tl, target );
        if ( it != tl.end() )
          tl.erase( it );

        it = range::find( tl, p()->havoc_target );
        if ( it != tl.end() )
          tl.erase( it );

        return tl.size();
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( p()->bugs && p()->talents.diabolic_embers.ok() && s->result == RESULT_CRIT )
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.incinerate_crits );
      }

      double action_multiplier() const override
      {
        double m = warlock_spell_t::action_multiplier();

        m *= 1.0 + p()->buffs.burn_to_ashes->check_value();

        return m;
      }

      double composite_crit_chance() const override
      {
        double c = warlock_spell_t::composite_crit_chance();

        if ( p()->talents.indiscriminate_flames.ok() && p()->buffs.backdraft->check() )
          c += p()->talents.indiscriminate_flames->effectN( 2 ).percent();

        return c;
      }

      double composite_crit_chance_multiplier() const override
      {
        double m = warlock_spell_t::composite_crit_chance_multiplier();

        if ( active_2pc( TWW1 ) )
          m *= 1.0 + p()->tier.hexflame_destro_2pc->effectN( 1 ).percent();

        return m;
      }
    };

    double energize_mult;
    incinerate_fnb_t* fnb_action;

    incinerate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Incinerate", p, p->warlock_base.incinerate, options_str ),
      fnb_action( new incinerate_fnb_t( p ) )
    {
      energize_type = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount = ( p->warlock_base.incinerate_energize->effectN( 1 ).base_value() ) / 10.0;
      
      energize_mult = 1.0 + p->talents.diabolic_embers->effectN( 1 ).percent();
      energize_amount *= energize_mult;

      affected_by.chaotic_energies = true;
      affected_by.havoc = true;
      affected_by.ashen_remains = p->talents.ashen_remains.ok();

      triggers.dimension_ripper = p->talents.dimension_ripper.ok();

      add_child( fnb_action );

      base_dd_multiplier *= 1.0 + p->talents.sargerei_technique->effectN( 2 ).percent();
    }

    bool ready() override
    {
      if ( diabolist() && p()->executing != this && p()->buffs.infernal_bolt->check() )
        return false;

      return warlock_spell_t::ready();
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      m *= 1.0 + p()->talents.emberstorm->effectN( 2 ).percent();

      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();
      
      if ( p()->talents.fire_and_brimstone.ok() )
        fnb_action->execute_on_target( target );

      p()->buffs.backdraft->decrement();
      p()->buffs.burn_to_ashes->decrement(); // Must do after Fire and Brimstone execute so that child picks up buff
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( s->result == RESULT_CRIT )
        p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1 * energize_mult, p()->gains.incinerate_crits );
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      m *= 1.0 + p()->buffs.burn_to_ashes->check_value();

      return m;
    }

    double composite_crit_chance() const override
    {
      double c = warlock_spell_t::composite_crit_chance();

      if ( p()->talents.indiscriminate_flames.ok() && p()->buffs.backdraft->check() )
        c += p()->talents.indiscriminate_flames->effectN( 2 ).percent();

      return c;
    }

    double composite_crit_chance_multiplier() const override
    {
      double m = warlock_spell_t::composite_crit_chance_multiplier();

      if ( active_2pc( TWW1 ) )
        m *= 1.0 + p()->tier.hexflame_destro_2pc->effectN( 1 ).percent();

      return m;
    }
  };

  struct immolate_t : public warlock_spell_t
  {
    struct immolate_dot_t : public warlock_spell_t
    {
      immolate_dot_t( warlock_t* p )
        : warlock_spell_t( "Immolate", p, p->warlock_base.immolate_dot )
      {
        background = dual = true;

        affected_by.chaotic_energies = true;

        dot_duration += p->talents.scalding_flames->effectN( 3 ).time_value();

        base_multiplier *= 1.0 + p->talents.scalding_flames->effectN( 2 ).percent();
        base_td_multiplier *= 1.0 + p->talents.socrethars_guile->effectN( 5 ).percent();
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( d->state->result == RESULT_CRIT && rng().roll( p()->warlock_base.immolate_old->effectN( 2 ).percent() ) )
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits );

        p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate );

        if ( p()->talents.flashpoint.ok() && d->state->target->health_percentage() >= p()->talents.flashpoint->effectN( 2 ).base_value() )
          p()->buffs.flashpoint->trigger();
      }
    };

    immolate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Immolate (direct)", p, p->warlock_base.immolate->ok() && !p->hero.wither.ok() ? p->warlock_base.immolate_old : spell_data_t::not_found(), options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      impact_action = new immolate_dot_t( p );
      add_child( impact_action );

      base_multiplier *= 1.0 + p->talents.scalding_flames->effectN( 1 ).percent();
      base_dd_multiplier *= 1.0 + p->talents.socrethars_guile->effectN( 3 ).percent();

      triggers.decimation = p->talents.decimation.ok() && !dual;
    }

    dot_t* get_dot( player_t* t ) override
    { return impact_action->get_dot( t ); }
  };

  struct internal_combustion_t : public warlock_spell_t
  {
    internal_combustion_t( warlock_t* p )
      : warlock_spell_t( "Internal Combustion", p, p->talents.internal_combustion )
    {
      background = dual = true;

      triggers.decimation = false;
    }

    void init() override
    {
      warlock_spell_t::init();

      snapshot_flags &= STATE_NO_MULTIPLIER;
    }

    void execute() override
    {
      dot_t* dot = p()->hero.wither.ok() ? td( target )->dots_wither : td( target )->dots_immolate;

      assert( dot->current_action );
      action_state_t* state = dot->current_action->get_state( dot->state );
      dot->current_action->calculate_tick_amount( state, 1.0 );

      double tick_base_damage = state->result_raw;

      if ( td( target )->debuffs_conflagrate->up() )
        tick_base_damage /= 1.0 + td( target )->debuffs_conflagrate->check_value();

      timespan_t remaining = std::min( dot->remains(), timespan_t::from_seconds( p()->talents.internal_combustion->effectN( 1 ).base_value() ) );
      timespan_t dot_tick_time = dot->current_action->tick_time( state );
      double ticks_left = remaining / dot_tick_time;
      double total_damage = ticks_left * tick_base_damage;

      action_state_t::release( state );

      base_dd_min = base_dd_max = total_damage;

      warlock_spell_t::execute();

      dot->adjust_duration( -remaining );
    }
  };

  struct chaos_bolt_t : public warlock_spell_t
  {
    internal_combustion_t* internal_combustion;

    chaos_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Chaos Bolt", p, p->warlock_base.chaos_bolt, options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;
      affected_by.ashen_remains = p->talents.ashen_remains.ok();
      affected_by.chaos_incarnate = p->talents.chaos_incarnate.ok();
      affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();

      triggers.diabolic_ritual = triggers.demonic_art = p->hero.diabolic_ritual.ok();

      base_dd_multiplier *= 1.0 + p->talents.improved_chaos_bolt->effectN( 1 ).percent();

      if ( p->talents.internal_combustion.ok() )
      {
        internal_combustion = new internal_combustion_t( p );
        add_child( internal_combustion );
      }
    }

    timespan_t execute_time_flat_modifier() const override
    {
      timespan_t m = warlock_spell_t::execute_time_flat_modifier();

      m += p()->talents.improved_chaos_bolt->effectN( 2 ).time_value();

      return m;
    }

    double cost_pct_multiplier() const override
    {
      double c = warlock_spell_t::cost_pct_multiplier();

      if ( p()->buffs.ritual_of_ruin->check() )
        c *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 2 ).percent();

      return c;
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      // 2022-10-15: Backdraft is not consumed for Ritual of Ruin empowered casts, but IS hasted by it
      if ( p()->buffs.ritual_of_ruin->check() )
        m *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 3 ).percent();

      return m;
    }

    bool ready() override
    {
      if ( p()->hero.ruination.ok() && p()->executing != this && p()->buffs.ruination->check() )
        return false;

      return warlock_spell_t::ready();
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      if ( p()->buffs.crashing_chaos->check() )
        m *= 1.0 + p()->talents.crashing_chaos->effectN( 1 ).percent();

      if ( p()->talents.indiscriminate_flames.ok() && p()->buffs.backdraft->check() )
        m *= 1.0 + p()->talents.indiscriminate_flames->effectN( 1 ).percent();

      return m;
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.internal_combustion.ok() && result_is_hit( s->result ) && ( td( s->target )->dots_immolate->is_ticking() || td( s->target )->dots_wither->is_ticking() ) )
        internal_combustion->execute_on_target( s->target );

      if ( p()->talents.eradication.ok() && result_is_hit( s->result ) )
        td( s->target )->debuffs_eradication->trigger();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      // 2022-10-15: Backdraft is not consumed for Ritual of Ruin empowered casts, but IS hasted by it
      if ( !p()->buffs.ritual_of_ruin->check() )
        p()->buffs.backdraft->decrement();

      if ( p()->talents.avatar_of_destruction.ok() && p()->buffs.ritual_of_ruin->check() )
      {
        p()->warlock_pet_list.overfiends.spawn();
        p()->buffs.summon_overfiend->trigger();
      }

      p()->buffs.ritual_of_ruin->expire();

      p()->buffs.crashing_chaos->decrement();

      if ( p()->talents.burn_to_ashes.ok() )
        p()->buffs.burn_to_ashes->trigger( as<int>( p()->talents.burn_to_ashes->effectN( 3 ).base_value() ) );
    }

    double composite_crit_chance() const override
    { return 1.0; }

    double calculate_direct_amount( action_state_t* s ) const override
    {
      warlock_spell_t::calculate_direct_amount( s );

      s->result_total *= 1.0 + player->cache.spell_crit_chance();

      return s->result_total;
    }
  };

  struct conflagrate_t : public warlock_spell_t
  {
    conflagrate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Conflagrate", p, p->talents.conflagrate, options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      energize_type = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount = ( p->talents.conflagrate_2->effectN( 1 ).base_value() ) / 10.0;

      cooldown->hasted = true;
      cooldown->charges += as<int>( p->talents.improved_conflagrate->effectN( 1 ).base_value() );
      cooldown->duration += p->talents.explosive_potential->effectN( 1 ).time_value();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.roaring_blaze.ok() && result_is_hit( s->result ) )
        td( s->target )->debuffs_conflagrate->trigger();

      if ( active_4pc( TWW1 ) && s->result == RESULT_CRIT )
      {
        p()->buffs.echo_of_the_azjaqir->trigger();
        p()->procs.echo_of_the_azjaqir->occur();
      }
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.conflagration_of_chaos_cf->expire();

      if ( p()->talents.conflagration_of_chaos.ok() && rng().roll( p()->talents.conflagration_of_chaos->effectN( 1 ).percent() ) )
      {
        p()->buffs.conflagration_of_chaos_cf->trigger();
        p()->procs.conflagration_of_chaos_cf->occur();
      }

      if ( p()->talents.backdraft.ok() )
        p()->buffs.backdraft->trigger();
    }

    double composite_crit_chance() const override
    {
      double c = warlock_spell_t::composite_crit_chance();

      c += p()->buffs.conflagration_of_chaos_cf->check_value();

      return c;
    }

    double composite_crit_chance_multiplier() const override
    {
      double m = warlock_spell_t::composite_crit_chance_multiplier();

      if ( active_2pc( TWW1 ) )
        m *= 1.0 + p()->tier.hexflame_destro_2pc->effectN( 2 ).percent();

      return m;
    }

    double calculate_direct_amount( action_state_t* s ) const override
    {
      double amt = warlock_spell_t::calculate_direct_amount( s );

      if ( p()->buffs.conflagration_of_chaos_cf->check() )
      {
        s->result_total *= 1.0 + player->cache.spell_crit_chance();
        return s->result_total;
      }

      return amt;
    }
  };

  struct rain_of_fire_t : public warlock_spell_t
  {
    struct rain_of_fire_tick_t : public warlock_spell_t
    {
      rain_of_fire_tick_t( warlock_t* p )
        : warlock_spell_t( "Rain of Fire (tick)", p, p->talents.rain_of_fire_tick )
      {
        background = dual = true;
        aoe = -1;
        radius = p->talents.rain_of_fire->effectN( 1 ).radius();

        affected_by.chaotic_energies = true;
        affected_by.chaos_incarnate = p->talents.chaos_incarnate.ok();
        affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();

        base_multiplier *= 1.0 + p->talents.inferno->effectN( 2 ).percent();

        triggers.decimation = false;
      }
      
      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( p()->talents.pyrogenics.ok() )
          td( s->target )->debuffs_pyrogenics->trigger();
      }

      double composite_persistent_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_persistent_multiplier( s );

        if ( p()->buffs.crashing_chaos->check() )
          m *= 1.0 + p()->talents.crashing_chaos->effectN( 2 ).percent();

        return m;
      }
    };

    rain_of_fire_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Rain of Fire", p, p->talents.rain_of_fire, options_str )
    {
      may_miss = may_crit = false;
      base_tick_time = 1_s;
      dot_duration = 0_s;
      aoe = -1; // Needed to apply Pyrogenics

      affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();

      triggers.diabolic_ritual = triggers.demonic_art = p->hero.diabolic_ritual.ok();

      base_costs[ RESOURCE_SOUL_SHARD ] += p->talents.inferno->effectN( 1 ).base_value() / 10.0;

      if ( !p->proc_actions.rain_of_fire_tick )
      {
        p->proc_actions.rain_of_fire_tick = new rain_of_fire_tick_t( p );
        p->proc_actions.rain_of_fire_tick->stats = stats;
      }
    }

    double cost_pct_multiplier() const override
    {
      double c = warlock_spell_t::cost_pct_multiplier();

      if ( p()->buffs.ritual_of_ruin->check() )
        c *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 5 ).percent();

      return c;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.burn_to_ashes.ok() )
        p()->buffs.burn_to_ashes->trigger( as<int>( p()->talents.burn_to_ashes->effectN( 3 ).base_value() ) );

      make_event<ground_aoe_event_t>( *sim, p(), 
                                      ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time * player->cache.spell_haste() )
                                        .duration( p()->talents.rain_of_fire->duration() * player->cache.spell_haste() )
                                        .start_time( sim->current_time() )
                                        .action( p()->proc_actions.rain_of_fire_tick ) );

      if ( p()->talents.avatar_of_destruction.ok() && p()->buffs.ritual_of_ruin->check() )
      {
        p()->warlock_pet_list.overfiends.spawn();
        p()->buffs.summon_overfiend->trigger();
      }

      p()->buffs.ritual_of_ruin->expire();

      p()->buffs.crashing_chaos->decrement();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.pyrogenics.ok() )
        td( s->target )->debuffs_pyrogenics->trigger();
    }
  };

  struct havoc_t : public warlock_spell_t
  {
    havoc_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Havoc", p, p->talents.havoc, options_str )
    { may_crit = false; }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      td( s->target )->debuffs_havoc->trigger();
    }
  };

  struct cataclysm_t : public warlock_spell_t
  {
    action_t* applied_dot;

    cataclysm_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Cataclysm", p, p->talents.cataclysm, options_str )
    {
      aoe = -1;

      affected_by.chaotic_energies = true;

      if ( p->hero.wither.ok() )
        applied_dot = new wither_t( p, "" );
      else
        applied_dot = new immolate_t( p, "" );

      applied_dot->background = true;
      applied_dot->dual = true;
      applied_dot->base_costs[ RESOURCE_MANA ] = 0;
      applied_dot->base_dd_multiplier = 0.0;
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
        applied_dot->execute_on_target( s->target );
    }
  };

  struct shadowburn_t : public warlock_spell_t
  {
    shadowburn_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Shadowburn", p, p->talents.shadowburn, options_str )
    {
      cooldown->hasted = true;
      
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;
      affected_by.ashen_remains = p->talents.ashen_remains.ok();
      affected_by.chaos_incarnate = p->talents.chaos_incarnate.ok();
      affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();

      triggers.diabolic_ritual = triggers.demonic_art = p->hero.diabolic_ritual.ok();

      base_dd_multiplier *= 1.0 + p->talents.blistering_atrophy->effectN( 1 ).percent();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        td( s->target )->debuffs_shadowburn->trigger();

        if ( p()->talents.eradication.ok() )
          td( s->target )->debuffs_eradication->trigger();

        // Fiendish Cruelty checks for state after damage is applied
        if ( p()->talents.fiendish_cruelty.ok() && s->target->health_percentage() <= p()->talents.fiendish_cruelty->effectN( 2 ).base_value() )
          make_event( *sim, 0_ms, [ this ] { p()->cooldowns.shadowburn->adjust( timespan_t::from_seconds( -p()->talents.fiendish_cruelty->effectN( 1 ).base_value() ) ); } );
      }
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.conflagration_of_chaos_sb->expire();

      if ( p()->talents.conflagration_of_chaos.ok() && rng().roll( p()->talents.conflagration_of_chaos->effectN( 1 ).percent() ) )
      {
        p()->buffs.conflagration_of_chaos_sb->trigger();
        p()->procs.conflagration_of_chaos_sb->occur();
      }

      if ( p()->talents.burn_to_ashes.ok() )
        p()->buffs.burn_to_ashes->trigger( as<int>( p()->talents.burn_to_ashes->effectN( 4 ).base_value() ) );
    }

    double composite_target_crit_chance( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_crit_chance( t );

      if ( target->health_percentage() <= p()->talents.shadowburn->effectN( 4 ).base_value() )
        m += p()->talents.shadowburn->effectN( 3 ).percent();

      if ( target->health_percentage() <= p()->talents.blistering_atrophy->effectN( 4 ).base_value() )
        m += p()->talents.blistering_atrophy->effectN( 3 ).percent();

      return m;
    }

    double composite_crit_chance() const override
    {
      double c = warlock_spell_t::composite_crit_chance();

      c += p()->buffs.conflagration_of_chaos_sb->check_value();

      return c;
    }

    double calculate_direct_amount( action_state_t* state ) const override
    {
      double amt = warlock_spell_t::calculate_direct_amount( state );

      if ( p()->buffs.conflagration_of_chaos_sb->check() )
      {
        state->result_total *= 1.0 + player->cache.spell_crit_chance();
        return state->result_total;
      }

      return amt;
    }
  };

  struct channel_demonfire_t : public warlock_spell_t
  {
    struct channel_demonfire_tick_t : public warlock_spell_t
    {
      channel_demonfire_tick_t( warlock_t* p )
        : warlock_spell_t( "Channel Demonfire (tick)", p, p->talents.channel_demonfire_tick )
      {
        background = dual = true;
        may_miss = false;
        aoe = -1;
        travel_speed = p->talents.channel_demonfire_travel->missile_speed();

        affected_by.chaotic_energies = true;

        triggers.decimation = false;

        spell_power_mod.direct = p->talents.channel_demonfire_tick->effectN( 1 ).sp_coeff();

        base_dd_multiplier *= 1.0 + p->talents.demonfire_mastery->effectN( 1 ).percent();
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( p()->talents.raging_demonfire.ok() && td( s->target )->dots_immolate->is_ticking() )
          td( s->target )->dots_immolate->adjust_duration( p()->talents.raging_demonfire->effectN( 2 ).time_value() );

        if ( p()->talents.raging_demonfire.ok() && td( s->target )->dots_wither->is_ticking() )
          td( s->target )->dots_wither->adjust_duration( p()->talents.raging_demonfire->effectN( 2 ).time_value() );
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_da_multiplier( s );

        if ( s->chain_target != 0 )
          m *= p()->talents.channel_demonfire_tick->effectN( 2 ).sp_coeff() / p()->talents.channel_demonfire_tick->effectN( 1 ).sp_coeff();

        return m;
      }
    };

    channel_demonfire_tick_t* channel_demonfire_tick;

    channel_demonfire_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Channel Demonfire", p, p->talents.channel_demonfire, options_str ),
      channel_demonfire_tick( new channel_demonfire_tick_t( p ) )
    {
      channeled = true;
      hasted_ticks = true;
      may_crit = false;
      cooldown->hasted = true;

      add_child( channel_demonfire_tick );

      if ( p->talents.demonfire_mastery.ok() )
      {
        base_tick_time *= 1.0 + p->talents.demonfire_mastery->effectN( 2 ).percent();
        dot_duration *= 1.0 + p->talents.demonfire_mastery->effectN( 3 ).percent();
      }

      if ( p->talents.raging_demonfire.ok() )
      {
        int num_ticks = as<int>( dot_duration / base_tick_time + p->talents.raging_demonfire->effectN( 1 ).base_value() );
        base_tick_time *= 1.0 + p->talents.raging_demonfire->effectN( 3 ).percent();
        dot_duration = num_ticks * base_tick_time;
      }
    }

    std::vector<player_t*>& target_list() const override
    {
      target_cache.list = warlock_spell_t::target_list();

      size_t i = target_cache.list.size();
      while ( i > 0 )
      {
        i--;

        if ( !td( target_cache.list[ i ] )->dots_immolate->is_ticking() && !td( target_cache.list[ i ] )->dots_wither->is_ticking() )
          target_cache.list.erase( target_cache.list.begin() + i );
      }

      return target_cache.list;
    }

    void tick( dot_t* d ) override
    {
      target_cache.is_valid = false;

      const auto& targets = target_list();

      if ( !targets.empty() )
      {
        channel_demonfire_tick->set_target( targets[ rng().range( size_t(), targets.size() ) ] );
        channel_demonfire_tick->execute();
      }

      warlock_spell_t::tick( d );
    }

    bool ready() override
    {
      if ( p()->get_active_dots( td( target )->dots_immolate ) == 0 && p()->get_active_dots( td( target )->dots_wither ) == 0 )
        return false;

      return warlock_spell_t::ready();
    }
  };

  struct dimensional_rift_t : public warlock_spell_t
  {
    dimensional_rift_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Dimensional Rift", p, p->talents.dimensional_rift, options_str )
    {
      harmful = true;

      energize_type = action_energize::ON_CAST;
      energize_amount = p->talents.dimensional_rift->effectN( 2 ).base_value() / 10.0;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      int rift = rng().range( 3 );

      switch ( rift )
      {
      case 0:
        p()->warlock_pet_list.shadow_rifts.spawn( p()->talents.shadowy_tear_summon->duration() );
        break;
      case 1:
        p()->warlock_pet_list.unstable_rifts.spawn( p()->talents.unstable_tear_summon->duration() );
        break;
      case 2:
        p()->warlock_pet_list.chaos_rifts.spawn( p()->talents.chaos_tear_summon->duration() );
        break;
      default:
        break;
      }
    }
  };

  struct infernal_awakening_t : public warlock_spell_t
  {
    infernal_awakening_t( warlock_t* p )
      : warlock_spell_t( "Infernal Awakening", p, p->talents.infernal_awakening )
    {
      background = dual = true;
      aoe = -1;

      triggers.decimation = false;
    }

    void execute() override
    {
      warlock_spell_t::execute();
      
      p()->warlock_pet_list.infernals.spawn();
    }
  };

  struct summon_infernal_t : public warlock_spell_t
  {
    summon_infernal_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Summon Infernal", p, p->talents.summon_infernal, options_str )
    {
      may_crit = false;
      resource_current = RESOURCE_SOUL_SHARD; // For Cruelty of Kerxan proccing

      triggers.diabolic_ritual = p->hero.cruelty_of_kerxan.ok();

      impact_action = new infernal_awakening_t( p );
      add_child( impact_action );
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.crashing_chaos.ok() )
        p()->buffs.crashing_chaos->trigger();

      if ( p()->talents.rain_of_chaos.ok() )
        p()->buffs.rain_of_chaos->trigger();

      if ( p()->hero.cruelty_of_kerxan.ok() )
      {
        timespan_t reduction = -p()->hero.cruelty_of_kerxan->effectN( 1 ).time_value();

        p()->buffs.ritual_overlord->extend_duration( p(), reduction );
        p()->buffs.ritual_mother->extend_duration( p(), reduction );
        p()->buffs.ritual_pit_lord->extend_duration( p(), reduction );
      }
    }
  };

  struct soul_fire_t : public warlock_spell_t
  {
    action_t* applied_dot;

    soul_fire_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Soul Fire", p, p->talents.soul_fire, options_str )
    {
      energize_type = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount = ( p->talents.soul_fire_2->effectN( 1 ).base_value() ) / 10.0;

      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      if ( p->hero.wither.ok() )
        applied_dot = new wither_t( p, "" );
      else
        applied_dot = new immolate_t( p, "" );

      applied_dot->background = true;
      applied_dot->dual = true;
      applied_dot->base_costs[ RESOURCE_MANA ] = 0;
      applied_dot->base_dd_multiplier = 0.0;
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      m *= 1.0 + p()->buffs.decimation->check_value();

      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      applied_dot->execute_on_target( target );

      p()->buffs.backdraft->decrement();

      p()->buffs.decimation->decrement();
    }

    double composite_crit_chance() const override
    {
      double c = warlock_spell_t::composite_crit_chance();

      if ( p()->talents.indiscriminate_flames.ok() && p()->buffs.backdraft->check() )
        c += p()->talents.indiscriminate_flames->effectN( 2 ).percent();

      return c;
    }
  };

  // Destruction Actions End
  // Diabolist Actions Begin
  
  struct infernal_bolt_t : public warlock_spell_t
  {
    infernal_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Infernal Bolt", p, p->hero.infernal_bolt, options_str )
    {
      energize_type = action_energize::ON_CAST;
      energize_amount += p->warlock_base.destruction_warlock->effectN( 9 ).base_value() / 10.0;

      affected_by.havoc = true;
      affected_by.ashen_remains = p->talents.ashen_remains.ok();

      if ( demonology() )
      {
        base_dd_multiplier *= 1.0 + p->talents.sargerei_technique->effectN( 1 ).percent();
        base_dd_multiplier *= 1.0 + p->talents.rune_of_shadows->effectN( 3 ).percent();

        if ( active_2pc( TWW1 ) )
          base_dd_multiplier *= 1.0 + p->tier.hexflame_demo_2pc->effectN( 2 ).percent();
      }

      if ( destruction() )
        base_dd_multiplier *= 1.0 + p->talents.sargerei_technique->effectN( 2 ).percent();
    }

    bool ready() override
    {
      if ( !p()->buffs.infernal_bolt->check() && p()->executing != this )
        return false;

      return warlock_spell_t::ready();
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      m *= 1.0 + p()->talents.rune_of_shadows->effectN( 2 ).percent();

      m *= 1.0 + p()->talents.emberstorm->effectN( 2 ).percent();

      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.demonic_calling.ok() )
        p()->buffs.demonic_calling->trigger();

      p()->buffs.burn_to_ashes->decrement();
      p()->buffs.infernal_bolt->decrement();
    }

    double composite_crit_chance() const override
    {
      double c = warlock_spell_t::composite_crit_chance();

      if ( p()->talents.indiscriminate_flames.ok() && p()->buffs.backdraft->check() )
        c += p()->talents.indiscriminate_flames->effectN( 2 ).percent();

      return c;
    }

    double composite_crit_chance_multiplier() const override
    {
      double m = warlock_spell_t::composite_crit_chance_multiplier();

      if ( destruction() && active_2pc( TWW1 ) )
        m *= 1.0 + p()->tier.hexflame_destro_2pc->effectN( 1 ).percent();

      return m;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      m *= 1.0 + p()->buffs.burn_to_ashes->check_value();

      return m;
    }
  };

  struct ruination_t : public warlock_spell_t
  {
    struct ruination_impact_t : public warlock_spell_t
    {
      ruination_impact_t( warlock_t* p )
        : warlock_spell_t( "Ruination (Impact)", p, p->hero.ruination_impact )
      {
        background = dual = true;
        aoe = -1;
        reduced_aoe_targets = p->hero.ruination_cast->effectN( 2 ).base_value();

        affected_by.chaotic_energies = true;
        affected_by.backdraft = true;
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( result_is_hit( s->result ) && s->target == target )
        {
          if ( demonology() )
          {
            for ( int i = 1; i <= as<int>( p()->hero.ruination_buff->effectN( 2 ).base_value() ); i++ )
            {
              auto ev = make_event<imp_delay_event_t>( *sim, p(), rng().gauss( 180.0 * i, 25.0 ), 180.0 * i );
              p()->wild_imp_spawns.push_back( ev );
            }
          }

          if ( destruction() )
          {
            p()->warlock_pet_list.diabolic_imps.spawn( as<int>( p()->hero.ruination_buff->effectN( 3 ).base_value() ) );
          }
        }
      }
    };

    ruination_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Ruination", p, p->hero.ruination_cast, options_str )
    {
      impact_action = new ruination_impact_t( p );
      add_child( impact_action );
    }

    bool ready() override
    {
      if ( !p()->buffs.ruination->check() && p()->executing != this )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.ruination->decrement();
    }
  };

  // Diabolist Actions End
  // Helper Functions Begin

  // Event for triggering delayed refunds from Soul Conduit
  // Delay prevents instant reaction time issues for rng refunds
  sc_event_t::sc_event_t( warlock_t* p, int c )
    : player_event_t( *p, 100_ms ),
    shard_gain( p->gains.soul_conduit ),
    pl( p ),
    shards_used( c )
  { }

  const char* sc_event_t::name() const
  { return "soul_conduit_event"; }

  void sc_event_t::execute()
  {
    double soul_conduit_rng = 0.0;

    if ( pl->specialization() == WARLOCK_AFFLICTION )
      soul_conduit_rng = pl->talents.soul_conduit->effectN( 1 ).percent();

    if ( pl->specialization() == WARLOCK_DEMONOLOGY )
      soul_conduit_rng = pl->talents.soul_conduit->effectN( 2 ).percent();

    if ( pl->specialization() == WARLOCK_DESTRUCTION )
      soul_conduit_rng = pl->talents.soul_conduit->effectN( 3 ).percent();

    for ( int i = 0; i < shards_used; i++ )
    {
      if ( rng().roll( soul_conduit_rng ) )
      {
        pl->sim->print_log( "Soul Conduit proc occurred for Warlock {}, refunding 1.0 soul shards.", pl->name() );
        pl->resource_gain( RESOURCE_SOUL_SHARD, 1.0, shard_gain );
        pl->procs.soul_conduit->occur();
      }
    }
  }

  // Checks whether Tormented Crescendo conditions are met
  bool helpers::crescendo_check( warlock_t* p )
  {
    bool agony = false;
    bool corruption = false;
    for ( const auto target : p->sim->target_non_sleeping_list )
    {
      warlock_td_t* td = p->get_target_data( target );
      if ( !td )
        continue;

      agony = agony || td->dots_agony->is_ticking();

      if ( p->hero.wither.ok() )
        corruption = corruption || td->dots_wither->is_ticking();
      else
        corruption = corruption || td->dots_corruption->is_ticking();

      if ( agony && corruption )
        break;
    }

    return agony && corruption && ( p->ua_target && p->get_target_data( p->ua_target )->dots_unstable_affliction->is_ticking() );
  }

  void helpers::nightfall_updater( warlock_t* p, dot_t* d )
  {
    // Blizzard did not publicly release how nightfall was changed.
    // We determined this is the probable functionality copied from Agony by first confirming the
    // DR formula was the same and then confirming that you can get procs on 1st tick.
    // The procs also have a regularity that suggest it does not use a proc chance or rppm.
    // Last checked 09-28-2020.
    double increment_max = p->rng_settings.nightfall.setting_value;

    double active_corruptions = p->get_active_dots( d );
    increment_max *= std::pow( active_corruptions, -2.0 / 3.0 );

    p->corruption_accumulator += p->rng().range( 0.0, increment_max );

    if ( p->corruption_accumulator >= 1 )
    {
      p->procs.nightfall->occur();
      p->buffs.nightfall->trigger();
      p->corruption_accumulator -= 1.0;
    }
  }

  void helpers::trigger_blackened_soul( warlock_t* p, bool malevolence )
  {
    for ( const auto target : p->sim->target_non_sleeping_list )
    {
      warlock_td_t* tdata = p->get_target_data( target );
      if ( !tdata )
        continue;

      if ( !tdata->dots_wither->is_ticking() )
        continue;

      tdata->dots_wither->increment( malevolence ? as<int>( p->hero.malevolence->effectN( 1 ).base_value() ) : 1 );

      if ( p->buffs.malevolence->check() && !malevolence )
        tdata->dots_wither->increment( as<int>( p->hero.malevolence->effectN( 2 ).base_value() ) );

      // TOCHECK: Chance for this effect is not in spell data!
      if ( p->hero.bleakheart_tactics.ok() && p->rng().roll( p->rng_settings.bleakheart_tactics.setting_value ) )
      {
        tdata->dots_wither->increment( 1 );
        p->procs.bleakheart_tactics->occur();
      }

      bool collapse = p->buffs.malevolence->check();
      collapse = collapse || ( p->hero.seeds_of_their_demise.ok() && target->health_percentage() <= p->hero.seeds_of_their_demise->effectN( 2 ).base_value() ) ;
      collapse = collapse || ( p->hero.seeds_of_their_demise.ok() && tdata->dots_wither->current_stack() >= as<int>( p->hero.seeds_of_their_demise->effectN( 1 ).base_value() ) );

      if ( collapse )
      {
        tdata->debuffs_blackened_soul->trigger();
      }
      else if ( p->rng().roll( p->rng_settings.blackened_soul.setting_value ) )
      {
        // TOCHECK: Chance for this effect is not in spell data!
        tdata->debuffs_blackened_soul->trigger();
        p->procs.blackened_soul->occur();
      }

      if ( malevolence )
        p->proc_actions.malevolence->execute_on_target( target );
    }
  }

  // Event for spawning Wild Imps for Demonology
  imp_delay_event_t::imp_delay_event_t( warlock_t* p, double delay, double exp ) : player_event_t( *p, timespan_t::from_millis( delay ) )
  { diff = timespan_t::from_millis( exp - delay ); }

  const char* imp_delay_event_t::name() const
  { return "imp_delay"; }

  void imp_delay_event_t::execute()
  {
    warlock_t* p = static_cast<warlock_t*>( player() );

    p->warlock_pet_list.wild_imps.spawn();

    // Remove this event from the vector
    auto it = std::find( p->wild_imp_spawns.begin(), p->wild_imp_spawns.end(), this );
    if ( it != p->wild_imp_spawns.end() )
      p->wild_imp_spawns.erase( it );
  }

  // Used for APL expressions to estimate when imp is "supposed" to spawn
  timespan_t imp_delay_event_t::expected_time()
  { return std::max( 0_ms, this->remains() + diff ); }

  // Helper Functions End
  
  // Action Creation Begin

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

    if ( action_t* diabolist_action = create_action_diabolist( action_name, options_str ) )
      return diabolist_action;

    if ( action_t* hellcaller_action = create_action_hellcaller( action_name, options_str ) )
      return hellcaller_action;

    if ( action_t* soul_harvester_action = create_action_soul_harvester( action_name, options_str ) )
      return soul_harvester_action;

    if ( action_t* generic_action = create_action_warlock( action_name, options_str ) )
      return generic_action;

    return player_t::create_action( action_name, options_str );
  }

  action_t* warlock_t::create_action_warlock( util::string_view action_name, util::string_view options_str )
  {
    if ( ( action_name == "summon_pet" ) && default_pet.empty() )
    {
      sim->errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
      return nullptr;
    }

    // Pets
    if ( action_name == "summon_felhunter" )
      return new summon_main_pet_t( "felhunter", this );
    if ( action_name == "summon_felguard" && specialization() == WARLOCK_DEMONOLOGY )
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

    // Shared Spells
    if ( action_name == "drain_life" )
      return new drain_life_t( this, options_str );
    if ( action_name == "corruption" && specialization() != WARLOCK_DESTRUCTION )
      return new corruption_t( this, options_str, false );
    if ( action_name == "shadow_bolt" && specialization() != WARLOCK_DESTRUCTION )
      return new shadow_bolt_t( this, options_str );
    if ( action_name == "grimoire_of_sacrifice" && specialization() != WARLOCK_DEMONOLOGY )
      return new grimoire_of_sacrifice_t( this, options_str );
    if ( action_name == "interrupt" )
      return new interrupt_t( action_name, this, options_str );
    if ( action_name == "soulburn" )
      return new soulburn_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_affliction( util::string_view action_name, util::string_view options_str )
  {
    if ( action_name == "agony" )
      return new agony_t( this, options_str );
    if ( action_name == "unstable_affliction" )
      return new unstable_affliction_t( this, options_str );
    if ( action_name == "summon_darkglare" )
      return new summon_darkglare_t( this, options_str );
    if ( action_name == "drain_soul" )
      return new drain_soul_t( this, options_str );
    if ( action_name == "haunt" )
      return new haunt_t( this, options_str );
    if ( action_name == "phantom_singularity" )
      return new phantom_singularity_t( this, options_str );
    if ( action_name == "vile_taint" )
      return new vile_taint_t( this, options_str );
    if ( action_name == "malefic_rapture" )
      return new malefic_rapture_t( this, options_str );
    if ( action_name == "soul_rot" )
      return new soul_rot_t( this, options_str );
    if ( action_name == "seed_of_corruption" )
      return new seed_of_corruption_t( this, options_str );
    if ( action_name == "oblivion" )
      return new oblivion_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_demonology( util::string_view action_name, util::string_view options_str )
  {
    if ( action_name == "demonbolt" )
      return new demonbolt_t( this, options_str );
    if ( action_name == "hand_of_guldan" )
      return new hand_of_guldan_t( this, options_str );
    if ( action_name == "implosion" )
      return new implosion_t( this, options_str );
    if ( action_name == "demonic_strength" )
      return new demonic_strength_t( this, options_str );
    if ( action_name == "bilescourge_bombers" )
      return new bilescourge_bombers_t( this, options_str );
    if ( action_name == "power_siphon" )
      return new power_siphon_t( this, options_str );
    if ( action_name == "call_dreadstalkers" )
      return new call_dreadstalkers_t( this, options_str );
    if ( action_name == "summon_demonic_tyrant" )
      return new summon_demonic_tyrant_t( this, options_str );
    if ( action_name == "summon_vilefiend" )
      return new summon_vilefiend_t( this, options_str );
    if ( action_name == "grimoire_felguard" )
      return new grimoire_felguard_t( this, options_str );
    if ( action_name == "guillotine" )
      return new guillotine_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_destruction( util::string_view action_name, util::string_view options_str )
  {
    if ( action_name == "conflagrate" )
      return new conflagrate_t( this, options_str );
    if ( action_name == "incinerate" )
      return new incinerate_t( this, options_str );
    if ( action_name == "immolate" )
      return new immolate_t( this, options_str );
    if ( action_name == "chaos_bolt" )
      return new chaos_bolt_t( this, options_str );
    if ( action_name == "rain_of_fire" )
      return new rain_of_fire_t( this, options_str );
    if ( action_name == "havoc" )
      return new havoc_t( this, options_str );
    if ( action_name == "summon_infernal" )
      return new summon_infernal_t( this, options_str );
    if ( action_name == "soul_fire" )
      return new soul_fire_t( this, options_str );
    if ( action_name == "shadowburn" )
      return new shadowburn_t( this, options_str );
    if ( action_name == "cataclysm" )
      return new cataclysm_t( this, options_str );
    if ( action_name == "channel_demonfire" )
      return new channel_demonfire_t( this, options_str );
    if ( action_name == "dimensional_rift" )
      return new dimensional_rift_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_diabolist( util::string_view action_name, util::string_view options_str )
  {
    if ( action_name == "infernal_bolt" )
      return new infernal_bolt_t( this, options_str );

    if ( action_name == "ruination" )
      return new ruination_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_hellcaller( util::string_view action_name, util::string_view options_str )
  {
    if ( action_name == "wither" )
      return new wither_t( this, options_str );

    if ( action_name == "malevolence" )
      return new malevolence_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_soul_harvester( util::string_view action_name, util::string_view options_str )
  {
    return nullptr;
  }

  void warlock_t::create_actions()
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      create_affliction_proc_actions();

    if ( specialization() == WARLOCK_DEMONOLOGY )
      create_demonology_proc_actions();

    if ( specialization() == WARLOCK_DESTRUCTION )
      create_destruction_proc_actions();

    create_diabolist_proc_actions();

    create_hellcaller_proc_actions();

    create_soul_harvester_proc_actions();

    player_t::create_actions();
  }

  void warlock_t::create_affliction_proc_actions()
  { }

  void warlock_t::create_demonology_proc_actions()
  {
    proc_actions.bilescourge_bombers_proc = new bilescourge_bombers_proc_t( this );
    proc_actions.doom_proc = new doom_t( this );
  }

  void warlock_t::create_destruction_proc_actions()
  { }

  void warlock_t::create_diabolist_proc_actions()
  { }

  void warlock_t::create_hellcaller_proc_actions()
  {
    proc_actions.blackened_soul = new blackened_soul_t( this );
    proc_actions.malevolence = new malevolence_damage_t( this );
  }

  void warlock_t::create_soul_harvester_proc_actions()
  {
    proc_actions.demonic_soul = new demonic_soul_t( this );
    proc_actions.shared_fate = new shared_fate_t( this );
    proc_actions.wicked_reaping = new wicked_reaping_t( this );
  }

  void warlock_t::init_special_effects()
  {
    player_t::init_special_effects();

    if ( talents.grimoire_of_sacrifice.ok() )
    {
      auto const sac_effect = new special_effect_t( this );
      sac_effect->name_str = "grimoire_of_sacrifice_effect";
      sac_effect->spell_id = talents.grimoire_of_sacrifice_buff->id();
      sac_effect->execute_action = new grimoire_of_sacrifice_damage_t( this );
      special_effects.push_back( sac_effect );

      auto cb = new dbc_proc_callback_t( this, *sac_effect );

      cb->initialize();
      cb->deactivate();

      buffs.grimoire_of_sacrifice->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ){
          if ( new_ == 1 ) cb->activate();
          else cb->deactivate();
        } );
    }
  }

  // Action Creation End

} //namespace warlock
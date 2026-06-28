#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

#include "sc_warlock_pets.hpp"
#include "util/util.hpp"

namespace warlock
{
using namespace helpers;
  struct warlock_spell_t : public parse_action_effects_t<spell_t>
  {
    using action_base_t = parse_action_effects_t<spell_t>;
    using base_t = warlock_spell_t;

    struct affected_by_t
    {
      // Class

      // Affliction
      bool deaths_embrace = false;

      // Demonology
      bool sacrificed_souls = false;

      // Destruction
      bool chaotic_energies = false;
      bool havoc = false;
      bool chaos_incarnate = false;

      // Diabolist
      bool touch_of_rancora = false;
      bool touch_of_rancora_casted = false;
    } affected_by;

    struct triggers_t
    {
      // Class

      // Affliction
      bool ravenous_afflictions = false;

      // Destruction
      bool fiendish_cruelty = false;
      bool dimensional_rift = false;
      bool embers_of_nihilam_1 = false;

      // Diabolist
      bool diabolic_ritual = false;
      bool demonic_art = false;
      bool demonic_art_buff = false;
      bool rancora_cb_bonus = false;
    } triggers;

    warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : parse_action_effects_t<spell_t>( token, p, s ),
      affected_by(),
      triggers()
    {
      if ( !this->data().flags( spell_attribute::SX_CANNOT_CRIT ) && this->harmful )
        this->may_crit = true;

      if ( this->data().flags( spell_attribute::SX_TICK_MAY_CRIT ) )
        this->tick_may_crit = true;

      weapon_multiplier = 0.0;

      if ( this->data().ok() )
      {
        apply_action_effects();

        if ( this->type == action_e::ACTION_SPELL || this->type == action_e::ACTION_ATTACK )
          apply_target_effects();

        if ( this->data().flags( spell_attribute::SX_ABILITY ) || this->trigger_gcd > 0_ms )
          this->not_a_proc = true;
      }
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

    template <typename T>
    target_filter_callback_t dot_or_debuff_only( T d )
    {
      return [ this, d ]( const action_t*, player_t* t ) {
        return p()->dot_or_debuff_active( d, p()->get_target_data( t ) );
      };
    }

    target_filter_callback_t primary_target_or( target_filter_callback_t secondary_filter )
    {
      return [ secondary_filter = std::move( secondary_filter ) ]( const action_t* a, player_t* t ) {
        return t == a->target || secondary_filter( a, t );
      };
    }

    target_filter_callback_t immolate_or_wither_only()
    {
      return [ this ]( const action_t*, player_t* t ) {
        return td( t )->dots.immolate->is_ticking() || td( t )->dots.wither->is_ticking();
      };
    }

    target_filter_callback_t corruption_or_wither_only()
    {
      return [ this ]( const action_t*, player_t* t ) {
        return td( t )->dots.corruption->is_ticking() || td( t )->dots.wither->is_ticking();
      };
    }

    target_filter_callback_t affliction_core_dots_only()
    {
      return [ this ]( const action_t*, player_t* t ) {
        return td( t )->dots.corruption->is_ticking() || td( t )->dots.wither->is_ticking()
               || td( t )->dots.agony->is_ticking() || td( t )->dots.unstable_affliction->is_ticking();
      };
    }

    void reset() override
    { action_base_t::reset(); }

    void trigger_extra_tick( dot_t* dot, double multiplier, warlock_spell_t* tick_action, bool stack_multiplier = true )
    {
      if ( !tick_action || !dot->is_ticking() )
        return;

      // The extra tick is its own spell with its own modifiers and only inherits the base damage
      // Calculate the base tick damage (without modifiers) based on how 'calculate_tick_amount' does it
      assert( dot->current_action && "Extra DoT Tick has no action" );
      double tick_dmg = dot->current_action->base_ta( dot->state );
      tick_dmg = std::round( tick_dmg * 1000 ) * 0.001;
      tick_dmg += dot->current_action->bonus_ta( dot->state );
      tick_dmg += dot->state->composite_spell_power() * dot->current_action->spell_tick_power_coefficient( dot->state );
      tick_dmg += dot->state->composite_attack_power() * dot->current_action->attack_tick_power_coefficient( dot->state );

      if ( !sim->average_range )
        tick_dmg = floor( tick_dmg + rng().real() );

      if ( stack_multiplier )
        tick_dmg *= dot->current_stack();

      tick_dmg *= multiplier;

      tick_action->execute_on_target( dot->target, tick_dmg );
    }

    /* ----------------------------------------------------------
    * NOTE NOTE NOTE
    * Applies DYNAMIC (Buffs, Debuffs, DoTs, or anything else that could change state during combat)
    * effects that effect the action.
    * NEEDS TO BE A WHITELIST EFFECT!
    * NOTE NOTE NOTE
    *
    * This system can also handle passive effects, but increases sim initialization time!
    *
    * General Useage is parse_effects( buff, modifying_spell_1, modifying_spell_2, modifying_spell_3 );
    *
    * USEAGE EXAMPLES *
    * -----------------
    ** apply_action_effects() **
    * Baseline effect with no affecting talents, or spells
    * --
    * parse_effects( warlock_base.affliction_warlock );
    * --
    ** apply_target_effects() **
    * Debuff
    * --
    * parse_target_effects( d_fn( &warlock_td_t::debuffs_t::fel_sunder ), talents.fell_sunder );
    * --
    * DoT
    * --
    * parse_target_effects( d_fn( &warlock_td_t::dots_t::unstable_affliction ), talents.unstable_affliction );
    * --
    * More advanced examples can be found in other modules that use this system.
    * A few are sc_druid.cpp, sc_death_knight.cpp, and sc_demon_hunter.cpp
    ------------------------------------------------------------- */
    void apply_action_effects()
    {
      // Shared

      // Affliction
      if ( affliction() )
      {
        parse_effects( p()->warlock_base.potent_afflictions ); // 77215
        parse_effects( p()->buffs.nightfall, effect_mask_t( true ).disable( 3 ) ); // 264571/1260279 // Effect #3 is handled in a custom action_state
        parse_effects( p()->buffs.darkglare_presence ); // 1280663
        parse_effects( p()->buffs.shard_instability ); // 1260269
      }

      // Demonology
      if ( demonology() )
      {
        parse_effects( p()->warlock_base.master_demonologist ); // 77219
        parse_effects( p()->buffs.demonic_core ); // 264173
        parse_effects( p()->buffs.power_siphon ); // 334581
      }

      // Destruction
      if ( destruction() )
      {
        parse_effects( p()->buffs.backdraft ); // 117828
        parse_effects( p()->buffs.fiendish_cruelty ); // 1245664
        parse_effects( p()->buffs.chaotic_inferno ); // 1244860
        parse_effects( p()->buffs.conflagration_of_chaos ); // 387109
        parse_effects( p()->buffs.crashing_chaos ); // 417282 // RoF is dummy
        parse_effects( p()->buffs.alythesss_ire ); // 1244947
      }

      // Diabolist
      if ( diabolist() )
      {
        // The Demonic Art buffs now include the damage increase from Touch of Rancora (except for RoF), which causes several effects (bugs):
        // - SB benefits twice from Touch of Rancora dmg amp during Demonic Art
        // - CB benefits twice from Touch of Rancora if the cast was started with Demonic Art, and only once if it was started without it
        // - Although the HoG Impact spell is listed, the buff is expired before the HoG impact spell is executed, so it has no effect
        parse_effects( p()->buffs.art_overlord ); // 428524
        parse_effects( p()->buffs.art_mother ); // 432794
        parse_effects( p()->buffs.art_pit_lord ); // 432795
      }

      // Hellcaller
      if ( hellcaller() )
      {
        //parse_effects( p()->buffs.malevolence ); // 442726 // Increased effectiveness of Through the Felvine during Malevolence implemented manually
      }

      // Soul Harvester

    }

    void apply_target_effects()
    {
      // Shared

      // Affliction
      if ( affliction() )
      {
        parse_target_effects( d_fn( &warlock_td_t::dots_t::unstable_affliction ), p()->talents.unstable_affliction ); // 316099
      }

      // Demonology
      if ( demonology() )
      {
      }

      // Destruction
      if ( destruction() )
      {
        parse_target_effects( d_fn( &warlock_td_t::dots_t::immolate ), p()->warlock_base.immolate_dot ); // 157736
        parse_target_effects( d_fn( &warlock_td_t::debuffs_t::lake_of_fire ), p()->talents.lake_of_fire_debuff ); // 1244918
      }

      // Diabolist

      // Hellcaller
      if ( hellcaller() )
      {
        if ( destruction() )
          parse_target_effects( d_fn( &warlock_td_t::dots_t::wither, false ), p()->hero.wither_dot ); // 445474
      }

      // Soul Harvester
    }

    void consume_resource() override
    {
      action_base_t::consume_resource();

      if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
      {
        int shards_used = as<int>( last_resource_cost );
        int base_shards = as<int>( base_cost() );

        // Only effective shards consumed count towards the Rain of Chaos proc
        if ( p()->buffs.rain_of_chaos->check() && shards_used > 0 )
        {
          for ( int i = 0; i < shards_used; i++ )
          {
            if ( p()->deck_rng.rain_of_chaos->trigger() )
            {
              p()->summons.roc->execute();
              p()->procs.rain_of_chaos->occur();
            }
          }
        }

        if ( diabolist() && triggers.diabolic_ritual )
        {
          timespan_t adjustment = 0_ms;

          if ( demonology() )
            adjustment = -timespan_t::from_seconds( p()->hero.diabolic_ritual->effectN( 1 ).base_value() ) * shards_used;

          if ( destruction() )
            adjustment = -timespan_t::from_seconds( p()->hero.diabolic_ritual->effectN( 2 ).base_value() );

          if ( demonology() && p()->hero.infernal_machine.ok() && p()->warlock_pet_list.demonic_tyrants.n_active_pets() > 0 )
            adjustment += -p()->hero.infernal_machine->effectN( 1 ).time_value();

          if ( destruction() && p()->hero.infernal_machine.ok() && p()->warlock_pet_list.infernals.n_active_pets() > 0 )
            adjustment += -p()->hero.infernal_machine->effectN( 1 ).time_value();

          if ( destruction() && p()->hero.touch_of_rancora.ok() && triggers.rancora_cb_bonus )
            adjustment += -timespan_t::from_seconds( p()->hero.touch_of_rancora->effectN( 3 ).base_value() );

          switch ( p()->diabolic_ritual )
          {
            case 0:
              if ( p()->buffs.ritual_overlord->check() )
              {
                p()->buffs.ritual_overlord->extend_duration( adjustment );
              }
              else
              {
                p()->buffs.ritual_overlord->trigger();
                make_event( sim, 1_ms, [ this, adjustment ] { p()->buffs.ritual_overlord->extend_duration( adjustment ); } );
              }
              break;
            case 1:
              if ( p()->buffs.ritual_mother->check() )
              {
                p()->buffs.ritual_mother->extend_duration( adjustment );
              }
              else
              {
                p()->buffs.ritual_mother->trigger();
                make_event( sim, 1_ms, [ this, adjustment ] { p()->buffs.ritual_mother->extend_duration( adjustment ); } );
              }
              break;
            case 2:
              if ( p()->buffs.ritual_pit_lord->check() )
              {
                p()->buffs.ritual_pit_lord->extend_duration( adjustment );
              }
              else
              {
                p()->buffs.ritual_pit_lord->trigger();
                make_event( sim, 1_ms, [ this, adjustment ] { p()->buffs.ritual_pit_lord->extend_duration( adjustment ); } );
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
      action_base_t::execute();

      // NOTE: Casted spells do not consume any Demonic Art buff if none were active at the start of the cast
      if ( diabolist() && triggers.demonic_art && triggers.demonic_art_buff )
      {
        if ( p()->hero.diabolic_oculi.ok() )
        {
          make_event( *sim, 0_ms, [ this ] {
            if ( p()->buffs.demonic_oculi->check() &&
                 ( p()->buffs.art_overlord->check() || p()->buffs.art_mother->check() ||
                   p()->buffs.art_pit_lord->check() ) )
            {
              // NOTE: 2026-03-17 Explosion damage is bugged and may not occur on instantiated content for some triggers (bug)
              // It seems that having the GoSac buff prevents these bugs
              if ( p()->bugs && !p()->buffs.grimoire_of_sacrifice->check()
                    && ( ( this->id == p()->talents.chaos_bolt->id() && p()->eye_explosion_instanced_bug_cb )
                      || ( this->id == p()->hero.ruination_cast->id() && destruction() && p()->eye_explosion_instanced_bug_cb )
                      || ( this->id == p()->talents.shadowburn->id() && p()->eye_explosion_instanced_bug_sb )
                      || ( this->id == p()->talents.rain_of_fire->id() && p()->eye_explosion_instanced_bug_rof ) ) )
              {
                // Even if the explosion damage does not occur, the Mind's Eyes buff is still granted and the Demonic Oculi still expire
                if ( p()->hero.minds_eyes.ok() )
                  p()->buffs.minds_eyes->trigger( p()->buffs.demonic_oculi->check() );

                p()->buffs.demonic_oculi->expire();
              }
              else
              {
                p()->proc_actions.eye_explosion->execute_on_target( this->target );
              }
            }
          } );
        }
        // Force event sequencing in a manner that lets Rain of Fire pick up the persistent multiplier for Touch of Rancora
        make_event( sim, 0_ms, [ this ] { p()->buffs.art_overlord->decrement(); } );
        make_event( sim, 0_ms, [ this ] { p()->buffs.art_mother->decrement(); } );
        make_event( sim, 0_ms, [ this ] { p()->buffs.art_pit_lord->decrement(); } );
      }
    }

    void impact( action_state_t* s ) override
    {
      action_base_t::impact( s );

      if ( affected_by.havoc && p()->talents.mayhem.ok() )
      {
        // Havoc debuff has an ICD, so it is safe to attempt a trigger
        auto tl = target_list();
        auto n = available_targets( tl );

        if ( n > 1u )
        {
          player_t* trigger_target = tl.at( 1u + rng().range( n - 1u ) );
          if ( td( trigger_target )->debuffs.havoc->trigger() )
          {
            assert( p()->havoc_target == trigger_target );
            p()->procs.mayhem->occur();
          }
        }
      }

      if ( destruction() && p()->talents.reverse_entropy.ok() )
      {
        if ( p()->buffs.reverse_entropy->trigger() )
          p()->procs.reverse_entropy->occur();
      }

      if ( destruction() && triggers.fiendish_cruelty )
      {
        if ( s->result == RESULT_CRIT && p()->prd_rng.fiendish_cruelty->trigger() )
        {
          p()->buffs.fiendish_cruelty->trigger();
          p()->procs.fiendish_cruelty->occur();
        }
      }

      if ( destruction() && triggers.dimensional_rift )
      {
        if ( p()->prd_rng.dimensional_rift->trigger() )
          p()->proc_actions.dimensional_rift->execute_on_target( s->target );
      }

      if ( destruction() && triggers.embers_of_nihilam_1 )
      {
        if ( p()->prd_rng.echo_of_sargeras->trigger() )
        {
          p()->proc_actions.echo_of_sargeras->execute_on_target( s->target );
          p()->procs.echo_of_sargeras->occur();
          p()->buffs.vision_of_nihilam->trigger();
        }
      }
    }

    void tick( dot_t* d ) override
    {
      action_base_t::tick( d );

      if ( affliction() && triggers.ravenous_afflictions && d->state->result == RESULT_CRIT && p()->rppm_rng.ravenous_afflictions->trigger() )
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

    double action_multiplier() const override
    {
      double m = action_base_t::action_multiplier();

      if ( destruction() && affected_by.chaotic_energies )
      {
        double min_percentage = affected_by.chaos_incarnate ? p()->talents.chaos_incarnate->effectN( 1 ).percent() : 0.5;

        double chaotic_energies_rng = rng().range( min_percentage , 1.0 );

        if ( p()->normalize_destruction_mastery )
          chaotic_energies_rng = ( 1.0 + min_percentage ) * 0.5;

        m *= 1.0 + chaotic_energies_rng * p()->cache.mastery_value();
      }

      return m;
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double m = action_base_t::composite_persistent_multiplier( s );

      // Demonology only has Hand of Gul'dan affected by Touch of Rancora, which requires special handling
      // Spells affected by touch_of_rancora_casted use a custom action_state_t and require special handling
      if ( diabolist() && destruction() && affected_by.touch_of_rancora && !affected_by.touch_of_rancora_casted )
      {
        if ( p()->buffs.art_overlord->check() || p()->buffs.art_mother->check() || p()->buffs.art_pit_lord->check() )
        {
          // NOTE: Shadowburn is double dipping the dmg amp bonus (bug). It works fine for RoF.
          if ( p()->bugs || ( this->id != p()->talents.shadowburn->id() ) )
            m *= 1.0 + p()->hero.touch_of_rancora->effectN( 1 ).percent();
        }
      }

      return m;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = action_base_t::composite_da_multiplier( s );

      double deaths_embrace_health = p()->talents.deaths_embrace->effectN( 2 ).base_value();
      if ( affliction() && affected_by.deaths_embrace && s->target->health_percentage() < deaths_embrace_health )
        m *= 1.0 + p()->talents.deaths_embrace->effectN( 1 ).percent() * ( 1 - s->target->health_percentage() / deaths_embrace_health );

      // NOTE: 2026-02-17 Diabolist guardians do not count towards Sacrificed Souls talent (bug?)
      if ( demonology() && affected_by.sacrificed_souls )
        m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_demon_count( !p()->bugs );

      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = action_base_t::composite_ta_multiplier( s );

      double deaths_embrace_health = p()->talents.deaths_embrace->effectN( 2 ).base_value();
      if ( affliction() && affected_by.deaths_embrace && s->target->health_percentage() < deaths_embrace_health )
        m *= 1.0 + p()->talents.deaths_embrace->effectN( 1 ).percent() * ( 1 - s->target->health_percentage() / deaths_embrace_health );

      return m;
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

      bool passed = action_base_t::select_target();

      if ( passed && target != saved_target && use_havoc() )
        target_cache.is_valid = false;

      return passed;
    }

    int n_targets() const override
    {
      if ( destruction() && use_havoc() )
      {
        assert( action_base_t::n_targets() == 0 );
        return 2;
      }
      else
      {
        return action_base_t::n_targets();
      }
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      action_base_t::available_targets( tl );

      // Check target list size to prevent some silly scenarios where Havoc target
      // is the only target in the list.
      if ( destruction() && tl.size() > 1 && use_havoc() )
      {
        // We need to make sure that the Havoc target ends up second in the target list,
        // so that Havoc spells can pick it up correctly.
        auto it = range::find( tl, p()->havoc_target );
        if ( it != tl.end() )
        {
          tl.erase( it );
          tl.insert( tl.begin() + 1, p()->havoc_target );
        }
      }

      return tl.size();
    }

    void init() override
    {
      action_base_t::init();

      if ( destruction() && affected_by.havoc )
      {
        base_aoe_multiplier *= p()->talents.havoc_debuff->effectN( 1 ).percent();
        p()->havoc_spells.push_back( this );
      }
    }

    bool affliction() const
    { return p()->affliction(); }

    bool demonology() const
    { return p()->demonology(); }

    bool destruction() const
    { return p()->destruction(); }

    bool diabolist() const
    { return p()->diabolist(); }

    bool hellcaller() const
    { return p()->hellcaller(); }

    bool soul_harvester() const
    { return p()->soul_harvester(); }

    template <set_bonus_type_e Tier>
    bool active_2pc() const
    { return p()->active_2pc<Tier>(); }

    template <set_bonus_type_e Tier>
    bool active_4pc() const
    { return p()->active_4pc<Tier>(); }
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
    drain_life_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Drain Life", p, p->warlock_base.drain_life, options_str )
    {
      channeled = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.soulburn->expire();
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
        }

        triggers.ravenous_afflictions = affliction() && p->talents.ravenous_afflictions.ok();

        affected_by.deaths_embrace = affliction() && p->talents.deaths_embrace.ok();
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( result_is_hit( d->state->result ) && p()->talents.nightfall.ok() && p()->progress_rng.nightfall->trigger( d->state ) )
        {
          p()->procs.nightfall->occur();
          p()->buffs.nightfall->trigger();
        }
      }
    };

    corruption_dot_t* periodic;

    corruption_t( warlock_t* p, util::string_view options_str, bool seed_action )
      : warlock_spell_t( "Corruption (Direct)", p, !p->hero.wither.ok() ? p->warlock_base.corruption : spell_data_t::not_found(), options_str )
    {
      periodic = new corruption_dot_t( p );
      impact_action = periodic;
      add_child( periodic );

      if ( seed_action )
        spell_power_mod.direct = 0; // Corruption does not deal instant damage when applied from SoC

      // NOTE: 2026-02-20: Death's Embrace talent is not applying to the Corruption (direct damage) spell (bug?)
      affected_by.deaths_embrace = !p->bugs && p->talents.deaths_embrace.ok();
    }

    dot_t* get_dot( player_t* t ) override
    { return periodic->get_dot( t ); }
  };

  struct shadowbolt_volley_t : public warlock_spell_t
  {
    shadowbolt_volley_t( warlock_t* p )
      : warlock_spell_t( "Shadowbolt Volley", p, p->talents.shadowbolt_volley )
    {
      background = dual = true;

      affected_by.deaths_embrace = false; // Shadowbolt Volley is not affected by Death's Embrace
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( t );

      // NOTE: 2026-02-20 Shadowbolt Volley (Cunning Cruelty) is affected by Withering Bolt
      if ( p()->talents.withering_bolt.ok() )
        m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( ( int )( p()->talents.withering_bolt->effectN( 2 ).base_value() ), p()->get_target_data( t )->count_affliction_dots() );

      return m;
    }
  };

  struct shadow_bolt_t : public warlock_spell_t
  {
    shadowbolt_volley_t* volley;

    shadow_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Shadow Bolt", p, p->talents.drain_soul.ok() ? spell_data_t::not_found() : p->warlock_base.shadow_bolt, options_str )
    {
      affected_by.sacrificed_souls = demonology() && p->talents.sacrificed_souls.ok();
      affected_by.deaths_embrace = affliction() && p->talents.deaths_embrace.ok();

      if ( demonology() )
      {
        energize_type = action_energize::ON_CAST;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = 1.0;
      }

      if ( p->talents.cunning_cruelty.ok() )
        volley = new shadowbolt_volley_t( p );
    }

    bool ready() override
    {
      if ( diabolist() && p()->executing != this && p()->buffs.infernal_bolt->check() )
        return false;

      if ( affliction() && p()->executing != this && p()->talents.malefic_grasp.ok() && p()->warlock_pet_list.darkglares.n_active_pets() > 0 )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( time_to_execute == 0_ms && soul_harvester() && p()->buffs.nightfall->check() )
      {
        if ( p()->hero.wicked_reaping.ok() )
          p()->proc_actions.wicked_reaping->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.shared_fate.ok() )
          p()->proc_actions.shared_fate->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && p()->prd_rng.feast_of_souls->trigger( execute_state ) )
          p()->feast_of_souls_gain();
      }

      // Shadow Bolt energize spell triggers procs
      p()->trigger_aura_applied_callbacks( p()->proc_data_entries.shadow_bolt_energize, p() );

      if ( time_to_execute == 0_ms )
        p()->buffs.nightfall->decrement();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        if ( p()->talents.shard_instability.ok() && p()->prd_rng.shard_instability_sb->trigger() )
        {
          p()->buffs.shard_instability->trigger();
          p()->procs.shard_instability->occur();
        }

        if ( p()->talents.cunning_cruelty.ok() && p()->prd_rng.cunning_cruelty->trigger() )
        {
          p()->procs.shadowbolt_volley->occur();
          volley->execute_on_target( s->target );
        }
      }
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      if ( time_to_execute == 0_ms && p()->buffs.nightfall->check() )
        m *= 1.0 + p()->talents.nightfall_buff->effectN( 2 ).percent();

      return m;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( t );

      if ( p()->talents.withering_bolt.ok() )
        m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( ( int )( p()->talents.withering_bolt->effectN( 2 ).base_value() ), p()->get_target_data( t )->count_affliction_dots() );

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

      switch ( pet->pet_type )
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

      switch ( pet->pet_type )
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

        if ( affliction() )
        {
          if ( p->talents.absolute_corruption.ok() )
          {
            dot_duration = sim->expected_iteration_time > 0_ms
              ? 2 * sim->expected_iteration_time
              : 2 * sim->max_time * ( 1.0 + sim->vary_combat_length );
          }

          triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

          affected_by.deaths_embrace = p->talents.deaths_embrace.ok();
        }
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( affliction() )
        {
          if ( result_is_hit( d->state->result ) && p()->talents.nightfall.ok() && p()->progress_rng.nightfall->trigger( d->state ) )
          {
            p()->procs.nightfall->occur();
            p()->buffs.nightfall->trigger();
          }
          if ( p()->talents.siphon_life.ok() || ( p()->hero.seeds_of_their_demise.ok() && d->target->health_percentage() <= p()->hero.seeds_of_their_demise->effectN( 2 ).base_value() ) )
          {
            // Affliction Wither DoT ticks trigger procs when talented into Siphon Life
            // Affliction Wither DoT ticks also trigger procs when attempting to start a collapse via Seeds of Their Demise
            p()->trigger_aura_applied_callbacks( proc_data, p() );
          }
        }

        if ( destruction() )
        {
          if ( d->state->result == RESULT_CRIT && p()->flat_rng.wither_crit_energize->trigger() )
            p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.wither_crits );

          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.wither );

          if ( p()->talents.flashpoint.ok() && d->target->health_percentage() >= p()->talents.flashpoint->effectN( 2 ).base_value() )
            p()->buffs.flashpoint->trigger();

          if ( p()->talents.demonfire_infusion.ok() && p()->flat_rng.demonfire_infusion_dot->trigger() )
          {
            p()->proc_actions.demonfire_infusion->execute_on_target( d->target );
            p()->procs.demonfire_infusion_dot->occur();
          }

          // Destruction Wither DoT ticks trigger procs through some hidden trigger
          p()->trigger_aura_applied_callbacks( proc_data, p() );
        }

        // Seeds of their Demise collapse conditions must be checked periodically for every Wither tick
        bool collapse = false;
        collapse = collapse || ( p()->hero.seeds_of_their_demise.ok() && d->current_stack() > 1 && d->target->health_percentage() <= p()->hero.seeds_of_their_demise->effectN( 2 ).base_value() );
        collapse = collapse || ( p()->hero.seeds_of_their_demise.ok() && d->current_stack() >= as<int>( p()->hero.seeds_of_their_demise->effectN( 1 ).base_value() ) );
        if ( collapse )
        {
          const int prev_collapse_stacks = td( d->target )->debuffs.blackened_soul->check();
          assert( prev_collapse_stacks >= 0 );
          const int diff_stacks = d->current_stack() - prev_collapse_stacks;

          assert( d->current_stack() >= 1 );
          if ( diff_stacks > 0 )
            td( d->target )->debuffs.blackened_soul->trigger( diff_stacks );
          else if ( diff_stacks < 0 )
            td( d->target )->debuffs.blackened_soul->decrement( -diff_stacks );

          assert( td( d->target )->debuffs.blackened_soul->check() );
          if ( !prev_collapse_stacks )
          {
            p()->sim->print_debug( "{} wither stack collapse in {} started (seeds of their demise) (wither tick check). wither_current_stack={}, wither_target_health_percentage={:.2f}%",
                                   p()->name(), d->target->name(), d->current_stack(), d->target->health_percentage() );
          }
        }

        if ( d->state->result == RESULT_CRIT && p()->hero.mark_of_perotharn.ok() && p()->prd_rng.mark_of_perotharn->trigger() )
        {
          // Wither stack gain by Mark of Perotharn does not directly trigger collapse in that tick (it will be trigged on the next tick)
          // Wither stack gain by Mark of Perotharn does not benefit from Bleakheart Tactics
          d->increment( 1 );
          td( d->target )->debuffs.wither->bump( 1 );
          assert( d->current_stack() == td( d->target )->debuffs.wither->check() && d->remains() == td( d->target )->debuffs.wither->remains() );
          p()->procs.mark_of_perotharn->occur();
        }

        if ( p()->hero.devil_fruit.ok() )
        {
          if ( p()->rppm_rng.devil_fruit->trigger() )
          {
            p()->buffs.malevolence->trigger( timespan_t::from_seconds( p()->hero.devil_fruit->effectN( 1 ).base_value() ) );
            p()->procs.devil_fruit->occur();
          }
        }
      }

      void trigger_dot( action_state_t* s ) override
      {
        warlock_spell_t::trigger_dot( s );
        auto& debuff = td( s->target )->debuffs.wither;
        auto& dot = td( s->target )->dots.wither;
        debuff->trigger( 1, dot->remains() );
        assert( dot->current_stack() == debuff->check() && dot->remains() == debuff->remains() );

        timespan_t duration = composite_dot_duration( s );
        if ( duration <= timespan_t::zero() )
          return;

        // In simc dots increase their stack count when refreshed.
        // Ingame, however, Wither does not increase stacks on dot refresh.
        // Therefore, its stack count should be decreased by 1 after refreshing to match ingame behavior.
        if ( dot->is_ticking() && dot->current_stack() > 1 )
        {
          dot->decrement( 1 );
          debuff->decrement( 1 );
          assert( dot->current_stack() == debuff->check() && dot->remains() == debuff->remains() );
        }
      }
    };

    wither_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Wither (Direct)", p, p->hero.wither.ok() ? p->hero.wither_direct : spell_data_t::not_found(), options_str )
    {
      affected_by.chaotic_energies = destruction();
      affected_by.havoc = destruction();

      impact_action = new wither_dot_t( p );
      add_child( impact_action );

      // NOTE: 2026-02-20: Death's Embrace talent is not applying to the Wither (direct damage) spell (bug?)
      affected_by.deaths_embrace = affliction() && !p->bugs && p->talents.deaths_embrace.ok();
    }

    wither_t( warlock_t* p, bool havoc, util::string_view options_str ) : wither_t( p, options_str )
    { affected_by.havoc = havoc; }

    dot_t* get_dot( player_t* t ) override
    { return impact_action->get_dot( t ); }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( s->result == RESULT_CRIT && p()->hero.mark_of_perotharn.ok() && p()->prd_rng.mark_of_perotharn->trigger() )
      {
        auto& wither_dot = td( s->target )->dots.wither;
        auto& wither_debuff = td( s->target )->debuffs.wither;
        // Wither stack gain by Mark of Perotharn does not directly trigger collapse (it will be trigged on the next Wither tick)
        // Wither stack gain by Mark of Perotharn does not benefit from Bleakheart Tactics
        wither_dot->increment( 1 );
        wither_debuff->bump( 1 );
        assert( wither_dot->current_stack() == wither_debuff->check() && wither_dot->remains() == wither_debuff->remains() );
        p()->procs.mark_of_perotharn->occur();
      }
    }
  };

  struct blackened_soul_t : public warlock_spell_t
  {
    blackened_soul_t( warlock_t* p )
      : warlock_spell_t( "Blackened Soul", p, p->hero.blackened_soul_dmg )
    {
      background = dual = true;

      affected_by.chaotic_energies = destruction();
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( target );

      if ( p()->hero.mark_of_xavius.ok() )
      {
        double val = p()->hero.mark_of_xavius->effectN( 3 ).percent();

        m *= 1.0 + td( target )->dots.wither->current_stack() * val;
      }

      return m;
    }

    void execute() override
    {
      // Wither stack decrement is done before damage. Relevant for Mark of Xavius talent.
      // (e.g.) Blackened Soul where 10 to 9 stacks: Mark of Xavius talent bonus damage is calculated on 9 stacks.
      auto& wither_dot = td( target )->dots.wither;
      if ( wither_dot->current_stack() > 1 )
      {
        auto& wither_debuff = td( target )->debuffs.wither;
        wither_dot->decrement( 1 );
        wither_debuff->decrement( 1 );
        assert( wither_dot->current_stack() == wither_debuff->check() && wither_dot->remains() == wither_debuff->remains() );
      }

      warlock_spell_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      player_t* tar = s->target;

      // Blackened Soul damage impact runs during blackened_soul_debuff tick callback.
      // Its frozen stacks make direct expire/decrement safe here without deferring to a follow-up event.
      auto& blackened_soul_debuff = td( tar )->debuffs.blackened_soul;
      assert( blackened_soul_debuff->check() );
      assert( blackened_soul_debuff->freeze_stacks );
      assert( blackened_soul_debuff->buff_duration() == 0_ms );
      assert( blackened_soul_debuff->expiration.empty() );
      assert( blackened_soul_debuff->tick_event == nullptr );
      if ( td( tar )->dots.wither->current_stack() <= 1 )
      {
        blackened_soul_debuff->expire();
        p()->sim->print_debug( "{} wither stack collapse in {} ended (wither stacks reach 1). wither_current_stack={}", p()->name(), tar->name(), td( tar )->dots.wither->current_stack() );
      }
      else
      {
        blackened_soul_debuff->decrement();
        if ( !blackened_soul_debuff->check() )
          p()->sim->print_debug( "{} wither stack collapse in {} ended (collapse consumed its stacks). wither_current_stack={}", p()->name(), tar->name(), td( tar )->dots.wither->current_stack() );
      }

      if ( affliction() && p()->hero.seeds_of_their_demise.ok() && p()->progress_rng.seeds_of_their_demise->trigger( s ) )
      {
        p()->buffs.shard_instability->trigger();
        p()->procs.seeds_of_their_demise->occur();
      }

      if ( destruction() && p()->hero.seeds_of_their_demise.ok() && p()->progress_rng.seeds_of_their_demise->trigger( s ) )
      {
        p()->buffs.flashpoint->trigger( as<int>( p()->hero.seeds_of_their_demise->effectN( 3 ).base_value() ) );
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
      cooldown->duration = p->hero.malevolence_buff->cooldown();
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
    }
  };

  struct demonic_soul_t : public warlock_spell_t
  {
    demonic_soul_t( warlock_t* p )
      : warlock_spell_t( "Demonic Soul", p, p->hero.demonic_soul_dmg )
    {
      aoe = -1;
      reduced_aoe_targets = 8;
      background = dual = true;

      if ( p->hero.soul_anathema.ok() )
        impact_action = new soul_anathema_t( p );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( demonology() && p()->hero.demoniacs_fervor.ok() && s->chain_target == 0 )
        m *= 1.0 + p()->hero.demoniacs_fervor->effectN( 1 ).percent();

      if ( affliction() && p()->hero.demoniacs_fervor.ok() && td( s->target )->dots.unstable_affliction->is_ticking() )
        m *= 1.0 + p()->hero.demoniacs_fervor->effectN( 1 ).percent();

      return m;
    }
  };

  struct shared_fate_t : public warlock_spell_t
  {
    shared_fate_t( warlock_t* p )
      : warlock_spell_t( "Shared Fate", p, p->hero.shared_fate_dot )
    {
      background = dual = true;
      aoe = -1; // DoT is applied in AoE
      // Despite what its description says, the damage of Shared Fate does not seem to deal reduced damage beyond 8 targets
      // reduced_aoe_targets = as<int>( p->hero.shared_fate->effectN( 1 ).base_value() );
    }
  };

  struct wicked_reaping_t : public warlock_spell_t
  {
    wicked_reaping_t( warlock_t* p )
      : warlock_spell_t( "Wicked Reaping", p, p->hero.wicked_reaping_dmg )
    {
      background = dual = true;

      if ( demonology() )
        base_dd_multiplier *= p->hero.wicked_reaping->effectN( 2 ).percent();

      if ( p->hero.soul_anathema.ok() )
        impact_action = new soul_anathema_t( p );
    }
  };

  struct summon_manifested_demonic_soul_t : public warlock_spell_t
  {
    summon_manifested_demonic_soul_t( warlock_t* p )
      : warlock_spell_t( "Manifested Demonic Soul (Summon)", p, p->hero.manifested_avarice_spell )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.demonic_souls.spawn( data().duration() );

      p()->buffs.manifested_demonic_soul->trigger();
    }
  };

  // Soul Harvester Actions End
  // Affliction Actions Begin

  struct agony_t : public warlock_spell_t
  {
    agony_t* twin = nullptr;
    double twin_range = 0.0;

    agony_t( warlock_t* p, util::string_view options_str, bool is_twin = false )
      : warlock_spell_t( "Agony", p, p->talents.agony, options_str )
    {
      may_crit = false;

      triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

      if ( !is_twin )
      {
        if ( p->talents.shared_agony.ok() )
          twin = new agony_t( p, options_str, true );
      }
      else
      {
        background = dual = proc = true;
        base_costs[ RESOURCE_MANA ] = 0;
      }

      twin_range = p->talents.shared_agony->effectN( 2 ).base_value();
    }

    void last_tick( dot_t* d ) override
    {
      // TOCHECK: Is this reset to a random state still a thing?
      if ( p()->get_active_dots( d ) == 1 )
        p()->progress_rng.agony_energize->reset( reset_type_e::COMBAT );

      warlock_spell_t::last_tick( d );
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( twin != nullptr )
      {
        const auto& tl = target_list();
        if ( auto twin_target = p()->get_smart_target( tl, &warlock_td_t::dots_t::agony, target, twin_range, true ) )
          twin->execute_on_target( twin_target );
      }

      int initial_stacks = 0;

      if ( p()->talents.sudden_onset.ok() )
        initial_stacks += ( int )( p()->talents.sudden_onset->effectN( 2 ).base_value() );

      if ( active_4pc<MID1>() )
        initial_stacks += ( int )( p()->tier.wl_affliction_12_0_class_set_4pc->effectN( 1 ).base_value() );

      int delta_stacks = initial_stacks - td( execute_state->target )->dots.agony->current_stack();

      if ( delta_stacks > 0 )
          td( execute_state->target )->dots.agony->increment( delta_stacks );
    }

    void tick( dot_t* d ) override
    {
      if ( p()->progress_rng.agony_energize->trigger( d->state ) )
      {
        p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony );

        // Agony energize spell triggers procs
        p()->trigger_aura_applied_callbacks( p()->proc_data_entries.agony_energize, p() );
      }

      warlock_spell_t::tick( d );

      d->increment( 1 );
    }
  };

  struct unstable_affliction_t : public warlock_spell_t
  {
    bool is_fatal_echoes_execute = false;

    unstable_affliction_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Unstable Affliction", p, p->talents.unstable_affliction, options_str )
    {
      triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();
    }

    void execute() override
    {
      // NOTE: 2026-04-29 Currently ingame a UA applied by Fatal Echoes also processes/consumes some UA 'execute' effects:
      // - Succulent Soul: consumes a stack and triggers its effects (Demonic Soul dmg and Manifested Avarice rng proc)
      // - Cull the Weak: reduces the cooldown of Dark Harvest
      // - Hellcaller Blackened Soul: increments Wither stacks
      // - Shard Instability: unaffected; Fatal Echoes does not consume a stack of this buff

      warlock_spell_t::execute();

      if ( p()->talents.cull_the_weak.ok() )
        p()->cooldowns.dark_harvest->adjust( -p()->talents.cull_the_weak->effectN( 1 ).time_value() );

      // NOTE: 2026-04-29 If Shard Instability buff is gained during the casting of Unstable Affliction, that UA cast benefits from the cost
      // reduction but does not consume the effect (bug?). As expected, a Fatal Echoes UA proc does not consume it either.
      if ( time_to_execute == 0_ms && !is_fatal_echoes_execute )
      {
        p()->buffs.shard_instability->decrement();
      }

      if ( soul_harvester() && p()->buffs.succulent_soul->check() )
      {
        p()->buffs.succulent_soul->decrement();

        if ( p()->hero.manifested_avarice.ok() && p()->prd_rng.manifested_avarice->trigger() )
        {
          p()->summons.manifested_demonic_soul->execute();
          p()->procs.manifested_avarice->occur();
        }

        p()->proc_actions.demonic_soul->execute_on_target( target );
      }
    }

    void impact( action_state_t* s ) override
    {
      auto dot = td( s->target )->dots.unstable_affliction;

      if ( p()->talents.cascading_calamity.ok() && dot->is_ticking() )
        p()->buffs.cascading_calamity->trigger();

      timespan_t dot_new_last_duration = composite_dot_duration( s );
      // NOTE: If Blizzard change the UA DoT Behavior, this need to be redesigned
      assert( dot_behavior == DOT_REFRESH_DURATION && "UA DoT Behavior has changed" );

      warlock_spell_t::impact( s );

      // We need to handle the UA stacks/duration manually
      if ( result_is_hit( s->result ) )
      {
        // NOTE: The spell data is using DOT_REFRESH_DURATION, which should add the time-until-the-next-full-tick to the total duration
        // However, ingame, the duration does not add the last tick and only refresh the dot to the total duration (always 8 seconds)
        dot_t* dot = td( s->target )->dots.unstable_affliction;
        if ( dot->duration() > dot_new_last_duration )
          dot->adjust_duration( dot_new_last_duration - dot->duration() );

        make_event<ua_stack_drop_event_t>( *sim, p(), dot, dot_new_last_duration );
      }
    }

    void last_tick( dot_t* d ) override
    {
      int stacks = d->current_stack();

      warlock_spell_t::last_tick( d );

      if ( p()->talents.fatal_echoes.ok() && !d->target->is_sleeping() )
      {
        for ( int i = 0; i < stacks; i++ )
        {
          if ( p()->prd_rng.fatal_echoes->trigger() )
          {
            p()->procs.fatal_echoes->occur();
            make_event( sim, 1_ms, [ this, t = d->target ] {
              const bool prev_ua_ticking = td( t )->dots.unstable_affliction->is_ticking();
              this->set_target( t );
              this->time_to_execute = 0_ms;
              this->is_fatal_echoes_execute = true;
              this->execute();
              this->is_fatal_echoes_execute = false;
              // When UA is applied by Fatal Echoes, Cascading Calamity is also triggered
              if ( p()->talents.cascading_calamity.ok() && !prev_ua_ticking )
                p()->buffs.cascading_calamity->trigger();
            } );
          }
        }
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      // The base effect of Through the Felvine is automatically applied by the parse_effects system
      // However, it is necessary to manually apply its duplicate effect during Malevolence
      if ( p()->hero.through_the_felvine.ok() && p()->hero.malevolence.ok() && p()->buffs.malevolence->check() )
      {
        double felvine_mul = p()->hero.through_the_felvine->effectN( 1 ).percent();
        m *= 1.0 + felvine_mul / ( 1.0 + felvine_mul );
      }

      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      // The base effect of Through the Felvine is automatically applied by the parse_effects system
      // However, it is necessary to manually apply its duplicate effect during Malevolence
      if ( p()->hero.through_the_felvine.ok() && p()->hero.malevolence.ok() && p()->buffs.malevolence->check() )
      {
        double felvine_mul = p()->hero.through_the_felvine->effectN( 2 ).percent();
        m *= 1.0 + felvine_mul / ( 1.0 + felvine_mul );
      }

      return m;
    }

    double cost_pct_multiplier() const override
    {
      if ( is_fatal_echoes_execute )
        return 0.0;

      return warlock_spell_t::cost_pct_multiplier();
    }
  };

  struct seed_of_corruption_t : public warlock_spell_t
  {
    struct seed_of_corruption_state_t : public action_state_t
    {
      double effectiveness;

      seed_of_corruption_state_t( action_t* action, player_t* target )
        : action_state_t( action, target ),
        effectiveness( 1.0 )
      { }

      void initialize() override
      {
        action_state_t::initialize();
        effectiveness = 1.0;
      }

      std::ostringstream& debug_str( std::ostringstream& s ) override
      {
        action_state_t::debug_str( s );
        s << " effectiveness=" << effectiveness;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        effectiveness = debug_cast<const seed_of_corruption_state_t*>( s )->effectiveness;
      }
    };

    struct seed_of_corruption_aoe_t : public warlock_spell_t
    {
      action_t* applied_dot;
      double effectiveness;

      seed_of_corruption_aoe_t( warlock_t* p )
        : warlock_spell_t( "Seed of Corruption (AoE)", p, p->talents.seed_of_corruption_aoe ),
        effectiveness( 1.0 )
      {
        aoe = -1;
        background = dual = true;
        reduced_aoe_targets = as<int>( p->talents.seed_of_corruption->effectN( 4 ).base_value() );

        affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

        if ( p->hero.wither.ok() )
          applied_dot = new wither_t( p, "" );
        else
          applied_dot = new corruption_t( p, "", true );

        applied_dot->background = true;
        applied_dot->dual = true;
        applied_dot->base_costs[ RESOURCE_MANA ] = 0;
        applied_dot->base_dd_multiplier = 0.0;
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_da_multiplier( s );

        // The base effect of Through the Felvine is automatically applied by the parse_effects system
        // However, it is necessary to manually apply its duplicate effect during Malevolence
        if ( p()->hero.through_the_felvine.ok() && p()->hero.malevolence.ok() && p()->buffs.malevolence->check() )
        {
          double felvine_mul = p()->hero.through_the_felvine->effectN( 3 ).percent();
          m *= 1.0 + felvine_mul / ( 1.0 + felvine_mul );
        }

        return m;
      }

      double composite_target_da_multiplier( player_t* t ) const override
      {
        double m = warlock_spell_t::composite_target_da_multiplier( t );

        if ( p()->talents.patient_zero.ok() )
        {
          // NOTE (2026-04-24): Patient Zero does not track seeds individually (bug?). Instead, it
          // uses a single per-caster target reference updated on cast success to the target of the
          // primary Seed of Corruption. Any seed explosion hitting that target gets the Patient Zero
          // bonus. If the target is out of range, dead, or otherwise invalid (e.g., immune) at the
          // time of explosion, the bonus is not reassigned and is simply not applied.
          if ( p()->bugs )
          {
            assert( p()->patient_zero_target && "SoC does not have a valid Patient Zero target" );
            if ( t == p()->patient_zero_target )
              m *= 1.0 + p()->talents.patient_zero->effectN( 1 ).percent();
          }
          else
          {
            if ( t == target )
              m *= 1.0 + p()->talents.patient_zero->effectN( 1 ).percent();
          }
        }

        if ( p()->talents.sow_the_seeds.ok() )
        {
          m *= effectiveness;
        }

        return m;
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( result_is_hit( s->result ) )
        {
          auto tdata = td( s->target );

          if ( tdata->dots.seed_of_corruption->is_ticking() && tdata->soc_threshold > 0 )
          {
            tdata->soc_threshold = 0;
            tdata->dots.seed_of_corruption->cancel();
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

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

      // Set aoe = 1 even without Sow the Seeds so the special target selection logic is used
      aoe = 1 + as<int>( p->talents.sow_the_seeds->effectN( 1 ).base_value() );

      add_child( explosion );
    }

    action_state_t* new_state() override
    { return new seed_of_corruption_state_t( this, target ); }

    void snapshot_state( action_state_t* s, result_amount_type rt ) override
    {
      if ( s->chain_target == 0 || !p()->talents.sow_the_seeds.ok() )
        debug_cast<seed_of_corruption_state_t*>( s )->effectiveness = 1.0;
      else
        debug_cast<seed_of_corruption_state_t*>( s )->effectiveness = p()->talents.sow_the_seeds->effectN( 2 ).percent();

      warlock_spell_t::snapshot_state( s, rt );
    }

    void init() override
    {
      warlock_spell_t::init();
      snapshot_flags |= STATE_SP;
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      warlock_spell_t::available_targets( tl );

      // Seed of Corruption has special target selection behavior (smart targeting):
      // - The primary seed prefers the original target if it does not already have a SoC debuff.
      //   - If the original target already has a SoC debuff, the primary seed is redirected to a random
      //     target (from the original target list) without a SoC debuff.
      //   - If no such target exists, the primary seed falls back to the original target even though
      //     it already has a SoC debuff.
      // - With Sow the Seeds, secondary seeds are selected from the remaining targets.
      //   - Normally they can only select targets without a SoC debuff; if none are available, no
      //     secondary seed is applied.
      //   - If the primary seed had to fall back to the original target, secondary seeds may select
      //     targets that already have a SoC debuff.
      // - Targets selected by this cast are not duplicated; the primary seed is kept in first position,
      //   and the remaining targets are shuffled for secondary seed selection. Invalid secondary targets
      //   are removed from the target list.
      // - Formerly, SoC smart targeting was based on whether the target had the debuff or had a seed in
      //   travel. This is no longer the case, and only the presence of the SoC debuff matters. (bug?)

      player_t* main_seed_target = target;
      bool main_seed_fallback = false;

      std::vector<player_t*> pool = tl;

      range::erase_remove( pool, [ this ]( player_t* t ) {
        return ( t == target || td( t )->dots.seed_of_corruption->is_ticking() || ( !p()->bugs && has_travel_events_for( t ) ) );
      } );

      if ( td( target )->dots.seed_of_corruption->is_ticking() || ( !p()->bugs && has_travel_events_for( target ) ) )
      {
        if ( !pool.empty() )
          main_seed_target = pool[ rng().range( size_t{}, pool.size() ) ];
        else
          main_seed_fallback = true;
      }

      auto it = range::find( tl, main_seed_target );
      if ( it != tl.end() && it != tl.begin() )
      {
        tl.erase( it );
        tl.insert( tl.begin(), main_seed_target );
      }

      if ( !main_seed_fallback )
      {
        range::erase_remove( tl, [ this, main_seed_target ]( player_t* t ) {
          return ( t != main_seed_target && ( td( t )->dots.seed_of_corruption->is_ticking() || ( !p()->bugs && has_travel_events_for( t ) ) ) );
        } );
      }

      if ( tl.size() > 1 )
        rng().shuffle( tl.begin() + 1, tl.end() );

      return tl.size();
    }

    void execute() override
    {
      target_cache.is_valid = false;

      const auto& tl = target_list();
      player_t* main_seed_target = !tl.empty() ? tl.front() : target;

      // Patient Zero target is updated on SoC cast success, not on impact or debuff application
      if ( p()->talents.patient_zero.ok() )
        p()->patient_zero_target = main_seed_target;

      warlock_spell_t::execute();

      p()->buffs.seed_of_corruption_is_out_dnt->trigger();

      if ( time_to_execute == 0_ms && soul_harvester() && p()->talents.nocturnal_yield.ok() && p()->buffs.nightfall->check() )
      {
        if ( p()->hero.wicked_reaping.ok() )
          p()->proc_actions.wicked_reaping->execute_on_target( main_seed_target );

        if ( p()->hero.quietus.ok() && p()->hero.shared_fate.ok() )
          p()->proc_actions.shared_fate->execute_on_target( main_seed_target );

        // Feast of Souls is processed before the decrement of Succulent Soul, causing the same SoC cast that gains the Succulent Soul stack to consume it
        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && p()->prd_rng.feast_of_souls->trigger( execute_state ) )
          p()->feast_of_souls_gain( true );
      }

      // NOTE: 2026-02-26 If Nightfall is obtained during the casting of Seed of Corruption, that SoC cast
      // benefits from the cost reduction but does not consume the effect. (bug?)
      if ( p()->talents.nocturnal_yield.ok() && time_to_execute == 0_ms )
        p()->buffs.nightfall->decrement();

      if ( p()->talents.cull_the_weak.ok() )
        p()->cooldowns.dark_harvest->adjust( -p()->talents.cull_the_weak->effectN( 1 ).time_value() );

      if ( soul_harvester() && p()->buffs.succulent_soul->check() )
      {
        p()->buffs.succulent_soul->decrement();

        if ( p()->hero.manifested_avarice.ok() && p()->prd_rng.manifested_avarice->trigger() )
        {
          p()->summons.manifested_demonic_soul->execute();
          p()->procs.manifested_avarice->occur();
        }

        p()->proc_actions.demonic_soul->execute_on_target( main_seed_target );
      }
    }

    void impact( action_state_t* s ) override
    {
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
      // This function can be called while executing other actions by the assessor in charge of the Seed
      // of Corruption damage accumulator that triggers the end of the dot (and thus the explosion).
      // Each explosion is encapsulated in an event to ensure there are no nested explosion actions.
      // Explosion parameters must be captured here in the lambda by value for that same reason.
      make_event( sim, 0_ms, [ this,
                               t = d->target,
                               effectiveness = debug_cast<seed_of_corruption_state_t*>( d->state )->effectiveness ]
      {
        explosion->effectiveness = effectiveness;
        explosion->set_target( t );
        explosion->execute();
      } );

      warlock_spell_t::last_tick( d );
    }
  };

  struct malefic_grasp_t : public warlock_spell_t
  {
    struct malefic_grasp_state_t : public action_state_t
    {
      double tick_time_multiplier;
      double td_multiplier;

      malefic_grasp_state_t( action_t* action, player_t* target )
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
        action_state_t::debug_str( s );
        s << " tick_time_multiplier=" << tick_time_multiplier;
        s << " td_multiplier=" << td_multiplier;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        tick_time_multiplier = debug_cast<const malefic_grasp_state_t*>( s )->tick_time_multiplier;
        td_multiplier = debug_cast<const malefic_grasp_state_t*>( s )->td_multiplier;
      }
    };

    struct mg_extra_tick_base_t : public warlock_spell_t
    {
      mg_extra_tick_base_t( std::string_view n, warlock_t* p, const spell_data_t* s ) : warlock_spell_t( n, p, s )
      {
        background = dual = true;
        base_dd_min = base_dd_max = 0;
        spell_power_mod.direct = 0;

        // DoT (Malefic Grasp) extra ticks are affected by Death's Embrace
        affected_by.deaths_embrace = p->talents.deaths_embrace.ok();
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( result_is_hit( s->result ) )
        {
          // DoT (Malefic Grasp) extra tick crits can trigger Ravenous Afflictions
          if ( p()->talents.ravenous_afflictions.ok() && s->result == RESULT_CRIT && p()->rppm_rng.ravenous_afflictions->trigger() )
          {
            p()->buffs.nightfall->trigger();
            p()->procs.ravenous_afflictions->occur();
          }
        }
      }
    };

    // NOTE: 2026-02-20 Agony (Malefic Grasp) ticks cannot generate soul shards (we assume they also do not contribute to the accumulator)
    struct agony_mg_t : public mg_extra_tick_base_t
    {
      agony_mg_t( warlock_t* p )
        : mg_extra_tick_base_t( "Agony (Malefic Grasp)", p, ( p->talents.malefic_grasp.ok() && p->talents.agony.ok() ) ? p->talents.agony_mg : spell_data_t::not_found() )
      {
        // NOTE: 2026-02-20 Agony (Malefic Grasp) extra tick is not whitelisted in the Direct Damage component of many effects
        // (Summoner's Embrace, Niskaran Methods, Mastery: Potent Afflictions), and others don't even have this effect currently
        // (Shared Agony, Sudden Onset, Mark of Xavius). However, it seems to do almost twice dmg in compensation (bug?)
        if ( p->bugs )
        {
          // TODO: Look for this/these multiplier/s in spell data
          base_dd_multiplier *= 1.95;
        }
        else
        {
          base_dd_multiplier *= 1.0 + p->talents.shared_agony->effectN( 1 ).percent();
          base_dd_multiplier *= 1.0 + p->talents.sudden_onset->effectN( 1 ).percent();
          base_dd_multiplier *= 1.0 + p->hero.mark_of_xavius->effectN( 1 ).percent();
        }
      }

      void impact( action_state_t* s ) override
      {
        mg_extra_tick_base_t::impact( s );

        // Agony (Malefic Grasp) ticks also appear to be increasing the number of stacks of the Agony DoT
        if ( result_is_hit( s->result ) )
          td( s->target )->dots.agony->increment( 1 );
      }
    };

    struct unstable_affliction_mg_t : public mg_extra_tick_base_t
    {
      unstable_affliction_mg_t( warlock_t* p )
        : mg_extra_tick_base_t( "Unstable Affliction (Malefic Grasp)", p, ( p->talents.malefic_grasp.ok() && p->talents.unstable_affliction.ok() ) ? p->talents.unstable_affliction_mg : spell_data_t::not_found() )
      { }
    };

    // NOTE: 2026-02-20 Corruption (Malefic Grasp) ticks do not trigger Nightfall (we assume they also do not contribute to the accumulator)
    struct corruption_mg_t : public mg_extra_tick_base_t
    {
      corruption_mg_t( warlock_t* p )
        : mg_extra_tick_base_t( "Corruption (Malefic Grasp)", p, ( p->talents.malefic_grasp.ok() && !p->hero.wither.ok() ) ? p->talents.corruption_mg : spell_data_t::not_found() )
      { }
    };

    struct wither_mg_t : public mg_extra_tick_base_t
    {
      wither_mg_t( warlock_t* p )
        : mg_extra_tick_base_t( "Wither (Malefic Grasp)", p, ( p->talents.malefic_grasp.ok() && p->hero.wither.ok() ) ? p->talents.wither_mg : spell_data_t::not_found() )
      {
        // NOTE: 2026-02-20 Wither (Malefic Grasp) extra tick is not affected by Hatefury Rituals because it does
        // not have an effect that affects direct damage (it only affects periodic damage)
        if ( !p->bugs )
          base_dd_multiplier *= 1.0 + p->hero.hatefury_rituals->effectN( 1 ).percent();
      }
    };

    const double extra_tick_mul;

    shadowbolt_volley_t* volley;

    agony_mg_t* agony_mg = nullptr;
    unstable_affliction_mg_t* unstable_affliction_mg = nullptr;
    corruption_mg_t* corruption_mg = nullptr;
    wither_mg_t* wither_mg = nullptr;

    malefic_grasp_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Malefic Grasp", p, p->talents.malefic_grasp.ok() ? p->talents.malefic_grasp_2 : spell_data_t::not_found(), options_str ),
      extra_tick_mul( p->talents.malefic_grasp_2->effectN( 2 ).percent() )
    {
      channeled = true;
      // NOTE: 2026-04-29 Malefic Grasp ticks are not affected by Death's Embrace (bug?)
      affected_by.deaths_embrace = !p->bugs && p->talents.deaths_embrace.ok();

      if ( p->talents.cunning_cruelty.ok() )
        volley = new shadowbolt_volley_t( p );

      if ( p->talents.malefic_grasp.ok() )
      {
        if ( p->talents.agony.ok() )
        {
          agony_mg = new agony_mg_t( p );
          add_child( agony_mg );
        }
        if ( p->talents.unstable_affliction.ok() )
        {
          unstable_affliction_mg = new unstable_affliction_mg_t( p );
          add_child( unstable_affliction_mg );
        }
        if ( p->hero.wither.ok() )
        {
          wither_mg = new wither_mg_t( p );
          add_child( wither_mg );
        }
        else
        {
          corruption_mg = new corruption_mg_t( p );
          add_child( corruption_mg );
        }
      }
    }

    action_state_t* new_state() override
    { return new malefic_grasp_state_t( this, target ); }

    bool ready() override
    {
      if ( !p()->talents.malefic_grasp.ok() || p()->warlock_pet_list.darkglares.n_active_pets() <= 0 )
        return false;

      return warlock_spell_t::ready();
    }

    void snapshot_state( action_state_t* s, result_amount_type rt ) override
    {
      // NOTE: 2026-02-20 Malefic Grasp does not benefit from the Nightfall damage bonus under any circumstances (bug)
      double dmg_mul = p()->bugs ? 0.0 : p()->talents.nightfall_buff->effectN( 2 ).percent();

      debug_cast<malefic_grasp_state_t*>( s )->td_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? dmg_mul : 0.0 );
      debug_cast<malefic_grasp_state_t*>( s )->tick_time_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? p()->talents.nightfall_buff->effectN( 3 ).percent() : 0.0 );
      warlock_spell_t::snapshot_state( s, rt );
    }

    double tick_time_pct_multiplier( const action_state_t* s ) const override
    {
      auto mul = warlock_spell_t::tick_time_pct_multiplier( s );

      mul *= debug_cast<const malefic_grasp_state_t*>( s )->tick_time_multiplier;

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
          p()->proc_actions.shared_fate->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && p()->prd_rng.feast_of_souls->trigger( execute_state ) )
          p()->feast_of_souls_gain();
      }
      p()->buffs.nightfall->decrement();
    }

    void tick( dot_t* d ) override
    {
      warlock_spell_t::tick( d );

      if ( result_is_hit( d->state->result ) )
      {
        // NOTE: 2026-02-20 Malefic Grasp can proc Shard Instability
        if ( p()->talents.shard_instability.ok() && p()->prd_rng.shard_instability_ds->trigger() )
        {
          p()->buffs.shard_instability->trigger();
          p()->procs.shard_instability->occur();
        }

        // NOTE: 2026-02-20 Malefic Grasp can proc Cunning Cruelty (PRD: 50% nominal rate if SB is used, 25% nominal rate if DS is used)
        if ( p()->talents.cunning_cruelty.ok() && p()->prd_rng.cunning_cruelty->trigger() )
        {
          p()->procs.shadowbolt_volley->occur();
          volley->execute_on_target( d->target );
        }

        warlock_td_t* tdata = td( d->target );
        if ( !tdata )
          return;

        // Trigger extra DoT Ticks
        trigger_extra_tick( tdata->dots.agony, extra_tick_mul, agony_mg );
        trigger_extra_tick( tdata->dots.unstable_affliction, extra_tick_mul, unstable_affliction_mg );
        trigger_extra_tick( tdata->dots.wither, extra_tick_mul, wither_mg, false );
        trigger_extra_tick( tdata->dots.corruption, extra_tick_mul, corruption_mg );
      }
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      m *= debug_cast<const malefic_grasp_state_t*>( s )->td_multiplier;

      return m;
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
        action_state_t::debug_str( s );
        s << " tick_time_multiplier=" << tick_time_multiplier;
        s << " td_multiplier=" << td_multiplier;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        tick_time_multiplier = debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;
        td_multiplier = debug_cast<const drain_soul_state_t*>( s )->td_multiplier;
      }
    };

    shadowbolt_volley_t* volley;

    drain_soul_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Drain Soul", p, p->talents.drain_soul.ok() ? p->talents.drain_soul_dot : spell_data_t::not_found(), options_str )
    {
      channeled = true;

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

      if ( p->talents.cunning_cruelty.ok() )
        volley = new shadowbolt_volley_t( p );
    }

    action_state_t* new_state() override
    { return new drain_soul_state_t( this, target ); }

    void snapshot_state( action_state_t* s, result_amount_type rt ) override
    {
      // NOTE: 2026-02-20 Nightfall does not buff Drain Soul damage unless the Necrolyte Teachings hero talent (Soul Harvester) is used (bug)
      double dmg_mul = ( p()->bugs && !p()->hero.necrolyte_teachings.ok() ) ? 0.0 : p()->talents.nightfall_buff->effectN( 2 ).percent();

      debug_cast<drain_soul_state_t*>( s )->td_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? dmg_mul : 0.0 );
      debug_cast<drain_soul_state_t*>( s )->tick_time_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? p()->talents.nightfall_buff->effectN( 3 ).percent() : 0.0 );
      warlock_spell_t::snapshot_state( s, rt );
    }

    double tick_time_pct_multiplier( const action_state_t* s ) const override
    {
      auto mul = warlock_spell_t::tick_time_pct_multiplier( s );

      mul *= debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;

      return mul;
    }

    bool ready() override
    {
      if ( p()->talents.malefic_grasp.ok() && p()->warlock_pet_list.darkglares.n_active_pets() > 0 )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( soul_harvester() && p()->buffs.nightfall->check() )
      {
        if ( p()->hero.wicked_reaping.ok() )
          p()->proc_actions.wicked_reaping->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.shared_fate.ok() )
          p()->proc_actions.shared_fate->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && p()->prd_rng.feast_of_souls->trigger( execute_state ) )
          p()->feast_of_souls_gain();
      }
      p()->buffs.nightfall->decrement();
    }

    void tick( dot_t* d ) override
    {
      warlock_spell_t::tick( d );

      if ( result_is_hit( d->state->result ) )
      {
        if ( p()->talents.shard_instability.ok() && p()->prd_rng.shard_instability_ds->trigger() )
        {
          p()->buffs.shard_instability->trigger();
          p()->procs.shard_instability->occur();
        }

        if ( p()->talents.cunning_cruelty.ok() && p()->prd_rng.cunning_cruelty->trigger() )
        {
          p()->procs.shadowbolt_volley->occur();
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
        m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( ( int )( p()->talents.withering_bolt->effectN( 2 ).base_value() ), td( t )->count_affliction_dots() );

      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      m *= debug_cast<const drain_soul_state_t*>( s )->td_multiplier;

      return m;
    }
  };

  struct haunt_t : public warlock_spell_t
  {
    haunt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Haunt", p, p->talents.haunt, options_str )
    { }

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
        // Haunt debuff can only be active on one target at same time
        if ( p()->haunt_target )
          td( p()->haunt_target )->debuffs.haunt->expire();

        td( s->target )->debuffs.haunt->trigger();

        // TOCHECK: Does Wrath of Nathreza also proc from the initial Haunt hit?
        if ( p()->talents.shadow_of_nathreza_3.ok() )
          helpers::trigger_wrath_of_nathreza( p(), s->target );
      }
    }
  };

  struct summon_darkglare_t : public warlock_spell_t
  {
    summon_darkglare_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Summon Darkglare", p, p->talents.summon_darkglare, options_str )
    {
      harmful = callbacks = true; // Set to true because of 10.1 class trinket
      may_crit = may_miss = false;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.darkglares.spawn( data().duration() );
    }
  };

  struct dark_harvest_t : public warlock_spell_t
  {
    struct dark_harvest_dmg_t : public warlock_spell_t
    {
      dark_harvest_dmg_t( warlock_t* p )
        : warlock_spell_t( "Dark Harvest (tick)", p, p->talents.dark_harvest_dmg )
      {
        background = dual = true;
      }
    };

    dark_harvest_dmg_t* dark_harvest_dmg;

    dark_harvest_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Dark Harvest", p, p->talents.dark_harvest, options_str ),
      dark_harvest_dmg( new dark_harvest_dmg_t( p ) )
    {
      channeled = true;

      target_filter_callback = affliction_core_dots_only();

      add_child( dark_harvest_dmg );
    }

    bool ready() override
    {
      if ( !warlock_spell_t::ready() )
        return false;

      target_cache.is_valid = false;
      return !target_list().empty();
    }

    void tick( dot_t* d ) override
    {
      target_cache.is_valid = false;

      warlock_spell_t::tick( d );

      const auto& tl = target_list();
      for ( auto t : tl )
        dark_harvest_dmg->execute_on_target( t );

      // Shadow of Death gain starts on the second tick
      if ( soul_harvester() && p()->hero.shadow_of_death.ok() && d->current_tick > 0 )
      {
        // NOTE: 2026-02-20 The shards gained by Shadow of Death can also proc another Succulent Soul each (bug?)
        double gain = p()->hero.shadow_of_death->effectN( 2 ).base_value();
        if ( p()->bugs )
          p()->resource_gain( RESOURCE_SOUL_SHARD, gain, p()->gains.shadow_of_death );
        else
          p()->player_t::resource_gain( RESOURCE_SOUL_SHARD, gain, p()->gains.shadow_of_death );

        p()->buffs.succulent_soul->trigger( as<int>( gain ) );
        for ( int i = 0; i < as<int>( gain ); i++ )
          p()->procs.succulent_soul->occur();
      }
    }

    void execute() override
    {
      target_cache.is_valid = false;

      warlock_spell_t::execute();
    }
  };

  struct summon_desperate_soul_t : public warlock_spell_t
  {
    summon_desperate_soul_t( warlock_t* p )
      : warlock_spell_t( "Summon Desperate Soul", p, p->talents.summon_desperate_soul )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.desperate_souls.spawn( data().duration() );
    }
  };

  struct shadow_of_nathreza_dmg_t : public warlock_spell_t
  {
    shadow_of_nathreza_dmg_t( warlock_t* p )
      : warlock_spell_t( "Shadow of Nathreza", p, p->talents.shadow_of_nathreza_dot )
    {
      background = dual = true;

      target_filter_callback = primary_target_or( corruption_or_wither_only() );
    }

    void execute() override
    {
      target_cache.is_valid = false;

      warlock_spell_t::execute();
    }
  };

  // Affliction Actions End
  // Demonology Actions Begin

  struct summon_wild_imp_base_t : public warlock_spell_t
  {
    std::vector<warlock::pets::demonology::wild_imp_pet_t*> last_summoned_imps;

    summon_wild_imp_base_t( util::string_view n, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
      : warlock_spell_t( n, p, s )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      last_summoned_imps = p()->warlock_pet_list.wild_imps.spawn( data().duration() );

      // Wild Imp summon spell triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );
    }

    std::vector<warlock::pets::demonology::wild_imp_pet_t*> execute_spawn( unsigned n_imps = 1 )
    {
      std::vector<warlock::pets::demonology::wild_imp_pet_t*> imps;
      for ( unsigned i = 0; i < n_imps; i++ )
      {
        execute();
        imps.insert( imps.end(), last_summoned_imps.begin(), last_summoned_imps.end() );
      }
      return imps;
    }
  };

  struct summon_wild_imp_t : public summon_wild_imp_base_t
  {
    summon_wild_imp_t( warlock_t* p )
      : summon_wild_imp_base_t( "Wild Imp (Summon)", p, p->warlock_base.wild_imp )
    { }
  };

  struct summon_wild_imp_2_t : public summon_wild_imp_base_t
  {
    summon_wild_imp_2_t( warlock_t* p )
      : summon_wild_imp_base_t( "Wild Imp (Summon) (Alternate)", p, p->warlock_base.wild_imp_2 )
    { }
  };

  struct hand_of_guldan_t : public warlock_spell_t
  {
    struct hand_of_guldan_state_t : public action_state_t
    {
      bool demonic_art_buffed;
      bool rancora_empowered;

      hand_of_guldan_state_t( action_t* action, player_t* target )
        : action_state_t( action, target ),
        demonic_art_buffed( false ),
        rancora_empowered( false )
      { }

      void initialize() override
      {
        action_state_t::initialize();
        demonic_art_buffed = false;
        rancora_empowered = false;
      }

      std::ostringstream& debug_str( std::ostringstream& s ) override
      {
        action_state_t::debug_str( s );
        s << " demonic_art_buffed=" << demonic_art_buffed;
        s << " rancora_empowered=" << rancora_empowered;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        demonic_art_buffed = debug_cast<const hand_of_guldan_state_t*>( s )->demonic_art_buffed;
        rancora_empowered = debug_cast<const hand_of_guldan_state_t*>( s )->rancora_empowered;
      }
    };

    struct hog_impact_t : public warlock_spell_t
    {
      struct hogi_state_t {
        int shards_used;
        bool rancora_empowered;
        int last_hit_random_target;

        hogi_state_t()
          : shards_used( 0 ),
          rancora_empowered( false ),
          last_hit_random_target( 0 )
        { }
      };

      struct hog_impact_state_t : public action_state_t
      {
        hogi_state_t state;

        hog_impact_state_t( action_t* action, player_t* target )
          : action_state_t( action, target ),
          state()
        { }

        void initialize() override
        {
          action_state_t::initialize();
          state.shards_used = 0;
          state.rancora_empowered = false;
          state.last_hit_random_target = 0;
        }

        std::ostringstream& debug_str( std::ostringstream& s ) override
        {
          action_state_t::debug_str( s );
          s << " shards_used=" << state.shards_used;
          s << " rancora_empowered=" << state.rancora_empowered;
          s << " last_hit_random_target=" << state.last_hit_random_target;
          return s;
        }

        void copy_state( const action_state_t* s ) override
        {
          action_state_t::copy_state( s );
          state = debug_cast<const hog_impact_state_t*>( s )->state;
        }
      };

      timespan_t meteor_time;
      hogi_state_t state;

      hog_impact_t( warlock_t* p )
        : warlock_spell_t( "Hand of Gul'dan (Impact)", p, p->talents.hog_impact ),
        meteor_time( 8_ms )
      {
        aoe = -1;
        dual = true;

        affected_by.touch_of_rancora = affected_by.touch_of_rancora_casted = p->hero.touch_of_rancora.ok();
      }

      action_state_t* new_state() override
      { return new hog_impact_state_t( this, target ); }

      void snapshot_state( action_state_t* s, result_amount_type rt ) override
      {
        debug_cast<hog_impact_state_t*>( s )->state = state;
        warlock_spell_t::snapshot_state( s, rt );
      }

      timespan_t travel_time() const override
      { return meteor_time; }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_da_multiplier( s );

        int last_hit_random_target = debug_cast<const hog_impact_state_t*>( s )->state.last_hit_random_target;

        // NOTE: Dominant Hand seems to be applied to a random target instead of the primary one (bug?)
        if ( p()->talents.dominant_hand.ok() && ( p()->bugs ? ( s->chain_target == last_hit_random_target ) : ( s->chain_target == 0 ) ) )
          m *= 1.0 + p()->talents.dominant_hand->effectN( 1 ).percent();

        bool rancora_empowered = debug_cast<const hog_impact_state_t*>( s )->state.rancora_empowered;
        if ( diabolist() && affected_by.touch_of_rancora && rancora_empowered )
        {
          // NOTE: 2026-03-17 Touch of Rancora is not affecting HoG damage (bug)
          if ( !p()->bugs )
            m *= 1.0 + p()->hero.touch_of_rancora->effectN( 1 ).percent();
        }

        return m;
      }

      void execute() override
      {
        // NOTE: Some effects only affect one of HoG's hits in AoE (bug?), randomly selected
        const std::vector<player_t*>& tl = target_list();
        state.last_hit_random_target     = rng().range( as<int>( tl.size() ) );

        // Wild Imp spawn events
        // NOTE: Old Behavior (pre Midnight):
        //   Wild Imp spawns appear to have been sped up in Shadowlands. Last tested 2021-04-16.
        //   HoG will spawn a meteor on cast finish. Travel time in spell data is 0.7 seconds.
        //   However, damage event occurs before spell effect lands, happening 0.4 seconds after cast.
        //   Imps then spawn roughly every 0.18 seconds after the damage event.
        // NOTE: New Behavior (from Midnight onwards):
        //   The HoG meteor damage event no longer takes 0.4 seconds after cast impact (only about 8ms).
        //   Wild Imps spawn on HoG meteor execute, not from HoG meteor damage impact.
        //   Wild Imps spawn sequentially, with a 1-2ms delay from one spawn to the next.
        //   Last tested: 2026-05-03
        static constexpr std::array<double, 2> imp_delay{ 1.0, 2.0 };

        double delay = 0.0;
        for ( int i = 1; i <= state.shards_used; i++ )
        {
          delay += rng().range( imp_delay );
          const double expected_delay = static_cast<double>( ( 3 * i + 1 ) / 2 );

          auto ev = make_event<imp_delay_event_t>( *sim, p(), delay, expected_delay, i - 1 );
          p()->wild_imp_spawns.push_back( ev );
        }

        warlock_spell_t::execute();
      }

      double action_multiplier() const override
      {
        double m = warlock_spell_t::action_multiplier();

        double gloom = 0.0;

        if ( p()->hero.gloom_of_nathreza.ok() )
           gloom = state.shards_used * p()->hero.gloom_of_nathreza->effectN( 1 ).percent();

        m *= state.shards_used * ( 1.0 + gloom );

        return m;
      }
    };

    hog_impact_t* impact_spell;

    hand_of_guldan_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Hand of Gul'dan", p, p->talents.hand_of_guldan_cast, options_str ),
      impact_spell( new hog_impact_t( p ) )
    {
      affected_by.touch_of_rancora = affected_by.touch_of_rancora_casted = p->hero.touch_of_rancora.ok();

      triggers.diabolic_ritual = triggers.demonic_art = p->hero.diabolic_ritual.ok();

      add_child( impact_spell );
    }

    action_state_t* new_state() override
    { return new hand_of_guldan_state_t( this, target ); }

    // Small travel time to mimic ingame ordering of cast-consumed effects
    timespan_t travel_time() const override
    { return 1_ms; }

    bool ready() override
    {
      if ( diabolist() && p()->executing != this && p()->buffs.ruination->check() )
        return false;

      return warlock_spell_t::ready();
    }

    void schedule_execute( action_state_t* s ) override
    {
      // NOTE: Any of the Demonic Art buffs must be present when the cast begins for the spell to be empowered by Touch of Rancora
      // and/or for the Demonic Art buff to be consumed upon executing the spell
      if ( diabolist() && triggers.demonic_art )
      {
        action_state_t*& action_state = s ? s : pre_execute_state;
        if ( !action_state )
          action_state = get_state();

        const bool demonic_art_buff_up = p()->buffs.art_overlord->check() || p()->buffs.art_mother->check() || p()->buffs.art_pit_lord->check();
        debug_cast<hand_of_guldan_state_t*>( action_state )->demonic_art_buffed = demonic_art_buff_up;
        debug_cast<hand_of_guldan_state_t*>( action_state )->rancora_empowered = affected_by.touch_of_rancora && demonic_art_buff_up;
      }

      warlock_spell_t::schedule_execute( s );
    }

    void execute() override
    {
      int shards_used = as<int>( cost() );
      impact_spell->state.shards_used = shards_used;
      if ( pre_execute_state )
      {
        snapshot_state( pre_execute_state, amount_type( pre_execute_state ) );
        // An incoming rancora empowered casted spell will remain empowered even if the Demonic Art buff falls off during cast
        impact_spell->state.rancora_empowered = debug_cast<hand_of_guldan_state_t*>( pre_execute_state )->rancora_empowered;
        // Casted spells do not consume any Demonic Art buff if none were active at the start of the cast
        triggers.demonic_art_buff = debug_cast<hand_of_guldan_state_t*>( pre_execute_state )->demonic_art_buffed;
      }
      else
      {
        impact_spell->state.rancora_empowered = false;
        triggers.demonic_art_buff = false;
      }

      if ( p()->hero.diabolic_oculi.ok() )
        p()->buffs.demonic_oculi->trigger();

      warlock_spell_t::execute();

      if ( p()->talents.doom.ok() )
      {
        for ( const auto t : p()->sim->target_non_sleeping_list )
        {
          if ( td( t )->debuffs.doom->check() )
            td( t )->debuffs.doom->extend_duration( -p()->talents.doom->effectN( 1 ).time_value() );
        }
      }

      if ( p()->talents.demonic_knowledge.ok() && p()->deck_rng.demonic_knowledge->trigger() )
      {
        p()->buffs.demonic_core->trigger();
        p()->procs.demonic_knowledge->occur();
      }

      if ( soul_harvester() && p()->buffs.succulent_soul->check() )
      {
          p()->buffs.succulent_soul->decrement();

          if ( p()->hero.manifested_avarice.ok() && p()->prd_rng.manifested_avarice->trigger() )
          {
            p()->summons.manifested_demonic_soul->execute();
            p()->procs.manifested_avarice->occur();
          }

          p()->proc_actions.demonic_soul->execute_on_target( target );
      }

      // TODO: Are these execute, or impact? Check ingame timings to see when these effects are applied
      if ( p()->talents.dominion_of_argus_1.ok() && p()->buffs.dominion_of_argus->check() )
        p()->buffs.dominion_of_argus->trigger();

      if ( p()->talents.dominion_of_argus_3.ok() && p()->buffs.dominion_of_argus->check() )
        p()->resource_gain( RESOURCE_SOUL_SHARD, p()->talents.dominion_of_argus_3_gain->effectN( 1 ).resource(),
                            p()->gains.dominion_of_argus );
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      impact_spell->execute_on_target( s->target );
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

      affected_by.sacrificed_souls = p->talents.sacrificed_souls.ok();
    }

    action_state_t* new_state() override
    { return new demonbolt_state_t( this, target ); }

    void snapshot_state( action_state_t* s, result_amount_type rt ) override
    {
      debug_cast<demonbolt_state_t*>( s )->core_spent = p()->buffs.demonic_core->up();
      warlock_spell_t::snapshot_state( s, rt );
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.demonic_core->up(); // For benefit tracking

      if ( p()->buffs.demonic_core->check() )
      {
        if ( p()->talents.spiteful_reconstitution.ok() && p()->prd_rng.spiteful_reconstitution->trigger() )
        {
          p()->summons.wild_imp_2->execute();
          p()->procs.spiteful_reconstitution->occur();
        }
      }

      if ( soul_harvester() && p()->buffs.demonic_core->check() )
      {
        if ( p()->hero.wicked_reaping.ok() )
          p()->proc_actions.wicked_reaping->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.shared_fate.ok() )
          p()->proc_actions.shared_fate->execute_on_target( target );

        if ( p()->hero.quietus.ok() && p()->hero.feast_of_souls.ok() && p()->prd_rng.feast_of_souls->trigger() )
          p()->feast_of_souls_gain();
      }

      if ( p()->talents.summon_doomguard.ok() && p()->buffs.demonic_core->check() )
        p()->cooldowns.summon_doomguard->adjust( timespan_t::from_seconds( -p()->talents.summon_doomguard->effectN( 2 ).base_value() ) );

      // Demonbolt energize spell triggers procs
      p()->trigger_aura_applied_callbacks( p()->proc_data_entries.demonbolt_energize, p() );

      p()->buffs.demonic_core->decrement();

      p()->buffs.power_siphon->decrement();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.doom.ok() && debug_cast<demonbolt_state_t*>( s )->core_spent && !td( s->target )->debuffs.doom->check() )
        td( s->target )->debuffs.doom->trigger();
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
      }

      double action_multiplier() const override
      {
        double m = warlock_spell_t::action_multiplier();

        // NOTE: 2026-02-17 The Imp Gang Boss Wild Imps implosions do not do 100% more damage (bug?)
        if ( !p()->bugs && debug_cast<pets::demonology::wild_imp_pet_t*>( next_imp )->buffs.imp_gang_boss->check() )
          m *= 1.0 + p()->talents.imp_gang_boss_buff->effectN( 2 ).percent();

        // NOTE: 2026-02-17 The Unstable Soul Wild Imps implosions do not do 50% more damage (bug?)
        if ( !p()->bugs && debug_cast<pets::demonology::wild_imp_pet_t*>( next_imp )->buffs.unstable_soul->check() )
          m *= 1.0 + p()->talents.unstable_soul_buff->effectN( 1 ).percent();

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

      auto imps = p()->warlock_pet_list.wild_imps.active_pets();

      // NOTE: 2026-02-17: Seems than older wild imps (or with less energy) are prioritized for implosion.
      // It hasn't yet been determined whether those with less energy or the oldest are prioritized first.
      // The Imp Gang Boss / Unstable Soul buffs do not seem to affect the selection.
      // There also seem to exist some unusual interactions with the priority of wild imps to implode (not implemented):
      // - The distance of the wild imps from the player can affect their selection.
      // - When there are many imps (more than 9), the selection of some of them seems to become somewhat random
      //   (maybe not random; in any case, their actual behavior in this situation has not been fully determined).
      range::sort( imps, [ &bugs = p()->bugs ]( const pets::demonology::wild_imp_pet_t* imp1, const pets::demonology::wild_imp_pet_t* imp2 ) {
        double lv = imp1->resources.current[ RESOURCE_ENERGY ];
        double rv = imp2->resources.current[ RESOURCE_ENERGY ];
        if ( lv == rv )
          return imp1->actor_spawn_index < imp2->actor_spawn_index;

        return lv < rv;
      } );

      unsigned launch_counter = 0;
      for ( auto imp : imps )
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

        if ( launch_counter >= as<unsigned>( data().effectN( 1 ).base_value() ) )
          break;
      }
      if ( p()->talents.to_hell_and_back.ok() )
      {
        unsigned new_imps = ( launch_counter / as<unsigned>( p()->talents.to_hell_and_back->effectN( 2 ).base_value() ) ) * as<unsigned>( p()->talents.to_hell_and_back->effectN( 1 ).base_value() );
        if ( new_imps > 0 )
        {
          auto imps = debug_cast<summon_wild_imp_2_t*>( p()->summons.wild_imp_2 )->execute_spawn( new_imps );
          for ( auto imp : imps )
          {
            imp->buffs.imp_gang_boss->trigger();
            imp->buffs.unstable_soul->trigger();
          }
        }
      }

      warlock_spell_t::execute();

      // Implosion cast triggers procs through some hidden trigger
      p()->trigger_aura_applied_callbacks( proc_data, p() );
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

  struct summon_vilefiend_base_t : public warlock_spell_t
  {
    summon_vilefiend_base_t( util::string_view n, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
      : warlock_spell_t( n, p, s )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.vilefiends.spawn( data().duration() );
    }
  };

  struct summon_vilefiend_t : public summon_vilefiend_base_t
  {
    summon_vilefiend_t( warlock_t* p )
      : summon_vilefiend_base_t( "Summon Vilefiend", p, p->talents.vilefiend )
    { }
  };

  struct summon_gloomhound_t : public summon_vilefiend_base_t
  {
    summon_gloomhound_t( warlock_t* p )
      : summon_vilefiend_base_t( "Summon Gloomhound", p, p->talents.gloomhound )
    { }
  };

  struct summon_charhound_t : public summon_vilefiend_base_t
  {
    summon_charhound_t( warlock_t* p )
      : summon_vilefiend_base_t( "Summon Charhound", p, p->talents.charhound )
    { }
  };

  struct summon_dreadstalker_base_t : public warlock_spell_t
  {
    timespan_t dur_adjust;
    timespan_t server_action_delay;

    summon_dreadstalker_base_t( util::string_view n, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
      : warlock_spell_t( n, p, s )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      auto dogs = p()->warlock_pet_list.dreadstalkers.spawn( data().duration() + dur_adjust );

      for ( auto dog : dogs )
      {
        if ( dog->is_active() )
          dog->server_action_delay = server_action_delay;
      }

      // Call Dreadstalkers summon spell triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );
    }

    void execute( timespan_t dur_adjust_, timespan_t server_action_delay_ )
    {
      dur_adjust = dur_adjust_;
      server_action_delay = server_action_delay_;
      execute();
    }
  };

  struct summon_dreadstalker_1_t : public summon_dreadstalker_base_t
  {
    summon_dreadstalker_1_t( warlock_t* p )
      : summon_dreadstalker_base_t( "Call Dreadstalkers (Summon) (1)", p, p->talents.call_dreadstalkers_summon_1 )
    { }
  };

  struct summon_dreadstalker_2_t : public summon_dreadstalker_base_t
  {
    summon_dreadstalker_2_t( warlock_t* p )
      : summon_dreadstalker_base_t( "Call Dreadstalkers (Summon) (2)", p, p->talents.call_dreadstalkers_summon_2 )
    { }
  };

  struct call_dreadstalkers_t : public warlock_spell_t
  {
    call_dreadstalkers_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Call Dreadstalkers", p, p->talents.call_dreadstalkers, options_str )
    {
      may_crit = false;
      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      unsigned count = as<unsigned>( p()->talents.call_dreadstalkers->effectN( 1 ).base_value() );

      const auto delay_dur_adjusts = p()->dreadstalkers_delay_duration_adjustment_helper( *target );
      const timespan_t& delay = delay_dur_adjusts.first;
      const timespan_t& dur_adjust = delay_dur_adjusts.second;

      for ( unsigned i = 0; i < count; i++ )
      {
        summon_dreadstalker_base_t* summon_dreadstalker_action = i ? debug_cast<summon_dreadstalker_base_t*>( p()->summons.dreadstalker_2 )
                                                                   : debug_cast<summon_dreadstalker_base_t*>( p()->summons.dreadstalker_1 );

        summon_dreadstalker_action->execute( dur_adjust, delay );
      }

      if ( p()->talents.summon_vilefiend.ok() )
        p()->summons.vilefiend->execute_on_target( target );
    }
  };

  // Dreadstalkers Blighted Maw uses Player as a source, not the Pet
  struct blighted_maw_t : public warlock_spell_t
  {
    blighted_maw_t( warlock_t* p )
      : warlock_spell_t( "Blighted Maw", p, p->talents.blighted_maw_dmg )
    {
      background = dual = true;
      may_crit = false;
      base_dd_min = base_dd_max = 0;
    }

    void init_finished() override
    {
      warlock_spell_t::init_finished();

      // NOTE: Blighted Maw was expected to ignore multipliers, but it doesn't (bug?)
      if ( !p()->bugs )
        snapshot_flags &= STATE_NO_MULTIPLIER;
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

      range::sort( imps, [ &bugs = p()->bugs ]( const pets::demonology::wild_imp_pet_t* imp1, const pets::demonology::wild_imp_pet_t* imp2 ) {
        double lv = imp1->resources.current[ RESOURCE_ENERGY ];
        double rv = imp2->resources.current[ RESOURCE_ENERGY ];

        // Power Siphon deprioritizes Wild Imps that are Gang Bosses
        // Padding ensures they still sort in order at the back of the list
        // NOTE: 2026-02-17: This is not longer true in Midnight (bug?)
        if ( !bugs )
        {
          lv += ( imp1->buffs.imp_gang_boss->check() ) ? 200.0 : 0.0;
          rv += ( imp2->buffs.imp_gang_boss->check() ) ? 200.0 : 0.0;
        }

        if ( lv == rv )
        {
          // NOTE: In Midnight, if they have the same energy, they are no longer
          // prioritized by expiration time first, but directly by spawn time (bug?)
          if ( bugs )
          {
            return imp1->actor_spawn_index < imp2->actor_spawn_index;
          }
          else
          {
            timespan_t lr = imp1->expiration->remains();
            timespan_t rr = imp2->expiration->remains();
            if ( lr == rr )
            {
              return imp1->actor_spawn_index < imp2->actor_spawn_index;
            }
            return lr < rr;
          }
        }

        return lv < rv;
      } );

      unsigned max_imps = as<int>( p()->talents.power_siphon->effectN( 1 ).base_value() );

      if ( imps.size() > max_imps )
        imps.resize( max_imps );

      unsigned sac_counter = 0;
      while ( !imps.empty() )
      {
        p()->buffs.power_siphon->trigger();
        p()->buffs.demonic_core->trigger();
        pets::demonology::wild_imp_pet_t* imp = imps.front();
        imps.erase( imps.begin() );
        imp->power_siphon = true;
        imp->dismiss();
        sac_counter++;
      }

      if ( p()->talents.to_hell_and_back.ok() )
      {
        unsigned new_imps = ( sac_counter / as<unsigned>( p()->talents.to_hell_and_back->effectN( 2 ).base_value() ) ) * as<unsigned>( p()->talents.to_hell_and_back->effectN( 1 ).base_value() );
        if ( new_imps > 0 )
        {
          auto imps = debug_cast<summon_wild_imp_2_t*>( p()->summons.wild_imp_2 )->execute_spawn( new_imps );
          for ( auto imp : imps )
          {
            imp->buffs.imp_gang_boss->trigger();
            imp->buffs.unstable_soul->trigger();
          }
        }
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

      int demonic_power_counter = 0;
      const timespan_t extension_time = 15_s; // TODO: Where is this 15_s in the spell data?

      for ( auto dreadstalker : p()->warlock_pet_list.dreadstalkers )
      {
        if ( dreadstalker->is_sleeping() )
          continue;

        if ( p()->talents.reign_of_tyranny.ok() )
        {
          if ( dreadstalker->expiration )
            dreadstalker->expiration->reschedule_time = dreadstalker->expiration->time + extension_time;
        }

        demonic_power_counter++;
      }

      for ( auto wild_imp : p()->warlock_pet_list.wild_imps )
      {
        if ( !wild_imp->is_sleeping() )
          demonic_power_counter++;
      }

      // NOTE: 2026-04-24: Vilefiend (all variants) and Felguard count for Demonic Power buff (only at Tyrant summon) (bug?)
      if ( p()->bugs )
      {
        for ( auto vilefiend : p()->warlock_pet_list.vilefiends )
        {
          if ( !vilefiend->is_sleeping() )
            demonic_power_counter++;
        }

        auto active_pet = p()->warlock_pet_list.active;
        if ( active_pet && active_pet->pet_type == PET_FELGUARD )
          demonic_power_counter++;
      }

      if ( p()->talents.reign_of_tyranny.ok() )
      {
        if ( p()->buffs.dreadstalkers->check() )
          p()->buffs.dreadstalkers->extend_duration( extension_time );
      }

      if ( demonic_power_counter > 0 )
      {
        for ( auto t : tyrants )
        {
          if ( t->is_active() )
            t->buffs.demonic_power->trigger( demonic_power_counter );
        }
      }

      p()->buffs.tyrant->trigger();

      if ( p()->talents.tyrants_oblation.ok() )
        p()->buffs.tyrants_oblation->trigger();

      if ( p()->hero.abyssal_dominion.ok() )
        p()->buffs.abyssal_dominion->trigger();

      if ( p()->hero.cruelty_of_kerxan.ok() )
      {
        timespan_t reduction = -p()->hero.cruelty_of_kerxan->effectN( 1 ).time_value();

        p()->buffs.ritual_overlord->extend_duration( reduction );
        p()->buffs.ritual_mother->extend_duration( reduction );
        p()->buffs.ritual_pit_lord->extend_duration( reduction );
      }

      if ( soul_harvester() && p()->hero.shadow_of_death.ok() )
      {
        // NOTE: 2026-02-20 The shards gained by Shadow of Death can also proc another Succulent Soul each (bug?)
        double gain = p()->hero.shadow_of_death_energize->effectN( 1 ).base_value() / 10.0;
        if ( p()->bugs )
          p()->resource_gain( RESOURCE_SOUL_SHARD, gain, p()->gains.shadow_of_death );
        else
          p()->player_t::resource_gain( RESOURCE_SOUL_SHARD, gain, p()->gains.shadow_of_death );

        p()->buffs.succulent_soul->trigger( as<int>( gain ) );
        for ( int i = 0; i < as<int>( gain ); i++ )
          p()->procs.succulent_soul->occur();
      }

      if ( p()->talents.dominion_of_argus_1.ok() )
        p()->buffs.dominion_of_argus->trigger();

    }
  };

  struct grimoire_imp_lord_t : public warlock_spell_t
  {
    grimoire_imp_lord_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Grimoire: Imp Lord", p, p->talents.grimoire_imp_lord, options_str )
    {
      harmful = may_crit = false;

      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.grimoire_imp_lords.spawn( data().duration() );

      // Grimoire: Imp Lord summon spell triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );
    }
  };

  struct grimoire_fel_ravager_t : public warlock_spell_t
  {
    grimoire_fel_ravager_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Grimoire: Fel Ravager", p, p->talents.grimoire_fel_ravager, options_str )
    {
      harmful = may_crit = false;

      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.grimoire_fel_ravagers.spawn( data().duration() );

      // Grimoire: Fel Ravager summon spell triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );
    }
  };

  struct summon_doomguard_t : public warlock_spell_t
  {
    summon_doomguard_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Summon Doomguard", p, p->talents.summon_doomguard, options_str )
    {
      harmful = may_crit = false;

      triggers.diabolic_ritual = p->hero.diabolic_ritual.ok();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.doomguards.spawn( data().duration() );

      // Summon Doomguard summon spell triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );
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
    }
  };

  struct summon_lady_sacrolash_t : public warlock_spell_t
  {
    summon_lady_sacrolash_t( std::string_view n, warlock_t* p )
      : warlock_spell_t( n, p, p->talents.doa_lady_sacrolash_summon )
    {
      harmful = may_crit = false;
      background = not_a_proc = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();
      p()->warlock_pet_list.lady_sacrolash.spawn( data().duration() );
    }
  };

  struct summon_grand_warlock_alythess_t : public warlock_spell_t
  {
    summon_grand_warlock_alythess_t( std::string_view n, warlock_t* p )
      : warlock_spell_t( n, p, p->talents.doa_grand_warlock_alythess_summon )
    {
      harmful = may_crit = false;
      background = not_a_proc = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();
      p()->warlock_pet_list.grand_warlock_alythess.spawn( data().duration() );
    }
  };

  struct summon_antoran_inquisitor_t : public warlock_spell_t
  {
    summon_antoran_inquisitor_t( std::string_view n, warlock_t* p )
      : warlock_spell_t( n, p, p->talents.doa_antoran_inquisitor_summon )
    {
      harmful = may_crit = false;
      background = not_a_proc = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();
      p()->warlock_pet_list.antoran_inquisitor.spawn( data().duration() );
    }
  };

  struct summon_antoran_jailer_t : public warlock_spell_t
  {
    summon_antoran_jailer_t( std::string_view n, warlock_t* p )
      : warlock_spell_t( n, p, p->talents.doa_antoran_jailer_summon )
    {
      harmful = may_crit = false;
      background = not_a_proc = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();
      p()->warlock_pet_list.antoran_jailer.spawn( data().duration() );
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

        triggers.fiendish_cruelty = p->talents.fiendish_cruelty.ok(); // Incinerate FnB crits can trigger Fiendish Cruelty
        triggers.embers_of_nihilam_1 = p->talents.embers_of_nihilam_1.ok(); // Incinerate FnB hits can trigger Embers of Nihilam 1

        base_multiplier *= p->talents.fire_and_brimstone->effectN( 1 ).percent();
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

      triggers.fiendish_cruelty = p->talents.fiendish_cruelty.ok();
      triggers.embers_of_nihilam_1 = p->talents.embers_of_nihilam_1.ok();

      add_child( fnb_action );
    }

    // Custom init() to combine Havoc+FnB coefficients instead of using the generic warlock_spell_t::init() Havoc multiplier
    void init() override
    {
      action_base_t::init();

      if ( affected_by.havoc )
      {
        // NOTE: The FnB talent adds its bonus damage to Incinerate Havoc (regardless of havoc target range)
        base_aoe_multiplier *= p()->talents.havoc_debuff->effectN( 1 ).percent() +
                               p()->talents.fire_and_brimstone->effectN( 1 ).percent();
        p()->havoc_spells.push_back( this );
      }
    }

    bool ready() override
    {
      if ( diabolist() && p()->executing != this && p()->buffs.infernal_bolt->check() )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.fire_and_brimstone.ok() )
        fnb_action->execute_on_target( target );

      if ( p()->talents.demonfire_infusion.ok() && p()->flat_rng.demonfire_infusion_inc->trigger() )
      {
        p()->proc_actions.demonfire_infusion->execute_on_target( target );
        p()->procs.demonfire_infusion_inc->occur();
      }

      // Incinerate energize spell triggers procs
      p()->trigger_aura_applied_callbacks( p()->proc_data_entries.incinerate_energize, p() );

      // Backdraft is not consumed by an instant Incinerate cast benefiting from Chaotic Inferno
      // NOTE: To achieve this, the game checks if the player has the Chaotic Inferno buff
      bool consume_backdraft = p()->bugs ? !p()->buffs.chaotic_inferno->check() : ( time_to_execute != 0_ms );
      if ( consume_backdraft )
        p()->buffs.backdraft->decrement();

      // Chaotic Inferno buff is only consumed by an Incinerate cast that benefits from the effect
      if ( time_to_execute == 0_ms )
        p()->buffs.chaotic_inferno->decrement();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      // TOCHECK: 2025-08-27 Incinerate Havoc crit impacts don't give extra shards (bug?), and only 1 extra shard with Diabolic Embers
      if ( s->result == RESULT_CRIT )
      {
        if ( !p()->bugs || s->chain_target == 0 )
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1 * energize_mult, p()->gains.incinerate_crits );
        else if ( p()->talents.diabolic_embers.ok() )
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.incinerate_crits );
      }
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
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( d->state->result == RESULT_CRIT && p()->flat_rng.immolate_crit_energize->trigger() )
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits );

        p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate );

        if ( p()->talents.flashpoint.ok() && d->target->health_percentage() >= p()->talents.flashpoint->effectN( 2 ).base_value() )
          p()->buffs.flashpoint->trigger();

        if ( p()->talents.demonfire_infusion.ok() && p()->flat_rng.demonfire_infusion_dot->trigger() )
        {
          p()->proc_actions.demonfire_infusion->execute_on_target( d->target );
          p()->procs.demonfire_infusion_dot->occur();
        }

        // Immolate DoT ticks trigger procs through some hidden trigger
        p()->trigger_aura_applied_callbacks( proc_data, p() );
      }
    };

    immolate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Immolate (direct)", p, p->warlock_base.immolate->ok() && !p->hero.wither.ok() ? p->warlock_base.immolate_old : spell_data_t::not_found(), options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      impact_action = new immolate_dot_t( p );
      add_child( impact_action );
    }

    immolate_t( warlock_t* p, bool havoc, util::string_view options_str ) : immolate_t( p, options_str )
    { affected_by.havoc = havoc; }

    dot_t* get_dot( player_t* t ) override
    { return impact_action->get_dot( t ); }
  };

  struct internal_combustion_t : public warlock_spell_t
  {
    internal_combustion_t( warlock_t* p )
      : warlock_spell_t( "Internal Combustion", p, p->talents.internal_combustion_dmg )
    {
      background = dual = true;
    }

    void init() override
    {
      warlock_spell_t::init();

      snapshot_flags &= STATE_NO_MULTIPLIER;
    }

    void execute() override
    {
      dot_t* dot = p()->hero.wither.ok() ? td( target )->dots.wither : td( target )->dots.immolate;

      assert( dot->current_action );
      action_state_t* state = dot->current_action->get_state( dot->state );
      dot->current_action->calculate_tick_amount( state, 1.0 );

      double tick_base_damage = state->result_raw;
      timespan_t remaining = std::min( dot->remains(), timespan_t::from_seconds( p()->talents.internal_combustion->effectN( 1 ).base_value() ) );
      timespan_t dot_tick_time = dot->current_action->tick_time( state );
      double ticks_left = remaining / dot_tick_time;
      double total_damage = ticks_left * tick_base_damage;

      action_state_t::release( state );

      base_dd_min = base_dd_max = total_damage;

      warlock_spell_t::execute();

      dot->adjust_duration( -remaining );
      if ( p()->hero.wither.ok() && remaining != 0_ms )
      {
        auto& wither_debuff = td( target )->debuffs.wither;
        if ( wither_debuff->remains() - remaining <= timespan_t::zero() )
          wither_debuff->expire();
        else
          wither_debuff->extend_duration( -remaining );

        assert( dot->current_stack() == wither_debuff->check() && dot->remains() == wither_debuff->remains() );
      }
    }
  };

  struct summon_shadowy_tear_t : public warlock_spell_t
  {
    summon_shadowy_tear_t( warlock_t* p )
      : warlock_spell_t( "Shadowy Tear (Summon)", p, p->talents.shadowy_tear_summon )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.shadowy_rifts.spawn( data().duration() );
    }
  };

  struct summon_unstable_tear_t : public warlock_spell_t
  {
    summon_unstable_tear_t( warlock_t* p )
      : warlock_spell_t( "Unstable Tear (Summon)", p, p->talents.unstable_tear_summon )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.unstable_rifts.spawn( data().duration() );
    }
  };

  struct summon_chaos_tear_t : public warlock_spell_t
  {
    summon_chaos_tear_t( warlock_t* p )
      : warlock_spell_t( "Chaos Tear (Summon)", p, p->talents.chaos_tear_summon )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.chaos_rifts.spawn( data().duration() );
    }
  };

  struct summon_overfiend_t : public warlock_spell_t
  {
    summon_overfiend_t( warlock_t* p )
      : warlock_spell_t( "Summon Overfiend", p, p->talents.summon_overfiend )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.overfiends.spawn( data().duration() );

      p()->buffs.summon_overfiend->trigger();
    }
  };

  struct dimensional_rift_t : public warlock_spell_t
  {
    dimensional_rift_t( warlock_t* p )
      : warlock_spell_t( "Dimensional Rift", p, p->talents.dimensional_rift )
    {
      background = dual = proc = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      dimensional_rift_pet_e rift_pet_index = p()->deck_rng.dimensional_rift_summon->draw();

      switch ( rift_pet_index )
      {
        case DR_PET_SHADOWY_TEAR:
          p()->summons.shadowy_rift->execute();
          p()->procs.dimensional_rift->occur();
          break;
        case DR_PET_UNSTABLE_TEAR:
          p()->summons.unstable_rift->execute();
          p()->procs.dimensional_rift->occur();
          break;
        case DR_PET_CHAOS_TEAR:
          p()->summons.chaos_rift->execute();
          p()->procs.dimensional_rift->occur();
          break;
        case DR_PET_OVERFIEND:
          p()->summons.overfiend->execute();
          p()->procs.avatar_of_destruction->occur();
        default:
          break;
      }
    }
  };

  struct chaos_bolt_t : public warlock_spell_t
  {
    struct chaos_bolt_state_t : public action_state_t
    {
      bool demonic_art_buffed;
      bool rancora_empowered;

      chaos_bolt_state_t( action_t* action, player_t* target )
        : action_state_t( action, target ),
        demonic_art_buffed( false ),
        rancora_empowered( false )
      { }

      void initialize() override
      {
        action_state_t::initialize();
        demonic_art_buffed = false;
        rancora_empowered = false;
      }

      std::ostringstream& debug_str( std::ostringstream& s ) override
      {
        action_state_t::debug_str( s );
        s << " demonic_art_buffed=" << demonic_art_buffed;
        s << " rancora_empowered=" << rancora_empowered;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        demonic_art_buffed = debug_cast<const chaos_bolt_state_t*>( s )->demonic_art_buffed;
        rancora_empowered = debug_cast<const chaos_bolt_state_t*>( s )->rancora_empowered;
      }
    };

    double havoc_rancora_mul_adjust;
    internal_combustion_t* internal_combustion;

    chaos_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Chaos Bolt", p, p->talents.chaos_bolt, options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;
      affected_by.chaos_incarnate = p->talents.chaos_incarnate.ok();
      affected_by.touch_of_rancora = affected_by.touch_of_rancora_casted = p->hero.touch_of_rancora.ok();

      triggers.fiendish_cruelty = p->talents.fiendish_cruelty.ok();
      triggers.dimensional_rift = p->talents.dimensional_rift.ok();
      triggers.diabolic_ritual = triggers.demonic_art = p->hero.diabolic_ritual.ok();
      triggers.rancora_cb_bonus = true;

      const double havoc_coeff = p->talents.havoc_debuff->effectN( 1 ).percent();
      const double rancora_bonus = p->hero.touch_of_rancora->effectN( 1 ).percent();
      // Observed bug: on Rancora-empowered Havoc copies, the second Touch of Rancora 20% damage bonus is added to the Havoc
      // coefficient instead of being applied multiplicatively. Since execute() already includes the empowered damage multiplier,
      // we divide out one Touch of Rancora multiplier from the coefficient adjustment below.
      const double rancora_havoc_coeff = ( havoc_coeff + rancora_bonus ) / ( 1.0 + rancora_bonus );
      havoc_rancora_mul_adjust = rancora_havoc_coeff / havoc_coeff;

      if ( p->talents.internal_combustion.ok() )
      {
        internal_combustion = new internal_combustion_t( p );
        add_child( internal_combustion );
      }
    }

    action_state_t* new_state() override
    { return new chaos_bolt_state_t( this, target ); }

    bool ready() override
    {
      if ( p()->hero.ruination.ok() && p()->executing != this && p()->buffs.ruination->check() )
        return false;

      return warlock_spell_t::ready();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( p()->talents.internal_combustion.ok() && result_is_hit( s->result ) && ( td( s->target )->dots.immolate->is_ticking() || td( s->target )->dots.wither->is_ticking() ) )
        internal_combustion->execute_on_target( s->target );

      if ( p()->talents.embers_of_nihilam_3.ok() && result_is_hit( s->result ) )
        helpers::trigger_echo_of_sargeras( p(), s->target, p()->proc_actions.echo_of_sargeras_cb, p()->procs.echo_of_sargeras_cb );
    }

    void schedule_execute( action_state_t* s ) override
    {
      // NOTE: Any of the Demonic Art buffs must be present when the cast begins for the spell to be empowered by Touch of Rancora
      // and/or for the Demonic Art buff to be consumed upon executing the spell
      if ( diabolist() && triggers.demonic_art )
      {
        action_state_t*& action_state = s ? s : pre_execute_state;
        if ( !action_state )
          action_state = get_state();

        const bool demonic_art_buff_up = p()->buffs.art_overlord->check() || p()->buffs.art_mother->check() || p()->buffs.art_pit_lord->check();
        debug_cast<chaos_bolt_state_t*>( action_state )->demonic_art_buffed = demonic_art_buff_up;
        debug_cast<chaos_bolt_state_t*>( action_state )->rancora_empowered = affected_by.touch_of_rancora && demonic_art_buff_up;
      }

      warlock_spell_t::schedule_execute( s );
    }

    void execute() override
    {
      if ( pre_execute_state )
      {
        snapshot_state( pre_execute_state, amount_type( pre_execute_state ) );
        // Casted spells do not consume any Demonic Art buff if none were active at the start of the cast
        triggers.demonic_art_buff = debug_cast<chaos_bolt_state_t*>( pre_execute_state )->demonic_art_buffed;
      }
      else
      {
        triggers.demonic_art_buff = false;
      }

      if ( p()->hero.diabolic_oculi.ok() )
        p()->buffs.demonic_oculi->trigger();

      // NOTE: 2026-03-17 Rancora-empowered Havoc copies behave as if +0.20 were added to the Havoc coefficient before only one 1.20x bonus, instead
      // of applying the second 1.20x multiplicatively to the final damage (bug). To emulate that behavior, temporarily rescale 'base_aoe_multiplier'
      // from: 0.60 -> 0.6667 and 0.70 -> 0.75; so that execute() produces final damage multipliers of 0.96 / 1.08 on Havoc copies.
      const double prev_base_aoe_multiplier = base_aoe_multiplier;
      const bool rancora_empowered = pre_execute_state && debug_cast<chaos_bolt_state_t*>( pre_execute_state )->rancora_empowered;
      if ( p()->bugs && diabolist() && affected_by.touch_of_rancora && affected_by.havoc && rancora_empowered )
        base_aoe_multiplier *= havoc_rancora_mul_adjust;

      warlock_spell_t::execute();

      base_aoe_multiplier = prev_base_aoe_multiplier; // Restore original previous havoc aoe multiplier

      p()->buffs.backdraft->decrement();

      p()->buffs.crashing_chaos->decrement();

      if ( p()->talents.chaotic_inferno.ok() && p()->prd_rng.chaotic_inferno->trigger() )
      {
        // Delay the buff a bit to simulate the ingame behavior where an Incinerate cast
        // queued right after a Chaos Bolt that procs Chaotic Inferno is not affected by it
        make_event( *sim, 10_ms, [ this ] {
          p()->buffs.chaotic_inferno->trigger();
          p()->procs.chaotic_inferno->occur();
        } );
      }
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_persistent_multiplier( s );

      // An incoming rancora empowered casted spell will remain empowered even if the Demonic Art buff falls off during cast
      // NOTE: Chaos Bolt is double dipping the dmg amp bonus (from Demonic Art buff effect #1 and the custom rancora_empowered state) (bug)
      if ( debug_cast<const chaos_bolt_state_t*>( s )->rancora_empowered && p()->bugs )
        m *= 1.0 + p()->hero.touch_of_rancora->effectN( 1 ).percent();

      return m;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      m *= 1.0 + player->cache.spell_crit_chance();

      // The base effect of Through the Felvine is automatically applied by the parse_effects system
      // However, it is necessary to manually apply its duplicate effect during Malevolence
      if ( p()->hero.through_the_felvine.ok() && p()->hero.malevolence.ok() && p()->buffs.malevolence->check() )
      {
        double felvine_mul = p()->hero.through_the_felvine->effectN( 4 ).percent();
        m *= 1.0 + felvine_mul / ( 1.0 + felvine_mul );
      }

      return m;
    }

    double composite_crit_chance() const override
    { return 1.0; }
  };

  struct conflagrate_t : public warlock_spell_t
  {
    warlock_spell_t* spread_dot;
    double spread_range = 8.0; // TODO: Check if this is indeed the range, and also check where this value would be in the spell data

    conflagrate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Conflagrate", p, p->talents.conflagrate, options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      triggers.fiendish_cruelty = p->talents.fiendish_cruelty.ok();

      energize_type = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount = ( p->talents.conflagrate_2->effectN( 1 ).base_value() ) / 10.0;

      cooldown->hasted = true;

      if ( p->talents.roaring_blaze.ok() )
      {
        if ( p->hero.wither.ok() )
          spread_dot = new wither_t( p, false, "" );
        else
          spread_dot = new immolate_t( p, false, "" );

        spread_dot->background = true;
        spread_dot->dual = true;
        spread_dot->base_costs[ RESOURCE_MANA ] = 0;
        spread_dot->base_dd_multiplier = 0.0;
      }
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      // Roaring Blaze doesn't apply to havoc targets
      if ( p()->talents.roaring_blaze.ok() && ( s->chain_target == 0 ) && result_is_hit( s->result ) )
      {
        if ( td( s->target )->dots.immolate->is_ticking() || td( s->target )->dots.wither->is_ticking() )
        {
          int n_spread = as<int>( p()->talents.roaring_blaze->effectN( 2 ).base_value() );
          const auto& tl = target_list();
          // NOTE: 2026-02-17 It appears there is a bug ingame where the dot spreads to an additional target, but only if that target already has the dot
          if ( p()->bugs )
          {
            auto spread_targets = p()->get_smart_targets( tl, &warlock_td_t::dots_t::immolate, n_spread + 1, s->target, spread_range, false );
            int c = 0;
            for ( auto t : spread_targets )
            {
              if ( c < n_spread || ( td( t )->dots.immolate->is_ticking() || td( t )->dots.wither->is_ticking() ) )
                spread_dot->execute_on_target( t );

              c++;
            }
          }
          else
          {
            auto spread_targets = p()->get_smart_targets( tl, &warlock_td_t::dots_t::immolate, n_spread, s->target, spread_range, false );
            for ( auto t : spread_targets )
              spread_dot->execute_on_target( t );
          }
        }
      }
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.conflagration_of_chaos->expire();

      if ( p()->talents.conflagration_of_chaos.ok() )
      {
        bool success = p()->buffs.conflagration_of_chaos->trigger();

        if ( success )
          p()->procs.conflagration_of_chaos->occur();
      }

      if ( p()->talents.backdraft.ok() )
        p()->buffs.backdraft->trigger();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( p()->buffs.conflagration_of_chaos->check() )
        m *= 1.0 + player->cache.spell_crit_chance();

      return m;
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
      }

      double composite_persistent_multiplier( const action_state_t* s ) const override
      {
        double m = warlock_spell_t::composite_persistent_multiplier( s );

        // The base effect of Through the Felvine is automatically applied by the parse_effects system
        // However, it is necessary to manually apply its duplicate effect during Malevolence
        if ( p()->hero.through_the_felvine.ok() && p()->hero.malevolence.ok() && p()->buffs.malevolence->check() )
        {
          double felvine_mul = p()->hero.through_the_felvine->effectN( 5 ).percent();
          m *= 1.0 + felvine_mul / ( 1.0 + felvine_mul );
        }

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
      aoe = -1;

      affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();

      triggers.diabolic_ritual = triggers.demonic_art = triggers.demonic_art_buff = p->hero.diabolic_ritual.ok();

      if ( !p->proc_actions.rain_of_fire_tick )
      {
        p->proc_actions.rain_of_fire_tick = new rain_of_fire_tick_t( p );
        p->proc_actions.rain_of_fire_tick->stats = stats;
      }
    }

    void execute() override
    {
      if ( p()->hero.diabolic_oculi.ok() )
      {
        // NOTE: 2026-03-17 Demonic Oculi stack buff is obtained after the explosion for RoF (bug?)
        if ( p()->bugs )
          make_event( *sim, 1_ms, [ this ] { p()->buffs.demonic_oculi->trigger(); } );
        else
          p()->buffs.demonic_oculi->trigger();
      }

      warlock_spell_t::execute();

      // Rain of Fire has no expiration pulse (none, neither partial nor full) (NO_EXPIRATION_PULSE is already the default for ground_aoe_params_t)
      make_event<ground_aoe_event_t>( *sim, p(),
                                      ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time * player->cache.spell_haste() * ( 1.0 + p()->talents.destructive_rapidity->effectN( 1 ).percent() ) )
                                        .duration( p()->talents.rain_of_fire->duration() * player->cache.spell_haste() )
                                        .start_time( sim->current_time() )
                                        .action( p()->proc_actions.rain_of_fire_tick ) );

      // Rain of Fire spell cast triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );

      if ( p()->talents.embers_of_nihilam_3.ok() )
        helpers::trigger_echo_of_sargeras( p(), execute_state->target, p()->proc_actions.echo_of_sargeras_rof, p()->procs.echo_of_sargeras_rof );

      p()->buffs.crashing_chaos->decrement();

      if ( p()->talents.alythesss_ire.ok() )
      {
        p()->buffs.alythesss_ire->decrement();

        if ( p()->cycle_proc.alythesss_ire->trigger() )
        {
          p()->buffs.alythesss_ire->trigger();
          p()->procs.alythesss_ire->occur();
        }
      }
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

      td( s->target )->debuffs.havoc->trigger();
    }
  };

  struct lake_of_fire_t : public warlock_spell_t
  {
    timespan_t pulse_time;

    struct lake_of_fire_tick_t : public warlock_spell_t
    {
      timespan_t pulse_time;
      timespan_t end_time;

      lake_of_fire_tick_t( warlock_t* p )
        : warlock_spell_t( "Lake of Fire (tick)", p, p->talents.lake_of_fire_tick )
      {
        background = dual = true;
        aoe = -1;
        radius = p->talents.cataclysm->effectN( 1 ).radius();

        // TODO: Lake of Fire is doing double the expected damage, but we can't find where that multiplier effect is in the spell data (bug?)
        if ( p->bugs )
          base_dd_multiplier *= 2.0;

        affected_by.chaotic_energies = true;
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        const bool is_last_tick = ( ( sim->current_time() + pulse_time ) > end_time );
        if ( !is_last_tick )
          td( s->target )->debuffs.lake_of_fire->trigger( pulse_time + 1_ms );
      }
    };

    lake_of_fire_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Lake of Fire", p, p->talents.lake_of_fire_aoe, options_str )
    {
      background = dual = true;
      may_miss = may_crit = false;
      base_tick_time = 1_s;
      dot_duration = 0_s;
      aoe = -1;

      if ( !p->proc_actions.lake_of_fire_tick )
      {
        p->proc_actions.lake_of_fire_tick = new lake_of_fire_tick_t( p );
        p->proc_actions.lake_of_fire_tick->stats = stats;
      }
    }

    void execute() override
    {
      pulse_time = base_tick_time * player->cache.spell_haste();
      const timespan_t duration = p()->talents.lake_of_fire_aoe->duration() * player->cache.spell_haste();
      const timespan_t start_time = sim->current_time();

      // No need to use a custom action_state thanks to Cataclysm cooldown
      debug_cast<lake_of_fire_tick_t*>( p()->proc_actions.lake_of_fire_tick )->pulse_time = pulse_time;
      debug_cast<lake_of_fire_tick_t*>( p()->proc_actions.lake_of_fire_tick )->end_time = start_time + duration;

      warlock_spell_t::execute();

      make_event<ground_aoe_event_t>( *sim, p(),
                                      ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( pulse_time )
                                        .duration( duration )
                                        .start_time( start_time )
                                        .action( p()->proc_actions.lake_of_fire_tick ) );
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      td( s->target )->debuffs.lake_of_fire->trigger( pulse_time + 1_ms );
    }
  };

  struct cataclysm_t : public warlock_spell_t
  {
    warlock_spell_t* applied_dot;
    lake_of_fire_t* lake_of_fire;

    cataclysm_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Cataclysm", p, p->talents.cataclysm, options_str )
    {
      aoe = -1;

      affected_by.chaotic_energies = true;

      if ( p->hero.wither.ok() )
        applied_dot = new wither_t( p, false, "" );
      else
        applied_dot = new immolate_t( p, false, "" );

      applied_dot->background = true;
      applied_dot->dual = true;
      applied_dot->base_costs[ RESOURCE_MANA ] = 0;
      applied_dot->base_dd_multiplier = 0.0;

      if ( p->talents.lake_of_fire.ok() )
      {
        lake_of_fire = new lake_of_fire_t( p, "" );
        add_child( lake_of_fire );
      }
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->talents.lake_of_fire.ok() )
      {
        lake_of_fire->set_target( target );
        lake_of_fire->execute();
      }
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
    double havoc_rancora_mul_adjust;

    shadowburn_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Shadowburn", p, p->talents.shadowburn, options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;
      affected_by.chaos_incarnate = p->talents.chaos_incarnate.ok();
      affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();

      triggers.dimensional_rift = p->talents.dimensional_rift.ok();
      triggers.diabolic_ritual = triggers.demonic_art = triggers.demonic_art_buff = p->hero.diabolic_ritual.ok();

      const double havoc_coeff = p->talents.havoc_debuff->effectN( 1 ).percent();
      const double rancora_bonus = p->hero.touch_of_rancora->effectN( 1 ).percent();
      // Observed bug: on Rancora-empowered Havoc copies, the second Touch of Rancora 20% damage bonus is added to the Havoc
      // coefficient instead of being applied multiplicatively. Since execute() already includes the empowered damage multiplier,
      // we divide out one Touch of Rancora multiplier from the coefficient adjustment below.
      const double rancora_havoc_coeff = ( havoc_coeff + rancora_bonus ) / ( 1.0 + rancora_bonus );
      havoc_rancora_mul_adjust = rancora_havoc_coeff / havoc_coeff;
    }

    bool ready() override
    {
      if ( ( target->health_percentage() > p()->talents.shadowburn->effectN( 4 ).base_value() ) && !p()->buffs.fiendish_cruelty->check() )
        return false;

      return warlock_spell_t::ready();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        td( s->target )->debuffs.shadowburn->trigger();

        if ( p()->talents.embers_of_nihilam_3.ok() )
          helpers::trigger_echo_of_sargeras( p(), s->target, p()->proc_actions.echo_of_sargeras_sb, p()->procs.echo_of_sargeras_sb );
      }
    }

    void execute() override
    {
      if ( p()->hero.diabolic_oculi.ok() )
        p()->buffs.demonic_oculi->trigger();

      // NOTE: 2026-03-17 Rancora-empowered Havoc copies behave as if +0.20 were added to the Havoc coefficient before only one 1.20x bonus, instead
      // of applying the second 1.20x multiplicatively to the final damage (bug). To emulate that behavior, temporarily rescale 'base_aoe_multiplier'
      // from: 0.60 -> 0.6667 and 0.70 -> 0.75; so that execute() produces final damage multipliers of 0.96 / 1.08 on Havoc copies.
      const double prev_base_aoe_multiplier = base_aoe_multiplier;
      const bool rancora_empowered = p()->buffs.art_overlord->check() || p()->buffs.art_mother->check() || p()->buffs.art_pit_lord->check();
      if ( p()->bugs && diabolist() && affected_by.touch_of_rancora && affected_by.havoc && rancora_empowered )
        base_aoe_multiplier *= havoc_rancora_mul_adjust;

      warlock_spell_t::execute();

      base_aoe_multiplier = prev_base_aoe_multiplier; // Restore original previous havoc aoe multiplier

      p()->buffs.conflagration_of_chaos->expire();

      if ( p()->talents.conflagration_of_chaos.ok() )
      {
        bool success = p()->buffs.conflagration_of_chaos->trigger();

        if ( success )
          p()->procs.conflagration_of_chaos->occur();
      }

      p()->buffs.fiendish_cruelty->decrement();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( p()->buffs.conflagration_of_chaos->check() )
        m *= 1.0 + player->cache.spell_crit_chance();

      return m;
    }
  };

  struct channel_demonfire_tick_t : public warlock_spell_t
  {
    bool demonfire_infusion;

    channel_demonfire_tick_t( warlock_t* p )
      : warlock_spell_t( "Channel Demonfire (tick)", p, p->talents.channel_demonfire_tick )
    {
      background = dual = true;
      may_miss = false;
      aoe = -1;
      travel_speed = p->talents.channel_demonfire_travel->missile_speed();

      demonfire_infusion = false;

      affected_by.chaotic_energies = true;

      spell_power_mod.direct = p->talents.channel_demonfire_tick->effectN( 1 ).sp_coeff();
    }

    channel_demonfire_tick_t( warlock_t* p, bool dfi )
      : channel_demonfire_tick_t( p )
    { demonfire_infusion = dfi; }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      warlock_td_t* tdata = td( s->target );
      const timespan_t extra_time = p()->talents.raging_demonfire->effectN( 2 ).time_value();

      if ( p()->talents.raging_demonfire.ok() && tdata->dots.immolate->is_ticking() )
        tdata->dots.immolate->adjust_duration( extra_time );

      if ( p()->talents.raging_demonfire.ok() && tdata->dots.wither->is_ticking() )
      {
        tdata->dots.wither->adjust_duration( extra_time );
        tdata->debuffs.wither->extend_duration( extra_time );
        assert( tdata->dots.wither->current_stack() == tdata->debuffs.wither->check() && tdata->dots.wither->remains() == tdata->debuffs.wither->remains() );
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( s->chain_target != 0 )
        m *= p()->talents.channel_demonfire_tick->effectN( 2 ).sp_coeff() / p()->talents.channel_demonfire_tick->effectN( 1 ).sp_coeff();

      if ( demonfire_infusion )
        m *= 1.0 + p()->talents.demonfire_infusion->effectN( 3 ).percent();

      return m;
    }
  };

  struct demonfire_infusion_t : public warlock_spell_t
  {
    channel_demonfire_tick_t* demonfire_tick;

    demonfire_infusion_t( warlock_t* p )
      : warlock_spell_t( "Demonfire Infusion", p, p->talents.demonfire_infusion ),
      demonfire_tick( new channel_demonfire_tick_t( p, true ) )
    {
      background = true;

      if ( p->talents.demonfire_infusion.ok() && !p->talents.channel_demonfire.ok() )
        add_child( demonfire_tick );
    }

    void execute() override
    {
      warlock_spell_t::execute();

      auto t = execute_state->target;

      demonfire_tick->execute_on_target( t );

      if ( p()->talents.raging_demonfire.ok() )
      {
        int extra_bolts = as<int>( p()->talents.raging_demonfire->effectN( 1 ).base_value() );
        for ( int i = 0; i < extra_bolts; i++ )
        {
          demonfire_tick->execute_on_target( t );
        }
      }
    }
  };

  struct channel_demonfire_t : public warlock_spell_t
  {
    channel_demonfire_tick_t* channel_demonfire_tick;

    channel_demonfire_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Channel Demonfire", p, p->talents.channel_demonfire, options_str ),
      channel_demonfire_tick( new channel_demonfire_tick_t( p ) )
    {
      channeled = true;
      hasted_ticks = true;
      may_crit = false;
      cooldown->hasted = true;

      target_filter_callback = immolate_or_wither_only();

      if ( !p->talents.demonfire_infusion.ok() || p->talents.channel_demonfire.ok() )
        add_child( channel_demonfire_tick );

      if ( p->talents.channel_demonfire.ok() && p->talents.raging_demonfire.ok() )
      {
        int num_ticks = ( int )( dot_duration / base_tick_time );
        dot_duration = num_ticks * base_tick_time;
      }
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
      if ( !warlock_spell_t::ready() )
        return false;

      target_cache.is_valid = false;
      return !target_list().empty();
    }
  };

  struct summon_main_infernal_pet_t : public warlock_spell_t
  {
    summon_main_infernal_pet_t( warlock_t* p )
      : warlock_spell_t( "Summon Infernal (Summon) (Main)", p, p->talents.summon_infernal_main )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      // Random extra duration time between 0_ms and 820_ms following a uniform distribution
      const timespan_t dur_adjust = timespan_t::from_millis( rng().range( 0.0, 820.0 ) );
      p()->warlock_pet_list.infernals.spawn( data().duration() + dur_adjust );
    }
  };

  struct summon_roc_infernal_pet_t : public warlock_spell_t
  {
    summon_roc_infernal_pet_t( warlock_t* p )
      : warlock_spell_t( "Summon Infernal (Summon) (Roc)", p, p->talents.summon_infernal_roc )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      // Random extra duration time between 0_ms and 820_ms following a uniform distribution
      const timespan_t dur_adjust = timespan_t::from_millis( rng().range( 0.0, 820.0 ) );
      auto spawned = p()->warlock_pet_list.rocs.spawn( data().duration() + dur_adjust );
      for ( pets::destruction::infernal_t* s : spawned )
        s->type = pets::destruction::infernal_t::infernal_type_e::RAIN;
    }
  };

  struct summon_fragment_infernal_pet_t : public warlock_spell_t
  {
    summon_fragment_infernal_pet_t( warlock_t* p )
      : warlock_spell_t( "Infernal Fragmentation (Summon) (Fragment)", p, p->hero.infernal_fragmentation )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      // Random extra duration time between 0_ms and 820_ms following a uniform distribution
      const timespan_t dur_adjust = timespan_t::from_millis( rng().range( 0.0, 820.0 ) );
      p()->warlock_pet_list.fragments.spawn( data().duration() + dur_adjust );
    }
  };

  struct infernal_awakening_t : public warlock_spell_t
  {
    infernal_awakening_t( warlock_t* p )
      : warlock_spell_t( "Infernal Awakening", p, p->talents.infernal_awakening )
    {
      background = dual = true;
      aoe = -1;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->summons.infernal->execute();
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

        p()->buffs.ritual_overlord->extend_duration( reduction );
        p()->buffs.ritual_mother->extend_duration( reduction );
        p()->buffs.ritual_pit_lord->extend_duration( reduction );
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

    void execute() override
    {
      warlock_spell_t::execute();

      applied_dot->execute_on_target( target );

      p()->buffs.backdraft->decrement();

      if ( p()->talents.avatar_of_destruction.ok() )
      {
        p()->summons.overfiend->execute();
        p()->procs.avatar_of_destruction->occur();
      }
    }
  };

  struct echo_of_sargeras_t : public warlock_spell_t
  {
    echo_of_sargeras_t( warlock_t* p, util::string_view name = "echo_of_sargeras", double effectiveness = 1.0 )
      : warlock_spell_t( name, p, p->talents.echo_of_sargeras )
    {
      background = dual = true;
      aoe = -1;
      reduced_aoe_targets = as<int>( p->talents.echo_of_sargeras->effectN( 3 ).base_value() );

      affected_by.chaotic_energies = true;

      spell_power_mod.direct = p->talents.echo_of_sargeras->effectN( 1 ).sp_coeff();

      base_dd_multiplier *= effectiveness;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      if ( s->chain_target != 0 )
        m *= p()->talents.echo_of_sargeras->effectN( 2 ).sp_coeff() / p()->talents.echo_of_sargeras->effectN( 1 ).sp_coeff();

      return m;
    }
  };

  struct embers_of_nihilam_t : public warlock_spell_t
  {
    embers_of_nihilam_t( warlock_t* p )
      : warlock_spell_t( "Embers of Nihilam", p, p->talents.embers_of_nihilam_1.ok() ? p->talents.embers_of_nihilam_1
                                                 : ( p->talents.embers_of_nihilam_3.ok() ? p->talents.embers_of_nihilam_3 : spell_data_t::not_found() ) )
    {
      background = dual = true;

      if ( p->talents.embers_of_nihilam_1.ok() )
        add_child( p->proc_actions.echo_of_sargeras );

      if ( p->talents.embers_of_nihilam_3.ok() )
      {
        add_child( p->proc_actions.echo_of_sargeras_cb );
        add_child( p->proc_actions.echo_of_sargeras_sb );
        add_child( p->proc_actions.echo_of_sargeras_rof );
      }
    }
  };

  // Destruction Actions End
  // Diabolist Actions Begin

  struct infernal_bolt_t : public warlock_spell_t
  {
    const double havoc_mod_value = 1.0;

    infernal_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Infernal Bolt", p, p->hero.infernal_bolt, options_str )
    {
      energize_type = action_energize::ON_CAST;

      affected_by.havoc = true;

      triggers.fiendish_cruelty = p->talents.fiendish_cruelty.ok(); // Infernal Bolt crits can trigger Fiendish Cruelty
      triggers.embers_of_nihilam_1 = p->talents.embers_of_nihilam_1.ok(); // Infernal Bolt hits can trigger Embers of Nihilam 1
    }

    void init() override
    {
      spell_t::init();

      if ( destruction() && affected_by.havoc )
      {
        // NOTE: 2026-02-18 Infernal Bolt Havoc deals 100% of the original damage to havoc target (bug?)
        base_aoe_multiplier *= p()->bugs ? havoc_mod_value : ( p()->talents.havoc_debuff->effectN( 1 ).percent() );
        p()->havoc_spells.push_back( this );
      }
    }

    bool ready() override
    {
      if ( !p()->buffs.infernal_bolt->check() && p()->executing != this )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      // 2026-02-18 Infernal Bolt can proc Demonfire Infusion
      if ( p()->talents.demonfire_infusion.ok() && p()->flat_rng.demonfire_infusion_inc->trigger() )
      {
        p()->proc_actions.demonfire_infusion->execute_on_target( target );
        p()->procs.demonfire_infusion_inc->occur();
      }

      // Infernal Bolt energize spell effect triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );

      p()->buffs.infernal_bolt->decrement();

      p()->buffs.backdraft->decrement();

      // Chaotic Inferno buff is only consumed by an Infernal Bolt cast that benefits from the effect
      if ( time_to_execute == 0_ms )
        p()->buffs.chaotic_inferno->decrement();
    }
  };

  struct ruination_t : public warlock_spell_t
  {
    struct ruination_impact_t : public warlock_spell_t
    {
      hand_of_guldan_t::hog_impact_t* hog_impact_spell = nullptr;

      ruination_impact_t( warlock_t* p )
        : warlock_spell_t( "Ruination (Impact)", p, p->hero.ruination_impact )
      {
        background = dual = true;
        aoe = -1;
        reduced_aoe_targets = p->hero.ruination_cast->effectN( 2 ).base_value();

        affected_by.chaotic_energies = true;

        if ( demonology() )
        {
          hog_impact_spell = new hand_of_guldan_t::hog_impact_t( p );
          hog_impact_spell->state.shards_used = as<int>( p->hero.ruination_buff->effectN( 2 ).base_value() );
          hog_impact_spell->state.rancora_empowered = false; // Ruination HoG impact is never rancora empowered
        }
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( result_is_hit( s->result ) && s->target == target )
        {
          if ( demonology() )
          {
            // Ruination appears to trigger a HoG-like meteor after a certain delay, which summons the Wild Imps.
            // This delay can be modeled fairly closely using a normal distribution.
            make_event( *sim, rng().gauss<395,35>(), [ this, t = target ] {
              hog_impact_spell->execute_on_target( t );
            } );
          }

          if ( destruction() )
          {
            for ( int i = 0; i < as<int>( p()->hero.ruination_buff->effectN( 3 ).base_value() ); i++ )
              p()->summons.diabolic_imp->execute();
          }
        }
      }
    };

    ruination_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Ruination", p, p->hero.ruination_cast, options_str )
    {
      // Ruination triggers Demonic Art, but not Diabolic Ritual.
      // On top of that, it does so even if the cast starts before gaining the Demonic Art buff.
      triggers.demonic_art = triggers.demonic_art_buff = p->hero.diabolic_ritual.ok();

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

      // Ruination triggers Dominion of Argus
      if ( p()->talents.dominion_of_argus_1.ok() && p()->buffs.dominion_of_argus->check() )
        p()->buffs.dominion_of_argus->trigger();

      // Ruination "refunds" one shard even though it doesn't actually consume any
      if ( p()->talents.dominion_of_argus_3.ok() && p()->buffs.dominion_of_argus->check() )
        p()->resource_gain( RESOURCE_SOUL_SHARD, p()->talents.dominion_of_argus_3_gain->effectN( 1 ).resource(),
                            p()->gains.dominion_of_argus );
    }
  };

  struct eye_explosion_t : public warlock_spell_t
  {
    eye_explosion_t( warlock_t* p )
      : warlock_spell_t( "Eye Explosion", p, p->hero.eye_explosion )
    {
      // Affected by Chaotis Energies (Destruction Mastery)
      affected_by.chaotic_energies = destruction();

      background = true;
      aoe = -1;
      reduced_aoe_targets = as<int>( p->hero.diabolic_oculi->effectN( 1 ).base_value() );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_da_multiplier( s );

      m *= p()->buffs.demonic_oculi->check();

      return m;
    }

    void execute() override
    {
      // In-game happens just before the damage
      if ( p()->hero.minds_eyes.ok() )
        p()->buffs.minds_eyes->trigger( p()->buffs.demonic_oculi->check() );

      warlock_spell_t::execute();

      p()->buffs.demonic_oculi->expire();
    }
  };

  struct diabolic_gaze_1_t : public warlock_spell_t
  {
    diabolic_gaze_1_t( warlock_t* p )
      : warlock_spell_t( "Diabolic Gaze (1)", p, p->hero.diabolic_gaze_dmg_1 )
    {
      // Not affected by Chaotis Energies (Destruction Mastery)
      // Excluded (not whitelisted) from many warlock talents/spells (bug?)

      background = true;
    }
  };

  struct diabolic_gaze_2_t : public warlock_spell_t
  {
    diabolic_gaze_2_t( warlock_t* p )
      : warlock_spell_t( "Diabolic Gaze (2)", p, p->hero.diabolic_gaze_dmg_2 )
    {
      // Not affected by Chaotis Energies (Destruction Mastery)

      background = true;
    }
  };

  struct diabolic_gaze_3_t : public warlock_spell_t
  {
    diabolic_gaze_3_t( warlock_t* p )
      : warlock_spell_t( "Diabolic Gaze (3)", p, p->hero.diabolic_gaze_dmg_3 )
    {
      // Not affected by Chaotis Energies (Destruction Mastery)
      // Excluded (not whitelisted) from many warlock talents/spells (bug?)

      background = true;
    }
  };

  struct diabolic_oculi_t : public warlock_spell_t
  {
    diabolic_oculi_t( warlock_t* p )
      : warlock_spell_t( "Diabolic Oculi", p, p->hero.diabolic_oculi )
    {
      background = dual = true;

      add_child( p->proc_actions.eye_explosion );
      add_child( p->proc_actions.diabolic_gaze_3 );
      add_child( p->proc_actions.diabolic_gaze_2 );
      add_child( p->proc_actions.diabolic_gaze_1 );
    }
  };

  struct summon_overlord_t : public warlock_spell_t
  {
    summon_overlord_t( warlock_t* p )
      : warlock_spell_t( "Summon Overlord", p, p->hero.summon_overlord )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.overlords.spawn( data().duration() );
    }
  };

  struct summon_mother_of_chaos_t : public warlock_spell_t
  {
    summon_mother_of_chaos_t( warlock_t* p )
      : warlock_spell_t( "Summon Mother of Chaos", p, p->hero.summon_mother )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.mothers.spawn( data().duration() );

      if ( p()->hero.secrets_of_the_coven.ok() )
        p()->buffs.infernal_bolt->trigger();
    }
  };

  struct summon_pit_lord_t : public warlock_spell_t
  {
    summon_pit_lord_t( warlock_t* p )
      : warlock_spell_t( "Summon Pit Lord", p, p->hero.summon_pit_lord )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.pit_lords.spawn( data().duration() );

      if ( p()->hero.ruination.ok() )
        p()->buffs.ruination->trigger();
    }
  };

  struct summon_diabolic_imp_t : public warlock_spell_t
  {
    summon_diabolic_imp_t( warlock_t* p )
      : warlock_spell_t( "Diabolic Imp (Summon)", p, p->hero.diabolic_imp )
    {
      harmful = may_crit = false;
      background = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->warlock_pet_list.diabolic_imps.spawn( data().duration() );

      // Diabolic Imp summon spell triggers procs
      p()->trigger_aura_applied_callbacks( proc_data, p() );
    }
  };

  // Diabolist Actions End
  // Helper Functions Begin

  void helpers::trigger_blackened_soul( warlock_t* p, bool malevolence )
  {
    if ( !malevolence && p->cooldowns.blackened_soul->down() )
      return;

    bool stack_gained = false;

    for ( const auto target : p->sim->target_non_sleeping_list )
    {
      warlock_td_t* tdata = p->get_target_data( target );
      if ( !tdata )
        continue;

      if ( !tdata->dots.wither->is_ticking() )
        continue;

      int stacks = 1;

      if( malevolence )
      {
        stacks = as<int>( p->hero.malevolence->effectN( 1 ).base_value() );
      }

      // Wither extra stack from Malevolence Effect #2 does not benefit from Bleakheart Tactics
      if ( p->buffs.malevolence->check() && !malevolence )
      {
        stacks += as<int>( p->hero.malevolence->effectN( 2 ).base_value() );
      }

      // Bleakheart Tactics proc uses a global BLP (PRD-accumulator)
      // Malevolence stack gains do not benefit from Bleakheart Tactics
      if ( p->hero.bleakheart_tactics.ok() && !malevolence && p->prd_rng.bleakheart_tactics->trigger() )
      {
        stacks += as<int>( p->hero.bleakheart_tactics->effectN( 3 ).base_value() );
        p->procs.bleakheart_tactics->occur();
      }

      assert( stacks >= 1 );
      tdata->dots.wither->increment( stacks );
      tdata->debuffs.wither->bump( stacks );
      assert( tdata->dots.wither->current_stack() == tdata->debuffs.wither->check() && tdata->dots.wither->remains() == tdata->debuffs.wither->remains() );
      stack_gained = true;

      const int prev_collapse_stacks = tdata->debuffs.blackened_soul->check();
      assert( prev_collapse_stacks >= 0 );
      bool collapse = false; // Malevolence no longer initiates collapse automatically. Last tested 2026-03-17
      collapse = collapse || ( p->hero.seeds_of_their_demise.ok() && tdata->dots.wither->current_stack() > 1 && target->health_percentage() <= p->hero.seeds_of_their_demise->effectN( 2 ).base_value() );
      collapse = collapse || ( p->hero.seeds_of_their_demise.ok() && tdata->dots.wither->current_stack() >= as<int>( p->hero.seeds_of_their_demise->effectN( 1 ).base_value() ) );

      if ( collapse )
      {
        const int diff_stacks = tdata->dots.wither->current_stack() - prev_collapse_stacks;

        assert( tdata->dots.wither->current_stack() >= 1 );
        if ( diff_stacks > 0 )
          tdata->debuffs.blackened_soul->trigger( diff_stacks );
        else if ( diff_stacks < 0 )
          tdata->debuffs.blackened_soul->decrement( -diff_stacks );

        assert( tdata->debuffs.blackened_soul->check() );
        if ( !prev_collapse_stacks )
        {
          p->sim->print_debug( "{} wither stack collapse in {} started (seeds of their demise) (stack gain check). wither_current_stack={}, wither_target_health_percentage={:.2f}%",
                      p->name(), target->name(), tdata->dots.wither->current_stack(), target->health_percentage() );
        }
      }
      else if ( !prev_collapse_stacks && !malevolence && p->flat_rng.blackened_soul->trigger() ) // Malevolence stack gains do not trigger Blackened Soul collapse proc
      {
        const int new_collapse_stacks = tdata->dots.wither->current_stack();
        assert( new_collapse_stacks >= 1 && !tdata->debuffs.blackened_soul->check() );
        tdata->debuffs.blackened_soul->trigger( new_collapse_stacks );
        p->procs.blackened_soul->occur();
        p->sim->print_debug( "{} wither stack collapse in {} started (blackened soul proc). wither_current_stack={}", p->name(), target->name(), tdata->dots.wither->current_stack() );
      }

      if ( malevolence )
        p->proc_actions.malevolence->execute_on_target( target );
    }

    if ( stack_gained )
      p->cooldowns.blackened_soul->start();
  }

  void helpers::trigger_echo_of_sargeras( warlock_t* p, player_t* target, action_t* echo_action, proc_t* proc )
  {
    if ( p->cooldowns.echo_of_sargeras->down() )
      return;

    // NOTE: 2026-03-17 RoF does not proc Embers of Sargeras out of combat (bug?)
    if ( p->bugs && !p->in_combat && proc == p->procs.echo_of_sargeras_rof )
      return;

    // If no valid target provided, find a random target with Immolate or Wither ticking
    if ( !target || target->is_sleeping() )
    {

      std::vector<player_t*> candidates;

      for ( const auto t : p->sim->target_non_sleeping_list )
      {
        warlock_td_t* tdata = p->get_target_data( t );
        if ( !tdata )
          continue;

        if ( tdata->dots.immolate->is_ticking() || tdata->dots.wither->is_ticking() )
          candidates.push_back( t );
      }

      if ( candidates.empty() )
        return;

      target = p->rng().range( candidates );
    }

    echo_action->execute_on_target( target );
    proc->occur();
    p->buffs.vision_of_nihilam->trigger();
    p->cooldowns.echo_of_sargeras->start();
  }

  void helpers::trigger_wrath_of_nathreza( warlock_t* p, player_t* target )
  {
    if ( !p->talents.shadow_of_nathreza_3.ok() )
      return;

    // TOCHECK: Spell data suggests ~2 RPPM (hasted) - verify in-game
    if ( !p->rppm_rng.wrath_of_nathreza->trigger() )
      return;

    p->summons.desperate_soul->execute_on_target( target );
    p->procs.wrath_of_nathreza->occur();
  }

  // Event for spawning Wild Imps for Demonology
  imp_delay_event_t::imp_delay_event_t( warlock_t* p, double delay, double exp, int _index ) : player_event_t( *p, timespan_t::from_millis( delay ) )
  {
    diff = timespan_t::from_millis( exp - delay );
    index = _index;
  }

  const char* imp_delay_event_t::name() const
  { return "imp_delay"; }

  void imp_delay_event_t::execute()
  {
    warlock_t* p = static_cast<warlock_t*>( player() );

    auto imps = debug_cast<summon_wild_imp_t*>( p->summons.wild_imp )->execute_spawn();

    if ( p->talents.imp_gang_boss.ok() && index == 0 )
    {
      for ( auto imp : imps )
      {
        imp->buffs.imp_gang_boss->trigger();
      }
    }

    // Remove this event from the vector
    auto it = std::find( p->wild_imp_spawns.begin(), p->wild_imp_spawns.end(), this );
    if ( it != p->wild_imp_spawns.end() )
      p->wild_imp_spawns.erase( it );
  }

  // Used for APL expressions to estimate when imp is "supposed" to spawn
  timespan_t imp_delay_event_t::expected_time()
  { return std::max( 0_ms, this->remains() + diff ); }

  // Event to handle UA stacks decreases
  ua_stack_drop_event_t::ua_stack_drop_event_t( warlock_t* p, dot_t* _dot , timespan_t event_time )
    : player_event_t( *p, event_time ),
    dot( _dot )
  { }

  const char* ua_stack_drop_event_t::name() const
  { return "ua_stack_drop"; }

  void ua_stack_drop_event_t::execute()
  {
    warlock_t* p = static_cast<warlock_t*>( player() );

    if ( dot->is_ticking() && dot->tick_event && dot->current_action && dot->remains() > 0_ms )
    {
      player_t* target = dot->target;

      dot->decrement( 1 );
      assert( ( dot->is_ticking() && dot->current_stack() > 0 ) && "UA stack decrement event should not cancel the DoT" );

      if ( p->talents.fatal_echoes.ok() && !target->is_sleeping() && p->prd_rng.fatal_echoes->trigger() )
      {
        p->procs.fatal_echoes->occur();
        dot->current_action->set_target( target );
        dot->current_action->time_to_execute = 0_ms;
        debug_cast<unstable_affliction_t*>( dot->current_action )->is_fatal_echoes_execute = true;
        dot->current_action->execute();
        debug_cast<unstable_affliction_t*>( dot->current_action )->is_fatal_echoes_execute = false;
      }
    }
  }

  // Helper Functions End

  // Action Creation Begin

  action_t* warlock_t::create_action( util::string_view action_name, util::string_view options_str )
  {
    if ( affliction() )
    {
      if ( action_t* aff_action = create_action_affliction( action_name, options_str ) )
        return aff_action;
    }

    if ( demonology() )
    {
      if ( action_t* demo_action = create_action_demonology( action_name, options_str ) )
        return demo_action;
    }

    if ( destruction() )
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
    if ( action_name == "summon_felguard" && demonology() )
      return new summon_main_pet_t( "felguard", this, 30146 );
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
      if ( default_pet == "felguard" )
        return new summon_main_pet_t( "felguard", this, 30146 );

      return new summon_main_pet_t( default_pet, this );
    }

    // Shared Spells
    if ( action_name == "drain_life" )
      return new drain_life_t( this, options_str );
    if ( action_name == "corruption" && affliction() )
      return new corruption_t( this, options_str, false );
    if ( action_name == "shadow_bolt" && !destruction() )
      return new shadow_bolt_t( this, options_str );
    if ( action_name == "grimoire_of_sacrifice" && !demonology() )
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
    if ( action_name == "malefic_grasp" )
      return new malefic_grasp_t( this, options_str );
    if ( action_name == "haunt" )
      return new haunt_t( this, options_str );
    if ( action_name == "dark_harvest" )
      return new dark_harvest_t( this, options_str );
    if ( action_name == "seed_of_corruption" )
      return new seed_of_corruption_t( this, options_str );

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
    if ( action_name == "power_siphon" )
      return new power_siphon_t( this, options_str );
    if ( action_name == "call_dreadstalkers" )
      return new call_dreadstalkers_t( this, options_str );
    if ( action_name == "summon_demonic_tyrant" )
      return new summon_demonic_tyrant_t( this, options_str );
    if ( action_name == "grimoire_imp_lord" )
      return new grimoire_imp_lord_t( this, options_str );
    if ( action_name == "grimoire_fel_ravager" )
      return new grimoire_fel_ravager_t( this, options_str );
    if ( action_name == "summon_doomguard" )
      return new summon_doomguard_t( this, options_str );

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

  action_t* warlock_t::create_action_soul_harvester( util::string_view /* action_name */, util::string_view /* options_str */ )
  {
    return nullptr;
  }

  void warlock_t::create_actions()
  {
    if ( affliction() )
      create_affliction_proc_actions();

    if ( demonology() )
      create_demonology_proc_actions();

    if ( destruction() )
      create_destruction_proc_actions();

    create_diabolist_proc_actions();

    create_hellcaller_proc_actions();

    create_soul_harvester_proc_actions();

    player_t::create_actions();
  }

  void warlock_t::create_affliction_proc_actions()
  {
    if ( talents.shadow_of_nathreza_1.ok() )
      proc_actions.shadow_of_nathreza = new shadow_of_nathreza_dmg_t( this );

    if ( talents.shadow_of_nathreza_3.ok() )
      summons.desperate_soul = new summon_desperate_soul_t( this );
  }

  void warlock_t::create_demonology_proc_actions()
  {
    proc_actions.doom_proc     = new doom_t( this );
    proc_actions.blighted_maw  = new blighted_maw_t( this );
    summons.wild_imp           = new summon_wild_imp_t( this );
    summons.wild_imp_2         = new summon_wild_imp_2_t( this );
    summons.dreadstalker_1     = new summon_dreadstalker_1_t( this );
    summons.dreadstalker_2     = new summon_dreadstalker_2_t( this );

    if ( talents.mark_of_shatug.ok() )
      summons.vilefiend        = new summon_gloomhound_t( this );
    else if ( talents.mark_of_fharg.ok() )
      summons.vilefiend        = new summon_charhound_t( this );
    else
      summons.vilefiend        = new summon_vilefiend_t( this );

    summons.antoran_inquisitor = get_action<summon_antoran_inquisitor_t>( "dominion_of_argus_antoran_inquisitor", this );
    summons.antoran_jailer     = get_action<summon_antoran_jailer_t>( "dominion_of_argus_antoran_jailer", this );
    summons.lady_sacrolash     = get_action<summon_lady_sacrolash_t>( "dominion_of_argus_lady_sacrolash", this );
    summons.grand_warlock_alythess =
        get_action<summon_grand_warlock_alythess_t>( "dominion_of_argus_grand_warlock_alythess", this );
  }

  void warlock_t::create_destruction_proc_actions()
  {
    proc_actions.dimensional_rift = new dimensional_rift_t( this );
    proc_actions.demonfire_infusion = new demonfire_infusion_t( this );

    if ( talents.embers_of_nihilam_1.ok() )
      proc_actions.echo_of_sargeras = new echo_of_sargeras_t( this );

    if ( talents.embers_of_nihilam_3.ok() )
    {
      // NOTE: 2026-03-17 Echo of Sargeras is not scaled as stated for any of the spenders (bug)
      proc_actions.echo_of_sargeras_cb = new echo_of_sargeras_t( this, "echo_of_sargeras_cb", bugs ? 1.0 : talents.embers_of_nihilam_3->effectN( 1 ).percent() );
      proc_actions.echo_of_sargeras_sb = new echo_of_sargeras_t( this, "echo_of_sargeras_sb", bugs ? 1.0 : talents.embers_of_nihilam_3->effectN( 2 ).percent() );
      proc_actions.echo_of_sargeras_rof = new echo_of_sargeras_t( this, "echo_of_sargeras_rof", bugs ? 1.0 : talents.embers_of_nihilam_3->effectN( 3 ).percent() );
    }

    if ( talents.embers_of_nihilam_1.ok() || talents.embers_of_nihilam_3.ok() )
      proc_actions.embers_of_nihilam = new embers_of_nihilam_t( this );

    summons.infernal = new summon_main_infernal_pet_t( this );
    summons.roc = new summon_roc_infernal_pet_t( this );
    summons.fragment = new summon_fragment_infernal_pet_t( this );
    summons.shadowy_rift = new summon_shadowy_tear_t( this );
    summons.unstable_rift = new summon_unstable_tear_t( this );
    summons.chaos_rift = new summon_chaos_tear_t( this );
    summons.overfiend = new summon_overfiend_t( this );
  }

  void warlock_t::create_diabolist_proc_actions()
  {
    proc_actions.eye_explosion= new eye_explosion_t( this );
    proc_actions.diabolic_gaze_1 = new diabolic_gaze_1_t( this );
    proc_actions.diabolic_gaze_2 = new diabolic_gaze_2_t( this );
    proc_actions.diabolic_gaze_3 = new diabolic_gaze_3_t( this );
    proc_actions.diabolic_oculi = new diabolic_oculi_t( this );
    summons.overlord = new summon_overlord_t( this );
    summons.mother = new summon_mother_of_chaos_t( this );
    summons.pit_lord = new summon_pit_lord_t( this );
    summons.diabolic_imp = new summon_diabolic_imp_t( this );
  }

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
    summons.manifested_demonic_soul = new summon_manifested_demonic_soul_t( this );
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

      buffs.grimoire_of_sacrifice->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
          if ( new_ == 1 ) cb->activate();
          else cb->deactivate();
        } );
    }
  }

  // Action Creation End

} //namespace warlock

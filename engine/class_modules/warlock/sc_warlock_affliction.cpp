#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock
{
  namespace actions_affliction
  {
    using namespace actions;

    enum db_state
    {
      DB_DOT_DAMAGE = 0u,
      DB_DOT_TICKS_LEFT
    };

    struct affliction_spell_t : public warlock_spell_t
    {
    public:
      gain_t * gain;
      timespan_t db_max_contribution;

      affliction_spell_t(warlock_t* p, const std::string& n) :
        affliction_spell_t(n, p, p -> find_class_spell(n))
      {
      }

      affliction_spell_t(warlock_t* p, const std::string& n, specialization_e s) :
        affliction_spell_t(n, p, p -> find_class_spell(n, s))
      {
      }

      affliction_spell_t(const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil()) :
        warlock_spell_t(token, p, s)
      {
        may_crit = true;
        tick_may_crit = true;
        weapon_multiplier = 0.0;
        gain = player->get_gain(name_str);

        db_max_contribution = 0_ms;
      }

      void init() override
      {
        warlock_spell_t::init();

        if (p()->talents.creeping_death->ok())
        {
          if (data().affected_by(p()->talents.creeping_death->effectN(1)))
            base_tick_time *= 1.0 + p()->talents.creeping_death->effectN(1).percent();
          if (data().affected_by(p()->talents.creeping_death->effectN(2)))
            dot_duration *= 1.0 + p()->talents.creeping_death->effectN(2).percent();
        }
      }

      double action_multiplier() const override
      {
        double pm = warlock_spell_t::action_multiplier();

        if ( this->data().affected_by( p()->mastery_spells.potent_afflictions->effectN( 1 ) ) )
          pm *= 1.0 + p()->cache.mastery_value();

        return pm;
      }

      virtual timespan_t get_db_dot_duration( dot_t* dot ) const
      {
        return std::min(db_max_contribution,dot->remains());
      }

      virtual std::tuple<double, double> get_db_dot_state( dot_t* dot )
      {
        action_state_t* state = dot->current_action->get_state( dot->state );
        dot->current_action->calculate_tick_amount( state, 1.0 );

        timespan_t db_duration = get_db_dot_duration(dot);
        timespan_t dot_tick_time = dot->current_action->tick_time( state );

        double ticks_left = 1.0;

        if (db_duration < dot->remains())
        {
          //If using the full duration, time divided by tick time always gives proper results
          ticks_left = db_duration/dot_tick_time;
        }
        else
        {
          if (db_duration <= dot_tick_time && dot->time_to_next_tick() >= db_duration)
          {
            //All that's left is a partial tick
            ticks_left = db_duration/dot_tick_time;
          }
          else
          {
            //Make sure calculations are always done relative to a tick time
            timespan_t shifted_duration = db_duration - dot->time_to_next_tick();

            //Number of ticks remaining, including the tick we just "removed"
            ticks_left = 1+shifted_duration/dot_tick_time;

            //If a tick is about to happen but we haven't ticked it yet, update this
            //This is a small edge case that should only happen when Deathbolt is executed at the 
            //exact same time as a tick event and comes earlier in the stack
            if(dot->time_to_next_tick() == 0_ms)
              ticks_left += 1;
          }
        }

        if ( sim->debug )
        {
          sim->out_debug.printf( "%s %s dot_remains=%.3f duration=%.3f time_to_next=%.3f tick_time=%.3f "
                                "ticks_left=%.3f amount=%.3f total=%.3f",
            name(), dot->name(), dot->remains().total_seconds(), dot->duration().total_seconds(),
            dot->time_to_next_tick().total_seconds(), dot_tick_time.total_seconds(),
            ticks_left, state->result_raw, state->result_raw * ticks_left );
        }

        auto s = std::make_tuple( state->result_raw, ticks_left );

        action_state_t::release( state );

        return s;
      }
    };

    const std::array<int, MAX_UAS> ua_spells = { { 233490, 233496, 233497, 233498, 233499 } };

    struct shadow_bolt_t : public affliction_spell_t
    {

      shadow_bolt_t(warlock_t* p, const std::string& options_str) :
        affliction_spell_t(p, "Shadow Bolt", p->specialization())
      {
        parse_options(options_str);
      }

      timespan_t execute_time() const override
      {
        if (p()->buffs.nightfall->check())
        {
          return 0_ms;
        }

        return affliction_spell_t::execute_time();
      }

      bool ready() override
      {
        if ( p()->talents.drain_soul->ok() )
          return false;

        return affliction_spell_t::ready();
      }

      void impact(action_state_t* s) override
      {
        affliction_spell_t::impact(s);
        if (result_is_hit(s->result))
        {
          if (p()->talents.shadow_embrace->ok())
            td(s->target)->debuffs_shadow_embrace->trigger();
        }
      }

      double action_multiplier()const override
      {
        double m = affliction_spell_t::action_multiplier();

        if (time_to_execute == 0_ms && p()->buffs.nightfall->check())
          m *= 1.0 + p()->buffs.nightfall->default_value;

        return m;
      }

      void schedule_execute(action_state_t* s) override
      {
        affliction_spell_t::schedule_execute(s);
      }

      void execute() override
      {
        affliction_spell_t::execute();
        if(time_to_execute == 0_ms)
          p()->buffs.nightfall->expire();
      }
    };

    // Dots
    struct agony_t : public affliction_spell_t
    {
      double chance;
      bool pandemic_invocation_usable;

      agony_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( p, "Agony")
      {
        parse_options( options_str );
        may_crit = false;
        pandemic_invocation_usable = false;

        dot_max_stack = data().max_stacks() + p->spec.agony_2->effectN(1).base_value();
        db_max_contribution = data().duration();
      }

      void last_tick( dot_t* d ) override
      {
        if ( p()->get_active_dots( internal_id ) == 1 )
        {
          p()->agony_accumulator = rng().range( 0.0, 0.99 );
        }

        affliction_spell_t::last_tick( d );
      }

      void init() override
      {
        dot_max_stack += p()->talents.writhe_in_agony->ok() ? p()->talents.writhe_in_agony->effectN(1).base_value() : 0;

        affliction_spell_t::init();
      }

      void execute() override
      {
        // Do checks for Pandemic Invocation before parent execute() is called so we get the correct DoT states.
        if ( p()->azerite.pandemic_invocation.ok() && td( target )->dots_agony->is_ticking() && td( target )->dots_agony->remains() <= p()->azerite.pandemic_invocation.spell_ref().effectN( 2 ).time_value() )
          pandemic_invocation_usable = true;

        affliction_spell_t::execute();

        if ( pandemic_invocation_usable )
        {
          p()->active.pandemic_invocation->schedule_execute();
          pandemic_invocation_usable = false;
        }

        if (p()->azerite.sudden_onset.ok() && td(execute_state->target)->dots_agony->current_stack() < (int)p()->azerite.sudden_onset.spell_ref().effectN(2).base_value())
        {
          td(execute_state->target)->dots_agony->increment((int)p()->azerite.sudden_onset.spell_ref().effectN(2).base_value() - td(execute_state->target)->dots_agony->current_stack());
        }
      }

      double bonus_ta(const action_state_t* s) const override
      {
        double ta = affliction_spell_t::bonus_ta(s);
        //TOCHECK: How does Sudden Onset behave with Writhe in Agony's increased stack cap?
        ta += p()->azerite.sudden_onset.value();
        return ta;
      }

      void tick( dot_t* d ) override
      {
        td(d->state->target)->dots_agony->increment(1);

        // Blizzard has not publicly released the formula for Agony's chance to generate a Soul Shard.
        // This set of code is based on results from 500+ Soul Shard sample sizes, and matches in-game
        // results to within 0.1% of accuracy in all tests conducted on all targets numbers up to 8.
        // Accurate as of 08-24-2018. TOCHECK regularly. If any changes are made to this section of
        // code, please also update the Time_to_Shard expression in sc_warlock.cpp.
        double increment_max = 0.368;

        double active_agonies = p()->get_active_dots( internal_id );
        increment_max *= std::pow(active_agonies, -2.0 / 3.0);

        if ( p()->talents.creeping_death->ok() )
        {
          increment_max *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
        }

        p()->agony_accumulator += rng().range( 0.0, increment_max );

        if ( p()->agony_accumulator >= 1 )
        {
          if ( p()->azerite.wracking_brilliance.ok() )
          {
            if ( p()->wracking_brilliance ) {
              p()->wracking_brilliance = false;
              p()->buffs.wracking_brilliance->trigger();
            }
            else {
              p()->wracking_brilliance = true;
            }
          }

          p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony );
          p()->agony_accumulator -= 1.0;
        }

        if ( result_is_hit( d->state->result ) && p()->azerite.inevitable_demise.ok() && !p()->buffs.drain_life->check() )
        {
          p()->buffs.inevitable_demise->trigger();
        }

        affliction_spell_t::tick( d );
      }

      std::tuple<double, double> get_db_dot_state( dot_t* dot ) override
      {
        std::tuple<double, double>  agony = affliction_spell_t::get_db_dot_state( dot );

        auto s = std::make_tuple( std::get<0>( agony ) * td( execute_state->target )->dots_agony->current_stack(),
                                  std::get<1>( agony ) );

        return s;
      }
    };

    struct corruption_t : public affliction_spell_t
    {
      bool pandemic_invocation_usable;

      corruption_t( warlock_t* p, const std::string& options_str) :
        affliction_spell_t( "Corruption", p, p -> find_spell(172) )  //triggers 146739
      {
        parse_options(options_str);
        may_crit = false;
        tick_zero = false;
        pandemic_invocation_usable = false;
        dot_duration = db_max_contribution = data().effectN( 1 ).trigger()->duration();
        spell_power_mod.tick = data().effectN( 1 ).trigger()->effectN( 1 ).sp_coeff();
        base_tick_time = data().effectN( 1 ).trigger()->effectN( 1 ).period();
        // TOCHECK see if we can redo corruption in a way that spec aura applies to corruption naturally in init.
        base_multiplier *= 1.0 + p->spec.affliction->effectN(2).percent();

        if ( p->talents.absolute_corruption->ok() )
        {
          dot_duration = sim->expected_iteration_time > 0_ms ?
            2 * sim->expected_iteration_time :
            2 * sim->max_time * ( 1.0 + sim->vary_combat_length ); // "infinite" duration
          base_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent();
        }
      }

      void tick( dot_t* d ) override
      {
        if (result_is_hit(d->state->result) && p()->talents.nightfall->ok())
        {
          auto success = p()->buffs.nightfall->trigger();
          if (success)
          {
            p()->procs.nightfall->occur();
          }
        }

        affliction_spell_t::tick( d );
      }

      void execute() override
      {
        // Do checks for Pandemic Invocation before parent execute() is called so we get the correct DoT states.
        if ( p()->azerite.pandemic_invocation.ok() && td( target )->dots_corruption->is_ticking() && td( target )->dots_corruption->remains() <= p()->azerite.pandemic_invocation.spell_ref().effectN( 2 ).time_value() )
          pandemic_invocation_usable = true;

        affliction_spell_t::execute();

        if ( pandemic_invocation_usable )
        {
          p()->active.pandemic_invocation->schedule_execute();
          pandemic_invocation_usable = false;
        }
      }
    };

    struct unstable_affliction_t : public affliction_spell_t
    {
      struct real_ua_t : public affliction_spell_t
      {
        int self;

        real_ua_t( warlock_t* p, int num ) :
          affliction_spell_t( "unstable_affliction_" + std::to_string( num + 1 ), p, p -> find_spell( ua_spells[num] ) ),
          self( num )
        {
          background = true;
          dual = true;
          tick_may_crit = true;
          hasted_ticks = false;
          tick_zero = true;
          db_max_contribution = data().duration();
        }

        timespan_t composite_dot_duration( const action_state_t* s ) const override
        {
          return s->action->tick_time( s ) * 4.0;
        }

        void init() override
        {
          affliction_spell_t::init();

          update_flags &= ~STATE_HASTE;
        }

        void last_tick( dot_t* d ) override
        {
          p()->buffs.active_uas->decrement( 1 );

          affliction_spell_t::last_tick( d );
        }

        double bonus_ta( const action_state_t* s ) const override
        {
          double ta = affliction_spell_t::bonus_ta( s );
          ta += p()->azerite.dreadful_calling.value( 2 );
          return ta;
        }
      };

      std::array<real_ua_t*, MAX_UAS> ua_dots;

      unstable_affliction_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( "unstable_affliction", p, p -> spec.unstable_affliction ),
        ua_dots()
      {
        parse_options( options_str );
        for ( unsigned i = 0; i < ua_dots.size(); ++i ) {
          ua_dots[i] = new real_ua_t( p, i );
          add_child( ua_dots[i] );
        }
        const spell_data_t* ptr_spell = p->find_spell( 233490 );
        spell_power_mod.direct = ptr_spell->effectN( 1 ).sp_coeff();
        dot_duration = 0_ms; // DoT managed by ignite action.
      }

      void init() override
      {
        affliction_spell_t::init();
        snapshot_flags &= ~( STATE_CRIT | STATE_TGT_CRIT );
      }

      void impact( action_state_t* s ) override
      {
        if ( result_is_hit( s->result ) )
        {
          real_ua_t* real_ua = nullptr;
          timespan_t min_duration = 100_s;

          warlock_td_t* target_data = td( s->target );
          for ( int i = 0; i < MAX_UAS; i++ )
          {
            if ( ! target_data || !target_data->dots_unstable_affliction[i]->is_ticking() )
            {
              real_ua = ua_dots[i];
              p()->buffs.active_uas->increment( 1 );
              break;
            }

            timespan_t rem = target_data->dots_unstable_affliction[i]->remains();

            if ( rem < min_duration )
            {
              real_ua = ua_dots[i];
              min_duration = rem;
            }
          }

          if (p()->azerite.cascading_calamity.ok())
          {
            for (int i = 0; i < MAX_UAS; i++)
            {
              if (target_data && target_data->dots_unstable_affliction[i]->is_ticking())
              {
                p()->buffs.cascading_calamity->trigger();
                break;
              }
            }
          }

          real_ua->set_target( s->target );
          real_ua->schedule_execute();
        }
      }

      void execute() override
      {
        affliction_spell_t::execute();

        if (p()->azerite.dreadful_calling.ok())
        {
          p()->cooldowns.darkglare->adjust((-1 * p()->azerite.dreadful_calling.spell_ref().effectN(1).time_value()));
        }
      }
    };

    struct summon_darkglare_t : public affliction_spell_t
    {
      summon_darkglare_t(warlock_t* p, const std::string& options_str) :
        affliction_spell_t("summon_darkglare", p, p -> spec.summon_darkglare)
      {
        parse_options(options_str);
        harmful = may_crit = may_miss = false;
      }

      void execute() override
      {
        affliction_spell_t::execute();

        for (auto& darkglare : p()->warlock_pet_list.darkglare)
        {
          if (darkglare->is_sleeping())
          {
            darkglare->summon(data().duration());
          }
        }

        for (const auto target : sim->target_non_sleeping_list)
        {
          auto td = this->td(target);
          if (!td)
          {
            continue;
          }
          timespan_t darkglare_extension = timespan_t::from_seconds(p()->spec.summon_darkglare->effectN(2).base_value());
          td->dots_agony->extend_duration(darkglare_extension);
          td->dots_corruption->extend_duration(darkglare_extension);
          td->dots_siphon_life->extend_duration(darkglare_extension);
          td->dots_phantom_singularity->extend_duration(darkglare_extension);
          td->dots_vile_taint->extend_duration(darkglare_extension);
          for (auto& current_ua : td->dots_unstable_affliction)
          {
            current_ua->extend_duration(darkglare_extension);
          }
        }
      }
    };

    // AOE
    struct seed_of_corruption_t : public affliction_spell_t
    {
      struct seed_of_corruption_aoe_t : public affliction_spell_t
      {
        corruption_t* corruption;

        seed_of_corruption_aoe_t( warlock_t* p ) :
          affliction_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) ),
          corruption( new corruption_t(p,"") )
        {
          aoe = -1;
          background = true;
          p->spells.seed_of_corruption_aoe = this;
          base_costs[RESOURCE_MANA] = 0;

          corruption->background = true;
          corruption->dual = true;
          corruption->base_costs[RESOURCE_MANA] = 0;
        }

        double bonus_da(const action_state_t* s) const override
        {
          double da = affliction_spell_t::bonus_da(s);
          return da;
        }

        void impact( action_state_t* s ) override
        {
          affliction_spell_t::impact( s );

          if ( result_is_hit( s->result ) )
          {
            warlock_td_t* tdata = this->td( s->target );
            if ( tdata->dots_seed_of_corruption->is_ticking() && tdata->soc_threshold > 0 )
            {
              tdata->soc_threshold = 0;
              tdata->dots_seed_of_corruption->cancel();
            }

            corruption->set_target(s->target);
            corruption->execute();
          }
        }
      };

      double threshold_mod;
      double sow_the_seeds_targets;
      seed_of_corruption_aoe_t* explosion;

      seed_of_corruption_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( "seed_of_corruption", p, p -> find_spell( 27243 ) ),
          threshold_mod( 3.0 ),
          sow_the_seeds_targets( p->talents.sow_the_seeds->effectN( 1 ).base_value() ),
          explosion( new seed_of_corruption_aoe_t( p ) )
      {
        parse_options( options_str );
        may_crit = false;
        tick_zero = false;
        base_tick_time = dot_duration;
        hasted_ticks = false;
        add_child( explosion );
        if ( p->talents.sow_the_seeds->ok() )
          aoe = 2;
      }

      void init() override
      {
        affliction_spell_t::init();
        snapshot_flags |= STATE_SP;
      }

      void execute() override
      {
        if (td(target)->dots_seed_of_corruption->is_ticking() || has_travel_events_for(target))
        {
          for (auto& possible : target_list())
          {
            if (!(td(possible)->dots_seed_of_corruption->is_ticking() || has_travel_events_for(possible)))
            {
              set_target(possible);
              break;
            }
          }
        }

        affliction_spell_t::execute();
      }

      void impact( action_state_t* s ) override
      {
        //TOCHECK: Does the threshold reset if the duration is refreshed before explosion?
        if ( result_is_hit( s->result ) )
        {
          td( s->target )->soc_threshold = s->composite_spell_power();
        }

        affliction_spell_t::impact( s );
      }

      //Seed of Corruption is currently bugged on pure single target, extending the duration
      //but still exploding at the original time, wiping the debuff. tick() should be used instead
      //of last_tick() for now to model this more appropriately. TOCHECK regularly
      void tick( dot_t* d ) override
      {
        affliction_spell_t::tick(d);
        
        if(d->remains() > 0_ms)
          d->cancel();
      }

      void last_tick(dot_t* d) override
      {
        explosion->set_target( d->target );
        explosion->schedule_execute();

        affliction_spell_t::last_tick(d);
      }
    };

    // Talents

    // lvl 15 - nightfall|drain soul|haunt
    struct drain_soul_t : public affliction_spell_t
    {
      drain_soul_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( "drain_soul", p, p -> talents.drain_soul )
      {
        parse_options(options_str);
        channeled = true;
        hasted_ticks = may_crit = true;
      }

      void tick(dot_t* d) override
      {
        affliction_spell_t::tick(d);
        if (result_is_hit(d->state->result))
        {
          if (p()->talents.shadow_embrace->ok())
            td(d->target)->debuffs_shadow_embrace->trigger();
        }
      }

      virtual double composite_target_multiplier(player_t* t) const override
      {
        double m = affliction_spell_t::composite_target_multiplier(t);

        if (t->health_percentage() < p()->talents.drain_soul->effectN(3).base_value())
          m *= 1.0 + p()->talents.drain_soul->effectN(2).percent();

        return m;
      }
    };

    struct haunt_t : public affliction_spell_t
    {
      haunt_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( "haunt", p, p -> talents.haunt )
      {
        parse_options( options_str );
      }

      void impact( action_state_t* s ) override
      {
        affliction_spell_t::impact( s );

        if ( result_is_hit( s->result ) )
        {
          td( s->target )->debuffs_haunt->trigger();
        }
      }
    };

    // lvl 30 - writhe|ac|siphon life
    struct siphon_life_t : public affliction_spell_t
    {
      bool pandemic_invocation_usable;

      siphon_life_t(warlock_t* p, const std::string& options_str) :
        affliction_spell_t("siphon_life", p, p -> talents.siphon_life)
      {
        parse_options(options_str);
        may_crit = false;
        db_max_contribution = data().duration();
        pandemic_invocation_usable = false;
      }

      void execute() override
      {
        // Do checks for Pandemic Invocation before parent execute() is called so we get the correct DoT states.
        if ( p()->azerite.pandemic_invocation.ok() && td( target )->dots_siphon_life->is_ticking() && td( target )->dots_siphon_life->remains() <= p()->azerite.pandemic_invocation.spell_ref().effectN( 2 ).time_value() )
          pandemic_invocation_usable = true;

        affliction_spell_t::execute();

        if ( pandemic_invocation_usable )
        {
          p()->active.pandemic_invocation->schedule_execute();
          pandemic_invocation_usable = false;
        }
      }
    };
    // lvl 45 - demon skin|burning rush|dark pact

    // lvl 60 - sow the seeds|phantom singularity|vile taint
    struct phantom_singularity_tick_t : public affliction_spell_t
    {
      phantom_singularity_tick_t( warlock_t* p ) :
        affliction_spell_t( "phantom_singularity_tick", p, p -> find_spell( 205246 ) )
      {
        background = true;
        may_miss = false;
        dual = true;
        aoe = -1;
      }

      result_e calculate_result( action_state_t* s) const
      {
        //TOCHECK Bug - Phantom Singularity does not crit it's primary target. Live as of 10-19-2018.
        if (s->chain_target == 0)
          return RESULT_HIT;
        return affliction_spell_t::calculate_result(s);
      }
    };

    struct phantom_singularity_t : public affliction_spell_t
    {

      phantom_singularity_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( "phantom_singularity", p, p -> talents.phantom_singularity )
      {
        parse_options( options_str );
        callbacks = false;
        hasted_ticks = true;
        tick_action = new phantom_singularity_tick_t( p );

        spell_power_mod.tick = 0;

        db_max_contribution = data().duration();
      }

      void init() override
      {
        affliction_spell_t::init();

        update_flags &= ~STATE_HASTE;
      }

      timespan_t composite_dot_duration( const action_state_t* s ) const override
      { return s->action->tick_time( s ) * 8.0; }

      // Phantom singularity damage for the Deathbolt is a composite of two things
      // 1) The number of ticks left on this spell (phantom_singularity_t)
      // 2) The direct damage the tick action does (phantom_singularity_tick_t)
      std::tuple<double, double> get_db_dot_state( dot_t* dot ) override
      {
        // Get base state so we get the correct number of ticks_left
        auto base_state = affliction_spell_t::get_db_dot_state( dot );

        // Then calculate damage based on the tick action
        action_state_t* damage_action_state = tick_action->get_state();
        damage_action_state->target = dot->target;
        tick_action->snapshot_state( damage_action_state, DMG_DIRECT );
        tick_action->calculate_direct_amount( damage_action_state );

        // Recreate the db state object with the calculated tick action damage, and the base db
        // state ticks left
        auto state = std::make_tuple( damage_action_state->result_raw,
            std::get<DB_DOT_TICKS_LEFT>( base_state ) );

        if ( sim->debug )
        {
          sim->out_debug.printf( "%s %s amount=%.3f total=%.3f",
            name(), dot->name(),
            std::get<DB_DOT_DAMAGE>( state ),
            std::get<DB_DOT_DAMAGE>( state ) * std::get<DB_DOT_TICKS_LEFT>( state ) );
        }

        action_state_t::release( damage_action_state );

        return state;
      }
    };

    struct vile_taint_t : public affliction_spell_t
    {
      vile_taint_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( "vile_taint", p, p -> talents.vile_taint )
      {
        parse_options( options_str );

        hasted_ticks = tick_zero = true;
        aoe = -1;

        db_max_contribution = data().duration();
      }
    };
    // lvl 75 - darkfury|mortal coil|demonic circle

    // lvl 90 - nightfall|deathbolt|grimoire of sacrifice
    struct deathbolt_t : public affliction_spell_t
    {

      deathbolt_t(warlock_t* p, const std::string& options_str) :
        affliction_spell_t("deathbolt", p, p -> talents.deathbolt)
      {
        parse_options(options_str);
      }

      void init() override
      {
        affliction_spell_t::init();

        snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;
      }

      double get_contribution_from_dot( dot_t* dot )
      {
        if ( !dot->is_ticking() )
          return 0.0;

        auto dot_action = debug_cast<affliction_spell_t*>( dot->current_action );

        auto tick_state = dot_action -> get_db_dot_state( dot );

        return std::get<DB_DOT_TICKS_LEFT>( tick_state ) *
               std::get<DB_DOT_DAMAGE>( tick_state );
      }

      void execute() override
      {
        warlock_td_t* td = this->td(target);

        double total_dot_dmg = 0.0;

        total_dot_dmg += get_contribution_from_dot(td->dots_agony);
        total_dot_dmg += get_contribution_from_dot(td->dots_corruption);

        if (p()->talents.siphon_life->ok())
          total_dot_dmg += get_contribution_from_dot(td->dots_siphon_life);

        if ( p()->talents.phantom_singularity->ok() )
          total_dot_dmg += get_contribution_from_dot( td->dots_phantom_singularity );

        if ( p()->talents.vile_taint->ok() )
          total_dot_dmg += get_contribution_from_dot( td->dots_vile_taint );

        for (auto& current_ua : td->dots_unstable_affliction)
        {
          total_dot_dmg += get_contribution_from_dot(current_ua);
        }

        this->base_dd_min = this->base_dd_max = (total_dot_dmg * data().effectN(2).percent());

        if (sim->debug) {
          sim->out_debug.printf("%s deathbolt damage_remaining=%.3f", name(), total_dot_dmg);
        }

        affliction_spell_t::execute();
      }

      void impact( action_state_t* s ) override
      {
        s->result_total = base_dd_min; // we already calculated how much the hit should be

        affliction_spell_t::impact( s );
      }
    };

    // Azerite
    //TOCHECK: Does this damage proc affect Seed of Corruption?
    struct pandemic_invocation_t : public affliction_spell_t
    {
      pandemic_invocation_t( warlock_t* p ):
        affliction_spell_t( "Pandemic Invocation", p, p->find_spell( 289367 ) )
      {
        background = true;

        base_dd_min = base_dd_max = p->azerite.pandemic_invocation.value();
      }

      void execute() override
      {
        affliction_spell_t::execute();

        if ( p()->rng().roll( p()->azerite.pandemic_invocation.spell_ref().effectN( 3 ).percent() / 100.0 ) )
          p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.pandemic_invocation );
      }
    };

    // lvl 100 - soul conduit|creeping death|dark soul misery
    struct dark_soul_t : public affliction_spell_t
    {
      dark_soul_t(warlock_t* p, const std::string& options_str) :
        affliction_spell_t("dark_soul", p, p -> talents.dark_soul_misery)
      {
        parse_options(options_str);
        harmful = may_crit = may_miss = false;
      }

      void execute() override
      {
        affliction_spell_t::execute();

        p()->buffs.dark_soul_misery->trigger();
      }
    };
  } // end actions namespace

  namespace buffs_affliction
  {
    using namespace buffs;

  } // end buffs namespace

  // add actions
  action_t* warlock_t::create_action_affliction( const std::string& action_name, const std::string& options_str )
  {
    using namespace actions_affliction;

    if ( action_name == "shadow_bolt" ) return new                    shadow_bolt_t(this, options_str);
    if ( action_name == "corruption" ) return new                     corruption_t( this, options_str );
    if ( action_name == "agony" ) return new                          agony_t( this, options_str );
    if ( action_name == "unstable_affliction" ) return new            unstable_affliction_t( this, options_str );
    if ( action_name == "summon_darkglare") return new                summon_darkglare_t(this, options_str);
    // aoe
    if ( action_name == "seed_of_corruption" ) return new             seed_of_corruption_t( this, options_str );
    // talents
    if ( action_name == "drain_soul") return new                      drain_soul_t(this, options_str);
    if ( action_name == "haunt" ) return new                          haunt_t( this, options_str );
    if ( action_name == "deathbolt" ) return new                      deathbolt_t( this, options_str );
    if ( action_name == "phantom_singularity" ) return new            phantom_singularity_t( this, options_str );
    if ( action_name == "siphon_life" ) return new                    siphon_life_t( this, options_str );
    if ( action_name == "dark_soul" ) return new                      dark_soul_t( this, options_str );
    if ( action_name == "vile_taint" ) return new                     vile_taint_t(this, options_str);

    return nullptr;
  }

  void warlock_t::create_buffs_affliction()
  {
    //spells
    buffs.active_uas = make_buff( this, "active_uas" )
      ->set_max_stack( 20 );
    buffs.drain_life = make_buff( this, "drain_life" );
    //talents
    buffs.dark_soul_misery = make_buff(this, "dark_soul", talents.dark_soul_misery)
      ->set_default_value(talents.dark_soul_misery->effectN(1).percent())
      ->add_invalidate(CACHE_SPELL_HASTE);
    buffs.nightfall = make_buff( this, "nightfall", find_spell( 264571 ) )
      ->set_default_value( find_spell( 264571 )->effectN( 2 ).percent() )
      ->set_trigger_spell( talents.nightfall );
    //azerite
    buffs.cascading_calamity = make_buff<stat_buff_t>(this, "cascading_calamity", azerite.cascading_calamity)
      ->add_stat(STAT_HASTE_RATING, azerite.cascading_calamity.value())
      ->set_duration(find_spell(275378)->duration())
      ->set_refresh_behavior(buff_refresh_behavior::DURATION);
    buffs.wracking_brilliance = make_buff<stat_buff_t>(this, "wracking_brilliance", azerite.wracking_brilliance)
      ->add_stat(STAT_INTELLECT, azerite.wracking_brilliance.value())
      ->set_duration(find_spell(272893)->duration())
      ->set_refresh_behavior(buff_refresh_behavior::DURATION);
    buffs.inevitable_demise = make_buff(this, "inevitable_demise", azerite.inevitable_demise)
      ->set_max_stack( find_spell(273525)->max_stacks() )
      // Inevitable Demise has a built in 25% reduction to the value of ranks 2 and 3. This is applied as a flat multiplier to the total value.
      ->set_default_value( azerite.inevitable_demise.value() * ( ( 1.0 + 0.75 * ( azerite.inevitable_demise.n_items() - 1 ) ) / azerite.inevitable_demise.n_items() ) );
  }

  void warlock_t::init_spells_affliction()
  {
    using namespace actions_affliction;
    // General
    spec.affliction                     = find_specialization_spell( 137043 );
    mastery_spells.potent_afflictions   = find_mastery_spell( WARLOCK_AFFLICTION );
    // Specialization Spells
    spec.unstable_affliction            = find_specialization_spell( "Unstable Affliction" );
    spec.agony                          = find_specialization_spell( "Agony" );
    spec.agony_2                        = find_spell( 231792 );
    spec.summon_darkglare               = find_specialization_spell( "Summon Darkglare" );
    // Talents
    talents.nightfall                   = find_talent_spell( "Nightfall" );
    talents.drain_soul                  = find_talent_spell( "Drain Soul" );
    talents.haunt                       = find_talent_spell( "Haunt" );
    talents.writhe_in_agony             = find_talent_spell( "Writhe in Agony" );
    talents.absolute_corruption         = find_talent_spell( "Absolute Corruption" );
    talents.siphon_life                 = find_talent_spell( "Siphon Life" );
    talents.sow_the_seeds               = find_talent_spell( "Sow the Seeds" );
    talents.phantom_singularity         = find_talent_spell( "Phantom Singularity" );
    talents.vile_taint                  = find_talent_spell( "Vile Taint" );
    talents.shadow_embrace              = find_talent_spell( "Shadow Embrace" );
    talents.deathbolt                   = find_talent_spell( "Deathbolt" );
    talents.creeping_death              = find_talent_spell( "Creeping Death" );
    talents.dark_soul_misery            = find_talent_spell( "Dark Soul: Misery" );
    // Azerite
    azerite.cascading_calamity          = find_azerite_spell("Cascading Calamity");
    azerite.dreadful_calling            = find_azerite_spell("Dreadful Calling");
    azerite.inevitable_demise           = find_azerite_spell("Inevitable Demise");
    azerite.sudden_onset                = find_azerite_spell("Sudden Onset");
    azerite.wracking_brilliance         = find_azerite_spell("Wracking Brilliance");
    azerite.pandemic_invocation         = find_azerite_spell( "Pandemic Invocation" );
    // Actives
    active.pandemic_invocation          = new pandemic_invocation_t( this );
  }

  void warlock_t::init_gains_affliction()
  {
    gains.agony                         = get_gain( "agony" );
    gains.seed_of_corruption            = get_gain( "seed_of_corruption" );
    gains.unstable_affliction_refund    = get_gain( "unstable_affliction_refund" );
    gains.drain_soul                    = get_gain( "drain_soul" );
    gains.pandemic_invocation           = get_gain( "pandemic_invocation" );
  }

  void warlock_t::init_rng_affliction()
  {
  }

  void warlock_t::init_procs_affliction()
  {
    procs.nightfall                     = get_proc( "nightfall" );
  }

  void warlock_t::create_apl_affliction()
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    action_priority_list_t* fil = get_action_priority_list( "fillers" );
    action_priority_list_t* cds = get_action_priority_list( "cooldowns" );
    action_priority_list_t* dots = get_action_priority_list( "dots" );
    action_priority_list_t* spend = get_action_priority_list( "spenders" );
    action_priority_list_t* dbr = get_action_priority_list( "db_refresh" );


    def->add_action( "variable,name=use_seed,value=talent.sow_the_seeds.enabled&spell_targets.seed_of_corruption_aoe>=3+raid_event.invulnerable.up|talent.siphon_life.enabled&spell_targets.seed_of_corruption>=5+raid_event.invulnerable.up|spell_targets.seed_of_corruption>=8+raid_event.invulnerable.up" );
    def->add_action( "variable,name=padding,op=set,value=action.shadow_bolt.execute_time*azerite.cascading_calamity.enabled" );
    def->add_action( "variable,name=padding,op=reset,value=gcd,if=azerite.cascading_calamity.enabled&(talent.drain_soul.enabled|talent.deathbolt.enabled&cooldown.deathbolt.remains<=gcd)" );
    def->add_action( "variable,name=maintain_se,value=spell_targets.seed_of_corruption_aoe<=1+talent.writhe_in_agony.enabled+talent.absolute_corruption.enabled*2+(talent.writhe_in_agony.enabled&talent.sow_the_seeds.enabled&spell_targets.seed_of_corruption_aoe>2)+(talent.siphon_life.enabled&!talent.creeping_death.enabled&!talent.drain_soul.enabled)+raid_event.invulnerable.up" );
    def->add_action( "call_action_list,name=cooldowns" );
    def->add_action( "drain_soul,interrupt_global=1,chain=1,cycle_targets=1,if=target.time_to_die<=gcd&soul_shard<5" );
    def->add_action( "haunt,if=spell_targets.seed_of_corruption_aoe<=2+raid_event.invulnerable.up" );
    def->add_action( "summon_darkglare,if=dot.agony.ticking&dot.corruption.ticking&(buff.active_uas.stack=5|soul_shard=0)&(!talent.phantom_singularity.enabled|dot.phantom_singularity.remains)&(!talent.deathbolt.enabled|cooldown.deathbolt.remains<=gcd|!cooldown.deathbolt.remains|spell_targets.seed_of_corruption_aoe>1+raid_event.invulnerable.up)" );
    def->add_action( "deathbolt,if=cooldown.summon_darkglare.remains&spell_targets.seed_of_corruption_aoe=1+raid_event.invulnerable.up" );
    def->add_action( "agony,target_if=min:dot.agony.remains,if=remains<=gcd+action.shadow_bolt.execute_time&target.time_to_die>8" );
    def->add_action( "unstable_affliction,target_if=!contagion&target.time_to_die<=8" );
    def->add_action( "drain_soul,target_if=min:debuff.shadow_embrace.remains,cancel_if=ticks_remain<5,if=talent.shadow_embrace.enabled&variable.maintain_se&debuff.shadow_embrace.remains&debuff.shadow_embrace.remains<=gcd*2" );
    def->add_action( "shadow_bolt,target_if=min:debuff.shadow_embrace.remains,if=talent.shadow_embrace.enabled&variable.maintain_se&debuff.shadow_embrace.remains&debuff.shadow_embrace.remains<=execute_time*2+travel_time&!action.shadow_bolt.in_flight" );
    def->add_action( "phantom_singularity,target_if=max:target.time_to_die,if=time>35&target.time_to_die>16*spell_haste" );
    def->add_action( "vile_taint,target_if=max:target.time_to_die,if=time>15&target.time_to_die>=10" );
    def->add_action( "unstable_affliction,target_if=min:contagion,if=!variable.use_seed&soul_shard=5" );
    def->add_action( "seed_of_corruption,if=variable.use_seed&soul_shard=5" );
    def->add_action( "call_action_list,name=dots" );
    def->add_action( "phantom_singularity,if=time<=35" );
    def->add_action( "vile_taint,if=time<15" );
    def->add_action( "dark_soul,if=cooldown.summon_darkglare.remains<10&dot.phantom_singularity.remains|target.time_to_die<20+gcd|spell_targets.seed_of_corruption_aoe>1+raid_event.invulnerable.up" );
    def->add_action( "berserking" );
    def->add_action( "call_action_list,name=spenders" );
    def->add_action( "call_action_list,name=fillers" );

    cds->add_action( "potion,if=(talent.dark_soul_misery.enabled&cooldown.summon_darkglare.up&cooldown.dark_soul.up)|cooldown.summon_darkglare.up|target.time_to_die<30" );
    cds->add_action( "use_items,if=!cooldown.summon_darkglare.up,if=cooldown.summon_darkglare.remains>70|time_to_die<20|((buff.active_uas.stack=5|soul_shard=0)&(!talent.phantom_singularity.enabled|cooldown.phantom_singularity.remains)&(!talent.deathbolt.enabled|cooldown.deathbolt.remains<=gcd|!cooldown.deathbolt.remains)&!cooldown.summon_darkglare.remains)" );
    cds->add_action( "fireblood,if=!cooldown.summon_darkglare.up" );
    cds->add_action( "blood_fury,if=!cooldown.summon_darkglare.up" );

    dots->add_action( "seed_of_corruption,if=dot.corruption.remains<=action.seed_of_corruption.cast_time+time_to_shard+4.2*(1-talent.creeping_death.enabled*0.15)&spell_targets.seed_of_corruption_aoe>=3+raid_event.invulnerable.up+talent.writhe_in_agony.enabled&!dot.seed_of_corruption.remains&!action.seed_of_corruption.in_flight" );
    dots->add_action( "agony,target_if=min:remains,if=talent.creeping_death.enabled&active_dot.agony<6&target.time_to_die>10&(remains<=gcd|cooldown.summon_darkglare.remains>10&(remains<5|!azerite.pandemic_invocation.rank&refreshable))" );
    dots->add_action( "agony,target_if=min:remains,if=!talent.creeping_death.enabled&active_dot.agony<8&target.time_to_die>10&(remains<=gcd|cooldown.summon_darkglare.remains>10&(remains<5|!azerite.pandemic_invocation.rank&refreshable))" );
    dots->add_action( "siphon_life,target_if=min:remains,if=(active_dot.siphon_life<8-talent.creeping_death.enabled-spell_targets.sow_the_seeds_aoe)&target.time_to_die>10&refreshable&(!remains&spell_targets.seed_of_corruption_aoe=1|cooldown.summon_darkglare.remains>soul_shard*action.unstable_affliction.execute_time)" );
    dots->add_action( "corruption,cycle_targets=1,if=spell_targets.seed_of_corruption_aoe<3+raid_event.invulnerable.up+talent.writhe_in_agony.enabled&(remains<=gcd|cooldown.summon_darkglare.remains>10&refreshable)&target.time_to_die>10" );

    spend->add_action( "unstable_affliction,if=cooldown.summon_darkglare.remains<=soul_shard*execute_time&(!talent.deathbolt.enabled|cooldown.deathbolt.remains<=soul_shard*execute_time)" );
    spend->add_action( "call_action_list,name=fillers,if=(cooldown.summon_darkglare.remains<time_to_shard*(6-soul_shard)|cooldown.summon_darkglare.up)&time_to_die>cooldown.summon_darkglare.remains" );
    spend->add_action( "seed_of_corruption,if=variable.use_seed" );
    spend->add_action( "unstable_affliction,if=!variable.use_seed&!prev_gcd.1.summon_darkglare&(talent.deathbolt.enabled&cooldown.deathbolt.remains<=execute_time&!azerite.cascading_calamity.enabled|(soul_shard>=5&spell_targets.seed_of_corruption_aoe<2|soul_shard>=2&spell_targets.seed_of_corruption_aoe>=2)&target.time_to_die>4+execute_time&spell_targets.seed_of_corruption_aoe=1|target.time_to_die<=8+execute_time*soul_shard)" );
    spend->add_action( "unstable_affliction,if=!variable.use_seed&contagion<=cast_time+variable.padding" );
    spend->add_action( "unstable_affliction,cycle_targets=1,if=!variable.use_seed&(!talent.deathbolt.enabled|cooldown.deathbolt.remains>time_to_shard|soul_shard>1)&(!talent.vile_taint.enabled|soul_shard>1)&contagion<=cast_time+variable.padding&(!azerite.cascading_calamity.enabled|buff.cascading_calamity.remains>time_to_shard)" );

    dbr->add_action( "siphon_life,line_cd=15,if=(dot.siphon_life.remains%dot.siphon_life.duration)<=(dot.agony.remains%dot.agony.duration)&(dot.siphon_life.remains%dot.siphon_life.duration)<=(dot.corruption.remains%dot.corruption.duration)&dot.siphon_life.remains<dot.siphon_life.duration*1.3" );
    dbr->add_action( "agony,line_cd=15,if=(dot.agony.remains%dot.agony.duration)<=(dot.corruption.remains%dot.corruption.duration)&(dot.agony.remains%dot.agony.duration)<=(dot.siphon_life.remains%dot.siphon_life.duration)&dot.agony.remains<dot.agony.duration*1.3" );
    dbr->add_action( "corruption,line_cd=15,if=(dot.corruption.remains%dot.corruption.duration)<=(dot.agony.remains%dot.agony.duration)&(dot.corruption.remains%dot.corruption.duration)<=(dot.siphon_life.remains%dot.siphon_life.duration)&dot.corruption.remains<dot.corruption.duration*1.3" );

    fil->add_action( "unstable_affliction,line_cd=15,if=cooldown.deathbolt.remains<=gcd*2&spell_targets.seed_of_corruption_aoe=1+raid_event.invulnerable.up&cooldown.summon_darkglare.remains>20" );
    fil->add_action( "call_action_list,name=db_refresh,if=talent.deathbolt.enabled&spell_targets.seed_of_corruption_aoe=1+raid_event.invulnerable.up&(dot.agony.remains<dot.agony.duration*0.75|dot.corruption.remains<dot.corruption.duration*0.75|dot.siphon_life.remains<dot.siphon_life.duration*0.75)&cooldown.deathbolt.remains<=action.agony.gcd*4&cooldown.summon_darkglare.remains>20" );
    fil->add_action( "call_action_list,name=db_refresh,if=talent.deathbolt.enabled&spell_targets.seed_of_corruption_aoe=1+raid_event.invulnerable.up&cooldown.summon_darkglare.remains<=soul_shard*action.agony.gcd+action.agony.gcd*3&(dot.agony.remains<dot.agony.duration*1|dot.corruption.remains<dot.corruption.duration*1|dot.siphon_life.remains<dot.siphon_life.duration*1)" );
    fil->add_action( "deathbolt,if=cooldown.summon_darkglare.remains>=30+gcd|cooldown.summon_darkglare.remains>140" );
    fil->add_action( "shadow_bolt,if=buff.movement.up&buff.nightfall.remains" );
    fil->add_action( "agony,if=buff.movement.up&!(talent.siphon_life.enabled&(prev_gcd.1.agony&prev_gcd.2.agony&prev_gcd.3.agony)|prev_gcd.1.agony)" );
    fil->add_action( "siphon_life,if=buff.movement.up&!(prev_gcd.1.siphon_life&prev_gcd.2.siphon_life&prev_gcd.3.siphon_life)" );
    fil->add_action( "corruption,if=buff.movement.up&!prev_gcd.1.corruption&!talent.absolute_corruption.enabled" );
    fil->add_action( "drain_life,if=(buff.inevitable_demise.stack>=40-(spell_targets.seed_of_corruption_aoe-raid_event.invulnerable.up>2)*20&(cooldown.deathbolt.remains>execute_time|!talent.deathbolt.enabled)&(cooldown.phantom_singularity.remains>execute_time|!talent.phantom_singularity.enabled)&(cooldown.dark_soul.remains>execute_time|!talent.dark_soul_misery.enabled)&(cooldown.vile_taint.remains>execute_time|!talent.vile_taint.enabled)&cooldown.summon_darkglare.remains>execute_time+10|buff.inevitable_demise.stack>10&target.time_to_die<=10)" );
    fil->add_action( "haunt" );
    fil->add_action( "drain_soul,interrupt_global=1,chain=1,interrupt=1,cycle_targets=1,if=target.time_to_die<=gcd" );
    fil->add_action( "drain_soul,target_if=min:debuff.shadow_embrace.remains,chain=1,interrupt_if=ticks_remain<5,interrupt_global=1,if=talent.shadow_embrace.enabled&variable.maintain_se&!debuff.shadow_embrace.remains" );
    fil->add_action( "drain_soul,target_if=min:debuff.shadow_embrace.remains,chain=1,interrupt_if=ticks_remain<5,interrupt_global=1,if=talent.shadow_embrace.enabled&variable.maintain_se" );
    fil->add_action( "drain_soul,interrupt_global=1,chain=1,interrupt=1" );
    fil->add_action( "shadow_bolt,cycle_targets=1,if=talent.shadow_embrace.enabled&variable.maintain_se&!debuff.shadow_embrace.remains&!action.shadow_bolt.in_flight" );
    fil->add_action( "shadow_bolt,target_if=min:debuff.shadow_embrace.remains,if=talent.shadow_embrace.enabled&variable.maintain_se" );
    fil->add_action( "shadow_bolt" );
  }

  expr_t* warlock_t::create_aff_expression(const std::string& name_str)
  {
    if (name_str == "deathbolt_setup")
    {
      return make_fn_expr("deathbolt_setup", [this]() {
        bool ready = false;

        timespan_t setup;
        double gcds_required = 0.0;
        timespan_t gcd = base_gcd * gcd_current_haste_value;

        auto td = get_target_data(target);

        gcds_required += td->dots_agony->remains() <= td->dots_agony->duration() ? 1 : 0;
        gcds_required += talents.absolute_corruption->ok() ? 0 : (td->dots_corruption->remains() <= td->dots_corruption->duration() ? 1 : 0);
        gcds_required += talents.siphon_life->ok() ? (td->dots_siphon_life->remains() <= td->dots_siphon_life->duration() ? 1 : 0) : 0;
        gcds_required += resources.current[RESOURCE_SOUL_SHARD];
        setup = gcd * gcds_required;
        if (talents.phantom_singularity->ok() && cooldowns.phantom_singularity->remains() <= setup)
        {
          gcds_required += 1;
          setup += gcd;
        }
        if (cooldowns.darkglare->remains() <= setup)
        {
          gcds_required += 1;
          setup += gcd;
        }
        if (sim->log)
          sim->out_log.printf("setup required %s", setup);

        if (cooldowns.deathbolt->remains() <= setup)
          ready = true;

        return ready;
      });
    }

    return player_t::create_expression(name_str);
  }
}

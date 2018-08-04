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
      }

      void init() override
      {
        warlock_spell_t::init();

        if (data().affected_by(p()->spec.affliction->effectN(1)))
          base_dd_multiplier *= 1.0 + p()->spec.affliction->effectN(1).percent();

        if (data().affected_by(p()->spec.affliction->effectN(2)))
          base_td_multiplier *= 1.0 + p()->spec.affliction->effectN(2).percent();

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

      void consume_resource() override
      {
        warlock_spell_t::consume_resource();

        if (resource_current == RESOURCE_SOUL_SHARD && p()->in_combat)
        {
          if (p()->legendary.the_master_harvester)
          {
            double sh_proc_chance = p()->find_spell(p()->legendary.the_master_harvester->spell_id)->effectN(1).percent();

            for (int i = 0; i < last_resource_cost; i++)
            {
              if (p()->rng().roll(sh_proc_chance))
              {
                p()->buffs.soul_harvest->trigger();
              }
            }
          }

          p()->buffs.demonic_speed->trigger();
        }
      }

      void tick(dot_t* d) override
      {
        warlock_spell_t::tick(d);

        if (d->state->result > 0 && result_is_hit(d->state->result))
        {
          auto target_data = td(d->target);
          if (target_data->dots_seed_of_corruption->is_ticking() && id != p()->spells.seed_of_corruption_aoe->id)
          {
            accumulate_seed_of_corruption(target_data, d->state->result_amount);
          }
        }
      }

      void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        if (s->result_amount > 0 && result_is_hit(s->result))
        {
          auto td = this->td(s->target);
          if (td->dots_seed_of_corruption->is_ticking() && id != p()->spells.seed_of_corruption_aoe->id)
          {
            accumulate_seed_of_corruption(td, s->result_amount);
            if (sim->log)
              sim->out_log.printf("remaining damage to explode seed %f", td->soc_threshold);
          }
        }
      }

      virtual timespan_t get_db_dot_duration( dot_t* dot ) const
      { return dot->remains(); }

      virtual std::tuple<double, double> get_db_dot_state( dot_t* dot )
      {
        action_state_t* state = dot->current_action->get_state( dot->state );
        dot->current_action->calculate_tick_amount( state, 1.0 );

        timespan_t remaining = get_db_dot_duration( dot );

        timespan_t dot_tick_time = dot->current_action->tick_time( state );
        double ticks_left = ( remaining - dot->time_to_next_tick() ) / dot_tick_time;

        if ( ticks_left == 0.0 )
        {
          ticks_left += dot->time_to_next_tick() / dot_tick_time;
        }
        else
        {
          ticks_left += 1;
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

      static void accumulate_seed_of_corruption(warlock_td_t* td, double amount)
      {
        td->soc_threshold -= amount;

        if (td->soc_threshold <= 0)
        {
          td->dots_seed_of_corruption->cancel();
        }
      }
    };

    const std::array<int, MAX_UAS> ua_spells = { 233490, 233496, 233497, 233498, 233499 };

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
          return timespan_t::zero();
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

        if (p()->buffs.nightfall->check())
        {
          m *= 1.0 + p()->buffs.nightfall->default_value;
        }

        return m;
      }

      void execute() override
      {
        affliction_spell_t::execute();
        p()->buffs.nightfall->expire();
      }
    };

    // Dots
    struct agony_t : public affliction_spell_t
    {
      struct wracking_brilliance_t
      {
        wracking_brilliance_t()
        {

        }

        void run(warlock_t* p) {
          if (p->wracking_brilliance) {
            p->wracking_brilliance = false;
            p->buffs.wracking_brilliance->trigger();
          }
          else {
            p->wracking_brilliance = true;
          }
        }
      };

      double chance;
      wracking_brilliance_t* wb;

      agony_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( p, "Agony")
      {
        parse_options( options_str );
        may_crit = false;
        wb = new wracking_brilliance_t();

        dot_max_stack = data().max_stacks() + p->spec.agony_2->effectN(1).base_value();
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

        if ( p()->legendary.hood_of_eternal_disdain )
        {
          base_tick_time *= 1.0 + p()->legendary.hood_of_eternal_disdain->driver()->effectN( 2 ).percent();
          dot_duration *= 1.0 + p()->legendary.hood_of_eternal_disdain->driver()->effectN( 1 ).percent();
        }
      }

      void execute() override
      {
        affliction_spell_t::execute();

        if (p()->azerite.sudden_onset.ok() && td(execute_state->target)->dots_agony->current_stack() < (int)p()->azerite.sudden_onset.spell_ref().effectN(2).base_value())
        {
          td(execute_state->target)->dots_agony->increment((int)p()->azerite.sudden_onset.spell_ref().effectN(2).base_value() - td(execute_state->target)->dots_agony->current_stack());
        }
      }

      double bonus_ta(const action_state_t* s) const override
      {
        double ta = affliction_spell_t::bonus_ta(s);
        ta += p()->azerite.sudden_onset.value();
        return ta;
      }

      void tick( dot_t* d ) override
      {
        td(d->state->target)->dots_agony->increment(1);

        double increment_max = 0.368;
        
        double active_agonies = p()->get_active_dots( internal_id );
        increment_max *= std::pow(active_agonies, -2.0 / 3.0);
        
        if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T19, B4 ) )
        {
          increment_max *= 1.0 + p()->sets->set( WARLOCK_AFFLICTION, T19, B4 )->effectN( 1 ).percent();
        }

        if ( p()->talents.creeping_death->ok() )
        {
          increment_max *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
        }

        p()->agony_accumulator += rng().range( 0.0, increment_max );

        if ( p()->agony_accumulator >= 1 )
        {
          if ( p()->azerite.wracking_brilliance.ok() )
          {
            wb->run( p() );
          }

          p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony );
          p()->agony_accumulator -= 1.0;
        }

        if ( rng().roll( p()->sets->set( WARLOCK_AFFLICTION, T21, B2 )->proc_chance() ) )
        {
          warlock_td_t* target_data = td( d->state->target );
          for ( auto& current_ua : target_data->dots_unstable_affliction )
          {
            if ( current_ua->is_ticking() )
              current_ua->extend_duration( p()->sets->set( WARLOCK_AFFLICTION, T21, B2 )->effectN( 1 ).time_value(), true );
          }

          p()->procs.affliction_t21_2pc->occur();
        }

        affliction_spell_t::tick( d );
      }
    };

    struct corruption_t : public affliction_spell_t
    {
      corruption_t( warlock_t* p, const std::string& options_str) :
        affliction_spell_t( "Corruption", p, p -> find_spell(172) )  //triggers 146739
      {
        parse_options(options_str);
        may_crit = false;
        tick_zero = false;
        dot_duration = data().effectN( 1 ).trigger()->duration();
        spell_power_mod.tick = data().effectN( 1 ).trigger()->effectN( 1 ).sp_coeff();
        base_tick_time = data().effectN( 1 ).trigger()->effectN( 1 ).period();
        base_multiplier *= 1.0 + p->spec.affliction->effectN(2).percent();

        if ( p->talents.absolute_corruption->ok() )
        {
          dot_duration = sim->expected_iteration_time > timespan_t::zero() ?
            2 * sim->expected_iteration_time :
            2 * sim->max_time * ( 1.0 + sim->vary_combat_length ); // "infinite" duration
          base_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent();
        }
      }

      timespan_t get_db_dot_duration( dot_t* dot ) const override
      {
        if ( p()->talents.deathbolt->ok() )
        {
          return timespan_t::from_seconds( p()->talents.deathbolt->effectN( 3 ).base_value() );
        }
        else
        {
          return affliction_spell_t::get_db_dot_duration( dot );
        }
      }

      void init() override
      {
        affliction_spell_t::init();

        if ( p()->legendary.sacrolashs_dark_strike )
        {
          base_multiplier *= 1.0 + p()->legendary.sacrolashs_dark_strike->driver()->effectN( 1 ).percent();
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

        if (result_is_hit(d->state->result) && p()->sets->has_set_bonus(WARLOCK_AFFLICTION, T20, B2))
        {
          bool procced = p()->affliction_t20_2pc_rppm->trigger(); //check for RPPM

          if (procced)
            p()->resource_gain(RESOURCE_SOUL_SHARD, 1.0, p()->gains.affliction_t20_2pc); //trigger the buff
        }

        if (result_is_hit(d->state->result) && p()->azerite.inevitable_demise.ok())
        {
          p()->buffs.inevitable_demise->trigger();
        }

        affliction_spell_t::tick( d );
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
          if ( p->sets->has_set_bonus( WARLOCK_AFFLICTION, T19, B2 ) )
            base_multiplier *= 1.0 + p->sets->set( WARLOCK_AFFLICTION, T19, B2 )->effectN( 1 ).percent();
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
          p()->buffs.stretens_insanity->decrement( 1 );
          p()->buffs.active_uas->decrement( 1 );

          affliction_spell_t::last_tick( d );
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
        dot_duration = timespan_t::zero(); // DoT managed by ignite action.
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
          timespan_t min_duration = timespan_t::from_seconds( 100 );

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

      double bonus_ta(const action_state_t* s) const override
      {
        double ta = affliction_spell_t::bonus_ta(s);
        ta += p()->azerite.dreadful_calling.value(2);
        return ta;
      }

      void execute() override
      {
        affliction_spell_t::execute();

        if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T21, B4 ) )
          p()->active.tormented_agony->schedule_execute();

        if (p()->azerite.dreadful_calling.ok())
        {
          p()->cooldowns.darkglare->adjust((-1 * p()->azerite.dreadful_calling.spell_ref().effectN(1).time_value()));
        }

        bool flag = false;
        for ( int i = 0; i < MAX_UAS; i++ )
        {
          if ( td( target )->dots_unstable_affliction[i]->is_ticking() )
          {
            flag = true;
            break;
          }
        }

        if ( p()->legendary.power_cord_of_lethtendris )
        {
          if ( !flag && rng().roll( p()->legendary.power_cord_of_lethtendris->driver()->effectN( 1 ).percent() ) )
          {
            p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.power_cord_of_lethtendris );
          }
        }
        if ( p()->legendary.stretens_sleepless_shackles )
        {
          if ( !flag )
          {
            p()->buffs.stretens_insanity->increment( 1 );
          }
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
          if (td->dots_agony->is_ticking())
          {
            td->dots_agony->extend_duration(timespan_t::from_seconds(p()->spec.summon_darkglare->effectN(2).base_value()));
          }
          if (td->dots_corruption->is_ticking())
          {
            td->dots_corruption->extend_duration(timespan_t::from_seconds(p()->spec.summon_darkglare->effectN(2).base_value()));
          }
          if (td->dots_siphon_life->is_ticking())
          {
            td->dots_siphon_life->extend_duration(timespan_t::from_seconds(p()->spec.summon_darkglare->effectN(2).base_value()));
          }
          if (td->dots_phantom_singularity->is_ticking())
          {
            td->dots_phantom_singularity->extend_duration(timespan_t::from_seconds(p()->spec.summon_darkglare->effectN(2).base_value()));
          }
          if (td->dots_vile_taint->is_ticking())
          {
            td->dots_vile_taint->extend_duration(timespan_t::from_seconds(p()->spec.summon_darkglare->effectN(2).base_value()));
          }
          for (auto& current_ua : td->dots_unstable_affliction)
          {
            if (current_ua->is_ticking())
              current_ua->extend_duration(timespan_t::from_seconds(p()->spec.summon_darkglare->effectN(2).base_value()));
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
        bool deathbloom; //azerite_trait

        seed_of_corruption_aoe_t( warlock_t* p ) :
          affliction_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) ),
          corruption( new corruption_t(p,"") )
        {
          aoe = -1;
          background = true;
          deathbloom = false;
          p->spells.seed_of_corruption_aoe = this;
          base_costs[RESOURCE_MANA] = 0;

          corruption->background = true;
          corruption->dual = true;
          corruption->base_costs[RESOURCE_MANA] = 0;
        }

        double bonus_da(const action_state_t* s) const override
        {
          double da = affliction_spell_t::bonus_da(s);
          if(deathbloom)
            da += p()->azerite.deathbloom.value();
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
        if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T21, B4 ) )
          p()->active.tormented_agony->schedule_execute();

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
        if ( result_is_hit( s->result ) )
        {
          td( s->target )->soc_threshold = s->composite_spell_power();
        }

        affliction_spell_t::impact( s );
      }

      void last_tick( dot_t* d ) override
      {
        affliction_spell_t::last_tick( d );

        if (!d->end_event) {
          explosion->deathbloom = true;
        }
        explosion->set_target( d->target );
        explosion->schedule_execute();
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
      siphon_life_t(warlock_t* p, const std::string& options_str) :
        affliction_spell_t("siphon_life", p, p -> talents.siphon_life)
      {
        parse_options(options_str);
        may_crit = false;
      }

      double composite_target_multiplier(player_t* target) const override
      {
        double m = affliction_spell_t::composite_target_multiplier(target);

        auto td = this->td(target);

        if (td->debuffs_tormented_agony->check())
          m *= 1.0 + td->debuffs_tormented_agony->data().effectN(1).percent();

        return m;
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
      }
    };
    // lvl 75 - darkfury|mortal coil|demonic circle

    // lvl 90 - nightfall|deathbolt|grimoire of sacrifice
    struct deathbolt_t : public affliction_spell_t
    {
      timespan_t ac_max;

      deathbolt_t(warlock_t* p, const std::string& options_str) :
        affliction_spell_t("deathbolt", p, p -> talents.deathbolt)
      {
        parse_options(options_str);
        ac_max = timespan_t::from_seconds(data().effectN(3).base_value());
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

        double total_damage_agony = get_contribution_from_dot(td->dots_agony);
        double total_damage_corruption = get_contribution_from_dot(td->dots_corruption);

        double total_damage_siphon_life = 0.0;
        if (p()->talents.siphon_life->ok())
          total_damage_siphon_life = get_contribution_from_dot(td->dots_siphon_life);

        double total_damage_phantom_singularity = 0.0;
        if ( p()->talents.phantom_singularity->ok() )
          total_damage_phantom_singularity = get_contribution_from_dot( td->dots_phantom_singularity );

        double total_damage_vile_taint = 0.0;
        if ( p()->talents.vile_taint->ok() )
          total_damage_vile_taint = get_contribution_from_dot( td->dots_vile_taint );

        double total_damage_unstable_afflictions = 0.0;
        for (auto& current_ua : td->dots_unstable_affliction)
        {
          total_damage_unstable_afflictions += get_contribution_from_dot(current_ua);
        }

        const double total_dot_dmg = total_damage_agony + total_damage_corruption + total_damage_siphon_life +
                     total_damage_unstable_afflictions + total_damage_phantom_singularity + total_damage_vile_taint;

        this->base_dd_min = this->base_dd_max = (total_dot_dmg * data().effectN(2).percent());

        if (sim->debug) {
          sim->out_debug.printf("%s deathbolt damage_remaining=%.3f", name(), total_dot_dmg);
        }

        affliction_spell_t::execute();
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

    // Tier
    struct tormented_agony_t : public affliction_spell_t
    {
      struct tormented_agony_debuff_engine_t : public affliction_spell_t
      {
        tormented_agony_debuff_engine_t( warlock_t* p ) :
          affliction_spell_t( "tormented agony", p, p -> find_spell( 256807 ) )
        {
          harmful = may_crit = callbacks = false;
          background = proc = true;
          aoe = 0;
          trigger_gcd = timespan_t::zero();
        }

        void impact( action_state_t* s ) override
        {
          affliction_spell_t::impact( s );

          td( s->target )->debuffs_tormented_agony->trigger();
        }
      };

      propagate_const<player_t*> source_target;
      tormented_agony_debuff_engine_t* tormented_agony;

      tormented_agony_t( warlock_t* p ) :
        affliction_spell_t( "tormented agony", p, p -> find_spell( 256807 ) ),
        source_target( nullptr ),
        tormented_agony( new tormented_agony_debuff_engine_t( p ) )
      {
        harmful = may_crit = callbacks = false;
        background = proc = true;
        aoe = -1;
        radius = data().effectN( 1 ).radius();
        trigger_gcd = timespan_t::zero();
      }

      void execute() override
      {
        affliction_spell_t::execute();

        for ( const auto target : sim->target_non_sleeping_list )
        {
          auto td = this->td(target);
          if ( td->dots_agony->is_ticking() )
          {
            tormented_agony->set_target( target );
            tormented_agony->execute();
          }
        }
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
    //tier
    buffs.demonic_speed =
        make_buff( this, "demonic_speed", sets->set( WARLOCK_AFFLICTION, T20, B4 )->effectN( 1 ).trigger() )
      ->set_chance( sets->set( WARLOCK_AFFLICTION, T20, B4 )->proc_chance() )
      ->set_default_value( sets->set( WARLOCK_AFFLICTION, T20, B4 )->effectN( 1 ).trigger()->effectN( 1 ).percent() )
      ->add_invalidate(CACHE_HASTE);
    buffs.inevitable_demise = make_buff(this, "inevitable_demise", azerite.inevitable_demise)
      ->set_max_stack(find_spell(273525)->max_stacks())
      ->set_default_value(azerite.inevitable_demise.value());
    //legendary
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
    // Tier
    active.tormented_agony              = new tormented_agony_t( this );
    // Azerite
    azerite.cascading_calamity          = find_azerite_spell("Cascading Calamity");
    azerite.dreadful_calling            = find_azerite_spell("Dreadful Calling");
    azerite.inevitable_demise           = find_azerite_spell("Inevitable Demise");
    azerite.sudden_onset                = find_azerite_spell("Sudden Onset");
    azerite.wracking_brilliance         = find_azerite_spell("Wracking Brilliance");
    azerite.deathbloom                  = find_azerite_spell("Deathbloom");
  }

  void warlock_t::init_gains_affliction()
  {
    gains.agony                         = get_gain( "agony" );
    gains.seed_of_corruption            = get_gain( "seed_of_corruption" );
    gains.unstable_affliction_refund    = get_gain( "unstable_affliction_refund" );
    gains.affliction_t20_2pc            = get_gain( "affliction_t20_2pc" );
  }

  void warlock_t::init_rng_affliction()
  {
    affliction_t20_2pc_rppm             = get_rppm( "affliction_t20_2pc", sets->set( WARLOCK_AFFLICTION, T20, B2 ) );
  }

  void warlock_t::init_procs_affliction()
  {
    procs.affliction_t21_2pc            = get_proc( "affliction_t21_2pc" );
    procs.nightfall                     = get_proc( "nightfall" );
  }

  void warlock_t::create_apl_affliction()
  {
    action_priority_list_t* def = get_action_priority_list( "default" );
    action_priority_list_t* fil = get_action_priority_list( "fillers" );

    def->add_action( "variable,name=spammable_seed,value=talent.sow_the_seeds.enabled&spell_targets.seed_of_corruption_aoe>=3|talent.siphon_life.enabled&spell_targets.seed_of_corruption>=5|spell_targets.seed_of_corruption>=8" );
    def->add_action( "variable,name=padding,op=set,value=action.shadow_bolt.execute_time*azerite.cascading_calamity.enabled" );
    def->add_action( "variable,name=padding,op=reset,value=gcd,if=azerite.cascading_calamity.enabled&(talent.drain_soul.enabled|talent.deathbolt.enabled&cooldown.deathbolt.remains<=gcd)" );
    def->add_action( "potion,if=(talent.dark_soul_misery.enabled&cooldown.summon_darkglare.up&cooldown.dark_soul.up)|cooldown.summon_darkglare.up|target.time_to_die<30" );
    def->add_action( "use_items,if=!cooldown.summon_darkglare.up" );
    def->add_action( "fireblood,if=!cooldown.summon_darkglare.up" );
    def->add_action( "blood_fury,if=!cooldown.summon_darkglare.up" );
    def->add_action( "drain_soul,interrupt_global=1,chain=1,cycle_targets=1,if=target.time_to_die<=gcd&soul_shard<5" );
    def->add_action( "haunt" );
    def->add_action( "summon_darkglare,if=dot.agony.ticking&dot.corruption.ticking&(buff.active_uas.stack=5|soul_shard=0)&(!talent.phantom_singularity.enabled|cooldown.phantom_singularity.remains)" );
    def->add_action( "agony,cycle_targets=1,if=remains<=gcd" );
    def->add_action( "phantom_singularity,if=time>40" );
    def->add_action( "vile_taint,if=time>20" );
    def->add_action( "seed_of_corruption,if=dot.corruption.remains<=action.seed_of_corruption.cast_time+time_to_shard+4.2*(1-talent.creeping_death.enabled*0.15)&spell_targets.seed_of_corruption_aoe>=3+talent.writhe_in_agony.enabled&!dot.seed_of_corruption.remains&!action.seed_of_corruption.in_flight" );
    def->add_action( "agony,cycle_targets=1,max_cycle_targets=6,if=talent.creeping_death.enabled&target.time_to_die>10&(remains<=gcd|(refreshable&!(cooldown.summon_darkglare.remains<=soul_shard*cast_time)))" );
    def->add_action( "agony,cycle_targets=1,max_cycle_targets=8,if=(!talent.creeping_death.enabled)&target.time_to_die>10&(remains<=gcd|(refreshable&!(cooldown.summon_darkglare.remains<=soul_shard*cast_time)))" );
    def->add_action( "siphon_life,cycle_targets=1,max_cycle_targets=1,if=refreshable&target.time_to_die>10&((!(cooldown.summon_darkglare.remains<=soul_shard*cast_time)&active_enemies>8)|active_enemies<2)" );
    def->add_action( "siphon_life,cycle_targets=1,max_cycle_targets=2,if=refreshable&target.time_to_die>10&!(cooldown.summon_darkglare.remains<=soul_shard*cast_time)&active_enemies=7" );
    def->add_action( "siphon_life,cycle_targets=1,max_cycle_targets=3,if=refreshable&target.time_to_die>10&!(cooldown.summon_darkglare.remains<=soul_shard*cast_time)&active_enemies=6" );
    def->add_action( "siphon_life,cycle_targets=1,max_cycle_targets=4,if=refreshable&target.time_to_die>10&!(cooldown.summon_darkglare.remains<=soul_shard*cast_time)&active_enemies=5" );
    def->add_action( "corruption,cycle_targets=1,if=active_enemies<3&refreshable&target.time_to_die>10" );
    def->add_action( "dark_soul" );
    def->add_action( "vile_taint" );
    def->add_action( "berserking" );
    def->add_action( "unstable_affliction,if=soul_shard>=5" );
    def->add_action( "unstable_affliction,if=cooldown.summon_darkglare.remains<=soul_shard*cast_time" );
    def->add_action( "phantom_singularity" );
    def->add_action( "call_action_list,name=fillers,if=(cooldown.summon_darkglare.remains<time_to_shard*(5-soul_shard)|cooldown.summon_darkglare.up)&time_to_die>cooldown.summon_darkglare.remains" );
    def->add_action( "seed_of_corruption,if=variable.spammable_seed" );
    def->add_action( "unstable_affliction,if=!prev_gcd.1.summon_darkglare&!variable.spammable_seed&(talent.deathbolt.enabled&cooldown.deathbolt.remains<=execute_time&!azerite.cascading_calamity.enabled|soul_shard>=2&target.time_to_die>4+cast_time&active_enemies=1|target.time_to_die<=8+cast_time*soul_shard)" );
    def->add_action( "unstable_affliction,if=!variable.spammable_seed&contagion<=cast_time+variable.padding" );
    def->add_action( "unstable_affliction,cycle_targets=1,if=!variable.spammable_seed&(!talent.deathbolt.enabled|cooldown.deathbolt.remains>time_to_shard|soul_shard>1)&contagion<=cast_time+variable.padding" );
    def->add_action( "call_action_list,name=fillers" );

    fil->add_action( "deathbolt" );
    fil->add_action( "shadow_bolt,if=buff.movement.up&buff.nightfall.remains" );
    fil->add_action( "agony,if=buff.movement.up&!(talent.siphon_life.enabled&(prev_gcd.1.agony&prev_gcd.2.agony&prev_gcd.3.agony)|prev_gcd.1.agony)" );
    fil->add_action( "siphon_life,if=buff.movement.up&!(prev_gcd.1.siphon_life&prev_gcd.2.siphon_life&prev_gcd.3.siphon_life)" );
    fil->add_action( "corruption,if=buff.movement.up&!prev_gcd.1.corruption&!talent.absolute_corruption.enabled" );
    fil->add_action( "drain_life,if=(buff.inevitable_demise.stack=100|buff.inevitable_demise.stack>60&target.time_to_die<=10)&(target.health.pct>=20|!talent.drain_soul.enabled)" );
    fil->add_action( "drain_soul,interrupt_global=1,chain=1,cycle_targets=1,if=target.time_to_die<=gcd" );
    fil->add_action( "drain_soul,interrupt_global=1,chain=1" );
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

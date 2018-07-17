#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock
{
  namespace actions_affliction
  {
    using namespace actions;

    struct affliction_spell_t : public warlock_spell_t
    {
    public:
      gain_t * gain;

      bool affected_by_deaths_embrace;

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

        parse_spell_coefficient(*this);
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

          if (p()->talents.soul_conduit->ok())
          {
            double soul_conduit_rng = p()->talents.soul_conduit->effectN(1).percent();

            for (int i = 0; i < last_resource_cost; i++)
            {
              if (rng().roll(soul_conduit_rng))
              {
                p()->resource_gain(RESOURCE_SOUL_SHARD, 1.0, p()->gains.soul_conduit);
                p()->procs.soul_conduit->occur();
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
          if (auto td = find_td(d->target))
          {
            if (td->dots_seed_of_corruption->is_ticking() && id != p()->spells.seed_of_corruption_aoe->id)
            {
              accumulate_seed_of_corruption(td, d->state->result_amount);
            }
          }
        }
      }

      void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        if (s->result_amount > 0 && result_is_hit(s->result))
        {
          if (auto td = find_td(s->target))
          {
            if (td->dots_seed_of_corruption->is_ticking() && id != p()->spells.seed_of_corruption_aoe->id)
            {
              accumulate_seed_of_corruption(td, s->result_amount);
            }
          }
        }
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
        if (p()->talents.drain_soul->ok())
        {
          return false;
        }

        return spell_t::ready();
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

      int agony_action_id;
      int agony_max_stacks;
      double chance;
      wracking_brilliance_t* wb;

      agony_t( warlock_t* p, const std::string& options_str ) :
        affliction_spell_t( p, "Agony"),
        agony_action_id( 0 ),
        agony_max_stacks( 0 )
      {
        parse_options( options_str );
        may_crit = false;
        affected_by_deaths_embrace = true;
        wb = new wracking_brilliance_t();
      }

      double action_multiplier() const override
      {
        double m = affliction_spell_t::action_multiplier();

        if ( p()->mastery_spells.potent_afflictions->ok() )
          m *= 1.0 + p()->cache.mastery_value();

        return m;
      }

      void last_tick( dot_t* d ) override
      {
        if ( p()->get_active_dots( internal_id ) == 1 )
        {
          p()->agony_accumulator = rng().range( 0.0, 0.99 );
          p()->agony_expression_thing = 0.0;
        }

        affliction_spell_t::last_tick( d );
      }

      void init() override
      {
        agony_max_stacks = ( p()->talents.writhe_in_agony->ok() ? p()->talents.writhe_in_agony->effectN( 2 ).base_value() : 10 );

        affliction_spell_t::init();

        this->dot_max_stack = agony_max_stacks;

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
        dot_t* agony = td( d->state->target )->dots_agony;
        timespan_t dot_tick_time = agony->current_action->tick_time( agony->current_action->get_state( agony->state ) );

        td(d->state->target)->dots_agony->increment(1);

        double tier_bonus = 1.0 + p()->sets->set( WARLOCK_AFFLICTION, T19, B4 )->effectN( 1 ).percent();
        double active_agonies = p()->get_active_dots( internal_id );
        double accumulator_increment = rng().range( 0.0, p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T19, B4 ) ? 0.368 * tier_bonus : 0.32 ) / std::sqrt( active_agonies );
        p()->agony_expression_thing = 1 / ( 0.16 / std::sqrt( active_agonies ) * ( active_agonies == 1 ? 1.15 : 1.0 ) * active_agonies / dot_tick_time.total_seconds() );

        if (active_agonies == 1)
        {
          accumulator_increment *= 1.15;
        }

        p()->agony_accumulator += accumulator_increment;

        if ( p()->agony_accumulator >= 1 )
       { 
          if (p()->azerite.wracking_brilliance.ok())
            wb->run(p());
          p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony );
          p()->agony_accumulator -= 1.0;

          if ( p()->resources.current[RESOURCE_SOUL_SHARD] == 1 )
            p()->shard_react = p()->sim->current_time() + p()->total_reaction_time();

          else if ( p()->resources.current[RESOURCE_SOUL_SHARD] >= 1 )
            p()->shard_react = p()->sim->current_time();

          else
            p()->shard_react = timespan_t::max();
        }

        if ( rng().roll( p()->sets->set( WARLOCK_AFFLICTION, T21, B2 )->proc_chance() ) )
        {
          if (warlock_td_t* target_data = find_td( d->state->target ))
          {
            for ( auto& current_ua : target_data->dots_unstable_affliction )
            {
              if ( current_ua->is_ticking() )
                current_ua->extend_duration( p()->sets->set( WARLOCK_AFFLICTION, T21, B2 )->effectN( 1 ).time_value(), true );
            }
          }

          p()->procs.affliction_t21_2pc->occur();
        }

        affliction_spell_t::tick( d );
      }
    };

    struct corruption_t : public affliction_spell_t
    {
      corruption_t( warlock_t* p, const std::string& options_str) :
        affliction_spell_t( "Corruption", p, p -> find_spell( 172 ) )
      {
        parse_options(options_str);
        may_crit = false;
        affected_by_deaths_embrace = true;
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

      void init() override
      {
        affliction_spell_t::init();

        if ( p()->legendary.sacrolashs_dark_strike )
        {
          base_multiplier *= 1.0 + p()->legendary.sacrolashs_dark_strike->driver()->effectN( 1 ).percent();
        }
      }

      double action_multiplier() const override
      {
        double m = affliction_spell_t::action_multiplier();

        if ( p()->mastery_spells.potent_afflictions->ok() )
          m *= 1.0 + p()->cache.mastery_value();

        return m;
      }

      void tick( dot_t* d ) override
      {
        if ( result_is_hit( d->state->result ) && p()->talents.nightfall->ok() )
        {
          auto success = p()->buffs.nightfall->trigger();
          if ( success )
          {
            p()->procs.nightfall->occur();
          }
        }

        if ( result_is_hit( d->state->result ) && p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T20, B2 ) )
        {
          bool procced = p()->affliction_t20_2pc_rppm->trigger(); //check for RPPM

          if ( procced )
            p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.affliction_t20_2pc ); //trigger the buff
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
          affected_by_deaths_embrace = true;
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

        double action_multiplier() const override
        {
          double m = affliction_spell_t::action_multiplier();

          if ( p()->mastery_spells.potent_afflictions->ok() )
            m *= 1.0 + p()->cache.mastery_value();

          return m;
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

          auto td = find_td( s->target );
          for ( int i = 0; i < MAX_UAS; i++ )
          {
            if ( ! td || !td->dots_unstable_affliction[i]->is_ticking() )
            {
              real_ua = ua_dots[i];
              p()->buffs.active_uas->increment( 1 );
              break;
            }

            timespan_t rem = td->dots_unstable_affliction[i]->remains();

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
              if (td && td->dots_unstable_affliction[i]->is_ticking())
              {
                p()->buffs.cascading_calamity->trigger();
                break;
              }
            }
          }

          real_ua->target = s->target;
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
          auto td = find_td(target);
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
        bool deathbloom; //azerite_trait
        seed_of_corruption_aoe_t( warlock_t* p ) :
          affliction_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) )
        {
          aoe = -1;
          dual = true;
          background = true;
          deathbloom = false;
          affected_by_deaths_embrace = true;
          p->spells.seed_of_corruption_aoe = this;
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
            if (warlock_td_t* tdata = find_td( s->target ))
            {
              if ( tdata->dots_seed_of_corruption->is_ticking() && tdata->soc_threshold > 0 )
              {
                tdata->soc_threshold = 0;
                tdata->dots_seed_of_corruption->cancel();
              }
            }
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
        base_tick_time = dot_duration;
        hasted_ticks = false;
        add_child( explosion );
      }

      void init() override
      {
        affliction_spell_t::init();
        snapshot_flags |= STATE_SP;
      }

      void execute() override
      {
        if ( p()->talents.sow_the_seeds->ok() )
        {
          aoe = 2;
        }

        if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T21, B4 ) )
          p()->active.tormented_agony->schedule_execute();

        affliction_spell_t::execute();
      }

      void impact( action_state_t* s ) override
      {
        if ( result_is_hit( s->result ) )
        {
          td( s->target )->soc_threshold = s->composite_spell_power();
        }

        assert(p()->active.corruption);
        p()->active.corruption->target = s->target;
        p()->active.corruption->schedule_execute();

        affliction_spell_t::impact( s );
      }

      void last_tick( dot_t* d ) override
      {
        affliction_spell_t::last_tick( d );

        if (!d->end_event) {
          explosion->deathbloom = true;
        }
        explosion->target = d->target;
        explosion->execute();
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
        affected_by_deaths_embrace = true;
      }

      double action_multiplier() const override
      {
        double m = affliction_spell_t::action_multiplier();

        if (p()->mastery_spells.potent_afflictions->ok())
          m *= 1.0 + p()->cache.mastery_value();

        return m;
      }

      double composite_target_multiplier(player_t* target) const override
      {
        double m = affliction_spell_t::composite_target_multiplier(target);

        if (auto td = find_td(target))
        {
          if (td->debuffs_tormented_agony->check())
            m *= 1.0 + td->debuffs_tormented_agony->data().effectN(1).percent();
        }

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
        affected_by_deaths_embrace = true;
        aoe = -1;
      }
    };

    struct phantom_singularity_t : public affliction_spell_t
    {
      phantom_singularity_tick_t* phantom_singularity;

      phantom_singularity_t( warlock_t* p, const std::string& options_str ) : affliction_spell_t( "phantom_singularity", p, p -> talents.phantom_singularity )
      {
        parse_options( options_str );
        callbacks = false;
        hasted_ticks = true;
        phantom_singularity = new phantom_singularity_tick_t( p );
        add_child( phantom_singularity );
      }

      timespan_t composite_dot_duration( const action_state_t* s ) const override
      {
        return s->action->tick_time( s ) * 8.0;
      }

      void tick( dot_t* d ) override
      {
        phantom_singularity->execute();

        affliction_spell_t::tick( d );
      }
    };

    struct vile_taint_damage_t : public affliction_spell_t
    {
      vile_taint_damage_t(warlock_t* p)
        : affliction_spell_t("vile_taint_damage", p, p -> talents.vile_taint)
      {
        aoe = -1;
        background = dual = ground_aoe = true;
        hasted_ticks = false;
      }

      double cost() const override
      {
        return 0.0;
      }

      dot_t* get_dot(player_t* t)
      {
        if (!t) t = target;
        if (!t) return nullptr;

        return td(t)->dots_vile_taint;
      }

      void make_ground_aoe_event(warlock_t* p, action_state_t* execute_state)
      {
        make_event<ground_aoe_event_t>(*sim, p, ground_aoe_params_t()
          .target(execute_state->target)
          .x(p->talents.vile_taint->ok() ? p->x_position : execute_state->target->x_position)
          .y(p->talents.vile_taint->ok() ? p->y_position : execute_state->target->y_position)
          .pulse_time(p->talents.vile_taint->effectN(1).period())
          .duration(p->talents.vile_taint->duration())
          .start_time(sim->current_time())
          .action(this));
      }
    };

    struct vile_taint_t : public affliction_spell_t
    {
      vile_taint_damage_t* damage;

      vile_taint_t(warlock_t* p, const std::string& options_str)
        : affliction_spell_t("vile_taint", p, p -> talents.vile_taint)
      {
        parse_options(options_str);
        may_miss = may_crit = false;
        damage = new vile_taint_damage_t(p);
        damage->stats = stats;
      }

      // Don't record data for this action.
      void record_data(action_state_t* s) override
      {
        (void)s;
      }

      void execute() override
      {
        affliction_spell_t::execute();
        damage->make_ground_aoe_event(p(), execute_state);
      }

      expr_t* create_expression(const std::string& name) override
      {
        return affliction_spell_t::create_expression(name);
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

      double get_contribution_from_dot(dot_t* dot)
      {
        if (!(dot->is_ticking()))
          return 0.0;

        action_state_t* state = dot->current_action->get_state(dot->state);
        dot->current_action->calculate_tick_amount(state, 1.0);
        double tick_base_damage = state->result_raw;
        timespan_t remaining = dot->remains();

        if (dot->duration() > sim->expected_iteration_time)
          remaining = ac_max;

        timespan_t dot_tick_time = dot->current_action->tick_time(state);
        double ticks_left = (remaining - dot->time_to_next_tick()) / dot_tick_time;

        if (ticks_left == 0.0)
        {
          ticks_left += dot->time_to_next_tick() / dot->current_action->tick_time(state);
        }
        else
        {
          ticks_left += 1;
        }

        double total_damage = ticks_left * tick_base_damage;

        if (sim->debug)
        {
          sim->out_debug.printf("%s %s dot_remains=%.3f duration=%.3f time_to_next=%.3f tick_time=%.3f ticks_left=%.3f amount=%.3f total=%.3f",
            name(), dot->name(), dot->remains().total_seconds(), dot->duration().total_seconds(),
            dot->time_to_next_tick().total_seconds(), dot_tick_time.total_seconds(),
            ticks_left, tick_base_damage, total_damage);
        }

        action_state_t::release(state);
        return total_damage;
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
          if ( auto td = find_td(target) )
          {
            if ( td->dots_agony->is_ticking() )
            {
              tormented_agony->set_target( target );
              tormented_agony->execute();
            }
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

    // seed applies corruption
    if (specialization() == WARLOCK_AFFLICTION)
    {
      active.corruption = new corruption_t(this, "");
      active.corruption->background = true;
      active.corruption->aoe = -1;
    }
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
    //procs.the_master_harvester          = get_proc( "the_master_harvester" );
    procs.affliction_t21_2pc            = get_proc( "affliction_t21_2pc" );
    procs.nightfall                     = get_proc( "nightfall" );
  }

  void warlock_t::create_options_affliction()
  {
    add_option( opt_bool( "deaths_embrace_fixed_time", deaths_embrace_fixed_time ) );
  }

  void warlock_t::create_apl_affliction()
  {
    action_priority_list_t* def = get_action_priority_list( "default" );

    def->add_action( "dark_soul,if=buff.active_uas.stack>0" );
    def->add_action( "haunt" );
    def->add_action( "agony,if=refreshable" );
    def->add_action( "siphon_life,if=refreshable" );
    def->add_action( "corruption,if=refreshable" );
    def->add_action( "phantom_singularity" );
    def->add_action( "vile_taint" );
    def->add_action( "unstable_affliction,if=soul_shard=5" );
    def->add_action( "unstable_affliction,if=(dot.unstable_affliction_1.ticking+dot.unstable_affliction_2.ticking+dot.unstable_affliction_3.ticking+dot.unstable_affliction_4.ticking+dot.unstable_affliction_5.ticking=0)|soul_shard>2" );
    def->add_action( "summon_darkglare" );
    def->add_action( "deathbolt" );
    def->add_talent( this, "Drain Soul", "chain=1,interrupt=1" );
    def->add_action( "shadow_bolt" );
  }
}

#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
    #define MAX_UAS 5
    namespace actions {
        const int ua_spells[5] = { 233490, 233496, 233497, 233498, 233499 };
        // Dots
        struct agony_t : public warlock_spell_t {
            int agony_action_id;
            double chance;

            agony_t(warlock_t* p, const std::string& options_str) : warlock_spell_t(p, "Agony"), agony_action_id(0) {
                parse_options(options_str);
                may_crit = false;
                affected_by_deaths_embrace = true;
                chance = p->find_spell(199282)->proc_chance();
            }

            void init() override { warlock_spell_t::init(); }

            virtual double action_multiplier() const override {
                double m = warlock_spell_t::action_multiplier();
                if (p()->mastery_spells.potent_afflictions->ok())
                    m *= 1.0 + p()->cache.mastery_value();
                return m;
            }
            double composite_crit_chance() const override {
                double cc = warlock_spell_t::composite_crit_chance();
                return cc;
            }
            double composite_crit_damage_bonus_multiplier() const override {
                double cd = warlock_spell_t::composite_crit_damage_bonus_multiplier();
                return cd;
            }
            virtual double composite_target_multiplier(player_t* target) const override {
                double m = warlock_spell_t::composite_target_multiplier(target);
                warlock_td_t* td = this->td(target);
                m *= td->agony_stack;
                return m;
            }
            virtual void last_tick(dot_t* d) override {
                td(d->state->target)->agony_stack = 1;
                td(d->state->target)->debuffs_agony->expire();
                if (p()->get_active_dots(internal_id) == 1)
                    p()->agony_accumulator = rng().range(0.0, 0.99);
                warlock_spell_t::last_tick(d);
            }
            virtual void execute() override{
                warlock_spell_t::execute();
                td(execute_state->target)->debuffs_agony->trigger();
            }
            virtual void tick(dot_t* d) override {
                int agony_max_stacks;
                agony_max_stacks = (p()->talents.writhe_in_agony->ok() ? p()->talents.writhe_in_agony->effectN(2).base_value() : 10);
                if (td(d->state->target)->agony_stack < agony_max_stacks)
                    td(d->state->target)->agony_stack++;

                td(d->target)->debuffs_agony->trigger();

                double tier_bonus = 1.0 + p()->sets->set(WARLOCK_AFFLICTION, T19, B4)->effectN(1).percent();

                double active_agonies = p()->get_active_dots(internal_id);
                double accumulator_increment = rng().range(0.0, p()->sets->has_set_bonus(WARLOCK_AFFLICTION, T19, B4) ? 0.32 * tier_bonus : 0.32) / sqrt(active_agonies);

                p()->agony_accumulator += accumulator_increment;

                if (p()->agony_accumulator >= 1) {
                    p()->resource_gain(RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony);
                    p()->agony_accumulator -= 1.0;
                    if (p()->resources.current[RESOURCE_SOUL_SHARD] == 1)
                        p()->shard_react = p()->sim->current_time() + p()->total_reaction_time();
                    else if (p()->resources.current[RESOURCE_SOUL_SHARD] >= 1)
                        p()->shard_react = p()->sim->current_time();
                    else
                        p()->shard_react = timespan_t::max();
                }

                if (rng().roll(p()->sets->set(WARLOCK_AFFLICTION, T21, B2)->proc_chance())) {
                    warlock_td_t* target_data = td(d->state->target);
                    for (int i = 0; i < MAX_UAS; i++) {
                        auto current_ua = target_data->dots_unstable_affliction[i];
                        if (current_ua->is_ticking())
                            current_ua->extend_duration(p()->sets->set(WARLOCK_AFFLICTION, T21, B2)->effectN(1).time_value(), true);
                    }
                    p()->procs.affliction_t21_2pc->occur();
                }

                warlock_spell_t::tick(d);
            }
        };
        struct corruption_t : public warlock_spell_t {
            double chance;

            corruption_t(warlock_t* p) : warlock_spell_t("Corruption", p, p -> find_spell(172)) {
                may_crit = false;
                affected_by_deaths_embrace = true;
                dot_duration = data().effectN(1).trigger()->duration();
                spell_power_mod.tick = data().effectN(1).trigger()->effectN(1).sp_coeff();
                base_tick_time = data().effectN(1).trigger()->effectN(1).period();
                base_multiplier *= 1.0 + p->spec.affliction->effectN(2).percent();

                if (p->talents.absolute_corruption->ok()) {
                    dot_duration = sim->expected_iteration_time > timespan_t::zero() ?
                        2 * sim->expected_iteration_time :
                        2 * sim->max_time * (1.0 + sim->vary_combat_length); // "infinite" duration
                    base_multiplier *= 1.0 + p->talents.absolute_corruption->effectN(2).percent();
                }

                chance = p->find_spell(199282)->proc_chance();
            }

            corruption_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("Corruption", p, p -> find_spell(172)) {
                parse_options(options_str);
                may_crit = false;
                affected_by_deaths_embrace = true;
                dot_duration = data().effectN(1).trigger()->duration();
                spell_power_mod.tick = data().effectN(1).trigger()->effectN(1).sp_coeff();
                base_tick_time = data().effectN(1).trigger()->effectN(1).period();
                base_multiplier *= 1.0 + p->spec.affliction->effectN(2).percent();

                if (p->talents.absolute_corruption->ok()) {
                    dot_duration = sim->expected_iteration_time > timespan_t::zero() ?
                        2 * sim->expected_iteration_time :
                        2 * sim->max_time * (1.0 + sim->vary_combat_length); // "infinite" duration
                    base_multiplier *= 1.0 + p->talents.absolute_corruption->effectN(2).percent();
                }

                chance = p->find_spell(199282)->proc_chance();
            }

            void init() override { warlock_spell_t::init(); }

            virtual double action_multiplier() const override {
                double m = warlock_spell_t::action_multiplier();
                if (p()->mastery_spells.potent_afflictions->ok())
                    m *= 1.0 + p()->cache.mastery_value();

                return m;
            }

            double composite_crit_chance() const override {
                double cc = warlock_spell_t::composite_crit_chance();
                return cc;
            }
            double composite_crit_damage_bonus_multiplier() const override {
                double cd = warlock_spell_t::composite_crit_damage_bonus_multiplier();
                return cd;
            }
            virtual double composite_target_multiplier(player_t* target) const override {
                double m = warlock_spell_t::composite_target_multiplier(target);
                warlock_td_t* td = this->td(target);

                return m;
            }
            virtual void tick(dot_t* d) override {
              if (result_is_hit(d->state->result) && p()->talents.nightfall->ok()) {
                bool procced = p()->nightfall_rppm->trigger(); //check for RPPM
                if (procced) {
                  p()->buffs.nightfall->trigger();
                  p()->procs.nightfall->occur();
                }
              }
              if (result_is_hit(d->state->result) && p()->sets->has_set_bonus(WARLOCK_AFFLICTION, T20, B2)) {
                  bool procced = p()->affliction_t20_2pc_rppm->trigger(); //check for RPPM
                  if (procced)
                      p()->resource_gain(RESOURCE_SOUL_SHARD, 1.0, p()->gains.affliction_t20_2pc); //trigger the buff
              }

              warlock_spell_t::tick(d);
            }
        };
        struct unstable_affliction_t : public warlock_spell_t {
            struct real_ua_t : public warlock_spell_t
            {
                int self;
                real_ua_t(warlock_t* p, int num) : warlock_spell_t("unstable_affliction_" + std::to_string(num + 1), p, p -> find_spell(ua_spells[num])), self(num) {
                    background = true;
                    dual = true;
                    tick_may_crit = hasted_ticks = true;
                    affected_by_deaths_embrace = true;
                    if (p->sets->has_set_bonus(WARLOCK_AFFLICTION, T19, B2))
                        base_multiplier *= 1.0 + p->sets->set(WARLOCK_AFFLICTION, T19, B2)->effectN(1).percent();
                }

                timespan_t composite_dot_duration(const action_state_t* s) const override {
                    return s->action->tick_time(s) * 4.0;
                }
                void init() override {
                    warlock_spell_t::init();
                    update_flags &= ~STATE_HASTE;
                }
                void tick(dot_t* d) override {
                    warlock_spell_t::tick(d);
                }
                void last_tick(dot_t* d) override {
                    p()->buffs.active_uas->decrement(1);
                    warlock_spell_t::last_tick(d);
                }
                double composite_crit_chance() const override {
                    double cc = warlock_spell_t::composite_crit_chance();
                    return cc;
                }
                double composite_crit_damage_bonus_multiplier() const override {
                    double cd = warlock_spell_t::composite_crit_damage_bonus_multiplier();
                    return cd;
                }
                virtual double composite_target_multiplier(player_t* target) const override {
                    double m = warlock_spell_t::composite_target_multiplier(target);
                    warlock_td_t* td = this->td(target);

                    return m;
                }
                virtual double action_multiplier() const override {
                    double m = warlock_spell_t::action_multiplier();
                    if (p()->mastery_spells.potent_afflictions->ok())
                        m *= 1.0 + p()->cache.mastery_value();

                    return m;
                }
            };
            real_ua_t* ua_dots[MAX_UAS];
            unstable_affliction_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("unstable_affliction", p, p -> spec.unstable_affliction) {
                parse_options(options_str);
                for (int i = 0; i < MAX_UAS; i++) {
                    ua_dots[i] = new real_ua_t(p, i);
                    add_child(ua_dots[i]);
                }
                const spell_data_t* ptr_spell = p->find_spell(233490);
                spell_power_mod.direct = ptr_spell->effectN(1).sp_coeff();
                dot_duration = timespan_t::zero(); // DoT managed by ignite action.
            }

            double cost() const override {
                double c = warlock_spell_t::cost();
                return c;
            }
            void init() override {
                warlock_spell_t::init();
                snapshot_flags &= ~(STATE_CRIT | STATE_TGT_CRIT);
            }
            virtual void impact(action_state_t* s) override {
                if (result_is_hit(s->result)) {
                    real_ua_t* real_ua = nullptr;
                    timespan_t min_duration = timespan_t::from_seconds(100);
                    for (int i = 0; i < MAX_UAS; i++)
                    {
                        dot_t* curr_ua = td(s->target)->dots_unstable_affliction[i];
                        if (!(curr_ua->is_ticking())) {
                            real_ua = ua_dots[i];
                            p()->buffs.active_uas->increment(1);
                            break;
                        }

                        timespan_t rem = curr_ua->remains();
                        if (rem < min_duration)
                        {
                            real_ua = ua_dots[i];
                            min_duration = rem;
                        }
                    }
                    real_ua->target = s->target;
                    real_ua->schedule_execute();
                }
            }
            virtual void execute() override {
                warlock_spell_t::execute();
                bool flag = false;
                for (int i = 0; i < MAX_UAS; i++) {
                    if (td(target)->dots_unstable_affliction[i]->is_ticking()) {
                        flag = true;
                        break;
                    }
                }

               if (p()->sets->has_set_bonus(WARLOCK_AFFLICTION, T21, B4))
                    p()->active.tormented_agony->schedule_execute();
            }
        };
        // AOE
        struct seed_of_corruption_t : public warlock_spell_t {
            struct seed_of_corruption_aoe_t : public warlock_spell_t {
                seed_of_corruption_aoe_t(warlock_t* p) : warlock_spell_t("seed_of_corruption_aoe", p, p -> find_spell(27285)) {
                    aoe = -1;
                    dual = true;
                    background = true;
                    affected_by_deaths_embrace = true;
                    p->spells.seed_of_corruption_aoe = this;
                }

                double action_multiplier() const override {
                    double m = warlock_spell_t::action_multiplier();
                    return m;
                }
                void impact(action_state_t* s) override {
                    warlock_spell_t::impact(s);
                    if (result_is_hit(s->result)) {
                        warlock_td_t* tdata = td(s->target);
                        if (tdata->dots_seed_of_corruption->is_ticking() && tdata->soc_threshold > 0) {
                            tdata->soc_threshold = 0;
                            tdata->dots_seed_of_corruption->cancel();
                        }
                    }
                }
            };

            double threshold_mod;
            double sow_the_seeds_targets;
            seed_of_corruption_aoe_t* explosion;

            seed_of_corruption_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("seed_of_corruption", p, p -> find_spell(27243)), explosion(new seed_of_corruption_aoe_t(p)) {
                parse_options(options_str);
                may_crit = false;
                threshold_mod = 3.0;
                base_tick_time = dot_duration;
                hasted_ticks = false;
                sow_the_seeds_targets = p->talents.sow_the_seeds->effectN(1).base_value();
                add_child(explosion);
            }

            void init() override {
                warlock_spell_t::init();
                snapshot_flags |= STATE_SP;
            }
            void execute() override {
                if (p()->talents.sow_the_seeds->ok()) {
                    aoe = 2;
                }
                if (p()->sets->has_set_bonus(WARLOCK_AFFLICTION, T21, B4))
                    p()->active.tormented_agony->schedule_execute();

                warlock_spell_t::execute();
            }
            void impact(action_state_t* s) override {
                if (result_is_hit(s->result)) {
                    td(s->target)->soc_threshold = s->composite_spell_power();
                }

                warlock_spell_t::impact(s);
            }
            void last_tick(dot_t* d) override {
                warlock_spell_t::last_tick(d);
                explosion->target = d->target;
                explosion->execute();
            }
        };
        // Talents
        // lvl 15 - shadow embrace|haunt|deathbolt
        struct haunt_t : public warlock_spell_t {
          haunt_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("haunt", p, p -> talents.haunt) {
            parse_options(options_str);
          }
          void impact(action_state_t* s) override {
              warlock_spell_t::impact(s);
              if (result_is_hit(s->result)) {
                  td(s->target)->debuffs_haunt->trigger();
              }
          }
        };
        struct deathbolt_t : public warlock_spell_t {
          timespan_t ac_max;

          deathbolt_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("deathbolt", p, p -> talents.deathbolt) {
            parse_options(options_str);
            ac_max = timespan_t::from_seconds(data().effectN(3).base_value());
          }

          void init() override {
            warlock_spell_t::init();
            snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;
          }

          void execute() override {
            warlock_td_t* td = this->td(target);

            double tick_base_damage;
            unsigned ticks_left;

            td->dots_agony->current_action->calculate_tick_amount(td->dots_agony->state, td->dots_agony->current_stack());
            td->dots_corruption->current_action->calculate_tick_amount(td->dots_corruption->state, td->dots_corruption->current_stack());
            for (int i = 0; i < MAX_UAS; i++) {
              if (td->dots_unstable_affliction[i]->is_ticking()) {
                td->dots_unstable_affliction[i]->current_action->calculate_tick_amount(td->dots_unstable_affliction[i]->state, td->dots_unstable_affliction[i]->current_stack());
              }
            }

            tick_base_damage = td->dots_agony->state->result_raw;
            ticks_left = td->dots_agony->ticks_left();
            double total_damage_agony = ticks_left * tick_base_damage;
            if (sim->debug) {
              sim->out_debug.printf("%s agony dot_remains=%.3f duration=%.3f ticks_left=%u amount=%.3f total=%.3f",
                name(), td->dots_agony->remains().total_seconds(),
                td->dots_agony->duration().total_seconds(),
                td->dots_agony->ticks_left(), ( tick_base_damage * td->agony_stack ), total_damage_agony );
            }

            tick_base_damage = td->dots_corruption->state->result_raw;
            ticks_left = td->dots_corruption->remains() / td->dots_corruption->time_to_tick;
            if (p()->talents.absolute_corruption->ok())
              ticks_left = ac_max / td->dots_corruption->time_to_tick;
            double total_damage_corruption = ticks_left * tick_base_damage;
            if (sim->debug) {
              sim->out_debug.printf("%s corruption dot_remains=%.3f duration=%.3f ticks_left=%u amount=%.3f total=%.3f",
                name(), td->dots_corruption->remains().total_seconds(),
                td->dots_corruption->duration().total_seconds(),
                td->dots_corruption->ticks_left(), tick_base_damage, total_damage_corruption );
            }

            double total_damage_siphon_life = 0.0;
            if ( p() -> talents.siphon_life -> ok() ) {
              tick_base_damage = td->dots_siphon_life->state->result_raw;
              ticks_left = td->dots_siphon_life->ticks_left();
              total_damage_siphon_life += ( ticks_left * tick_base_damage );
              if (sim->debug) {
                sim->out_debug.printf("%s siphon life dot_remains=%.3f duration=%.3f ticks_left=%u amount=%.3f total=%.3f",
                  name(), td->dots_siphon_life->remains().total_seconds(),
                  td->dots_siphon_life->duration().total_seconds(),
                  td->dots_siphon_life->ticks_left(), tick_base_damage, total_damage_siphon_life);
              }
            }

            double total_damage_phantom_singularity = 0.0;
            if (p()->talents.phantom_singularity->ok()) {
              tick_base_damage = td->dots_phantom_singularity->state->result_raw;
              ticks_left = td->dots_phantom_singularity->ticks_left();
              total_damage_phantom_singularity += (ticks_left * tick_base_damage);
              if (sim->debug) {
                sim->out_debug.printf("%s phantom singularity dot_remains=%.3f duration=%.3f ticks_left=%u amount=%.3f total=%.3f",
                  name(), td->dots_phantom_singularity->remains().total_seconds(),
                  td->dots_phantom_singularity->duration().total_seconds(),
                  td->dots_phantom_singularity->ticks_left(), tick_base_damage, total_damage_phantom_singularity);
              }
            }

            double total_damage_ua = 0.0;
            for (int i = 0; i < MAX_UAS; i++) {
              if (td->dots_unstable_affliction[i]->is_ticking()) {
                tick_base_damage = td->dots_unstable_affliction[i]->state->result_raw;
                ticks_left = td->dots_unstable_affliction[i]->ticks_left();
                double current_ua_total = ticks_left * tick_base_damage;
                total_damage_ua = total_damage_ua + current_ua_total;
                if (sim->debug) {
                  sim->out_debug.printf("%s ua dot_remains=%.3f duration=%.3f ticks_left=%u amount=%.3f total=%.3f",
                    name(), td->dots_unstable_affliction[i]->remains().total_seconds(),
                    td->dots_unstable_affliction[i]->duration().total_seconds(),
                    td->dots_unstable_affliction[i]->ticks_left(), tick_base_damage, current_ua_total);
                }
              }
            }
            const double total_dot_dmg = total_damage_agony + total_damage_corruption + total_damage_siphon_life + total_damage_phantom_singularity + total_damage_ua;
            this->base_dd_min = this->base_dd_max = ( total_dot_dmg * data().effectN(2).percent() );
            if (sim->debug) {
              sim->out_debug.printf("%s deathbolt damage_remaining=%.3f", name(), total_dot_dmg);
            }
            warlock_spell_t::execute();
          }
        };
        // lvl 30 - writhe|ac|deaths embrace
        // lvl 45 - demon skin|burning rush|dark pact
        // lvl 60 - sow the seeds|phantom singularity|soul harvest
        struct phantom_singularity_tick_t : public warlock_spell_t {
            phantom_singularity_tick_t(warlock_t* p) : warlock_spell_t("phantom_singularity_tick", p, p -> find_spell(205246)) {
                background = true;
                may_miss = false;
                dual = true;
                affected_by_deaths_embrace = true;
                aoe = -1;
            }
        };
        struct phantom_singularity_t : public warlock_spell_t {
            phantom_singularity_tick_t* phantom_singularity;

            phantom_singularity_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("phantom_singularity", p, p -> talents.phantom_singularity) {
                parse_options(options_str);
                callbacks = false;
                hasted_ticks = true;
                phantom_singularity = new phantom_singularity_tick_t(p);
                add_child(phantom_singularity);
            }

            timespan_t composite_dot_duration(const action_state_t* s) const override {
                return s->action->tick_time(s) * 8.0;
            }

            void tick(dot_t* d) override {
                phantom_singularity->execute();
                warlock_spell_t::tick(d);
            }
        };
        // lvl 75 - darkfury|mortal coil|demonic circle
        // lvl 90 - nightfall|nightfall|grimoire of sacrifice
        // lvl 100 - soul conduit|creeping death|siphon life
        struct siphon_life_t : public warlock_spell_t {
            siphon_life_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("siphon_life", p, p -> talents.siphon_life) {
                parse_options(options_str);
                may_crit = false;
                affected_by_deaths_embrace = true;
            }

            virtual double action_multiplier() const override {
                double m = warlock_spell_t::action_multiplier();
                if (p()->mastery_spells.potent_afflictions->ok())
                    m *= 1.0 + p()->cache.mastery_value();
                return m;
            }
            double composite_crit_chance() const override {
                double cc = warlock_spell_t::composite_crit_chance();
                return cc;
            }
            virtual double composite_target_multiplier(player_t* target) const override {
                double m = warlock_spell_t::composite_target_multiplier(target);
                warlock_td_t* td = this->td(target);

                if (td->debuffs_tormented_agony->check())
                    m *= 1.0 + td->debuffs_tormented_agony->data().effectN(1).percent();

                return m;
            }
        };
        // Buffs
        struct soul_harvest_t : public warlock_spell_t {
            int agony_action_id;
            timespan_t base_duration;
            timespan_t total_duration;
            timespan_t time_per_agony;
            timespan_t max_duration;

            soul_harvest_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("soul_harvest", p, p -> talents.soul_harvest) {
                parse_options(options_str);
                harmful = may_crit = may_miss = false;
                base_duration = data().duration();
                time_per_agony = timespan_t::from_seconds(data().effectN(2).base_value());
                max_duration = timespan_t::from_seconds(data().effectN(3).base_value());
            }

            virtual void execute() override {
                warlock_spell_t::execute();
                p()->buffs.soul_harvest->expire(); //Potentially bugged check when live

                total_duration = base_duration + time_per_agony * p()->get_active_dots(agony_action_id);
                p()->buffs.soul_harvest->trigger(1, buff_t::DEFAULT_VALUE(), 1.0, std::min(total_duration, max_duration));
            }

            virtual void init() override {
                warlock_spell_t::init();
                agony_action_id = p()->find_action_id("agony");
            }
        };
        // Summoning

        // Tier
        struct tormented_agony_t : public warlock_spell_t {
            struct tormented_agony_debuff_engine_t : public warlock_spell_t {
                tormented_agony_debuff_engine_t(warlock_t* p) : warlock_spell_t("tormented agony", p, p -> find_spell(256807)) {
                    harmful = may_crit = callbacks = false;
                    background = proc = true;
                    aoe = 0;
                    trigger_gcd = timespan_t::zero();
                }
                virtual void impact(action_state_t* s) override {
                    warlock_spell_t::impact(s);
                    td(s->target)->debuffs_tormented_agony->trigger();
                }
            };

            propagate_const<player_t*> source_target;
            tormented_agony_debuff_engine_t* tormented_agony;

            tormented_agony_t(warlock_t* p) : warlock_spell_t("tormented agony", p, p -> find_spell(256807)),source_target(nullptr),tormented_agony(new tormented_agony_debuff_engine_t(p)) {
                harmful = may_crit = callbacks = false;
                background = proc = true;
                aoe = -1;
                radius = data().effectN(1).radius();
                trigger_gcd = timespan_t::zero();
            }

            void execute() override {
                warlock_spell_t::execute();
                for (const auto target : sim->target_non_sleeping_list) {
                    if (td(target)->dots_agony->is_ticking()) {
                        tormented_agony->set_target(target);
                        tormented_agony->execute();
                    }
                }
            }
        };
    } // end actions namespace

    namespace buffs {
        struct debuff_agony_t : public warlock_buff_t < buff_t >{
            debuff_agony_t(warlock_td_t& p) : base_t(p, buff_creator_t(static_cast<actor_pair_t>(p), "agony", p.source -> find_spell(980))) { }

            void expire_override(int expiration_stacks, timespan_t remaining_duration) override {
                base_t::expire_override(expiration_stacks, remaining_duration);
            }
        };
    } // end buffs namespace

    // add actions
    action_t* warlock_t::create_action_affliction(const std::string& action_name,const std::string& options_str) {
        using namespace actions;

        if (action_name == "corruption") return new                     corruption_t(this, options_str);
        if (action_name == "agony") return new                          agony_t(this, options_str);
        if (action_name == "unstable_affliction") return new            unstable_affliction_t(this, options_str);
        // aoe
        if (action_name == "seed_of_corruption") return new             seed_of_corruption_t(this, options_str);
        // talents
        if (action_name == "haunt") return new                          haunt_t(this, options_str);
        if (action_name == "deathbolt") return new                      deathbolt_t(this, options_str);
        if (action_name == "phantom_singularity") return new            phantom_singularity_t(this, options_str);
        if (action_name == "siphon_life") return new                    siphon_life_t(this, options_str);
        if (action_name == "soul_harvest") return new                   soul_harvest_t(this, options_str);

        return nullptr;
    }
    void warlock_t::create_buffs_affliction() {
        //spells
        buffs.active_uas = buff_creator_t(this, "active_uas")
            .tick_behavior(BUFF_TICK_NONE)
            .refresh_behavior(BUFF_REFRESH_DURATION)
            .max_stack(20);
        //talents
        buffs.soul_harvest = buff_creator_t(this, "soul_harvest", find_spell(196098))
            .add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER)
            .refresh_behavior(BUFF_REFRESH_EXTEND)
            .cd(timespan_t::zero())
            .default_value(find_spell(196098)->effectN(1).percent());
        buffs.nightfall = buff_creator_t(this, "nightfall", find_spell(264571))
          .default_value(find_spell(264571)->effectN(2).percent())
          .refresh_behavior(BUFF_REFRESH_DURATION)
          .add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER);
        //tier
        buffs.demonic_speed = haste_buff_creator_t(this, "demonic_speed", sets->set(WARLOCK_AFFLICTION, T20, B4)->effectN(1).trigger())
            .chance(sets->set(WARLOCK_AFFLICTION, T20, B4)->proc_chance())
            .default_value(sets->set(WARLOCK_AFFLICTION, T20, B4)->effectN(1).trigger()->effectN(1).percent());
        //legendary
    }
    void warlock_t::init_spells_affliction(){
        // General
        spec.affliction                     = find_specialization_spell(137043);
        mastery_spells.potent_afflictions   = find_mastery_spell(WARLOCK_AFFLICTION);
        // Specialization Spells
        spec.unstable_affliction            = find_specialization_spell("Unstable Affliction");
        spec.agony                          = find_specialization_spell("Agony");
        // Talents
        talents.shadow_embrace              = find_talent_spell("Shadow Embrace");
        talents.haunt                       = find_talent_spell("Haunt");
        talents.deathbolt                   = find_talent_spell("Deathbolt");
        talents.writhe_in_agony             = find_talent_spell("Writhe in Agony");
        talents.absolute_corruption         = find_talent_spell("Absolute Corruption");
        talents.deaths_embrace              = find_talent_spell("Death's Embrace");
        talents.sow_the_seeds               = find_talent_spell("Sow the Seeds");
        talents.phantom_singularity         = find_talent_spell("Phantom Singularity");
        talents.soul_harvest                = find_talent_spell("Soul Harvest");
        talents.nightfall                   = find_talent_spell("Nightfall");
        talents.creeping_death              = find_talent_spell("Creeping Death");
        talents.siphon_life                 = find_talent_spell("Siphon Life");
        // Tier
        active.tormented_agony              = new actions::tormented_agony_t(this);

        // seed applies corruption
        if (specialization() == WARLOCK_AFFLICTION) {
            active.corruption = new actions::corruption_t(this);
            active.corruption->background = true;
            active.corruption->aoe = -1;
        }
    }
    void warlock_t::init_gains_affliction() {
        gains.agony = get_gain("agony");
        gains.seed_of_corruption = get_gain("seed_of_corruption");
        gains.unstable_affliction_refund = get_gain("unstable_affliction_refund");
        gains.affliction_t20_2pc = get_gain("affliction_t20_2pc");
    }
    void warlock_t::init_rng_affliction() {
        affliction_t20_2pc_rppm = get_rppm("affliction_t20_2pc", sets->set(WARLOCK_AFFLICTION, T20, B2));
        nightfall_rppm = get_rppm("nightfall", talents.nightfall);
    }
    void warlock_t::init_procs_affliction() {
        procs.the_master_harvester = get_proc("the_master_harvester");
        procs.affliction_t21_2pc = get_proc("affliction_t21_2pc");
        procs.nightfall = get_proc("nightfall");
    }
    void warlock_t::create_options_affliction() {
        add_option(opt_bool("deaths_embrace_fixed_time", deaths_embrace_fixed_time));
    }

    void warlock_t::create_apl_affliction() {
        action_priority_list_t* def = get_action_priority_list("default");

        def -> add_action("soul_harvest,if=buff.active_uas.stack>0");
        def -> add_action("haunt");
        def -> add_action("agony,if=refreshable");
        def -> add_action("siphon_life,if=refreshable");
        def -> add_action("corruption,if=refreshable");
        def -> add_action("phantom_singularity");
        def -> add_action("unstable_affliction,if=soul_shard=5");
        def -> add_action("unstable_affliction,if=(dot.unstable_affliction_1.ticking+dot.unstable_affliction_2.ticking+dot.unstable_affliction_3.ticking+dot.unstable_affliction_4.ticking+dot.unstable_affliction_5.ticking=0)|soul_shard>2");
        def -> add_action("deathbolt");
        def -> add_action("shadow_bolt");
    }

    using namespace unique_gear;
    using namespace actions;
    struct hood_of_eternal_disdain_t : public scoped_action_callback_t<agony_t>
    {
        hood_of_eternal_disdain_t() : super(WARLOCK, "agony")
        {}

        void manipulate(agony_t* a, const special_effect_t& e) override
        {
            a->base_tick_time *= 1.0 + e.driver()->effectN(2).percent();
            a->dot_duration *= 1.0 + e.driver()->effectN(1).percent();
        }
    };
    struct sacrolashs_dark_strike_t : public scoped_action_callback_t<corruption_t>
    {
        sacrolashs_dark_strike_t() : super(WARLOCK, "corruption")
        {}

        void manipulate(corruption_t* a, const special_effect_t& e) override
        {
            a->base_multiplier *= 1.0 + e.driver()->effectN(1).percent();
        }
    };

    void warlock_t::legendaries_affliction() {
        register_special_effect(205797, hood_of_eternal_disdain_t());
    }
}
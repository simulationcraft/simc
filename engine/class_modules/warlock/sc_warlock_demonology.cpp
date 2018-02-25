#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
    namespace pets {
        namespace felguard {
            struct legion_strike_t : public warlock_pet_melee_attack_t {
                legion_strike_t(warlock_pet_t* p) : warlock_pet_melee_attack_t(p, "Legion Strike") {
                    aoe = -1;
                    weapon = &(p->main_hand_weapon);
                }
                virtual bool ready() override {
                    if (p()->special_action->get_dot()->is_ticking()) return false;
                    return warlock_pet_melee_attack_t::ready();
                }
            };
            struct axe_toss_t : public warlock_pet_spell_t {
                axe_toss_t(warlock_pet_t* p) : warlock_pet_spell_t("Axe Toss", p, p -> find_spell(89766)) {
                }

                void execute() override {
                    warlock_pet_spell_t::execute();
                    p()->trigger_sephuzs_secret(execute_state, MECHANIC_STUN);
                }
            };
            struct felstorm_tick_t : public warlock_pet_melee_attack_t {
                felstorm_tick_t(warlock_pet_t* p, const spell_data_t& s) : warlock_pet_melee_attack_t("felstorm_tick", p, s.effectN(1).trigger()) {
                    aoe = -1;
                    background = true;
                    weapon = &(p->main_hand_weapon);
                }
            };
            struct felstorm_t : public warlock_pet_melee_attack_t {
                felstorm_t(warlock_pet_t* p) : warlock_pet_melee_attack_t("felstorm", p, p -> find_spell(89751)) {
                    tick_zero = true;
                    hasted_ticks = false;
                    may_miss = false;
                    may_crit = false;
                    weapon_multiplier = 0;

                    dynamic_tick_action = true;
                    tick_action = new felstorm_tick_t(p, data());
                }

                virtual void cancel() override {
                    warlock_pet_melee_attack_t::cancel();
                    get_dot()->cancel();
                }

                virtual void execute() override {
                    warlock_pet_melee_attack_t::execute();
                    p()->melee_attack->cancel();
                }

                virtual void last_tick(dot_t* d) override {
                    warlock_pet_melee_attack_t::last_tick(d);
                    if (!p()->is_sleeping() && !p()->melee_attack->target->is_sleeping())
                        p()->melee_attack->execute();
                }
            };

            felguard_pet_t::felguard_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
                warlock_pet_t(sim, owner, name, PET_FELGUARD, name != "felguard") {
                action_list_str += "/felstorm";
                action_list_str += "/legion_strike,if=cooldown.felstorm.remains";
                owner_coeff.ap_from_sp = 1.1; // HOTFIX
                owner_coeff.ap_from_sp *= 1.2; // PTR
            }

            void felguard_pet_t::init_base_stats() {
                warlock_pet_t::init_base_stats();

                melee_attack = new warlock_pet_melee_t(this);
                special_action = new felstorm_t(this);
                special_action_two = new axe_toss_t(this);
            }

            action_t* felguard_pet_t::create_action(const std::string& name, const std::string& options_str) {
                if (name == "legion_strike") return new legion_strike_t(this);
                if (name == "felstorm") return new felstorm_t(this);
                if (name == "axe_toss") return new axe_toss_t(this);

                return warlock_pet_t::create_action(name, options_str);
            }
        }
    }
    namespace actions_demonology {
      using namespace actions;

      struct shadow_bolt_t : public warlock_spell_t
      {
        shadow_bolt_t(warlock_t* p, const std::string& options_str) :
          warlock_spell_t(p, "Shadow Bolt", p->specialization())
        {
          parse_options(options_str);
          energize_type = ENERGIZE_ON_CAST;
          energize_resource = RESOURCE_SOUL_SHARD;
          energize_amount = 1;
        }
      };

      struct hand_of_guldan_t : public warlock_spell_t {
            /*
            struct trigger_imp_event_t : public player_event_t {
            bool initiator;
            int count;
            trigger_imp_event_t(warlock_t* p, int c, bool init = false) :
            player_event_t(*p, timespan_t::from_millis(1)), initiator(init), count(c) {

            }

            virtual const char* name() const override {
            return  "trigger_imp";
            }

            virtual void execute() override {
            warlock_t* p = static_cast< warlock_t* >(player());
            }
            };
            */

            //trigger_imp_event_t* imp_event;
            int shards_used;

            hand_of_guldan_t(warlock_t* p, const std::string& options_str) : warlock_spell_t(p, "Hand of Gul'dan") {
                parse_options(options_str);
                aoe = -1;
                shards_used = 0;

                parse_effect_data(p->find_spell(86040)->effectN(1));
            }

            virtual timespan_t travel_time() const override {
                return timespan_t::from_millis(700);
            }

            virtual bool ready() override {
                bool r = warlock_spell_t::ready();
                if (p()->resources.current[RESOURCE_SOUL_SHARD] == 0.0)
                    r = false;

                return r;
            }

            void consume_resource() override {
                warlock_spell_t::consume_resource();

                shards_used = last_resource_cost;

                if (last_resource_cost == 1.0)
                    p()->procs.one_shard_hog->occur();
                if (last_resource_cost == 2.0)
                    p()->procs.two_shard_hog->occur();
                if (last_resource_cost == 3.0)
                    p()->procs.three_shard_hog->occur();
                if (last_resource_cost == 4.0)
                    p()->procs.four_shard_hog->occur();
            }

            virtual void impact(action_state_t* s) override {
                warlock_spell_t::impact(s);

                if (result_is_hit(s->result)) {
                    /*
                    if (s->chain_target == 0)
                    imp_event = make_event<trigger_imp_event_t>(*sim, p(), floor(shards_used), true);
                    */
                    if (p()->sets->has_set_bonus(WARLOCK_DEMONOLOGY, T21, B2)) {
                        for (int i = 0; i < shards_used; i++) {
                            p()->buffs.rage_of_guldan->trigger();
                        }
                    }
                }
            }
        };

      // Talents
      struct doom_t : public warlock_spell_t {
            doom_t(warlock_t* p, const std::string& options_str) : warlock_spell_t("doom", p, p -> talents.doom) {
                parse_options(options_str);
                may_crit = true;
                hasted_ticks = true;
            }

            timespan_t composite_dot_duration(const action_state_t* s) const override {
                timespan_t duration = warlock_spell_t::composite_dot_duration(s);
                return duration * p()->cache.spell_haste();
            }

            virtual double action_multiplier()const override {
                double m = warlock_spell_t::action_multiplier();
                double pet_counter = 0.0;

                return m;
            }

            virtual void tick(dot_t* d) override {
                warlock_spell_t::tick(d);

                if (d->state->result == RESULT_HIT || result_is_hit(d->state->result)) {
                    if (p()->sets->has_set_bonus(WARLOCK_DEMONOLOGY, T19, B2) && rng().roll(p()->sets->set(WARLOCK_DEMONOLOGY, T19, B2)->effectN(1).percent()))
                        p()->resource_gain(RESOURCE_SOUL_SHARD, 1, p()->gains.t19_2pc_demonology);
                }
            }
        };

      struct grimoire_of_service_t : public summon_pet_t {
            grimoire_of_service_t(warlock_t* p, const std::string& pet_name, const std::string& options_str) :
                summon_pet_t("service_" + pet_name, p, p -> talents.grimoire_of_service -> ok() ? p -> find_class_spell("Grimoire: " + pet_name) : spell_data_t::not_found()) {
                parse_options(options_str);
                cooldown = p->get_cooldown("grimoire_of_service");
                cooldown->duration = data().cooldown();
                summoning_duration = data().duration() + timespan_t::from_millis(1);
            }
            virtual void execute() override {
                pet->is_grimoire_of_service = true;
                summon_pet_t::execute();
            }
            bool init_finished() override {
                if (pet) {
                    pet->summon_stats = stats;
                }
                return summon_pet_t::init_finished();
            }
        };
    } // end actions namespace
    namespace buffs {

    } // end buffs namespace

      // add actions
    action_t* warlock_t::create_action_demonology(const std::string& action_name, const std::string& options_str) {
        using namespace actions_demonology;

        if (action_name == "shadow_bolt") return new          shadow_bolt_t(this, options_str);
        if (action_name == "doom")          return new        doom_t(this, options_str);
        if (action_name == "hand_of_guldan") return new       hand_of_guldan_t(this, options_str);

        if (action_name == "summon_felguard") return new      summon_main_pet_t("felguard", this);
        if (action_name == "service_felguard") return new     grimoire_of_service_t(this, "felguard", options_str);
        if (action_name == "service_felhunter") return new    grimoire_of_service_t(this, "felhunter", options_str);
        if (action_name == "service_imp") return new          grimoire_of_service_t(this, "imp", options_str);
        if (action_name == "service_succubus") return new     grimoire_of_service_t(this, "succubus", options_str);
        if (action_name == "service_voidwalker") return new   grimoire_of_service_t(this, "voidwalker", options_str);

        return nullptr;
    }

    void warlock_t::create_buffs_demonology() {

    }

    void warlock_t::init_spells_demonology() {
        spec.demonology                         = find_specialization_spell(137044);
        mastery_spells.master_demonologist      = find_mastery_spell(WARLOCK_DEMONOLOGY);
        // spells
        // Talents
        talents.demonic_strength                = find_talent_spell("Demonic Strength");
        talents.demonic_calling                 = find_talent_spell("Demonic Calling");
        talents.doom                            = find_talent_spell("Doom");
        talents.riders                          = find_talent_spell("Riders");
        talents.power_siphon                    = find_talent_spell("Power Siphon");
        talents.summon_vilefiend                = find_talent_spell("Summon Vilefiend");
        talents.overloaded                      = find_talent_spell("Overloaded");
        talents.demonic_strength                = find_talent_spell("Demonic Strength");
        talents.biliescourge_bombers            = find_talent_spell("Biliescourge Bombers");
        talents.grimoire_of_synergy             = find_talent_spell("Grimoire of Synergy");
        talents.demonic_consumption             = find_talent_spell("Demonic Consumption");
        talents.grimoire_of_service             = find_talent_spell("Grimoire of Service");
        talents.inner_demons                    = find_talent_spell("Inner Demons");
        talents.nether_portal                   = find_talent_spell("Nether Portal");
    }

    void warlock_t::init_gains_demonology() {

    }

    void warlock_t::init_rng_demonology() {
    }

    void warlock_t::init_procs_demonology() {

    }

    void warlock_t::create_options_demonology() {
    }

    void warlock_t::create_apl_demonology() {
        action_priority_list_t* def = get_action_priority_list("default");

        def -> add_action("shadow_bolt");
    }

    using namespace unique_gear;
    using namespace actions;

    void warlock_t::legendaries_demonology() {

    }
}
